/*
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 * Author: Tony Rogvall <tony@bluetail.com>
 *
 * Converted to scancode mode by Greg Haerr
 *
 * X11 Keyboard driver
 */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include "device.h"

static int  kbdX11_Open(KBDDEVICE *pkd);
static void kbdX11_Close(void);
static void kbdX11_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  kbdX11_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);

static int init_modstate(void);

static MWKEYMOD key_modstate;

extern Display*     x11_dpy;
extern int          x11_scr;
extern Visual*      x11_vis;
extern Window       x11_win;
extern GC           x11_gc;
int          x11_setup_display(void);

#define X_SCR_MASK 0x80
#define X_CAP_MASK 0x2
#define X_NUM_MASK 0x10

KBDDEVICE kbddev = {
    kbdX11_Open,
    kbdX11_Close,
    kbdX11_GetModifierInfo,
    kbdX11_Read,
    NULL
};

/*
 * Open the keyboard.
 */
static int
kbdX11_Open(KBDDEVICE *pkd)
{
    if (x11_setup_display() < 0) {
		EPRINTF("nano-X: Can't connect to X11 server\n");
		return DRIVER_OKNULLDEV;	/* don't display another errmsg*/
    }

    if(init_modstate() < 0)
    	return DRIVER_FAIL;
    
    /* return the x11 file descriptor for select */
    return DRIVER_OKFILEDESC(ConnectionNumber(x11_dpy));
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
kbdX11_Close(void)
{
    /* nop */
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
kbdX11_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
		*curmodifiers = key_modstate;
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the modifier keys (ALT, SHIFT, etc).  Returns -1 on error, 0 if no data
 * is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
kbdX11_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	XEvent ev;
	MWKEY mwkey;
	char ignored_char;
	unsigned int mods;
	Window w;
	int i;

	static int grabbed = 0;
	static int x11_accel_num;
	static int x11_accel_den;
	static int x11_thres;

    /* check if we have a KeyPressedEvent */
    if (XCheckMaskEvent(x11_dpy, KeyPressMask|KeyReleaseMask, &ev)) {
	KeySym sym = XKeycodeToKeysym(x11_dpy, ev.xkey.keycode, 0);
	if (sym == NoSymbol)
	    return KBD_FAIL;		/* FAIL rather than NODATA will give GsError on unmapped keys*/

	/* calculate kbd modifiers*/
	key_modstate = mods = 0;
	XQueryPointer(x11_dpy, x11_win, &w, &w, &i, &i, &i, &i, &mods);
	if (mods & ControlMask)
		key_modstate |= MWKMOD_CTRL; 
	if (mods & ShiftMask)
		key_modstate |= MWKMOD_SHIFT;
	if (mods & (Mod1Mask | 0x2000))		// OSX alt/option returns 0x2000
		key_modstate |= MWKMOD_ALT;
	if (mods & X_CAP_MASK)
		key_modstate |= MWKMOD_CAPS;
	if (mods & X_SCR_MASK)
		key_modstate |= MWKMOD_SCR;
	if (mods & X_NUM_MASK)
		key_modstate |= MWKMOD_NUM;

	if (sym == XK_Escape) {
	    mwkey = MWKEY_ESCAPE;

	    if (mods & ControlMask) {
		/* toggle grab control */
		if (grabbed) {
		    XUngrabPointer(x11_dpy, CurrentTime);
		    XUngrabKeyboard(x11_dpy, CurrentTime);
		    XChangePointerControl(x11_dpy, True, False, x11_accel_num, 
			  x11_accel_den, 0);
		    grabbed = 0;
		}
		else {
		    /* save pointer config */
		    XGetPointerControl(x11_dpy, &x11_accel_num, &x11_accel_den,
			    &x11_thres);
		    XChangePointerControl(x11_dpy, True, False, 1, 1, 0);
		    XGrabKeyboard(x11_dpy, x11_win,
			    True,  /* only to this window */
			    GrabModeAsync, GrabModeAsync, CurrentTime);
		    XGrabPointer(x11_dpy, x11_win, False,
				 PointerMotionMask | ButtonPressMask,
				 GrabModeAsync, GrabModeAsync, None, None,
				 CurrentTime);
		    grabbed = 1;
		}
		return KBD_NODATA;
	    } else if (grabbed)
		XChangePointerControl(x11_dpy, True, False, x11_accel_num, 
		      x11_accel_den, 0);
	    *kbuf = mwkey;
	    *modifiers = key_modstate;
	    *scancode = ev.xkey.keycode;
	    return (ev.xkey.type == KeyPress)? KBD_KEYPRESS: KBD_KEYRELEASE;
	} else {
	    switch (sym) {
	    case XK_Delete:
		    mwkey = MWKEY_DELETE;
		    break;
	    case XK_Home:
		    mwkey = MWKEY_HOME;
		    break;
	    case XK_Left:
		    mwkey = MWKEY_LEFT;
		    break;
	    case XK_Up:
		    mwkey = MWKEY_UP;
		    break;
	    case XK_Right:
		    mwkey = MWKEY_RIGHT;
		    break;
	    case XK_Down:
		    mwkey = MWKEY_DOWN;
		    break;
	    case XK_Page_Up:
		    mwkey = MWKEY_PAGEUP;
		    break;
	    case XK_Page_Down:
		    mwkey = MWKEY_PAGEDOWN;
		    break;
	    case XK_End:
		    mwkey = MWKEY_END;
		    break;
	    case XK_Insert:
		    mwkey = MWKEY_INSERT;
		    break;
	    case XK_Pause:
	    case XK_Break:
	    case XK_F15:
		    mwkey = MWKEY_QUIT;
		    break;
	    case XK_Print:
	    case XK_Sys_Req:
		    mwkey = MWKEY_PRINT;
		    break;
	    case XK_Menu:
		    mwkey = MWKEY_MENU;
		    break;
	    case XK_Cancel:
		    mwkey = MWKEY_CANCEL;
		    break;
	    case XK_KP_Enter:
		    mwkey = MWKEY_KP_ENTER;
		    break;
	    case XK_KP_Home:
		    mwkey = MWKEY_KP7;
		    break;
	    case XK_KP_Left:
		    mwkey = MWKEY_KP4;
		    break;
	    case XK_KP_Up:
		    mwkey = MWKEY_KP8;
		    break;
	    case XK_KP_Right:
		    mwkey = MWKEY_KP6;
		    break;
	    case XK_KP_Down:
		    mwkey = MWKEY_KP2;
		    break;
	    case XK_KP_Page_Up:
		    mwkey = MWKEY_KP9;
		    break;
	    case XK_KP_Page_Down:
		    mwkey = MWKEY_KP3;
		    break;
	    case XK_KP_End:
		    mwkey = MWKEY_KP1;
		    break;
	    case XK_KP_Insert:
		    mwkey = MWKEY_KP0;
		    break;
	    case XK_KP_Delete:
		    mwkey = MWKEY_KP_PERIOD;
		    break;
	    case XK_KP_Equal:
		    mwkey = MWKEY_KP_EQUALS;
		    break;
	    case XK_KP_Multiply:
		    mwkey = MWKEY_KP_MULTIPLY;
		    break;
	    case XK_KP_Add:
		    mwkey = MWKEY_KP_PLUS;
		    break;
	    case XK_KP_Subtract:
		    mwkey = MWKEY_KP_MINUS;
		    break;
	    case XK_KP_Decimal:
		    mwkey = MWKEY_KP_PERIOD;
		    break;
	    case XK_KP_Divide:
		    mwkey = MWKEY_KP_DIVIDE;
		    break;
	    case XK_KP_5:
	    case XK_KP_Begin:
		    mwkey = MWKEY_KP5;
		    break;
	    case XK_F1:
		    mwkey = MWKEY_F1;
		    break;
	    case XK_F2:
		    mwkey = MWKEY_F2;
		    break;
	    case XK_F3:
		    mwkey = MWKEY_F3;
		    break;
	    case XK_F4:
		    mwkey = MWKEY_F4;
		    break;
	    case XK_F5:
		    mwkey = MWKEY_F5;
		    break;
	    case XK_F6:
		    mwkey = MWKEY_F6;
		    break;
	    case XK_F7:
		    mwkey = MWKEY_F7;
		    break;
	    case XK_F8:
		    mwkey = MWKEY_F8;
		    break;
	    case XK_F9:
		    mwkey = MWKEY_F9;
		    break;
	    case XK_F10:
		    mwkey = MWKEY_F10;
		    break;
	    case XK_F11:
		    mwkey = MWKEY_F11;
		    break;
	    case XK_F12:
		    mwkey = MWKEY_F12;
		    break;

	    /* state modifiers*/
	    case XK_Num_Lock:
		    /* not sent, used only for state*/
		    if (ev.xkey.type == KeyRelease)
		    	key_modstate ^= MWKMOD_NUM;
		    return KBD_NODATA;
	    case XK_Shift_Lock:
	    case XK_Caps_Lock:
		    /* not sent, used only for state*/
		    if (ev.xkey.type == KeyRelease)
		    	key_modstate ^= MWKMOD_CAPS;
		    return KBD_NODATA;
	    case XK_Scroll_Lock:
		    /* not sent, used only for state*/
		    if (ev.xkey.type == KeyRelease)
		    	key_modstate ^= MWKMOD_SCR;
		    return KBD_NODATA;
		    break;
	    case XK_Shift_L:
		    mwkey = MWKEY_LSHIFT;
		    break;
	    case XK_Shift_R:
		    mwkey = MWKEY_RSHIFT;
		    break;
	    case XK_Control_L:
		    mwkey = MWKEY_LCTRL;
		    break;
	    case XK_Control_R:
		    mwkey = MWKEY_RCTRL;
		    break;
	    case XK_Alt_L:
		case XK_Mode_switch:				/* MACOSX left or right option/alt*/
		    mwkey = MWKEY_LALT;
		    break;
	    case XK_Alt_R:
		    mwkey = MWKEY_RALT;
		    break;
	    case XK_Meta_L:
	    case XK_Super_L:
	    case XK_Hyper_L:
		    mwkey = MWKEY_LMETA;
		    break;
	    case XK_Meta_R:
	    case XK_Super_R:
	    case XK_Hyper_R:
		    mwkey = MWKEY_RMETA;
		    break;
	    default:
		    switch (sym) {
		    case XK_BackSpace:
		    case XK_Tab:
		    case XK_Return:
			break;
		    default:
		    	if (sym & 0xFF00)
			    	EPRINTF("Unhandled X11 keysym: %04x\n", (int)sym);
		    }

		    XLookupString(&ev.xkey, &ignored_char, 1, &sym, NULL );

		    if (key_modstate & MWKMOD_CTRL)
				mwkey = sym & 0x1f;	/* Control code */ 
			else
				mwkey = sym & 0xff;	/* ASCII*/

		    break;

	    }
	    if (key_modstate & MWKMOD_NUM) {
		switch (mwkey) {
		case MWKEY_KP0:
		case MWKEY_KP1:
		case MWKEY_KP2:
		case MWKEY_KP3:
		case MWKEY_KP4:
		case MWKEY_KP5:
		case MWKEY_KP6:
		case MWKEY_KP7:
		case MWKEY_KP8:
		case MWKEY_KP9:
			mwkey = mwkey - MWKEY_KP0 + '0';
			break;
		case MWKEY_KP_PERIOD:
			mwkey = '.';
			break;
		case MWKEY_KP_DIVIDE:
			mwkey = '/';
			break;
		case MWKEY_KP_MULTIPLY:
			mwkey = '*';
			break;
		case MWKEY_KP_MINUS:
			mwkey = '-';
			break;
		case MWKEY_KP_PLUS:
			mwkey = '+';
			break;
		case MWKEY_KP_ENTER:
			mwkey = MWKEY_ENTER;
			break;
		case MWKEY_KP_EQUALS:
			mwkey = '-';
			break;
		}
	    }
	    *modifiers = key_modstate;
	    *scancode = ev.xkey.keycode;
	    *kbuf = mwkey;

	    /*printf("mods: 0x%x  scan: 0x%x  key: 0x%x\n",*modifiers,*scancode, *kbuf);*/
	    return (ev.xkey.type == KeyPress)? KBD_KEYPRESS : KBD_KEYRELEASE;
	}
    }
    return KBD_NODATA;
}

#define NUM_LOCK_MASK    0x00000002
#define CAPS_LOCK_MASK   0x00000001
#define SCROLL_LOCK_MASK 0x00000004

/* initialise key_modstate */ 
static int init_modstate(void)
{
	unsigned int state;
	int capsl, numl, scrolll;

	if(XkbGetIndicatorState (x11_dpy, XkbUseCoreKbd, &state) != Success) {
		EPRINTF("nano-X: Error reading kbd indicator status\n");
		return 0;	/* no error*/
	} 
	capsl = state & CAPS_LOCK_MASK;
	numl = state & NUM_LOCK_MASK;
	scrolll = state & SCROLL_LOCK_MASK;

	if(numl != 0)
		key_modstate |= MWKMOD_NUM;

	if(capsl != 0)
		key_modstate |= MWKMOD_CAPS;

	if(scrolll != 0)
		key_modstate |= MWKMOD_SCR;

	return 0;
}
/* vim: set ts=8: */
