#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/vt.h>
#include "device.h"
#include "fb.h"
/*
 * VT switch handling code for Linux
 */

/* signal to use when VT swithing*/
#ifndef SIGUNUSED
#define SIGUNUSED	SIGUSR1		/* some systems lack SIGUNUSED*/
#endif

#define SIGVTSWITCH	SIGUNUSED	/* SIGUSR2 is used by pthreads...*/

int	mwvterm;		/* the VT we were started on */
volatile int mwdrawing;		/* nonzero when drawing is happening*/
static int mwcvt, mwocvt;
static int ttyfd = -1;		/* /dev/tty0*/
static int visible = 1;		/* VT visible flag*/
static struct vt_mode mode;	/* terminal mode*/
static SUBDRIVER save;		/* saved subdriver when VT switched*/

extern SCREENDEVICE	scrdev;	/* FIXME */

/* entry points*/
int 	MwInitVt(void);
int  	MwCurrentVt(void);
int  	MwCheckVtChange(void);
void 	MwRedrawVt(int t);

/* local routines*/
static void  	draw_enable(void);
static void 	draw_disable(void);
static void	vt_switch(int sig);

/* null subdriver for drawing when switched out*/
static void 	null_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c) {}
static MWPIXELVAL null_readpixel(PSD psd,MWCOORD x, MWCOORD y) { return 0;}
static void	null_drawhorzline(PSD psd,MWCOORD x1,MWCOORD x2,MWCOORD y,
			MWPIXELVAL c) {}
static void	null_drawvertline(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2,
			MWPIXELVAL c) {}
static void	null_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,
			MWCOORD y2,MWPIXELVAL c) {}
static void	null_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,
			MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,
			long op) {}
static void 	null_drawarea(PSD psd, driver_gc_t *gc, int op) {}
static SUBDRIVER nulldriver = {
	NULL,
	null_drawpixel,
	null_readpixel,
	null_drawhorzline,
	null_drawvertline,
	null_fillrect,
	null_blit,
	null_drawarea
};

static void
draw_enable(void)
{
	if(visible)
		return;
	visible = 1;

	/* restore screen drawing functions*/
	set_subdriver(&scrdev, &save, FALSE);
}
      
static void
draw_disable(void)
{
	if(!visible)
		return;
	visible = 0;

	/* save screen drawing functions and reroute drawing*/
	get_subdriver(&scrdev, &save);

	/* set null driver*/
	set_subdriver(&scrdev, &nulldriver, FALSE);
}

/* Timer handler used to do the VT switch at a time when not drawing */
static void
vt_do_switch(void *arg)
{
    static unsigned short r[16], g[16], b[16];

    /*
     * If a drawing function is in progress then we cannot mode
     * switch right now because the drawing function would continue to
     * scribble on the screen after the switch.  So disable further
     * drawing and schedule an alarm to try again in .1 second.
     */
    if(mwdrawing) {
    	draw_disable ();
	GdAddTimer(100, vt_do_switch, NULL);
    	return;
    }
      
    if(visible) {
    	draw_disable ();
	ioctl_getpalette(0, 16, r, g, b);

	if(ioctl (ttyfd, VT_RELDISP, 1) == -1)
	    EPRINTF("Error can't switch away from VT: %m\n");
    } else {
	ioctl_setpalette(0, 16, r, g, b);
    	draw_enable ();
      
	if(ioctl (ttyfd, VT_RELDISP, VT_ACKACQ) == -1)
		EPRINTF("Error can't acknowledge VT switch: %m\n");
    }
}

/* Signal handler called when kernel switches to or from our tty*/
static void
vt_switch(int sig)
{
	signal(SIGVTSWITCH, vt_switch);
	vt_do_switch(NULL);
}

/*
 * Init VT switch catching code
 * 	return 0 on success, -1 on error
 */
int
MwInitVt(void)
{
	ttyfd = open("/dev/tty0", O_RDONLY);
	if(ttyfd == -1)
		return EPRINTF("Error can't open tty0: %m\n");
	
	/* setup new tty mode*/
	if(ioctl (ttyfd, VT_GETMODE, &mode) == -1)
		return EPRINTF("Error can't get VT mode: %m\n");

	mode.mode = VT_PROCESS;
	mode.relsig = SIGVTSWITCH;
	mode.acqsig = SIGVTSWITCH;
	signal (SIGVTSWITCH, vt_switch);
	if(ioctl (ttyfd, VT_SETMODE, &mode) == -1)
		return EPRINTF("Error can't set VT mode: %m\n");

	mwcvt = mwocvt = mwvterm = MwCurrentVt();
	/*
	 * Note: this hack is required to get Linux
	 * to orient virtual 0,0 with physical 0,0
	 * I have no idea why this kluge is required...
	 */
	MwRedrawVt(mwvterm);

	return 0;
}

/*
 * This function is used to find out what the current active VT is.
 */
int
MwCurrentVt(void)
{
	struct vt_stat stat;

	ioctl(ttyfd, VT_GETSTATE, &stat);
	return stat.v_active;
}

/*
 * Check if our VT has changed.  Return 1 if so.
 */
int
MwCheckVtChange(void)
{
	mwcvt = MwCurrentVt();
	if(mwcvt != mwocvt && mwcvt == mwvterm) {
		mwocvt = mwcvt;
		return 1;
	}
	mwocvt = mwcvt;
	return 0;
}

/*
 * This function is used to cause a redraw of the text console.
 * FIXME: Switching to another console and
 * back works, but that's a really dirty hack
 */
void
MwRedrawVt(int t)
{
	if(MwCurrentVt() == mwvterm) {
		ioctl(ttyfd, VT_ACTIVATE, t == 1 ? 2 : 1); /* Awful hack!!*/
		ioctl(ttyfd, VT_ACTIVATE, t);
	}
}

void
MwExitVt(void)
{
	signal(SIGVTSWITCH, SIG_DFL);
	mode.mode = VT_AUTO;
	mode.relsig = 0;
	mode.acqsig = 0;
	ioctl(ttyfd, VT_SETMODE, &mode);

	if(ttyfd != -1)
		close(ttyfd);
}
