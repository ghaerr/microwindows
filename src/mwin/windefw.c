/*
 * Copyright (c) 1999, 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * DefWindowProc implementation for Micro-Windows
 *	This file should ideally only include windows.h, and be built
 *	on top of regular win32 api calls.  For speed, however,
 *	certain knowledge of the internal hwnd is known...
 */
#define MWINCLUDECOLORS
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include "wintools.h"		/* Draw3dBox, etc*/
#include <stdlib.h>
#include <string.h>

/* desktop wallpaper image*/
static PMWIMAGEHDR	pImageWallpaper = NULL;

/* local routines*/
static void	GetCloseBoxRect(HWND hwnd, LPRECT lprc);
static void	DrawXORFrame(HWND hwnd,int x, int y, BOOL bDrawCurrent);
static RECT	lastrc;

BOOL WINAPI
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
	GdSetMode(MWROP_XOR);
	if(!IsRectEmpty(&lastrc))
		Rectangle(hdc, lastrc.left, lastrc.top, lastrc.right,
			lastrc.bottom);
	GetWindowRect(hwnd, &rc);
	SetRect(&lastrc, rc.left+x, rc.top+y, rc.right+x, rc.bottom+y);
	if(bDrawCurrent)
		Rectangle(hdc, lastrc.left, lastrc.top, lastrc.right,
			lastrc.bottom);
	ReleaseDC(NULL, hdc);
	GdSetMode(MWROP_COPY);
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

	if (!IsWindow(hwnd))
		return 0;

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
				Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
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
				if(GetWindowText(hwnd, szTitle, sizeof(szTitle))) {
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
					SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
					GetWindowRect(hwnd, &rc);
					TextOut(hdc, rc.left+4, rc.top+2, szTitle, strlen(szTitle));
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
				Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
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
		/* Disabled windows don't process this message. */
		if( !IsWindowEnabled(hwnd) )
			break;
			
		/* Handle default actions for mouse down on window*/
		if(wParam == HTCLOSE) {
			SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
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
				MoveWindow(hwnd, rc.left+x, rc.top+y, rc.right-rc.left, rc.bottom-rc.top, TRUE);
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

		if((hwnd->style & WS_CAPTION) && (hwnd->style & WS_MAXIMIZEBOX)) {
			SendMessage (hwnd, WM_SYSCOMMAND,
				((hwnd->style & (WS_MINIMIZE | WS_MAXIMIZE))? SC_RESTORE: SC_MAXIMIZE), 0);
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
		{
		/* Set window text.  This routine requires
		 * knowledge of the internal window structure.
		 * Note that setting text doesn't invalidate the window.
		 */
		LPTSTR newTit = (LPTSTR) malloc ( 1+strlen((LPCSTR)lParam) );
		if (newTit == NULL)
			return FALSE;
		free (hwnd->szTitle);
		hwnd->szTitle = newTit;
		strcpy(hwnd->szTitle, (LPSTR)lParam);
		return TRUE;
		}

	case WM_CLOSE:
		DestroyWindow(hwnd);
		if(hwnd == rootwp)
			PostQuitMessage(0);
		break;

	case WM_ERASEBKGND:
		/* erase background with class background brush*/
		hbr = (HBRUSH)(LONG_PTR)GetClassLongPtr(hwnd, GCL_HBRBACKGROUND);
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

	case WM_SYSCOMMAND:
		switch (wParam & 0xfff0) {
		case SC_MINIMIZE:
			ShowWindow (hwnd, SW_MINIMIZE);
	        break;
		case SC_MAXIMIZE:
			ShowWindow (hwnd, SW_MAXIMIZE);
	        break;
		case SC_RESTORE:
			ShowWindow (hwnd, SW_RESTORE);
	        break;
		case SC_CLOSE:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_VKEYTOITEM:
	case WM_CHARTOITEM:
		return -1;
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLOR:
		return 0;

	case WM_CTLCOLORSTATIC:
		{
		HWND hCtl = (HWND) lParam;
		hdc = (HDC)wParam;
		dwStyle = hCtl->style;
		if (dwStyle & WS_DISABLED)
			SetTextColor (hdc, DKGRAY);
		else
			SetTextColor (hdc, BLACK);
		SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
		SetBkMode(hdc, TRANSPARENT);
		SelectObject(hdc, (HFONT)(LRESULT)SendMessage(hCtl, WM_GETFONT, (WPARAM)0, (LPARAM)0));
		SelectObject(hdc, GetStockObject(BLACK_PEN) );

		switch ( dwStyle & SS_ETCTYPEMASK )
			{
			/*  FIXME: Frames should use NULL_BRUSH, but with current
			    microwindows version does not works... */
			case SS_BLACKFRAME:
				hCtl->paintBrush = CreateSolidBrush ( GetSysColor(COLOR_BTNFACE) );
				return (LPARAM)hCtl->paintBrush;

			case SS_GRAYFRAME:
			case SS_ETCHEDFRAME:
				hCtl->paintPen = CreatePen ( PS_SOLID, 1, GRAY );
				SelectObject ( hdc, hCtl->paintPen );
				hCtl->paintBrush = CreateSolidBrush ( GetSysColor(COLOR_BTNFACE) );
				return (LPARAM)hCtl->paintBrush;

			case SS_WHITEFRAME:
				SelectObject ( hdc, GetStockObject(WHITE_PEN) );
				hCtl->paintBrush = CreateSolidBrush ( GetSysColor(COLOR_BTNFACE) );
				return (LPARAM)hCtl->paintBrush;

			case SS_GRAYRECT:
				return (LPARAM)(HGDIOBJ)GetStockObject(DKGRAY_BRUSH);

			case SS_BLACKRECT:
				return (LPARAM)(HGDIOBJ)GetStockObject(BLACK_BRUSH);

			case SS_WHITERECT:
				return (LPARAM)(HGDIOBJ)GetStockObject(WHITE_BRUSH);

			case SS_SIMPLE:
				hCtl->paintBrush = CreateSolidBrush(((MWBRUSHOBJ *)hCtl->pClass->hbrBackground)->color);
				return (LPARAM)hCtl->paintBrush;

			default:
				hCtl->paintBrush = CreateSolidBrush ( GetSysColor(COLOR_BTNFACE) );
				return (LPARAM)hCtl->paintBrush;
			}
		}
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
