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

  Notes: 
   - text in GrText must be (void*)
   - there should be no warnings during compilation otherwise there is no text at all
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


/* ===== TEXT BASELINE FIX ===== */
/* Menu has 4 px padding → +10 is correct.
   Taskbar has no padding → needs +15. */
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

#define ENABLE_MEMORY_USAGE   1
#define APP_PATH "/bin/"

const char *apps[] =
    { "nxcalc","nxclock","nxmine","nxterm","nxtetris","nxworld" };

#define APP_COUNT (sizeof(apps)/sizeof(apps[0]))


/* ===== GLOBALS ===== */
static GR_WINDOW_ID win;
static GR_GC_ID gc_bar, gc_text;
static int width, height;
static int menu_open = 0;

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

    draw3drect(0,height-TASKBAR_HEIGHT,width,4,1);

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
                 MENU_ITEM_EXIT_HEIGHT + 4;

    int my = height - TASKBAR_HEIGHT - menu_h;

    GrSetGCForeground(gc_bar, GrGetSysColor(GR_COLOR_BTNFACE));
    GrFillRect(win,gc_bar,mx,my,MENU_WIDTH,menu_h);
    draw3drect(mx,my,MENU_WIDTH,menu_h,1);

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));

    int y = my + 4 + TEXT_Y_OFFSET_MENU;

    for(int i = 0; i < APP_COUNT; i++) {
        GrText(win,gc_text,4,y,(void *)apps[i],strlen(apps[i]),GR_TFASCII);
        y += MENU_ITEM_HEIGHT;
    }

    GrLine(win,gc_text,mx+2,y-TEXT_Y_OFFSET_MENU,mx+MENU_WIDTH-2,y-TEXT_Y_OFFSET_MENU);
    y += 4;

    GrText(win,gc_text,4,y,(void *)"Exit",4,GR_TFASCII);
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
    char line[160];
    FILE *fp = popen("meminfo -b","r");
    if(!fp) return;

    while (fgets(line,sizeof(line),fp)) {
        char *p=line;
        while(*p==' '||*p=='\t') p++;

        if (strncmp(p,"Main ",5)==0) {
            unsigned int used,total,freec;
            if (sscanf(p,"Main %u/%uK used, %uK free",
                       &used,&total,&freec)==3)
            {
                mem_total = total;
                mem_free  = freec;
                mem_valid = 1;
            }
            break;
        }
    }
    pclose(fp);
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
    snprintf(clk,sizeof(clk),"%02d:%02d",tm->tm_hour,tm->tm_min);
    strcat(buf,clk);

    /* Draw transparent text aligned properly */
    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrText(win,gc_text,
           x + 6,
           y + TEXT_Y_OFFSET_TASKBAR,
           (void *)buf,strlen(buf),GR_TFASCII);
}


/* ===== MAIN LOOP ===== */
int main(void)
{
    GR_EVENT ev;
    time_t last_update = 0;

    signal(SIGCHLD, reaper);

    if(GrOpen() < 0) {
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

    /* ===== FIX: Transparent text ===== */
    GrSetGCUseBackground(gc_text, GR_FALSE);

    GrSelectEvents(win,
        GR_EVENT_MASK_EXPOSURE |
        GR_EVENT_MASK_BUTTON_DOWN |
        GR_EVENT_MASK_BUTTON_UP |
        GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(win);

    /* Initial draw */
    draw_taskbar();
#if ENABLE_MEMORY_USAGE
    update_memory_now();
#endif
    draw_status_field();


    /* ===== EVENT LOOP ===== */
    for(;;)
    {
        GrGetNextEvent(&ev);

        switch(ev.type) {

        case GR_EVENT_TYPE_EXPOSURE:
            draw_taskbar();
            if(menu_open) draw_menu();
            draw_status_field();
            break;


        case GR_EVENT_TYPE_BUTTON_DOWN: {
            int mx = 0;
            int menu_h =
                (APP_COUNT * MENU_ITEM_HEIGHT) +
                MENU_ITEM_EXIT_HEIGHT + 4;

            int my = height - TASKBAR_HEIGHT - menu_h;

            int start_btn =
                in_rect(ev.button.x,ev.button.y,
                        0,height-TASKBAR_HEIGHT,
                        START_WIDTH,TASKBAR_HEIGHT);

            int clicked_menu =
                menu_open &&
                in_rect(ev.button.x,ev.button.y,
                        mx,my,MENU_WIDTH,menu_h);


            if(start_btn) {
                menu_open = !menu_open;

                if(menu_open)
                    draw_menu();
                else {
                    GrSetGCForeground(gc_bar,GR_COLOR_LIGHTSKYBLUE);
                    GrFillRect(win,gc_bar,mx,my,MENU_WIDTH,menu_h);
                }

                draw_taskbar();
                draw_status_field();
            }
            else if(clicked_menu) {

                for(int i=0;i<APP_COUNT;i++) {
                    if(in_rect(ev.button.x,ev.button.y,
                               mx,my+(i*MENU_ITEM_HEIGHT),
                               MENU_WIDTH,MENU_ITEM_HEIGHT))
                    {
                        char cmd[64];
                        snprintf(cmd,sizeof(cmd),
                                 APP_PATH "%s",apps[i]);

                        if(fork()==0) {
                            execl(cmd,cmd,NULL);
                            _exit(1);
                        }

                        menu_open = 0;

                        GrSetGCForeground(gc_bar,GR_COLOR_LIGHTSKYBLUE);
                        GrFillRect(win,gc_bar,
                                   mx,my,MENU_WIDTH,menu_h);

                        break;
                    }
                }

                /* Exit */
                if(in_rect(ev.button.x,ev.button.y,
                       mx,my+(APP_COUNT*MENU_ITEM_HEIGHT)+4,
                       MENU_WIDTH,MENU_ITEM_EXIT_HEIGHT))
                {
                    GrClose();
                    return 0;
                }

                draw_taskbar();
                draw_status_field();
            }
            else {
                if(menu_open) {
                    menu_open = 0;

                    GrSetGCForeground(gc_bar,GR_COLOR_LIGHTSKYBLUE);
                    GrFillRect(win,gc_bar,
                               mx,my,MENU_WIDTH,menu_h);

                    draw_taskbar();
                    draw_status_field();
                }
            }
        }
        break;


        case GR_EVENT_TYPE_CLOSE_REQ:
            GrClose();
            return 0;
        }

        /* 10-second update */
        time_t now = time(NULL);
        if(now - last_update >= 10)
        {
#if ENABLE_MEMORY_USAGE
            update_memory_now();
#endif
            draw_status_field();
            last_update = now;
        }
    }

    GrClose();
    return 0;
}

