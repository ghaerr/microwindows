/*
 * Copyright (c) 2000, 2005, 2019 Greg Haerr <greg@censoft.com>
 *
 * Microsoft Windows Keyboard Driver
 */

#include <string.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "..\include\device.h"

static int  winkbd_Open(KBDDEVICE *pkd);
static void winkbd_Close(void);
static void winkbd_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  winkbd_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  winkbd_Poll(void);

extern HWND mwAppWindow;

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

	if (PeekMessage(&msg, mwAppWindow, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE))
		return 1;

	return 0;
}

static int keytrans[] = {
	VK_NUMPAD0,		MWKEY_KP0,
	VK_NUMPAD1,		MWKEY_KP1,
	VK_NUMPAD2,		MWKEY_KP2,
	VK_NUMPAD3,		MWKEY_KP3,
	VK_NUMPAD4,		MWKEY_KP4,
	VK_NUMPAD5,		MWKEY_KP5,
	VK_NUMPAD6,		MWKEY_KP6,
	VK_NUMPAD7,		MWKEY_KP7,
	VK_NUMPAD8,		MWKEY_KP8,
	VK_NUMPAD9,		MWKEY_KP9,
	VK_ADD,			MWKEY_KP_PLUS,
	VK_SUBTRACT,	MWKEY_KP_MINUS,
	VK_MULTIPLY,	MWKEY_KP_MULTIPLY,
	VK_DIVIDE,		MWKEY_KP_DIVIDE,
	VK_DECIMAL,		MWKEY_KP_PERIOD,
	//VK_RETURN,		MWKEY_KP_ENTER,
	VK_F1,			MWKEY_F1,
	VK_F2,			MWKEY_F2,
	VK_F3,			MWKEY_F3,
	VK_F4,			MWKEY_F4,
	VK_F5,			MWKEY_F5,
	VK_F6,			MWKEY_F6,
	VK_F7,			MWKEY_F7,
	VK_F8,			MWKEY_F8,
	VK_F9,			MWKEY_F9,
	VK_F10,			MWKEY_F10,
	VK_F11,			MWKEY_F11,
	VK_F12,			MWKEY_F12,
	VK_INSERT,		MWKEY_INSERT,
	VK_DELETE,		MWKEY_DELETE,
	VK_HOME,		MWKEY_HOME,
	VK_END,			MWKEY_END,
	VK_PRIOR,		MWKEY_PAGEUP,
	VK_NEXT,		MWKEY_PAGEDOWN,
	VK_LEFT,		MWKEY_LEFT,
	VK_RIGHT,		MWKEY_RIGHT,
	VK_UP,			MWKEY_UP,
	VK_DOWN,		MWKEY_DOWN,
	VK_PRINT,		MWKEY_PRINT,
	VK_PAUSE,		MWKEY_QUIT,
	VK_SELECT,		MWKEY_SELECTDOWN,
	VK_LSHIFT,		MWKEY_LSHIFT,
	VK_RSHIFT,		MWKEY_RSHIFT,
	VK_LCONTROL,	MWKEY_LCTRL,
	VK_RCONTROL,	MWKEY_RCTRL,
	VK_LWIN,		MWKEY_LALT,
	VK_RWIN,		MWKEY_RALT,
	VK_LMENU,		MWKEY_MENU,
	VK_SCROLL,		MWKEY_SCROLLOCK,
	VK_NUMLOCK,		MWKEY_NUMLOCK,
	VK_CAPITAL,		MWKEY_CAPSLOCK,
	0
};

static int
fnkey_convert(int key)
{
	int *kp = keytrans;

	while (*kp)
	{
		if (key == *kp)
			return *(kp + 1);
		kp += 2;
	}
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

	if (!PeekMessage(&msg, mwAppWindow, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
		return KBD_NODATA;

	/* translate WM_KEYDOWN/UP to WM_CHAR*/
	TranslateMessage(&msg);

	//FIXME add kbd modifier reporting
	*modifiers = 0;

//GdError("key message %d\n", msg.message);
	switch (msg.message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		mwkey = fnkey_convert((int)msg.wParam);
		if (!mwkey)
				return KBD_NODATA;
		*kbuf = mwkey;
		*scancode = (msg.lParam >> 16) & 0xff;
		return KBD_KEYPRESS;

	case WM_CHAR:
		mwkey = (MWKEY)msg.wParam; 		/* 16 bit ascii or unicode char*/
		*kbuf = mwkey;
		*scancode = (msg.lParam >> 16) & 0xff;
//GdError("key %d\n", mwkey);
		return KBD_KEYPRESS;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		mwkey = fnkey_convert((int)msg.wParam);
		if (!mwkey)
				return KBD_NODATA;
		*kbuf = mwkey;
		*scancode = (msg.lParam >> 16) & 0xff;
		return KBD_KEYRELEASE;
	}

    return KBD_NODATA;
}
