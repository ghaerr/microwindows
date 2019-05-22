/*
 * Copyright (C) 1999, 2000, Wei Yongming.
 * Portions Copyright (c) 2000, 2005, 2010, 2019 Greg Haerr <greg@censoft.com>
 *
 * Static control for Microwindows win32 api.
 */

/*
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Library General Public
**  License as published by the Free Software Foundation; either
**  version 2 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Library General Public License for more details.
**
**  You should have received a copy of the GNU Library General Public
**  License along with this library; if not, write to the Free
**  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
**  MA 02111-1307, USA
*/

/*
**  Alternatively, the contents of this file may be used under the terms 
**  of the Mozilla Public License (the "MPL License") in which case the
**  provisions of the MPL License are applicable instead of those above.
*/

/* Create date: 1999/5/22
**
** Modify records:
**
**  Who             When        Where       For What                Status
**-----------------------------------------------------------------------------
**  WEI Yongming    1999/8/21   Tsinghua    Rearrangment            Finished
**  WEI Yongming    1999/10/27  Tsinghua    SETTEXT bug             Finished
**  WEI Yongming    1999/10/27  Tsinghua    SETTEXT bug             Finished
**  WEI Yongming    2000/02/24  Tsinghua    Add MPL License         Finished
**  Kevin Tseng     2000/06/26  gv          port to microwin        ported
**  Greg Haerr      2000/07/05  Utah        bug fixes               Finished
** Gabriele Brugnoni 2003/08/30 Italy      WM_SETFONT implementation Finished
** Gabriele Brugnoni 2003/09/10 Italy      Style SS_BITMAP loads bmp from resources.
** Gabriele Brugnoni 2003/09/10 Italy      Static text now support '\n' and word wrap
** Gabriele Brugnoni 2003/09/10 Italy      Implemented WM_CTLCOLORSTATIC
** Gabriele Brugnoni 2004/12/27 Italy      Implemented UTF8 and internationalizations.
*/

#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "windows.h"		/* windef.h, winuser.h */
#include "wintern.h"
#include "wintools.h"
#include "device.h"		/* GdGetTextSize */
#include "uni_std.h"
#include "intl.h"

/* jmt: should be SYSTEM_FIXED_FONT because of minigui's GetSysCharXXX() */
#define FONT_NAME	SYSTEM_FIXED_FONT	/* was DEFAULT_GUI_FONT */

static LRESULT CALLBACK
StaticControlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#define GET_WND_FONT(hWnd)	((HFONT)(LONG_PTR)GetWindowLongPtr(hWnd, 0))
#define SET_WND_FONT(hWnd, fnt)	SetWindowLongPtr(hWnd, 0, (LONG_PTR)fnt)

int
MwRegisterStaticControl(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc = (WNDPROC) StaticControlProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(LONG_PTR);	// allow space for storing font information
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = 0;		/*LoadCursor(NULL, IDC_ARROW); */
	/* FIXME: should be NULL_BRUSH, but does not works with current mw ver. */
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "STATIC";

	return RegisterClass(&wc);
}

#if 1
#define RECTW(rc) (rc.right-rc.left)
#define RECTH(rc) (rc.bottom-rc.top)

static DWORD
GetWindowStyle(HWND hwnd)
{
	return hwnd->style;
}

//static COLORREF
//GetWindowBkColor(HWND hwnd)
//{
//	MWBRUSHOBJ *hbr;
//	hbr = (MWBRUSHOBJ *) hwnd->pClass->hbrBackground;
//	return hbr->color;
//}

static char *
GetWindowCaption(HWND hwnd)
{
	return hwnd->szTitle;
}


static int
GetSysCharHeight(HWND hwnd)
{
	HDC hdc;
	int xw, xh, xb;

	hdc = GetDC(hwnd);

	SelectObject(hdc, GET_WND_FONT(hwnd));

#if MWCLIENT			/* nanox client */
	GrGetGCTextSize(hdc->gc, "X", 1, MWTF_ASCII, &xw, &xh, &xb);
#else
	GdGetTextSize(hdc->font->pfont, "X", 1, &xw, &xh, &xb, MWTF_ASCII);
#endif
	ReleaseDC(hwnd, hdc);

	return xh;
}

static int
GetSysCharWidth(HWND hwnd)
{
	HDC hdc;
	int xw, xh, xb;

	hdc = GetDC(hwnd);

	SelectObject(hdc, GET_WND_FONT(hwnd));

#if MWCLIENT			/* nanox client */
	GrGetGCTextSize(hdc->gc, "X", 1, MWTF_ASCII, &xw, &xh, &xb);
#else
	GdGetTextSize(hdc->font->pfont, "X", 1, &xw, &xh, &xb, MWTF_ASCII);
#endif
	ReleaseDC(hwnd, hdc);

	return xw;
}
#endif

static void
ssDrawStaticLabel(HWND hwnd, HDC hdc, LPRECT pRcClient)
{
	LPTSTR spCaption;
	UINT uFormat;
	RECT rc;
#if OLD_DRAWTEXT
	int y, maxy;
	SIZE sz;
#endif
	DWORD dwStyle = hwnd->style;
	unsigned long attrib = 0;

	rc = *pRcClient;
#if OLD_DRAWTEXT
	maxy = rc.bottom - rc.top;
	y = rc.top;
#endif

	uFormat = DT_TOP;
	if ((dwStyle & SS_TYPEMASK) == SS_LEFT)
		uFormat |= DT_LEFT | DT_WORDBREAK;
	else if ((dwStyle & SS_TYPEMASK) == SS_CENTER)
		uFormat |= DT_CENTER | DT_WORDBREAK;
	else if ((dwStyle & SS_TYPEMASK) == SS_RIGHT)
		uFormat |= DT_RIGHT | DT_WORDBREAK;
	else if ((dwStyle & SS_TYPEMASK) == SS_LEFTNOWORDWRAP)
		uFormat |= DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS;

	if (dwStyle & SS_CENTERIMAGE)
		uFormat |= DT_VCENTER | DT_WORDBREAK;


	spCaption = GetWindowCaption(hwnd);
	if (dwStyle & SS_NOPREFIX)
		uFormat |= DT_NOPREFIX;

	if (spCaption) {
		LPTSTR caption;
		int n, ln = strlen(spCaption);

		//  Duplicate text. If coding is UTF-8, generati it by checking shape/joining
		if (mwTextCoding == MWTF_UTF8)
			caption = doCharShape_UTF8(spCaption, ln, &ln, &attrib);
		else
			caption = strdup(spCaption);

		if (caption == NULL)
			return;

		spCaption = caption;
		SelectObject(hdc, GET_WND_FONT(hwnd));

#if OLD_DRAWTEXT
		while (ln > 0) {
			int n, nCount;
			for (n = 0; n < ln; n += MW_CHRNBYTE(spCaption[n]))
				if (spCaption[n] == '\n')
					break;
			nCount = n;

			attrib &= ~TEXTIP_RTOL;	// resets RTOL text prop

			GetTextExtentPoint(hdc, spCaption, n, &sz);
			while (sz.cx > (rc.right - rc.left)) {
				while ((n > 0) && (spCaption[n] == ' '
					   || spCaption[n] == '\t'))
					n--;
				while ((n > 0) && !(spCaption[n] == ' '
					    || spCaption[n] == '\t'))
					n--;
				if (n <= 0)
					break;
				GetTextExtentPoint(hdc, spCaption, n, &sz);
			}

			// In case we only have one word, approximate
			if (n<=0) {
				for (n = nCount; n>0 && sz.cx > (rc.right - rc.left); n--)
					GetTextExtentPoint(hdc, spCaption, n, &sz);
			}

			rc.top = y;
			rc.bottom = y + sz.cy;
#else
			n=ln;
#endif
#if MW_FEATURE_INTL
			//  If the UTF8 text has non-ascii characters, check also bidi
			if (attrib & TEXTIP_EXTENDED) {
				DWORD vuFormat = uFormat;
				LPSTR virtCaption =
					doCharBidi_UTF8(spCaption, n, NULL, NULL, &attrib);
				if (virtCaption) {
					if ((attrib & TEXTIP_RTOL)) {
						if ((vuFormat & (DT_LEFT | DT_CENTER | DT_RIGHT)) == DT_LEFT)
							vuFormat = (vuFormat &
								 ~(DT_LEFT | DT_CENTER | DT_RIGHT))
								| DT_RIGHT;
					}
					DrawTextA(hdc, virtCaption, n, &rc, vuFormat);
					free(virtCaption);
				}
			} else
#endif
				DrawTextA(hdc, spCaption, n, &rc, uFormat);
#if OLD_DRAWTEXT
			y += sz.cy;
			if (y > maxy)
				break;
			spCaption += n + 1;
			ln -= n + 1;
		}
#endif
		free(caption);
	}
}

/*
 *  Show DIB for SS_BITMAP style...
 */
static void
ssShowBitmap(HWND hwnd, HDC hdc)
{
	PMWIMAGEHDR himg;
	LPCTSTR resName = hwnd->szTitle;

	if (resName[0] == '\xff')
		resName = MAKEINTRESOURCE((UCHAR) resName[1]);
	himg = resLoadBitmap(hwnd->hInstance, resName);
	if (himg != NULL) {
		DrawDIB(hdc, 0, 0, himg);
		resFreeBitmap(himg);
	}
}

static LRESULT CALLBACK
StaticControlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rcClient;
	HDC hdc;
	char *spCaption;
	HWND pCtrl;
	DWORD dwStyle;

	pCtrl = hwnd;
	switch (message) {
	case STM_GETIMAGE:
		return (LRESULT)pCtrl->userdata;

	case STM_SETIMAGE:
		{
			LRESULT pOldValue = (LRESULT)pCtrl->userdata;
			pCtrl->userdata = (ULONG_PTR)lParam;
			InvalidateRect(hwnd, NULL, FALSE);
			return pOldValue;
		}

	case WM_GETDLGCODE:
		return DLGC_STATIC;

	case WM_ERASEBKGND:
		{
			HBRUSH hbr;
			dwStyle = GetWindowStyle(hwnd);
			hbr = (HBRUSH)(LRESULT)SendMessage(GetParent(hwnd), WM_CTLCOLORSTATIC, wParam, (LPARAM)hwnd);
			if (hbr == NULL)
				return DefWindowProc(hwnd, message, wParam, lParam);
			GetClientRect(hwnd, &rcClient);
			FillRect((HDC) wParam, &rcClient, hbr);
			return 1;
		}

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
			HBRUSH hbr;

			hdc = BeginPaint(hwnd, &ps);
			/* Color, pen and brush are now choosed by defwindowproc, or by user. */
			hbr = (HBRUSH)(LRESULT)SendMessage(GetParent(hwnd), WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd);
			if (hbr == NULL)
				hbr = (HBRUSH)GetStockObject(NULL_BRUSH);

			SelectObject(hdc, hbr);

			GetClientRect(hwnd, &rcClient);
			dwStyle = GetWindowStyle(hwnd);

			switch (dwStyle & SS_ETCTYPEMASK) {
			case SS_GRAYRECT:
			case SS_BLACKRECT:
			case SS_WHITERECT:
				FillRect(hdc, &rcClient, hbr);
				break;

			case SS_GRAYFRAME:
			case SS_BLACKFRAME:
			case SS_WHITEFRAME:
				Rectangle(hdc, rcClient.left, rcClient.top,
					  rcClient.right, rcClient.bottom);
				break;

			case SS_BITMAP:
				ssShowBitmap(hwnd, hdc);
#if 0				/* jmt: fix: no FillBoxWithBitmap() */
				FillBoxWithBitmap(hdc, 0, 0, 0, 0, (PBITMAP) (pCtrl->userdata));
#endif
				break;

			case SS_ICON:
#if 0				/* jmt: fix: no DrawIcon */
				hIcon = (HICON) (pCtrl->userdata);
				DrawIcon(hdc, 0, 0, 0, 0, hIcon);
#endif
				FillRect(hdc, &rcClient, hbr);
				break;

			case SS_SIMPLE:
#if 0
				SetBrushColor(hdc, GetWindowBkColor(hwnd));
				FillBox(hdc, 0, 0, rcClient.right, rcClient.bottom);
#else
				rc.left = 0;
				rc.top = 0;
				rc.bottom = rcClient.bottom;
				rc.right = rcClient.right;
				FillRect(hdc, &rc, hbr);
#endif
				spCaption = GetWindowCaption(hwnd);
				if (spCaption)
					TextOut(hdc, 0, 0, spCaption, -1);
				break;

			case SS_LEFT:
			case SS_CENTER:
			case SS_RIGHT:
			case SS_LEFTNOWORDWRAP:
				rc = rcClient;
				if (dwStyle & SS_SUNKEN)
					rc.left += 1, rc.top += 1, rc.bottom -= 1, rc.right -= 1;
				if (dwStyle & WS_BORDER)
					rc.left += 1, rc.top += 1, rc.bottom -= 1, rc.right -= 1;
				FillRect(hdc, &rc, hbr);
				ssDrawStaticLabel(hwnd, hdc, &rc);
				break;

			case SS_ETCHEDFRAME:
				Draw3dBox(hdc, rcClient.left, rcClient.top,
					  rcClient.right - rcClient.left,
					  rcClient.bottom - rcClient.top,
					  GetSysColor(COLOR_BTNSHADOW),
					  GetSysColor(COLOR_BTNHIGHLIGHT));
				Draw3dBox(hdc, rcClient.left + 1, rcClient.top + 1,
					  rcClient.right - rcClient.left - 2,
					  rcClient.bottom - rcClient.top - 2,
					  GetSysColor(COLOR_BTNHIGHLIGHT),
					  GetSysColor(COLOR_BTNSHADOW));
				break;

			case SS_GROUPBOX:
#if 0
				Draw3DBorder(hdc, rcClient.left, rcClient.top +
					     (GetSysCharHeight(hwnd) >> 1),
					     rcClient.right, rcClient.bottom);
#else
				Draw3dInset(hdc, rcClient.left, rcClient.top +
					    (GetSysCharHeight(hwnd) >> 1),
					    rcClient.right - rcClient.left,
					    rcClient.bottom - rcClient.top);
#endif

				spCaption = GetWindowCaption(hwnd);
				if (spCaption)
					TextOut(hdc, GetSysCharWidth(hwnd), 2, spCaption, -1);
				break;
			}

			if (dwStyle & SS_SUNKEN)
				Draw3dBox(hdc, rcClient.left, rcClient.top,
					  rcClient.right - rcClient.left,
					  rcClient.bottom - rcClient.top,
					  GetSysColor(COLOR_BTNSHADOW),
					  GetSysColor(COLOR_BTNHIGHLIGHT));

			EndPaint(hwnd, &ps);
		}
		break;

#if 0				/* jmt: SS_NOTIFY isn't standard in win32 */
	case WM_LBUTTONDBLCLK:
		if (GetWindowStyle(hwnd) & SS_NOTIFY)
			SendMessage(GetParent(hwnd), WM_COMMAND,
				    (WPARAM) MAKELONG(pCtrl->id, STN_DBLCLK), (LPARAM) hwnd);
		break;
#endif
	case WM_LBUTTONDOWN:
		break;

	case WM_NCLBUTTONDBLCLK:
		break;

	case WM_NCLBUTTONDOWN:
		break;

	case WM_NCHITTEST:
		dwStyle = GetWindowStyle(hwnd);
		if ((dwStyle & SS_ETCTYPEMASK) == SS_GROUPBOX)
			return HTTRANSPARENT;

#if 0				/* jmt: SS_NOTIFY isn't standard in win32 */
		if (GetWindowStyle(hwnd) & SS_NOTIFY)
			return HTCLIENT;
		else
#endif
			return HTNOWHERE;
		break;


	case WM_SETTEXT:
		DefWindowProc(hwnd, message, wParam, lParam);
		InvalidateRect(hwnd, NULL, TRUE);
		break;


	case WM_SETFONT:
		SET_WND_FONT(hwnd, (HFONT) wParam);
		if (LOWORD(lParam) != 0)
			InvalidateRect(hwnd, NULL, TRUE);
		break;

	case WM_GETFONT:
		return (LRESULT)GET_WND_FONT(hwnd);

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
