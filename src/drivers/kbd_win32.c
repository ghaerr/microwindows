/*
 * Copyright (c) 2000, 2005 Greg Haerr <greg@censoft.com>
 *
 * Microsoft Windows Keyboard Driver
 */

#include <string.h>
#include <stdio.h>
#include "device.h"
#include <windows.h>

static int  winkbd_Open(KBDDEVICE *pkd);
static void winkbd_Close(void);
static void winkbd_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  winkbd_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  winkbd_Poll(void);

extern HWND winRootWindow;

KBDDEVICE kbddev = {
	winkbd_Open,
	winkbd_Close,
	winkbd_GetModifierInfo,
	winkbd_Read,
	winkbd_Poll
};

/*
 * Open the keyboard.
 */
static int
winkbd_Open(KBDDEVICE *pkd)
{
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the keyboard.
 */
static void
winkbd_Close(void)
{
	
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
winkbd_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = 0;		/* no modifiers available */
	if (curmodifiers)
		*curmodifiers = 0;
}

/*
 * Poll for keyboard events
 */
static int
winkbd_Poll(void)
{
	MSG	msg;

	if (PeekMessage(&msg, winRootWindow, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE))
		return 1;
	return 0;
}

/*
 * This reads a keystroke event, and the current state of the modifier keys (ALT, SHIFT, etc). 
 * Returns KBD_NODATA, KBD_QUIT, KBD_KEYPRESS or KBD_KEYRELEASE
 * This is a non-blocking call.
 */
static int
winkbd_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	int mwkey;
	MSG msg;

	if (!PeekMessage(&msg, winRootWindow, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
		return MOUSE_NODATA;

	switch (msg.message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		mwkey = msg.wParam; 		/* virtual-key code*/
		*kbuf = mwkey;				// FIXME needs MWKEY_ translation
		*scancode = (msg.lParam >> 16) & 0xff;
		return KBD_KEYPRESS;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		mwkey = msg.wParam; 		/* virtual-key code*/
		*kbuf = mwkey;				// FIXME needs MWKEY_ translation
		*scancode = (msg.lParam >> 16) & 0xff;
		return KBD_KEYRELEASE;
	}

    return KBD_NODATA;
}
