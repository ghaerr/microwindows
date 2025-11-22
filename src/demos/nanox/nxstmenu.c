/* 

A Windows 95 like start menu for ELKS and Nano-X 

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "nano-X.h"
#include "nxcolors.h"

#define TASKBAR_HEIGHT 24
#define START_WIDTH    60
#define CLOCK_WIDTH    60
#define APP_PATH "/bin/"

const char *apps[] = { "nxcalc", "nxclock", "nxstart", "nxmine" };
#define APP_COUNT (sizeof(apps)/sizeof(apps[0]))

static GR_WINDOW_ID win;
static GR_GC_ID gc_bar, gc_text;
static int width, height;
static int menu_open = 0;

// Reap child processes so multiple apps can start
static void reaper(int signum) { while(waitpid(-1,NULL,WNOHANG)>0); }

static void draw3drect(int x,int y,int w,int h,int raised) {
    GrSetGCForeground(gc_bar, raised ? GrGetSysColor(GR_COLOR_WINDOW) : GrGetSysColor(GR_COLOR_BTNSHADOW));
    GrLine(win,gc_bar,x,y,x+w-1,y);
    GrLine(win,gc_bar,x,y,x,y+h-1);
    GrSetGCForeground(gc_bar, raised ? GrGetSysColor(GR_COLOR_BTNSHADOW) : GrGetSysColor(GR_COLOR_WINDOW));
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

    int cx = width - CLOCK_WIDTH;
    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,cx,height-TASKBAR_HEIGHT,CLOCK_WIDTH,TASKBAR_HEIGHT);
    draw3drect(cx,height-TASKBAR_HEIGHT,CLOCK_WIDTH,TASKBAR_HEIGHT,1);
}

static void draw_menu(void) {
    int menu_w = 120;
    int menu_h = (APP_COUNT+1)*16 + 4;
    int mx = 0;
    int my = height-TASKBAR_HEIGHT-menu_h;

    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,mx,my,menu_w,menu_h);
    draw3drect(mx,my,menu_w,menu_h,1);

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    int y = my + 4;
    for(int i=0;i<APP_COUNT;i++) {
        GrText(win,gc_text,4,y,(void *)apps[i],strlen(apps[i]),GR_TFASCII|GR_TFTOP);
        y+=16;
    }
    GrLine(win,gc_text,mx+2,y,mx+menu_w-2,y);
    y+=4;
    GrText(win,gc_text,4,y,"Exit",4,GR_TFASCII|GR_TFTOP);
}

static void draw_clock(void) {
    int cx = width - CLOCK_WIDTH;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[16];
    snprintf(buf,sizeof(buf),"%02d:%02d",tm->tm_hour,tm->tm_min);
    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,cx+6,height-TASKBAR_HEIGHT+6,buf,strlen(buf),GR_TFASCII|GR_TFTOP);
}

static int in_rect(int x,int y,int rx,int ry,int rw,int rh) {
    return x>=rx && x<rx+rw && y>=ry && y<ry+rh;
}

int main(void) {
    GR_EVENT ev;
    time_t last_clock = 0;

    signal(SIGCHLD, reaper);

    if (GrOpen() < 0) {
        fprintf(stderr,"Cannot open Nano-X\n");
        return 1;
    }

    GR_SCREEN_INFO sinfo;
    GrGetScreenInfo(&sinfo);
    width = sinfo.cols;
    height = sinfo.rows;

    win = GrNewWindowEx(GR_WM_PROPS_NODECORATE,"stmenu",GR_ROOT_WINDOW_ID,0,0,width,height,GR_COLOR_LIGHTSKYBLUE);

    gc_bar = GrNewGC();
    gc_text = GrNewGC();

    GrSelectEvents(win, GR_EVENT_MASK_EXPOSURE|GR_EVENT_MASK_BUTTON_DOWN|GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(win);

    draw_taskbar();
    draw_clock();

    for(;;) {
        GrGetNextEvent(&ev);

        switch(ev.type) {
            case GR_EVENT_TYPE_EXPOSURE:
                draw_taskbar();
                if(menu_open) draw_menu();
                draw_clock();
                break;

            case GR_EVENT_TYPE_BUTTON_DOWN:
            {
                int start_btn = in_rect(ev.button.x, ev.button.y, 0, height-TASKBAR_HEIGHT, START_WIDTH, TASKBAR_HEIGHT);
                int mx = 0;
                int my = height-TASKBAR_HEIGHT-(APP_COUNT+1)*16-4;
                int clicked_in_menu = menu_open && in_rect(ev.button.x, ev.button.y, mx, my, 120, (APP_COUNT+1)*16+4);

                // Toggle menu if Start button clicked
                if(start_btn) {
                    menu_open = !menu_open;
                }
                // Check if an app is clicked
                else if(clicked_in_menu) {
                    for(int i=0;i<APP_COUNT;i++) {
                        if(in_rect(ev.button.x, ev.button.y, mx, my+i*16, 120,16)) {
                            char cmd[64];
                            snprintf(cmd,sizeof(cmd),APP_PATH "%s",apps[i]);
                            if(fork() == 0) {
                                execl(cmd, cmd, NULL);
                                _exit(1);
                            }
                            menu_open = 0; // close menu after launch
                            break;
                        }
                    }
                    // Exit clicked
                    if(in_rect(ev.button.x, ev.button.y, mx, my+APP_COUNT*16+4, 120,16)) {
                        GrClose();
                        return 0;
                    }
                }
                // Clicked outside menu and Start button
                else {
                    menu_open = 0;
                }

                // Redraw taskbar and menu according to menu_open state
                draw_taskbar();
                if(menu_open) draw_menu();
                draw_clock();
            }
            break;

            case GR_EVENT_TYPE_CLOSE_REQ:
                GrClose();
                return 0;
        }

        // Update clock every minute
        time_t now = time(NULL);
        if(now != last_clock) {
            draw_clock();
            last_clock = now;
        }
    }

    GrClose();
    return 0;
}

