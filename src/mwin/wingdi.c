/*
 * Copyright (c) 1999, 2000, 2001 Greg Haerr <greg@censoft.com>
 *
 * Win32 API upper level graphics drawing routines
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include <stdlib.h>
#include <string.h>

#define MAXSYSCOLORS	29	/* # of COLOR_* system colors*/
#define MAXSTOCKOBJECTS	18	/* # of stock objects*/

#if ERASEMOVE
BOOL mwERASEMOVE = TRUE;	/* default XORMOVE repaint algorithm*/
#else
BOOL mwERASEMOVE = FALSE;	/* default ERASEMOVE repaint algorithm*/
#endif

/* cast a pointer to an integer*/
#if DOS_TURBOC
#define PTRTOINT	unsigned long
#else
#define PTRTOINT	unsigned int
#endif

static HDC	cliphdc;	/* current window cliprects*/

/* default bitmap for new DCs*/
static MWBITMAPOBJ default_bitmap = {
	{OBJ_BITMAP, TRUE}, 1, 1, 1, 1, 1, 1
};

static BOOL MwExtTextOut(HDC hdc, int x, int y, UINT fuOptions,
		CONST RECT *lprc, LPCVOID lpszString, UINT cbCount,
		CONST INT *lpDx, int flags);
static int  MwDrawText(HDC hdc, LPCVOID lpString, int nCount, LPRECT lpRect,
		UINT uFormat, int flags);

HDC WINAPI
GetDCEx(HWND hwnd,HRGN hrgnClip,DWORD flags)
{
	HDC	hdc;

	if(!hwnd)		/* handle NULL hwnd => desktop*/
		hwnd = rootwp;

	/* handle private DC's*/
	if(hwnd->owndc && !(flags & DCX_WINDOW))
		return hwnd->owndc;

	/* add caching?*/
	hdc = GdItemNew(struct hdc);
	if(!hdc)
		return NULL;

	hdc->psd = &scrdev;
	hdc->hwnd = hwnd;
	if(flags & DCX_DEFAULTCLIP) {
		flags &= ~DCX_DEFAULTCLIP;
		if(hwnd->style & WS_CLIPSIBLINGS)
			flags |= DCX_CLIPSIBLINGS;
		if(hwnd->style & WS_CLIPCHILDREN)
			flags |= DCX_CLIPCHILDREN;
	}
	hdc->flags = flags;
	hdc->bkmode = OPAQUE;
	hdc->textalign = TA_LEFT | TA_TOP | TA_NOUPDATECP;
	hdc->bkcolor = RGB(255, 255, 255);	/* WHITE*/
	hdc->textcolor = RGB(0, 0, 0);		/* BLACK*/
	hdc->brush = (MWBRUSHOBJ *)GetStockObject(WHITE_BRUSH);
	hdc->pen = (MWPENOBJ *)GetStockObject(BLACK_PEN);
	hdc->font = (MWFONTOBJ *)GetStockObject(SYSTEM_FONT);
#if UPDATEREGIONS
	if(hrgnClip) {
		/* make a copy of passed region*/
		hdc->region = (MWRGNOBJ *)CreateRectRgn(0, 0, 0, 0);
		CombineRgn((HRGN)hdc->region, hrgnClip, NULL, RGN_COPY);
	}
#endif

	/* make default bitmap compatible with scrdev
	 * otherwise problems occur later because selecting
	 * in the default bitmap overwrite planes and bpp
	 * in a memory dc, and thus it becomes incompatible
	 * with scrdev.
	 */
	default_bitmap.planes = scrdev.planes;
	default_bitmap.bpp = scrdev.bpp;
	hdc->bitmap = &default_bitmap;

	hdc->drawmode = R2_COPYPEN;
	hdc->pt.x = 0;
	hdc->pt.y = 0;

	/* assign private DC if CS_OWNDC and not WindowDC*/
	if((hwnd->pClass->style & CS_OWNDC) && !(flags & DCX_WINDOW)) {
		/* must exclude update region due to BeginPaint GetDCEx call*/
		hdc->flags |= DCX_EXCLUDEUPDATE;
		hwnd->owndc = hdc;
	}

	return hdc;
}

HDC WINAPI
GetDC(HWND hwnd)
{
	/*
	 * Exclude update regions when drawing with GetDC.
	 * This is required because some programs use GetDC
	 * when painting outside of BeginPaint/EndPaint, and
	 * the update region is empty then.
	 */
	return GetDCEx(hwnd, NULL, DCX_DEFAULTCLIP|DCX_EXCLUDEUPDATE);
}

HDC WINAPI
GetWindowDC(HWND hwnd)
{
	/* 
	 * Exclude update region for now, since we
	 * don't keep track of non-client update regions yet
	 */
	return GetDCEx(hwnd, NULL,DCX_WINDOW|DCX_DEFAULTCLIP|DCX_EXCLUDEUPDATE);
}

/* free a DC allocated from GetDC*/
int WINAPI 
ReleaseDC(HWND hwnd, HDC hdc)
{
	/* don't delete a memory dc on release*/
	if(!hdc || (hdc->psd->flags&PSF_MEMORY))
		return 0;

	if(hdc == cliphdc)
		cliphdc = NULL;

	/* handle private DC's*/
	if(hdc->hwnd->owndc && !(hdc->flags & DCX_WINDOW))
		return 1;

	DeleteObject((HBRUSH)hdc->brush);
	DeleteObject((HPEN)hdc->pen);
#if 0 /* don't delete font resources on ReleaseDC... use DeleteObject instead*/
	DeleteObject((HFONT)hdc->font);
#endif
	DeleteObject((HRGN)hdc->region);
	/*
	 * We can only select a bitmap in a memory DC,
	 * so bitmaps aren't released except through DeleteDC.
	 */
	DeleteObject((HBITMAP)hdc->bitmap);
	GdItemFree(hdc);
	return 1;
}

/* free a dc allocated from CreateCompatibleDC*/
BOOL WINAPI
DeleteDC(HDC hdc)
{
	/* don't delete a normal dc, only memory dc's*/
	if(!hdc || !(hdc->psd->flags&PSF_MEMORY))
		return 0;

	/* free allocated memory screen device*/
	hdc->psd->FreeMemGC(hdc->psd);

	/* make it look like a GetDC dc, and free it*/
	hdc->psd = &scrdev;
	return ReleaseDC(NULL, hdc);
}

void
MwPaintNCArea(HWND hwnd)
{
	SendMessage(hwnd, WM_NCPAINT, 0, 0L);

	/* for now, we always paint NC scrollbar areas*/
	MwPaintNCScrollbars(hwnd, NULL);
}

HDC WINAPI 
BeginPaint(HWND hwnd, LPPAINTSTRUCT lpPaint)
{
	HDC	hdc;

	/* first update non-client area*/
	if(mwforceNCpaint || hwnd->paintNC != mwpaintNC) {
		MwPaintNCArea(hwnd);
		hwnd->paintNC = mwpaintNC;
	}

	/* If ERASEMOVE:
	 * Don't allow windows to repaint while user is moving
	 * a window.  Instead, just erase backgrounds
	 * and indicate delayed painting required, which
	 * will occur after user completes window move.
	 */
	if(mwERASEMOVE && dragwp) {
		hdc = NULL;
		lpPaint->fErase = !DefWindowProc(hwnd, WM_ERASEBKGND, 0, 0L);
		hwnd->gotPaintMsg = PAINT_DELAYPAINT;
	} else {
		HideCaret(hwnd);

		/* FIXME: mdemo requires update excluded or draw errors occur*/
		hdc = GetDCEx(hwnd, NULL, DCX_DEFAULTCLIP
				|DCX_EXCLUDEUPDATE);	/* FIXME - bug*/

		/* erase client background*/
		lpPaint->fErase = !SendMessage(hwnd, WM_ERASEBKGND, (WPARAM)hdc,
			0L);
	}
	lpPaint->hdc = hdc;

	GetUpdateRect(hwnd, &lpPaint->rcPaint, FALSE);
	return hdc;
}

BOOL WINAPI 
EndPaint(HWND hwnd, CONST PAINTSTRUCT *lpPaint)
{
	ReleaseDC(hwnd, lpPaint->hdc);
#if UPDATEREGIONS
	/* don't clear update region until done dragging*/
	if(mwERASEMOVE && !dragwp)
		GdSetRectRegion(hwnd->update, 0, 0, 0, 0);
#endif
	ShowCaret(hwnd);
	return TRUE;
}

COLORREF WINAPI
SetTextColor(HDC hdc, COLORREF crColor)
{
	COLORREF	oldtextcolor;

	if (!hdc)
		return CLR_INVALID;
	oldtextcolor = hdc->textcolor;
	hdc->textcolor = (MWCOLORVAL)crColor;
	return oldtextcolor;
}

COLORREF WINAPI
SetBkColor(HDC hdc, COLORREF crColor)
{
	COLORREF	oldbkcolor;

	if (!hdc)
		return CLR_INVALID;
	oldbkcolor = hdc->bkcolor;
	hdc->bkcolor = crColor;
	return oldbkcolor;
}

int WINAPI
SetBkMode(HDC hdc, int iBkMode)
{
	int	oldbkmode;

	if(!hdc)
		return 0;
	oldbkmode = hdc->bkmode;
	hdc->bkmode = iBkMode;
	return oldbkmode;
}

UINT WINAPI
SetTextAlign(HDC hdc, UINT fMode)
{
	UINT	oldfMode;

	if(!hdc)
		return GDI_ERROR;
	oldfMode = hdc->textalign;
	hdc->textalign = fMode;
	return oldfMode;
}

/* FIXME: releasing a DC does NOT change back the drawing mode!*/
int WINAPI
SetROP2(HDC hdc, int fnDrawMode)
{
	int	newmode, oldmode;

	if(!hdc || (fnDrawMode <= 0 || fnDrawMode > R2_LAST))
		return 0;

	oldmode = hdc->drawmode;
	newmode = fnDrawMode - 1;	/* map to MWMODE_xxx*/
	hdc->drawmode = newmode;
	GdSetMode(newmode);
	return oldmode;
}

/* 
 * Setup clip region from device context's associated window or bitmap.
 * Memory DC's are always associated with the desktop window, and are
 * always visible.  Return the DC's hwnd if window is visible.
 */
HWND
MwPrepareDC(HDC hdc)
{
	HWND	hwnd;

	if(!hdc || !hdc->hwnd)
		return NULL;

	hwnd = hdc->hwnd;
	if (hwnd->unmapcount)
		return NULL;

	/*
	 * If the window is not the currently clipped one, then
	 * make it the current one and define its clip rectangles.
	 */
	if(hdc != cliphdc) {
		/* clip memory dc's to the bitmap size*/
		if(hdc->psd->flags&PSF_MEMORY) {
#if DYNAMICREGIONS
			GdSetClipRegion(hdc->psd,
				GdAllocRectRegion(0, 0, hdc->psd->xvirtres,
					hdc->psd->yvirtres));
#else
			static MWCLIPRECT crc = {0, 0, 0, 0};

			crc.width = hdc->psd->xvirtres;
			crc.height = hdc->psd->yvirtres;
			GdSetClipRects(hdc->psd, 1, &crc);
#endif
		} else MwSetClipWindow(hdc);
		cliphdc = hdc;
	}

	return hwnd;
}

/* return RGB value at specified coordinates*/
COLORREF WINAPI
GetPixel(HDC hdc, int x, int y)
{
	HWND		hwnd;
	POINT		pt;
	MWPIXELVAL	pixel;
	MWPALENTRY	rgb;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return CLR_INVALID;
	pt.x = x;
	pt.y = y;
	if(MwIsClientDC(hdc))
		ClientToScreen(hwnd, &pt);

	/* read pixel value*/
	GdReadArea(hdc->psd, pt.x, pt.y, 1, 1, &pixel);

	switch(hdc->psd->pixtype) {
	case MWPF_TRUECOLOR0888:
	case MWPF_TRUECOLOR888:
		/* create RGB colorval from 8/8/8 pixel*/
		return PIXEL888TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR565:
		/* create RGB colorval from 5/6/5 pixel*/
		return PIXEL565TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR555:
		/* create RGB colorval from 5/5/5 pixel*/
		return PIXEL555TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR332:
		/* create RGB colorval from 3/3/2 pixel*/
		return PIXEL332TOCOLORVAL(pixel);

	case MWPF_PALETTE:
		if(GdGetPalette(hdc->psd, pixel, 1, &rgb))
			return RGB(rgb.r, rgb.g, rgb.b);
	}
	return CLR_INVALID;
}

COLORREF WINAPI
SetPixel(HDC hdc, int x, int y, COLORREF crColor)
{
	HWND		hwnd;
	POINT		pt;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return 0;	/* doesn't return previous color*/
	pt.x = x;
	pt.y = y;
	if(MwIsClientDC(hdc))
		ClientToScreen(hwnd, &pt);

	/* draw point in passed color*/
	GdSetForeground(GdFindColor(crColor));
	GdPoint(hdc->psd, pt.x, pt.y);
	return 0;		/* doesn't return previous color*/
}

BOOL WINAPI 
MoveToEx(HDC hdc, int x, int y, LPPOINT lpPoint)
{
	if(!hdc)
		return FALSE;
	if(lpPoint)
		*lpPoint = hdc->pt;
	hdc->pt.x = x;
	hdc->pt.y = y;
	return TRUE;
}

BOOL WINAPI
LineTo(HDC hdc, int x, int y)
{
	HWND		hwnd;
	POINT		beg, end;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return FALSE;

	beg.x = hdc->pt.x;
	beg.y = hdc->pt.y;
	end.x = x;
	end.y = y;
	if(MwIsClientDC(hdc)) {
		ClientToScreen(hwnd, &beg);
		ClientToScreen(hwnd, &end);
	}

	/* draw line in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForeground(GdFindColor(hdc->pen->color));
		/* don't draw last point*/
		GdLine(hdc->psd, beg.x, beg.y, end.x, end.y, FALSE);
	}
	hdc->pt.x = x;
	hdc->pt.y = y;
	return TRUE;
}

/* draw line segments by connecting passed points*/
BOOL WINAPI
Polyline(HDC hdc, CONST POINT *lppt, int cPoints)
{
	HWND		hwnd;
	POINT		beg, end;

	if(cPoints <= 1)
		return FALSE;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return FALSE;

	if(hdc->pen->style == PS_NULL)
		return TRUE;

	/* draw line in current pen color*/
	GdSetForeground(GdFindColor(hdc->pen->color));

	beg = *lppt++;
	if(MwIsClientDC(hdc))
		ClientToScreen(hwnd, &beg);
	while(--cPoints > 0) {
		end = *lppt++;
		if(MwIsClientDC(hdc))
			ClientToScreen(hwnd, &end);

		/* don't draw last point*/
		GdLine(hdc->psd, beg.x, beg.y, end.x, end.y, FALSE);

		beg = end;
	}
	return TRUE;
}

BOOL WINAPI
Rectangle(HDC hdc, int nLeft, int nTop, int nRight, int nBottom)
{
	HWND	hwnd;
	RECT	rc;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return FALSE;

	SetRect(&rc, nLeft, nTop, nRight, nBottom);
	if(MwIsClientDC(hdc))
		MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);

	/* draw rectangle in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForeground(GdFindColor(hdc->pen->color));
		GdRect(hdc->psd, rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top);
	}

	/* fill rectangle in current brush color*/
	if(hdc->brush->style != BS_NULL) {
		InflateRect(&rc, -1, -1);
		GdSetForeground(GdFindColor(hdc->brush->color));
		GdFillRect(hdc->psd, rc.left, rc.top, rc.right - rc.left,
			rc.bottom - rc.top);
	}

	return TRUE;
}

BOOL WINAPI
Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	HWND	hwnd;
	int	rx, ry;
	RECT	rc;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return FALSE;

	SetRect(&rc, nLeftRect, nTopRect, nRightRect, nBottomRect);
	if(MwIsClientDC(hdc))
		MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);

	rx = (rc.right - rc.left)/2 - 1;
	ry = (rc.bottom - rc.top)/2 - 1;
	rc.left += rx;
	rc.top += ry;

	/* fill ellipse in current brush color*/
	if(hdc->brush->style != BS_NULL) {
		InflateRect(&rc, -1, -1);
		GdSetForeground(GdFindColor(hdc->brush->color));
		GdEllipse(hdc->psd, rc.left, rc.top, rx, ry, TRUE);
	}

	/* draw ellipse outline in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForeground(GdFindColor(hdc->pen->color));
		GdEllipse(hdc->psd, rc.left, rc.top, rx, ry, FALSE);
	}

	return TRUE;
}

static void
dopiearc(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
	int ax, int ay, int bx, int by, int type)
{
	HWND	hwnd;
	int	rx, ry;
	RECT	rc, rc2;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return;

	SetRect(&rc, nLeftRect, nTopRect, nRightRect, nBottomRect);
	SetRect(&rc2, ax, ay, bx, by);
	if(MwIsClientDC(hdc)) {
		MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);
		MapWindowPoints(hwnd, NULL, (LPPOINT)&rc2, 2);
	}

	rx = (rc.right - rc.left)/2 - 1;
	ry = (rc.bottom - rc.top)/2 - 1;
	rc.left += rx;
	rc.top += ry;

	/* fill ellipse in current brush color*/
	if(hdc->brush->style != BS_NULL && type == MWPIE) {
		GdSetForeground(GdFindColor(hdc->brush->color));
		GdArc(hdc->psd, rc.left, rc.top, rx, ry,
			rc2.left, rc2.top, rc2.right, rc2.bottom, MWPIE);
	}

	/* draw ellipse outline in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForeground(GdFindColor(hdc->pen->color));
		if(type == MWPIE)
			type = MWARC;	/* MWARCOUTLINE?*/
		GdArc(hdc->psd, rc.left, rc.top, rx, ry,
			rc2.left, rc2.top, rc2.right, rc2.bottom, type);
	}
}

BOOL WINAPI
Arc(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
	int nXStartArc, int nYStartArc, int nXEndArc, int nYEndArc)
{
	dopiearc(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect,
		nXStartArc, nYStartArc, nXEndArc, nYEndArc, MWARC);
	return TRUE;
}

BOOL WINAPI
Pie(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
	int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2)
{
	dopiearc(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect,
		nXRadial1, nYRadial1, nXRadial2, nYRadial2, MWPIE);
	return TRUE;
}

BOOL WINAPI
Polygon(HDC hdc, CONST POINT *lpPoints, int nCount)
{
	HWND	hwnd;
	int	i;
	LPPOINT	pp, ppAlloc = NULL;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return FALSE;

	if(MwIsClientDC(hdc)) {
		/* convert points to client coords*/
		ppAlloc = (LPPOINT)malloc(nCount * sizeof(POINT));
		if(!ppAlloc)
			return FALSE;
		memcpy(ppAlloc, lpPoints, nCount*sizeof(POINT));
		pp = ppAlloc;
		for(i=0; i<nCount; ++i)
			ClientToScreen(hwnd, pp++);
		pp = ppAlloc;
	} else pp = (LPPOINT)lpPoints;

	/* fill polygon in current brush color*/
	if(hdc->brush->style != BS_NULL) {
		GdSetForeground(GdFindColor(hdc->brush->color));
		GdFillPoly(hdc->psd, nCount, pp);
	}

	/* draw polygon outline in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForeground(GdFindColor(hdc->pen->color));
		GdPoly(hdc->psd, nCount, pp);
	}

	if(ppAlloc)
		free(ppAlloc);
	return TRUE;
}

/* draw nCount polygons*/
BOOL WINAPI
PolyPolygon(HDC hdc, CONST POINT *lpPoints, LPINT lpPolyCounts, int nCount)
{
	while(--nCount >= 0) {
		if (!Polygon(hdc, lpPoints, *lpPolyCounts))
			return FALSE;
		lpPoints += *lpPolyCounts++;
	}
	return TRUE;
}

int WINAPI
FillRect(HDC hdc, CONST RECT *lprc, HBRUSH hbr)
{
	HWND		hwnd;
	RECT 		rc;
	MWBRUSHOBJ *	obr = (MWBRUSHOBJ *)hbr;
	COLORREF	crFill;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd || !obr)
		return FALSE;

	if(!lprc) {
		if(MwIsClientDC(hdc))
			GetClientRect(hwnd, &rc);
		else
			GetWindowRect(hwnd, &rc);
		lprc = &rc;
	} else
		rc = *lprc;
	if(MwIsClientDC(hdc))
		MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);

	/* handle COLOR_xxx + 1 passed as HBRUSH*/
	if((PTRTOINT)obr <= MAXSYSCOLORS)
		crFill = GetSysColor((int)obr-1);
	else {
		/* get color from passed HBRUSH*/
		if(obr->style == BS_NULL)
			return TRUE;
		crFill = obr->color;
	}

	/* fill rectangle in passed brush color*/
	GdSetForeground(GdFindColor(crFill));
	GdFillRect(hdc->psd, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top);
	return TRUE;
}

/* ascii*/
BOOL WINAPI
TextOut(HDC hdc, int x, int y, LPCSTR lpszString, int cbString)
{
	/* kaffe port wants MWTF_UTF8 here...*/
	return MwExtTextOut(hdc, x, y, 0, NULL, lpszString, cbString, NULL,
		MWTF_ASCII);
}

/* ascii*/
BOOL WINAPI
ExtTextOut(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc,
	LPCSTR lpszString, UINT cbCount, CONST INT *lpDx)
{
	return MwExtTextOut(hdc, x, y, fuOptions, lprc, lpszString,
		cbCount, lpDx, MWTF_ASCII);
}

/* unicode*/
BOOL WINAPI
ExtTextOutW(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc,
	LPCWSTR lpszString, UINT cbCount, CONST INT *lpDx)
{
	return MwExtTextOut(hdc, x, y, fuOptions, lprc, lpszString,
		cbCount, lpDx, MWTF_UC16);
}

/* internal version of ExtTextOut, passed flags for text data type*/
static BOOL
MwExtTextOut(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc,
	LPCVOID lpszString, UINT cbCount, CONST INT *lpDx, int flags)
{
	HWND	hwnd;
	POINT	pt;
	RECT	rc;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return FALSE;

	pt.x = x;
	pt.y = y;
	if(MwIsClientDC(hdc))
		ClientToScreen(hwnd, &pt);

	/* optionally fill passed rectangle*/
	if(lprc && (fuOptions&ETO_OPAQUE)) {
		rc = *lprc;
		if(MwIsClientDC(hdc))
			MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);

		/* fill rectangle with current background color*/
		GdSetForeground(GdFindColor(hdc->bkcolor));
		GdFillRect(hdc->psd, rc.left, rc.top, rc.right - rc.left,
			rc.bottom - rc.top);
		GdSetUseBackground(FALSE);
	} else {
		/* use current background mode for text background draw*/
		GdSetUseBackground(hdc->bkmode == OPAQUE? TRUE: FALSE);
		/* always set background color in case GdArea is
		 * used to draw, which compares gr_foreground != gr_background
		 * if gr_usebg is false...
		 */
		/*if(hdc->bkmode == OPAQUE)*/
			GdSetBackground(GdFindColor(hdc->bkcolor));
	}

	/* nyi: lpDx*/

	/* draw text in current text foreground and background color*/
	GdSetForeground(GdFindColor(hdc->textcolor));
	GdSetFont(hdc->font->pfont);

	/* this whole text alignment thing needs rewriting*/
	if((hdc->textalign & TA_BASELINE) == TA_BASELINE) {
		 /* this is not right... changed for kaffe port
		flags |= MWTF_TOP;
		 */
		flags |= MWTF_BASELINE;
	} else if(hdc->textalign & TA_BOTTOM) {
		MWCOORD	ph, pw, pb;

		if(lprc)
			pt.y += lprc->bottom - lprc->top;
		else {
			GdGetTextSize(hdc->font->pfont, lpszString, cbCount,
				&pw, &ph, &pb, flags);
			pt.y += ph;
		}
		flags |= MWTF_BOTTOM;
	} else
		flags |= MWTF_TOP;
	GdText(hdc->psd, pt.x, pt.y, lpszString, cbCount, flags);

	return TRUE;
}

/* ascii*/
int WINAPI
DrawTextA(HDC hdc, LPCSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
	return MwDrawText(hdc, lpString, nCount, lpRect, uFormat, MWTF_ASCII);
}

/* unicode*/
int WINAPI
DrawTextW(HDC hdc, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
	return MwDrawText(hdc, lpString, nCount, lpRect, uFormat, MWTF_UC16);
}

/* note: many DT_x aren't implemented in this function*/
/* internal version of DrawText, passed flags for text data type*/
static int
MwDrawText(HDC hdc, LPCVOID lpString, int nCount, LPRECT lpRect, UINT uFormat,
	int flags)
{
	MWCOORD	x, y, width, height, baseline;

	if(nCount == -1)
		nCount = strlen(lpString);

	if(uFormat & (DT_CALCRECT|DT_CENTER|DT_RIGHT)) {
		if(!hdc)
			return 0;
		GdGetTextSize(hdc->font->pfont, lpString, nCount,
			&width, &height, &baseline, MWTF_ASCII);
	}
	x = lpRect->left;
	y = lpRect->top;

	if(uFormat & DT_CALCRECT) {
		lpRect->right = x + width;
		lpRect->bottom = y + height;
		return height;
	}

	if(uFormat & DT_CENTER)
		x = (lpRect->left + lpRect->right - width) / 2;
	else if(uFormat & DT_RIGHT)
		x += lpRect->right - width;

	/* draw text at DT_TOP using current fg, bg and bkmode*/
	MwExtTextOut(hdc, x, y, 0, NULL, lpString, nCount, NULL, flags);
	return height;
}

/* Microwindows only*/
BOOL WINAPI
DrawDIB(HDC hdc,int x,int y,PMWIMAGEHDR pimage)
{
	HWND		hwnd;
	POINT		pt;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd || !pimage)
		return FALSE;
	pt.x = x;
	pt.y = y;
	if(MwIsClientDC(hdc))
		ClientToScreen(hwnd, &pt);

	GdDrawImage(hdc->psd, pt.x, pt.y, pimage);
	return TRUE;
}

/* define color scheme: A (tan), B (winstd) or C (old)*/
#if ELKS
#define B
#else
#define A
#endif

#define A_RGB(r,g,b)
#define B_RGB(r,g,b)
#define C_RGB(r,g,b)

#ifdef A
#undef  A_RGB
#define A_RGB(r,g,b)	RGB(r,g,b),
#endif
#ifdef B
#undef  B_RGB
#define B_RGB(r,g,b)	RGB(r,g,b),
#endif
#ifdef C
#undef  C_RGB
#define C_RGB(r,g,b)	RGB(r,g,b),
#endif

static COLORREF sysColors[MAXSYSCOLORS] = {
	RGB(192, 192, 192),	/* COLOR_SCROLLBAR          0*/
	RGB(  0, 128, 128),     /* COLOR_BACKGROUND          */
	A_RGB(128,   0,   0)	/* COLOR_ACTIVECAPTION       */
	B_RGB(128,   0, 128)	/* COLOR_ACTIVECAPTION       */
	C_RGB(128,   0, 128)	/* COLOR_ACTIVECAPTION       */
	A_RGB(162, 141, 104)	/* COLOR_INACTIVECAPTION     */
	B_RGB(128, 128, 128)  	/* COLOR_INACTIVECAPTION     */
	C_RGB(  0,  64, 128)	/* COLOR_INACTIVECAPTION     */
	RGB(192, 192, 192),     /* COLOR_MENU                */
	RGB(255, 255, 255),     /* COLOR_WINDOW             5*/
	RGB(  0,   0,   0),     /* COLOR_WINDOWFRAME         */
	RGB(  0,   0,   0),     /* COLOR_MENUTEXT            */
	RGB(  0,   0,   0),     /* COLOR_WINDOWTEXT          */
	RGB(255, 255, 255),     /* COLOR_CAPTIONTEXT         */
	RGB(192, 192, 192),   	/* COLOR_ACTIVEBORDER      10*/
	RGB(192, 192, 192), 	/* COLOR_INACTIVEBORDER      */
	RGB(128, 128, 128),   	/* COLOR_APPWORKSPACE        */
	RGB(128,   0,   0),     /* COLOR_HIGHLIGHT           */
	RGB(255, 255, 255),     /* COLOR_HIGHLIGHTTEXT       */
	A_RGB(213, 204, 187)	/* COLOR_BTNFACE           15*/
	B_RGB(192, 192, 192)	/* COLOR_BTNFACE           15*/
	C_RGB(160, 160, 160)	/* COLOR_BTNFACE           15*/
	A_RGB(162, 141, 104)	/* COLOR_BTNSHADOW           */
	B_RGB(128, 128, 128)	/* COLOR_BTNSHADOW           */
	C_RGB(128, 128, 128)	/* COLOR_BTNSHADOW           */
	RGB( 64,  64,  64),     /* COLOR_GRAYTEXT            */
	RGB(  0,   0,   0),     /* COLOR_BTNTEXT             */
	RGB(192, 192, 192),    	/* COLOR_INACTIVECAPTIONTEXT */
	A_RGB(234, 230, 221)  	/* COLOR_BTNHIGHLIGHT      20*/
	B_RGB(255, 255, 255)  	/* COLOR_BTNHIGHLIGHT      20*/
	C_RGB(223, 223, 223)  	/* COLOR_BTNHIGHLIGHT      20*/
	RGB(  0,   0,   0),     /* COLOR_3DDKSHADOW          */
	A_RGB(213, 204, 187)	/* COLOR_3DLIGHT             */
	B_RGB(223, 223, 223)	/* COLOR_3DLIGHT             */
	C_RGB(192, 192, 192)	/* COLOR_3DLIGHT             */
	RGB(  0,   0,   0),     /* COLOR_INFOTEXT            */
	RGB(225, 255, 255), 	/* COLOR_INFOBK              */
	RGB(184, 180, 184),  	/* COLOR_ALTERNATEBTNFACE  25*/
	RGB(  0,   0, 255),     /* COLOR_HOTLIGHT 	     */
	RGB( 16, 132, 208),  	/* COLOR_GRADIENTACTIVECAPTION */
	RGB(184, 180, 184) 	/* COLOR_GRADIENTINACTIVECAPTION 28*/
};

DWORD WINAPI
GetSysColor(int nIndex)
{
	if(nIndex >= 0 && nIndex < MAXSYSCOLORS)
		return sysColors[nIndex];
	return 0;
}

COLORREF WINAPI
SetSysColor(int nIndex, COLORREF crColor)	/* Microwindows only*/
{
	COLORREF oldColor;

	if(nIndex >= 0 && nIndex < MAXSYSCOLORS) {
		oldColor = sysColors[nIndex];
		sysColors[nIndex] = crColor;
		return oldColor;
	}
	return 0;
}

static MWBRUSHOBJ OBJ_WHITE_BRUSH = {
	{OBJ_BRUSH, TRUE}, BS_SOLID, RGB(255, 255, 255)
};

static MWBRUSHOBJ OBJ_LTGRAY_BRUSH = {
	{OBJ_BRUSH, TRUE}, BS_SOLID, RGB(192, 192, 192)
};

static MWBRUSHOBJ OBJ_GRAY_BRUSH = {
	{OBJ_BRUSH, TRUE}, BS_SOLID, RGB(128, 128, 128)
};

static MWBRUSHOBJ OBJ_DKGRAY_BRUSH = {
	{OBJ_BRUSH, TRUE}, BS_SOLID, RGB(32, 32, 32)
};

static MWBRUSHOBJ OBJ_BLACK_BRUSH = {
	{OBJ_BRUSH, TRUE}, BS_SOLID, RGB(0, 0, 0)
};

static MWBRUSHOBJ OBJ_NULL_BRUSH = {
	{OBJ_BRUSH, TRUE}, BS_NULL, RGB(0, 0, 0)
};

static MWPENOBJ OBJ_WHITE_PEN = {
	{OBJ_PEN, TRUE}, PS_SOLID, RGB(255, 255, 255)
};

static MWPENOBJ OBJ_BLACK_PEN = {
	{OBJ_PEN, TRUE}, PS_SOLID, RGB(0, 0, 0)
};

static MWPENOBJ OBJ_NULL_PEN = {
	{OBJ_PEN, TRUE}, PS_NULL, RGB(0, 0, 0)
};

static MWFONTOBJ OBJ_OEM_FIXED_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_OEM_FIXED
};

static MWFONTOBJ OBJ_ANSI_FIXED_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_FIXED
};

static MWFONTOBJ OBJ_ANSI_VAR_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_VAR
};

static MWFONTOBJ OBJ_SYSTEM_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_VAR
};

static MWFONTOBJ OBJ_DEVICE_DEFAULT_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_OEM_FIXED
};

static MWFONTOBJ OBJ_SYSTEM_FIXED_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_FIXED
};

static MWFONTOBJ OBJ_DEFAULT_GUI_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_GUI_VAR
};

static struct hgdiobj *stockObjects[MAXSTOCKOBJECTS] = {
	(struct hgdiobj *)&OBJ_WHITE_BRUSH,		/* WHITE_BRUSH*/
	(struct hgdiobj *)&OBJ_LTGRAY_BRUSH,		/* LTGRAY_BRUSH*/
	(struct hgdiobj *)&OBJ_GRAY_BRUSH,		/* GRAY_BRUSH*/
	(struct hgdiobj *)&OBJ_DKGRAY_BRUSH,		/* DKGRAY_BRUSH*/
	(struct hgdiobj *)&OBJ_BLACK_BRUSH,		/* BLACK_BRUSH*/
	(struct hgdiobj *)&OBJ_NULL_BRUSH,		/* NULL_BRUSH*/
	(struct hgdiobj *)&OBJ_WHITE_PEN,		/* WHITE_PEN*/
	(struct hgdiobj *)&OBJ_BLACK_PEN,		/* BLACK_PEN*/
	(struct hgdiobj *)&OBJ_NULL_PEN,		/* NULL_PEN*/
	(struct hgdiobj *)NULL,
	(struct hgdiobj *)&OBJ_OEM_FIXED_FONT,		/* OEM_FIXED_FONT*/
	(struct hgdiobj *)&OBJ_ANSI_FIXED_FONT,		/* ANSI_FIXED_FONT*/
	(struct hgdiobj *)&OBJ_ANSI_VAR_FONT,		/* ANSI_VAR_FONT*/
	(struct hgdiobj *)&OBJ_SYSTEM_FONT,		/* SYSTEM_FONT*/
	(struct hgdiobj *)&OBJ_DEVICE_DEFAULT_FONT,	/* DEVICE_DEFAULT_FONT*/
	(struct hgdiobj *)NULL,				/* DEFAULT_PALETTE*/
	(struct hgdiobj *)&OBJ_SYSTEM_FIXED_FONT,	/* SYSTEM_FIXED_FONT*/
	(struct hgdiobj *)&OBJ_DEFAULT_GUI_FONT		/* DEFAULT_GUI_FONT*/
};

HGDIOBJ WINAPI
GetStockObject(int nObject)
{
	HGDIOBJ		pObj;
	MWFONTOBJ *	pFont;

	if(nObject >= 0 && nObject < MAXSTOCKOBJECTS) {
		pObj = stockObjects[nObject];

		/* create stock fonts on first access*/
		if(pObj->hdr.type == OBJ_FONT) {
			pFont = (MWFONTOBJ *)pObj;
			if(pFont->pfont == NULL) {
				pFont->pfont = GdCreateFont(&scrdev,
					pFont->name, 0, NULL);
			}
			return pObj;
		}

		/* implement multiple color schemes with
		 * standard background brushes...
		 */
		switch(nObject) {
		case LTGRAY_BRUSH:
		case GRAY_BRUSH:
			((MWBRUSHOBJ *)pObj)->color =GetSysColor(COLOR_BTNFACE);
			break;
		case DKGRAY_BRUSH:
			((MWBRUSHOBJ *)pObj)->color =
				GetSysColor(COLOR_BTNSHADOW);
			break;
		}
		return pObj;
	}
	return NULL;
}

HGDIOBJ WINAPI
SelectObject(HDC hdc, HGDIOBJ hObject)
{
	HGDIOBJ		objOrg;
	MWBITMAPOBJ *	pb;

	if(!hdc || !hObject)
		return NULL;

	switch(hObject->hdr.type) {
	case OBJ_PEN:
		objOrg = (HGDIOBJ)hdc->pen;
		hdc->pen = (MWPENOBJ *)hObject;
		break;
	case OBJ_BRUSH:
		objOrg = (HGDIOBJ)hdc->brush;
		hdc->brush = (MWBRUSHOBJ *)hObject;
		break;
	case OBJ_FONT:
		objOrg = (HGDIOBJ)hdc->font;
		hdc->font = (MWFONTOBJ *)hObject;
		break;
	case OBJ_BITMAP:
		/* must be memory dc to select bitmap*/
		if(!(hdc->psd->flags&PSF_MEMORY))
			return NULL;
		objOrg = (HGDIOBJ)hdc->bitmap;

		/* setup mem dc for drawing into bitmap*/
		pb = (MWBITMAPOBJ *)hObject;

		/* init memory context*/
		if (!hdc->psd->MapMemGC(hdc->psd, pb->width, pb->height,
			pb->planes, pb->bpp, pb->linelen, pb->size,
			&pb->bits[0]))
				return NULL;

		hdc->bitmap = (MWBITMAPOBJ *)hObject;
	    	break;
#if UPDATEREGIONS
	case OBJ_REGION:
		/*objOrg = (HGDIOBJ)hdc->region;*/
		objOrg = NULL;	/* FIXME? hdc->region is destroyed below*/
		SelectClipRgn(hdc, (HRGN)hObject);
		break;
#endif
	default:
		return NULL;
	}

	return objOrg;
}

BOOL WINAPI
DeleteObject(HGDIOBJ hObject)
{
	if(!hObject || hObject->hdr.stockobj)
		return FALSE;
	if(hObject->hdr.type == OBJ_FONT)
		GdDestroyFont(((MWFONTOBJ *)hObject)->pfont);
	if(hObject->hdr.type == OBJ_REGION)
		GdDestroyRegion(((MWRGNOBJ *)hObject)->rgn);
	GdItemFree(hObject);
	return TRUE;
}

#if UPDATEREGIONS
/* region is passed in client coords (win32 api doc bug)*/
int WINAPI
SelectClipRgn(HDC hdc, HRGN hrgn)
{
	return ExtSelectClipRgn(hdc, hrgn, RGN_COPY);
}

/*
 * Select a user clip region into DC, recalculate final clipregion.
 * Only a copy of the passed region is used.
 */
/* region is passed in client coords (win32 api doc bug)*/
int WINAPI
ExtSelectClipRgn(HDC hdc, HRGN hrgn, int fnMode)
{
	HRGN	newrgn;

	if(!hdc)
		return ERROR;
	if(hdc->region != (MWRGNOBJ *)hrgn) {
		/* combine region if not null*/
		if(hrgn) {
			newrgn = CreateRectRgn(0, 0, 0, 0);

			/* 
			 * Temporarily convert region from
			 * client coords to screen coords, since
			 * hwnd->update is kept in screen coords.
			 */
			OffsetRgn(hrgn, hdc->hwnd->clirect.left,
				hdc->hwnd->clirect.top);

			if(fnMode == RGN_COPY)
				CombineRgn(newrgn, hrgn, NULL, fnMode);
			else CombineRgn(newrgn, (HRGN)hdc->region, hrgn,fnMode);

			/* convert passed region back to client coords*/
			OffsetRgn(hrgn, -hdc->hwnd->clirect.left,
				-hdc->hwnd->clirect.top);


			hrgn = newrgn;
		}
		DeleteObject((HRGN)hdc->region);
		hdc->region = (MWRGNOBJ *)hrgn;

		/* force recalc of clipregion*/
		cliphdc = NULL;
		MwPrepareDC(hdc);
	}
	if(hrgn)
		return ((MWRGNOBJ *)hrgn)->rgn->type;
	return NULLREGION;
}

/* update region is returned in client coordinates*/
int WINAPI
GetUpdateRgn(HWND hwnd, HRGN hrgn, BOOL bErase)
{
	/* FIXME bErase*/
	if(!hwnd)
		return ERROR;

	/* convert internal update region to client coords*/
	GdOffsetRegion(hwnd->update, -hwnd->clirect.left, -hwnd->clirect.top);
	GdCopyRegion(((MWRGNOBJ *)hrgn)->rgn, hwnd->update);
	GdOffsetRegion(hwnd->update, hwnd->clirect.left, hwnd->clirect.top);
	return hwnd->update->type;
}
#endif /* UPDATEREGIONS*/

/* update rectangle is returned in client coords*/
BOOL WINAPI
GetUpdateRect(HWND hwnd, LPRECT lpRect, BOOL bErase)
{
	/* FIXME bErase*/
	if(!hwnd)
		return FALSE;
#if UPDATEREGIONS
	if(lpRect) {
		*lpRect = hwnd->update->extents;
		/* convert to client coords*/
		ScreenToClient(hwnd, (LPPOINT)&lpRect->left);
		ScreenToClient(hwnd, (LPPOINT)&lpRect->right);
	}

	/* return TRUE if update region is non-empty*/
	return hwnd->update->type != NULLREGION;
#else
	GetClientRect(hwnd, lpRect);
	return TRUE;
#endif
}

HBRUSH WINAPI
CreateSolidBrush(COLORREF crColor)
{
	MWBRUSHOBJ *hbr;

	hbr = GdItemNew(MWBRUSHOBJ);
	if(!hbr)
		return NULL;
	hbr->hdr.type = OBJ_BRUSH;
	hbr->hdr.stockobj = FALSE;
	hbr->style = BS_SOLID;
	hbr->color = crColor;
	return (HBRUSH)hbr;
}

HPEN WINAPI
CreatePen(int nPenStyle, int nWidth, COLORREF crColor)
{
	MWPENOBJ *hpen;

	/* fix: nWidth > 1*/
	hpen = GdItemNew(MWPENOBJ);
	if(!hpen)
		return NULL;
	hpen->hdr.type = OBJ_PEN;
	hpen->hdr.stockobj = FALSE;
	hpen->style = nPenStyle;
	hpen->color = crColor;
	return (HPEN)hpen;
}

HBITMAP WINAPI
CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight)
{
	MWBITMAPOBJ *	hbitmap;
	int		size;
	int		linelen;

	if(!hdc)
		return NULL;

	nWidth = MWMAX(nWidth, 1);
	nHeight = MWMAX(nHeight, 1);

	/* calc memory allocation size and linelen from width and height*/
	if(!GdCalcMemGCAlloc(hdc->psd, nWidth, nHeight, 0, 0, &size, &linelen))
		return NULL;

	/* allocate gdi object*/
	hbitmap = (MWBITMAPOBJ *)GdItemAlloc(sizeof(MWBITMAPOBJ)-1+size);
	if(!hbitmap)
		return NULL;
	hbitmap->hdr.type = OBJ_BITMAP;
	hbitmap->hdr.stockobj = FALSE;
	hbitmap->width = nWidth;
	hbitmap->height = nHeight;

	/* create compatible with hdc*/
	hbitmap->planes = hdc->psd->planes;
	hbitmap->bpp = hdc->psd->bpp;
	hbitmap->linelen = linelen;
	hbitmap->size = size;

	return (HBRUSH)hbitmap;
}

/* return NULL if no driver bitblit available*/
HDC WINAPI
CreateCompatibleDC(HDC hdc)
{
	HDC	hdcmem;
	PSD	psd;
	PSD	mempsd;

	/* allow NULL hdc to mean screen*/
	psd = hdc? hdc->psd: &scrdev;

	/* allocate memory device, if driver doesn't blit will fail*/
	mempsd = psd->AllocateMemGC(psd);
	if(!mempsd)
		return NULL;

	/* allocate a DC for DesktopWindow*/
	hdcmem = GetDCEx(NULL, NULL, DCX_DEFAULTCLIP);
	if(!hdcmem) {
		mempsd->FreeMemGC(mempsd);
		return NULL;
	}
	hdcmem->psd = mempsd;

	/* select in default bitmap to setup mem device parms*/
	SelectObject(hdcmem, (HGDIOBJ)&default_bitmap);
	return hdcmem;
}

BOOL WINAPI
BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
	HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop)
{
	/* use stretch blit with equal src and dest width/height*/
	return StretchBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
		hdcSrc, nXSrc, nYSrc, nWidth, nHeight, dwRop);
}

BOOL WINAPI
StretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest,
	int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc,
	int nWidthSrc, int nHeightSrc, DWORD dwRop)
{

	HWND	hwnd;
	POINT	dst, src;

	if(!hdcDest || !hdcSrc)
		return FALSE;
	dst.x = nXOriginDest;
	dst.y = nYOriginDest;
	src.x = nXOriginSrc;
	src.y = nYOriginSrc;

	/* if src screen DC, convert coords*/
	/* FIXME: src clipping isn't checked, only one set of cliprects also*/
	if(!MwIsMemDC(hdcSrc) && MwIsClientDC(hdcSrc)) {
		if(!(hwnd = MwPrepareDC(hdcSrc)))
			return FALSE;
		ClientToScreen(hwnd, &src);
	}
	/* if dst screen DC, convert coords and set clipping*/
	/* FIXME: if dest is also screen, src clipping will be overwritten*/
	if(!MwIsMemDC(hdcDest) && MwIsClientDC(hdcDest)) {
		if(!(hwnd = MwPrepareDC(hdcDest)))
			return FALSE;
		ClientToScreen(hwnd, &dst);
	}

	if (nWidthDest == nWidthSrc && nHeightDest == nHeightSrc) {
		GdBlit(hdcDest->psd, dst.x, dst.y, nWidthDest, nHeightDest,
			hdcSrc->psd, src.x, src.y, dwRop);
	} else {
		GdStretchBlit(hdcDest->psd, dst.x, dst.y,
			nWidthDest, nHeightDest, hdcSrc->psd, src.x, src.y,
			nWidthSrc, nHeightSrc, dwRop);
	}
	return TRUE;
}

UINT WINAPI
GetSystemPaletteEntries(HDC hdc,UINT iStartIndex,UINT nEntries,
	LPPALETTEENTRY lppe)
{
	UINT		i;
	MWPALENTRY	rgb;

	/* currently, we only work for screen device*/
	if(!hdc || hdc->psd != &scrdev)
		return 0;

	for(i=0; i<nEntries; ++i) {
		if(!GdGetPalette(hdc->psd, i+iStartIndex, 1, &rgb))
			break;
		lppe->peRed = rgb.r;
		lppe->peGreen = rgb.g;
		lppe->peBlue = rgb.b;
		lppe->peFlags = 0;
		++lppe;
	}
	return i;
}

/* allow NULL hdc for scrdev*/
int WINAPI
GetDeviceCaps(HDC hdc, int nIndex)
{
	PSD	psd;

	if (!hdc)
		psd = &scrdev;
	else psd = hdc->psd;

	switch(nIndex) {
	case HORZRES:
		return psd->xvirtres;
	case VERTRES:
		return psd->yvirtres;
	case BITSPIXEL:
		return psd->bpp;
	case PLANES:
		return psd->planes;
	case LOGPIXELSX:
	case LOGPIXELSY:
		return 96;
	case SIZEPALETTE:
		if (psd->bpp <= 8)
			return psd->ncolors;
		break;
	}
	return 0;
}
