/*
 * Copyright (c) 1999, 2000, 2005, 2010 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 *
 * Graphics server event routines for windows.
 *
 * Modifications:
 *  Date		Author			Description
 *	2003/09/24	Gabriele Brugnoni	In MwDeliverMouseEvent, if window is disabled,
 *                                      	(or wnd is child and parent disab), do nothing.
 *
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include <stdlib.h>

static MWKEYMOD mwCurrentModifiers;

static LPFN_KEYBTRANSLATE mwPtrKeyboardTranslator = NULL;

/*
 * Update mouse status and issue events on it if necessary.
 * This function doesn't block, but is only called when Poll() returns TRUE
 * or MwSelect shows some data waiting to be read on the mouse file descriptor.
 */
BOOL
MwCheckMouseEvent(void)
{
	MWCOORD		rootx;		/* latest mouse x position */
	MWCOORD		rooty;		/* latest mouse y position */
	int		newbuttons;	/* latest buttons */
	int		mousestatus;	/* latest mouse status */

	if (mousedev.Poll && (mousedev.Poll() == 0))
			return FALSE;

	/* Read the latest mouse status*/
	mousestatus = GdReadMouse(&rootx, &rooty, &newbuttons);
	if (mousestatus <= 0)
		return FALSE;		/* read failed or no new data*/

	/* Deliver mouse events as appropriate*/
	MwHandleMouseStatus(rootx, rooty, newbuttons);
	return TRUE;
}

/*
 * Update keyboard status and issue events on it if necessary.
 * This function doesn't block, but is only called when Poll() returns TRUE
 * or MwSelect shows some data waiting to be read on the keyboard file descriptor.
 */
BOOL
MwCheckKeyboardEvent(void)
{
	MWKEY	 	mwkey;		/* latest character */
	MWKEYMOD 	modifiers;	/* latest modifiers */
	MWSCANCODE	scancode;
	int	 	keystatus;	/* latest keyboard status */

	if (kbddev.Poll && (kbddev.Poll() == 0))
			return FALSE;

	/* Read the latest keyboard status*/
	keystatus = GdReadKeyboard(&mwkey, &modifiers, &scancode);
	if (keystatus <= 0)
	{
		if (keystatus == KBD_QUIT)	/* special case for quit message*/
			MwTerminate();
		return FALSE;				/* read failed or no new data*/
	}

	/* handle special keys*/
	switch (mwkey)
	{
	case MWKEY_QUIT:
#if DEBUG
		if (modifiers & MWKMOD_CTRL)
			GdCaptureScreen(NULL, "screen.bmp");
		else
#endif
		MwTerminate();
		break;
	case MWKEY_REDRAW:
		MwRedrawScreen();
		break;
#if DEBUG
	case MWKEY_PRINT:
		if (keystatus == KBD_KEYPRESS)
			GdCaptureScreen(NULL, "screen.bmp");
		break;
#endif
	}

	/* Deliver keyboard events as appropriate*/
	MwDeliverKeyboardEvent(mwkey, modifiers, scancode, keystatus == KBD_KEYPRESS? TRUE: FALSE);
	return TRUE;
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
MwTranslateMouseMessage(HWND hwnd,UINT msg,int hittest,int buttons)
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
		    MWABS(cursorx-lastx) < mwSYSMETRICS_CXDOUBLECLK &&
		    MWABS(cursory-lasty) < mwSYSMETRICS_CYDOUBLECLK)
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
		PostMessage(hwnd, msg, buttons, MAKELONG(pt.x, pt.y));
	}
}

/*
 * Deliver a mouse button or motion event.
 */

void 
MwDeliverMouseEvent(int buttons, int changebuttons, MWKEYMOD modifiers)
{
	HWND	hwnd, top;
	int	hittest;
	UINT	msg;

	mwCurrentModifiers = modifiers;

	hwnd = GetCapture();
	if(!hwnd)
		hwnd = mousewp;

	/* if wnd is disabled, or window is child and parent disabled, do nothing*/
	if( hwnd == NULL || !IsWindowEnabled(hwnd) )
		return;

	top = MwGetTopWindow(hwnd);
	if( top != NULL && !IsWindowEnabled(top) )
		return;

	hittest = SendMessage(hwnd, WM_NCHITTEST, 0, MAKELONG(cursorx,cursory));

	if(!changebuttons)
		MwTranslateMouseMessage(hwnd, WM_MOUSEMOVE, hittest, buttons);

	if(changebuttons & MWBUTTON_L) {
		msg = (buttons&MWBUTTON_L)? WM_LBUTTONDOWN: WM_LBUTTONUP;
		MwTranslateMouseMessage(hwnd, msg, hittest, buttons);
	}

	if(changebuttons & MWBUTTON_M) {
		msg = (buttons&MWBUTTON_M)? WM_MBUTTONDOWN: WM_MBUTTONUP;
		MwTranslateMouseMessage(hwnd, msg, hittest, buttons);
	}

	if(changebuttons & MWBUTTON_R) {
		msg = (buttons&MWBUTTON_R)? WM_RBUTTONDOWN: WM_RBUTTONUP;
		MwTranslateMouseMessage(hwnd, msg, hittest, buttons);
	}
}

/*
 *  Keyboard filter function
 */
void WINAPI
MwSetKeyboardTranslator(LPFN_KEYBTRANSLATE pFn)
{
	mwPtrKeyboardTranslator = pFn;
}

/*
 * Deliver a keyboard event.
 */
void
MwDeliverKeyboardEvent(MWKEY keyvalue, MWKEYMOD modifiers, MWSCANCODE scancode,
	BOOL pressed)
{
	WPARAM VK_Code = -1;		/* set to default if no VK found. */
    	LPARAM lParam = 0L;		/* used to specify control keys */

	mwCurrentModifiers = modifiers;	/* save current keyboard modifiers*/

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
	case MWKEY_LMETA:
		VK_Code = VK_F1;	// Smartphone 2003 compliance
		break;
	case MWKEY_RMETA:
		VK_Code = VK_F2;	// Smartphone 2003 compliance
		break;
	case MWKEY_ACCEPT:
		VK_Code = VK_RETURN;// Smartphone 2003 compliance
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
	case MWKEY_ALTGR:
	/* Handheld function keys*/
	case MWKEY_RECORD:
	case MWKEY_PLAY:
	case MWKEY_CONTRAST:
	case MWKEY_BRIGHTNESS:
	case MWKEY_SELECTUP:
	case MWKEY_SELECTDOWN:
	case MWKEY_CANCEL:
	case MWKEY_APP1:
	case MWKEY_APP2:
	case MWKEY_LAST:
#endif
	default:
		VK_Code = VK_NONAME;
		break;
	}

	/* if no VK defined, set to keyvalue default*/
	if( VK_Code == -1 )
	    VK_Code = keyvalue;
	else
	    lParam |= (1 << 24);	/* set control bit in lParam*/
		
	if (mwPtrKeyboardTranslator)
		mwPtrKeyboardTranslator(&VK_Code, &lParam, &pressed);

#if WINEXTRA
	extern void update_input_key_state(unsigned char *keystate,UINT message,WPARAM wParam);
	extern unsigned char MwKeyState[256];
	update_input_key_state(MwKeyState, pressed? WM_KEYDOWN: WM_KEYUP, VK_Code);
#endif

	if (!MwDeliverHotkey (VK_Code, pressed)) {
		if (pressed)
			PostMessage(focuswp, WM_KEYDOWN, VK_Code, lParam);
		else
			PostMessage(focuswp, WM_KEYUP, VK_Code, lParam);
	}
}

#if !WINEXTRA
SHORT WINAPI
GetKeyState(int nVirtKey)
{
	switch(nVirtKey) {
	case VK_CONTROL:
		if (mwCurrentModifiers & MWKMOD_CTRL)
			return 0x8000;
		break;
	case VK_SHIFT:
		if (mwCurrentModifiers & MWKMOD_SHIFT)
			return 0x8000;
		break;
	case VK_MENU:		/* alt*/
		if (mwCurrentModifiers & (MWKMOD_ALT|MWKMOD_META))
			return 0x8000;
		break;
	}
	return 0;
}
#endif

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
	wp->nEraseBkGnd++;

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
