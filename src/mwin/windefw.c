/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * DefWindowProc implementation for Micro-Windows
 *	This file should ideally only include windows.h, and be built
 *	on top of regular win32 api calls.  For speed, however,
 *	certain knowledge of the internal hwnd is known...
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include "wintools.h"		/* Draw3dBox, etc*/
#include <string.h>

/* desktop wallpaper image*/
static PMWIMAGEHDR	pImageWallpaper = NULL;

/* local routines*/
static void	GetCloseBoxRect(HWND hwnd, LPRECT lprc);
static void	DrawXORFrame(HWND hwnd,int x, int y, BOOL bDrawCurrent);
static RECT	lastrc;

BOOL
MwSetDesktopWallpaper(PMWIMAGEHDR pImage)
{
	pImageWallpaper = pImage;
	InvalidateRect(rootwp, NULL, TRUE);
	return TRUE;
}

/* needed only for XORMOVE repaint algorithm*/
static void
DrawXORFrame(HWND hwnd,int x, int y, BOOL bDrawCurrent)
{
	HDC	hdc;
	RECT	rc;

	hdc = GetDCEx(NULL, NULL, DCX_WINDOW|DCX_EXCLUDEUPDATE);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));
	SelectObject(hdc, GetStockObject(WHITE_PEN));
	GdSetMode(MWMODE_XOR);
	if(!IsRectEmpty(&lastrc))
		Rectangle(hdc, lastrc.left, lastrc.top, lastrc.right,
			lastrc.bottom);
	GetWindowRect(hwnd, &rc);
	SetRect(&lastrc, rc.left+x, rc.top+y, rc.right+x, rc.bottom+y);
	if(bDrawCurrent)
		Rectangle(hdc, lastrc.left, lastrc.top, lastrc.right,
			lastrc.bottom);
	ReleaseDC(NULL, hdc);
	GdSetMode(MWMODE_COPY);
}

/*
 * This procedure implements the messages passed by the window
 * manager for default processing on behalf of the window.
 */
LRESULT WINAPI
DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC		hdc;
	RECT		rc;
	DWORD		dwStyle;
	HBRUSH		hbr;
	HPEN		hpen, holdpen;
	PAINTSTRUCT	ps;
	POINT		curpt;
	int 		x, y;
	HWND		wp;
	HWND		oldActive;
	COLORREF	crCaption;
	LPNCCALCSIZE_PARAMS lpnc;
	CHAR		szTitle[64];
	static POINT 	startpt;

	switch(msg) {
	case WM_NCCALCSIZE:
		/* calculate client rect from passed window rect in rgrc[0]*/
		lpnc = (LPNCCALCSIZE_PARAMS)lParam;
		dwStyle = GetWindowLong(hwnd, GWL_STYLE);
		if(dwStyle & WS_BORDER) {
			if((dwStyle & WS_CAPTION) == WS_CAPTION) {
				InflateRect(&lpnc->rgrc[0],
					-mwSYSMETRICS_CXFRAME,
					-mwSYSMETRICS_CYFRAME);
				lpnc->rgrc[0].top += mwSYSMETRICS_CYCAPTION + 1;
			} else
				InflateRect(&lpnc->rgrc[0], -1, -1);
		}
		break;

	case WM_NCPAINT:
		/* repaint all non-client area*/
		dwStyle = GetWindowLong(hwnd, GWL_STYLE);

		if(dwStyle & WS_BORDER) {
			hdc = GetWindowDC(hwnd);
			GetWindowRect(hwnd, &rc);

			if((dwStyle & WS_CAPTION) == WS_CAPTION) {
				/* draw 2-line 3d border around window*/
				Draw3dOutset(hdc, rc.left, rc.top,
					rc.right-rc.left, rc.bottom-rc.top);
				InflateRect(&rc, -2, -2);

				/* draw 1-line inset inside border*/
				hpen = CreatePen(PS_SOLID, 1,
					GetSysColor(COLOR_BTNFACE));
				holdpen = SelectObject(hdc, hpen);
				SelectObject(hdc, GetStockObject(NULL_BRUSH));
				Rectangle(hdc, rc.left, rc.top, rc.right,
					rc.bottom);
				InflateRect(&rc, -1, -1);

				/* fill caption*/
				rc.bottom = rc.top + mwSYSMETRICS_CYCAPTION;
				crCaption = GetActiveWindow()==hwnd?
					GetSysColor(COLOR_ACTIVECAPTION):
					GetSysColor(COLOR_INACTIVECAPTION);
				hbr = CreateSolidBrush(crCaption);
				FillRect(hdc, &rc, hbr);
				DeleteObject(hbr);

				/* draw 1 line under caption*/
				MoveToEx(hdc, rc.left, rc.bottom, NULL);
				LineTo(hdc, rc.right, rc.bottom);
				DeleteObject(SelectObject(hdc, holdpen));

				/* draw caption text*/
				if(GetWindowText(hwnd, szTitle,
				   sizeof(szTitle))) {
					SetBkMode(hdc, TRANSPARENT);
					/* set background color even though
					 * transparent in case GdArea is used
					 * to draw text which compares
					 * gr_foreground != gr_background
					 * when transparent...
					 */
					SetBkColor(hdc, crCaption);
					SetTextColor(hdc,
						GetActiveWindow()==hwnd?
						GetSysColor(COLOR_CAPTIONTEXT):
						GetSysColor(COLOR_INACTIVECAPTIONTEXT));
					SelectObject(hdc,
					    GetStockObject(DEFAULT_GUI_FONT));
					GetWindowRect(hwnd, &rc);
					TextOut(hdc, rc.left+4, rc.top+2,
						szTitle, strlen(szTitle));
				}

				/* draw close box*/
				GetCloseBoxRect(hwnd, &rc);
				/*DrawDIB(hdc, rc.right-XSIZE_CLOSEBOX-3,
					rc.top+3, &image_close4);*/
				Draw3dBox(hdc, rc.left, rc.top,
					rc.right-rc.left, rc.bottom-rc.top,
					GetSysColor(COLOR_BTNHIGHLIGHT),
					GetSysColor(COLOR_WINDOWFRAME));
				InflateRect(&rc, -1, -1);
				hbr = CreateSolidBrush(
					GetSysColor(COLOR_BTNFACE));
				FillRect(hdc, &rc, hbr);
				DeleteObject(hbr);

				InflateRect(&rc, -1, -1);
				MoveToEx(hdc, rc.left, rc.top, NULL);
				LineTo(hdc, rc.right-1, rc.bottom-1);
				MoveToEx(hdc, rc.left, rc.bottom-1, NULL);
				LineTo(hdc, rc.right-1, rc.top);
			} else {
				SelectObject(hdc, GetStockObject(NULL_BRUSH));
				Rectangle(hdc, rc.left, rc.top, rc.right,
					rc.bottom);
			}
			ReleaseDC(hwnd, hdc);
		}
		break;

	case WM_NCHITTEST:
		/* if system is dragging a window, always return caption*/
		if(dragwp)
			return HTCAPTION;

		/* Determine what part of the window the mouse is over*/
		POINTSTOPOINT(curpt, lParam);

		if(PtInRect(&hwnd->clirect, curpt))
			return HTCLIENT;

		if(PtInRect(&hwnd->vscroll.rc, curpt))
			return HTVSCROLL;
		if(PtInRect(&hwnd->hscroll.rc, curpt))
			return HTHSCROLL; 

		dwStyle = GetWindowLong(hwnd, GWL_STYLE);
		if((dwStyle & WS_CAPTION) == WS_CAPTION) {
			GetCloseBoxRect(hwnd, &rc);
			if(PtInRect(&rc, curpt))
				return HTCLOSE;

			GetWindowRect(hwnd, &rc);
			InflateRect(&rc, -2, -2);
			rc.bottom = rc.top + mwSYSMETRICS_CYCAPTION;
			if(PtInRect(&rc, curpt))
				return HTCAPTION;

			GetWindowRect(hwnd, &rc);
			InflateRect(&rc, -2, -2);
			rc.top += mwSYSMETRICS_CYCAPTION;
			if(PtInRect(&rc, curpt))
				return HTCLIENT;

			return HTBORDER;
		}
		return HTNOWHERE;

	case WM_NCLBUTTONDOWN:
		/* Handle default actions for mouse down on window*/
		if(wParam == HTCLOSE) {
			SendMessage(hwnd, WM_CLOSE, 0, 0L);
			break;
		}

		/* set focus on mouse down, repaint if necessary*/
		oldActive = GetActiveWindow();
		if(wParam == HTCLIENT || wParam == HTVSCROLL ||
		   wParam == HTHSCROLL)
			/* activate and raise window if in client area*/
			/* kaffe port requires this commented out*/
			SetForegroundWindow(hwnd);
		else {
			/* otherwise just change focus window, same z order*/
			/* this will activate it's top level parent*/
			SetFocus(hwnd);
		}
		/* repaint captions now because of activation change*/
		UpdateWindow(oldActive);
		UpdateWindow(hwnd);

		if(wParam == HTVSCROLL || wParam == HTHSCROLL) {
			MwHandleNCMessageScrollbar(hwnd, msg, wParam, lParam);
			break;
		}

		/* start window drag if in caption area*/
		if(wParam == HTCAPTION && hwnd != rootwp) {
			POINTSTOPOINT(startpt, lParam);
			if(!(GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE))
				dragwp = hwnd;
			SetRectEmpty(&lastrc);	/* XORMOVE only*/
		}
		break;

	case WM_NCMOUSEMOVE:
		if(wParam == HTVSCROLL || wParam == HTHSCROLL) {
			MwHandleNCMessageScrollbar(hwnd, msg, wParam, lParam);
			break;
		}

		/* drag window with mousemove after mousedown*/
		if(dragwp == hwnd) {
			POINTSTOPOINT(curpt, lParam);
			x = curpt.x - startpt.x;
			y = curpt.y - startpt.y;

			if(mwERASEMOVE) {
				GetWindowRect(hwnd, &rc);
				MoveWindow(hwnd, rc.left+x, rc.top+y,
					rc.right-rc.left, rc.bottom-rc.top,
					TRUE);
				startpt = curpt;
			} else
				DrawXORFrame(hwnd, x, y, TRUE);
		}
		break;

	case WM_NCLBUTTONUP:
		/* stop window drag*/
		if(dragwp == hwnd) {
			dragwp = NULL;

			if(mwERASEMOVE) {
				/*
				 * User stopped moving window, repaint 
				 * windows previously queued for painting.
				 */
				for(wp=listwp; wp; wp=wp->next)
					if(wp->gotPaintMsg == PAINT_DELAYPAINT)
					    wp->gotPaintMsg = PAINT_NEEDSPAINT;
			} else {
				POINTSTOPOINT(curpt, lParam);
				x = curpt.x - startpt.x;
				y = curpt.y - startpt.y;
				DrawXORFrame(hwnd, x, y, FALSE);
				GetWindowRect(hwnd, &rc);
				MoveWindow(hwnd, rc.left+x, rc.top+y,
				    rc.right-rc.left, rc.bottom-rc.top, TRUE);
			}
		}

		if(wParam == HTVSCROLL || wParam == HTHSCROLL) {
			MwHandleNCMessageScrollbar(hwnd, msg, wParam, lParam);
			break;
		}
		break;

	case WM_NCLBUTTONDBLCLK:
		if(wParam == HTVSCROLL || wParam == HTHSCROLL) {
			MwHandleNCMessageScrollbar(hwnd, msg, wParam, lParam);
			break;
		}

		/* maximize/restore processing*/
		if(wParam != HTCAPTION)
			break;

		if((hwnd->style & WS_CAPTION) == WS_CAPTION) {
			if(hwnd->style & WS_MAXIMIZE) {
				rc = hwnd->restorerc;
				MoveWindow(hwnd, rc.left, rc.top,
					rc.right-rc.left, rc.bottom-rc.top,
					TRUE);
				hwnd->style &= ~WS_MAXIMIZE;
			} else {
				hwnd->restorerc = hwnd->winrect;
				GetWindowRect(rootwp, &rc);
				MoveWindow(hwnd, -mwSYSMETRICS_CXFRAME,
					-mwSYSMETRICS_CYFRAME,
					rc.right+2*mwSYSMETRICS_CXFRAME,
					rc.bottom+2*mwSYSMETRICS_CYFRAME, TRUE);
				hwnd->style |= WS_MAXIMIZE;
			}
		}
		break;

	case WM_GETTEXTLENGTH:
		/* Get window text length.  This routine requires
		 * knowledge of the internal window structure
		 */
		return strlen(hwnd->szTitle);

	case WM_GETTEXT:
		/* Get window text.  This routine requires
		 * knowledge of the internal window structure
		 */
		return strzcpy((LPSTR)lParam, hwnd->szTitle, wParam);

	case WM_SETTEXT:
		/* Set window text.  This routine requires
		 * knowledge of the internal window structure.
		 * Note that setting text doesn't invalidate the window.
		 */
		strzcpy(hwnd->szTitle, (LPSTR)lParam, sizeof(hwnd->szTitle));
		return TRUE;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		if(hwnd == rootwp)
			PostQuitMessage(0);
		break;

	case WM_ERASEBKGND:
		/* erase background with class background brush*/
		hbr = (HBRUSH)GetClassLong(hwnd, GCL_HBRBACKGROUND);
		if(!hbr)
			return 0;
		/* don't exclude update region*/
		hdc = GetDCEx(hwnd, NULL, DCX_DEFAULTCLIP);
		FillRect(hdc, NULL, hbr);
		ReleaseDC(hwnd, hdc);
		return 1;

	case WM_PAINT:
		/* required to send erasebkgnd for desktop window*/
		hdc = BeginPaint(hwnd, &ps);

		/* draw desktop wallpaper*/
		if(hwnd == rootwp && pImageWallpaper) {
			GetWindowRect(hwnd, &rc);
			DrawDIB(hdc,
				(rc.right-rc.left-pImageWallpaper->width)/2,
				(rc.bottom-rc.top-pImageWallpaper->height)/2,
				pImageWallpaper);
		}

		EndPaint(hwnd, &ps);
		break;
	}
	return 0;
}

static void
GetCloseBoxRect(HWND hwnd, LPRECT lprc)
{
#define XSIZE_CLOSEBOX	9
#define YSIZE_CLOSEBOX	9
	GetWindowRect(hwnd, lprc);
	lprc->left = lprc->right - XSIZE_CLOSEBOX - 5;
	lprc->top = lprc->top + 5;
	lprc->right = lprc->left + XSIZE_CLOSEBOX;
	lprc->bottom = lprc->top + YSIZE_CLOSEBOX;
}
