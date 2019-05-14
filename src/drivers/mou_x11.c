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

static int  	mouX11_Open(MOUSEDEVICE *pmd);
static void 	mouX11_Close(void);
static int  	mouX11_GetButtonInfo(void);
static void	mouX11_GetDefaultAccel(int *pscale,int *pthresh);
static int  	mouX11_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

extern Display*     x11_dpy;
extern int          x11_scr;
extern Visual*      x11_vis;
extern Window       x11_win;
extern GC           x11_gc;
extern unsigned int x11_event_mask;
int          x11_setup_display(void);
void         x11_handle_event(XEvent*);

MOUSEDEVICE mousedev = {
    mouX11_Open,
    mouX11_Close,
    mouX11_GetButtonInfo,
    mouX11_GetDefaultAccel,
    mouX11_Read,
    NULL,
    MOUSE_NORMAL	/* flags*/
};

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int mouX11_Open(MOUSEDEVICE *pmd)
{
    if (x11_setup_display() < 0)
		return DRIVER_FAIL;
    /* return the x11 file descriptor for select */
    return DRIVER_OKFILEDESC(ConnectionNumber(x11_dpy));
}

/*
 * Close the mouse device.
 */
static void
mouX11_Close(void)
{
    /* nop */
}

/*
 * Get mouse buttons supported
 */
static int
mouX11_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

/*
 * Get default mouse acceleration settings
 */
static void
mouX11_GetDefaultAccel(int *pscale,int *pthresh)
{
    *pscale = SCALE;
    *pthresh = THRESH;
}

/*
 * Read mouse event.
 * Returns MOUSE_NODATA or MOUSE_ABSPOS.
 * This is a non-blocking call.
 */
static int
mouX11_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
    XEvent ev;
    long mask = x11_event_mask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

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
		if (ev.xmotion.state & Button4Mask)
		    button |= MWBUTTON_SCROLLUP;
		if (ev.xmotion.state & Button5Mask)
		    button |= MWBUTTON_SCROLLDN;
		*bp = button;
		return MOUSE_ABSPOS;		/* absolute position returned*/
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
		else if(ev.xbutton.button == 4)
			button = MWBUTTON_SCROLLUP;
		else if(ev.xbutton.button == 5)
			button = MWBUTTON_SCROLLDN;

		/* Get any other buttons that might be already held */
		if (ev.xbutton.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xbutton.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xbutton.state & Button3Mask)
		    button |= MWBUTTON_R;
		if (ev.xbutton.state & Button4Mask)
		    button |= MWBUTTON_SCROLLUP;
		if (ev.xbutton.state & Button5Mask)
		    button |= MWBUTTON_SCROLLDN;
		
		/*DPRINTF("!Pressing button: 0x%x, state: 0x%x, button: 0x%x\n",
			button,ev.xbutton.state, ev.xbutton.button);*/
		*bp = button;
		*dx = ev.xbutton.x;
		*dy = ev.xbutton.y;
		*dz = 0;
		return MOUSE_ABSPOS;		/* absolute position returned*/
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
		else if(ev.xbutton.button == 4)
			released = MWBUTTON_SCROLLUP;
		else if(ev.xbutton.button == 5)
			released = MWBUTTON_SCROLLDN;
		
		/* Get any other buttons that might be already held */
		if (ev.xbutton.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xbutton.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xbutton.state & Button3Mask)
		    button |= MWBUTTON_R;
		if (ev.xbutton.state & Button4Mask)
		    button |= MWBUTTON_SCROLLUP;
		if (ev.xbutton.state & Button5Mask)
		    button |= MWBUTTON_SCROLLDN;
	
		/* We need to remove the released button from the button mask*/
		button &= ~released; 

		*bp = button;
		*dx = ev.xbutton.x;
		*dy = ev.xbutton.y;
		*dz = 0;
		return MOUSE_ABSPOS;		/* absolute position returned*/
	    }
	} else 
	    x11_handle_event(&ev);
    } /* while*/
    return MOUSE_NODATA;
}
