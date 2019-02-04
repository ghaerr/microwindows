/*
 * written by Georg Potthast 2012
 *
 * Allegro Mouse Driver
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#define ALLEGRO_USE_CONSOLE
#include <allegro.h>

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	AM_Open(MOUSEDEVICE *pmd);
static void 	AM_Close(void);
static int  	AM_GetButtonInfo(void);
static void		AM_GetDefaultAccel(int *pscale,int *pthresh);
static int  	AM_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	AM_Poll(void);

MOUSEDEVICE mousedev = {
	AM_Open,
	AM_Close,
	AM_GetButtonInfo,
	AM_GetDefaultAccel,
	AM_Read,
	AM_Poll,
};

extern SCREENDEVICE	scrdev;

/*
 * Poll for events
 */

static int
AM_Poll(void)
{
	return 1;
}

/*
 * Open up the mouse device.
 */
static int
AM_Open(MOUSEDEVICE *pmd)
{
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the mouse device.
 */
static void
AM_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
AM_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
AM_GetDefaultAccel(int *pscale,int *pthresh)
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
AM_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
static int mz; 
static int hidingmouse;   

		int mickeyx = 0;
		int mickeyy = 0;
		int mickeyz = 0;
		int buttons = 0;

        poll_mouse();

if (ALLEGRO_SUB_VERSION >2){
//mouse_on_screen() not available on version 4.2 -> starting with 4.3
        if (mouse_on_screen()){ //mouse inside microwindows window
            if (hidingmouse == 1){ //already shown?
                GdShowCursor(&scrdev);
                hidingmouse == 0;
            }
        }else{
            if (hidingmouse == 0){ //already hidden?
                GdHideCursor(&scrdev);
                hidingmouse=1;
            }
        }
}
		/* microwindows reads the motion */
		get_mouse_mickeys(&mickeyx,&mickeyy);
#if 0
		*dx=mickeyx;
		*dy=mickeyy;
#else
        /* return position */
	    *dx=mouse_x;
        *dy=mouse_y; 
#endif
//calculate wheel button (up/down)
		if(mouse_z != mz)
		    mickeyz = mouse_z - mz;
		else
		    mickeyz = 0;
		mz = mouse_z;

        *dz = 0;
        *bp = 0;
		buttons = 0;
		//the buttons seem to be returned different than documented in allegro
		if (mouse_b & 1)
			buttons |= MWBUTTON_L;
		if (mouse_b & 2)
			buttons |= MWBUTTON_R;
		if (mouse_b & 4)
			buttons |= MWBUTTON_M;
		if (mickeyz > 0)
		    buttons |= MWBUTTON_SCROLLUP;  
		if (mickeyz < 0)
		    buttons |= MWBUTTON_SCROLLDN;  
			
		*bp = buttons;
        /* return absolute mouse position */
		return MOUSE_ABSPOS;
}
