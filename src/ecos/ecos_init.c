//
// Microwindows/Nano-X additional setup for eCos
//

#include <pkgconf/kernel.h>
#include <pkgconf/hal.h>
#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

#define MWINCLUDECOLORS
#include "nano-X.h"

static bool closed = false;

// Display a number of ticks as microseconds
// Note: for improved calculation significance, values are kept in ticks*1000
static void
show_ns(long long ns)
{
    diag_printf("%5d.%02d", (int)(ns/1000), (int)((ns%1000)/10));
}

static long rtc_resolution[] = CYGNUM_KERNEL_COUNTERS_RTC_RESOLUTION;
static long ns_per_system_clock;

static long long
ns_time(void)
{
    unsigned long off;
    long long ns, clocks;

    ns_per_system_clock = 1000000/rtc_resolution[1];
    HAL_CLOCK_READ(&off);
    ns = (ns_per_system_clock * (long long)off) / CYGNUM_KERNEL_COUNTERS_RTC_PERIOD;
    ns += 5;  // for rounding to .01us
    clocks = (cyg_current_time() * 10000000) + ns;
    return clocks;
}

static void
test_file_io(void)
{
    long long start_time, end_time;
    FILE *fp;
    unsigned char buf[256];
    int len, n, fd;

    start_time = ns_time();
    if ((fp = fopen("/bin/letters.cl", "r")) != (FILE *)NULL) {
        len = 0;
        while (fgets(buf, sizeof(buf), fp)) {
            len += strlen(buf);
        }
        fclose(fp);
    }
    end_time = ns_time();
    diag_printf("'fgets': %d bytes in ", len); show_ns(end_time-start_time);  diag_printf("ns\n");

    start_time = ns_time();
    if ((fp = fopen("/bin/letters.cl", "r")) != (FILE *)NULL) {
        len = 0;
        while (n = fread(buf, 1, sizeof(buf), fp)) {
            len += n;
        }
        fclose(fp);
    }
    end_time = ns_time();
    diag_printf("'fread': %d bytes in ", len); show_ns(end_time-start_time);  diag_printf("ns\n");

    start_time = ns_time();
    if ((fd = open("/bin/letters.cl", O_RDONLY)) >= 0) {
        len = 0;
        while (n = read(fd, buf, sizeof(buf))) {
            len += n;
        }
        close(fd);
    }
    end_time = ns_time();
    diag_printf("'read': %d bytes in ", len); show_ns(end_time-start_time);  diag_printf("ns\n");
}

#if NWIDGETS
static void 
do_close(NBUTTON * w, int b)
{
   printf("Button %d was clicked in widget %p\n",b,w);
   closed = true;
}
#endif

void
ecos_nx_init(CYG_ADDRWORD data)
{
    GR_SCREEN_INFO	si;		/* window information */
    GR_FONT_INFO	fi;		/* font information */
    GR_WINDOW_ID	mainwid;	/* main window id */
    GR_WM_PROPERTIES    props;
    GR_GC_ID		gct = 0;
    NWIDGET             *w;
    NBUTTON             *b;
    NTEXTFIELD          *t;

    cyg_thread_delay(50);
    INIT_PER_THREAD_DATA();

    test_file_io();

    if(GrOpen() < 0) {
        fprintf(stderr, "Couldn't connect to Nano-X server\n");
        exit(1);
    }

    GrGetScreenInfo(&si);
    GrGetFontInfo(0, &fi);

#ifndef NWIDGETS
    mainwid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, si.cols, si.rows,
                          0, RED, WHITE);

    props.flags = GR_WM_FLAGS_PROPS;
    props.props = GR_WM_PROPS_BORDER;
    GrSetWMProperties(mainwid, &props);

    GrMapWindow(mainwid);
    GrFlush();
    cyg_thread_delay(50);

    gct = GrNewGC();
    GrSetGCForeground(gct, WHITE);
    /*GrSetGCFont(gct, GrCreateFont(GR_FONT_GUI_VAR, 0, NULL));*/
    GrDrawImageFromFile(mainwid, gct, 0, 0, si.cols, si.rows, "/redhat.logo", 0);
    GrText(mainwid, gct, 80, 350, "Tap all 4 corners", 17, GR_TFTOP);
    GrFlush();
    printf("Tap all four corners\n");
    cyg_thread_delay(10*100);

#else
    n_init_button_class();
    n_init_textfield_class();

    w = NEW_NOBJECT(widget);
    n_widget_init(w, 0);    
    n_widget_resize(w, si.cols - 10, si.rows - 30);
    n_widget_background(w, "/redhat.logo");
    n_widget_show(w);

    b = NEW_NOBJECT(button);
    n_button_init(b, w, "Close");
    n_button_onclick(b, do_close);
    n_widget_resize(b, 40, 20);
    n_widget_move(b,180,260);
    n_widget_show(b);

    t = NEW_NOBJECT(textfield);
    n_textfield_init(t,w,"Tap all 4 corners");
    n_widget_move(t,45,220);
    n_widget_resize(t,120,20);
    n_widget_show(t);

    t = NEW_NOBJECT(textfield);
    n_textfield_init(t,w,"Then press close");
    n_widget_move(t,45,250);
    n_widget_resize(t,120,20);
    n_widget_show(t);

    while (!closed) {
        n_handle_event();
    }

    n_widget_hide(w);
    n_object_cleanup(w);
#endif

    GrClose();
}
