/* 

A Windows 95 like start menu for ELKS and Nano-X.
It specifically targets slow 8086/8088 such as Amstrad 1640 at 8Mhz. 

TODO:
  V1:
   - improve colors
   - rename to nxdsktop
   - add commands such as: Halt, Restart - DONE
   - add About section with MessageBox - needs another nxapp to display nicely
   - add free/total conventional memory in taskbar - DONE
   - support for nxjpeg, nxselect, nxterm and edit - DONE
   
  V2:
   - add proper exit - this app, all other nxapp and the Nano-X server
   - reduce redraw frequency and avoid heavy redraws triggered by button clicks
        - only redraw what changed, not the whole UI
        - avoid clearing big rectangles in event handlers
        - avoid status redraw on EXPOSE for the entire window
        - add throttling to redraws - never allow redraws more than once every 100ms.
   - Improve 3D feel
   
  V3:
   - add message bar or busy mouse pointer
   - add menu item hoover (blue background, white text)
   - add "themes" from a config file 
   - add process list window or application that shows how much memory is used per process

  Notes: 
    Rules to fllow on slow 8086/8088 systems:
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
//#include "uihelper.h"

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
#define ENABLE_MEMORY_USAGE   1
#define GR_EVENT_TIMEOUT      70
#define APP_PATH "/bin/"

const char *apps[] =
    { "About", "Calculator","Clock","Mine","Tetris","World map zoom","Terminal","View jpg as 16c", "View jpg as 8c", "View jpg as 4c", "Edit file"};
	
const char *sys[] =
    { "Exit", "Restart","Sync disk"}; /* TODO: Implement 'Shutdown' item with a message, shutdown does both sync and umount */ 

#define APP_COUNT (sizeof(apps)/sizeof(apps[0]))
#define SYS_COUNT (sizeof(sys)/sizeof(sys[0]))

/* ===== GLOBALS ===== */
static GR_WINDOW_ID win;
static GR_GC_ID gc_bar, gc_text;
static int width, height;
static int menu_open = 0;
//static UIMessageBox *about_box = NULL;

#if ENABLE_MEMORY_USAGE
static unsigned int mem_free = 0;
static unsigned int mem_total = 0;
static int mem_valid = 0;
#endif


/* ===== CHILD REAPER ===== */
static void reaper(int s) { (void)s; wait(NULL); }


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
#if ENABLE_MEMORY_USAGE
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
#endif


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

#if ENABLE_MEMORY_USAGE
    if(mem_valid)
        snprintf(buf,sizeof(buf),"%u / %u KB  ", mem_free, mem_total);
    else
        snprintf(buf,sizeof(buf),"-- / -- KB  ");
#else
    buf[0] = '\0';
#endif

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char clk[16];
    snprintf(clk,sizeof(clk),"%02d : %02d",tm->tm_hour,tm->tm_min);
    strcat(buf,clk);

    /* Draw transparent text aligned properly */
    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,
           x + 6,
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

static FILE *nx_fp = NULL;
static int nx_fd = -1;
static int nxselect_running = 0;
char buf[80]; 
static int path_received = 0;
static int image_view_requested = 0;
static int edit_file_requested =0;
static int image_view_color_mode = 16;

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
            printf("path received: %s\n", buf);
			path_received = 1;
        } else {
            printf("nxselect closed (no path)\n");
        }

        pclose(nx_fp);
        nx_fp = NULL;
        nx_fd = -1;
        nxselect_running = 0;   /* stop polling */
    }
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

                if (!nxselect_running) {
                    nx_fp = popen("nxselect", "r");
                    if (nx_fp) {
                        nx_fd = fileno(nx_fp);
                        nxselect_running = 1;
                        path_received = 0;
                        image_view_requested = 1;
                    }
                }

            } else if (!strcmp(apps[i], "Edit file")) {

                if (!nxselect_running) {
                    nx_fp = popen("nxselect", "r");
                    if (nx_fp) {
                        nx_fd = fileno(nx_fp);
                        nxselect_running = 1;
                        path_received = 0;
                        edit_file_requested = 1;
                    }
                }

            } else if (!strcmp(apps[i], "About")) {

                /* Modeless About box (optional) */
                /* about_box = UI_MessageBoxCreate(win,
                        "About",
                        "nxDesktop 1.0\n(c) Anton Andreev"); */

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
            if (!strcmp(sys[i], "Exit")) {
                GrClose();
                exit(0);  //TODO: use new code for exit ?
            }
            else if (!strcmp(sys[i], "Restart")) {
                
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
int main(void)
{
    GR_EVENT ev;
    int ui_initialized = 0;
    int need_redraw = 0;
	
    signal(SIGCHLD, reaper);

    if (GrOpen() < 0) {
        fprintf(stderr,"Cannot open Nano-X\n");
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
                GrClose();
                exit(0);
            }
        }
		
		if (nxselect_running && nx_fd!=-1 && nx_fp != NULL)
             poll_for_nxselect_result();

		if (!nxselect_running && path_received == 1 && image_view_requested == 1)
		{
			path_received = 0;
			image_view_requested = 0;
			pid_t pid = fork();

			if (pid == 0) {
				if (image_view_color_mode == 4) {
					execl("/bin/nxjpeg",
						  "nxjpeg",
						  "-m",
						  "-g",
						  buf,
						  NULL);
				}
				else if (image_view_color_mode == 8) {
					execl("/bin/nxjpeg",
						  "nxjpeg",
						  "-g",
						  "-8",
						  buf,
						  NULL);
				}
				else {
					/* Default mode 16 colors ega */
					execl("/bin/nxjpeg",
						  "nxjpeg",
						  buf,
						  NULL);
				}
				/* Only reached if execl() fails */
				_exit(1);
			}
		}

		if (!nxselect_running && path_received == 1 && edit_file_requested == 1) {

			path_received = 0;
			edit_file_requested = 0;

			pid_t pid = fork();
			if (pid == 0) {

				char cmd[70];

				/* Build: "/bin/edit <path> && exit" including the double quotes */
				snprintf(cmd, sizeof(cmd),
						 "/bin/edit %s && exit",
						 buf);

				/* Pass whole quoted command as one argument to nxterm */
				execl("/bin/nxterm", "nxterm", cmd, NULL);

				_exit(1);
			}
		}
		
		/*if (about_box) {
			if (UI_MessageBoxHandleEvent(about_box, &ev)) {
				about_box = NULL;
			}
		}*/

        /* ===== STATUS TIMER (18s first time, then 10s) ===== */
        time_t now = time(NULL);

        if (ui_initialized && now >= next_update) {
#if ENABLE_MEMORY_USAGE
            update_memory_now();
#endif
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

