/*
 * Author: Tony Rogvall <tony@bluetail.com>
 *
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	X11_Open(MOUSEDEVICE *pmd);
static void 	X11_Close(void);
static int  	X11_GetButtonInfo(void);
static void	X11_GetDefaultAccel(int *pscale,int *pthresh);
static int  	X11_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

extern Display*     x11_dpy;
extern int          x11_scr;
extern Visual*      x11_vis;
extern Window       x11_win;
extern GC           x11_gc;
extern unsigned int x11_event_mask;
extern int          x11_setup_display();
extern void         x11_handle_event(XEvent*);

MOUSEDEVICE mousedev = {
    X11_Open,
    X11_Close,
    X11_GetButtonInfo,
    X11_GetDefaultAccel,
    X11_Read,
    NULL
};

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int X11_Open(MOUSEDEVICE *pmd)
{
    if (x11_setup_display() < 0)
	return -1;
    /* return the x11 file descriptor for select */
    return ConnectionNumber(x11_dpy);  
}

/*
 * Close the mouse device.
 */
static void
X11_Close(void)
{
    /* nop */
}

/*
 * Get mouse buttons supported
 */
static int
X11_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
X11_GetDefaultAccel(int *pscale,int *pthresh)
{
    *pscale = SCALE;
    *pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
static int
X11_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
    static int noevent_count = 0;
    XEvent ev;
    int events = 0;
    long mask = /* x11_event_mask | */
#ifdef USE_EXPOSURE
      ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ExposureMask;
#else
      ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
#endif

    while (XCheckMaskEvent(x11_dpy, mask, &ev)) {
	if (ev.type == MotionNotify) {
	    if (ev.xmotion.window == x11_win) {
		int button = 0;
		*dx = ev.xmotion.x;
		*dy = ev.xmotion.y;
		*dz = 0;
		if (ev.xmotion.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xmotion.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xmotion.state & Button3Mask)
		    button |= MWBUTTON_R;
		*bp = button;
		events++;
	    }
	}
	else if (ev.type == ButtonPress) {
	    if (ev.xbutton.window == x11_win) {
	        int button = 0;
		
		/* Get pressed button */
	    	if(ev.xbutton.button == 1)
			button = MWBUTTON_L;
		else if(ev.xbutton.button == 2)
			button = MWBUTTON_M;
		else if(ev.xbutton.button == 3)
			button = MWBUTTON_R;

		/* Get any other buttons that might be already held */
		if (ev.xbutton.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xbutton.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xbutton.state & Button3Mask)
		    button |= MWBUTTON_R;
		
/*		printf("!Pressing button: 0x%x, state: 0x%x, button: 0x%x\n",
			button,ev.xbutton.state, ev.xbutton.button);*/
		*bp = button;
		*dx = ev.xbutton.x;
		*dy = ev.xbutton.y;
		*dz = 0;
		events++;
	    }
	}
	else if (ev.type == ButtonRelease) {
	    if (ev.xbutton.window == x11_win) {
	        int button = 0;
		int released = 0;
	
		/* Get released button */
	    	if(ev.xbutton.button == 1)
			released = MWBUTTON_L;
		else if(ev.xbutton.button == 2)
			released = MWBUTTON_M;
		else if(ev.xbutton.button == 3)
			released = MWBUTTON_R;
		
		/* Get any other buttons that might be already held */
		if (ev.xbutton.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xbutton.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xbutton.state & Button3Mask)
		    button |= MWBUTTON_R;
	
		/* We need to remove the released button from the button mask*/
		button &= ~released;

		/*printf("!Releasing button: 0x%x, state: 0x%x, button: 0x%x\n",
			button,ev.xbutton.state, ev.xbutton.button);*/

		*bp = button;
		*dx = ev.xbutton.x;
		*dy = ev.xbutton.y;
		*dz = 0;
		events++;
	    }
	}
	else {
	    x11_handle_event(&ev);
	}
    }
    if (events == 0) {
	/* after a bunch of consecutive noevent calls here
           (meaning select() says there's something to read but nothing
            is returned......), force an event read (which will
            most likely terminate the connection) */
	if (++noevent_count >= 50) {
            while(XNextEvent(x11_dpy, &ev)) {
                /* if we return, then we got an event...put it back
                   so we can properly process it next time through */
                XPutBackEvent(x11_dpy, &ev);
            }
            noevent_count = 0;
	}
	return 0;
    }
    noevent_count = 0;
    return 2;		/* absolute position returned*/
}
