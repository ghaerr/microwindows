/* 
A desktop environment with a Windows 95 like start-up menu. 
It specifically targets slow 8086/8088 such as Amstrad 1640 at 8Mhz. 

Created by: Anton Andreev

Online help: https://github.com/toncho11/microwindows/wiki

History
   Version 1.1, A. Andreev:
   - added nxmsg, Help and About menu entries, code improvements, fixes
   - added -s option for slow systems 
   Version 1.0, A. Andreev:
   - event handling of a startup menu with both commands and applications
   - auto updates for free/total conventional memory in task-bar and clock display
   - support for viewing images in 3 modes with nxjpeg (subproject)
   - support for file selection with nxselect (subproject)
   - support for editing files using: nxselect -> nxterm -> edit
   - a list of nx apps to launch: Calculator, Clock ...
   - commands: Restart, Exit, Disk sync

Notes: 
    Rules to follow on slow 8086/8088 systems:
     - Only draw in EXPOSE (or via a single scheduled redraw triggered by EXPOSE).
     - Never draw directly in event handlers—change state and request a redraw instead.
     - Keep drawing ordered and centralized so UI elements repaint consistently.
     - Avoid rapid redraw loops—use timeouts only for scheduling, not rendering.
     - Do not draw until the first EXPOSE arrives (after GrMapWindow).
*/
 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "nano-X.h"
#include "nxcolors.h"
#include <linuxmt/mem.h> 
#include <sys/ioctl.h>
#include <sys/select.h>

#define TEXT_Y_OFFSET_MENU     10
#define TEXT_Y_OFFSET_TASKBAR  15


/* ===== UI CONSTANTS ===== */
#define TASKBAR_HEIGHT        24
#define START_WIDTH           40
#define CLOCK_WIDTH           40
#define MEMORY_WIDTH          80
#define STATUS_WIDTH          (CLOCK_WIDTH + MEMORY_WIDTH)

#define MENU_WIDTH            100
#define MENU_ITEM_HEIGHT      18
#define MENU_ITEM_EXIT_HEIGHT 20

/* ===== CONFIG ===== */
#define GR_EVENT_TIMEOUT      70
#define APP_PATH "/bin/"
static int is_slow_system = 0;

const char *apps[] =
    { "About", "Help", "Calculator","Clock","Mine","Tetris","World map zoom","Terminal","View jpg as 16c", "View jpg as 8c", "View jpg as 4c", "Edit file"};
	
const char *sys[] =
    { "Exit to terminal", "Restart computer","Sync disk"};

#define APP_COUNT (sizeof(apps)/sizeof(apps[0]))
#define SYS_COUNT (sizeof(sys)/sizeof(sys[0]))

/* ===== GLOBALS ===== */
static GR_WINDOW_ID win;
static GR_GC_ID gc_bar, gc_text;
static int width, height;
static int menu_open = 0;
static FILE *nx_fp = NULL;
static int nx_fd = -1;
static int nxselect_running = 0;
static int nxmsg_running = 0;
char buf[80];
static int response_received = 0;
static int image_view_requested = 0;
static int edit_file_requested = 0;
static int message_box_requested = 0;
static int image_view_color_mode = 16;

typedef void (*nx_modal_cb_t)(const char *result);
static nx_modal_cb_t nxmsg_cb = NULL; /* pointer to a function that handles user's response */
static nx_modal_cb_t nxselect_cb = NULL;

static unsigned int mem_free = 0;
static unsigned int mem_total = 0;
static int mem_valid = 0;

/* ===== EXIT HELPERS =====*/
static void exitwait(void)
{
    signal(SIGCHLD, SIG_IGN);
    GrClose();
    /*      
     * Wait for all children to exit before we do.
     * This prevents the shell we return to from reading
     * simultaneously with nxterm or another NX app and
     * causing bad behaviour.
     */
    while (waitpid(-1, NULL, 0) != -1)
        continue;
    exit(0);
}

/* handle SIGTERM by sending SIGTERM to all children, then wait for them to exit, then exit */
static void sigterm(int sig)
{
    signal(SIGTERM, SIG_IGN);
    kill(-getpid(), SIGTERM);
    exitwait();
}  

/* ===== CHILD REAPER ===== */
static void reaper(int s) 
{
    signal(SIGCHLD, reaper);
    while(waitpid(-1, NULL, WNOHANG) > 0)
        continue;
}

/* ===== DRAWING HELPERS ===== */
static void draw3drect(int x,int y,int w,int h,int raised)
{
    GrSetGCForeground(gc_bar,
         raised ? GrGetSysColor(GR_COLOR_WINDOW)
                : GrGetSysColor(GR_COLOR_BTNSHADOW));
    GrLine(win,gc_bar,x,y,x+w-1,y);
    GrLine(win,gc_bar,x,y,x,y+h-1);

    GrSetGCForeground(gc_bar,
         raised ? GrGetSysColor(GR_COLOR_BTNSHADOW)
                : GrGetSysColor(GR_COLOR_WINDOW));
    GrLine(win,gc_bar,x,y+h-1,x+w-1,y+h-1);
    GrLine(win,gc_bar,x+w-1,y,x+w-1,y+h-1);
}

/* ===== TASKBAR ===== */
static void draw_taskbar(void)
{
    /* Background */
    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,0,height-TASKBAR_HEIGHT,width,TASKBAR_HEIGHT);

    /* 3D border */
    //draw3drect(0,height-TASKBAR_HEIGHT,width,4,1);

    /* Start button area */
    GrFillRect(win,gc_bar,0,height-TASKBAR_HEIGHT,START_WIDTH,TASKBAR_HEIGHT);
    draw3drect(0,height-TASKBAR_HEIGHT,START_WIDTH,TASKBAR_HEIGHT,1);

    /* Start text */
    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,
           8,
           height - TASKBAR_HEIGHT + TEXT_Y_OFFSET_TASKBAR,
           (void *)"Start",5,GR_TFASCII);
}


/* ===== MENU ===== */
static void draw_menu(void)
{
    int mx = 0;
    int menu_h = (APP_COUNT * MENU_ITEM_HEIGHT) +
             (SYS_COUNT * MENU_ITEM_EXIT_HEIGHT) + 4;

    int my = height - TASKBAR_HEIGHT - menu_h;

    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win, gc_bar, mx, my, MENU_WIDTH, menu_h);
    draw3drect(mx, my, MENU_WIDTH, menu_h, 1);

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));

    int y = my + 4 + TEXT_Y_OFFSET_MENU;

    /* Applications */
    for (int i = 0; i < APP_COUNT; i++) {
        GrText(win, gc_text,
               4, y,
               (void *)apps[i],
               strlen(apps[i]),
               GR_TFASCII);
        y += MENU_ITEM_HEIGHT;
    }

    /* Separator line */
    GrLine(win, gc_text,
           mx + 2,
           y - TEXT_Y_OFFSET_MENU,
           mx + MENU_WIDTH - 2,
           y - TEXT_Y_OFFSET_MENU);

    y += 4;

    /* System commands (Exit, Restart, Sync & Halt) */
    for (int i = 0; i < SYS_COUNT; i++) {
        GrText(win, gc_text,
               4, y,
               (void *)sys[i],
               strlen(sys[i]),
               GR_TFASCII);
        y += MENU_ITEM_EXIT_HEIGHT;
    }
}

/* ===== RECT CHECK ===== */
static int in_rect(int x,int y,int rx,int ry,int rw,int rh)
{
    return (x>=rx && x<rx+rw && y>=ry && y<ry+rh);
}

/* ===== MEMORY UPDATE ===== */
static void update_memory_now(void)
{
    static int fd = -1;
    struct mem_usage mu;

    if (fd < 0) {
        fd = open("/dev/kmem", O_RDONLY);
        if (fd < 0) {
            mem_valid = 0;
            return;
        }
    }

    /* Directly query the kernel for memory stats */
    if (ioctl(fd, MEM_GETUSAGE, &mu) == 0) {
        mem_total = mu.main_used + mu.main_free;
        mem_free  = mu.main_free;
        mem_valid = 1;
    } else {
        mem_valid = 0;
    }
}

/* ===== CLOCK + MEMORY STATUS FIELD ===== */
static void draw_status_field(void)
{
    int x = width - STATUS_WIDTH;
    int y = height - TASKBAR_HEIGHT;

    /* Background SAME AS TASKBAR */
    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,x,y,STATUS_WIDTH,TASKBAR_HEIGHT);

    /* Build combined text */
    char buf[64];

	if (!is_slow_system)
		if(mem_valid)
			snprintf(buf,sizeof(buf),"%u / %u KB  ", mem_free, mem_total);
		else
			snprintf(buf,sizeof(buf),"-- / -- KB  ");
	else
		buf[0] = '\0';

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char clk[16];
    snprintf(clk,sizeof(clk),"%02d : %02d",tm->tm_hour,tm->tm_min);
    strcat(buf,clk);

    /* Draw transparent text aligned properly */
	int text_x = x + 6;
	if (is_slow_system) {
		text_x += 70;   
	}
    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,
           text_x,
           y + TEXT_Y_OFFSET_TASKBAR,
           (void *)buf,strlen(buf),GR_TFASCII);
}

/* ===== CENTRAL REDRAW ===== */
static void redraw_all(void)
{
    draw_taskbar();
    if(menu_open)
        draw_menu();
    draw_status_field();
}

void poll_for_nxselect_result(void)
{
    fd_set set;
    struct timeval tv;

    FD_ZERO(&set);
    FD_SET(nx_fd, &set);

    tv.tv_sec = 0;   /* Non-blocking */
    tv.tv_usec = 0;

    int rv = select(nx_fd + 1, &set, NULL, NULL, &tv);

    if (rv > 0 && FD_ISSET(nx_fd, &set)) {

        /* Read entire line (non-blocking because select says ready) */
        if (fgets(buf, sizeof(buf), nx_fp)) {
            buf[strcspn(buf, "\r\n")] = '\0';
			response_received = 1;
        } else {
            //printf("app closed (no path)\n");
        }

        pclose(nx_fp);
        nx_fp = NULL;
        nx_fd = -1;
        nxselect_running = 0;   /* stop polling */
    }
}

void poll_for_nxmsg_result(void) /* TODO: merge with above function? nx_fp and nx_fd are shared*/
{
    fd_set set;
    struct timeval tv;

    FD_ZERO(&set);
    FD_SET(nx_fd, &set);

    tv.tv_sec = 0;   /* Non-blocking */
    tv.tv_usec = 0;

    int rv = select(nx_fd + 1, &set, NULL, NULL, &tv);

    if (rv > 0 && FD_ISSET(nx_fd, &set)) {

        /* Read entire line (non-blocking because select says ready) */
        if (fgets(buf, sizeof(buf), nx_fp)) {
            buf[strcspn(buf, "\r\n")] = '\0';
			response_received = 1;
        } else {
            //printf("app closed (no path)\n");
        }

        pclose(nx_fp);
        nx_fp = NULL;
        nx_fd = -1;
        nxmsg_running = 0;   /* stop polling */
    }
}

/* text text_align: 0 is center, 1 is left, 2 is right */ 
int message_box(const char *title, const char *text, int text_align, nx_modal_cb_t cb)
{
    if (nxmsg_running) {
        return -1;
    }

    char cmd[340];

    snprintf(cmd, sizeof(cmd),
             "nxmsg -ta %d \"%s\" \"%s\"",
             text_align,
             title,
             text);

    nx_fp = popen(cmd, "r");
    if (!nx_fp)
        return -1;

    nx_fd = fileno(nx_fp);
    nxmsg_cb = cb;
    nxmsg_running = 1;

    return 0;
}

int file_select(nx_modal_cb_t cb)
{
    if (nxselect_running)
        return -1;

    nx_fp = popen("nxselect", "r");
    if (!nx_fp)
        return -1;

    nx_fd = fileno(nx_fp);
    nxselect_cb = cb;
    nxselect_running = 1;

    return 0;
}

void image_select_cb(const char *path)
{
    pid_t pid;

    if (!path || strcmp(path, "[]") == 0)
        return;

    pid = fork();
    if (pid != 0)
        return;

    if (image_view_color_mode == 4) {
        execl("/bin/nxjpeg",
              "nxjpeg",
              "-m",
              "-g",
              path,
              (char *)NULL);
    }
    else if (image_view_color_mode == 8) {
        execl("/bin/nxjpeg",
              "nxjpeg",
              "-g",
              "-8",
              path,
              (char *)NULL);
    }
    else {
        execl("/bin/nxjpeg",
              "nxjpeg",
              path,
              (char *)NULL);
    }

    _exit(1);
}

void edit_file_cb(const char *path)
{
    pid_t pid;
    char cmd[70];

    if (!path || strcmp(path, "[]") == 0)
        return;

    pid = fork();
    if (pid != 0)
        return;

    /* construct "/bin/edit <path> && exit" */
    snprintf(cmd, sizeof(cmd),
             "/bin/edit %s && exit",
             path);

    execl("/bin/nxterm",
          "nxterm",
          cmd,
          (char *)NULL);

    _exit(1);
}

static void handle_menu_click(int x, int y,
                              int *need_redraw)
{
    int mx = 0;
    int menu_h = (APP_COUNT * MENU_ITEM_HEIGHT) +
                 (SYS_COUNT * MENU_ITEM_EXIT_HEIGHT) + 4;

    int my = height - TASKBAR_HEIGHT - menu_h;

    /* ---------- Applications ---------- */
    for (int i = 0; i < APP_COUNT; i++) {

        if (in_rect(x, y,
                    mx,
                    my + (i * MENU_ITEM_HEIGHT),
                    MENU_WIDTH,
                    MENU_ITEM_HEIGHT))
        {
            char cmd[64];

            if (strncmp(apps[i], "View jpg", 8) == 0) {

                size_t len = strlen(apps[i]);

                if (len >= 2 && strcmp(apps[i] + len - 2, "8c") == 0)
					image_view_color_mode = 8;
				else if (len >= 2 && strcmp(apps[i] + len - 2, "4c") == 0)
					image_view_color_mode = 4;
				else
					image_view_color_mode = 16;

				image_view_requested = 1;
				
                if (!nxselect_running) {
                    file_select(image_select_cb);
                    }

            } else if (!strcmp(apps[i], "Edit file")) {

				edit_file_requested = 1;
				
                if (!nxselect_running) {
                    file_select(edit_file_cb);
                }

            } else if (!strcmp(apps[i], "About")) {

				message_box_requested = 1;
				message_box("About", "NXDSKTOP\nNano-X based graphical desktop environment\nDeveloped by: Anton Andreev\nVersion 1.1",0,NULL);
				
            } else if (!strcmp(apps[i], "Help")) {

				message_box_requested = 1;
				message_box("Help", "1) CTRL+A - force closes nxdsktop and Nano X\n2) Default editor: edit (mined editor from minix)\n3) CTRL+X - closes an opened file in edit\n4) Use 'Sync to disk' before powering off\n5) https://github.com/toncho11/microwindows/wiki",1,NULL);
				
            } else {

                const char *exe;

                if (!strcmp(apps[i], "Calculator"))      exe = "nxcalc";
                else if (!strcmp(apps[i], "Clock"))      exe = "nxclock";
                else if (!strcmp(apps[i], "Mine"))       exe = "nxmine";
                else if (!strcmp(apps[i], "Terminal"))   exe = "nxterm";
                else if (!strcmp(apps[i], "Tetris"))     exe = "nxtetris";
                else if (!strcmp(apps[i], "World map zoom")) exe = "nxworld";
                else                                     exe = apps[i];

                snprintf(cmd, sizeof(cmd),
                         APP_PATH "%s", exe);

                if (fork() == 0) {
                    execl(cmd, exe, NULL);
                    _exit(1);
                }
            }

            /* Close menu */
            menu_open = 0;

            GrSetGCForeground(gc_bar, GR_COLOR_LIGHTSKYBLUE);
            GrFillRect(win, gc_bar,
                       mx, my,
                       MENU_WIDTH, menu_h);

            *need_redraw = 1;
            return;
        }
    }

    /* ---------- System items ---------- */
    int sys_y = my + (APP_COUNT * MENU_ITEM_HEIGHT) + 4;

    for (int i = 0; i < SYS_COUNT; i++) {

        if (in_rect(x, y,
                    mx,
                    sys_y + (i * MENU_ITEM_EXIT_HEIGHT),
                    MENU_WIDTH,
                    MENU_ITEM_EXIT_HEIGHT))
        {
            if (!strcmp(sys[i], "Exit to terminal")) {
                sigterm(SIGTERM);
            }
            else if (!strcmp(sys[i], "Restart computer")) {
                unlink(GR_NAMED_SOCKET); /* ensures Nano X will start after PC restart */
				if (fork() == 0) {
                    execl("/bin/shutdown",
                          "shutdown",
                          "-r",
                          NULL);
                    _exit(1);
                }
            }
            else if (!strcmp(sys[i], "Sync disk")) {
				
				/* TODO: replace with shutdown and visual message */
                if (fork() == 0) {
                    execl("/bin/sync",
                          "sync",
                          NULL);
                    _exit(1);
                }
            }

            menu_open = 0;

			GrSetGCForeground(gc_bar, GR_COLOR_LIGHTSKYBLUE);
			GrFillRect(win, gc_bar,
					   mx, my,
					   MENU_WIDTH, menu_h);

			*need_redraw = 1;
			return;
        }
    }
}

/* ===== MAIN LOOP ===== */
int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            is_slow_system = 1;
            break;
        }
    }
	
    GR_EVENT ev;
    int ui_initialized = 0;
    int need_redraw = 0;
	
    signal(SIGCHLD, reaper);
    signal(SIGTERM, sigterm);
    signal(SIGHUP, SIG_IGN);
    setsid(); 

    if (GrOpen() < 0) {
        fprintf(stderr,"nxdsktop: cannot open Nano-X\n");
        return 1;
    }

    GR_SCREEN_INFO si;
    GrGetScreenInfo(&si);
    width = si.cols;
    height = si.rows;

    win = GrNewWindowEx(GR_WM_PROPS_NODECORATE,"stmenu",
            GR_ROOT_WINDOW_ID,0,0,width,height,GR_COLOR_LIGHTSKYBLUE);

    gc_bar  = GrNewGC();
    gc_text = GrNewGC();

    GrSetGCUseBackground(gc_text, GR_FALSE);

    GrSelectEvents(win,
        GR_EVENT_MASK_EXPOSURE |
        GR_EVENT_MASK_BUTTON_DOWN |
        GR_EVENT_MASK_BUTTON_UP |
        GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(win);

    /* First memory update scheduled 18 seconds later */
    time_t next_update = time(NULL) + 18;

    /* ==== EVENT LOOP ==== */
    for (;;) {

        memset(&ev, 0, sizeof(ev));
        GrGetNextEventTimeout(&ev, GR_EVENT_TIMEOUT);

        if (ev.type != 0) {
            switch (ev.type) {

            case GR_EVENT_TYPE_EXPOSURE:
                ui_initialized = 1;
                redraw_all();     /* draw immediately */
                need_redraw = 0;  /* cancel scheduled redraw */
                break;

            case GR_EVENT_TYPE_BUTTON_DOWN: {

                int mx = 0;
				int menu_h =
					(APP_COUNT * MENU_ITEM_HEIGHT) +
					(SYS_COUNT * MENU_ITEM_EXIT_HEIGHT) + 4;

                int my = height - TASKBAR_HEIGHT - menu_h;

                int start_btn =
                    in_rect(ev.button.x,ev.button.y,
                            0,height-TASKBAR_HEIGHT,
                            START_WIDTH,TASKBAR_HEIGHT);

                int clicked_menu =
                    menu_open &&
                    in_rect(ev.button.x,ev.button.y,
                            mx,my,MENU_WIDTH,menu_h);

                if (start_btn) {

                    menu_open = !menu_open;

                    if (!menu_open) {
                        /* clear menu area when closing */
                        GrSetGCForeground(gc_bar,GR_COLOR_LIGHTSKYBLUE);
                        GrFillRect(win,gc_bar,mx,my,MENU_WIDTH,menu_h);
                    }

                    need_redraw = 1;
                }
                else if (clicked_menu) {

                    handle_menu_click(ev.button.x,
                          ev.button.y,
                          &need_redraw);
                }
                else {
                    if (menu_open) {
                        menu_open = 0;

                        GrSetGCForeground(gc_bar,GR_COLOR_LIGHTSKYBLUE);
                        GrFillRect(win,gc_bar,
                                   mx,my,MENU_WIDTH,menu_h);

                        need_redraw = 1;
                    }
                }
            }
            break;

            case GR_EVENT_TYPE_CLOSE_REQ:
				exitwait(); /* wait for all children to exit, then exit ourselves */
            }
        }
		
		if (nxselect_running && nx_fd!=-1 && nx_fp != NULL)
             poll_for_nxselect_result();
		 
		if (nxmsg_running && nx_fd!=-1 && nx_fp != NULL)
             poll_for_nxmsg_result();

		/* cancel operation if nxselect or nxmsg returned "[]" */
		if (response_received == 1 && buf[0] == '[' &&  buf[1] == ']')
		{
			response_received = 0;
		    image_view_requested = 0;
			edit_file_requested = 0;
			message_box_requested = 0;
		}

		if (!nxselect_running && response_received == 1 && (image_view_requested == 1 || edit_file_requested == 1))
		{
			image_view_requested = 0;
			edit_file_requested = 0;
			response_received = 0;
			if (nxselect_cb)
				nxselect_cb(buf);
			nxselect_cb = NULL;
		}
		
		if (!nxmsg_running && response_received == 1 && message_box_requested == 1)
		{
			response_received = 0;
			message_box_requested = 0;

			if (nxmsg_cb)
            	nxmsg_cb(buf);
			nxmsg_cb = NULL;
		}

        /* ===== STATUS TIMER (18s first time, then 10s) ===== */
        time_t now = time(NULL);

        if (ui_initialized && now >= next_update && !is_slow_system) {

            update_memory_now();
			
            need_redraw = 1;
            next_update = now + 10; /* next every 10s */
        }

        /* ===== Deferred redraw ===== */
        if (ui_initialized && need_redraw) {
            redraw_all();
            need_redraw = 0;
        }
    }

    GrClose();
    return 0;
}

