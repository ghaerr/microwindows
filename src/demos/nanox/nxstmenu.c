/* 

A Windows 95 like start menu for ELKS and Nano-X 

TODO:
  V1:
   - improve colors
   - rename to nxdsktop
   - add commands such as: Halt, Restart
   - add About section with MessageBox
  V2:
   - add proper exit - this app, all other nxapp and the Nano-X server
   - add free/total conventional memory in taskbar
   - Improve 3D feel
  V3:
   - add message bar or busy mouse pointer
   - add menu item hoover (blue background, white text)
   - add "themes" from a config file 
   - add process list window or application that shows how much memory is used per process
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
//#include "tools.h"

#define TASKBAR_HEIGHT        24
#define START_WIDTH           40
#define CLOCK_WIDTH           40
#define MENU_WIDTH            100
#define MEMORY_WIDTH          80   /* Width of memory field */

#define MENU_ITEM_HEIGHT      18
#define MENU_ITEM_EXIT_HEIGHT 20

#define ENABLE_MEMORY_USAGE   1   /* Set 1 to enable memory display */
#define APP_PATH "/bin/"

const char *apps[] = { "nxcalc", "nxclock", "nxmine", "nxterm", "nxtetris", "nxworld" };
#define APP_COUNT (sizeof(apps)/sizeof(apps[0]))
static GR_WINDOW_ID win;
static GR_GC_ID gc_bar, gc_text;
static int width, height;
static int menu_open = 0;

#if ENABLE_MEMORY_USAGE
static FILE *mem_fp = NULL;
static int mem_pipe_fd = -1;
static unsigned int mem_free = 0;
static unsigned int mem_total = 0;
static int mem_valid = 0;  /* only draw memory field if valid data recovered */
#if ENABLE_MEMORY_USAGE
static void draw_memory_field(void);
#endif
#endif

/* Reap child processes */
static void reaper(int signum)
{
    (void)signum;
    /* ELKS only supports wait(), no waitpid/WNOHANG */
    while (wait(NULL) > 0)
        ;
}

static void draw3drect(int x,int y,int w,int h,int raised) {
    GrSetGCForeground(gc_bar, raised ? GrGetSysColor(GR_COLOR_WINDOW) 
                                     : GrGetSysColor(GR_COLOR_BTNSHADOW));
    GrLine(win,gc_bar,x,y,x+w-1,y);
    GrLine(win,gc_bar,x,y,x,y+h-1);

    GrSetGCForeground(gc_bar, raised ? GrGetSysColor(GR_COLOR_BTNSHADOW)
                                     : GrGetSysColor(GR_COLOR_WINDOW));
    GrLine(win,gc_bar,x,y+h-1,x+w-1,y+h-1);
    GrLine(win,gc_bar,x+w-1,y,x+w-1,y+h-1);
}

static void draw_taskbar(void) {
    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,0,height-TASKBAR_HEIGHT,width,TASKBAR_HEIGHT);

    draw3drect(0,height-TASKBAR_HEIGHT,width,4,1);

    GrFillRect(win,gc_bar,0,height-TASKBAR_HEIGHT,START_WIDTH,TASKBAR_HEIGHT);
    draw3drect(0,height-TASKBAR_HEIGHT,START_WIDTH,TASKBAR_HEIGHT,1);

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,8,height-TASKBAR_HEIGHT+6,"Start",5,GR_TFASCII|GR_TFTOP);

#if ENABLE_MEMORY_USAGE
    draw_memory_field();
#endif

    int cx = width - CLOCK_WIDTH;
    GrFillRect(win,gc_bar,cx,height-TASKBAR_HEIGHT,CLOCK_WIDTH,TASKBAR_HEIGHT);
    draw3drect(cx,height-TASKBAR_HEIGHT,CLOCK_WIDTH,TASKBAR_HEIGHT,1);
}

static void draw_menu(void) {
    int mx = 0;
    int menu_h = (APP_COUNT * MENU_ITEM_HEIGHT) + MENU_ITEM_EXIT_HEIGHT + 4;
    int my = height - TASKBAR_HEIGHT - menu_h;

    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,mx,my,MENU_WIDTH,menu_h);
    draw3drect(mx,my,MENU_WIDTH,menu_h,1);

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));

    int y = my + 4;

    for(int i = 0; i < APP_COUNT; i++) {
        GrText(win, gc_text, 4, y, (void*)apps[i], strlen(apps[i]),
               GR_TFASCII | GR_TFTOP);
        y += MENU_ITEM_HEIGHT;
    }

    GrLine(win,gc_text,mx+2,y,mx+MENU_WIDTH-2,y);
    y += 4;

    GrText(win,gc_text,4,y,"Exit",4,GR_TFASCII | GR_TFTOP);
}

static void draw_clock(void) {
    int cx = width - CLOCK_WIDTH;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[16];

    snprintf(buf,sizeof(buf),"%02d:%02d",tm->tm_hour,tm->tm_min);

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,cx+6,height-TASKBAR_HEIGHT+6,
           buf,strlen(buf),GR_TFASCII|GR_TFTOP);
}

static int in_rect(int x,int y,int rx,int ry,int rw,int rh) {
    return (x>=rx && x<rx+rw && y>=ry && y<ry+rh);
}

#if ENABLE_MEMORY_USAGE
/* --- Memory field functions --- */
static void update_memory_start(void)
{
    if(mem_fp) { pclose(mem_fp); mem_fp = NULL; }

    mem_fp = popen("meminfo -b", "r");
    if(!mem_fp) return;

    mem_pipe_fd = fileno(mem_fp);
    fcntl(mem_pipe_fd, F_SETFL, O_NONBLOCK);
}

static void update_memory_poll(void)
{
    if(!mem_fp) return;

    char line[160];
    while(fgets(line,sizeof(line),mem_fp)) {
        if(strncmp(line,"Memory usage",12)==0) {
            unsigned int total=0, used=0, free=0;
            if(sscanf(line,"Memory usage %uKB total, %uKB used, %uKB free",
                      &total, &used, &free) == 3) {
                mem_free = free;
                mem_total = total;
                mem_valid = 1;  /* data is valid */
                break;
            }
        }
    }

    if(feof(mem_fp)) {
        pclose(mem_fp);
        mem_fp = NULL;
        mem_pipe_fd = -1;
    }
}

static void draw_memory_field(void)
{
    if(!mem_valid) return;  /* draw only if valid data recovered */

    int mx = width - CLOCK_WIDTH - MEMORY_WIDTH;
    char buf[32];
    snprintf(buf,sizeof(buf),"%u / %u KB", mem_free, mem_total);

    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,mx,height-TASKBAR_HEIGHT,MEMORY_WIDTH,TASKBAR_HEIGHT);
    draw3drect(mx,height-TASKBAR_HEIGHT,MEMORY_WIDTH,TASKBAR_HEIGHT,1);

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,mx+6,height-TASKBAR_HEIGHT+6,buf,strlen(buf),GR_TFASCII|GR_TFTOP);
}
#endif

int main(void) {
    GR_EVENT ev;
    time_t last_clock = 0;
#if ENABLE_MEMORY_USAGE
    time_t last_mem = 0;
#endif

    signal(SIGCHLD, reaper);

    if(GrOpen() < 0) {
        fprintf(stderr,"Cannot open Nano-X\n");
        return 1;
    }

    GR_SCREEN_INFO sinfo;
    GrGetScreenInfo(&sinfo);
    width = sinfo.cols;
    height = sinfo.rows;

    win = GrNewWindowEx(GR_WM_PROPS_NODECORATE,"stmenu",
                        GR_ROOT_WINDOW_ID,0,0,width,height,
                        GR_COLOR_LIGHTSKYBLUE);

    gc_bar  = GrNewGC();
    gc_text = GrNewGC();

    GrSelectEvents(win, GR_EVENT_MASK_EXPOSURE |
                        GR_EVENT_MASK_BUTTON_DOWN |
                        GR_EVENT_MASK_BUTTON_UP |
                        GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(win);

    draw_taskbar();
    draw_clock();
#if ENABLE_MEMORY_USAGE
    update_memory_start();
#endif

    for(;;) {
        GrGetNextEvent(&ev);

        switch(ev.type) {

        case GR_EVENT_TYPE_EXPOSURE:
            draw_taskbar();
            if(menu_open) draw_menu();
            draw_clock();
#if ENABLE_MEMORY_USAGE
            draw_memory_field();
#endif
            break;

        case GR_EVENT_TYPE_BUTTON_DOWN: {
            int mx = 0;
            int menu_h =
                (APP_COUNT * MENU_ITEM_HEIGHT) +
                MENU_ITEM_EXIT_HEIGHT +
                4;

            int my = height - TASKBAR_HEIGHT - menu_h;

            int start_btn = in_rect(ev.button.x, ev.button.y,
                                    0, height-TASKBAR_HEIGHT,
                                    START_WIDTH, TASKBAR_HEIGHT);

            int clicked_menu =
                menu_open &&
                in_rect(ev.button.x, ev.button.y,
                        mx, my, MENU_WIDTH, menu_h);

            if(start_btn) {
                menu_open = !menu_open;

                if(menu_open) {
                    draw_menu();
                } else {
                    GrSetGCForeground(gc_bar, GR_COLOR_LIGHTSKYBLUE);
                    GrFillRect(win,gc_bar,mx,my,MENU_WIDTH,menu_h);
                }

                draw_taskbar();
                draw_clock();
#if ENABLE_MEMORY_USAGE
                draw_memory_field();
#endif
            }
            else if(clicked_menu) {

                for(int i=0;i<APP_COUNT;i++) {
                    if(in_rect(ev.button.x, ev.button.y,
                               mx,
                               my + (i * MENU_ITEM_HEIGHT),
                               MENU_WIDTH,
                               MENU_ITEM_HEIGHT))
                    {
                        char cmd[64];
                        snprintf(cmd,sizeof(cmd),APP_PATH "%s",apps[i]);

                        if(fork()==0) {
                            execl(cmd, cmd, NULL);
                            _exit(1);
                        }

                        menu_open = 0;
                        GrSetGCForeground(gc_bar, GR_COLOR_LIGHTSKYBLUE);
                        GrFillRect(win,gc_bar,mx,my,MENU_WIDTH,menu_h);
                        break;
                    }
                }

                /* EXIT item click */
                if(in_rect(ev.button.x, ev.button.y,
                           mx,
                           my + (APP_COUNT * MENU_ITEM_HEIGHT) + 4,
                           MENU_WIDTH,
                           MENU_ITEM_EXIT_HEIGHT))
                {
                    GrClose();
                    return 0;
                }

                draw_taskbar();
                draw_clock();
#if ENABLE_MEMORY_USAGE
                draw_memory_field();
#endif
            }
            else {
                if(menu_open) {
                    menu_open = 0;
                    GrSetGCForeground(gc_bar, GR_COLOR_LIGHTSKYBLUE);
                    GrFillRect(win,gc_bar,mx,my,MENU_WIDTH,menu_h);

                    draw_taskbar();
                    draw_clock();
#if ENABLE_MEMORY_USAGE
                    draw_memory_field();
#endif
                }
            }
        }
        break;

        case GR_EVENT_TYPE_CLOSE_REQ:
            GrClose();
            return 0;
        }

        time_t now = time(NULL);

        /* Update clock every second */
        if(now != last_clock) {
            draw_clock();
            last_clock = now;
        }

#if ENABLE_MEMORY_USAGE
        /* Update memory every 12 seconds */
        if(now - last_mem >= 12) {
            update_memory_start();
            last_mem = now;
        }

        update_memory_poll();
        draw_memory_field();
#endif
    }

    GrClose();
    return 0;
}
