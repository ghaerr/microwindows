/*
 * Copyright (c) 1999, 2005 Greg Haerr <greg@censoft.com>
 *
 * NULL Mouse Driver
 */
#include <stdio.h>
#include "device.h"
#include <windows.h>
#include <windowsx.h>

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

MSG *winMouseMsg = NULL;

static int  	NUL_Open(MOUSEDEVICE *pmd);
static void 	NUL_Close(void);
static int  	NUL_GetButtonInfo(void);
static void	NUL_GetDefaultAccel(int *pscale,int *pthresh);
static int  	NUL_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	NUL_Poll(void);

MOUSEDEVICE mousedev = {
	NUL_Open,
	NUL_Close,
	NUL_GetButtonInfo,
	NUL_GetDefaultAccel,
	NUL_Read,
	NUL_Poll,
    MOUSE_NORMAL	/* flags*/
};

/*
 * Poll for events
 */

static int
NUL_Poll(void)
{
	return 0;
}

/*
 * Open up the mouse device.
 */
static int
NUL_Open(MOUSEDEVICE *pmd)
{
	return -2;	/* no mouse*/
}

/*
 * Close the mouse device.
 */
static void
NUL_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
NUL_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
NUL_GetDefaultAccel(int *pscale,int *pthresh)
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
NUL_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
    if (winMouseMsg)
    {
        *dx = GET_X_LPARAM(winMouseMsg->lParam);
        *dy = GET_Y_LPARAM(winMouseMsg->lParam);
        *dz = 0;
        *bp = 0;
        if (winMouseMsg->wParam & MK_LBUTTON)
        {
            *bp |= MWBUTTON_L;
        }
        if (winMouseMsg->wParam & MK_MBUTTON)
        {
            *bp |= MWBUTTON_M;
        }
        if (winMouseMsg->wParam & MK_RBUTTON)
        {
            *bp |= MWBUTTON_R;
        }
        return 2;
    }
    return 0;
}
