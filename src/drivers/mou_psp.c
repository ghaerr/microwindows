/*
 * Crappy PSP analog stick to mouse driver by Jim Paris
 *	Enhanced by lurker0
 *
 * Mouse:
 * Analog stick moves
 * X = left click
 * Square = left click
 * Circle = right click
 * Triangle = middle click
 * 
 * Keyboard:
 * L Trigger = escape
 * R Trigger = enter
 * Left = letter L
 * Right = letter R
 * Up = letter U
 * Down = letter D
 * Select = backspace
 * Start = quit
 */

#include <psputils.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>

#include "device.h"

#define	SCALE	2	/* default scaling factor for acceleration WAS 3*/
#define	THRESH	8	/* default threshhold for acceleration WAS 0*/

static int  	MOU_Open(MOUSEDEVICE *pmd);
static void 	MOU_Close(void);
static int  	MOU_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	MOU_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz,int *bp);
static int	MOU_Poll(void);

MOUSEDEVICE mousedev = {
	MOU_Open,
	MOU_Close,
	MOU_GetButtonInfo,
	MOU_GetDefaultAccel,
	MOU_Read,
	MOU_Poll
};

int psp_keypress;  /* for keyboard driver */

/*
 * Open up the mouse device.
 */
static int
MOU_Open(MOUSEDEVICE *pmd)
{
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);

	psp_keypress = 0;
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the mouse device.
 */
static void
MOU_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
MOU_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
MOU_GetDefaultAccel(int *pscale,int *pthresh)
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
MOU_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	SceCtrlData pad;
	int new_keypress = 0;
        static MWCOORD cord_x=0, cord_y = 0;

	sceCtrlReadBufferPositive(&pad, 1); 

	*dx = cord_x + ((MWCOORD)pad.Lx - 128) / 32;
	*dy = cord_y + ((MWCOORD)pad.Ly - 128) / 32;
	*dz = 0;

	*bp = 0;
	if(pad.Buttons & PSP_CTRL_RTRIGGER)
		*bp |= MWBUTTON_M;

	if(pad.Buttons & PSP_CTRL_SQUARE)
	        *bp |= MWBUTTON_L;

	if(pad.Buttons & PSP_CTRL_CROSS)
		*bp |= MWBUTTON_R;

	if(pad.Buttons & PSP_CTRL_SQUARE)   new_keypress = MWKEY_HOME;
	if(pad.Buttons & PSP_CTRL_CROSS)    new_keypress = MWKEY_END;
	if(pad.Buttons & PSP_CTRL_LEFT) {
            new_keypress = MWKEY_LEFT;
            *dx -= 1;
	}
	if(pad.Buttons & PSP_CTRL_RIGHT) {
            new_keypress = MWKEY_RIGHT;
            *dx += 1;
	}
	if(pad.Buttons & PSP_CTRL_UP) {   
            new_keypress = MWKEY_UP;
	    *dy -= 1;
	}
	if(pad.Buttons & PSP_CTRL_DOWN) {
	    new_keypress = MWKEY_DOWN;
	    *dy += 1;
	}
	if(pad.Buttons & PSP_CTRL_TRIANGLE) new_keypress = MWKEY_PAGEUP;
	if(pad.Buttons & PSP_CTRL_CIRCLE)   new_keypress = MWKEY_PAGEDOWN;

	/* if keyboard driver got a key, only give it more once that
	 key was let go.  This is just a quick hack, there's surely a
	 better way to handle this.. */
	if(psp_keypress < 0 && (-psp_keypress) != new_keypress)
		psp_keypress=0;

	if(psp_keypress==0)
		psp_keypress = new_keypress;

	return MOUSE_RELPOS;
}

#define timerdiff(a,b) ((float)((a).tv_sec - (b).tv_sec) + \
                         (float)((a).tv_usec - (b).tv_usec)/1e6)

static int
MOU_Poll(void)
{
	static struct timeval tv, oldtv;
	int ret;

	gettimeofday(&tv, NULL);

	ret = 0;
	/* Say that we have data at most 100 times per second */
	if(timerdiff(tv, oldtv) > 0.01) {
		ret = 1;
		oldtv = tv;
	}

	return ret;
}
