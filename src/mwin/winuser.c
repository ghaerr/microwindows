/*
 * Copyright (c) 1999, 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * Win32 API upper level window creation, management and msg routines
 *
 * Modifications:
 *  Date        Author                  Description
 *  2003/09/01  Gabriele Brugnoni       Implemented multiple timers support
 *  2003/09/22  Gabriele Brugnoni       InvalidateRect adds NC-Area only when needed.
 *  2003/09/22  Gabriele Brugnoni       Implemented bErase for Invalidate functions.
 *  2003/09/24  Gabriele Brugnoni       Fixed WM_ACTIVATE msg sent in SetFocus
 *                                      (was sent active and inactive twice at new focused win)
 *  2003/09/24  Gabriele Brugnoni       WA_ACTIVE with WA_INACTIVE is sent before changing
 *                                      the pointer focus variable.
 *  2003/09/24  Gabriele Brugnoni       Implemented WM_SYSCHAR for ALT-Key
 *  2010/04/23	Ludwig Ertl				Fixed KillTimer to work in TimerProc for current timer
 *  									Implemented SetProp/GetProp/RemoveProp
 *  2010/06/24  Ludwig Ertl				Implemented RegisterHotKey/UnregisterHotKey
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uni_std.h"
#include "osdep.h"

#define PAINTONCE	1	/* =1 to queue paint msgs only once*/
#define MOUSETEST	1

MWLISTHEAD mwMsgHead;		/* application msg queue*/
MWLISTHEAD mwClassHead;		/* register class list*/
MWLISTHEAD mwHotkeyHead={0};/* Hotkey table list */

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
RECT mwSYSPARAM_WORKAREA = {0, 0, -1, -1};

struct timer {			/* private timer structure*/
	HWND	hwnd;		/* window associated with timer, NULL if none*/
	UINT	idTimer;	/* id for timer*/
	UINT	uTimeout;	/* timeout value, in msecs*/
	DWORD	dwClockExpires;	/* GetTickCount timer expiration value*/
	TIMERPROC lpTimerFunc;	/* callback function*/
	BOOL   bRemove;		/* Remove timer entry on next run */
	struct timer *next;
};
static struct timer *timerList = NULL;	/* global timer list*/

/* property */
typedef struct {
	MWLIST link;
	ATOM Atom;
	HANDLE hData;
} MWPROP;

typedef struct {
	MWLIST link;
	int id;
	HWND hWnd;
	UINT fsModifiers;
	UINT vk;
} MWHOTKEY;


static void MwOffsetChildren(HWND hwnd, int offx, int offy);
static void MwRemoveWndFromTimers(HWND hwnd);
static BOOL MwRemoveWndFromHotkeys (HWND hWnd);

LRESULT WINAPI
CallWindowProc(WNDPROC lpPrevWndFunc, HWND hwnd, UINT Msg, WPARAM wParam,
	LPARAM lParam)
{
	return (*lpPrevWndFunc)(hwnd, Msg, wParam, lParam);
}

LRESULT WINAPI
SendMessage(HWND hwnd, UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if(IsWindow(hwnd) && hwnd->lpfnWndProc) {
		hwnd->paintSerial = mwpaintSerial; /* assign msg sequence #*/
		return (*hwnd->lpfnWndProc)(hwnd, Msg, wParam, lParam);
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
			pMsg = MwItemAddr(p, MSG, link);
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
		 * the window is the moving window, or its the root window (for wallpaper).
		 */
		if(wp->gotPaintMsg == PAINT_NEEDSPAINT &&
		    (!dragwp || dragwp == wp || wp == rootwp)) {
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
			MwSelect(FALSE);
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
		MwSelect(FALSE);
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
	while(!PeekMessage(lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax,PM_REMOVE)) {
		/* Call select to suspend process until user input or scheduled timer */
		MwSelect(TRUE);
	    MwHandleTimers();
#if MW_CALL_IDLE_HANDLER	    
	    idle_handler();
#endif
		continue;
	}
	return lpMsg->message != WM_QUIT;
}

BOOL WINAPI
TranslateMessage(CONST MSG *lpMsg)
{
	static BOOL bAltStatus;	/* store ALT status to send WM_SYSCHAR */

	/* Creat the WM_CHAR on WM_KEYDOWN messages*/
   	/* if bit 24 in lParam is ON (control key), don't post WM_CHAR */
	if(lpMsg && (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP) &&
		(lpMsg->wParam == VK_MENU) )
		bAltStatus = (lpMsg->message == WM_KEYDOWN);

	if(lpMsg && (lpMsg->message == WM_KEYDOWN) &&
	   (!(lpMsg->lParam & (1<<24)) || bAltStatus) ) {
		if( !bAltStatus )
			PostMessage(lpMsg->hwnd, WM_CHAR, lpMsg->wParam, lpMsg->lParam);
		else
			PostMessage(lpMsg->hwnd, WM_SYSCHAR, lpMsg->wParam, lpMsg->lParam);
	}
	return FALSE;
}

LONG WINAPI
DispatchMessage(CONST MSG *lpMsg)
{
	return SendMessage(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
}

static MWHOTKEY *MwFindHotkey (int id)
{
	PMWLIST		p;
	MWHOTKEY	*pHotkey;

	for (p=mwHotkeyHead.head; p; p=p->next) {
		pHotkey = MwItemAddr (p, MWHOTKEY, link);
		if (pHotkey->id == id)
			return pHotkey;
	}
	return NULL;
}

static BOOL MwRemoveWndFromHotkeys (HWND hWnd)
{
	PMWLIST		p, pNext;
	MWHOTKEY	*pHotkey;
	BOOL        bRet = FALSE;

	for (p=mwHotkeyHead.head; p; p=pNext) {
		pNext = p->next;
		pHotkey = MwItemAddr (p, MWHOTKEY, link);
		if (pHotkey->hWnd == hWnd) {
			GdListRemove(&mwHotkeyHead, &pHotkey->link);
			GdItemFree(pHotkey);
			bRet = TRUE;
		}
	}
	return bRet;
}

BOOL MwDeliverHotkey (WPARAM VK_Code, BOOL pressed)
{
	PMWLIST		p;
	MWHOTKEY	*pHotkey;

	if (!pressed) return FALSE;
	for (p=mwHotkeyHead.head; p; p=p->next) {
		pHotkey = MwItemAddr (p, MWHOTKEY, link);
		if (pHotkey->vk == VK_Code && IsWindow(pHotkey->hWnd)) {
			PostMessage (pHotkey->hWnd, WM_HOTKEY, 0, MAKELPARAM(0, VK_Code));
			return TRUE;
		}
	}
	return FALSE;
}

BOOL RegisterHotKey(HWND hWnd, int id, UINT fsModifiers, UINT vk)
{
	MWHOTKEY *pHotkey;

	if (MwFindHotkey (id) || !(pHotkey = GdItemNew (MWHOTKEY)))
		return FALSE;
	pHotkey->hWnd        = hWnd;
	pHotkey->id          = id;
	pHotkey->fsModifiers = fsModifiers;
	pHotkey->vk          = vk;
	GdListAdd(&mwHotkeyHead, &pHotkey->link);

	return TRUE;
}

BOOL UnregisterHotKey(HWND hWnd, int id)
{
	MWHOTKEY *pHotkey = MwFindHotkey(id);

	if (!pHotkey) return FALSE;
	GdListRemove(&mwHotkeyHead, &pHotkey->link);
	GdItemFree(pHotkey);
	return TRUE;
}

/* find the registered window class struct by name*/
PWNDCLASS
MwFindClassByName(LPCSTR lpClassName)
{
	PMWLIST		p;
	PWNDCLASS	pClass;

	for(p=mwClassHead.head; p; p=p->next) {
		pClass = MwItemAddr(p, WNDCLASS, link);
		if(strcasecmp(pClass->szClassName, lpClassName) == 0)
			return pClass;
	}
	return NULL;
}

HMODULE WINAPI
GetModuleHandle(LPCSTR lpModuleName)
{
	return rootwp->hInstance;
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
#if !WINEXTRA
	pClass->lpfnWndProcBridge = NULL;	/* always init bridge to NULL*/
#endif
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
	int			titLen;
	static int	nextx = 20;
	static int	nexty = 20;
	
#if MW_FEATURE_RESIZEFRAME
	static int framechk = 1;
	if ((dwStyle & WS_OVERLAPPEDWINDOW) && framechk) {
		/* remove Microwindows frame and adjust placement to 0,0*/
		dwStyle &= ~WS_OVERLAPPEDWINDOW;
		dwStyle |= WS_POPUP;
		x = y = 0;

		/* resize OS frame to app size*/
		GdResizeFrameWindow(nWidth, nHeight, lpWindowName);
		framechk = 0;
	}
#endif
	/* WARNING: All modification made here should be repeated 
	   in MwInitialize for the rootwp window */

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
	wp->lpfnWndProc = pClass->lpfnWndProc;
	wp->lpfnWndProcBridge = pClass->lpfnWndProcBridge;
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
	wp->id = (int)hMenu;				// OK: Not pointer. Menu id always passed as int.
	wp->gotPaintMsg = PAINT_PAINTED;

	titLen = 0;
	if (lpWindowName != NULL)
		titLen = strlen(lpWindowName);
	if (titLen < 64) titLen = 64; /* old mw compatibility */
	wp->szTitle = (LPTSTR)malloc(titLen + 1);
	if (wp->szTitle == NULL) {
		free(wp);
		return NULL;
	}
	if (lpWindowName != NULL)
		strcpy(wp->szTitle, lpWindowName);
	else
		wp->szTitle[0] = '\0';

#if UPDATEREGIONS
	wp->update = GdAllocRegion();
#endif
	wp->nextrabytes = pClass->cbWndExtra;
	wp->hInstance = hInstance;
	wp->nEraseBkGnd = 1;
	wp->paintBrush = NULL;
	wp->paintPen = NULL;

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

	if (wp == rootwp || !IsWindow (hwnd))
		return;

	/*
	 * Unmap the window.
	 */
	if (wp->unmapcount == 0)
		MwHideWindow(wp, FALSE, FALSE);

	if(bSendMsg)
		SendMessage(hwnd, WM_DESTROY, 0, 0L);

	/*
	 * Remove from timers
	 */
	MwRemoveWndFromTimers(hwnd);

	/*
	 * Remove hotkeys
	 */
	MwRemoveWndFromHotkeys(hwnd);

	/*
	 * Disable all sendmessages to this window.
	 */
	wp->lpfnWndProc = NULL;
	wp->lpfnWndProcBridge = NULL;

	/*
	 * Destroy all children, sending WM_DESTROY messages.
	 */
	while (wp->children)
		MwDestroyWindow(wp->children, bSendMsg);

	wp->pClass = NULL;

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
		while (prevwp && prevwp->siblings != wp)
			prevwp = prevwp->siblings;
		if (prevwp) prevwp->siblings = wp->siblings;
	}
	wp->siblings = NULL;

	/*
	 * Remove this window from the complete list of windows.
	 */
	prevwp = listwp;
	if (prevwp == wp)
		listwp = wp->next;
	else {
		while (prevwp->next && prevwp->next != wp)
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
		pmsg = MwItemAddr(p, MSG, link);
		if(pmsg->hwnd == wp) {
			p = p->next;
			GdListRemove(&mwMsgHead, &pmsg->link);
			GdItemFree(pmsg);
		} else
			p = p->next;
	}

	/*
	 * Remove all properties from this window.
	 */
	for(p=hwnd->props.head; p; ) {
		MWPROP  *pProp = MwItemAddr(p, MWPROP, link);
		p = p->next;
		GdListRemove (&hwnd->props, &pProp->link);
		GdItemFree (pProp);
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

	if (wp->szTitle) {
		free(wp->szTitle);
		wp->szTitle = NULL;
	}

#if UPDATEREGIONS
	if (wp->update) {
		GdDestroyRegion(wp->update);
		wp->update = NULL;
	}
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
		return TRUE;

	case SW_MAXIMIZE:
		if (!(hwnd->style & WS_MAXIMIZE)) {
			RECT rc;

			hwnd->restorerc = hwnd->winrect;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
			MoveWindow(hwnd, rc.top, rc.left, rc.right - rc.left,
					rc.bottom - rc.top, TRUE);
			hwnd->style |= WS_MAXIMIZE;
		}
		break;
	case SW_RESTORE:
	case SW_SHOWDEFAULT:
		if(hwnd->style & WS_MAXIMIZE) {
			RECT rc = hwnd->restorerc;
			MoveWindow(hwnd, rc.left, rc.top,
				rc.right-rc.left, rc.bottom-rc.top,
				TRUE);
			hwnd->style &= ~WS_MAXIMIZE;
		}
		break;
	default:
		if (hwnd->style & WS_VISIBLE)
			return FALSE;
	}
	hwnd->style |= WS_VISIBLE;
	MwShowWindow(hwnd, TRUE);
	return TRUE;
}

BOOL WINAPI
InvalidateRect(HWND hwnd, CONST RECT *lpRect, BOOL bErase)
{
	if(!hwnd)
		MwRedrawScreen();
	else {
#if UPDATEREGIONS
		RECT	rc;

		/* add to update region*/
		if(!lpRect)
			GetClientRect(hwnd, &rc);
		else
			rc = *lpRect;

		if( hwnd->style & WS_CAPTION )
			rc.bottom += mwSYSMETRICS_CYCAPTION;
		if( (hwnd->style & (WS_BORDER | WS_DLGFRAME)) != 0 ) {
			rc.bottom += mwSYSMETRICS_CYFRAME + 1;
			rc.right += mwSYSMETRICS_CXFRAME;
		}

		MwUnionUpdateRegion(hwnd, rc.left, rc.top,
			rc.right-rc.left, rc.bottom-rc.top, TRUE);

		/* if update region not empty, mark as needing painting*/
		if(hwnd->update->numRects != 0)
#endif

			if(hwnd->gotPaintMsg == PAINT_PAINTED)
				hwnd->gotPaintMsg = PAINT_NEEDSPAINT;
		if( bErase )
			hwnd->nEraseBkGnd++;
	}
	return TRUE;
}

#if UPDATEREGIONS
/* add region to window update region*/
BOOL WINAPI
InvalidateRgn(HWND hwnd, HRGN hrgn, BOOL bErase)
{
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
		if( bErase )
			hwnd->nEraseBkGnd++;
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
		if(!lprc) {
			GetClientRect(hwnd, &rc);
			if( hwnd->style & WS_CAPTION )
				rc.bottom += mwSYSMETRICS_CYCAPTION;
			if( (hwnd->style & (WS_BORDER | WS_DLGFRAME)) != 0 ) {
				rc.bottom += mwSYSMETRICS_CYFRAME + 1;
		rc.right += mwSYSMETRICS_CXFRAME;
			}
		} else
			rc = *lprc;

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

BOOL WINAPI
RedrawWindow(HWND hWnd, const RECT *lprcUpdate, HRGN hrgnUpdate, UINT flags)
{
	/* currently ignores hrgnUpdate*/

	if (flags & RDW_INVALIDATE)
		InvalidateRect(hWnd, lprcUpdate, (flags & RDW_ERASE));
	if (flags & RDW_FRAME)
		MwPaintNCArea(hWnd);
	if (flags & (RDW_UPDATENOW|RDW_ERASENOW))
		UpdateWindow(hWnd);
	return TRUE;
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

	if (!IsWindow(hwnd))
		return 0;

	if(hwnd == focuswp)
		return focuswp;

	oldfocus = focuswp;
	top = MwGetTopWindow(oldfocus);
	top2 = MwGetTopWindow(hwnd);
	SendMessage(oldfocus, WM_KILLFOCUS, (WPARAM)hwnd, 0L);
	/* send deactivate. Note: should be sent before changing the focuswp var*/
	if(top2 != top)
		SendMessage(top, WM_ACTIVATE, (WPARAM)MAKELONG(WA_INACTIVE, 0), (LPARAM)top2);

	focuswp = hwnd;
	SendMessage(focuswp, WM_SETFOCUS, (WPARAM)oldfocus, 0L);

	/* FIXME SetActiveWindow() here?*/
	if(top2 != top) {
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
		/* send activate*/
		SendMessage(top2, WM_ACTIVATE, (WPARAM)MAKELONG(WA_ACTIVE, 0),
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
	while(IsWindow(hwnd) && (hwnd->style & WS_CHILD))
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

HWND WINAPI
SetParent(HWND hwnd, HWND parent)
{
	HWND old_parent;
	BOOL was_visible;
	POINT pt;

	if (!hwnd)
		return NULL;

	if (!parent)
		parent = GetDesktopWindow();

	if (!IsWindow(parent))
		return NULL;

	/* Some applications try to set a child as a parent */
	if (IsChild(hwnd, parent))
		return NULL;

	old_parent = hwnd->parent;

	/*
	 * Windows hides the window first, then shows it again
	 * including the WM_SHOWWINDOW messages and all
	 */
	was_visible = ShowWindow(hwnd, SW_HIDE);

	pt.x = hwnd->winrect.left;
	pt.y = hwnd->winrect.top;
	ScreenToClient(hwnd->parent, &pt);

	hwnd->parent = parent;

	if (parent == GetDesktopWindow() && !(hwnd->style & WS_CLIPSIBLINGS))
		hwnd->style |= WS_CLIPSIBLINGS;

	SetWindowPos(hwnd, (0 == (hwnd->exstyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
			pt.x, pt.y, 0, 0, SWP_NOSIZE);

	if (was_visible)
		ShowWindow(hwnd, SW_SHOW);

	return old_parent;
}

HWND WINAPI
GetAncestor(HWND hwnd, UINT type)
{
	switch (type) {
	case GA_PARENT:
	case GA_ROOT:
	case GA_ROOTOWNER:
	default:
		return GetParent(hwnd);
	}
}

BOOL WINAPI
EnableWindow(HWND hwnd, BOOL bEnable)
{
	if(!hwnd)
		return TRUE;
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

// 64bit function
ULONG_PTR WINAPI
GetClassLongPtr(HWND hwnd, int nIndex)
{
	switch(nIndex) {
	case GCL_HBRBACKGROUND:
		return (ULONG_PTR)hwnd->pClass->hbrBackground;
	}
	return (ULONG_PTR)GetClassLong(hwnd, nIndex);
}

// 32bit function
DWORD WINAPI
GetClassLong(HWND hwnd, int nIndex)
{
	switch(nIndex) {
	case GCL_CBWNDEXTRA:
		return (DWORD)hwnd->pClass->cbWndExtra;
	case GCL_HBRBACKGROUND:
		assert(sizeof(LONG_PTR) <= 32);		// 64bit must use GetClassLongPtr
		return (DWORD)hwnd->pClass->hbrBackground;		// OK: Pointer size checked above and is 32 bit.
	case GCL_HCURSOR:
	case GCL_HICON:
	case GCL_HMODULE:
	case GCL_CBCLSEXTRA:
	case GCL_WNDPROC:
	case GCL_HICONSM:
	default:					// NYI
		DPRINTF("GetClassLong unsupported GCL_ flag\n");
		break;
	}
	return 0;
}

// 64bit function
LONG_PTR WINAPI
GetWindowLongPtr(HWND hwnd, int nIndex)
{
	switch(nIndex) {
	case GWL_WNDPROC:
		return (LONG_PTR)hwnd->lpfnWndProc;
	case GWL_WNDPROCBRIDGE:
		return (LONG_PTR)hwnd->lpfnWndProcBridge;
	case GWL_HINSTANCE:
		return (LONG_PTR)hwnd->hInstance;
	case GWL_HWNDPARENT:
		return (LONG_PTR)hwnd->parent;
	case GWL_USERDATA:
		return hwnd->userdata;
	case GWL_ID:
		return (LONG_PTR)hwnd->id;
	case GWL_STYLE:
		return (LONG_PTR)hwnd->style;
	case GWL_EXSTYLE:
		return (LONG_PTR)hwnd->exstyle;
	default:
#if DEBUG
		if (nIndex < 0) {
			DPRINTF("GetWindowLongPtr unsupported GWL_ flag\n");
			break;
		} else if(nIndex+sizeof(LONG_PTR) > hwnd->nextrabytes) {
			DPRINTF("GetWindowLongPtr bad nIndex\n");
			break;
		} else
#endif
#ifdef ARCH_NEED_ALIGN32  /* architecture needs data to be 32bit aligned*/
		if(nIndex & (sizeof(LONG_PTR)-1)) {
			DPRINTF("GetWindowLongPtr bad alignment\n");
			assert(sizeof(LONG_PTR) == 32);
			return MAKELONG( MAKEWORD(hwnd->extrabytes[nIndex+0], hwnd->extrabytes[nIndex+1]),
					 MAKEWORD(hwnd->extrabytes[nIndex+2], hwnd->extrabytes[nIndex+3]));
			break;
		}
#endif	
		return *(LONG_PTR *)&hwnd->extrabytes[nIndex];
	}
	return 0;
}

// 32bit function
LONG WINAPI
GetWindowLong(HWND hwnd, int nIndex)
{
#if DEBUG
	if (sizeof(char *) > 32) {	// 64bit systems should use GetWindowLongPtr and revised indexes
		switch(nIndex) {
		case GWL_WNDPROC:
		case GWL_HINSTANCE:
		case GWL_HWNDPARENT:
		case GWL_USERDATA:
			DPRINTF("GetWindowLong 32bit returns DWORD, revise windows extrabytes\n");
			break;
		}
	}
	assert(sizeof(LONG) >= sizeof(LONG_PTR)); // if LONG truncates LONG_PTR, must recode this function
#endif
	return (LONG)GetWindowLongPtr(hwnd, nIndex);
}

// 64bit function
LONG_PTR WINAPI
SetWindowLongPtr(HWND hwnd, int nIndex, LONG_PTR lNewLong)
{
	LONG_PTR oldval = GetWindowLongPtr(hwnd, nIndex);

	switch(nIndex) {
	case GWL_WNDPROC:
		hwnd->lpfnWndProc = (WNDPROC)lNewLong;
		break;
	case GWL_WNDPROCBRIDGE:
		hwnd->lpfnWndProcBridge = (WNDPROC)lNewLong;
		break;
	case GWL_HINSTANCE:
		hwnd->hInstance = (HINSTANCE)lNewLong;
		break;
	case GWL_USERDATA:
		hwnd->userdata = lNewLong;
		break;
	case GWL_STYLE:
		hwnd->style = lNewLong;		// style is currently DWORD
		break;
	case GWL_EXSTYLE:
		hwnd->exstyle = lNewLong;	// exstyle currently DWORD
		break;
	case GWL_HWNDPARENT:			// NYI
	case GWL_ID:				// NYI
		DPRINTF("mwSetWindowLongPtr unsupported GWL_ flag\n");
		break;
	default:
#if DEBUG
		if (nIndex < 0) {
			DPRINTF("mwSetWindowLongPtr unsupported GWL_ flag\n");
			break;
		} else
		if(nIndex+sizeof(LONG_PTR) > hwnd->nextrabytes) {
			DPRINTF("mwSetWindowLongPtr bad nIndex\n");
			break;
		} else
#endif
#ifdef ARCH_NEED_ALIGN32 /* architecture needs data to be 32bit aligned*/
		if(nIndex & (sizeof(LONG_PTR)-1)) {
			DPRINTF("mwSetWindowLongPtr bad alignment\n");
			assert(sizeof(LONG_PTR) == 32);
			hwnd->extrabytes[nIndex+0] = LOBYTE(LOWORD(lNewLong));
			hwnd->extrabytes[nIndex+1] = HIBYTE(LOWORD(lNewLong));
			hwnd->extrabytes[nIndex+2] = LOBYTE(HIWORD(lNewLong));
			hwnd->extrabytes[nIndex+3] = HIBYTE(HIWORD(lNewLong));
			break;
		}
#endif
		*(LONG_PTR *)&hwnd->extrabytes[nIndex] = lNewLong;
		break;
	}
	return oldval;
}

// 32bit function
LONG WINAPI
SetWindowLong(HWND hwnd, int nIndex, LONG lNewLong)
{
#if DEBUG
	if (sizeof(char *) > 32) {	// 64bit systems should use GetWindowLongPtr and revised indexes
		switch(nIndex) {
		case GWL_WNDPROC:
		case GWL_HINSTANCE:
		case GWL_HWNDPARENT:
		case GWL_USERDATA:
			DPRINTF("GetWindowLong 32bit returns DWORD, revise windows extrabytes\n");
			break;
		}
	}
	assert(sizeof(LONG) >= sizeof(LONG_PTR)); // if LONG truncates LONG_PTR, must recode this function
#endif
	return (LONG)SetWindowLongPtr(hwnd, nIndex, (LONG_PTR)lNewLong);
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

/* -------------- begin STATIC atom functions --------------
 * FIXME:
 * Microwindows currently doesn't have functions for handling the Atom
 * table. TODO: Replace them by correct Atom handling functions.
 *
 * Therefore we are just implementing a stupid function that calculates
 * a "unique" value from a string and returns this as an "atom" so that
 * out property methods can work as expected.
 *
 */
ATOM WINAPI
GlobalFindAtom(LPCSTR lpString)
{
	LPCSTR p;
	ATOM atom = 0;

	for (p = lpString; *p; p++)
		atom = ((atom + *p) % 0xFFFF);
	return atom;
}

ATOM WINAPI
GlobalAddAtom(LPCSTR lpString)
{
	return GlobalFindAtom (lpString);
}
/* -------------- end STATIC atom functions -------------- */

BOOL WINAPI
SetProp(HWND hWnd, LPCSTR lpString, HANDLE hData)
{
	MWPROP *pProp;

	if (!(pProp = GdItemNew(MWPROP)))
		return FALSE;
	/* check if 16 bit atom passed instead of pointer*/
	if (PTR_IS_ATOM(lpString))
		pProp->Atom = LOWORD((DWORD)lpString);			// OK: Not pointer. Atom passed in low 16 bits.
	else
		pProp->Atom = GlobalAddAtom(lpString);
	pProp->hData = hData;

	GdListAdd (&hWnd->props, &pProp->link);
	return TRUE;
}

HANDLE WINAPI
GetProp(HWND hWnd, LPCSTR lpString)
{
	ATOM Atom;
	PMWLIST p;
	MWPROP *pProp;

	/* check if 16 bit atom passed instead of pointer*/
	if (PTR_IS_ATOM(lpString))
		Atom = LOWORD((DWORD)lpString);				// OK: Not pointer. Atom passed in low 16 bits.
	else
		Atom = GlobalFindAtom(lpString);

	for(p=hWnd->props.head; p; p=p->next) {
		pProp = MwItemAddr(p, MWPROP, link);
		if (pProp->Atom == Atom)
			return pProp->hData;
	}
	return NULL;
}

HANDLE WINAPI
RemoveProp(HWND hWnd, LPCSTR lpString)
{
	ATOM Atom;
	PMWLIST p;
	MWPROP *pProp;
	HANDLE hRet;

	/* check if 16 bit atom passed instead of pointer*/
	if (PTR_IS_ATOM(lpString))
		Atom = LOWORD((DWORD)lpString);				// OK: Not pointer. Atom passed in low 16 bits.
	else
		Atom = GlobalFindAtom(lpString);

	for(p=hWnd->props.head; p; p=p->next) {
		pProp = MwItemAddr(p, MWPROP, link);
		if (pProp->Atom == Atom) {
			hRet = pProp->hData;
			GdListRemove(&hWnd->props, &pProp->link);
			GdItemFree(pProp);
			return hRet;
		}
	}
	return NULL;
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
	BOOL r = SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)(LPCSTR)lpString);
	MwPaintNCArea(hwnd);
	return r;
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

BOOL WINAPI
SetWindowPos(HWND hwnd, HWND hwndInsertAfter, int x, int y, int cx, int cy, UINT fuFlags)
{
	int		hidden;
	BOOL		bMove, bSize, bZorder;
	MWCOORD		offx = 0, offy = 0;	/* = 0 for bad gcc warning*/
	WINDOWPOS	winpos;

	if(!hwnd || hwnd == rootwp || cx < 0 || cy < 0)
		return FALSE;

	if (!IsWindow(hwnd))
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
		switch((int)hwndInsertAfter) {			// OK: intentional HWND to int cast
		case (int)(HWND)HWND_TOP:				// OK: intentional HWND to int cast
			MwRaiseWindow(hwnd);
			break;
		case (int)(HWND)HWND_BOTTOM:			// OK: intentional HWND to int cast
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
	if(bMove) {
		hwnd->winrect.left = x;
		hwnd->winrect.top = y;
		hwnd->winrect.right = x + cx;
		hwnd->winrect.bottom = y + cy;
	}
	if(bSize) {
		hwnd->winrect.right = hwnd->winrect.left + cx;
		hwnd->winrect.bottom = hwnd->winrect.top + cy;
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
	hwnd->nEraseBkGnd++;
	if(!bZorder && !hidden)
		MwShowWindow(hwnd, FALSE);

	return TRUE;
}

BOOL SetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
	if ((hWnd->style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
	{
		SetWindowPos (hWnd, NULL, lpwndpl->rcNormalPosition.left,
				lpwndpl->rcNormalPosition.top,
				lpwndpl->rcNormalPosition.right - lpwndpl->rcNormalPosition.left,
				lpwndpl->rcNormalPosition.bottom - lpwndpl->rcNormalPosition.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
	}
	ShowWindow (hWnd, lpwndpl->showCmd);
	hWnd->restorerc = lpwndpl->rcNormalPosition;

	return TRUE;
}

BOOL GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
	RECT rc;

	lpwndpl->flags = 0;
	lpwndpl->rcNormalPosition = hWnd->restorerc;
	lpwndpl->showCmd = (hWnd->style & WS_MAXIMIZE)?SW_MAXIMIZE:SW_SHOWNORMAL;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
	lpwndpl->ptMaxPosition.x = rc.left;
	lpwndpl->ptMaxPosition.y = rc.top;
	memset (&lpwndpl->ptMinPosition, 0, sizeof(lpwndpl->ptMinPosition));
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

HCURSOR WINAPI
LoadCursor(HINSTANCE hInstance, LPCSTR lpCursorName)
{
	return 0;	/* nyi*/
}

HCURSOR WINAPI
SetCursor(HCURSOR hCursor)
{
	return 0;	/* nyi*/
}

HCURSOR WINAPI
GetCursor(VOID)
{
	return 0;	/* nyi*/
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

HWND GetWindow(HWND hWnd, UINT uCmd)
{
	switch (uCmd)
	{
	case GW_OWNER: return hWnd->parent;
	case GW_CHILD: return hWnd->children;
	case GW_HWNDNEXT: return hWnd->siblings;
	}
	return NULL;
}

HWND GetMenu (HWND hWnd)
{
	return NULL;
}

HWND GetForegroundWindow(VOID)
{
	return focuswp;
}

HWND WindowFromPoint(POINT pt)
{
	HWND wp,wp1=NULL;

	for(wp=GetDesktopWindow()->children; wp; wp=wp->next)
	{
		if (!(wp->style & WS_VISIBLE) || (wp->style & WS_DISABLED)
				|| (wp->style & WS_CHILD) || !PtInRect (&wp->winrect, pt))
			continue;
		wp1 = wp;
		break;
	}

	return wp1;
}

UINT WINAPI
SetTimer(HWND hwnd, UINT idTimer, UINT uTimeout, TIMERPROC lpTimerFunc)
{
	struct timer *tm = (struct timer *) malloc ( sizeof(struct timer) );
	static UINT nextID = 0;	/* next ID when hwnd is NULL*/

	/* assign timer id based on valid window handle*/
	if( tm == NULL )
		return 0;
	
	tm->hwnd = hwnd;
	tm->idTimer = hwnd? idTimer: ++nextID;
	tm->uTimeout = uTimeout;
	tm->dwClockExpires = GetTickCount() + uTimeout;
	tm->lpTimerFunc = lpTimerFunc;
	tm->next = timerList;
	tm->bRemove = FALSE;
	timerList = tm;

	return tm->idTimer;
}

BOOL WINAPI
KillTimer(HWND hwnd, UINT idTimer)
{
	struct timer *tm;

	/* Just mark it for removal, actual removal will
	 * be done in MwHandleTimers, otherwise killing a
	 * timer in a TimerProc will end up with memory errors
	 */
	for (tm=timerList; tm != NULL; tm = tm->next)
		if( (tm->hwnd == hwnd) && (tm->idTimer == idTimer) )
			return tm->bRemove = TRUE;
	return FALSE;
}

/*
 * Return the next timeout value in msecs
 */
UINT
MwGetNextTimeoutValue(void)
{
	int	bestTimeout = -1;
	int	timeout;
	struct timer *tm = timerList;

	while ( tm != NULL ) {
		timeout = tm->dwClockExpires - GetTickCount();
		if( (timeout > 0) && ((timeout < bestTimeout) || (bestTimeout == -1)) )
			bestTimeout = timeout;
		else {
			/*  If timer has expired, return zero*/
			if (timeout <= 0)
				return 0;
		}
		tm = tm->next;
	}

	return bestTimeout;
}

/*
 * Check if any timers have expired by looking at current system ticks
 */
void
MwHandleTimers(void)
{
	struct timer *tm = timerList;
	struct timer *ltm = NULL;
	DWORD	dwTime = 0;	/* should be system time in UTC*/

	while (tm != NULL) {
		if (!tm->bRemove) {
			if (GetTickCount() >= tm->dwClockExpires) {

				/* call timer function or post timer message*/
				if (tm->lpTimerFunc)
					tm->lpTimerFunc(tm->hwnd, WM_TIMER, tm->idTimer, dwTime);
				else
					PostMessage (tm->hwnd, WM_TIMER, tm->idTimer, 0);

				/* reset timer*/
				tm->dwClockExpires = GetTickCount() + tm->uTimeout;
			}
		}
		if (tm->bRemove) {
			if(ltm != NULL)
				ltm->next = tm->next;
			else
				timerList = tm->next;
			free (tm);
			tm = ltm?ltm->next:timerList;
		} else {
			ltm = tm;
			tm = tm->next;
		}
	}
}

/*
 *  Check in timers list if hwnd is present and remove it.
 */
static void
MwRemoveWndFromTimers(HWND hwnd)
{
	struct timer *next;
	struct timer *tm = timerList;

	while ( tm != NULL ) {
		next = tm->next;
		if( tm->hwnd == hwnd )
			KillTimer ( tm->hwnd, tm->idTimer );
		tm = next;
	}
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

BOOL WINAPI
SystemParametersInfo (UINT uiAction,  UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	switch (uiAction) {
	case SPI_GETWORKAREA:
		*(RECT*)pvParam = mwSYSPARAM_WORKAREA;
		return TRUE;
	case SPI_SETWORKAREA:
		if (pvParam)
			mwSYSPARAM_WORKAREA = *(RECT*)pvParam;
		else
			mwSYSPARAM_WORKAREA = rootwp->winrect;
		return TRUE;
	}
	return FALSE;
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

BOOL WINAPI
EnumChildWindows(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam)
{
	int	i;
	HWND *	hchilds;
	HWND	hwnd = hWndParent->children;
	int	count = 0;
	
	/*  Calculate the count of childs*/
	while (hwnd) {
		count++;
		hwnd = hwnd->siblings;
	}
	if (!count)
		return FALSE;

	/* Allocate an array of pointer, to store childs in reverse order*/
	hchilds = (HWND *)malloc(sizeof(HWND)*count);
	if (hchilds == NULL)
		return FALSE;
	
	hwnd = hWndParent->children;
	i = count;
    	while (hwnd && (i > 0)) {
		hchilds[--i] = hwnd;
		hwnd = hwnd->siblings;
	}
		
	/* Call lpEnumFunc with correct children order*/
	for (; i < count; i++)
		if (!lpEnumFunc(hchilds[i], lParam))
			break;
	free (hchilds);
	return TRUE;
}

int WINAPI
GetClassName(HWND hWnd, LPTSTR lpClassName, int nMaxCount)
{
	int	ln = 0;

	if (hWnd->pClass) {
		ln = strlen(hWnd->pClass->szClassName);
		if (ln > nMaxCount)
			ln = nMaxCount;
		strncpy(lpClassName, hWnd->pClass->szClassName, nMaxCount);
	}
	return ln;
}

BOOL WINAPI
SetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
	if (!hwnd || hwnd->unmapcount)
		hwnd = rootwp;
	hwnd->color_key = crKey;
	hwnd->alpha = bAlpha;
	hwnd->layered_flags = dwFlags;
	return TRUE;
}

BOOL WINAPI
GetLayeredWindowAttributes(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags)
{
	if (!hwnd || hwnd->unmapcount)
		hwnd = rootwp;
	*pcrKey = hwnd->color_key;
	*pbAlpha = hwnd->alpha;
	*pdwFlags = hwnd->layered_flags;
	return TRUE;
}

/*
 * Return # milliseconds elapsed since start of Microwindows
 * Granularity is 25 msec
 */
DWORD WINAPI
GetTickCount(VOID)
{
	return GdGetTickCount();
}
