/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Win32 API upper level window creation, management and msg routines
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include <stdlib.h>
#include <string.h>

#define PAINTONCE	1	/* =1 to queue paint msgs only once*/
#define MOUSETEST	1

MWLISTHEAD mwMsgHead;		/* application msg queue*/
MWLISTHEAD mwClassHead;		/* register class list*/

int	mwSYSMETRICS_CYCAPTION = 12;	/* Y caption height*/
int	mwSYSMETRICS_CXFRAME = 3;	/* width of frame border*/
int	mwSYSMETRICS_CYFRAME = 3;	/* height of frame border*/
int	mwSYSMETRICS_CXBORDER = 1;	/* width of single border*/
int	mwSYSMETRICS_CYBORDER = 1;	/* width of single border*/
int	mwSYSMETRICS_CXVSCROLL = 13;	/* width of vertical scrollbar*/
int	mwSYSMETRICS_CYHSCROLL = 13;	/* height of horizontal scrollbar*/
int	mwSYSMETRICS_CXHSCROLL = 13;	/* width of arrow on horz scrollbar*/
int	mwSYSMETRICS_CYVSCROLL = 13;	/* height of arrow on vert scrollbar*/
int	mwSYSMETRICS_CXDOUBLECLK = 2;	/* +/- X double click position*/
int	mwSYSMETRICS_CYDOUBLECLK = 2;	/* +/- Y double click position*/
int	mwpaintSerial = 1;		/* experimental alphablend sequencing*/
int	mwpaintNC = 1;			/* experimental NC paint handling*/
BOOL 	mwforceNCpaint = FALSE;		/* force NC paint when alpha blending*/

static void MwOffsetChildren(HWND hwnd, int offx, int offy);

LRESULT WINAPI
CallWindowProc(WNDPROC lpPrevWndFunc, HWND hwnd, UINT Msg, WPARAM wParam,
	LPARAM lParam)
{
	return (*lpPrevWndFunc)(hwnd, Msg, wParam, lParam);
}

LRESULT WINAPI
SendMessage(HWND hwnd, UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if(hwnd && hwnd->pClass) {
		hwnd->paintSerial = mwpaintSerial; /* assign msg sequence #*/
		return (*hwnd->pClass->lpfnWndProc)(hwnd, Msg, wParam, lParam);
	}
	return 0;
}

BOOL WINAPI
PostMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	MSG *	pMsg;

#if PAINTONCE
	/* don't queue paint msgs, set window paint status instead*/
	if(Msg == WM_PAINT) {
		hwnd->gotPaintMsg = PAINT_NEEDSPAINT;
		return TRUE;
	}
#endif
#if MOUSETEST
	/* replace multiple mouse messages with one for better mouse handling*/
	if(Msg == WM_MOUSEMOVE) {
		PMWLIST	p;
		for(p=mwMsgHead.head; p; p=p->next) {
			pMsg = GdItemAddr(p, MSG, link);
			if(pMsg->hwnd == hwnd && pMsg->message == Msg) {
				pMsg->wParam = wParam;
				pMsg->lParam = lParam;
				pMsg->time = GetTickCount();
				pMsg->pt.x = cursorx;
				pMsg->pt.y = cursory;
				return TRUE;
			}
		}
	}
#endif
	pMsg = GdItemNew(MSG);
	if(!pMsg)
		return FALSE;
	pMsg->hwnd = hwnd;
	pMsg->message = Msg;
	pMsg->wParam = wParam;
	pMsg->lParam = lParam;
	pMsg->time = GetTickCount();
	pMsg->pt.x = cursorx;
	pMsg->pt.y = cursory;
	GdListAdd(&mwMsgHead, &pMsg->link);
	return TRUE;
}

/* currently, we post to the single message queue, regardless of thread*/
BOOL WINAPI
PostThreadMessage(DWORD dwThreadId, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return PostMessage(NULL, Msg, wParam, lParam);
}

VOID WINAPI
PostQuitMessage(int nExitCode)
{
	PostMessage(NULL, WM_QUIT, nExitCode, 0L);
}

static BOOL
chkPaintMsg(HWND wp, LPMSG lpMsg)
{
		/*
		 * Tricky: only repaint window if there
		 * isn't a mouse capture (window move) in progress,
		 * or the window is the moving window.
		 */
		if(wp->gotPaintMsg == PAINT_NEEDSPAINT &&
		    (!dragwp || dragwp == wp)) {
	paint:
			wp->gotPaintMsg = PAINT_PAINTED;
			lpMsg->hwnd = wp;
			lpMsg->message = WM_PAINT;
			lpMsg->wParam = 0;
			lpMsg->lParam = 0;
			lpMsg->time = 0;
			lpMsg->pt.x = cursorx;
			lpMsg->pt.y = cursory;
			return TRUE;
		} else if(dragwp && wp->gotPaintMsg == PAINT_NEEDSPAINT) {
			/* All other windows we'll check for
			 * event input first, then allow repaint.
			 */
			MwSelect();
			if(mwMsgHead.head == NULL)
				goto paint;
		}
	return FALSE;
}

BOOL WINAPI
PeekMessage(LPMSG lpMsg, HWND hwnd, UINT uMsgFilterMin, UINT uMsgFilterMax,
	UINT wRemoveMsg)
{
	HWND	wp;
	PMSG	pNxtMsg;

	/* check if no messages in queue*/
	if(mwMsgHead.head == NULL) {
#if PAINTONCE
		/* check all windows for pending paint messages*/
		for(wp=listwp; wp; wp=wp->next) {
			if(!(wp->style & WS_CHILD)) {
				if(chkPaintMsg(wp, lpMsg))
					return TRUE;
			}
		}
		for(wp=listwp; wp; wp=wp->next) {
			if(wp->style & WS_CHILD) {
				if(chkPaintMsg(wp, lpMsg))
					return TRUE;
			}
		}
#endif
		MwSelect();
	}

	if(mwMsgHead.head == NULL)
		return FALSE;

	pNxtMsg = (PMSG)mwMsgHead.head;
	if(wRemoveMsg & PM_REMOVE)
		GdListRemove(&mwMsgHead, &pNxtMsg->link);
	*lpMsg = *pNxtMsg;
	if(wRemoveMsg & PM_REMOVE)
		GdItemFree(pNxtMsg);
	return TRUE;
}

BOOL WINAPI
GetMessage(LPMSG lpMsg,HWND hwnd,UINT wMsgFilterMin,UINT wMsgFilterMax)
{
	/*
	 * currently MwSelect() must poll for VT switch reasons,
	 * so this code will work
	 */
	while(!PeekMessage(lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax,PM_REMOVE))
		continue;
	return lpMsg->message != WM_QUIT;
}

BOOL WINAPI 
TranslateMessage(CONST MSG *lpMsg)
{
	return FALSE;
}

LONG WINAPI
DispatchMessage(CONST MSG *lpMsg)
{
	return SendMessage(lpMsg->hwnd, lpMsg->message, lpMsg->wParam,
		lpMsg->lParam);
}

/* find the registered window class struct by name*/
PWNDCLASS
MwFindClassByName(LPCSTR lpClassName)
{
	PMWLIST		p;
	PWNDCLASS	pClass;

	for(p=mwClassHead.head; p; p=p->next) {
		pClass = GdItemAddr(p, WNDCLASS, link);
		if(strcmpi(pClass->szClassName, lpClassName) == 0)
			return pClass;
	}
	return NULL;
}

ATOM WINAPI
RegisterClass(CONST WNDCLASS *lpWndClass)
{
	PWNDCLASS	pClass;

	/* check if already present*/
	pClass = MwFindClassByName(lpWndClass->lpszClassName);
	if(pClass)
		return 0;
	
	/* copy class into new struct*/
	pClass = GdItemNew(WNDCLASS);
	if(!pClass)
		return 0;
	*pClass = *lpWndClass;
	strcpy(pClass->szClassName, lpWndClass->lpszClassName);
	GdListAdd(&mwClassHead, &pClass->link);

	return 1;
}

BOOL WINAPI
UnregisterClass(LPCSTR lpClassName, HINSTANCE hInstance)
{
	PWNDCLASS	pClass;

	pClass = MwFindClassByName(lpClassName);
	if(!pClass)
		return FALSE;
	GdListRemove(&mwClassHead, &pClass->link);
	DeleteObject(pClass->hbrBackground);
	GdItemFree(pClass);
	return TRUE;
}

HWND WINAPI
CreateWindowEx(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
	DWORD dwStyle, int x, int y, int nWidth, int nHeight,
	HWND hwndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND		pwp;		/* parent window */
	HWND		wp;		/* new window */
	HWND		hwndOwner;
	PWNDCLASS	pClass;
	CREATESTRUCT	cs;
	static int	nextx = 20;
	static int	nexty = 20;

	pClass = MwFindClassByName(lpClassName);
	if(!pClass)
		return NULL;

	if(x == CW_USEDEFAULT || y == CW_USEDEFAULT) {
		x = nextx;
		nextx += 10;
		y = nexty;
		nexty += 10;
		if(nextx > 200)
			nextx = nexty = 20;
	}
	if(nWidth == CW_USEDEFAULT || nHeight == CW_USEDEFAULT) {
		nWidth = 250;
		nHeight = 250;
	}

	if(hwndParent == NULL) {
		if(dwStyle & WS_CHILD)
			return NULL;
		pwp = rootwp;
	} else
		pwp = hwndParent;

	/* WS_POPUP z-order parent is the root window (passed parent is owner)*/
	if(dwStyle & WS_POPUP)
		pwp = rootwp;		/* force clip to root, not z-parent*/

	/* window owner is NULL for child windows, else it's the passed parent*/
	if(dwStyle & WS_CHILD)
		hwndOwner = NULL;
	else hwndOwner = hwndParent;

	wp = (HWND)GdItemAlloc(sizeof(struct hwnd) - 1 + pClass->cbWndExtra);
	if(!wp)
		return NULL;

	/* force all clipping on by default*/
	dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	wp->pClass = pClass;
	wp->style = dwStyle;
	wp->exstyle = dwExStyle;
	wp->parent = pwp;
	wp->owner = hwndOwner;
	wp->children = NULL;
	wp->siblings = pwp->children;
	pwp->children = wp;
	wp->next = listwp;
	listwp = wp;
	wp->winrect.left = pwp->clirect.left + x;
	wp->winrect.top = pwp->clirect.top + y;
	wp->winrect.right = wp->winrect.left + nWidth;
	wp->winrect.bottom = wp->winrect.top + nHeight;
	wp->cursor = pwp->cursor;
	wp->cursor->usecount++;
	wp->unmapcount = pwp->unmapcount + 1;
	wp->id = (int)hMenu;
	wp->gotPaintMsg = PAINT_PAINTED;
	strzcpy(wp->szTitle, lpWindowName, sizeof(wp->szTitle));
#if UPDATEREGIONS
	wp->update = GdAllocRegion();
#endif
	wp->nextrabytes = pClass->cbWndExtra;

	/* calculate client area*/
	MwCalcClientRect(wp);

	cs.lpCreateParams = lpParam;
	cs.hInstance = hInstance;
	cs.hMenu = hMenu;
	cs.hwndParent = hwndParent;
	cs.cy = nHeight;
	cs.cx = nWidth;
	cs.y = y;
	cs.x = x;
	cs.style = dwStyle;
	cs.lpszName = lpWindowName;
	cs.lpszClass = lpClassName;
	cs.dwExStyle = dwExStyle;

	if(SendMessage(wp, WM_CREATE, 0, (LPARAM)(LPSTR)&cs) == -1) {
		MwDestroyWindow(wp, FALSE);
		return NULL;
	}

	/* send SIZE and MOVE msgs*/
	MwSendSizeMove(wp, TRUE, TRUE);

	if(wp->style & WS_VISIBLE) {
		MwShowWindow(wp, TRUE);
		SetFocus(wp);
	}

	return wp;
}

BOOL WINAPI
DestroyWindow(HWND hwnd)
{
	MwDestroyWindow(hwnd, TRUE);
	return TRUE;
}

/*
 * Destroy the specified window, and all of its children.
 * This is a recursive routine.
 */
void
MwDestroyWindow(HWND hwnd,BOOL bSendMsg)
{
	HWND	wp = hwnd;
	HWND	prevwp;
	PMWLIST	p;
	PMSG	pmsg;

	if (wp == rootwp)
		return;

	/*
	 * Unmap the window.
	 */
	if (wp->unmapcount == 0)
		MwHideWindow(wp, FALSE, FALSE);

	if(bSendMsg)
		SendMessage(hwnd, WM_DESTROY, 0, 0L);

	/*
	 * Disable all sendmessages to this window.
	 */
	wp->pClass = NULL;

	/*
	 * Destroy all children, sending WM_DESTROY messages.
	 */
	while (wp->children)
		MwDestroyWindow(wp->children, bSendMsg);

	/*
	 * Free any cursor associated with the window.
	 */
	if (wp->cursor->usecount-- == 1) {
		free(wp->cursor);
		wp->cursor = NULL;
	}

	/*
	 * Remove this window from the child list of its parent.
	 */
	prevwp = wp->parent->children;
	if (prevwp == wp)
		wp->parent->children = wp->siblings;
	else {
		while (prevwp->siblings != wp)
			prevwp = prevwp->siblings;
		prevwp->siblings = wp->siblings;
	}
	wp->siblings = NULL;

	/*
	 * Remove this window from the complete list of windows.
	 */
	prevwp = listwp;
	if (prevwp == wp)
		listwp = wp->next;
	else {
		while (prevwp->next != wp)
			prevwp = prevwp->next;
		prevwp->next = wp->next;
	}
	wp->next = NULL;

	/*
	 * Forget various information related to this window.
	 * Then finally free the structure.
	 */

	/* Remove all messages from msg queue for this window*/
	for(p=mwMsgHead.head; p; ) {
		pmsg = GdItemAddr(p, MSG, link);
		if(pmsg->hwnd == wp) {
			p = p->next;
			GdListRemove(&mwMsgHead, &pmsg->link);
			GdItemFree(p);
		} else
			p = p->next;
	}

	/* FIXME: destroy hdc's relating to window?*/

	if (wp == capturewp) {
		capturewp = NULL;
		MwCheckMouseWindow();
	}

	if (wp == MwGetTopWindow(focuswp))
		SetFocus(rootwp->children? rootwp->children: rootwp);

	/* destroy private DC*/
	if(wp->owndc) {
		HDC hdc = wp->owndc;
		wp->owndc = NULL;	/* force destroy with ReleaseDC*/
		ReleaseDC(wp, hdc);
	}
#if UPDATEREGIONS
	GdDestroyRegion(wp->update);
#endif
	GdItemFree(wp);
}

BOOL WINAPI
IsWindow(HWND hwnd)
{
	HWND	wp;

	for(wp=listwp; wp; wp=wp->next)
		if(wp == hwnd)
			return TRUE;
	return FALSE;
}

BOOL WINAPI
ShowWindow(HWND hwnd, int nCmdShow)
{
	if(!hwnd)
		return FALSE;
	
	/* fix: send show msg*/

	switch(nCmdShow) {
	case SW_HIDE:
		if (!(hwnd->style & WS_VISIBLE))
			return FALSE;
		MwHideWindow(hwnd, TRUE, TRUE);
		hwnd->style &= ~WS_VISIBLE;
		break;

	default:
		if (hwnd->style & WS_VISIBLE)
			return FALSE;
		hwnd->style |= WS_VISIBLE;
		MwShowWindow(hwnd, TRUE);
	}
	return TRUE;
}

BOOL WINAPI
InvalidateRect(HWND hwnd, CONST RECT *lpRect, BOOL bErase)
{
	/* FIXME: handle bErase*/
	if(!hwnd)
		MwRedrawScreen();
	else {
#if UPDATEREGIONS
		RECT	rc;

		/* add to update region*/
		if(!lpRect)
			GetClientRect(hwnd, &rc);
		else rc = *lpRect;
		rc.bottom += mwSYSMETRICS_CYCAPTION +
			mwSYSMETRICS_CYFRAME + 1;
		rc.right += mwSYSMETRICS_CXFRAME;
		MwUnionUpdateRegion(hwnd, rc.left, rc.top,
			rc.right-rc.left, rc.bottom-rc.top, TRUE);

		/* if update region not empty, mark as needing painting*/
		if(hwnd->update->numRects != 0)
#endif
			if(hwnd->gotPaintMsg == PAINT_PAINTED)
				hwnd->gotPaintMsg = PAINT_NEEDSPAINT;
	}
	return TRUE;
}

#if UPDATEREGIONS
/* add region to window update region*/
BOOL WINAPI
InvalidateRgn(HWND hwnd, HRGN hrgn, BOOL bErase)
{
	/* FIXME: handle bErase*/
	if(hwnd) {
		if(!hrgn)
			/* add client area to update region*/
			return InvalidateRect(hwnd, NULL, bErase);

		/* passed region is in client coords, convert to screen*/
		GdOffsetRegion(((MWRGNOBJ *)hrgn)->rgn,
			hwnd->clirect.left, hwnd->clirect.top);
		GdUnionRegion(hwnd->update, hwnd->update,
			((MWRGNOBJ *)hrgn)->rgn);
		GdOffsetRegion(((MWRGNOBJ *)hrgn)->rgn,
			-hwnd->clirect.left, -hwnd->clirect.top);

		/* if update region not empty, mark as needing painting*/
		if(hwnd->update->numRects != 0)
			if(hwnd->gotPaintMsg == PAINT_PAINTED)
				hwnd->gotPaintMsg = PAINT_NEEDSPAINT;
	}
	return TRUE;
}

BOOL WINAPI
ValidateRect(HWND hwnd, CONST RECT *lprc)
{
	RECT	rc;

	if(!hwnd)
		MwRedrawScreen();
	else {
		/* subtract from update region*/
		if(!lprc)
			GetClientRect(hwnd, &rc);
		else rc = *lprc;
		rc.bottom += mwSYSMETRICS_CYCAPTION +
			mwSYSMETRICS_CYFRAME + 1;
		rc.right += mwSYSMETRICS_CXFRAME;
		MwUnionUpdateRegion(hwnd, rc.left, rc.top,
			rc.right-rc.left, rc.bottom-rc.top, FALSE);

		/* if update region empty, mark window as painted*/
		if(hwnd->update->numRects == 0)
			if(hwnd->gotPaintMsg == PAINT_NEEDSPAINT)
				hwnd->gotPaintMsg = PAINT_PAINTED;
	}
	return TRUE;
}

/* remove region from window update region*/
BOOL WINAPI
ValidateRgn(HWND hwnd, HRGN hrgn)
{
	if(hwnd) {
		if(!hrgn)
			/* remove client area from update region*/
			return ValidateRect(hwnd, NULL);

		/* passed region is in client coords, convert to screen*/
		GdOffsetRegion(((MWRGNOBJ *)hrgn)->rgn,
			hwnd->clirect.left, hwnd->clirect.top);
		GdSubtractRegion(hwnd->update, hwnd->update,
			((MWRGNOBJ *)hrgn)->rgn);
		GdOffsetRegion(((MWRGNOBJ *)hrgn)->rgn,
			-hwnd->clirect.left, -hwnd->clirect.top);

		/* if update region empty, mark window as painted*/
		if(hwnd->update->numRects == 0)
			if(hwnd->gotPaintMsg == PAINT_NEEDSPAINT)
				hwnd->gotPaintMsg = PAINT_PAINTED;
	}
	return TRUE;
}
#endif /* UPDATEREGIONS*/

BOOL WINAPI
UpdateWindow(HWND hwnd)
{
#if PAINTONCE
	if(hwnd && hwnd->gotPaintMsg == PAINT_NEEDSPAINT) {
		SendMessage(hwnd, WM_PAINT, 0, 0L);
		hwnd->gotPaintMsg = PAINT_PAINTED;
		return TRUE;
	}
	return FALSE;
#else
	/* fix: remove other paint messages from queue*/
	SendMessage(hwnd, WM_PAINT, 0, 0L);
	return TRUE;
#endif
}

HWND WINAPI
GetFocus(VOID)
{
	return focuswp;
}

HWND WINAPI
SetFocus(HWND hwnd)
{
	HWND	oldfocus;
	HWND	top, top2;

	/* if NULL or hidden, set focus to desktop*/
	if(!hwnd || hwnd->unmapcount)
		hwnd = rootwp;

	if(hwnd == focuswp)
		return focuswp;

	oldfocus = focuswp;
	SendMessage(oldfocus, WM_KILLFOCUS, (WPARAM)hwnd, 0L);
	focuswp = hwnd;
	SendMessage(focuswp, WM_SETFOCUS, (WPARAM)oldfocus, 0L);

	/* FIXME SetActiveWindow() here?*/
	top = MwGetTopWindow(oldfocus);
	top2 = MwGetTopWindow(focuswp);
	if(top2 != top) {
		/* send deactivate*/
		SendMessage(top, WM_ACTIVATE, (WPARAM)MAKELONG(WA_INACTIVE, 0),
			(LPARAM)top2);
		/* repaint captions*/
		MwPaintNCArea(top);
#if 0
		/* Make sure that caption area is fully invalidated,
		 * as repaint will be in active color.
		 * FIXME: this doesn't work; breaks terminal emulator
		 * on focus out/in
		 */
		MwUnionUpdateRegion(top2, 0, 0,
			top2->winrect.right-top2->winrect.left,
			mwSYSMETRICS_CYCAPTION+4, TRUE);
#endif
		/* send deactivate*/
		SendMessage(top, WM_ACTIVATE, (WPARAM)MAKELONG(WA_ACTIVE, 0),
			(LPARAM)top);
		MwPaintNCArea(top2);
	}

	return oldfocus;
}

/* the foreground window is the top level window at the top of the z order*/
/* setting the foreground window sets focus and moves window to top*/
BOOL WINAPI
SetForegroundWindow(HWND hwnd)
{
	/* activate (set focus to) specified window*/
	SetFocus(hwnd);

	/* raise top level parent to top of z order*/
	SetWindowPos(MwGetTopWindow(hwnd), HWND_TOP, 0, 0, 0, 0,
		SWP_NOMOVE|SWP_NOSIZE);

	return TRUE;
}

/* our SetActiveWindow is the same as SetFocus, no z order change*/
HWND WINAPI
SetActiveWindow(HWND hwnd)
{
	HWND	oldActive;

	oldActive = GetActiveWindow();
	SetFocus(hwnd);		 /* WM_ACTIVATE sent by SetFocus*/
	return oldActive;
}

/* The active window is the first non-child ancestor of focus window*/
HWND WINAPI
GetActiveWindow(VOID)
{
	return MwGetTopWindow(focuswp);
}

/* activate the top level window associated with window*/
BOOL WINAPI
BringWindowToTop(HWND hwnd)
{
	return SetForegroundWindow(hwnd);
}

HWND WINAPI
GetDesktopWindow(VOID)
{
	return rootwp;
}

HWND
MwGetTopWindow(HWND hwnd)
{
	while(hwnd->style & WS_CHILD)
		hwnd = hwnd->parent;
	return hwnd;
}

HWND WINAPI
GetParent(HWND hwnd)
{
	/* top level windows return NULL instead of rootwp*/
	if(!hwnd || !(hwnd->style & (WS_POPUP|WS_CHILD)))
		return NULL;		/* toplevel window*/
	if(hwnd->style & WS_POPUP)
		return hwnd->owner;	/* popup window*/
	return hwnd->parent;		/* child window*/
}

BOOL WINAPI
EnableWindow(HWND hwnd, BOOL bEnable)
{
	if(bEnable && (hwnd->style & WS_DISABLED)) {
		/* enable window*/
		hwnd->style &= ~WS_DISABLED;
		SendMessage(hwnd, WM_ENABLE, TRUE, 0L);
		return TRUE;
	}
	if(!bEnable && !(hwnd->style & WS_DISABLED)) {
		/* disable window*/
		hwnd->style |= WS_DISABLED;
		/* FIXME: handle lost focus for child window of hwnd*/
		/* FIXME: handle lost focus for capture window*/
		if(hwnd == focuswp)
			SetFocus(NULL);
		SendMessage(hwnd, WM_ENABLE, FALSE, 0L);
		return FALSE;
	}
	return (hwnd->style & WS_DISABLED) != 0;
}

/* calc window rect from client rect in screen coords*/
BOOL WINAPI
AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
	int	yoffset;

	if(dwStyle & WS_BORDER) {
		if((dwStyle & WS_CAPTION) == WS_CAPTION) {
			InflateRect(lpRect, mwSYSMETRICS_CXFRAME,
				mwSYSMETRICS_CYFRAME);
			yoffset = mwSYSMETRICS_CYCAPTION + 1;
			lpRect->top -= yoffset;
			lpRect->bottom -= yoffset;
		} else
			InflateRect(lpRect, 1, 1);

		/* make sure upper left is on screen*/
		if(lpRect->left < 0) {
			lpRect->right -= lpRect->left;
			lpRect->left = 0;
		}
		if(lpRect->top < 0) {
			lpRect->bottom -= lpRect->top;
			lpRect->top = 0;
		}
	}
	return TRUE;
}

/* set the client rect for a window from the window position*/
void
MwCalcClientRect(HWND hwnd)
{
	NCCALCSIZE_PARAMS	nccs;

	/* set first rectangle to window rect*/
	nccs.rgrc[0] = hwnd->winrect;
	SendMessage(hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)(LPSTR)&nccs);
	hwnd->clirect = nccs.rgrc[0];

	/* adjust client area if scrollbar(s) visible*/
	MwAdjustNCScrollbars(hwnd);
}

BOOL WINAPI
GetClientRect(HWND hwnd, LPRECT lpRect)
{
	if(!hwnd || !lpRect)
		return FALSE;

	/* convert client area rect from screen coordinates*/
	lpRect->left = 0;
	lpRect->top = 0;
	lpRect->right = hwnd->clirect.right - hwnd->clirect.left;
	lpRect->bottom = hwnd->clirect.bottom - hwnd->clirect.top;
	return TRUE;
}

BOOL WINAPI
GetWindowRect(HWND hwnd, LPRECT lpRect)
{
	if(!hwnd || !lpRect)
		return FALSE;
	
	/* window rect is already in screen coordinates*/
	*lpRect = hwnd->winrect;
	return TRUE;
}

BOOL WINAPI
ClientToScreen(HWND hwnd, LPPOINT lpPoint)
{
	if(!hwnd || !lpPoint)
		return FALSE;
	MapWindowPoints(hwnd, NULL, lpPoint, 1);
	return TRUE;
}

BOOL WINAPI
ScreenToClient(HWND hwnd, LPPOINT lpPoint)
{
	if(!hwnd || !lpPoint)
		return FALSE;
	MapWindowPoints(NULL, hwnd, lpPoint, 1);
	return TRUE;
}

int WINAPI
MapWindowPoints(HWND hwndFrom, HWND hwndTo, LPPOINT lpPoints, UINT cPoints)
{
	MWCOORD	offx = 0;
	MWCOORD	offy = 0;

	/* map src window to screen coords*/
	if(hwndFrom) {
		offx = hwndFrom->clirect.left;
		offy = hwndFrom->clirect.top;
	}

	/* map to dst window client coords*/
	if(hwndTo) {
		offx -= hwndTo->clirect.left;
		offy -= hwndTo->clirect.top;
	}

	/* adjust points*/
	while(cPoints--) {
		lpPoints->x += offx;
		lpPoints->y += offy;
		++lpPoints;
	}
	return (int)MAKELONG(offx, offy);
}

BOOL WINAPI
SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
	lprc->left = xLeft;
	lprc->top = yTop;
	lprc->right = xRight;
	lprc->bottom = yBottom;
	return TRUE;
}

BOOL WINAPI
SetRectEmpty(LPRECT lprc)
{
	lprc->left = lprc->right = lprc->top = lprc->bottom = 0;
	return TRUE;
}

BOOL WINAPI
CopyRect(LPRECT lprcDst, CONST RECT *lprcSrc)
{
	*lprcDst = *lprcSrc;
	return TRUE;
}

BOOL WINAPI
IsRectEmpty(CONST RECT *lprc)
{
	/* FIXME: should this just be ==, not <= ?*/
	/*return lprc->left == lprc->right || lprc->top == lprc->bottom;*/
	return lprc->right <= lprc->left || lprc->bottom <= lprc->top;
}

BOOL WINAPI
InflateRect(LPRECT lprc, int dx, int dy)
{
	lprc->left -= dx;
	lprc->top -= dy;
	lprc->right += dx;
	lprc->bottom += dy;
	return TRUE;
}

BOOL WINAPI
OffsetRect(LPRECT lprc, int dx, int dy)
{
	lprc->left += dx;
	lprc->right += dx;
	lprc->top += dy;
	lprc->bottom += dy;
	return TRUE;
}

/* PtInRect is #defined to MwPTINRECT because of bcc struct passing bug*/
BOOL WINAPI
MwPTINRECT(CONST RECT *lprc, POINT pt)
{
	return (pt.x >= lprc->left && pt.x < lprc->right &&
		pt.y >= lprc->top && pt.y < lprc->bottom);
}

LONG WINAPI
GetWindowLong(HWND hwnd, int nIndex)
{
	switch(nIndex) {
	case GWL_WNDPROC:
		return (LONG)hwnd->pClass->lpfnWndProc;
	case GWL_HINSTANCE:
		/* nyi*/
		break;
	case GWL_HWNDPARENT:
		return (LONG)hwnd->parent;
	case GWL_ID:
		return hwnd->id;
	case GWL_STYLE:
		return hwnd->style;
	case GWL_EXSTYLE:
		return hwnd->exstyle;
	case GWL_USERDATA:
		return hwnd->userdata;
	default:
		if(nIndex+3 < hwnd->nextrabytes)
			return *(LONG *)&hwnd->extrabytes[nIndex];
	}
	return 0;
}

LONG WINAPI
SetWindowLong(HWND hwnd, int nIndex, LONG lNewLong)
{
	LONG	oldval = 0;

	switch(nIndex) {
	case GWL_USERDATA:
		oldval = hwnd->userdata;
		hwnd->userdata = lNewLong;
		break;
	case GWL_WNDPROC:
		oldval = (LONG)hwnd->pClass->lpfnWndProc;
		hwnd->pClass->lpfnWndProc = (WNDPROC)lNewLong;
		break;
	case GWL_HINSTANCE:
	case GWL_HWNDPARENT:
	case GWL_ID:
	case GWL_STYLE:
	case GWL_EXSTYLE:
		/* nyi*/
		break;
	default:
		if(nIndex+3 < hwnd->nextrabytes) {
			oldval = GetWindowLong(hwnd, nIndex);
			*(LONG *)&hwnd->extrabytes[nIndex] = lNewLong;
		}
		break;
	}
	return oldval;
}

WORD WINAPI
GetWindowWord(HWND hwnd, int nIndex)
{
	if(nIndex+1 < hwnd->nextrabytes)
		return *(WORD *)&hwnd->extrabytes[nIndex];
	return 0;
}

WORD WINAPI
SetWindowWord(HWND hwnd, int nIndex, WORD wNewWord)
{
	WORD	oldval = 0;

	if(nIndex+1 < hwnd->nextrabytes) {
		oldval = GetWindowWord(hwnd, nIndex);
		*(WORD *)&hwnd->extrabytes[nIndex] = wNewWord;
	}
	return oldval;
}

DWORD WINAPI
GetClassLong(HWND hwnd, int nIndex)
{
	switch(nIndex) {
	case GCL_HBRBACKGROUND:
		return (DWORD)hwnd->pClass->hbrBackground;
	case GCL_CBWNDEXTRA:
		return (DWORD)hwnd->pClass->cbWndExtra;
	}
	return 0;
}

int WINAPI
GetWindowTextLength(HWND hwnd)
{
	return SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0L);
}

int WINAPI
GetWindowText(HWND hwnd, LPSTR lpString, int nMaxCount)
{
	return SendMessage(hwnd, WM_GETTEXT, nMaxCount,(LPARAM)(LPSTR)lpString);
}

BOOL WINAPI
SetWindowText(HWND hwnd, LPCSTR lpString)
{
	return SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)(LPCSTR)lpString);
}

/* Recursively offset all children of passed window*/
static void
MwOffsetChildren(HWND hwnd, int offx, int offy)
{
	HWND	cp;

	/* offset all child windows for move*/
	for(cp=hwnd->children; cp; cp=cp->siblings) {

		/* offset window and client area rects*/
		OffsetRect(&cp->winrect, offx, offy);
		OffsetRect(&cp->clirect, offx, offy);

		/* offset scrollbar NC hittest rects*/
		if(!IsRectEmpty(&cp->vscroll.rc))
			OffsetRect(&cp->vscroll.rc, offx, offy);
		if(!IsRectEmpty(&cp->hscroll.rc))
			OffsetRect(&cp->hscroll.rc, offx, offy);
		MwOffsetChildren(cp, offx, offy);
	}
}

BOOL
SetWindowPos(HWND hwnd, HWND hwndInsertAfter, int x, int y, int cx, int cy,
	UINT fuFlags)
{
	int		hidden;
	BOOL		bMove, bSize, bZorder;
	MWCOORD		offx = 0, offy = 0;	/* = 0 for bad gcc warning*/
	WINDOWPOS	winpos;

	if(!hwnd || hwnd == rootwp || cx < 0 || cy < 0)
		return FALSE;

	/* FIXME SWP_NOACTIVATE*/

	if((fuFlags & SWP_SHOWWINDOW))
		return ShowWindow(hwnd, SW_SHOW);

	if((fuFlags & SWP_HIDEWINDOW))
		return ShowWindow(hwnd, SW_HIDE);

	/* move is relative to parent's client rect for child windows*/
	if(hwnd->style & WS_CHILD) {
		x += hwnd->parent->clirect.left;
		y += hwnd->parent->clirect.top;
	} else {
		x += hwnd->parent->winrect.left;
		y += hwnd->parent->winrect.top;
	}

	bMove = !(fuFlags & SWP_NOMOVE) &&
			(hwnd->winrect.left != x || hwnd->winrect.top != y);
	bSize = !(fuFlags & SWP_NOSIZE) &&
			((hwnd->winrect.right - hwnd->winrect.left) != cx ||
		 	(hwnd->winrect.bottom - hwnd->winrect.top) != cy);
	bZorder = !(fuFlags & SWP_NOZORDER);
	if(!bMove && !bSize && !bZorder)
		return TRUE;

	/* could optimize to not require redraw when possible*/
	hidden = hwnd->unmapcount || (fuFlags & SWP_NOREDRAW);

	if(bZorder) {
		switch((int)hwndInsertAfter) {
		case (int)HWND_TOP:
			MwRaiseWindow(hwnd);
			break;
		case (int)HWND_BOTTOM:
			MwLowerWindow(hwnd);
			break;
		default:
			/* FIXME for non top/bottom zorder*/
			break;
		}
	} else {
		if(!hidden)
			MwHideWindow(hwnd, FALSE, FALSE);
	}

	if(bMove) {
		offx = x - hwnd->winrect.left;
		offy = y - hwnd->winrect.top;
	}
	if(bMove || bSize) {
		hwnd->winrect.left = x;
		hwnd->winrect.top = y;
		hwnd->winrect.right = x + cx;
		hwnd->winrect.bottom = y + cy;
	}
	if(bMove)
		MwOffsetChildren(hwnd, offx, offy);

	if(bMove || bSize) {
		MwCalcClientRect(hwnd);

		/* send windowposchanged message*/
		/* FIXME: move WM_MOVE, WM_SIZE to defwndproc*/
		winpos.hwnd = hwnd;
		winpos.hwndInsertAfter = hwndInsertAfter;
		winpos.x = x;
		winpos.y = y;
		winpos.cx = cx;
		winpos.cy = cy;
		winpos.flags = fuFlags;
		SendMessage(hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&winpos);

		MwSendSizeMove(hwnd, bSize, bMove);
	}

	++mwpaintSerial;	/* increment paint serial # for alphablending*/
	++mwpaintNC;		/* increment paint serial # for NC painting*/
	if(!bZorder && !hidden)
		MwShowWindow(hwnd, FALSE);

	return TRUE;
}

BOOL WINAPI
MoveWindow(HWND hwnd, int x, int y, int nWidth, int nHeight, BOOL bRepaint)
{
	UINT	flags = SWP_NOZORDER | SWP_NOACTIVATE;

	if(!bRepaint)
		flags |= SWP_NOREDRAW;
	return SetWindowPos(hwnd, 0, x, y, nWidth, nHeight, flags);
}

void
MwSendSizeMove(HWND hwnd, BOOL bSize, BOOL bMove)
{
	DWORD	dwStyle;
	RECT	rc;

	if(bSize) {
		GetClientRect(hwnd, &rc);
		SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
			MAKELONG(rc.right, rc.bottom));
	}
	if(bMove) {
		dwStyle = GetWindowLong(hwnd, GWL_STYLE);
		GetWindowRect(hwnd, &rc);
		/* return parent coords for child windows*/
		if(dwStyle & WS_CHILD)
			ScreenToClient(hwnd->parent, (LPPOINT)&rc.left);
		SendMessage(hwnd, WM_MOVE, 0, MAKELONG(rc.left, rc.top));
	}
}

/*
 * Specify a cursor for a window.
 * This cursor will only be used within that window, and by default
 * for its new children.  If the cursor is currently within this
 * window, it will be changed to the new one immediately.
 */
void
MwSetCursor(HWND wp, PMWCURSOR pcursor)
{
	HCURSOR	cp;
	int	bytes;

	if(!wp || !pcursor)
		return;

	bytes = MWIMAGE_SIZE(pcursor->width,pcursor->height)
		* sizeof(MWIMAGEBITS);

	/*
	 * See if the window is using a shared cursor definition.
	 * If so, then allocate a new private one, otherwise reuse it.
	 */
	cp = wp->cursor;
	if (!cp || cp->usecount-- > 1) {
		cp = GdItemNew(struct hcursor);
		if(!cp)
			return;
	}

	cp->usecount = 1;
	cp->cursor.width = pcursor->width;
	cp->cursor.height = pcursor->height;
	cp->cursor.hotx = pcursor->hotx;
	cp->cursor.hoty = pcursor->hoty;
	cp->cursor.fgcolor = pcursor->fgcolor;
	cp->cursor.bgcolor = pcursor->bgcolor;
	memcpy(cp->cursor.image, pcursor->image, bytes);
	memcpy(cp->cursor.mask, pcursor->mask, bytes);
	wp->cursor = cp;

	/*
	 * If this was the current cursor, then draw the new one.
	 */
	if (cp == curcursor || curcursor == NULL) {
		GdMoveCursor(cursorx - cp->cursor.hotx,
			cursory - cp->cursor.hoty);
		GdSetCursor(&cp->cursor);
	}
}

BOOL WINAPI
GetCursorPos(LPPOINT lpPoint)
{
	MWCOORD	x, y;

	if(lpPoint) {
		GdGetCursorPos(&x, &y);
		lpPoint->x = x;
		lpPoint->y = y;
		return TRUE;
	}
	return FALSE;
}

HWND WINAPI
GetCapture(VOID)
{
	return capturewp;
}

HWND WINAPI
SetCapture(HWND hwnd)
{
	HWND	oldCapture = capturewp;

	capturewp = hwnd;
	MwCheckMouseWindow();
	return oldCapture;
}

BOOL WINAPI
ReleaseCapture(VOID)
{
	capturewp = NULL;
	MwCheckMouseWindow();
	return TRUE;
}

struct timer {			/* private timer structure*/
	HWND	hwnd;		/* window associated with timer, NULL if none*/
	UINT	idTimer;	/* id for timer*/
	UINT	uTimeout;	/* timeout value, in msecs*/
	DWORD	dwClockExpires;	/* GetTickCount timer expiration value*/
	TIMERPROC lpTimerFunc;	/* callback function*/
};

static struct timer timer;	/* single global timer FIXME*/

UINT WINAPI
SetTimer(HWND hwnd, UINT idTimer, UINT uTimeout, TIMERPROC lpTimerFunc)
{
	static UINT nextID = 0;	/* next ID when hwnd is NULL*/

	/* assign timer id based on valid window handle*/
	timer.hwnd = hwnd;
	timer.idTimer = hwnd? idTimer: ++nextID;
	timer.uTimeout = uTimeout;
	timer.dwClockExpires = GetTickCount() + uTimeout;
	timer.lpTimerFunc = lpTimerFunc;

	return timer.idTimer;
}

BOOL WINAPI
KillTimer(HWND hwnd, UINT idTimer)
{
	if(timer.hwnd == hwnd && timer.idTimer == idTimer) {
		timer.uTimeout = 0;
		return TRUE;
	}
	return FALSE;
}

/*
 * Return the next timeout value in msecs
 */
UINT
MwGetNextTimeoutValue(void)
{
	int	timeout;

	if(timer.uTimeout) {
		timeout = timer.dwClockExpires - GetTickCount();
		if(timeout > 0)
			return timeout;
	}
	return 0;
}

/*
 * Check if any timers have expired by looking at current system ticks
 */
void
MwHandleTimers(void)
{
	int	timeout;
	DWORD	dwTime;

	/* check if timer running*/
	if(timer.uTimeout == 0)
		return;

	/* determine if timer expired*/
	dwTime = GetTickCount();
	timeout = timer.dwClockExpires - dwTime;
	if(timeout > 0)
		return;

	/* call timer function or post timer message*/
	if(timer.lpTimerFunc)
		timer.lpTimerFunc(timer.hwnd, WM_TIMER, timer.idTimer, dwTime);
	else PostMessage(timer.hwnd, WM_TIMER, timer.idTimer, 0);

	/* reset timer*/
	timer.dwClockExpires = dwTime + timer.uTimeout;
}

int WINAPI
GetSystemMetrics(int nIndex)
{
	switch(nIndex) {
	case SM_CXSCREEN:
		return scrdev.xvirtres;
	case SM_CYSCREEN:
		return scrdev.yvirtres;
	case SM_CXVSCROLL:
		return mwSYSMETRICS_CXVSCROLL;
	case SM_CYHSCROLL:
		return mwSYSMETRICS_CYHSCROLL;
	case SM_CYCAPTION:
		/* + 1 for line under caption*/
		return mwSYSMETRICS_CYCAPTION + 1;
	case SM_CXBORDER:
		return mwSYSMETRICS_CXBORDER;
	case SM_CYBORDER:
		return mwSYSMETRICS_CYBORDER;
	case SM_CYMENU:
		break;		/* FIXME: 19 when menubars work*/
	case SM_CYVSCROLL:
		return mwSYSMETRICS_CYVSCROLL;
	case SM_CXHSCROLL:
		return mwSYSMETRICS_CXHSCROLL;
	case SM_CXFRAME:
	case SM_CXDLGFRAME:
		return mwSYSMETRICS_CXFRAME;
	case SM_CYFRAME:
	case SM_CYDLGFRAME:
		return mwSYSMETRICS_CYFRAME;
	}
	return 0;
}

HWND WINAPI
GetDlgItem(HWND hDlg, int nIDDlgItem)
{
	HWND	wp;

	if(hDlg) {
		for(wp=hDlg->children; wp; wp=wp->siblings)
			if(wp->id == nIDDlgItem)
				return wp;
	}
	return 0;
}
