/*
 * Copyright (C) 1999, 2000, Wei Yongming.
 * Portions Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Progress bar for Microwindows win32 api.
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

/* Copyright (C) 1999, 2000, Wei Yongming.
**
** Note:
**   Originally by Zhao Jianghua. 
**
** Create date: 1999/8/29
**
** Modify records:
**
**  Who             When        Where       For What                Status
**-----------------------------------------------------------------------------
**  WEI Yongming    1999/10/27  Tsinghua    unsigned int            Finished
**  WEI Yongming    1999/10/27  Tsinghua    FPException fixing      Finished
**  WEI Yongming    2000/02/24  Tsinghua    Add MPL License         Finished
**  Kevin Tseng     2000/05/24  gv          port to microwin        ported
**  Greg Haerr      2000/06/15  Utah        removed floats          Finished
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "windows.h"	/* windef.h, winuser.h */
#include "wintools.h" 	/* Draw3dBox */
#include "device.h" 	/* GdGetTextSize */

#define TEST	1	/* =1 for testing*/

#define  WIDTH_PBAR_BORDER  2 

typedef  struct _PROGRESSDATA {
    unsigned int nMin;
    unsigned int nMax;
    unsigned int nPos;
    unsigned int nStepInc;
} PROGRESSDATA, *PPROGRESSDATA;

static LRESULT CALLBACK
ProgressBarCtrlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI MwRegisterProgressBarControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)ProgressBarCtrlProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "PROGBAR";

	return RegisterClass(&wc);
}

static void
FillBox(HDC hdc, int x, int y, int w, int h, COLORREF cr)
{
	RECT	rc;

	if (w <= 0)
		return;
	SetRect(&rc, x, y, x+w, y+h);
	FastFillRect(hdc, &rc, cr);
}

static int
GetSysCharWidth(HDC hdc)
{
	int	cw, ch, cb;

	GdGetTextSize(hdc->font->pfont,"X",1, &cw,&ch,&cb,MWTF_ASCII);
	return cw;
}

static int
GetSysCharHeight(HDC hdc)
{
	int	cw, ch, cb;

	GdGetTextSize(hdc->font->pfont,"X",1, &cw,&ch,&cb,MWTF_ASCII);
	return ch;
}

void pbarOnDraw (HWND hwnd, HDC hdc, PROGRESSDATA* pData, BOOL fVertical,
	BOOL fErase)
{
    int     x, y, w, h;
    unsigned int     nAllPart;
    unsigned int     nNowPart;
    int     whOne, nRem;
    int     ix, iy;
    int     i;
    int     step;
    COLORREF cr;
    RECT    rcClient;
    char    szText[8];
    
    if (!hdc || (pData->nMax == pData->nMin))
        return;
    
    if ((pData->nMax - pData->nMin) > 5)
        step = 5;
    else
        step = 1;

    GetClientRect (hwnd, &rcClient);
    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

    x = rcClient.left + WIDTH_PBAR_BORDER;
    y = rcClient.top + WIDTH_PBAR_BORDER;
    w = (rcClient.right - rcClient.left) - (WIDTH_PBAR_BORDER << 1);
    h = (rcClient.bottom - rcClient.top) - (WIDTH_PBAR_BORDER << 1);

    nAllPart = (pData->nMax - pData->nMin) / step;
    nNowPart = (pData->nPos - pData->nMin) / step;
    if (fVertical) {
        whOne = h / nAllPart;
        nRem = h % nAllPart;
    } else {
        whOne = w / nAllPart;
        nRem = w % nAllPart;
    }
        
    if (fErase)
	FillBox (hdc, x, y, w, h, GetSysColor(COLOR_BTNSHADOW));

    if(whOne >= 4) {
        if (fVertical) {
            for (i = 0, iy = y + h - 1; i < nNowPart; ++i) {
#if 0
                if ((iy - whOne) < y) 
                    whOne = iy - y;
#endif

		FillBox (hdc, x, iy - whOne, w, whOne - 1, BLUE);

                iy -= whOne + 1;
#if 0
                if(nRem > 0) {
                    iy --;
                    nRem --;
                }
#endif
            }
        }
        else {
            for (i = 0, ix = x + 1; i < nNowPart; ++i) {
#if 0
                if ((ix + whOne) > (x + w)) 
                    whOne = x + w - ix;
#endif
		FillBox (hdc, ix, y, whOne - 1, h, BLUE);
                ix += whOne + 1;
#if 0
                if(nRem > 0) {
                    ix ++;
                    nRem --;
                }
#endif
            }
        }
    }
    else {
        /* no vertical support */
        int d = nNowPart*100/nAllPart;
	int maxw = GetSysCharWidth (hdc) << 2;
	int charh = GetSysCharHeight (hdc);

	if (d > 50)
	    cr = BLUE;
        else
	    cr = GetSysColor(COLOR_BTNSHADOW);
	FillBox (hdc, x + ((w - maxw)>>1), y + ((h - charh) > 1), maxw,
		charh - 1, cr);
	FillBox (hdc, x, y, (int)((long)w*d/100L), h, BLUE);
        SetTextColor (hdc, WHITE);
        SetBkMode (hdc, TRANSPARENT);
        sprintf (szText, "%d%%", d);
        TextOut (hdc, x + ((w - GetSysCharWidth (hdc) * strlen (szText) )>>1), 
                      y + ((h - GetSysCharHeight(hdc) )>>1), 
                      szText, strlen(szText));
    }
}

static void pbarNormalizeParams (const HWND pCtrl, 
                PROGRESSDATA* pData, BOOL fNotify)
{
    if (pData->nPos > pData->nMax) {
        if (fNotify)
            SendMessage (GetParent ((HWND)pCtrl), WM_COMMAND, 
		(WPARAM)MAKELONG (pCtrl->id, PBN_REACHMAX), (LPARAM)pCtrl);
        pData->nPos = pData->nMax;
    }

    if (pData->nPos < pData->nMin) {
        if (fNotify)
            SendMessage (GetParent ((HWND)pCtrl), WM_COMMAND, 
		(WPARAM)MAKELONG (pCtrl->id, PBN_REACHMIN), (LPARAM)pCtrl);
        pData->nPos = pData->nMin;
    }
}

static LRESULT CALLBACK
ProgressBarCtrlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC           hdc;
    HWND	  pCtrl;
    PROGRESSDATA* pData;
    BOOL          fErase;
    RECT	rc;
    PAINTSTRUCT ps;
    int		pos;
    
    pCtrl = hwnd;
    
    switch (message)
    {
        case WM_CREATE:
            if (!(pData = malloc (sizeof (PROGRESSDATA)))) {
                fprintf(stderr, "Create progress bar control failure!\n");
                return -1;
            }
            
#if TEST
            pData->nMax     = 1000;
            pData->nMin     = 0;
            pData->nPos     = 500;
            pData->nStepInc = 100;
#else
            pData->nMax     = 100;
            pData->nMin     = 0;
            pData->nPos     = 0;
            pData->nStepInc = 10;
#endif

            pCtrl->userdata = (DWORD)pData;
        break;
    
        case WM_DESTROY:
            free ((void *)(pCtrl->userdata));
        break;

        case WM_GETDLGCODE:
            return DLGC_STATIC;

	case WM_NCPAINT:
		if(GetWindowLong(hwnd, GWL_STYLE) & WS_BORDER) {
			GetWindowRect(hwnd, &rc);
			hdc = GetWindowDC(hwnd);
			Draw3dBox(hdc, rc.left, rc.top, rc.right-rc.left,
				rc.bottom-rc.top, GetSysColor(COLOR_BTNSHADOW),
				GetSysColor(COLOR_BTNHIGHLIGHT));
		}
	    break;

	case WM_PAINT:
            hdc = BeginPaint (hwnd,&ps);
            pbarOnDraw (hwnd, hdc, (PROGRESSDATA *)pCtrl->userdata, 
                            hwnd->style & PBS_VERTICAL, TRUE);
            EndPaint (hwnd, &ps);
            break;

        case PBM_SETRANGE:
            pData = (PROGRESSDATA *)pCtrl->userdata;
            pData->nMin = min (wParam, lParam);
            pData->nMax = max (wParam, lParam);
            if (pData->nPos > pData->nMax)
                pData->nPos = pData->nMax;
            if (pData->nPos < pData->nMin)
                pData->nPos = pData->nMin;
        break;
        
        case PBM_SETSTEP:
            pData = (PROGRESSDATA *)pCtrl->userdata;
            pData->nStepInc = wParam;
        break;
        
        case PBM_SETPOS:
            pData = (PROGRESSDATA *)pCtrl->userdata;
            
            if (pData->nPos == wParam)
                break;

            fErase = (wParam < pData->nPos);
            pData->nPos = wParam;
            pbarNormalizeParams (pCtrl, pData, hwnd->style & PBS_NOTIFY);
	    InvalidateRect(hwnd, NULL, fErase);
        break;
        
        case PBM_DELTAPOS:
            pData = (PROGRESSDATA *)pCtrl->userdata;

            if (wParam == 0)
                break;
            
            fErase = (wParam < 0);
            pData->nPos += wParam;
            pbarNormalizeParams (pCtrl, pData, hwnd->style & PBS_NOTIFY);
	    InvalidateRect(hwnd, NULL, fErase);
        break;
        
        case PBM_STEPIT:
            pData = (PROGRESSDATA *)pCtrl->userdata;
            
            if (pData->nStepInc == 0)
                break;

            fErase = (pData->nStepInc < 0);
            pData->nPos += pData->nStepInc;
            pbarNormalizeParams (pCtrl, pData, hwnd->style & PBS_NOTIFY);
	    InvalidateRect(hwnd, NULL, fErase);
        break;
            
#if TEST
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
                pData = (PROGRESSDATA *)pCtrl->userdata;
		pos = pData->nPos;
		pos += pData->nStepInc;
		if (pos > pData->nMax)
			pos = pData->nMin;
		SendMessage(hwnd, PBM_SETPOS, pos, 0L);
		break;
#endif
        default:
    		return DefWindowProc (hwnd, message, wParam, lParam);
    }
    return 0;
}
