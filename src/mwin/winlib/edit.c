/*
 * Copyright (C) 1999, 2000, Wei Yongming.
 * Portions Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Edit control for Microwindows win32 api.
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

/* Note:
**  Although there was a version by Zhao Jianghua, this version of
**  EDIT control is written by Wei Yongming from scratch.
**
** Create date: 1999/8/26
**
** Modify records:
**
**  Who             When        Where       For What                Status
**-----------------------------------------------------------------------------
**  WEI Yongming    2000/02/24  Tsinghua    Add MPL License         Finished
**  Kevin Tseng     2000/05/30  gv          port to microwin        ported
**  Greg Haerr      2000/06/16  Utah        3d look, bug fixes      Finished
**  Kevin Tseng     2000/06/22  gv          port to mw-nanox        ported
**
** TODO:
**    * Selection.
**    * Undo.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "windows.h"	/* windef.h, winuser.h */
#include "wintools.h"
#include "device.h" 	/* GdGetTextSize */

#define USE_BIG5

#if 0
#define DEFAULT_FONT	DEFAULT_GUI_FONT
#endif
#define DEFAULT_FONT	SYSTEM_FIXED_FONT

#define WIDTH_EDIT_BORDER       2
#define MARGIN_EDIT_LEFT        1
#define MARGIN_EDIT_TOP         1
#define MARGIN_EDIT_RIGHT       2
#define MARGIN_EDIT_BOTTOM      1

#define LEN_SLEDIT_BUFFER       3000
#define LEN_SLEDIT_UNDOBUFFER   1024

#define EST_FOCUSED     0x00000001L
#define EST_MODIFY      0x00000002L
#define EST_READONLY    0x00000004L
#define EST_REPLACE     0x00000008L

#define EDIT_OP_NONE    0x00
#define EDIT_OP_DELETE  0x01
#define EDIT_OP_INSERT  0x02
#define EDIT_OP_REPLACE 0x03

typedef struct tagSLEDITDATA {
    HFONT   hFont;          /* hFont used */
    int     bufferLen;      /* length of buffer */

    int     dataEnd;        /* data end position */
    int     editPos;        /* current edit position */
    int     caretOff;       /* caret offset in box */
    int     startPos;       /* start display position */
    
    int     selStart;       /* selection start position */
    int     selEnd;         /* selection end position */
    
    int     passwdChar;     /* password character */
    
    int     leftMargin;     /* left margin */
    int     topMargin;      /* top margin */
    int     rightMargin;    /* right margin */
    int     bottomMargin;   /* bottom margin */
    
    int     hardLimit;      /* hard limit */

    int     lastOp;         /* last operation */
    int     lastPos;        /* last operation position */
    int     affectedLen;    /* affected len of last operation */
    int     undoBufferLen;  /* undo buffer len */
    char    undoBuffer [LEN_SLEDIT_UNDOBUFFER];	/* Undo buffer; */
    char    buffer [LEN_SLEDIT_BUFFER];		/* buffer */
} SLEDITDATA, *PSLEDITDATA;

static LRESULT CALLBACK
SLEditCtrlProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static int GetSysCharHeight (HWND hwnd) 
{
#ifndef USE_BIG5	    
	HDC 		hdc;
    	int xw, xh, xb;

    	hdc = GetDC(hwnd);
	SelectObject(hdc, GetStockObject(DEFAULT_FONT));
	GdSetFont(hdc->font->pfont);
    	GdGetTextSize(hdc->font->pfont,"X",1, &xw,&xh,&xb,MWTF_ASCII);
    	ReleaseDC(hwnd,hdc);

	return xh;
#else
	return 12;
#endif
}

static int GetSysCharWidth (HWND hwnd) 
{
#ifndef USE_BIG5	    
	HDC 		hdc;
    	int xw, xh, xb;

    	hdc = GetDC(hwnd);
	SelectObject(hdc, GetStockObject(DEFAULT_FONT));
	GdSetFont(hdc->font->pfont);
    	GdGetTextSize(hdc->font->pfont,"X",1, &xw,&xh,&xb,MWTF_ASCII);
    	ReleaseDC(hwnd,hdc);

	return xw;
#else
	return 6;
#endif
}

static int GetSysCCharWidth (HWND hwnd)
{
	return (2*GetSysCharWidth(hwnd));
}

int WINAPI MwRegisterEditControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)SLEditCtrlProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "EDIT";

	return RegisterClass(&wc);
}

static int edtGetOutWidth (const HWND pCtrl)
{
    return pCtrl->clirect.right - pCtrl->clirect.left 
            - ((PSLEDITDATA)(pCtrl->userdata2))->leftMargin
            - ((PSLEDITDATA)(pCtrl->userdata2))->rightMargin;
}

static int edtGetStartDispPosAtEnd (const HWND pCtrl, PSLEDITDATA pSLEditData)
{
    int         nOutWidth = edtGetOutWidth (pCtrl);
    int         endPos  = pSLEditData->dataEnd;
    int         newStartPos = pSLEditData->startPos;
    const char* buffer = pSLEditData->buffer;

    while (TRUE) {
        if ((endPos - newStartPos) * GetSysCharWidth (pCtrl) < nOutWidth)
            break;
        
	/* FIXME: #ifdef GB2312?*/
        if ((BYTE)buffer [newStartPos] > 0xA0)	/* 1st:gb:a1-f7,big5:a1-f9 */
	{
            newStartPos ++;
            if (newStartPos < pSLEditData->dataEnd) 
	    {
#ifndef USE_BIG5
                if ((BYTE)buffer [newStartPos] > 0xA0)
#else	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
                if ( ((BYTE)buffer [newStartPos] >= 0x40 && (BYTE)buffer[newStartPos] <= 0x7e) ||
                     ((BYTE)buffer [newStartPos] >= 0xa1 && (BYTE)buffer[newStartPos] <= 0xfe)) 
#endif
                    newStartPos ++;
            }
        }
        else
            newStartPos ++;
    }

    return newStartPos;
}

static int edtGetDispLen (const HWND pCtrl)
{
    int i, n = 0;
    int nOutWidth = edtGetOutWidth (pCtrl);
    int nTextWidth = 0;
    PSLEDITDATA pSLEditData = (PSLEDITDATA)(pCtrl->userdata2);
    const char* buffer = pSLEditData->buffer;

    for (i = pSLEditData->startPos; i < pSLEditData->dataEnd; i++) {
	/* FIXME #ifdef GB2312?*/
        if ((BYTE)buffer [i] > 0xA0)	/* st:gb:a1-f7,big5:a1-f9 */
	{
            i++;
            if (i < pSLEditData->dataEnd) 
	    {
#ifndef USE_BIG5
                if ((BYTE)buffer [i] > 0xA0)	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
#else	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
                if ( ((BYTE)buffer [i] >= 0x40 && (BYTE)buffer[i] <= 0x7e) ||
                     ((BYTE)buffer [i] >= 0xa1 && (BYTE)buffer[i] <= 0xfe))
#endif
		{
                    nTextWidth += GetSysCCharWidth (pCtrl);
                    n += 2;
                }
                else
                    i--;
            }
            else 
            {
                nTextWidth += GetSysCharWidth (pCtrl);
                n++;
            }
        }
        else 
        {
            nTextWidth += GetSysCharWidth (pCtrl);
            n++;
        }

        if (nTextWidth > nOutWidth)
            break;
    }

    return n;
}

static int edtGetOffset (HWND hwnd,const SLEDITDATA* pSLEditData, int x)
{
    int i;
    int newOff = 0;
    int nTextWidth = 0;
    const char* buffer = pSLEditData->buffer;

    x -= pSLEditData->leftMargin;
    for (i = pSLEditData->startPos; i < pSLEditData->dataEnd; i++) {
        if ((nTextWidth + (GetSysCharWidth(hwnd) >> 1)) >= x)
            break;

	/* FIXME #ifdef GB2312?*/
        if ((BYTE)buffer [i] > 0xA0)	/* 1st:gb:a1-f7,big5:a1-f9 */
	{
            i++;
            if (i < pSLEditData->dataEnd) 
	    {
#ifndef USE_BIG5
                if ((BYTE)buffer [i] > 0xA0)	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
#else	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
                if ( ((BYTE)buffer [i] >= 0x40 && (BYTE)buffer[i] <= 0x7e) || 
                     ((BYTE)buffer [i] >= 0xa1 && (BYTE)buffer[i] <= 0xfe))
#endif
		{
                    nTextWidth += GetSysCCharWidth (hwnd);
                    newOff += 2;
                }
                else
                    i --;
            }
            else 
            {
                nTextWidth += GetSysCharWidth (hwnd);
                newOff ++;
            }
        }
        else 
        {
            nTextWidth += GetSysCharWidth (hwnd);
            newOff ++;
        }

    }

    return newOff;
}

static BOOL edtIsACCharBeforePosition (const char* string, int pos)
{
    if (pos < 2)
        return FALSE;

/* 1st:gb:a1-f7,big5:a1-f9  2nd:gb:a1-fe,big5:40-7e,a1-fe */
#ifndef USE_BIG5
    /* FIXME #ifdef GB2312?*/
    if ((BYTE)string [pos - 2] > 0xA0 && (BYTE)string [pos - 1] > 0xA0)
        return TRUE;
#else
    if ((BYTE)string [pos - 2] > 0xA0)
    {
	if ( ((BYTE)string [pos - 1] >= 0x40 && (BYTE)string[pos - 1] <= 0x7e) ||
	     ((BYTE)string [pos - 1] >= 0xa1 && (BYTE)string[pos - 1] <= 0xfe))
            return TRUE;
    }
#endif

    return FALSE;
}

static BOOL edtIsACCharAtPosition (const char* string, int len, int pos)
{
    if (pos > (len - 2))
        return FALSE;

/* 1st:gb:a1-f7,big5:a1-f9  2nd:gb:a1-fe,big5:40-7e,a1-fe */
#ifndef USE_BIG5
    if ((BYTE)string [pos] > 0xA0 && (BYTE)string [pos + 1] > 0xA0)
        return TRUE;
#else
    if ((BYTE)string [pos] > 0xA0)
    {
	if ( ((BYTE)string [pos + 1] >= 0x40 && (BYTE)string [pos + 1] <= 0x7e) ||
	     ((BYTE)string [pos + 1] >= 0xa1 && (BYTE)string [pos + 1] <= 0xfe)) {
	    /*fprintf(stderr,"true\n");
	    fflush(stderr);*/
	    return TRUE;
	}
    }
#endif

    return FALSE;
}

LRESULT CALLBACK
SLEditCtrlProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{   
    HWND	pCtrl;
    DWORD       dwStyle;
    HDC         hdc;
    PSLEDITDATA pSLEditData;
    RECT	rc;

    pCtrl       = hWnd;
    dwStyle     = pCtrl->style;

    switch (message)
    {
        case WM_CREATE:
            if (!(pSLEditData = malloc (sizeof (SLEDITDATA)))) {
                fprintf (stderr, "EDIT: malloc error!\n");
                return -1;
            }

            pSLEditData->hFont      	= GetStockObject(DEFAULT_FONT);

            pSLEditData->bufferLen      = LEN_SLEDIT_BUFFER;
            pSLEditData->editPos        = 0;
            pSLEditData->caretOff       = 0;
            pSLEditData->startPos       = 0;
            
            pSLEditData->selStart       = 0;
            pSLEditData->selEnd         = 0;
            pSLEditData->passwdChar     = '*';
            pSLEditData->leftMargin     = MARGIN_EDIT_LEFT;
            pSLEditData->topMargin      = MARGIN_EDIT_TOP;
            pSLEditData->rightMargin    = MARGIN_EDIT_RIGHT;
            pSLEditData->bottomMargin   = MARGIN_EDIT_BOTTOM;

            pSLEditData->hardLimit      = -1;
            
            /* undo information */
            pSLEditData->lastOp         = EDIT_OP_NONE;
            pSLEditData->lastPos        = 0;
            pSLEditData->affectedLen    = 0;
            pSLEditData->undoBufferLen  = LEN_SLEDIT_UNDOBUFFER;
            pSLEditData->undoBuffer [0] = '\0';

            pSLEditData->dataEnd        = strlen (pCtrl->szTitle);
            memcpy (pSLEditData->buffer, pCtrl->szTitle,
                    min (LEN_SLEDIT_BUFFER, pSLEditData->dataEnd));

            pCtrl->userdata2 = (DWORD) pSLEditData;

            pCtrl->userdata  = 0;
        break;

        case WM_DESTROY:
            DestroyCaret ();

            free ((void*)pCtrl->userdata2);
        break;
#if 0        
        case WM_CHANGESIZE:
        {
            pCtrl->cl = pCtrl->left   + WIDTH_EDIT_BORDER;
            pCtrl->ct = pCtrl->top    + WIDTH_EDIT_BORDER;
            pCtrl->cr = pCtrl->right  - WIDTH_EDIT_BORDER;
            pCtrl->cb = pCtrl->bottom - WIDTH_EDIT_BORDER;
        }
        break;
#endif

#if 1	/* jmt: for edit: chinese support */
        case WM_SETFONT:
	{
		pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
        	pSLEditData->hFont = (HFONT)wParam;

        	ShowWindow(hWnd, SW_HIDE);
        	ShowWindow(hWnd, SW_SHOWNA);

        	if(LOWORD(lParam))
            		InvalidateRect(hWnd,NULL,TRUE);
	}
	return (LRESULT)0;
        
        case WM_GETFONT:
		pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
        	return (LRESULT)pSLEditData->hFont;
#endif    

#if 0    
        case WM_SETCURSOR:
            if (dwStyle & WS_DISABLED) 
	    {
                SetCursor (GetSystemCursor (IDC_ARROW));
                return 0;
            }
        break;
#endif
        case WM_KILLFOCUS:
            pCtrl->userdata &= ~EST_FOCUSED;

            HideCaret (hWnd);
	    DestroyCaret ();

            SendMessage (GetParent (hWnd), WM_COMMAND, 
		 (WPARAM) MAKELONG (pCtrl->id, EN_KILLFOCUS), (LPARAM)hWnd);
        break;
        
        case WM_SETFOCUS:
            if (pCtrl->userdata & EST_FOCUSED)
                return 0;
            
            pCtrl->userdata |= EST_FOCUSED;

            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
            /* only implemented for ES_LEFT align format. */

            CreateCaret (hWnd, NULL, 1 /*+ GetSysCharWidth(hWnd)*/,
		    hWnd->clirect.bottom-hWnd->clirect.top-2);
            SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                    + pSLEditData->leftMargin, pSLEditData->topMargin);
            ShowCaret (hWnd);

            SendMessage (GetParent (hWnd), WM_COMMAND,
	       (WPARAM) MAKELONG (pCtrl->id, EN_SETFOCUS), (LPARAM) hWnd);
        break;
        
        case WM_ENABLE:
            if ( (!(dwStyle & WS_DISABLED) && !wParam)
                    || ((dwStyle & WS_DISABLED) && wParam) ) {
                if (wParam)
                    pCtrl->style &= ~WS_DISABLED;
                else
                    pCtrl->style |=  WS_DISABLED;

                InvalidateRect (hWnd, NULL, FALSE);
            }
        break;

	case WM_NCCALCSIZE:
	{
		LPNCCALCSIZE_PARAMS lpnc;

		/* calculate client rect from passed window rect in rgrc[0]*/
		lpnc = (LPNCCALCSIZE_PARAMS)lParam;
		if(GetWindowLong(hWnd, GWL_STYLE) & WS_BORDER)
			InflateRect(&lpnc->rgrc[0], -2, -2);
	}
		break;

        case WM_NCPAINT:
            hdc = wParam? (HDC)wParam: GetWindowDC (hWnd);
	    GetWindowRect(hWnd, &rc);

            if (dwStyle & WS_BORDER)
		Draw3dInset(hdc, rc.left, rc.top,
			rc.right-rc.left, rc.bottom-rc.top);

            if (!wParam)
                ReleaseDC (hWnd, hdc);
            break;

        case WM_PAINT:
        {
            int     dispLen;
            char*   dispBuffer;
            RECT    rect,rc;
	    PAINTSTRUCT ps;

	    HGDIOBJ oldfont;
	    oldfont=NULL;

            hdc = BeginPaint (hWnd,&ps);
            GetClientRect (hWnd, &rect);
    
            if (dwStyle & WS_DISABLED)
            {
#if 0
                SetBrushColor (hdc, LTGRAY/*COLOR_lightgray*/);
                FillBox (hdc, 0, 0, rect.right, rect.bottom);
#else
		rc.left=0; rc.top=0; rc.bottom=rect.bottom; rc.right=rect.right;
		FillRect(hdc,&rc,GetStockObject(LTGRAY_BRUSH));
#endif
                SetBkColor (hdc, LTGRAY/*COLOR_lightgray*/);
            }
            else 
	    {
#if 0
                SetBrushColor (hdc, WHITE/*COLOR_lightwhite*/);
                FillBox (hdc, 0, 0, rect.right, rect.bottom);
#else
		rc.left=0; rc.top=0; rc.bottom=rect.bottom; rc.right=rect.right;
		FillRect(hdc,&rc,GetStockObject(WHITE_BRUSH));
#endif
                SetBkColor (hdc, WHITE/*COLOR_lightwhite*/);
            }

            SetTextColor (hdc, BLACK/*COLOR_black*/);
            dispLen = edtGetDispLen (pCtrl);
            if (dispLen == 0) 
	    {
                EndPaint (hWnd, &ps);
                break;
            }

            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);

#ifdef _DEBUG
            if (pSLEditData->startPos > pSLEditData->dataEnd)
                fprintf (stderr, "ASSERT failure: %s.\n", "Edit Paint");
#endif
            
            dispBuffer = ALLOCA(dispLen + 1);

            if (dwStyle & ES_PASSWORD)
                memset (dispBuffer, '*', dispLen);
            else
                memcpy (dispBuffer, 
                    pSLEditData->buffer + pSLEditData->startPos,
                    dispLen);

            dispBuffer [dispLen] = '\0';

            /* only implemented ES_LEFT align format for single line edit. */
            rect.left += pSLEditData->leftMargin;
            rect.top += pSLEditData->topMargin;
            rect.right -= pSLEditData->rightMargin;
            rect.bottom -= pSLEditData->bottomMargin;

#if 0	/* FIXME no ClipRectIntersect() */
#if 0            
            ClipRectIntersect (hdc, &rect);
#else
	    GdSetClipRects(hdc->psd,1,&rect);	/*??==ClipRectIntersect??*/
#endif
#endif

#ifdef USE_BIG5	    
	    oldfont=SelectObject(hdc,CreateFont(12,
			0,0,0,0,0,0,0,0,0,0,0,
			FF_DONTCARE|DEFAULT_PITCH,
			"HZXFONT"));
#else
    	    SelectObject(hdc, pSLEditData->hFont);
#endif
            TextOut (hdc, pSLEditData->leftMargin, pSLEditData->topMargin, 
                dispBuffer,-1);

#ifdef USE_BIG5	    
    	    DeleteObject(SelectObject(hdc,oldfont));
#endif
            
	    EndPaint (hWnd, &ps);

	    FREEA(dispBuffer);
        }
        break;
#if 1	/* jmt+ */
        case WM_KEYDOWN:
        {
            BOOL    bChange = FALSE;
            int     i;
            RECT    InvRect;
            int     deleted;

            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
        
            switch ((int)(wParam))	/* (LOWORD (wParam)) */
            {
#if 0
                case SCANCODE_ENTER:
                    SendMessage (GetParent (hWnd), WM_COMMAND, 
		    	(WPARAM) MAKELONG (pCtrl->id, EN_ENTER), (LPARAM) hWnd);
                return 0;

                case SCANCODE_HOME:
                    if (pSLEditData->editPos == 0)
                        return 0;

                    pSLEditData->editPos  = 0;
                    pSLEditData->caretOff = 0;

                    SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                        + pSLEditData->leftMargin, pSLEditData->topMargin);
                    if (pSLEditData->startPos != 0)
                        InvalidateRect (hWnd, NULL, FALSE);
                    
                    pSLEditData->startPos = 0;
                return 0;
           
                case SCANCODE_END:
                {
                    int newStartPos;
                   
                    if (pSLEditData->editPos == pSLEditData->dataEnd)
                        return 0;

                    newStartPos = edtGetStartDispPosAtEnd (pCtrl, pSLEditData);
                    
                    pSLEditData->editPos = pSLEditData->dataEnd;
                    pSLEditData->caretOff = pSLEditData->editPos - newStartPos;

                   SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd)
                        + pSLEditData->leftMargin, pSLEditData->topMargin);
                   if (pSLEditData->startPos != newStartPos)
                        InvalidateRect (hWnd, NULL, FALSE);
                    
                    pSLEditData->startPos = newStartPos;
                }
                return 0;
#endif

                case VK_LEFT: /* SCANCODE_CURSORBLOCKLEFT: */
                {
                    BOOL bScroll = FALSE;
                    int  scrollStep;
                    
                    if (pSLEditData->editPos == 0)
                        return 0;

                    if (edtIsACCharBeforePosition (pSLEditData->buffer, 
                            pSLEditData->editPos)) 
		    {
                        scrollStep = 2;
                        pSLEditData->editPos -= 2;
                    }
                    else {
                        scrollStep = 1;
                        pSLEditData->editPos --;
                    }

                    pSLEditData->caretOff -= scrollStep;
                    if (pSLEditData->caretOff == 0 
                            && pSLEditData->editPos != 0) 
                    {
                        bScroll = TRUE;

                        if (edtIsACCharBeforePosition (pSLEditData->buffer, 
                                pSLEditData->editPos)) 
                        {
                            pSLEditData->startPos -= 2;
                            pSLEditData->caretOff = 2;
                        }
                        else 
                        {
                            pSLEditData->startPos --;
                            pSLEditData->caretOff = 1;
                        }
                    }
                    else if (pSLEditData->caretOff < 0) 
                    {
                        pSLEditData->startPos = 0;
                        pSLEditData->caretOff = 0;
                    }
	        
                    SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                            + pSLEditData->leftMargin, pSLEditData->topMargin);
	
                    if (bScroll)
                        InvalidateRect (hWnd, NULL, FALSE);
                }
                return 0;
                
                case VK_RIGHT: /* SCANCODE_CURSORBLOCKRIGHT: */
                {
                    BOOL bScroll = FALSE;
                    int  scrollStep, moveStep;

                    if (pSLEditData->editPos == pSLEditData->dataEnd)
                        return 0;

                    if (edtIsACCharAtPosition (pSLEditData->buffer, 
                                pSLEditData->dataEnd,
                                pSLEditData->startPos)) 
		    {
                        if (edtIsACCharAtPosition (pSLEditData->buffer, 
                                    pSLEditData->dataEnd,
                                    pSLEditData->editPos)) 
                        {
                            scrollStep = 2;
                            moveStep = 2;
                            pSLEditData->editPos  += 2;
                        }
                        else 
                        {
                            scrollStep = 2;
                            moveStep = 1;
                            pSLEditData->editPos ++;
                        }
                    }
                    else 
		    {
                        if (edtIsACCharAtPosition (pSLEditData->buffer, 
                                    pSLEditData->dataEnd,
                                    pSLEditData->editPos)) 
			{
                            if (edtIsACCharAtPosition (pSLEditData->buffer, 
                                    pSLEditData->dataEnd,
                                    pSLEditData->startPos + 1))
                                scrollStep = 3;
                            else
                                scrollStep = 2;

                            moveStep = 2;
                            pSLEditData->editPos += 2;
                        }
                        else 
			{
                            scrollStep = 1;
                            moveStep = 1;
                            pSLEditData->editPos ++;
                        }
                    }

                    pSLEditData->caretOff += moveStep;
                    if (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                            > edtGetOutWidth (pCtrl)) 
		    {
                        bScroll = TRUE;
                        pSLEditData->startPos += scrollStep;

                        pSLEditData->caretOff = 
                            pSLEditData->editPos - pSLEditData->startPos;
                    }

                    SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                            + pSLEditData->leftMargin, pSLEditData->topMargin);
                    if (bScroll)
                        InvalidateRect (hWnd, NULL, FALSE);
                }
                return 0;
#if 0                
                case SCANCODE_INSERT:
                    pCtrl->userdata ^= EST_REPLACE;
                break;
#endif
                case VK_DELETE: /* SCANCODE_REMOVE: */
                    if ((pCtrl->userdata & EST_READONLY)
                            || (pSLEditData->editPos == pSLEditData->dataEnd)){
	#if 0	/* fix: no ping() */
			Ping ();
	#endif
                        return 0;
                    }
                    
                    if (edtIsACCharAtPosition (pSLEditData->buffer, 
			    pSLEditData->dataEnd, pSLEditData->editPos))
                        deleted = 2;
                    else
                        deleted = 1;
                        
                    for (i = pSLEditData->editPos; 
                            i < pSLEditData->dataEnd - deleted; i++)
                        pSLEditData->buffer [i] 
                            = pSLEditData->buffer [i + deleted];

                    pSLEditData->dataEnd -= deleted;
                    bChange = TRUE;
                    
                    InvRect.left = pSLEditData->leftMargin
			    + pSLEditData->caretOff * GetSysCharWidth (hWnd);
                    InvRect.top = pSLEditData->topMargin;
                    InvRect.right = pCtrl->clirect.right - pCtrl->clirect.left;
                    InvRect.bottom = pCtrl->clirect.bottom - pCtrl->clirect.top;

                    InvalidateRect (hWnd, &InvRect, FALSE);
                break;

                case VK_BACK: /* SCANCODE_BACKSPACE: */
                    if ((pCtrl->userdata & EST_READONLY)
                            || (pSLEditData->editPos == 0)) {
#if 0 	/* fix: no ping */
                        Ping ();
#endif
                        return 0;
                    }

                    if (edtIsACCharBeforePosition (pSLEditData->buffer, 
                                    pSLEditData->editPos))
                        deleted = 2;
                    else
                        deleted = 1;
                        
                    for (i = pSLEditData->editPos; 
                            i < pSLEditData->dataEnd;
                            i++)
                        pSLEditData->buffer [i - deleted] 
                            = pSLEditData->buffer [i];

                    pSLEditData->dataEnd -= deleted;
                    pSLEditData->editPos -= deleted;
                    bChange = TRUE;
                    
                    pSLEditData->caretOff -= deleted;
                    if (pSLEditData->caretOff == 0 
                            && pSLEditData->editPos != 0) {
                        if (edtIsACCharBeforePosition (pSLEditData->buffer, 
                                pSLEditData->editPos)) {
                            pSLEditData->startPos -= 2;
                            pSLEditData->caretOff = 2;
                        }
                        else {
                            pSLEditData->startPos --;
                            pSLEditData->caretOff = 1;
                        }
                        
                        InvRect.left = pSLEditData->leftMargin;
                        InvRect.top = pSLEditData->topMargin;
                        InvRect.right = pCtrl->clirect.right -
				pCtrl->clirect.left;
                        InvRect.bottom = pCtrl->clirect.bottom -
				pCtrl->clirect.top;
                    }
                    else {
                        InvRect.left = pSLEditData->leftMargin
			    + pSLEditData->caretOff * GetSysCharWidth (hWnd);
                        InvRect.top = pSLEditData->topMargin;
                        InvRect.right = pCtrl->clirect.right -
				pCtrl->clirect.left;
                        InvRect.bottom = pCtrl->clirect.bottom -
				pCtrl->clirect.top;
                    }

                    SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                            + pSLEditData->leftMargin, pSLEditData->topMargin);
                    InvalidateRect (hWnd, &InvRect, FALSE);
                break;

                default:
                break;
            }
       
            if (bChange)
                SendMessage (GetParent (hWnd), WM_COMMAND, 
                    (WPARAM) MAKELONG (pCtrl->id, EN_CHANGE), (LPARAM) hWnd);
        }
	break;
#endif
        case WM_CHAR:
        {
            char charBuffer [2];
            int  i, chars, scrollStep, inserting;
            RECT InvRect;

            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);

            if (dwStyle & ES_READONLY) {

#if 0	/* fix: no ping() */
                Ping();
#endif
                return 0;
            }
            
            if (HIBYTE (wParam)) {
                charBuffer [0] = LOBYTE (wParam);
                charBuffer [1] = HIBYTE (wParam);
                chars = 2;
            }
            else {
                charBuffer [0] = LOBYTE (wParam);
                chars = 1;
            }
            
            if (chars == 1) {
                switch (charBuffer [0])
                {
                    case 0x00:  /* NULL */
                    case 0x07:  /* BEL */
                    case 0x08:  /* BS */
                    case 0x09:  /* HT */
                    case 0x0A:  /* LF */
                    case 0x0B:  /* VT */
                    case 0x0C:  /* FF */
                    case 0x0D:  /* CR */
                    case 0x1B:  /* Escape */
                    return 0;
                }
            }

            if (pCtrl->userdata & EST_REPLACE) {
                if (pSLEditData->dataEnd == pSLEditData->editPos)
                    inserting = chars;
                else if (edtIsACCharAtPosition (pSLEditData->buffer, 
                                pSLEditData->dataEnd,
                                pSLEditData->editPos)) {
                    if (chars == 2)
                        inserting = 0;
                    else
                        inserting = -1;
                }
                else {
                    if (chars == 2)
                        inserting = 1;
                    else
                        inserting = 0;
                }
            }
            else
                inserting = chars;

            /* check space */
            if (pSLEditData->dataEnd + inserting > pSLEditData->bufferLen) {

#if 0	/* fix: no ping() */
                Ping ();
#endif
                SendMessage (GetParent (hWnd), WM_COMMAND,
		    (WPARAM) MAKELONG (pCtrl->id, EN_MAXTEXT), (LPARAM) hWnd);
                return 0;
            }
            else if ((pSLEditData->hardLimit >= 0) 
                        && ((pSLEditData->dataEnd + inserting) 
                            > pSLEditData->hardLimit)) {
#if 0	/* fix: no ping() */
                Ping ();
#endif
                SendMessage (GetParent (hWnd), WM_COMMAND,
		    (WPARAM) MAKELONG (pCtrl->id, EN_MAXTEXT), (LPARAM) hWnd);
                return 0;
            }

            if (inserting == -1) {
                for (i = pSLEditData->editPos; i < pSLEditData->dataEnd-1; i++)
                    pSLEditData->buffer [i] = pSLEditData->buffer [i + 1];
            }
            else if (inserting > 0) {
                for (i = pSLEditData->dataEnd + inserting - 1; 
                        i > pSLEditData->editPos + inserting - 1; 
                        i--)
                    pSLEditData->buffer [i] 
                            = pSLEditData->buffer [i - inserting];
            }

            for (i = 0; i < chars; i++)
                    pSLEditData->buffer [pSLEditData->editPos + i] 
                        = charBuffer [i];
            
            pSLEditData->editPos += chars;
            pSLEditData->caretOff += chars;
            pSLEditData->dataEnd += inserting;

            if (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                            > edtGetOutWidth (pCtrl))
            {
                if (edtIsACCharAtPosition (pSLEditData->buffer, 
                                pSLEditData->dataEnd,
                                pSLEditData->startPos))
                    scrollStep = 2;
                else {
                    if (chars == 2) {
                        if (edtIsACCharAtPosition (pSLEditData->buffer, 
                                pSLEditData->dataEnd,
                                pSLEditData->startPos + 1))
                            scrollStep = 3;
                        else
                            scrollStep = 2;
                    }
                    else
                        scrollStep = 1;
                }
                    
                pSLEditData->startPos += scrollStep;

                pSLEditData->caretOff = 
                            pSLEditData->editPos - pSLEditData->startPos;

                InvRect.left = pSLEditData->leftMargin;
                InvRect.top = pSLEditData->topMargin;
                InvRect.right = pCtrl->clirect.right - pCtrl->clirect.left;
                InvRect.bottom = pCtrl->clirect.bottom - pCtrl->clirect.top;
            }
            else {
                InvRect.left = pSLEditData->leftMargin
                                    + (pSLEditData->caretOff - chars)
                                        * GetSysCharWidth (hWnd);
                InvRect.top = pSLEditData->topMargin;
                InvRect.right = pCtrl->clirect.right - pCtrl->clirect.left;
                InvRect.bottom = pCtrl->clirect.bottom - pCtrl->clirect.top;
            }

            SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                            + pSLEditData->leftMargin, pSLEditData->topMargin);
            InvalidateRect (hWnd, &InvRect, FALSE);

            SendMessage (GetParent (hWnd), WM_COMMAND,
                (WPARAM) MAKELONG (pCtrl->id, EN_CHANGE), (LPARAM) hWnd);
        }
        break;

        case WM_GETTEXTLENGTH:
            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
            return pSLEditData->dataEnd;
        
        case WM_GETTEXT:
        {
            char*   buffer = (char*)lParam;
            int     len;

            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);

            len = min ((int)wParam, pSLEditData->dataEnd);

            memcpy (buffer, pSLEditData->buffer, len);
            buffer [len] = '\0';

            return len;
        }
        break;

        case WM_SETTEXT:
        {
            int len;

            if (dwStyle & ES_READONLY)
                return 0;

            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);

            
            len = strlen ((char*)lParam);
            len = min (len, pSLEditData->bufferLen);
            
            if (pSLEditData->hardLimit >= 0)
                len = min (len, pSLEditData->hardLimit);
           
            pSLEditData->dataEnd = len;
            memcpy (pSLEditData->buffer, (char*)lParam, len);

            pSLEditData->editPos        = 0;
            pSLEditData->caretOff       = 0;
            pSLEditData->startPos       = 0;
            
            InvalidateRect (hWnd, NULL, FALSE);
        }
        break;

        case WM_LBUTTONDBLCLK:
        break;
        
        case WM_LBUTTONDOWN:
        {
            int newOff;

            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
            newOff = edtGetOffset (hWnd,pSLEditData, LOWORD (lParam));
            
            if (newOff != pSLEditData->caretOff) 
	    {
                pSLEditData->editPos += newOff - pSLEditData->caretOff;
                pSLEditData->caretOff = newOff;

                SetCaretPos (pSLEditData->caretOff * GetSysCharWidth (hWnd) 
                        + pSLEditData->leftMargin, pSLEditData->topMargin);
            }
        }
        break;

        case WM_LBUTTONUP:
        break;
        
        case WM_MOUSEMOVE:
        break;

        case WM_GETDLGCODE:
        return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;

        case EM_SETREADONLY:
            if (wParam)
                pCtrl->style/*dwStyle*/ |= ES_READONLY;
            else
                pCtrl->style/*dwStyle*/ &= ~ES_READONLY;
        return 0;
        
        case EM_SETPASSWORDCHAR:
            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);

            if (pSLEditData->passwdChar != (int)wParam) {
                if (dwStyle & ES_PASSWORD) {
                    pSLEditData->passwdChar = (int)wParam;
                    InvalidateRect (hWnd, NULL, TRUE);
                }
            }
        return 0;
    
        case EM_GETPASSWORDCHAR:
        {
            int* passwdchar;
            
            pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
            passwdchar = (int*) lParam;

            *passwdchar = pSLEditData->passwdChar;
        }
        return 0;
    
        case EM_LIMITTEXT:
        {
            int newLimit = (int)wParam;
            
            if (newLimit >= 0) {
                pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
                if (pSLEditData->bufferLen < newLimit)
                    pSLEditData->hardLimit = -1;
                else
                    pSLEditData->hardLimit = newLimit;
            }
        }
        return 0;
    
        default:
    		return DefWindowProc (hWnd, message, wParam, lParam);
        break;
    } 
    return 0;
}
