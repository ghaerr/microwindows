/*
 * Copyright (C) 1999, 2000, Wei Yongming.
 * Portions Copyright (c) 2000 Greg Haerr <greg@censoft.com>
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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "windows.h"	/* windef.h, winuser.h */
#include "wintools.h"
#include "device.h" 	/* GdGetTextSize */

/* jmt: should be SYSTEM_FIXED_FONT because of minigui's GetSysCharXXX() */
#define FONT_NAME	SYSTEM_FIXED_FONT	/* was DEFAULT_GUI_FONT*/

static LRESULT CALLBACK
StaticControlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI MwRegisterStaticControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)StaticControlProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "STATIC";

	return RegisterClass(&wc);
}

#if 1
#define RECTW(rc) (rc.right-rc.left)
#define RECTH(rc) (rc.bottom-rc.top)

static DWORD GetWindowStyle (HWND hwnd)
{
	return hwnd->style;
}

static COLORREF GetWindowBkColor (HWND hwnd)
{
	MWBRUSHOBJ *hbr;
	hbr=(MWBRUSHOBJ *)hwnd->pClass->hbrBackground;
	return hbr->color;
}

static char *GetWindowCaption (HWND hwnd)
{
	return hwnd->szTitle;
}

static void SetWindowCaption (HWND hwnd,char *caption)
{
	if (strlen(caption)<=63)	/* mw: szTitle[64] */
		strcpy(hwnd->szTitle,caption);
	else
	{
		strncpy(hwnd->szTitle,caption,63);
		hwnd->szTitle[63]='\0';
	}
}

static int GetSysCharHeight (HWND hwnd) 
{
	HDC 		hdc;
    	int xw, xh, xb;

    	hdc = GetDC(hwnd);

	SelectObject(hdc, GetStockObject(FONT_NAME));

#if MWCLIENT	/* nanox client */
    	GrGetGCTextSize(hdc->gc, "X", 1, MWTF_ASCII, &xw, &xh, &xb);
#else
    	GdGetTextSize(hdc->font->pfont,"X",1, &xw,&xh,&xb,MWTF_ASCII);
#endif
    	ReleaseDC(hwnd,hdc);

	return xh;
}

static int GetSysCharWidth (HWND hwnd) 
{
	HDC 		hdc;
    	int xw, xh, xb;

    	hdc = GetDC(hwnd);

	SelectObject(hdc, GetStockObject(FONT_NAME));

#if MWCLIENT	/* nanox client */
    	GrGetGCTextSize(hdc->gc, "X", 1, MWTF_ASCII, &xw, &xh, &xb);
#else
    	GdGetTextSize(hdc->font->pfont,"X",1, &xw,&xh,&xb,MWTF_ASCII);
#endif
    	ReleaseDC(hwnd,hdc);

	return xw;
}
#endif

static LRESULT CALLBACK
StaticControlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT        rcClient;
    HDC         hdc;
    char*       spCaption;
    HWND    	pCtrl;
    UINT        uFormat;
    DWORD       dwStyle;
    
    pCtrl = hwnd;                        
    switch (message) {
        case WM_CREATE:
            return 0;
            
        case WM_DESTROY: 
            break;

        case STM_GETIMAGE:
            return (int)(pCtrl->userdata); 
        
        case STM_SETIMAGE:
        {
            int pOldValue;
            
            pOldValue  = (int)(pCtrl->userdata);
            pCtrl->userdata = (DWORD)wParam;
            InvalidateRect (hwnd, NULL, FALSE);
            return pOldValue;
        }
           
        case WM_GETDLGCODE:
            return DLGC_STATIC;


        case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    RECT rc;
	    HBRUSH hbr;

            hdc = BeginPaint (hwnd,&ps);

            GetClientRect (hwnd, &rcClient);

	    FastFillRect(hdc, &rcClient, GetSysColor(COLOR_BTNFACE));

            dwStyle = GetWindowStyle (hwnd);

            switch (dwStyle & SS_TYPEMASK)
            {
                case SS_GRAYRECT:
#if 0
                    SetBrushColor (hdc, LTGRAY);
                    FillBox(hdc, 0, 0, RECTW(rcClient), RECTH(rcClient));
#else
		    rc.left=0; rc.top=0; rc.bottom=RECTH(rcClient); rc.right=RECTW(rcClient);
		    FillRect(hdc,&rc,GetStockObject(LTGRAY_BRUSH));
#endif
                break;
                
                case SS_GRAYFRAME:
#if 0
                    Draw3DDownFrame (hdc, 
                            0, 0, rcClient.right, rcClient.bottom, 
                            DKGRAY);
#else
		    Draw3dInset(hdc, 0, 0,
			rcClient.right, rcClient.bottom);
#endif
                break;
                
                case SS_BITMAP:
#if 0	/* jmt: fix: no FillBoxWithBitmap() */
                    FillBoxWithBitmap(hdc, 0, 0, 0, 0,
                        (PBITMAP)(pCtrl->userdata));
#endif
                break;
                
                case SS_ICON:
#if 0	/* jmt: fix: no DrawIcon */
                    hIcon = (HICON)(pCtrl->userdata);
                    DrawIcon (hdc, 0, 0, 0, 0, hIcon);
#endif
                break;
      
                case SS_SIMPLE:
#if 0
                    SetBrushColor (hdc, GetWindowBkColor (hwnd));
                    FillBox (hdc, 0, 0, rcClient.right, rcClient.bottom);
#else
		    hbr=CreateSolidBrush(GetWindowBkColor(hwnd));
		    rc.left=0; rc.top=0; rc.bottom=rcClient.bottom; rc.right=rcClient.right;
		    FillRect(hdc,&rc,hbr);
		    DeleteObject(hbr);
#endif        
                    if (dwStyle & WS_DISABLED)
                        SetTextColor (hdc, DKGRAY);
                    else
                        SetTextColor (hdc, BLACK);
		    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
                    spCaption = GetWindowCaption (hwnd);
                    if (spCaption)
		    {
	    		SelectObject(hdc, GetStockObject(FONT_NAME));
                        TextOut (hdc, 0, 0, spCaption, -1); 
		    }
                break; 

                case SS_LEFT:
                case SS_CENTER:
                case SS_RIGHT:
                case SS_LEFTNOWORDWRAP:
                    uFormat = DT_TOP;
                    if ( (dwStyle & SS_TYPEMASK) == SS_LEFT)
                        uFormat |= DT_LEFT | DT_WORDBREAK;
                    else if ( (dwStyle & SS_TYPEMASK) == SS_CENTER)
                        uFormat |= DT_CENTER | DT_WORDBREAK;
                    else if ( (dwStyle & SS_TYPEMASK) == SS_RIGHT)
                        uFormat |= DT_RIGHT | DT_WORDBREAK;
                    else if ( (dwStyle & SS_TYPEMASK) == SS_LEFTNOWORDWRAP)
                        uFormat |= DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS;
                    
                    if (dwStyle & WS_DISABLED)
                        SetTextColor (hdc, DKGRAY);
                    else
                        SetTextColor (hdc, BLACK);

#if 0
                    SetBkColor (hdc, GetWindowBkColor (hwnd));
#endif
		    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
                    spCaption = GetWindowCaption (hwnd);
                    if (dwStyle & SS_NOPREFIX)
                        uFormat |= DT_NOPREFIX;
                        
                    if (spCaption)
		    {
	    		SelectObject(hdc, GetStockObject(FONT_NAME));
                        DrawText (hdc, spCaption, -1, &rcClient, uFormat);
		    }
                break;

                case SS_GROUPBOX:
#if 0
                    Draw3DBorder (hdc,  rcClient.left, 
			    rcClient.top + (GetSysCharHeight(hwnd) >> 1),
			    rcClient.right, rcClient.bottom);
#else
		    Draw3dInset(hdc, rcClient.left, 
			rcClient.top+(GetSysCharHeight(hwnd) >> 1),
			rcClient.right-rcClient.left,
			rcClient.bottom-rcClient.top);
#endif                    
                    if (dwStyle & WS_DISABLED)
                        SetTextColor (hdc, DKGRAY);
                    else
                        SetTextColor (hdc, BLACK);

#if 0
                    SetBkColor(hdc, GetWindowBkColor (GetParent (hwnd)));
#endif
		    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
                    spCaption = GetWindowCaption (hwnd);
                    if (spCaption)
		    {
	    		SelectObject(hdc, GetStockObject(FONT_NAME));
                        TextOut (hdc, GetSysCharWidth (hwnd), 2, spCaption, -1);
		    }
                break;
            }
	    EndPaint (hwnd, &ps);
	}
            break;

#if 0	/* jmt: SS_NOTIFY isn't standard in win32 */
        case WM_LBUTTONDBLCLK:
            if (GetWindowStyle (hwnd) & SS_NOTIFY)
                SendMessage (GetParent(hwnd), WM_COMMAND, 
                    (WPARAM)MAKELONG(pCtrl->id, STN_DBLCLK),
                    (LPARAM)hwnd);   
            break;
#endif
        case WM_LBUTTONDOWN:
            break;

        case WM_NCLBUTTONDBLCLK:
            break;

        case WM_NCLBUTTONDOWN:
            break;

	case WM_NCHITTEST:
            dwStyle = GetWindowStyle (hwnd);
            if ((dwStyle & SS_TYPEMASK) == SS_GROUPBOX)
                return HTTRANSPARENT;

#if 0	/* jmt: SS_NOTIFY isn't standard in win32 */
            if (GetWindowStyle (hwnd) & SS_NOTIFY)
                return HTCLIENT;
            else
#endif
                return HTNOWHERE;
        break;


#if 0	/* jmt: fix: no WM_GETFONT/WM_SETFONT */
        case WM_GETFONT:
            break;
        case WM_SETFONT:
            break;
#endif
        case WM_SETTEXT:
            SetWindowCaption (hwnd, (char*)lParam);
            InvalidateRect (hwnd, NULL, TRUE);
            break;
            
        default:
    		return DefWindowProc (hwnd, message, wParam, lParam);
    }
    return 0;
}
