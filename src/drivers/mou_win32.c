/*
 * Copyright (c) 1999, 2005, 2019 Greg Haerr <greg@censoft.com>
 *
 * Microsoft Windows Mouse Driver
 */
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "..\include\device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	winmou_Open(MOUSEDEVICE *pmd);
static void 	winmou_Close(void);
static int  	winmou_GetButtonInfo(void);
static void	winmou_GetDefaultAccel(int *pscale,int *pthresh);
static int  	winmou_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	winmou_Poll(void);

extern HWND mwAppWindow;

MOUSEDEVICE mousedev = {
	winmou_Open,
	winmou_Close,
	winmou_GetButtonInfo,
	winmou_GetDefaultAccel,
	winmou_Read,
	winmou_Poll,
    MOUSE_NORMAL	/* flags*/
};

/*
 * Open up the mouse device.
 */
static int
winmou_Open(MOUSEDEVICE *pmd)
{
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the mouse device.
 */
static void
winmou_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
winmou_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
winmou_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/*
 * Mouse poll entry point
 */
static int
winmou_Poll(void)
{
	MSG msg;

	if (PeekMessage(&msg, mwAppWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE))
		return 1;
	return 0;
}

/*
 * Read mouse event.
 * Returns MOUSE_NODATA or MOUSE_ABSPOS
 * This is a non-blocking call.
 */

static int
winmou_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	MSG msg;

	if (!PeekMessage(&msg, mwAppWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
		return MOUSE_NODATA;

	switch (msg.message) {
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
        *dx = GET_X_LPARAM(msg.lParam);
        *dy = GET_Y_LPARAM(msg.lParam);
        *dz = 0;
        *bp = 0;
        if (msg.wParam & MK_LBUTTON)
            *bp |= MWBUTTON_L;
        if (msg.wParam & MK_MBUTTON)
            *bp |= MWBUTTON_M;
        if (msg.wParam & MK_RBUTTON)
            *bp |= MWBUTTON_R;
        return MOUSE_ABSPOS;
    }
    return MOUSE_NODATA;
}
