/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1999, 2000, Wei Yongming.
 * jmt: scrollbar thumb ported
 *
 * Microwindows win32 NonClient Scrollbars
 */
#define MWINCLUDECOLORS
#include "windows.h"
#include "wintern.h"
#include "wintools.h"
#include <string.h>

#define MWM_DEFBARLEN	18		/* default scrollbar length*/
#define MWM_MINBARLEN	8		/* min scrollbar length*/

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

#define HSCROLLBARVISIBLE(hwnd)	((hwnd)->style & WS_HSCROLL)
#define VSCROLLBARVISIBLE(hwnd)	((hwnd)->style & WS_VSCROLL)

/* 
 * Adjust client area smaller if scrollbars visible.
 * Also, compute the NC hittest regions for the scrollbars.
 */
void
MwAdjustNCScrollbars(HWND hwnd)
{
	BOOL	vertbar = VSCROLLBARVISIBLE(hwnd);
	BOOL	horzbar = HSCROLLBARVISIBLE(hwnd);

	if (vertbar) {
		hwnd->clirect.right -= mwSYSMETRICS_CXVSCROLL;
		if (horzbar)
			hwnd->clirect.bottom -= mwSYSMETRICS_CYHSCROLL;
		hwnd->vscroll.rc.left = hwnd->clirect.right;
		hwnd->vscroll.rc.right = hwnd->clirect.right +
			mwSYSMETRICS_CXVSCROLL;
		hwnd->vscroll.rc.top = hwnd->clirect.top;
		hwnd->vscroll.rc.bottom = hwnd->clirect.bottom;
	} else
		SetRectEmpty(&hwnd->vscroll.rc);
	if (horzbar) {
		if (!vertbar)
			hwnd->clirect.bottom -= mwSYSMETRICS_CYHSCROLL;
		hwnd->hscroll.rc.top = hwnd->clirect.bottom;
		hwnd->hscroll.rc.bottom = hwnd->clirect.bottom +
			mwSYSMETRICS_CYHSCROLL;
		hwnd->hscroll.rc.left = hwnd->clirect.left;
		hwnd->hscroll.rc.right = hwnd->clirect.right;
	} else
		SetRectEmpty(&hwnd->hscroll.rc);
}

static BOOL
wndGetVScrollBarRect (HWND hwnd, RECT* rcVBar)
{
    if (hwnd->style & WS_VSCROLL) {
    	*rcVBar = hwnd->vscroll.rc;

        if (hwnd->style & WS_HSCROLL && !(hwnd->hscroll.status & SBS_HIDE))
            rcVBar->bottom -= mwSYSMETRICS_CYHSCROLL;
        
        return TRUE;
    }
    return FALSE;
}

static BOOL
wndGetHScrollBarRect (HWND hwnd, RECT* rcHBar)
{
    if (hwnd->style & WS_HSCROLL) {
    	*rcHBar = hwnd->hscroll.rc;

        if (hwnd->style & WS_VSCROLL && !(hwnd->vscroll.status & SBS_HIDE))
            rcHBar->right -= mwSYSMETRICS_CXVSCROLL;

        return TRUE;
    }
    return FALSE;
}

void
MwPaintNCScrollbars(HWND hwnd, HDC hdc)
{
	BOOL	vertbar = VSCROLLBARVISIBLE(hwnd);
	BOOL	horzbar = HSCROLLBARVISIBLE(hwnd);
	BOOL	fGotDC = FALSE;
	RECT	rc,rc2;
	POINT	p3[3];
	int	shrink=2;
	int start = 0;
	RECT rcHBar, rcVBar;

	if (!hdc && (horzbar || vertbar)) {
		hdc = GetWindowDC(hwnd);
		fGotDC = TRUE;
	}

	if (horzbar && vertbar) {
		rc.left = hwnd->clirect.right;
		rc.top = hwnd->clirect.bottom;
		rc.right = rc.left + mwSYSMETRICS_CXVSCROLL;
		rc.bottom = rc.top + mwSYSMETRICS_CYHSCROLL;
		FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE+1));
	}

	if (vertbar) {
		rc = hwnd->vscroll.rc;

		/* bkgnd */
		rc2.left=rc.left; rc2.right=rc2.left+ mwSYSMETRICS_CXVSCROLL;
		rc2.top=rc.top;
		rc2.bottom=rc2.top+ mwSYSMETRICS_CYHSCROLL;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));
		rc2.top=rc.bottom- mwSYSMETRICS_CYHSCROLL;
		rc2.bottom=rc2.top+ mwSYSMETRICS_CYHSCROLL;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));

		/* up */
		Draw3dUpDownState(hdc, rc.left, rc.top, mwSYSMETRICS_CXVSCROLL, mwSYSMETRICS_CYHSCROLL,
			hwnd->vscroll.status & SBS_UPARROW);

		/* down */
		Draw3dUpDownState(hdc, rc.left,rc.bottom-mwSYSMETRICS_CYHSCROLL, mwSYSMETRICS_CXVSCROLL, mwSYSMETRICS_CYHSCROLL,
			hwnd->vscroll.status & SBS_DOWNARROW);

		/* jmt: draw arrows */
		SelectObject(hdc,GetStockObject(BLACK_BRUSH));
		/* up */
		p3[0].x= rc.left + (mwSYSMETRICS_CXVSCROLL/2) - 1;
		p3[0].y= rc.top + 2 + shrink;
		p3[1].x= rc.left + 2 + shrink - 1;
		p3[1].y= rc.top + (mwSYSMETRICS_CYHSCROLL-4) - shrink;
		p3[2].x= rc.left + (mwSYSMETRICS_CXVSCROLL-4) - shrink;
		p3[2].y= rc.top + (mwSYSMETRICS_CYHSCROLL-4) - shrink;
		Polygon(hdc,p3,3);

		/* down */
		p3[0].x= rc.left + (mwSYSMETRICS_CXVSCROLL/2) - 1;
		p3[0].y= rc.bottom - 4 - shrink;
		p3[1].x= rc.left + 2 + shrink - 1;
		p3[1].y= rc.bottom-mwSYSMETRICS_CYHSCROLL + 2 + shrink;
		p3[2].x= rc.left + (mwSYSMETRICS_CXVSCROLL-4) - shrink;
		p3[2].y= rc.bottom-mwSYSMETRICS_CYHSCROLL + 2 + shrink;
		Polygon(hdc,p3,3);

        	/* draw moving bar */
    		wndGetVScrollBarRect (hwnd, &rcVBar);
        	start = rcVBar.top + mwSYSMETRICS_CYVSCROLL + hwnd->vscroll.barStart;
        	if (start + hwnd->vscroll.barLen > rcVBar.bottom)
            		start = rcVBar.bottom - hwnd->vscroll.barLen;
		if (hwnd->vscroll.barLen == 0)
			hwnd->vscroll.barLen=rc.bottom-rc.top-(mwSYSMETRICS_CYVSCROLL*2); 
		
		/* bkgnd */
		rc2.left=rc.left; rc2.right=rc.right/*-1*/;
		rc2.top=rc.top+mwSYSMETRICS_CYHSCROLL;
		rc2.bottom=start;
		if (rc2.bottom>rc2.top)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

		rc2.top=start+hwnd->vscroll.barLen;
		rc2.bottom=rc.bottom-mwSYSMETRICS_CYHSCROLL;
		if (rc2.bottom>rc2.top)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

        	Draw3dUpFrame (hdc, rcVBar.left, start, rcVBar.right, start + hwnd->vscroll.barLen);
		/*DPRINTF("barv:(l,t,r,b):(%d,%d,%d,%d)\n", rcVBar.left, start, rcVBar.right,
	    		start + hwnd->vscroll.barLen);*/
	}

	if (horzbar) {
		rc = hwnd->hscroll.rc;

		/* bkgnd */
		rc2.top=rc.top; rc2.bottom=rc2.top+ mwSYSMETRICS_CYHSCROLL;
		rc2.left=rc.left;
		rc2.right=rc2.left+ mwSYSMETRICS_CXVSCROLL;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));
		rc2.left=rc.right- mwSYSMETRICS_CXVSCROLL;
		rc2.right=rc2.left+ mwSYSMETRICS_CXVSCROLL;
		FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE+1));

		/* left */
		Draw3dUpDownState(hdc, rc.left, rc.top, mwSYSMETRICS_CXVSCROLL, mwSYSMETRICS_CYHSCROLL,
			hwnd->hscroll.status & SBS_LEFTARROW);

		/* right */
		Draw3dUpDownState(hdc, rc.right-mwSYSMETRICS_CXVSCROLL, rc.top, mwSYSMETRICS_CXVSCROLL, mwSYSMETRICS_CYHSCROLL,
			hwnd->hscroll.status & SBS_RIGHTARROW);

		/* jmt: draw arrows */
		SelectObject(hdc,GetStockObject(BLACK_BRUSH));
		/* left */
		p3[0].x= rc.left + 2 + shrink;
		p3[0].y= rc.top + (mwSYSMETRICS_CYHSCROLL/2) ;
		p3[1].x= rc.left + (mwSYSMETRICS_CXVSCROLL-4) - shrink ;
		p3[1].y= rc.top + 2 + shrink;
		p3[2].x= rc.left + (mwSYSMETRICS_CXVSCROLL-4) - shrink;
		p3[2].y= rc.bottom - 4 - shrink + 1;
		Polygon(hdc,p3,3);

		/* right */
		p3[0].x= rc.right - 4 - shrink;
		p3[0].y= rc.top + (mwSYSMETRICS_CYHSCROLL/2) ;
		p3[1].x= rc.right-mwSYSMETRICS_CXVSCROLL + 2 + shrink ;
		p3[1].y= rc.top + 2 + shrink;
		p3[2].x= rc.right-mwSYSMETRICS_CXVSCROLL + 2 + shrink;
		p3[2].y= rc.bottom - 4 - shrink + 1;
		Polygon(hdc,p3,3);

        	/* draw moving bar. */
    		wndGetHScrollBarRect (hwnd, &rcHBar);

        	start = rcHBar.left + mwSYSMETRICS_CXHSCROLL + hwnd->hscroll.barStart;

        	if (start + hwnd->hscroll.barLen > rcHBar.right)
            		start = rcHBar.right - hwnd->hscroll.barLen;

		if (hwnd->hscroll.barLen == 0)
			hwnd->hscroll.barLen=rc.right-rc.left-(mwSYSMETRICS_CXHSCROLL*2); 

		/* bkgnd */
		rc2.top=rc.top; rc2.bottom=rc.bottom/*-1*/;
		rc2.left=rc.left+mwSYSMETRICS_CXVSCROLL;
		rc2.right=start;
		if (rc2.right>rc2.left)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

		rc2.left=start+hwnd->hscroll.barLen;
		rc2.right=rc.right-mwSYSMETRICS_CXVSCROLL;
		if (rc2.right>rc2.left)
			FillRect(hdc, &rc2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));   

        	Draw3dUpFrame (hdc, start, rcHBar.top, start + hwnd->hscroll.barLen, rcHBar.bottom);
		/*DPRINTF("barh:(l,t,r,b):(%d,%d,%d,%d)\n", start, rcHBar.top, start + hwnd->hscroll.barLen,
	    		rcHBar.bottom);*/
	}

	if (fGotDC)
		ReleaseDC(hwnd, hdc);
}

/* handle a non-client message for a scrollbar*/
void
MwHandleNCMessageScrollbar(HWND hwnd, UINT msg, WPARAM hitcode, LPARAM lParam)
{
	int	pos = SBS_UNKNOWN;
	BOOL	vertbar = VSCROLLBARVISIBLE(hwnd);
	BOOL	horzbar = HSCROLLBARVISIBLE(hwnd);
	int *	pStat;
	POINT	pt;
	RECT	rc;
	int itemMoveable,itemCount,itemVisible,moveRange;	/* jmt:2k0819 */
	int moveTop,moveBottom,moveLeft,moveRight;	/* jmt:2k0819 */
	static BOOL bDraw;
    static int downPos = SBS_UNKNOWN;
    static int sbCode;

	POINTSTOPOINT(pt, lParam);
	for (;;) {	/* use for() to allow break statement*/
		if (hitcode == HTVSCROLL && vertbar) 
		{
			pStat = &hwnd->vscroll.status;
			rc = hwnd->vscroll.rc;
			rc.bottom = rc.top + mwSYSMETRICS_CYVSCROLL;
			if (PtInRect(&rc, pt)) 
			{
				pos = SBS_UPARROW;
				break;
			}
			rc.bottom = hwnd->vscroll.rc.bottom;
			rc.top = rc.bottom - mwSYSMETRICS_CYVSCROLL;
			if (PtInRect(&rc, pt)) 
			{
				pos = SBS_DOWNARROW;
				break;
			}
			pos = SBS_VERTTHUMB;
		} else if (hitcode == HTHSCROLL && horzbar) 
		{
			pStat = &hwnd->hscroll.status;
			rc = hwnd->hscroll.rc;
			rc.right = rc.left + mwSYSMETRICS_CXHSCROLL;
			if (PtInRect(&rc, pt)) {
				pos = SBS_LEFTARROW;
				break;
			}
			rc.right = hwnd->hscroll.rc.right;
			rc.left = rc.right - mwSYSMETRICS_CXHSCROLL;
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

	if (msg == WM_NCLBUTTONDOWN || msg == WM_NCLBUTTONDBLCLK)	/* jmt:2k0819 */
		*pStat |= pos;
	else *pStat &= ~pos;

	if (msg == WM_NCLBUTTONDOWN || msg == WM_NCLBUTTONDBLCLK)	/* jmt:2k0819 */
		bDraw=TRUE;

	if (bDraw)
		MwPaintNCScrollbars(hwnd, NULL);

	switch (pos)
	{
	case SBS_UPARROW:
		if (hwnd->vscroll.curPos != hwnd->vscroll.minPos)
			sbCode = SB_LINEUP;
		else return;
		break;
	case SBS_LEFTARROW:
		if (hwnd->hscroll.curPos != hwnd->hscroll.minPos)
			sbCode = SB_LINELEFT;
		else return;
		break;
	case SBS_DOWNARROW:
		if (hwnd->vscroll.curPos != hwnd->vscroll.maxPos)
			sbCode = SB_LINEDOWN;
		else return;
		break;
	case SBS_RIGHTARROW:
		if (hwnd->hscroll.curPos != hwnd->hscroll.maxPos)
			sbCode = SB_LINERIGHT;
		else return;
		break;
	case SBS_VERTTHUMB:
	case SBS_HORZTHUMB:
		sbCode = SB_THUMBTRACK;
		break;
	}

	switch(msg)
	{
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:	/* jmt:2k0819 */
	    downPos = pos;
#if MWCLIENT
	    InvalidateRect(hwnd,NULL,TRUE);
#endif
        break;

        case WM_NCMOUSEMOVE:	/* jmt:2k0819 */
	    if (hitcode == HTVSCROLL && vertbar) 
	    {
		if (sbCode == SB_THUMBTRACK && downPos == SBS_VERTTHUMB)
	    	{
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->vscroll.rc;
			moveTop = rc.top + mwSYSMETRICS_CYVSCROLL;
			moveBottom = hwnd->vscroll.rc.bottom - mwSYSMETRICS_CYVSCROLL;
			moveRange = moveBottom - moveTop;

			itemCount = hwnd->vscroll.maxPos - hwnd->vscroll.minPos + 1;
			itemVisible = hwnd->vscroll.pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			hwnd->vscroll.trackPos = ((pt.y - moveTop) * itemMoveable) / moveRange;
			//DPRINTF("((%d-%d)*%d)/%d=%d\n", pt.y,moveTop,itemMoveable,moveRange,hwnd->vscroll.trackPos);

			if (hwnd->vscroll.trackPos >= hwnd->vscroll.minPos && hwnd->vscroll.trackPos <= hwnd->vscroll.maxPos)
                    SendMessage (hwnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, hwnd->vscroll.trackPos), 0);
            break;
            }
	    }

	    if (hitcode == HTHSCROLL && horzbar) 
	    {
		if (sbCode == SB_THUMBTRACK && downPos == SBS_HORZTHUMB)
			{
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->hscroll.rc;
			moveLeft = rc.left + mwSYSMETRICS_CXHSCROLL;
			moveRight = hwnd->hscroll.rc.right - mwSYSMETRICS_CXHSCROLL;
			moveRange = moveRight - moveLeft;

			itemCount = hwnd->hscroll.maxPos - hwnd->hscroll.minPos + 1;
			itemVisible = hwnd->hscroll.pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			hwnd->hscroll.trackPos = ((pt.x - moveLeft) * itemMoveable) / moveRange;
			//DPRINTF("((%d-%d)*%d)/%d=%d\n",pt.y,moveLeft,itemMoveable,moveRange,hwnd->hscroll.trackPos);
    
			if (hwnd->hscroll.trackPos >= hwnd->hscroll.minPos && hwnd->hscroll.trackPos <= hwnd->hscroll.maxPos)
			    SendMessage (hwnd, WM_HSCROLL, MAKEWPARAM (SB_THUMBTRACK, hwnd->hscroll.trackPos), 0);
			break;
			}
        }
        break;

	case WM_NCLBUTTONUP:	/* jmt:2k0819 */
	    bDraw=FALSE;
	    downPos = SBS_UNKNOWN;

	    if (sbCode==SB_THUMBTRACK)
	    {
		    if (hitcode == HTVSCROLL && vertbar) 
		    {
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->vscroll.rc;
			moveTop = rc.top + mwSYSMETRICS_CYVSCROLL;
			moveBottom = hwnd->vscroll.rc.bottom - mwSYSMETRICS_CYVSCROLL;
			moveRange = moveBottom - moveTop;

			itemCount = hwnd->vscroll.maxPos - hwnd->vscroll.minPos + 1;
			itemVisible = hwnd->vscroll.pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			hwnd->vscroll.trackPos = ((pt.y - moveTop) * itemMoveable) / moveRange;
			//DPRINTF("((%d-%d)*%d)/%d=%d\n", pt.y,moveTop,itemMoveable,moveRange,hwnd->vscroll.trackPos);

			if ( hwnd->vscroll.trackPos >= hwnd->vscroll.minPos &&
				 hwnd->vscroll.trackPos <= hwnd->vscroll.maxPos)
				SendMessage (hwnd,
					WM_VSCROLL, MAKEWPARAM (SB_THUMBTRACK, hwnd->vscroll.trackPos), 0);
			break;
		    }

		    if (hitcode == HTHSCROLL && horzbar) 
		    {
			/* jmt(2k0819): new algorithm for SB_THUMBTRACK */

			rc = hwnd->hscroll.rc;
			moveLeft = rc.left + mwSYSMETRICS_CXHSCROLL;
			moveRight = hwnd->hscroll.rc.right - mwSYSMETRICS_CXHSCROLL;
			moveRange = moveRight - moveLeft;

			itemCount = hwnd->hscroll.maxPos - hwnd->hscroll.minPos + 1;
			itemVisible = hwnd->hscroll.pageStep;
			itemMoveable = itemCount - itemVisible + 1;

			hwnd->hscroll.trackPos = ((pt.x - moveLeft) * itemMoveable) / moveRange;
			//DPRINTF("((%d-%d)*%d)/%d=%d\n",pt.y,moveLeft,itemMoveable,moveRange,hwnd->hscroll.trackPos);
    
			if ( hwnd->hscroll.trackPos >= hwnd->hscroll.minPos &&
					hwnd->hscroll.trackPos <= hwnd->hscroll.maxPos)
				SendMessage (hwnd, WM_HSCROLL, MAKEWPARAM (SB_THUMBTRACK, hwnd->hscroll.trackPos), 0);
			break;
		    }
	     }
	     else	/* jmt:2k0820 */
	     {
		    if (hitcode == HTVSCROLL && vertbar) 
			SendMessage (hwnd, WM_VSCROLL, sbCode, 0);

		    if (hitcode == HTHSCROLL && horzbar) 
			SendMessage (hwnd, WM_HSCROLL, sbCode, 0);
	     }
		break;
	}
}

static void
wndScrollBarPos (HWND hwnd, BOOL bIsHBar, RECT* rcBar)
{
    int moveRange;
    PMWSCROLLBARINFO pSBar;

    if (bIsHBar)
        pSBar = &hwnd->hscroll;
    else
        pSBar = &hwnd->vscroll;

    if (pSBar->minPos == pSBar->maxPos)
    {
        pSBar->status |= SBS_HIDE;
        return;
    }

    if (bIsHBar)
        moveRange = (rcBar->right - rcBar->left) - (mwSYSMETRICS_CXHSCROLL<<1);
	else
        moveRange = (rcBar->bottom - rcBar->top) - (mwSYSMETRICS_CYVSCROLL<<1);
	if (moveRange < 0) moveRange = 0;

    if (pSBar->pageStep == 0) 
    {
        pSBar->barLen = MWM_DEFBARLEN;
        if (pSBar->barLen > moveRange)
            pSBar->barLen = MWM_MINBARLEN;
    }
    else 
    {
        pSBar->barLen = moveRange * pSBar->pageStep / (pSBar->maxPos - pSBar->minPos + 1);
        if (pSBar->barLen < MWM_MINBARLEN)
            pSBar->barLen = MWM_MINBARLEN;
    }

    pSBar->barStart = moveRange * (pSBar->curPos - pSBar->minPos) / (pSBar->maxPos - pSBar->minPos + 1);
    if (pSBar->barStart + pSBar->barLen > moveRange)
        pSBar->barStart = moveRange - pSBar->barLen;
    if (pSBar->barStart < 0)
        pSBar->barStart = 0;
}

static PMWSCROLLBARINFO wndGetScrollBar (HWND pWin, int iSBar)
{
    if (iSBar == SB_HORZ) {
        if (pWin->style & WS_HSCROLL)
            return &pWin->hscroll;
    }
    else if (iSBar == SB_VERT) {
        if (pWin->style & WS_VSCROLL)
            return &pWin->vscroll;
    }

    return NULL;
}

BOOL WINAPI
EnableScrollBar (HWND hWnd, int iSBar, BOOL bEnable)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    BOOL bPrevState;
    RECT rcBar;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    bPrevState = !(pSBar->status & SBS_DISABLED);

    if (bEnable && !bPrevState)
        pSBar->status &= ~SBS_DISABLED;
    else if (!bEnable && bPrevState)
        pSBar->status |= SBS_DISABLED;
    else
        return FALSE;

    if (iSBar == SB_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);
    //SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
    MwPaintNCScrollbars(hWnd,NULL);	/* a must */

    return TRUE;
}

BOOL WINAPI
GetScrollPos (HWND hWnd, int iSBar, int* pPos)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    *pPos = pSBar->curPos;
    return TRUE;
}

BOOL WINAPI
GetScrollRange (HWND hWnd, int iSBar, int* pMinPos, int* pMaxPos)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    *pMinPos = pSBar->minPos;
    *pMaxPos = pSBar->maxPos;
    return TRUE;
}

BOOL WINAPI
SetScrollPos (HWND hWnd, int iSBar, int iNewPos)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    RECT rcBar;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
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
    
    if (iSBar == SB_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);
    wndScrollBarPos (pWin, iSBar == SB_HORZ, &rcBar);

    //SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
    MwPaintNCScrollbars(hWnd,NULL);	/* a must */
    return TRUE;
}

BOOL WINAPI
SetScrollRange (HWND hWnd, int iSBar, int iMinPos, int iMaxPos)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    RECT rcBar;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
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

    if (iSBar == SB_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);
    wndScrollBarPos (pWin, iSBar == SB_HORZ, &rcBar);

    //SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
    MwPaintNCScrollbars(hWnd,NULL);	/* a must */

    return TRUE;
}

BOOL WINAPI
SetScrollInfo (HWND hWnd, int iSBar, LPCSCROLLINFO lpsi, BOOL fRedraw)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    RECT rcBar;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
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

    if(fRedraw)
    {
        if (iSBar == SB_VERT)
            wndGetVScrollBarRect (pWin, &rcBar);
        else
            wndGetHScrollBarRect (pWin, &rcBar);
        wndScrollBarPos (pWin, iSBar == SB_HORZ, &rcBar);

        //SendMessage (hWnd, WM_NCPAINT, 0, (LPARAM)(&rcBar));
		MwPaintNCScrollbars(hWnd,NULL);			/* a must */
    }
    return TRUE;
}

BOOL WINAPI
GetScrollInfo(HWND hWnd, int iSBar, LPSCROLLINFO lpsi)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
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
    
    if( lpsi->fMask & SIF_TRACKPOS )
    	lpsi->nTrackPos = pSBar->trackPos;

    return TRUE;
}

BOOL WINAPI
ShowScrollBar (HWND hWnd, int iSBar, BOOL bShow)
{
    PMWSCROLLBARINFO pSBar;
    HWND pWin;
    BOOL bPrevState;
    RECT rcBar;
    
    pWin = hWnd;
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    bPrevState = !(pSBar->status & SBS_HIDE);

    if (bShow && !bPrevState)
        pSBar->status &= ~SBS_HIDE;
    else if (!bShow && bPrevState)
        pSBar->status |= SBS_HIDE;
    else
        return FALSE;

    /* fix: no WM_CHANGESIZE */
    //SendMessage (hWnd, WM_CHANGESIZE, 0, 0);

    if (iSBar == SB_VERT)
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

		/* fix: no WM_SIZECHANGED */
        //SendMessage(hWnd, WM_SIZECHANGED, (WPARAM)&rcWin, (LPARAM)&rcClient);
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
