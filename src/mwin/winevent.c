/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Graphics server event routines for windows.
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !(DOS_TURBOC | DOS_QUICKC | _MINIX | VXWORKS)
static int
abs(int n)
{
	return n >= 0? n: -n;
}
#endif

/*
 * Update mouse status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the mouse.
 */
BOOL
MwCheckMouseEvent(void)
{
	MWCOORD		rootx;		/* latest mouse x position */
	MWCOORD		rooty;		/* latest mouse y position */
	int		newbuttons;	/* latest buttons */
	int		mousestatus;	/* latest mouse status */

	/* Read the latest mouse status: */
	mousestatus = GdReadMouse(&rootx, &rooty, &newbuttons);
	if(mousestatus < 0) {
		/*MwError(GR_ERROR_MOUSE_ERROR, 0);*/
		return FALSE;
	} else if(mousestatus) {	/* Deliver events as appropriate: */	
		MwHandleMouseStatus(rootx, rooty, newbuttons);
		return TRUE;
	}
	return FALSE;
}

/*
 * Update keyboard status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the keyboard.
 */
BOOL
MwCheckKeyboardEvent(void)
{
	MWKEY	 	mwkey;		/* latest character */
	MWKEYMOD 	modifiers;	/* latest modifiers */
	MWSCANCODE	scancode;
	int	 	keystatus;	/* latest keyboard status */

	/* Read the latest keyboard status: */
	keystatus = GdReadKeyboard(&mwkey, &modifiers, &scancode);
	if(keystatus < 0) {
		if(keystatus == -2)	/* special case for ESC pressed*/
			MwTerminate();
		/*MwError(GR_ERROR_KEYBOARD_ERROR, 0);*/
		return FALSE;
	} else if(keystatus) {		/* Deliver events as appropriate: */	
		switch (mwkey) {
		case MWKEY_QUIT:
			MwTerminate();
			/* no return*/
		case MWKEY_REDRAW:
			MwRedrawScreen();
			break;
		case MWKEY_PRINT:
			if (keystatus == 1)
				GdCaptureScreen("screen.bmp");
			break;
		}
		MwDeliverKeyboardEvent(mwkey, modifiers, scancode,
			keystatus==1? TRUE: FALSE);
		return TRUE;
	}
	return FALSE;
}

/*
 * Handle all mouse events.  These are mouse enter, mouse exit, mouse
 * motion, mouse position, button down, and button up.  This also moves
 * the cursor to the new mouse position and changes it shape if needed.
 */
void
MwHandleMouseStatus(MWCOORD newx, MWCOORD newy, int newbuttons)
{
	int		changebuttons;	/* buttons that have changed */
	MWKEYMOD	modifiers;	/* latest modifiers */
	static int curbuttons;

	GdGetModifierInfo(NULL, &modifiers); /* Read kbd modifiers */

	/*
	 * First, if the mouse has moved, then position the cursor to the
	 * new location, which will send mouse enter, mouse exit, focus in,
	 * and focus out events if needed.  Check here for mouse motion and
	 * mouse position events.
	 */
	if (newx != cursorx || newy != cursory) {
		MwMoveCursor(newx, newy);
		MwDeliverMouseEvent(newbuttons, 0, modifiers);
	}

	/*
	 * Next, generate a button up event if any buttons have been released.
	 */
	changebuttons = (curbuttons & ~newbuttons);
	if (changebuttons)
		MwDeliverMouseEvent(newbuttons, changebuttons, modifiers);

	/*
	 * Finally, generate a button down event if any buttons have been
	 * pressed.
	 */
	changebuttons = (~curbuttons & newbuttons);
	if (changebuttons)
		MwDeliverMouseEvent(newbuttons, changebuttons, modifiers);

	curbuttons = newbuttons;
}

/*
 * Translate and deliver hardware mouse message to proper window.
 */
void
MwTranslateMouseMessage(HWND hwnd,UINT msg,int hittest)
{
	POINT		pt;
	DWORD		tick;
	static UINT	lastmsg = 0;
	static HWND	lasthwnd;
	static DWORD	lasttick;
	static int	lastx, lasty;

	/* determine double click eligibility*/
	if(msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN) {
		tick = GetTickCount();
		if((hwnd->pClass->style & CS_DBLCLKS) &&
		    msg == lastmsg && hwnd == lasthwnd &&
		    tick - lasttick < DBLCLICKSPEED &&
		    abs(cursorx-lastx) < mwSYSMETRICS_CXDOUBLECLK &&
		    abs(cursory-lasty) < mwSYSMETRICS_CYDOUBLECLK)
			msg += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
		lastmsg = msg;
		lasthwnd = hwnd;
		lasttick = tick;
		lastx = cursorx;
		lasty = cursory;
	}

	/*
	 * We always send nc mouse message
	 * unlike Windows, for HTCLIENT default processing
	 */
	PostMessage(hwnd, msg + (WM_NCMOUSEMOVE-WM_MOUSEMOVE), hittest,
		MAKELONG(cursorx, cursory));

	/* then possibly send user mouse message*/
	if(hittest == HTCLIENT) {
		pt.x = cursorx;
		pt.y = cursory;
		ScreenToClient(hwnd, &pt);
		PostMessage(hwnd, msg, 0, MAKELONG(pt.x, pt.y));
	}
}

/*
 * Deliver a mouse button or motion event.
 */
int mwCurrentButtons;

void 
MwDeliverMouseEvent(int buttons, int changebuttons, MWKEYMOD modifiers)
{
	HWND	hwnd;
	int	hittest;
	UINT	msg;

	mwCurrentButtons = buttons;

	hwnd = GetCapture();
	if(!hwnd)
		hwnd = mousewp;
	hittest = SendMessage(hwnd, WM_NCHITTEST, 0, MAKELONG(cursorx,cursory));

	if(!changebuttons)
		MwTranslateMouseMessage(hwnd, WM_MOUSEMOVE, hittest);

	if(changebuttons & MWBUTTON_L) {
		msg = (buttons&MWBUTTON_L)? WM_LBUTTONDOWN: WM_LBUTTONUP;
		MwTranslateMouseMessage(hwnd, msg, hittest);
	}

	if(changebuttons & MWBUTTON_M) {
		msg = (buttons&MWBUTTON_M)? WM_MBUTTONDOWN: WM_MBUTTONUP;
		MwTranslateMouseMessage(hwnd, msg, hittest);
	}

	if(changebuttons & MWBUTTON_R) {
		msg = (buttons&MWBUTTON_R)? WM_RBUTTONDOWN: WM_RBUTTONUP;
		MwTranslateMouseMessage(hwnd, msg, hittest);
	}
}

/*
 * Deliver a keyboard event.
 */
void
MwDeliverKeyboardEvent(MWKEY keyvalue, MWKEYMOD modifiers, MWSCANCODE scancode,
	BOOL pressed)
{
	WPARAM VK_Code = keyvalue;	/* default no translation*/

	/* Keysyms from 1-255 are mapped to ASCII*/
	if (keyvalue < 1 || keyvalue > 255)
	  switch(keyvalue) {

	/* arrows + home/end pad*/
	case MWKEY_LEFT:
		VK_Code = VK_LEFT;
		break;
	case MWKEY_RIGHT:
		VK_Code = VK_RIGHT;
		break;
	case MWKEY_UP:
		VK_Code = VK_UP;
		break;
	case MWKEY_DOWN:
		VK_Code =  VK_DOWN;
		break;
	case MWKEY_INSERT:
		VK_Code = VK_INSERT;
		break;
	case MWKEY_DELETE:
		VK_Code = VK_DELETE;
		break;
	case MWKEY_HOME:
		VK_Code = VK_HOME;
		break;
	case MWKEY_END:
		VK_Code = VK_END;
		break;
	case MWKEY_PAGEUP:
		VK_Code = VK_PRIOR;
		break;
	case MWKEY_PAGEDOWN:
		VK_Code = VK_NEXT;
		break;

	/* Numeric keypad*/
	case MWKEY_KP0:
		VK_Code = VK_NUMPAD0;
		break;
	case MWKEY_KP1:
		VK_Code = VK_NUMPAD1;
		break;
	case MWKEY_KP2:
		VK_Code = VK_NUMPAD2;
		break;
	case MWKEY_KP3:
		VK_Code = VK_NUMPAD3;
		break;
	case MWKEY_KP4:
		VK_Code = VK_NUMPAD4;
		break;
	case MWKEY_KP5:
		VK_Code = VK_NUMPAD5;
		break;
	case MWKEY_KP6:
		VK_Code = VK_NUMPAD6;
		break;
	case MWKEY_KP7:
		VK_Code = VK_NUMPAD7;
		break;
	case MWKEY_KP8:
		VK_Code = VK_NUMPAD8;
		break;
	case MWKEY_KP9:
		VK_Code = VK_NUMPAD9;
		break;
	case MWKEY_KP_PERIOD:
		VK_Code = VK_DECIMAL;
		break;
	case MWKEY_KP_DIVIDE:
		VK_Code = VK_DIVIDE;
		break;
	case MWKEY_KP_MULTIPLY:
		VK_Code = VK_MULTIPLY;
		break;
	case MWKEY_KP_MINUS:
		VK_Code = VK_SUBTRACT;
		break;
	case MWKEY_KP_PLUS:
		VK_Code = VK_ADD;
		break;
	case MWKEY_KP_ENTER:
		VK_Code = VK_RETURN;
		break;
	
	/* Function keys */
	case MWKEY_F1:
		VK_Code = VK_F1;
		break;
	case MWKEY_F2:
		VK_Code = VK_F2;
		break;
	case MWKEY_F3:
		VK_Code = VK_F3;
		break;
	case MWKEY_F4:
		VK_Code = VK_F4;
		break;
	case MWKEY_F5:
		VK_Code = VK_F5;
		break;
	case MWKEY_F6:
		VK_Code = VK_F6;
		break;
	case MWKEY_F7:
		VK_Code = VK_F7;
		break;
	case MWKEY_F8:
		VK_Code = VK_F8;
		break;
	case MWKEY_F9:
		VK_Code = VK_F9;
		break;
	case MWKEY_F10:
		VK_Code = VK_F10;
		break;
	case MWKEY_F11:
		VK_Code = VK_F11;
		break;
	case MWKEY_F12:
		VK_Code = VK_F12;
		break;

	/* Key state modifier keys*/
	case MWKEY_NUMLOCK:
		VK_Code = VK_NUMLOCK;
		break;
	case MWKEY_CAPSLOCK:
		VK_Code = VK_CAPITAL;
		break;
	case MWKEY_SCROLLOCK:
		VK_Code = VK_CAPITAL;
		break;
	case MWKEY_LSHIFT:
		VK_Code = VK_LSHIFT;
		break;
	case MWKEY_RSHIFT:
		VK_Code = VK_RSHIFT;
		break;
	case MWKEY_LCTRL:
		VK_Code = VK_LCONTROL;
		break;
	case MWKEY_RCTRL:
		VK_Code = VK_RCONTROL;
		break;
	case MWKEY_LALT:
		VK_Code = VK_MENU;
		break;
	case MWKEY_RALT:
		VK_Code = VK_MENU;
		break;

	/* Misc function keys*/
	case MWKEY_PRINT:
		VK_Code = VK_PRINT;
		break;
	case MWKEY_PAUSE:
		VK_Code = VK_PAUSE;
		break;
	case MWKEY_MENU:
		VK_Code = VK_LMENU;	/* virtual key*/
		break;

	/* questionable mappings or no mappings...*/
	case MWKEY_KP_EQUALS:
		VK_Code = '=';	/* FIXME*/
		break;

	/* map all non-handled MWKEY values to VK_NONAME*/
#if 0
	case MWKEY_UNKNOWN:
	case MWKEY_SYSREQ:
	case MWKEY_BREAK
	case MWKEY_QUIT:
	case MWKEY_REDRAW:
	case MWKEY_LMETA:
	case MWKEY_RMETA:
	case MWKEY_ALTGR:
	/* Handheld function keys*/
	case MWKEY_RECORD:
	case MWKEY_PLAY:
	case MWKEY_CONTRAST:
	case MWKEY_BRIGHTNESS:
	case MWKEY_SELECTUP:
	case MWKEY_SELECTDOWN:
	case MWKEY_ACCEPT:
	case MWKEY_CANCEL:
	case MWKEY_APP1:
	case MWKEY_APP2:
	case MWKEY_LAST:
#endif
	default:
		VK_Code = VK_NONAME;
		break;
	}

	if (pressed)
		SendMessage(focuswp, WM_CHAR, VK_Code, 0L);
}

/*
 * Deliver a window expose event.
 * Most of the work is in calculating the update region
 * for better redraw look and feel, and then queuing a
 * WM_PAINT message to the window.
 */
void
MwDeliverExposureEvent(HWND wp, MWCOORD x, MWCOORD y, MWCOORD width,
	MWCOORD height)
{
	if (wp->unmapcount)
		return;

	MwUnionUpdateRegion(wp, x, y, width, height, TRUE);
	PostMessage(wp, WM_PAINT, 0, 0L);
}

/*
 * Combine the passed rectangle with the update region for the given window.
 * Coordinates are passed relative to window.
 * If bUnion is TRUE, union the rectangle, otherwise subtract it.
 */
void
MwUnionUpdateRegion(HWND wp, MWCOORD x, MWCOORD y, MWCOORD width,
	MWCOORD height, BOOL bUnion)
{
#if UPDATEREGIONS
	MWRECT rc;

	if (wp->unmapcount)
		return;

	/* convert window relative coords to screen coords*/
	rc.left = x + wp->winrect.left;
	rc.top = y + wp->winrect.top;
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;

	if(bUnion)
		GdUnionRectWithRegion(&rc, wp->update);
	else
		GdSubtractRectFromRegion(&rc, wp->update);
#endif
}

/*
 * Move the cursor to the specified absolute screen coordinates.
 * The coordinates are that of the defined hot spot of the cursor.
 * The cursor's appearance is changed to that defined for the window
 * in which the cursor is moved to.  In addition, the window the
 * cursor is in is recalculated.
 */
void MwMoveCursor(MWCOORD x, MWCOORD y)
{
	/*
	 * Move the cursor only if necessary, offsetting it to
	 * place the hot spot at the specified coordinates.
	 */
	if (x != cursorx || y != cursory) {
		if(curcursor)
			GdMoveCursor(x - curcursor->cursor.hotx,
				y - curcursor->cursor.hoty);
		cursorx = x;
		cursory = y;
	}

	/*
	 * Now check to see which window the mouse is in and whether or
	 * not the cursor shape should be changed.
	 */
	MwCheckMouseWindow();
	MwCheckCursor();
}

/*
 * Check to see if the cursor shape is the correct shape for its current
 * location.  If not, its shape is changed.
 */
void MwCheckCursor(void)
{
	HWND 		wp;		/* window cursor is in */
	HCURSOR		cp;		/* cursor definition */

	/*
	 * Get the cursor at its current position, and if it is not the
	 * currently defined one, then set the new cursor.  However,
	 * if the window is currently captured, then leave it alone.
	 */
	wp = capturewp;
	if (wp == NULL)
		wp = mousewp;

	cp = wp->cursor;
	if (cp == curcursor)
		return;

	/*
	 * It needs redefining, so do it.
	 */
	curcursor = cp;
	GdMoveCursor(cursorx - cp->cursor.hotx, cursory - cp->cursor.hoty);
	GdSetCursor(&cp->cursor);
}

/*
 * Find the window which is currently visible for the specified coordinates.
 * This just walks down the window tree looking for the deepest mapped
 * window which contains the specified point.  If the coordinates are
 * off the screen, the root window is returned.
 */
HWND
MwFindVisibleWindow(MWCOORD x, MWCOORD y)
{
	HWND	wp;		/* current window */
	HWND	retwp;		/* returned window */

	wp = rootwp;
	retwp = wp;
	while (wp) {
		if (!wp->unmapcount &&
		    wp->winrect.left <= x && wp->winrect.top <= y &&
		    wp->winrect.right > x && wp->winrect.bottom > y) {
			retwp = wp;
			wp = wp->children;
			continue;
		}
		wp = wp->siblings;
	}
	return retwp;
}

/*
 * Check to see if the window the mouse is currently in has changed.
 */
void MwCheckMouseWindow(void)
{
	HWND	wp;

	/* Don't change if window drag or capture in progress*/
	wp = dragwp;
	if(!wp)
		wp = capturewp;
	if(!wp)
		wp = MwFindVisibleWindow(cursorx, cursory);
	mousewp = wp;
}

/*
 *  Copy dstsiz bytes, including nul, from src to dst.
 *  Return # bytes, excluding nul, copied.
 */
int
strzcpy(char *dst,const char *src,int dstsiz)
{
	int	cc = dstsiz;

	/* return 0 on NULL src*/
	if(!src)
		cc = dstsiz = 1;

	while(--dstsiz > 0) {
		if((*dst++ = *src++) == '\0')
			return cc - dstsiz - 1;
	}
	*dst = 0;
	return cc - dstsiz - 1;
}
