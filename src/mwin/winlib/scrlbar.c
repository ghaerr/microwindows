/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1999, 2000, Wei Yongming.
 * jmt: scrollbar thumb ported
 *
 * Microwindows win32 Scrollbars control
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS	/* jmt: for color macros */
#include "windows.h"
#include "wintern.h"
#include "wintools.h"

/* scrollbar status/positions*/
#define SBS_UNKNOWN		0x0000
#define SBS_LEFTARROW		0x0001
#define SBS_RIGHTARROW		0x0002
#define SBS_LEFTSPACE		0x0004
#define SBS_RIGHTSPACE		0x0008
#define SBS_HORZTHUMB		0x0010
#define SBS_UPARROW		0x0020
#define SBS_DOWNARROW		0x0040
#define SBS_UPSPACE		0x0080
#define SBS_DOWNSPACE		0x0100
#define SBS_VERTTHUMB		0x0200
#define SBS_MASK		0x03ff
#define SBS_DISABLED		0x4000
#define SBS_HIDE		0x8000


#define MWM_DEFBARLEN	18
#define MWM_MINBARLEN	8

static LRESULT CALLBACK
ScrollbarControlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


int WINAPI MwRegisterScrollbarControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)ScrollbarControlProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "SCROLLBAR";

	return RegisterClass(&wc);
}

static DWORD GetWindowStyle (HWND hwnd)
{
	return hwnd->style;
}

static int
wndGetBorder(HWND hwnd)
{
	if (hwnd->style & WS_BORDER)  {
		if ((hwnd->style & WS_CAPTION) == WS_CAPTION)
			return mwSYSMETRICS_CXFRAME;
		return mwSYSMETRICS_CXBORDER;
	}
	return 0;
}

static BOOL
wndGetVScrollBarRect (HWND hwnd, RECT* rcVBar)
{
	int cx,cy; RECT rc;
    	MWSCROLLBARINFO* pData;
	
        pData = (MWSCROLLBARINFO *)hwnd->userdata;
	rc = hwnd->winrect;
	cx=rc.right-rc.left;
	cy=rc.bottom-rc.top;

	rcVBar->left = hwnd->winrect.right - cx
		- wndGetBorder (hwnd);
	rcVBar->right = hwnd->winrect.right - wndGetBorder (hwnd);
	rcVBar->top  = hwnd->winrect.top;
	rcVBar->bottom = hwnd->winrect.bottom - wndGetBorder (hwnd);

	return TRUE;
}

static BOOL
wndGetHScrollBarRect (HWND hwnd, RECT* rcHBar)
{
	int cx,cy; RECT rc;
    	MWSCROLLBARINFO* pData;
	
        pData = (MWSCROLLBARINFO *)hwnd->userdata;
	rc = hwnd->winrect;
	cx=rc.right-rc.left;
	cy=rc.bottom-rc.top;

        rcHBar->top = hwnd->winrect.bottom - cy
                        - wndGetBorder (hwnd);
        rcHBar->bottom = hwnd->winrect.bottom - wndGetBorder (hwnd);
        rcHBar->left  = hwnd->winrect.left;
        rcHBar->right = hwnd->winrect.right - wndGetBorder (hwnd);

        return TRUE;
}

void
MwPaintScrollbars(HWND hwnd, HDC hdc, DWORD style)
{
	BOOL	vertbar = (style==SBS_VERT);
        BOOL	horzbar = (style==SBS_HORZ);
	BOOL	fGotDC = FALSE;
	RECT	rc,rc2;

	POINT	p3[3];
	int	shrink=2;

        int start = 0;
        RECT rcHBar, rcVBar;

	int cx,cy;
    	MWSCROLLBARINFO* pData;
	
        pData = (MWSCROLLBARINFO *)hwnd->userdata;
	rc = hwnd->winrect;
	cx=rc.right-rc.left;
	cy=rc.bottom-rc.top;

	if (!hdc && (horzbar || vertbar)) {
		hdc = GetWindowDC(hwnd);
		fGotDC = TRUE;
	}

	if (vertbar) {

#if 1
		/* bkgnd */
		rc2.left=rc.left; rc2.right=rc2.left+ cx;
		rc2.top=rc.top;
		rc2.bottom=rc2.top+ cx;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));
		rc2.top=rc.bottom- cx;
		rc2.bottom=rc2.top+ cx;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));
#endif
		/* up */
		Draw3dUpDownState(hdc, rc.left, rc.top,
			cx, cx,
			pData->status & SBS_UPARROW);
		/* down */
		Draw3dUpDownState(hdc, rc.left,rc.bottom-cx,
			cx, cx,
			pData->status & SBS_DOWNARROW);
/* jmt: draw arrows */
		SelectObject(hdc,GetStockObject(BLACK_BRUSH));
		/* up */
		p3[0].x= rc.left + (cx/2) - 1;
		p3[0].y= rc.top + 2 + shrink;
		p3[1].x= rc.left + 2 + shrink - 1;
		p3[1].y= rc.top + (cx-4) - shrink;
		p3[2].x= rc.left + (cx-4) - shrink;
		p3[2].y= rc.top + (cx-4) - shrink;
		Polygon(hdc,p3,3);
		/* down */
		p3[0].x= rc.left + (cx/2) - 1;
		p3[0].y= rc.bottom - 4 - shrink;
		p3[1].x= rc.left + 2 + shrink - 1;
		p3[1].y= rc.bottom-cx + 2 + shrink;
		p3[2].x= rc.left + (cx-4) - shrink;
		p3[2].y= rc.bottom-cx + 2 + shrink;
		Polygon(hdc,p3,3);

        	/* draw moving bar */

    		wndGetVScrollBarRect (hwnd, &rcVBar);
    		rcVBar.left -- ;
    		/*rcVBar.right -- ;*/

        	start = rcVBar.top + cx + pData->barStart;
                    
        	if (start + pData->barLen > rcVBar.bottom)
            		start = rcVBar.bottom - pData->barLen;
		
		if (pData->barLen == 0)
			pData->barLen=rc.bottom-rc.top-(cx*2); 

		/* bkgnd */
		rc2.left=rc.left; rc2.right=rc.right/*-1*/;
		rc2.top=rc.top+cx;
		rc2.bottom=start;
		if (rc2.bottom>rc2.top)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

		rc2.top=start+pData->barLen;
		rc2.bottom=rc.bottom-cx;
		if (rc2.bottom>rc2.top)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

        	Draw3dUpFrame (hdc, rcVBar.left, start, rcVBar.right,
	    		start + pData->barLen);
		/*printf("barv:(l,t,r,b):(%d,%d,%d,%d)\n",
        		rcVBar.left, start, rcVBar.right,
	    		start + pData->barLen);*/

	}
	if (horzbar) {
#if 1
		/* bkgnd */
		rc2.top=rc.top; rc2.bottom=rc2.top+ cy;
		rc2.left=rc.left;
		rc2.right=rc2.left+ cy;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));
		rc2.left=rc.right- cy;
		rc2.right=rc2.left+ cy;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));
#endif
		/* left */
		Draw3dUpDownState(hdc, rc.left, rc.top,
			cy, cy,
			pData->status & SBS_LEFTARROW);
		/* right */
		Draw3dUpDownState(hdc, rc.right-cy, rc.top,
			cy, cy,
			pData->status & SBS_RIGHTARROW);
/* jmt: draw arrows */
		SelectObject(hdc,GetStockObject(BLACK_BRUSH));
		/* left */
		p3[0].x= rc.left + 2 + shrink;
		p3[0].y= rc.top + (cy/2) ;
		p3[1].x= rc.left + (cy-4) - shrink ;
		p3[1].y= rc.top + 2 + shrink;
		p3[2].x= rc.left + (cy-4) - shrink;
		p3[2].y= rc.bottom - 4 - shrink + 1;
		Polygon(hdc,p3,3);
		/* right */
		p3[0].x= rc.right - 4 - shrink;
		p3[0].y= rc.top + (cy/2) ;
		p3[1].x= rc.right-cy + 2 + shrink ;
		p3[1].y= rc.top + 2 + shrink;
		p3[2].x= rc.right-cy + 2 + shrink;
		p3[2].y= rc.bottom - 4 - shrink + 1;
		Polygon(hdc,p3,3);

        	/* draw moving bar. */

    		wndGetHScrollBarRect (hwnd, &rcHBar);
    		rcHBar.top -- ;
    		/*rcHBar.bottom -- ;*/

        	start = rcHBar.left + cy + pData->barStart;

        	if (start + pData->barLen > rcHBar.right)
            		start = rcHBar.right - pData->barLen;

		if (pData->barLen == 0)
			pData->barLen=rc.right-rc.left-(cy*2); 

		/* bkgnd */
		rc2.top=rc.top; rc2.bottom=rc.bottom/*-1*/;
		rc2.left=rc.left+cy;
		rc2.right=start;
		if (rc2.right>rc2.left)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

		rc2.left=start+pData->barLen;
		rc2.right=rc.right-cy;
		if (rc2.right>rc2.left)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

        	Draw3dUpFrame (hdc, start, rcHBar.top, start + pData->barLen,
	    		rcHBar.bottom);
		/*printf("barh:(l,t,r,b):(%d,%d,%d,%d)\n",
        		start, rcHBar.top, start + pData->barLen,
	    		rcHBar.bottom);*/
	}

	if (fGotDC)
		ReleaseDC(hwnd, hdc);
}

/* handle a non-client message for a scrollbar*/
void
MwHandleMessageScrollbar(HWND hwnd, WPARAM hitcode, LPARAM lParam, UINT msg, DWORD style)
{
	int	pos = SBS_UNKNOWN;
	BOOL	vertbar = (style==SBS_VERT);
        BOOL	horzbar = (style==SBS_HORZ);
	int *	pStat;
	POINT	pt;
	RECT	rc;

	static BOOL bDraw;

    	static int downPos = SBS_UNKNOWN;
    	static int sbCode;
    	int newThumbPos;

	int itemMoveable,itemCount,itemVisible,moveRange;	/* jmt:2k0819 */
	int moveTop,moveBottom,moveLeft,moveRight;	/* jmt:2k0819 */

	int cx,cy;
    	MWSCROLLBARINFO* pData;
	
        pData = (MWSCROLLBARINFO *)hwnd->userdata;
	rc = hwnd->winrect;
	cx=rc.right-rc.left;
	cy=rc.bottom-rc.top;

	POINTSTOPOINT(pt, lParam);
	for (;;) {	/* use for() to allow break statement*/
		if (vertbar) 
		{
			pStat = &pData->status;
			rc = hwnd->winrect;
			rc.bottom = rc.top + cx;
			if (PtInRect(&rc, pt)) 
			{
				pos = SBS_UPARROW;
				break;
			}
			rc.bottom = hwnd->winrect.bottom;
			rc.top = rc.bottom - cx;
			if (PtInRect(&rc, pt)) 
			{
				pos = SBS_DOWNARROW;
				break;
			}
			pos = SBS_VERTTHUMB;
		} else if (horzbar) 
		{
			pStat = &pData->status;
			rc = hwnd->winrect;
			rc.right = rc.left + cy;
			if (PtInRect(&rc, pt)) {
				pos = SBS_LEFTARROW;
				break;
			}
			rc.right = hwnd->winrect.right;
			rc.left = rc.right - cy;
			if (PtInRect(&rc, pt)) {
				pos = SBS_RIGHTARROW;
				break;
			}
			pos = SBS_HORZTHUMB;
		} else
			return;
		break;
	}

	if (pos == SBS_UNKNOWN)
		return;

	*pStat &= ~SBS_MASK;		/* remove stray mouse states*/

	if (msg == WM_NCLBUTTONDOWN || msg == WM_NCLBUTTONDBLCLK)
		*pStat |= pos;
	else *pStat &= ~pos;

	if (msg == WM_NCLBUTTONDOWN || msg == WM_NCLBUTTONDBLCLK)
		bDraw=TRUE;

	if (bDraw)
		MwPaintScrollbars(hwnd, NULL,style);

        if (pos == SBS_UPARROW || pos == SBS_LEFTARROW)	/* jmt:2k0820 */
	{
                if (pData->curPos != pData->minPos)
			sbCode = SB_LINEUP;
        }
        else if (pos == SBS_DOWNARROW || pos == SBS_RIGHTARROW)	/* jmt:2k0820 */
	{
                if (pData->curPos != pData->maxPos)
			sbCode = SB_LINEDOWN;
        }
        else if (pos == SBS_VERTTHUMB || pos == SBS_HORZTHUMB)
	{
                sbCode = SB_THUMBTRACK;
        }

	switch(msg)
	{
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	    downPos = pos;
        break;

        case WM_NCMOUSEMOVE:
	    if (vertbar) 
	    {
		if (sbCode == SB_THUMBTRACK && downPos == SBS_VERTTHUMB)
	    	{
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->winrect;
			moveTop = rc.top + cx;
			moveBottom = hwnd->winrect.bottom - cx;
			moveRange = moveBottom - moveTop;

			itemCount = pData->maxPos - pData->minPos + 1;
			itemVisible = pData->pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			newThumbPos = ((pt.y - moveTop) * itemMoveable) / moveRange;
			printf("((%d-%d)*%d)/%d=%d\n",
				pt.y,moveTop,itemMoveable,moveRange,newThumbPos);

                	if ( newThumbPos >= pData->minPos &&
                    	     newThumbPos <= pData->maxPos)
			{
                    	     	SendMessage (hwnd,
                        		WM_VSCROLL, SB_THUMBTRACK, newThumbPos);

                    	     	SendMessage (GetParent(hwnd),
                        		WM_VSCROLL, SB_THUMBTRACK, newThumbPos);
			}
                	break;
            	}
	    }
	    if (horzbar) 
	    {
		if (sbCode == SB_THUMBTRACK && downPos == SBS_HORZTHUMB)
		{
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->winrect;
			moveLeft = rc.left + cy;
			moveRight = hwnd->winrect.right - cy;
			moveRange = moveRight - moveLeft;

			itemCount = pData->maxPos - pData->minPos + 1;
			itemVisible = pData->pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			newThumbPos = ((pt.x - moveLeft) * itemMoveable) / moveRange;
			printf("((%d-%d)*%d)/%d=%d\n",
				pt.y,moveLeft,itemMoveable,moveRange,newThumbPos);
			    
			if ( newThumbPos >= pData->minPos &&
			     newThumbPos <= pData->maxPos)
			{
			    	SendMessage (hwnd,
					WM_HSCROLL, SB_THUMBTRACK, newThumbPos);

			    	SendMessage (GetParent(hwnd),
					WM_HSCROLL, SB_THUMBTRACK, newThumbPos);
			}
			break;
		}
             }
        break;

	case WM_NCLBUTTONUP:
	    bDraw=FALSE;
	    downPos = SBS_UNKNOWN;

	    if (sbCode==SB_THUMBTRACK)
	    {
		    if (vertbar) 
		    {
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->winrect;
			moveTop = rc.top + cx;
			moveBottom = hwnd->winrect.bottom - cx;
			moveRange = moveBottom - moveTop;

			itemCount = pData->maxPos - pData->minPos + 1;
			itemVisible = pData->pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			newThumbPos = ((pt.y - moveTop) * itemMoveable) / moveRange;
			printf("((%d-%d)*%d)/%d=%d\n",
				pt.y,moveTop,itemMoveable,moveRange,newThumbPos);

			if ( newThumbPos >= pData->minPos &&
			     newThumbPos <= pData->maxPos)
			{
				SendMessage (hwnd,
					WM_VSCROLL, SB_THUMBTRACK, newThumbPos);

				SendMessage (GetParent(hwnd),
					WM_VSCROLL, SB_THUMBTRACK, newThumbPos);
			}
			break;	/* case */
		    }
		    if (horzbar) 
		    {
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->winrect;
			moveLeft = rc.left + cy;
			moveRight = hwnd->winrect.right - cy;
			moveRange = moveRight - moveLeft;

			itemCount = pData->maxPos - pData->minPos + 1;
			itemVisible = pData->pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			newThumbPos = ((pt.x - moveLeft) * itemMoveable) / moveRange;
			printf("((%d-%d)*%d)/%d=%d\n",
				pt.y,moveLeft,itemMoveable,moveRange,newThumbPos);
				    
			if ( newThumbPos >= pData->minPos &&
			     newThumbPos <= pData->maxPos)
			{
			    SendMessage (hwnd,
					WM_HSCROLL, SB_THUMBTRACK, newThumbPos);

			    SendMessage (GetParent(hwnd),
					WM_HSCROLL, SB_THUMBTRACK, newThumbPos);
			}
			break;	/* case */
		    }
	    }
	    else
	    {
		if (vertbar) 
		{
			SendMessage (hwnd, WM_VSCROLL, sbCode, 0);
			SendMessage (GetParent(hwnd), WM_VSCROLL, sbCode, 0);
		}
		if (horzbar) 
		{
			SendMessage (hwnd, WM_HSCROLL, sbCode, 0);
			SendMessage (GetParent(hwnd), WM_HSCROLL, sbCode, 0);
		}
	    }
	break;
	}
}


static BOOL
PtInRect2(const RECT *lprc, int x, int y)
{
	POINT	p;

	p.x = x;
	p.y = y;
	return PtInRect(lprc, p);
}

static void
wndScrollBarPos (HWND hwnd, BOOL bIsHBar, RECT* rcBar)	/* jmt: 2k0820 */
{
    UINT moveRange;
    PMWSCROLLBARINFO pSBar;
    int cx,cy;
    RECT rc;
	
    rc = hwnd->winrect;
    cx=rc.right-rc.left;
    cy=rc.bottom-rc.top;
    pSBar = (MWSCROLLBARINFO *)hwnd->userdata;

    if (pSBar->minPos == pSBar->maxPos) {
        pSBar->status |= SBS_HIDE;
        return;
    }

    if (bIsHBar)
        moveRange = (rcBar->right - rcBar->left) - (cy << 1);
    else
        moveRange = (rcBar->bottom - rcBar->top) - (cx << 1);


    if (pSBar->pageStep == 0) 
    {
        pSBar->barLen = MWM_DEFBARLEN;

        if (pSBar->barLen > moveRange)
            pSBar->barLen = MWM_MINBARLEN;
    }
    else 
    {
        pSBar->barLen = moveRange * pSBar->pageStep /
	      (pSBar->maxPos - pSBar->minPos + 1);
        if (pSBar->barLen < MWM_MINBARLEN)
            pSBar->barLen = MWM_MINBARLEN;
    }

    pSBar->barStart = moveRange * (pSBar->curPos - pSBar->minPos) /
       (pSBar->maxPos - pSBar->minPos + 1);


    if (pSBar->barStart + pSBar->barLen > moveRange)
        pSBar->barStart = moveRange - pSBar->barLen;


    if (pSBar->barStart < 0)
        pSBar->barStart = 0;
}

static PMWSCROLLBARINFO wndGetScrollBar (HWND pWin)	/* jmt: 2k0820 */
{
    	MWSCROLLBARINFO* pData;
	
        pData = (MWSCROLLBARINFO *)pWin->userdata;

	if (!strcmp(pWin->pClass->lpszClassName,"SCROLLBAR"))
		return pData;
	else
    		return NULL;
}

BOOL EnableScrollBarEx (HWND hWnd, int iSBar, BOOL bEnable)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    BOOL bPrevState;
    RECT rcBar;
    
    DWORD dwStyle;	/* jmt:2k0820 */

    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;

    bPrevState = !(pSBar->status & SBS_DISABLED);

    if (bEnable && !bPrevState)
        pSBar->status &= ~SBS_DISABLED;
    else if (!bEnable && bPrevState)
        pSBar->status |= SBS_DISABLED;
    else
        return FALSE;

    dwStyle = (GetWindowStyle (hWnd) & SBS_TYPEMASK);	/* jmt: 2k0820 */

    if (dwStyle == SBS_VERT)
    {
        wndGetVScrollBarRect (pWin, &rcBar);
        rcBar.left --;
        rcBar.right --;
    }
    else
    {
        wndGetHScrollBarRect (pWin, &rcBar);
        rcBar.top  --;
        rcBar.bottom --;
    }
#if 0
    SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
#else
    MwPaintScrollbars(hWnd,NULL,dwStyle);	/* a must */
#endif

    return TRUE;
}

BOOL GetScrollPosEx (HWND hWnd, int iSBar, int* pPos)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    
    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;

    *pPos = pSBar->curPos;
    return TRUE;
}

BOOL GetScrollRangeEx (HWND hWnd, int iSBar, int* pMinPos, int* pMaxPos)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    
    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;

    *pMinPos = pSBar->minPos;
    *pMaxPos = pSBar->maxPos;
    return TRUE;
}

BOOL SetScrollPosEx (HWND hWnd, int iSBar, int iNewPos)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    RECT rcBar;
    
    DWORD dwStyle;	/* jmt:2k0820 */

    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;

    if (iNewPos < pSBar->minPos)
        pSBar->curPos = pSBar->minPos;
    else
        pSBar->curPos = iNewPos;

    {
        int max = pSBar->maxPos;
        max -= ((pSBar->pageStep - 1) > 0)?(pSBar->pageStep - 1):0;

        if (pSBar->curPos > max)
            pSBar->curPos = max;
    }
    
    dwStyle = (GetWindowStyle (hWnd) & SBS_TYPEMASK);	/* jmt: 2k0820 */

    if (dwStyle == SBS_VERT)
    {
        wndGetVScrollBarRect (pWin, &rcBar);
        rcBar.left --;
        rcBar.right --;
    }
    else
    {
        wndGetHScrollBarRect (pWin, &rcBar);
        rcBar.top  --;
        rcBar.bottom --;
    }

    wndScrollBarPos (pWin, dwStyle == SBS_HORZ, &rcBar);

#if 0
    SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
#else
    MwPaintScrollbars(hWnd,NULL,dwStyle);	/* a must */
#endif
    return TRUE;
}

BOOL SetScrollRangeEx (HWND hWnd, int iSBar, int iMinPos, int iMaxPos)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    RECT rcBar;
    
    DWORD dwStyle;	/* jmt:2k0820 */

    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;

    pSBar->minPos = (iMinPos < iMaxPos)?iMinPos:iMaxPos;
    pSBar->maxPos = (iMinPos > iMaxPos)?iMinPos:iMaxPos;
    
    /* validate parameters. */
    if (pSBar->curPos < pSBar->minPos)
        pSBar->curPos = pSBar->minPos;

    if (pSBar->pageStep <= 0)
        pSBar->pageStep = 0;
    else if (pSBar->pageStep > (pSBar->maxPos - pSBar->minPos + 1))
        pSBar->pageStep = pSBar->maxPos - pSBar->minPos + 1;
    
    {
        int max = pSBar->maxPos;
        max -= ((pSBar->pageStep - 1) > 0)?(pSBar->pageStep - 1):0;

        if (pSBar->curPos > max)
            pSBar->curPos = max;
    }

    dwStyle = (GetWindowStyle (hWnd) & SBS_TYPEMASK);	/* jmt: 2k0820 */

    if (dwStyle == SBS_VERT)
    {
        wndGetVScrollBarRect (pWin, &rcBar);
    	rcBar.left --;
      	rcBar.right --;
    }
    else
    {
        wndGetHScrollBarRect (pWin, &rcBar);
    	rcBar.top  --;
    	rcBar.bottom --;
    }
    wndScrollBarPos (pWin, dwStyle == SBS_HORZ, &rcBar);

#if 0
    SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
#else
    MwPaintScrollbars(hWnd,NULL,dwStyle);	/* a must */
#endif

    return TRUE;
}

BOOL SetScrollInfoEx (HWND hWnd, int iSBar, 
                LPCSCROLLINFO lpsi, BOOL fRedraw)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    RECT rcBar;
    
    DWORD dwStyle;	/* jmt:2k0820 */

    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;
        
    if( lpsi->fMask & SIF_RANGE )
    {
        pSBar->minPos = (lpsi->nMin < lpsi->nMax)?lpsi->nMin:lpsi->nMax;
        pSBar->maxPos = (lpsi->nMin < lpsi->nMax)?lpsi->nMax:lpsi->nMin;
    }
    
    if( lpsi->fMask & SIF_POS )
        pSBar->curPos = lpsi->nPos;
    
    if( lpsi->fMask & SIF_PAGE )
        pSBar->pageStep = lpsi->nPage;

    /* validate parameters. */
    if (pSBar->curPos < pSBar->minPos)
        pSBar->curPos = pSBar->minPos;

    if (pSBar->pageStep <= 0)
        pSBar->pageStep = 0;
    else if (pSBar->pageStep > (pSBar->maxPos - pSBar->minPos + 1))
        pSBar->pageStep = pSBar->maxPos - pSBar->minPos + 1;
    
    {
        int max = pSBar->maxPos;
        max -= ((pSBar->pageStep - 1) > 0)?(pSBar->pageStep - 1):0;

        if (pSBar->curPos > max)
            pSBar->curPos = max;
    }

    dwStyle = (GetWindowStyle (hWnd) & SBS_TYPEMASK);	/* jmt: 2k0820 */

    if(fRedraw)
    {
        if (dwStyle == SBS_VERT)
	{
            wndGetVScrollBarRect (pWin, &rcBar);
            rcBar.left --;
            rcBar.right --;
	}
        else
	{
            wndGetHScrollBarRect (pWin, &rcBar);
            rcBar.top --;
            rcBar.bottom --;
	}
        wndScrollBarPos (pWin, dwStyle == SBS_HORZ, &rcBar);

#if 0
        SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
#else
	MwPaintScrollbars(hWnd,NULL,dwStyle);	/* a must */
#endif
    }
    
    return TRUE;
}

BOOL GetScrollInfoEx(HWND hWnd, int iSBar, LPSCROLLINFO lpsi)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    
    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;
        
    if( lpsi->fMask & SIF_RANGE )
    {
        lpsi->nMin = pSBar->minPos;
        lpsi->nMax = pSBar->maxPos;
    }
    
    if( lpsi->fMask & SIF_POS )
    {
        lpsi->nPos = pSBar->curPos;
    }
    
    if( lpsi->fMask & SIF_PAGE )
        lpsi->nPage = pSBar->pageStep;
    
    return TRUE;
}

BOOL ShowScrollBarEx (HWND hWnd, int iSBar, BOOL bShow)	/* jmt: iSBar not used */
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    BOOL bPrevState;
    RECT rcBar;
    
    DWORD dwStyle;	/* jmt:2k0820 */

    pWin = (HWND)hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin)) )
        return FALSE;

    bPrevState = !(pSBar->status & SBS_HIDE);

    if (bShow && !bPrevState)
        pSBar->status &= ~SBS_HIDE;
    else if (!bShow && bPrevState)
        pSBar->status |= SBS_HIDE;
    else
        return FALSE;

#if 0	/* fix: no WM_CHANGESIZE */
    SendMessage (hWnd, WM_CHANGESIZE, 0, 0);
#endif

    dwStyle = (GetWindowStyle (hWnd) & SBS_TYPEMASK);	/* jmt: 2k0820 */

    if (dwStyle == SBS_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);

    {
        RECT rcWin, rcClient;
        
        memcpy (&rcWin, &pWin->winrect.left, sizeof (RECT));
        
        rcClient.left = 0;
        rcClient.top  = 0;
        rcClient.right = pWin->clirect.right - pWin->clirect.left;
        rcClient.bottom = pWin->clirect.bottom - pWin->clirect.top;
#if 0	/* fix: no WM_SIZECHANGED */
        SendMessage (hWnd, WM_SIZECHANGED, 
            (WPARAM)&rcWin, (LPARAM)&rcClient);
#endif
    }
    
    if (bShow) {
        SendMessage (hWnd, WM_NCPAINT, 0, 0);
    }
    else {
        rcBar.left -= pWin->clirect.left;
        rcBar.top  -= pWin->clirect.top;
        rcBar.right -= pWin->clirect.left;
        rcBar.bottom -= pWin->clirect.top;
        SendMessage (hWnd, WM_NCPAINT, 0, 0);
        InvalidateRect (hWnd, &rcBar, TRUE);
    }

    return TRUE;
}

static void sbSetScrollInfo (HWND hwnd, PMWSCROLLBARINFO pData, BOOL fRedraw)	/* jmt:2k0820 */
{
    SCROLLINFO si;

    int itemCount,itemVisibles;

    itemCount = pData->maxPos - pData->minPos + 1;
    itemVisibles = pData->pageStep;

    if (itemVisibles >= itemCount) 
    {
	SetScrollPosEx (hwnd, 0, 0);	/* jmt: arg2 not used */
	EnableScrollBarEx (hwnd, 0, FALSE);	/* jmt: arg2 not used */
        return;
    }
    
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMax = itemCount - 1;
    si.nMin = 0;

    si.nPage = itemVisibles;	/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

    si.nPos = pData->curPos;

    SetScrollInfoEx (hwnd, 0, &si, fRedraw);	/* jmt: arg2 not used */
    EnableScrollBarEx (hwnd, 0, TRUE);	/* jmt: arg2 not used */
}

static LRESULT CALLBACK
ScrollbarControlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)	/* jmt:2k0820 */
{
    	DWORD dwStyle;
    	MWSCROLLBARINFO* pData;

	int moveRange;
	RECT rcBar;

	dwStyle = (GetWindowStyle (hwnd) & SBS_TYPEMASK);
    	switch (message) 
	{
        case WM_CREATE:
            	if (!(pData = malloc (sizeof (MWSCROLLBARINFO)))) 
		{
                	fprintf(stderr, "Create scroll bar control failure!\n");
                	return -1;
            	}

		pData->minPos=0;           /* min value of scroll range.*/
		/* max value of scroll range.*/
		pData->maxPos=0;     
		if  (dwStyle==SBS_VERT)
			moveRange=((hwnd->winrect.bottom-hwnd->winrect.top)
				-((hwnd->winrect.right-hwnd->winrect.left)<<1));
		else
			moveRange=((hwnd->winrect.right-hwnd->winrect.left)
				-((hwnd->winrect.bottom-hwnd->winrect.top)<<1));
		if (moveRange > MWM_MINBARLEN)
		{


			pData->maxPos=moveRange / MWM_MINBARLEN;
			if( (moveRange % MWM_MINBARLEN) )
				pData->maxPos++;
		}     
		printf("maxPos=%d\n",pData->maxPos);

		pData->curPos=0;             /* current scroll pos.*/

		/* steps per page.*/
		pData->pageStep=1;
		if ( (pData->maxPos - 2) > 1)
			pData->pageStep = pData->maxPos - 2;
		printf("pageStep=%d\n",pData->pageStep);

		pData->barStart=0;           /* start pixel of bar.*/
		pData->barLen=MWM_MINBARLEN; /* length of bar.*/
		pData->status=SBS_UNKNOWN;   /* status of scroll bar.*/
#if 0	/* jmt: must handle WM_MOVE */
		pData->rc=hwnd->winrect;   /* screen coordinates position*/
#endif
            	hwnd->userdata = (DWORD)pData;

		if (dwStyle == SBS_VERT)
		{
		    wndGetVScrollBarRect (hwnd, &rcBar);
		    rcBar.left --;
		    rcBar.right --;
		}
		else
		{
		    wndGetHScrollBarRect (hwnd, &rcBar);
		    rcBar.top --;
		    rcBar.bottom --;
		}
		/* adjust pData->barLen */
		wndScrollBarPos (hwnd, dwStyle == SBS_HORZ, &rcBar);

        	break;
            
        case WM_DESTROY: 
            	free ((void *)(hwnd->userdata));
            	break;

        case WM_PAINT:
		MwPaintScrollbars(hwnd,NULL,dwStyle);
            	break;
	
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCMOUSEMOVE:
	case WM_NCLBUTTONUP:
		MwHandleMessageScrollbar(hwnd, wParam, lParam, message, dwStyle);
		break;

        case WM_HSCROLL:
        case WM_VSCROLL:
        {
            int newTop,itemCount,itemVisibles;

	    pData = (MWSCROLLBARINFO *)hwnd->userdata;
            newTop = pData->curPos;
	    itemCount = pData->maxPos - pData->minPos + 1;
	    itemVisibles = pData->pageStep;

            switch(wParam)
            {
                case SB_LINEDOWN:
#define ITEM_BOTTOM(x)  (x->curPos + itemVisibles - 1)
                    if (ITEM_BOTTOM (pData) < (itemCount - 1 )) 
		    {
                        newTop ++;
                    }
                break;
                
                case SB_LINEUP:
                    if (pData->curPos > 0) 
		    {
                        newTop --;
                    }
                break;
                
                case SB_PAGEDOWN:
                    if ((pData->curPos + (itemVisibles << 1)) <=
                            itemCount)
                        newTop += itemVisibles;
                    else
                        newTop = itemCount - itemVisibles;

                    if (newTop < 0)
                        return 0;

                break;

                case SB_PAGEUP:
                    if (pData->curPos >= itemVisibles)
                        newTop -= itemVisibles;
                    else
                        newTop = 0;

                break;

                case SB_THUMBTRACK:
                    newTop = (int)lParam;
                break;
            }
            
            pData->curPos = newTop;

            SendMessage (hwnd, WM_PAINT, 0, 0);

            sbSetScrollInfo (hwnd, pData, TRUE);

            return 0;
        }
        break;

        default:
    		return DefWindowProc (hwnd, message, wParam, lParam);
    	}
    	return 0;
}

