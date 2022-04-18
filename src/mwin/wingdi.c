/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2003, 2005, 2010, 2019 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Win32 API upper level graphics drawing routines
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include "../drivers/genmem.h"
#include "intl.h"
#include <stdlib.h>
#include <string.h>
#include "uni_std.h"

#define MAXSYSCOLORS	29	/* # of COLOR_* system colors*/
#define MAXSTOCKOBJECTS	18	/* # of stock objects*/

#if ERASEMOVE
BOOL mwERASEMOVE = TRUE;	/* default XORMOVE repaint algorithm*/
#else
BOOL mwERASEMOVE = FALSE;	/* default ERASEMOVE repaint algorithm*/
#endif

/* current encoding for non-wide char text functions*/
LONG mwTextCoding = MWTF_UTF8;	/* usually MWTF_ASCII or MWTF_UTF8*/

static HDC	cliphdc;	/* current window cliprects*/

/* default bitmap for new DCs*/
static MWBITMAPOBJ default_bitmap = {
	{OBJ_BITMAP, TRUE}, 1, 1, 1, 1, 1, 1
};

static MWPALOBJ default_palette = {
	{OBJ_PAL, TRUE}, 0, 0
};

static BOOL MwExtTextOut(HDC hdc, int x, int y, UINT fuOptions,
		CONST RECT *lprc, LPCVOID lpszString, UINT cbCount,
		CONST INT *lpDx, int flags);
static int MwDrawText(HDC hdc, LPCVOID lpString, int nCount, LPRECT lpRect, UINT uFormat,
	int flags);
static LONG mwTabbedTextOut(HDC hdc, int x, int y, LPCTSTR lpszString, int cbString,
	int ntabs, LPINT lpTabStops, int nTabOrigin, BOOL noDraw, int flags);

HDC WINAPI
GetDCEx(HWND hwnd,HRGN hrgnClip,DWORD flags)
{
	HDC	hdc;

	if(!hwnd)		/* handle NULL hwnd => desktop*/
		hwnd = rootwp;

	if (!IsWindow(hwnd))
		return NULL;

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
	default_bitmap.data_format = scrdev.data_format;
	hdc->bitmap = &default_bitmap;

	default_palette.palette.palNumEntries = scrdev.palsize / sizeof(PALETTEENTRY);
	hdc->palette = &default_palette;

	hdc->drawmode = R2_COPYPEN;
	hdc->pt.x = 0;
	hdc->pt.y = 0;

	/* assign private DC if CS_OWNDC and not WindowDC*/
	if(hwnd->pClass && (hwnd->pClass->style & CS_OWNDC) && !(flags & DCX_WINDOW)) {
		/* must exclude update region due to BeginPaint GetDCEx call*/
		hdc->flags |= DCX_EXCLUDEUPDATE;
		hwnd->owndc = hdc;
	}
#if WINEXTRA
	/* initialize world transform*/
	hdc->xformWorld2Wnd.eM11 = 1.0f;
	hdc->xformWorld2Wnd.eM12 = 0.0f;
	hdc->xformWorld2Wnd.eM21 = 0.0f;
	hdc->xformWorld2Wnd.eM22 = 1.0f;
	hdc->xformWorld2Wnd.eDx = 0.0f;
	hdc->xformWorld2Wnd.eDy = 0.0f;
	hdc->xformWorld2Vport = hdc->xformWorld2Wnd;
	hdc->xformVport2World = hdc->xformWorld2Wnd;
	hdc->GraphicsMode = GM_COMPATIBLE;
#endif

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
	DeleteObject((HPALETTE)hdc->palette);
	/*
	 * We can only select a bitmap in a memory DC,
	 * so bitmaps aren't released except through DeleteDC.
	 */
	//DeleteObject((HBITMAP)hdc->bitmap);
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
	if(mwERASEMOVE && dragwp && hwnd != rootwp) {	/* don't prohibit root window wallpaper*/
		hdc = NULL;
		lpPaint->fErase = !DefWindowProc(hwnd, WM_ERASEBKGND, (WPARAM)0, (LPARAM)0);
		hwnd->gotPaintMsg = PAINT_DELAYPAINT;
	} else {
		HideCaret(hwnd);

		/* FIXME: mdemo requires update excluded or draw errors occur*/
		hdc = GetDCEx(hwnd, NULL, DCX_DEFAULTCLIP
				|DCX_EXCLUDEUPDATE);	/* FIXME - bug*/

		/* erase client background, always w/alpha blending*/
		if(hwnd->nEraseBkGnd > 0 || mwforceNCpaint)
			lpPaint->fErase = !SendMessage(hwnd, WM_ERASEBKGND, (WPARAM)hdc, (LPARAM)0);
		else
			lpPaint->fErase = 0;

		hwnd->nEraseBkGnd = 0;
	}
	lpPaint->hdc = hdc;

	if( hwnd->paintBrush != NULL )
		DeleteObject ( hwnd->paintBrush );
	if( hwnd->paintPen != NULL )
		DeleteObject ( hwnd->paintPen );
	hwnd->paintBrush = NULL;
	hwnd->paintPen = NULL;

	GetUpdateRect(hwnd, &lpPaint->rcPaint, FALSE);
	return hdc;
}

BOOL WINAPI 
EndPaint(HWND hwnd, CONST PAINTSTRUCT *lpPaint)
{
	if( hwnd->paintBrush != NULL )
		{
		SelectObject ( lpPaint->hdc, GetStockObject(NULL_BRUSH) );
		DeleteObject ( hwnd->paintBrush );
		hwnd->paintBrush = NULL;
		}
	if( hwnd->paintPen != NULL )
		{
		SelectObject ( lpPaint->hdc, GetStockObject(BLACK_PEN) );
		DeleteObject ( hwnd->paintPen );
		hwnd->paintPen = NULL;
		}
	ReleaseDC(hwnd, lpPaint->hdc);
#if UPDATEREGIONS
	/* don't clear update region until done dragging*/
	if(mwERASEMOVE && !dragwp)
		GdSetRectRegion(hwnd->update, 0, 0, 0, 0);
#endif
	ShowCaret(hwnd);
	return TRUE;
}

HDWP
BeginDeferWindowPos(int nNumWindows)
{
	return (HDWP)1;
}

HDWP
DeferWindowPos(HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter,
	       int x, int y, int cx, int cy, UINT uFlags)
{
  SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
  return hWinPosInfo;
}

BOOL
EndDeferWindowPos(HDWP hWinPosInfo)
{
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
GetTextColor(HDC hdc)
{
	if (!hdc)
		return CLR_INVALID;
	return hdc->textcolor;
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

	oldfMode = hdc->textalign;
	hdc->textalign = fMode;
	return oldfMode;
}

UINT WINAPI
GetTextAlign(HDC hdc)
{
	if(!hdc)
		return GDI_ERROR;
	return hdc->textalign;
}

/* FIXME: releasing a DC does NOT change back the drawing mode!*/
int WINAPI
SetROP2(HDC hdc, int fnDrawMode)
{
	int	newmode, oldmode;

	if(!hdc || (fnDrawMode <= 0 || fnDrawMode > R2_LAST))
		return 0;

	oldmode = hdc->drawmode;
	newmode = fnDrawMode - 1;	/* map to MWROP_xxx*/
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
			/* If hdc has a clip region, use it! */
			if (hdc->region != NULL && hdc->region->rgn != NULL
			    && hdc->region->rgn->size != 0) {
				LPRECT rptr = &(hdc->region->rgn->extents);

				/* Bound left,top to 0,0 if they are negative
				 * to avoid an assertion later.
				 */
				MWCOORD left = (rptr->left <= 0) ? 0 : rptr->left;
				MWCOORD top = (rptr->top <= 0) ? 0 : rptr->top;
				GdSetClipRegion(hdc->psd,
					GdAllocRectRegion(left, top, rptr->right, rptr->bottom));
			} else {
				GdSetClipRegion(hdc->psd,
					GdAllocRectRegion(0, 0,
						hdc->psd->xvirtres, hdc->psd->yvirtres));
			}
#else
			static MWCLIPRECT crc = {0, 0, 0, 0};

			/* If hdc has a clip region, use it! */
			if (hdc->region != NULL && hdc->region->rgn != NULL
			    && hdc->region->rgn->size != 0) {
				LPRECT rptr = &(hdc->region->rgn->extents);

				/* Bound left,top to 0,0 if they are negative
				 * to avoid an assertion later.
				 */
				if (rptr->left > 0)
					crc.x = rptr->left;
				if (rptr->top > 0)
					crc.y = rptr->top;
				crc.width = rptr->right;
				crc.height = rptr->bottom;
			} else {
				crc.width = hdc->psd->xvirtres;
				crc.height = hdc->psd->yvirtres;
			}
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
	MWPIXELVALHW pixel;

	hwnd = MwPrepareDC(hdc);
	if(!hwnd)
		return CLR_INVALID;
	pt.x = x;
	pt.y = y;
	if(MwIsClientDC(hdc))
		ClientToScreen(hwnd, &pt);

	/* read pixel value*/
	GdReadArea(hdc->psd, pt.x, pt.y, 1, 1, &pixel);

	return GdGetColorRGB(hdc->psd, pixel);
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
	GdSetForegroundColor(hdc->psd, crColor);
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
		GdSetForegroundColor(hdc->psd, hdc->pen->color);
		MwSetPenStyle(hdc);
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
	GdSetForegroundColor(hdc->psd, hdc->pen->color);

	beg = *lppt++;
	if(MwIsClientDC(hdc))
		ClientToScreen(hwnd, &beg);
	while(--cPoints > 0) {
		end = *lppt++;
		if(MwIsClientDC(hdc))
			ClientToScreen(hwnd, &end);
		MwSetPenStyle(hdc);
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
		GdSetForegroundColor(hdc->psd, hdc->pen->color);
		MwSetPenStyle(hdc);
		GdRect(hdc->psd, rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top);
	}

	/* fill rectangle in current brush color*/
	if(hdc->brush->style != BS_NULL) {
		InflateRect(&rc, -1, -1);
		GdSetForegroundColor(hdc->psd, hdc->brush->color);
		GdFillRect(hdc->psd, rc.left, rc.top, rc.right - rc.left,
			rc.bottom - rc.top);
	}

	return TRUE;
}

#if MW_FEATURE_SHAPES
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
		GdSetForegroundColor(hdc->psd, hdc->brush->color);
		MwSetPenStyle(hdc);
		GdEllipse(hdc->psd, rc.left, rc.top, rx, ry, TRUE);
	}

	/* draw ellipse outline in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForegroundColor(hdc->psd, hdc->pen->color);
		MwSetPenStyle(hdc);
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
		GdSetForegroundColor(hdc->psd, hdc->brush->color);
		MwSetPenStyle(hdc);
		GdArc(hdc->psd, rc.left, rc.top, rx, ry,
			rc2.left, rc2.top, rc2.right, rc2.bottom, MWPIE);
	}

	/* draw ellipse outline in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForegroundColor(hdc->psd, hdc->pen->color);
		if(type == MWPIE)
			type = MWARC;	/* MWARCOUTLINE?*/
		MwSetPenStyle(hdc);
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
		GdSetForegroundColor(hdc->psd, hdc->brush->color);
		GdFillPoly(hdc->psd, nCount, pp);
	}

	/* draw polygon outline in current pen color*/
	if(hdc->pen->style != PS_NULL) {
		GdSetForegroundColor(hdc->psd, hdc->pen->color);
		MwSetPenStyle(hdc);
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
#endif /* MW_FEATURE_SHAPES*/

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
	if((UINT_PTR)obr <= MAXSYSCOLORS) {		// Convert pointer to long without truncation to check if small int passed
		crFill = GetSysColor((int)obr-1);	// OK: Not pointer. Convert to int then decrement.
	} else {
		/* get color from passed HBRUSH*/
		if(obr->style == BS_NULL)
			return TRUE;
		crFill = obr->color;
	}

	/* fill rectangle in passed brush color*/
	GdSetForegroundColor(hdc->psd, crFill);
	GdFillRect(hdc->psd, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top);
	return TRUE;
}


/* set current input coding*/
void WINAPI
MwSetTextCoding(LONG mode)
{
	mwTextCoding = mode;
}

/* ascii or utf8 */
BOOL WINAPI
TextOut(HDC hdc, int x, int y, LPCSTR lpszString, int cbString)
{
	/* kaffe port wants MWTF_UTF8 here...*/
	return MwExtTextOut(hdc, x, y, 0, NULL, lpszString, cbString, NULL,
		mwTextCoding);
}

/* UC16 */
BOOL WINAPI
TextOutW(HDC hdc, int x, int y, LPCWSTR lpszString, int cbString)
{
	/* kaffe port wants MWTF_UTF8 here...*/
	return MwExtTextOut(hdc, x, y, 0, NULL, lpszString, cbString, NULL, MWTF_UC16 );
}

/* ascii or utf8 */
BOOL WINAPI
ExtTextOut(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc,
	LPCSTR lpszString, UINT cbCount, CONST INT *lpDx)
{
	return MwExtTextOut(hdc, x, y, fuOptions, lprc, lpszString,
		cbCount, lpDx, mwTextCoding);
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
		GdSetForegroundColor(hdc->psd, hdc->bkcolor);
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
			GdSetBackgroundColor(hdc->psd, hdc->bkcolor);
	}

	if (cbCount == 0) {
		/* Special case - no text.  Used to fill rectangle. */
		return TRUE;
	}

	/* nyi: lpDx*/

	/* draw text in current text foreground and background color*/
	GdSetForegroundColor(hdc->psd, hdc->textcolor);
	//GdSetFont(hdc->font->pfont);

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
			GdGetTextSize(hdc->font->pfont, lpszString, cbCount, &pw, &ph, &pb, flags);
			pt.y += ph;
		}
		flags |= MWTF_BOTTOM;
	} else
		flags |= MWTF_TOP;

	if((hdc->textalign & TA_CENTER) == TA_CENTER) {
		MWCOORD     ph, pw, pb;

		GdGetTextSize(hdc->font->pfont, lpszString, cbCount, &pw, &ph, &pb, flags);
		pt.x -= pw/2;
	} else if(hdc->textalign & TA_RIGHT) {
		MWCOORD     ph, pw, pb;

		GdGetTextSize(hdc->font->pfont, lpszString, cbCount, &pw, &ph, &pb, flags);
		pt.x -= pw;
	}
	GdText(hdc->psd, hdc->font->pfont, pt.x, pt.y, lpszString, cbCount, flags);

	return TRUE;
}

/* ascii or utf8 */
int WINAPI
DrawTextA(HDC hdc, LPCSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
	return MwDrawText(hdc, lpString, nCount, lpRect, uFormat, mwTextCoding);
}

/* unicode*/
int WINAPI
DrawTextW(HDC hdc, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
	return MwDrawText(hdc, lpString, nCount, lpRect, uFormat, MWTF_UC16);
}

#if OLD_DRAWTEXT
/*
 *  Check in text for the '&' chr, remove it from text and sets rect for pos
 */
/* OLD_DRAWTEXT function*/
static LPCSTR
MwCheckUnderlineChar(HDC hdc, char *text, int *pLen, LPRECT rcLine)
{
	int i;
	int txtlen;

	if (pLen)
		txtlen = *pLen;
	else
		txtlen = strlen(text);

	for (i = 0; i < txtlen; i++) {
		if ((text[i] == '&') && (i + 1 < txtlen)
		    && (text[i + 1] != '&')) {
			SIZE sz;
			TEXTMETRIC tm;

			GetTextMetrics(hdc, &tm);
			GetTextExtentPoint(hdc, text, i, &sz);
			rcLine->left = sz.cx;
			rcLine->top = 0;
			rcLine->bottom = tm.tmAscent + 1;
			GetTextExtentPoint(hdc, text + i + 1, 1, &sz);
			rcLine->right = rcLine->left + sz.cx;
			text = strdup(text);
			memmove(text + i, text + i + 1, txtlen - i);
			txtlen--;
			if (pLen)
				*pLen = txtlen;
			return text;
		}
	}
	return FALSE;
}

/* note: many DT_x aren't implemented in this function*/
/* internal version of DrawText, passed flags for text data type*/
/* OLD_DRAWTEXT function*/
static int
MwDrawText(HDC hdc, LPCVOID lpString, int nCount, LPRECT lpRect, UINT uFormat, int flags)
{
	MWCOORD	x, y, width, height, baseline;
	LPCSTR lpStrDup = NULL;
	RECT rcUline;

	if(!hdc)
		return 0;

	if(nCount == -1)
		nCount = strlen(lpString);

	if (!(uFormat & DT_NOPREFIX) && (lpStrDup = MwCheckUnderlineChar(hdc, lpString, &nCount, &rcUline)))
		lpString = lpStrDup;
	if(uFormat & (DT_CALCRECT|DT_CENTER|DT_RIGHT)) {
		GdGetTextSize(hdc->font->pfont, lpString, nCount,
			&width, &height, &baseline, flags);
	}
	x = lpRect->left;
	y = lpRect->top;

	if(uFormat & DT_CALCRECT) {
#if 0
		if (!(uFormat & DT_SINGLELINE)) {
			char *p=(char*)lpString, *pLast=p, *pMax=p;
			int iLenCur, iLenMax = 0, iLines = 0;
			long lMaxWidth = lpRect->right - lpRect->left;

			/* Determine line with maximum width */
			for (pLast=p, pMax=p; ; p++) {
				if (*p=='\n' || !*p) {
					if (lMaxWidth) {
						do {
							iLenCur = p - pLast;
							GdGetTextSize(hdc->font->pfont, pLast, iLenCur,
								&width, &height, &baseline, flags);
							if (width <= lMaxWidth) break;
							while (p>(char*)lpString && (*p==' ' || *p=='\t')) p--;
							while (p>(char*)lpString && !(*p==' ' || *p=='\t')) p--;

						} while (width > lMaxWidth && p>(char*)lpString);
					}
					if ((iLenCur = p - pLast) > iLenMax) {
						iLenMax = iLenCur;
						pMax = pLast;
					}
					pLast = p+1;
					iLines++;
				}
				if (!*p) break;
			}
			GdGetTextSize(hdc->font->pfont, pMax, iLenMax,
				&width, &height, &baseline, flags);
			height *= iLines;
		}
#endif
		lpRect->right = x + width;
		lpRect->bottom = y + height;
		if (lpStrDup)
			free(lpStrDup);
		return height;
	}

	if(uFormat & DT_CENTER)
		x = (lpRect->left + lpRect->right - width) / 2;
	else if(uFormat & DT_RIGHT)
		x = lpRect->right - width;
	
	if(uFormat & DT_VCENTER)
	    y = lpRect->bottom / 2 - (height / 2);
	
	if(uFormat & DT_BOTTOM) {
	    flags |= MWTF_BOTTOM;
	    y = lpRect->bottom;
	}    
	
	/* draw text at DT_TOP using current fg, bg and bkmode*/
	MwExtTextOut(hdc, x, y, 0, NULL, lpString, nCount, NULL, flags);
	if (lpStrDup) {
		OffsetRect(&rcUline, x, y);
		SelectObject(hdc, GetStockObject(BLACK_PEN));
		MoveToEx(hdc, rcUline.left, rcUline.bottom, NULL);
		LineTo(hdc, rcUline.right, rcUline.bottom);
		free(lpStrDup);
	}
	return height;
}
#else /* !OLD_DRAWTEXT*/
/*
 * MwDrawText Support routine: given a position, a string and length,
 *	process underline, ampersands and tabs, and output
 *	using mwExtTextOut ormwTabbedTextOut
 */
/* !OLD_DRAWTEXT function*/
static void
mwDrawTextOut(HDC hDC, int x, int y, LPSTR str, int len, UINT uFormat, int flags)
{
	int i, j, k, dx, dy, x1;
	MWCOORD baseline;

	if(!hDC)
		return;

	for (i = 0, j = 0, k = 0, x1 = 0; i < len; i++) {
		if (str[i] == '&') {
			if (i) {
				if (uFormat & DT_EXPANDTABS) {
					mwTabbedTextOut(hDC, x, y, &str[j], i, 0, 0, x, FALSE, flags);
					x += LOWORD(mwTabbedTextOut(hDC, x, y, &str[j], i, 0, 0, 0, TRUE, flags));
				} else {
					MwExtTextOut(hDC, x, y, 0, NULL, &str[j], i - j, NULL, flags);
					GdGetTextSize(hDC->font->pfont, &str[j], i - j, &dx, &dy, &baseline, flags);
					x += dx;
				}
			}
			if (str[i + 1] == '&') {
				TextOut(hDC, x, y, "&", 1);
				GdGetTextSize(hDC->font->pfont, "&", 1, &dx, &dy, &baseline, flags);
				x += dx;
				i++;
			} else {
				x1 = x;
				k = i + 1;
			}
			j = i + 1;
		}
	}

	if (i > j) {
		if (uFormat & DT_EXPANDTABS)
			mwTabbedTextOut(hDC, x, y, &str[j], i - j, 0, 0, x, FALSE, flags);
		else
			MwExtTextOut(hDC, x, y, 0, NULL, &str[j], i - j, NULL, flags);
		if (k && str[k] != ' ') {
			GdGetTextSize(hDC->font->pfont, &str[k], 1, &dx, &dy, &baseline, flags);
			dy -= dy / 18 + 1;	/* underline position */
			MoveToEx(hDC, x1, y + dy, NULL);
			LineTo(hDC, x1 + dx, y + dy);
		}
	}
}

/*
 * Complex text output, supports:
 *	DT_BOTTOM, DT_CALCRECT, DT_CENTER, DT_EXTERNALLEADING,
 *	DT_LEFT, DT_RIGHT, DT_SINGLELINE, DT_TOP (default), DT_VCENTER,
 *	DT_WORDBREAK, DT_NOCLIP (default), DT_NOPREFIX, DT_EXPANDTABS
 */
/* !OLD_DRAWTEXT function*/
static int
MwDrawText(HDC hDC, LPCVOID lpsz, int cb, LPRECT lprc, UINT uFormat, int flags)
{
	LPCSTR str = lpsz;
	int lineheight;
	int textwidth;
	int x, y, baseline, baselinefnt;
	int dx, dy;
	int nlines = 0;
	//int tabsize = 8;
	int rtotal = 0;
	HPEN hPen = 0;
	TEXTMETRIC TextMetrics;
	int charwidth[256];

	if (!lprc || !hDC)
		return 0;

	if (uFormat & DT_TABSTOP) {
		//tabsize = uFormat >> 8;		/* FIXME: not used*/
		uFormat &= 0xff;
	}

	GetCharWidth(hDC, 0, 255, charwidth);
	GetTextMetrics(hDC, (LPTEXTMETRIC)&TextMetrics);
	lineheight = TextMetrics.tmHeight;

	if (uFormat & DT_EXTERNALLEADING)
		lineheight += TextMetrics.tmExternalLeading;

	if (uFormat & DT_CALCRECT) {
		lprc->left = HIWORD(lprc->left) ? 0 : lprc->left;
		lprc->right = HIWORD(lprc->right) ? 0 : lprc->right;
		lprc->top = HIWORD(lprc->top) ? 0 : lprc->top;
		lprc->bottom = HIWORD(lprc->bottom) ? 0 : lprc->bottom;
	} else {
		hPen = CreatePen(PS_SOLID, 1, GetTextColor(hDC));	/* underline text color */
		hPen = SelectObject(hDC, hPen);
	}

	textwidth = lprc->right - lprc->left;
	baseline = lprc->top;

	if (cb == -1) 
		cb = str? strlen(str): 0;

	while (cb) {
		int space = 0;
		int Ampersands = 0;
		int AmpersandPos = 0;
		int lbreak = 0;
		int place = 0;
		int cnt;

		rtotal = 0;

		for (cnt = 0; cnt < cb; cnt += MW_CHRNBYTE(str[cnt])) {
			switch (str[cnt]) {
			case 0:
				lbreak++;
				break;
			case ' ':
				space++;
				place = cnt;
				rtotal += charwidth[(BYTE) str[cnt]];
				break;
			case '\t':
				space++;
				place = cnt;
				break;
			case '&':
				if (uFormat & DT_NOPREFIX)
					rtotal += charwidth[(BYTE) str[cnt]];
				else {
					if (Ampersands && AmpersandPos + 1 == cnt) {
						rtotal += charwidth[(BYTE) str[cnt]];
					} else {
						AmpersandPos = cnt;
						Ampersands = 1;
					}
				}
				break;
			case '\r':
			case '\n':
				lbreak++;
				break;
			default:
				rtotal += charwidth[(BYTE) str[cnt]];
				break;
			}

			if (lbreak)
				break;

			/* check width */
			if (rtotal > textwidth) {
				if ((uFormat & DT_CALCRECT) && (uFormat & DT_SINGLELINE)) {
					lprc->right += (rtotal - textwidth);
					textwidth = lprc->right - lprc->left;
					continue;
				}
				if (uFormat & DT_WORDBREAK) {
					if (space) {
						cnt = place;
						GdGetTextSize(hDC->font->pfont, str, cnt - 1, &dx, &dy, &baselinefnt, flags);
						rtotal = dx;
					}
					cnt++;
					break;
				}
			}
		}

		/* calc width */
		dx = rtotal;
		dy = lineheight;

		if (uFormat & DT_RIGHT)
			x = lprc->right - dx + 1;
		else if (uFormat & DT_CENTER)
			x = (int) (lprc->left + lprc->right - dx) / 2;
		else
			x = lprc->left;

		/* calc height */
		if (uFormat & DT_VCENTER) {
			GdGetTextSize(hDC->font->pfont, str, cnt, &dx, &dy, &baselinefnt, flags);
			y = (int) (baseline + lprc->bottom - dy) / 2;
		} else if (uFormat & DT_BOTTOM)
			y = lprc->bottom - dy;
		else
			y = baseline;

		if ((uFormat & DT_CALCRECT) == 0) {
			/* handle prefixes */
			if (Ampersands)
				mwDrawTextOut(hDC, x, y, (LPSTR)str, cnt, uFormat, flags);
			else {
				if (uFormat & DT_EXPANDTABS) {
					mwTabbedTextOut(hDC, x, y, str, cnt, 0, 0, x, FALSE, flags);
				} else {
					if (uFormat & DT_NOCLIP)
						MwExtTextOut(hDC, x, y, 0, NULL, str, cnt, NULL, flags);
					else
						MwExtTextOut(hDC, x, y, ETO_CLIPPED, lprc, str, cnt, NULL, flags);
				}
			}
		}

		nlines++;
		baseline += lineheight;

		if (uFormat & DT_CALCRECT)
			lprc->bottom = baseline;

		if (uFormat & DT_SINGLELINE)
			break;

		if (lbreak) {
			if (str[cnt] == '\r')
				cnt++;
			if (str[cnt] == '\n')
				cnt++;
		}

		str += cnt;
		cb -= cnt;

		if (baseline > lprc->bottom) {
			break;
		}
	}

	if ((uFormat & DT_CALCRECT) && !(uFormat & DT_SINGLELINE) && nlines == 1)
		lprc->right = rtotal + lprc->left;

	if (hPen) {
		hPen = SelectObject(hDC, hPen);
		DeleteObject(hPen);
	}

	return baseline - lprc->top;
}
#endif /* !OLD_DRAWTEXT*/

#if MW_FEATURE_IMAGES
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
#endif /* MW_FEATURE_IMAGES*/

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

static MWBRUSHOBJ syscolorBrushes[MAXSYSCOLORS];

HBRUSH WINAPI
GetSysColorBrush(int nIndex)
{
	MWBRUSHOBJ *hbr;

	if(nIndex >= 0 && nIndex < MAXSYSCOLORS) {
		hbr = &syscolorBrushes[nIndex];
		hbr->hdr.type = OBJ_BRUSH;
		hbr->hdr.stockobj = TRUE;
		hbr->style = BS_SOLID;
		hbr->color = sysColors[nIndex];
		return (HBRUSH)hbr;
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
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_FIXED	/* was MWFONT_OEM_FIXED*/
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
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_FIXED	/* was MWFONT_OEM_FIXED*/
};

static MWFONTOBJ OBJ_SYSTEM_FIXED_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_FIXED
};

static MWFONTOBJ OBJ_DEFAULT_GUI_FONT = {
	{OBJ_FONT, TRUE}, NULL, MWFONT_SYSTEM_VAR	/* was MWFONT_GUI_VAR*/
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
				pFont->pfont = GdCreateFont(&scrdev, pFont->name, 0, 0, NULL);
				if (!pFont->pfont)
					pFont->pfont = GdCreateFont(&scrdev, NULL, 0, 0, NULL);
			}
			return pObj;
		}

		/* implement multiple color schemes with
		 * standard background brushes...
		 */
		switch(nObject) {
		case LTGRAY_BRUSH:
		case GRAY_BRUSH:
			((MWBRUSHOBJ *)pObj)->color = GetSysColor(COLOR_BTNFACE);
			break;
		case DKGRAY_BRUSH:
			((MWBRUSHOBJ *)pObj)->color = GetSysColor(COLOR_BTNSHADOW);
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
	case OBJ_PAL:
		objOrg = (HGDIOBJ)hdc->palette;
		hdc->palette = (MWPALOBJ *)hObject;
		break;
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
			pb->planes, pb->bpp, pb->data_format, pb->pitch, pb->size, &pb->bits[0]))
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

void
MwSetPenStyle(HDC hdc)
{
	uint32_t dm;
	int 	dc = 32;

	switch (hdc->pen->style) {
	case PS_SOLID:
		dm = 0xFFFFFFFF;		/* 1111 1111 1111 1111*/
		break;
	case PS_DASH:
		dm = 0x1F1F1F1F; 		/*0001 1111 0001 1111*/
		break;
	case PS_DOT:
		dm = 0x11111111; 		/*0001 0001 0001 0001*/
		break;
	case PS_DASHDOT:
		dm = 0x18FF18FF; 		/*0001 1000 1111 1111*/
		break;
	case PS_DASHDOTDOT:
		dm = 0x88F888F8; 		/*1000 1000 1111 1000*/
		break;
	case PS_NULL:
	default:
		dm = 0;
		break;
	}
	GdSetDash(&dm, &dc);
}

HBITMAP WINAPI
CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight)
{
	MWBITMAPOBJ *	hbitmap;
	unsigned int	size;
	unsigned int pitch;

	if(!hdc)
		return NULL;

	nWidth = MWMAX(nWidth, 1);
	nHeight = MWMAX(nHeight, 1);

	/* calc memory allocation size and pitch from width and height*/
	if(!GdCalcMemGCAlloc(hdc->psd, nWidth, nHeight, 0, 0, &size, &pitch))
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
	hbitmap->data_format = hdc->psd->data_format;
	hbitmap->pitch = pitch;
	hbitmap->size = size;

	return (HBRUSH)hbitmap;
}

HBITMAP WINAPI CreateDIBSection(
  HDC hdc, CONST BITMAPINFO *pbmi, UINT iUsage,
  VOID **ppvBits, HANDLE hSection, DWORD dwOffset)
{
	MWBITMAPOBJ *	hbitmap;
	unsigned int	size;
	unsigned int pitch;
	PSD psd = hdc? hdc->psd: &scrdev;

	/* calc memory allocation size and pitch from width and height*/
	if(!GdCalcMemGCAlloc(psd, pbmi->bmiHeader.biWidth, MWABS(pbmi->bmiHeader.biHeight),
		pbmi->bmiHeader.biPlanes, pbmi->bmiHeader.biBitCount, &size, &pitch))
			return NULL;

	/* allocate gdi object*/
	hbitmap = (MWBITMAPOBJ *)GdItemAlloc(sizeof(MWBITMAPOBJ)-1+size);
	if(!hbitmap)
		return NULL;
	hbitmap->hdr.type = OBJ_BITMAP;
	hbitmap->hdr.stockobj = FALSE;
	hbitmap->width = pbmi->bmiHeader.biWidth;
	hbitmap->height = MWABS(pbmi->bmiHeader.biHeight);
	hbitmap->planes = pbmi->bmiHeader.biPlanes;
	hbitmap->bpp = pbmi->bmiHeader.biBitCount;
	hbitmap->pitch = pitch;
	hbitmap->size = size;

	if (ppvBits) *ppvBits = &hbitmap->bits[0];

	return (HBITMAP)hbitmap;
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

	/* select in default palette */
	SelectObject(hdcmem, (HGDIOBJ)&default_palette);
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
	/* FIXME: src clipping doesn't check overlapped source window, only unmapped*/
	if(!MwIsMemDC(hdcSrc)) {
		hwnd = hdcSrc->hwnd;
		if (!hwnd || hwnd->unmapcount)
			return FALSE;
		if (MwIsClientDC(hdcSrc))
			ClientToScreen(hwnd, &src);
	}

	/* set dest clipping; if dst screen DC, convert coords*/
	hwnd = MwPrepareDC(hdcDest);
	if(!MwIsMemDC(hdcDest) && MwIsClientDC(hdcDest)) {
		if (!hwnd)
			return FALSE;
		ClientToScreen(hwnd, &dst);
	}

	if (nWidthDest == nWidthSrc && nHeightDest == nHeightSrc) {
		GdBlit(hdcDest->psd, dst.x, dst.y, nWidthDest, nHeightDest,
			hdcSrc->psd, src.x, src.y, dwRop);
	} else {
		GdStretchBlit(hdcDest->psd, dst.x, dst.y,
			dst.x + nWidthDest - 0, dst.y + nHeightDest - 0,
			hdcSrc->psd, src.x, src.y,
			src.x + nWidthSrc - 1, src.y + nHeightSrc - 1, dwRop);
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
	int caps = 0;

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
	case RASTERCAPS:
#ifdef MW_FEATURE_PALETTE
		caps |= RC_PALETTE;
#endif
		return caps;
	}
	return 0;
}

/*
 *  Draw a rectangle indicating focus
 */
BOOL WINAPI
DrawFocusRect(HDC hdc, LPRECT prect)
{
	uint32_t dm = 0xAAAAAAAA;
	int dc = 32;
	int oldmode = GdSetMode(MWROP_XOR);
	HPEN holdpen = SelectObject(hdc,
		CreatePen(PS_SOLID, 1, RGB(255, 255, 255)));

	GdSetDash(&dm, &dc);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, prect->left - 1, prect->top, prect->right + 1,
		  prect->bottom);
	GdSetDash(&dm, &dc);
	DeleteObject(SelectObject(hdc, holdpen));
	GdSetMode(oldmode);
	return TRUE;
}


/* ascii or utf8 */
static LONG
mwTabbedTextOut(HDC hdc, int x, int y, LPCTSTR lpszString, int cbString,
	int ntabs, LPINT lpTabStops, int nTabOrigin, BOOL noDraw, int flags)
{
	TEXTMETRIC tm;
	int count;
	int tot;
	int xw, xh, xb;
	int tabPos = x;
	int deftab = 32;
	int orgx = x;
	LPTSTR szShaped;
	LPCTSTR pstr;
	LPINT pTab = lpTabStops;
	unsigned long attrib = 0;

	if (!hdc)
		return 0;

	if (GetTextMetrics(hdc, &tm))
		deftab = 8 * tm.tmAveCharWidth;

	if (pTab == NULL)
		ntabs = 0;

	if (ntabs == 1) {
		deftab = *pTab;
		ntabs = 0;
	}

	if (cbString == -1)
		cbString = strlen(lpszString);

	/* Duplicate text. If coding is UTF-8, generate it by checking shape/joining*/
	if (flags & MWTF_UTF8)
		szShaped = doCharShape_UTF8(lpszString, cbString, &cbString, &attrib);
	else
		szShaped = strdup(lpszString);

	if (szShaped == NULL)
		return 0;

	pstr = szShaped;
	tot = cbString;
	//GdSetFont(hdc->font->pfont);
	while (tot > 0) {
		for (count = 0; (count < tot) && (pstr[count] != '\t'); count++)
		     continue;

		GdGetTextSize(hdc->font->pfont, pstr, count, &xw, &xh, &xb, flags);

		while ((ntabs > 0) && (nTabOrigin + *pTab <= (x + xw))) {
			pTab++;
			ntabs--;
		}

		if (count == tot)
			tabPos = x + xw;
		else if (ntabs > 0)
			tabPos = nTabOrigin + *pTab;
		else
			tabPos = nTabOrigin + ((x + xw - nTabOrigin) / deftab + 1) * deftab;

		if (!noDraw) {
			RECT rect;

			rect.left = x;
			rect.top = y;
			rect.right = x + tabPos;
			rect.bottom = y + xh;
			//TextOut ( hdc, x, y, pstr, count );
#if MW_FEATURE_INTL
			if (attrib & TEXTIP_EXTENDED) {
				LPSTR virtText = doCharBidi_UTF8(pstr, count, NULL, NULL, &attrib);
				if (virtText) {
					if (attrib & TEXTIP_RTOL)
						DrawTextA(hdc, virtText, count, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
					else
						DrawTextA(hdc, virtText, count, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
					free(virtText);
				}
			} else
#endif
				DrawTextA(hdc, pstr, count, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
		}

		x = tabPos;
		tot -= (count + 1);
		pstr += (count + 1);
	}

	free(szShaped);
	return MAKELONG((tabPos - orgx), xh);
}


/* ascii or utf8 */
LONG WINAPI
TabbedTextOut(HDC hdc, int x, int y, LPCTSTR lpszString, int cbString,
	int ntabs, LPINT lpTabStops, int nTabOrigin)
{
	return mwTabbedTextOut(hdc, x, y, lpszString, cbString, ntabs,
			       lpTabStops, nTabOrigin, FALSE, mwTextCoding);
}


DWORD WINAPI
GetTabbedTextExtent(HDC hdc, int x, int y, LPCTSTR lpszString, int cbString,
	int ntabs, LPINT lpTabStops)
{
	return mwTabbedTextOut(hdc, x, y, lpszString, cbString, ntabs,
			       lpTabStops, 0, TRUE, mwTextCoding);
}

int WINAPI
MulDiv(int nMultiplicand, int nMultiplier, int nDivisor)
{
    int         nResult;
    long        lMulti;
    lMulti = nMultiplicand * nMultiplier;
    if ( lMulti < 0 )
        lMulti -= nDivisor >> 1;
    else
        lMulti += nDivisor >> 1;
    if(nDivisor == 0) nDivisor = 1;
    nResult = lMulti / nDivisor;
    return nResult;
}

// The RealizePalette function maps palette entries from the current logical palette to the system palette
//
// Params
// hdc: A handle to the device context into which a logical palette has been selected
//
// Return Value
// If the function succeeds, the return value is the number of entries in the logical palette mapped to the system palette.
// If the function fails, the return value is GDI_ERROR.
UINT WINAPI RealizePalette(HDC hdc)
{
	PSD psd = hdc ? hdc->psd : &scrdev;

	GdSetPalette(psd, 0, hdc->palette->palette.palNumEntries, (MWPALENTRY *)&hdc->palette->palette.palPalEntry);
	return hdc->palette->palette.palNumEntries;
}

// The CreatePalette function creates a logical palette
//
// Params
// plpal: A pointer to a LOGPALETTE structure that contains information about the colors in the logical palette.
//
// Return Value:
// If the function succeeds, the return value is a handle to a logical palette.
// If the function fails, the return value is NULL.
HPALETTE WINAPI CreatePalette(const LOGPALETTE *plpal)
{
	MWPALOBJ *pal;

	if ((pal = (MWPALOBJ *)GdItemAlloc(sizeof(MWPALOBJ) + ((plpal->palNumEntries - 1) * sizeof(PALETTEENTRY)))))
	{
		pal->hdr.type = OBJ_PAL;
		pal->hdr.stockobj = FALSE;
		pal->palette.palVersion = plpal->palVersion;
		pal->palette.palNumEntries = plpal->palNumEntries;
		memcpy(pal->palette.palPalEntry, plpal->palPalEntry, (plpal->palNumEntries * sizeof(PALETTEENTRY)));
	}

	return (HPALETTE)pal;
}

// The SelectPalette function selects the specified logical palette into a device context
//
// Params
// hdc: A handle to the device context.
// hPal: A handle to the logical palette to be selected.
// bForceBkgd: Specifies whether the logical palette is forced to be a background palette.
//
// Return Value
// If the function succeeds, the return value is a handle to the device context's previous logical palette.
// If the function fails, the return value is NULL.
HPALETTE WINAPI SelectPalette(HDC hdc, HPALETTE hPal, BOOL bForceBkgd)
{

	return SelectObject(hdc, hPal);
}

// The SetDIBColorTable function sets RGB (red, green, blue) color values in a range of entries in the color table of the DIB that is currently selected into a specified device context
//
// Params
// hdc: A device context. A DIB must be selected into this device context
// iStart: A zero-based color table index that specifies the first color table entry to set
// Centries: The number of color table entries to set
// prgbq: A pointer to an array of RGBQUAD structures containing new color information for the DIB's color table
//
// Return Value
// If the function succeeds, the return value is the number of color table entries that the function sets.
// If the function fails, the return value is zero.
UINT SetDIBColorTable(HDC hdc, UINT iStart, UINT cEntries, const RGBQUAD *prgbq)
{
	MWPALENTRY *pal;
	PSD psd = hdc ? hdc->psd : &scrdev;
	int i;

	if ((psd->palsize/sizeof(psd->palette)) >= cEntries)
	{
		if ((pal = &psd->palette[iStart]))
		{
			for (i = 0; i < cEntries; i++)
			{
				pal->b = prgbq->rgbBlue;
				pal->g = prgbq->rgbGreen;
				pal->r = prgbq->rgbRed;
				//pal->_padding = prgbq->rgbReserved;
			}

			return cEntries;
		}
	}

	return 0;
}

// The SetSystemPaletteUse function allows an application to specify whether the system palette contains 2 or 20 static colors.
// The default system palette contains 20 static colors. (Static colors cannot be changed when an application realizes a logical palette.)
//
// Params
// hdc: A handle to the device context.This device context must refer to a device that supports color palettes
// use: The new use of the system palette. This parameter can be one of the following values SYSPAL_NOSTATIC, SYSPAL_NOSTATIC256, or SYSPAL_STATIC
//
// Return Value
// If the function succeeds, the return value is the previous system palette.It can be either SYSPAL_NOSTATIC, SYSPAL_NOSTATIC256, or SYSPAL_STATIC.
// If the function fails, the return value is SYSPAL_ERROR.
UINT SetSystemPaletteUse(HDC hdc, UINT use)
{

	return SYSPAL_NOSTATIC;
}
