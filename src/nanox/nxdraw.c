/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Nano-X Draw Library
 */
#define MWINCLUDECOLORS
#include <stdio.h>
#include "nano-X.h"
#include "nxdraw.h"

void
nxPaintNCArea(GR_DRAW_ID id, int w, int h, GR_CHAR *title, GR_BOOL active,
	GR_WM_PROPS props)
{
	int		x = 0;
	int		y = 0;
	GR_GC_ID	gc = GrNewGC();
	GR_FONT_ID	fontid;
	GR_RECT		r;


	if (props & GR_WM_PROPS_APPFRAME) {
		/* draw 2-line 3d border around window*/
		nxDraw3dOutset(id, x, y, w, h);
		x += 2; y += 2; w -= 4; h -= 4;

		/* draw 1-line inset inside border*/
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
		GrRect(id, gc, x, y, w, h);
		x += 1; y += 1; w -= 2; h -= 2;
	} else if (props & GR_WM_PROPS_BORDER) {
		/* draw 1-line black border around window*/
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_WINDOWFRAME));
		GrRect(id, gc, x, y, w, h);
		x += 1; y += 1; w -= 2; h -= 2;
	}

	if (!(props & GR_WM_PROPS_CAPTION))
		goto out;

	/* fill caption*/
	GrSetGCForeground(gc, 
		GrGetSysColor(active? GR_COLOR_ACTIVECAPTION:
			GR_COLOR_INACTIVECAPTION));
	GrFillRect(id, gc, x, y, w, CYCAPTION);

	/* draw caption text*/
	if (title) {
		GrSetGCForeground(gc,
			GrGetSysColor(active? GR_COLOR_ACTIVECAPTIONTEXT:
				GR_COLOR_INACTIVECAPTIONTEXT));
		GrSetGCUseBackground(gc, GR_FALSE);
		fontid = GrCreateFont(GR_FONT_GUI_VAR, 0, NULL);
		GrSetGCFont(gc, fontid);
		GrText(id, gc, x+4, y-1, title, -1, GR_TFASCII|GR_TFTOP);
		GrDestroyFont(fontid);
	}
	y += CYCAPTION;

	/* draw one line under caption*/
	if (props & GR_WM_PROPS_APPFRAME) {
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
		GrLine(id, gc, x, y, x+w-1, y);
	}

	if (props & GR_WM_PROPS_CLOSEBOX) {
		/* draw close box*/
		r.x = x + w - CXCLOSEBOX - 2;
		r.y = y - CYCAPTION + 2;
		r.width = CXCLOSEBOX;
		r.height = CYCLOSEBOX;

		nxDraw3dBox(id, r.x, r.y, r.width, r.height,
			GrGetSysColor(GR_COLOR_BTNHIGHLIGHT),
			GrGetSysColor(GR_COLOR_WINDOWFRAME));
		nxInflateRect(&r, -1, -1);
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
		GrFillRect(id, gc, r.x, r.y, r.width, r.height);

		nxInflateRect(&r, -1, -1);
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_BTNTEXT));
		GrLine(id, gc, r.x, r.y, r.x+r.width-1, r.y+r.height-1);
		GrLine(id, gc, r.x, r.y+r.height-1, r.x+r.width-1, r.y);
	}

#if 0
	/* fill in client area*/
	y++;
	h -= CYCAPTION+1;
	GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
	GrFillRect(id, gc, x, y, w, h);
#endif

out:
	GrDestroyGC(gc);
}

/*
 * Enlarge/decrease the size of a rectangle
 */
void
nxInflateRect(GR_RECT *prc, GR_SIZE dx, GR_SIZE dy)
{
	prc->x -= dx;
	prc->y -= dy;
	prc->width += dx * 2;
	prc->height += dy * 2;
}

/*
 * nxDraw3dShadow
 * 	NOINDENT_BLACK	T=white, B=black
 * 	NOINDENT_GRAY	T=white, B=dkgray
 * 	INDENT_BLACK	T=black, B=white
 * 	INDENT_GRAY		T=dkgray, B=white
 *
 *	TTTTTTTTTTTTTT
 *	T             B
 *	T             B
 *	 BBBBBBBBBBBBBB
 */
void
nxDraw3dShadow(GR_DRAW_ID id,int x,int y,int w,int h,GR_COLOR crTop,
	GR_COLOR crBottom)
{
	GR_GC_ID	gc = GrNewGC();

	GrSetGCForeground(gc, crTop);
	/*MoveToEx( hDC, x, y+h-2, NULL);*/
	/*LineTo( hDC, x, y);*/				/* left side*/
	GrLine(id, gc, x, y+h-2, x, y);			/* left*/
	/*LineTo( hDC, x+w-1, y);*/			/* top side*/
	GrLine(id, gc, x, y, x+w-2, y);			/* top*/

	GrSetGCForeground(gc, crBottom);
	/*MoveToEx( hDC, x+w-1, y+1, NULL);*/
	/*LineTo( hDC, x+w-1, y+h-1);*/			/* right side*/
	GrLine(id, gc, x+w-1, y+1, x+w-1, y+h-2);	/* right*/
	/*LineTo( hDC, x, y+h-1);*/			/* bottom side*/
	GrLine(id, gc, x+w-1, y+h-1, x, y+h-1);		/* bottom*/

	GrDestroyGC(gc);
}

/*
 * nxDraw3dBox
 *
 *	TTTTTTTTTTTTTTB
 *	T             B
 *	T             B
 *	BBBBBBBBBBBBBBB
 */
void
nxDraw3dBox(GR_WINDOW_ID id,int x,int y,int w,int h,GR_COLOR crTop,
	GR_COLOR crBottom)
{
	GR_GC_ID	gc = GrNewGC();

	GrSetGCForeground(gc, crTop);
	/*MoveToEx( hDC, x, y+h-2, NULL);*/
	/*LineTo( hDC, x, y);*/				/* left side*/
	GrLine(id, gc, x, y+h-2, x, y+1);		/* left*/
	/*MoveToEx( hDC, x, y, NULL);*/
	/*LineTo( hDC, x+w-1, y);*/			/* top side*/
	GrLine(id, gc, x, y, x+w-2, y);			/* top*/

	GrSetGCForeground(gc, crBottom);
	GrLine(id, gc, x+w-1, y, x+w-1, y+h-2);		/* right*/
	/*MoveToEx( hDC, x+w-1, y, NULL);*/
	/*LineTo( hDC, x+w-1, y+h-1);*/			/* right side*/
	GrLine(id, gc, x+w-1, y+h-1, x, y+h-1);		/* bottom*/
	/*LineTo( hDC, x-1, y+h-1);*/			/* bottom side*/

	GrDestroyGC(gc);
}

/*
 * Draw 2 line deep 3d inset
 */
void
nxDraw3dInset(GR_DRAW_ID id,int x,int y,int w,int h)
{
	nxDraw3dBox(id, x, y, w, h,
		GrGetSysColor(GR_COLOR_BTNSHADOW),
		GrGetSysColor(GR_COLOR_BTNHIGHLIGHT));
	++x; ++y; w -= 2; h -= 2;
	nxDraw3dBox(id, x, y, w, h,
		GrGetSysColor(GR_COLOR_WINDOWFRAME),
		GrGetSysColor(GR_COLOR_3DLIGHT));
}

/*
 * Draw 2 line deep 3d outset
 */
void
nxDraw3dOutset(GR_DRAW_ID id,int x,int y,int w,int h)
{
	nxDraw3dBox(id, x, y, w, h,
		GrGetSysColor(GR_COLOR_3DLIGHT),
		GrGetSysColor(GR_COLOR_WINDOWFRAME));
	++x; ++y; w -= 2; h -= 2;
	nxDraw3dBox(id, x, y, w, h,
		GrGetSysColor(GR_COLOR_BTNHIGHLIGHT),
		GrGetSysColor(GR_COLOR_BTNSHADOW));
}

/*
 * Draw 1 line pushed down rectangle
 */
void
nxDraw3dPushDown(GR_DRAW_ID id, int x, int y, int w, int h)
{
	nxDraw3dBox(id, x, y, w, h, GrGetSysColor(GR_COLOR_BTNSHADOW),
		GrGetSysColor(GR_COLOR_BTNSHADOW));
}

/*
 * Draw either 3d up or down depending on state
 */
void
nxDraw3dUpDownState(GR_DRAW_ID id, int x, int y, int w, int h, GR_BOOL fDown)
{
	if (fDown)
		nxDraw3dPushDown(id, x, y, w, h);
	else nxDraw3dOutset(id, x, y, w, h);
}

#if 0
void
nxDraw3dUpFrame(GR_DRAW_ID id, int l, int t, int r, int b)
{
	RECT	rc;
	HBRUSH	hbr;

	SetRect(&rc, l, t, r, b);
	nxDraw3dBox(hDC, rc.left, rc.top,
		rc.right-rc.left, rc.bottom-rc.top,
		GrGetSysColor(GR_COLOR_3DLIGHT),
		GrGetSysColor(GR_COLOR_WINDOWFRAME));
	nxInflateRect(&rc, -1, -1);
	nxDraw3dBox(hDC, rc.left, rc.top,
		rc.right-rc.left, rc.bottom-rc.top,
		GrGetSysColor(GR_COLOR_BTNHIGHLIGHT),
		GrGetSysColor(GR_COLOR_BTNSHADOW));
	nxInflateRect(&rc, -1, -1);

	hbr = CreateSolidBrush(GrGetSysColor(GR_COLOR_APPWINDOW));
	FillRect(hDC, &rc, hbr);
	DeleteObject(hbr);
}
#endif
