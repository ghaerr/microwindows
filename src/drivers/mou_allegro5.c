/*
 * Allegro/Android Mouse Driver
 *
 * Written by Georg Potthast 2012
 * Updated to new driver format Greg Haerr 2019
 */
#include <stdio.h>
#include <stdlib.h>
#include "device.h"

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro.h>

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	mallegro_Open(MOUSEDEVICE *pmd);
static void 	mallegro_Close(void);
static int  	mallegro_GetButtonInfo(void);
static void		mallegro_GetDefaultAccel(int *pscale,int *pthresh);
static int  	mallegro_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	mallegro_Poll(void);

MOUSEDEVICE mousedev = {
	mallegro_Open,
	mallegro_Close,
	mallegro_GetButtonInfo,
	mallegro_GetDefaultAccel,
	mallegro_Read,
	mallegro_Poll,
};

extern ALLEGRO_DISPLAY *allegro_display;
extern ALLEGRO_EVENT_QUEUE *allegro_mouqueue;
extern float allegro_zoom;

/*
 * Mouse Poll
 */
static int
mallegro_Poll(void)
{
	ALLEGRO_EVENT event;

	if (!al_is_mouse_installed())
		return 0;

	if (al_peek_next_event(allegro_mouqueue, &event))
		return 1;

	return 0;
}

/*
 * Open up the mouse device.
 */
static int
mallegro_Open(MOUSEDEVICE *pmd)
{
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the mouse device.
 */
static void
mallegro_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
mallegro_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

/*
 * Get default mouse acceleration settings
 */
static void mallegro_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/*
 * Read Mouse, non-blocking.
 */
static int
mallegro_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	int buttons = 0;
	int mickeyz = 0;
	ALLEGRO_EVENT event;
	ALLEGRO_MOUSE_STATE mstate;
	static int mz;

    if (!al_get_next_event(allegro_mouqueue, &event))
		return 0;

	switch(event.type) {
    case ALLEGRO_EVENT_MOUSE_AXES:
    case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
    case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        break;

    case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
		al_hide_mouse_cursor(allegro_display);		/* turn off allegro cursor*/
        return MOUSE_NODATA;

    case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
		al_show_mouse_cursor(allegro_display);		/* turn on allegro cursor*/
        return MOUSE_NODATA;

    default:
        return MOUSE_NODATA;
	}

	al_get_mouse_state(&mstate);		/* above call doesn't return button press*/
	*dx = mstate.x / allegro_zoom;		/* divide to get unzoomed postion*/
	*dy = mstate.y / allegro_zoom;
    *dz = 0;
	mstate.z /= allegro_zoom;

	if (mstate.buttons & 1)		/* Primary (e.g. left) mouse button is held*/
		buttons |= MWBUTTON_L;
	if (mstate.buttons & 2)		/* Secondary (e.g. right) mouse button is held*/
		buttons |= MWBUTTON_R;
	if (mstate.buttons & 4)		/* Tertiary (e.g. middle) mouse button is held*/
		buttons |= MWBUTTON_M;

	/* calculate wheel button up/down offset from last position*/
	if(mstate.z != mz)
	    mickeyz = mstate.z - mz;
	else
	    mickeyz = 0;
	mz = mstate.z;
	if (mickeyz > 0)
		buttons |= MWBUTTON_SCROLLUP;
	if (mickeyz < 0)
		buttons |= MWBUTTON_SCROLLDN;

	*bp = buttons;
	return MOUSE_ABSPOS; 		// return absolute mouse position
}
