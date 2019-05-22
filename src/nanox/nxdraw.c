/*
 * Copyright (c) 2000, 2019 Greg Haerr <greg@censoft.com>
 *
 * Nano-X Draw Library
 */
#define MWINCLUDECOLORS
#include <stdlib.h>
#include "nano-X.h"
#include "nanowm.h"
#include "nxdraw.h"

/*
 * GrGetSystemColor color scheme definitions
 */ 
#if NUKLEARUI
const GR_COLOR nxSysColors[MAXSYSCOLORS] = {
	/* desktop background*/
	GR_RGB(  0, 128, 128),  /* GR_COLOR_DESKTOP             */

	/* caption colors*/
	GR_RGB( 40,  40,  40),	/* GR_COLOR_ACTIVECAPTION       */
	GR_RGB(175, 175, 175),  /* GR_COLOR_ACTIVECAPTIONTEXT   */
	GR_RGB( 35,  35,  35),	/* GR_COLOR_INACTIVECAPTION     */
	GR_RGB(175, 175, 175),  /* GR_COLOR_INACTIVECAPTIONTEXT */

	/* 3d border shades (UNUSED in Nuklear)*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_3DFRAME             */
	GR_RGB(162, 141, 104),	/* GR_COLOR_BTNSHADOW           */
	GR_RGB(213, 204, 187),	/* GR_COLOR_3DLIGHT             */
	GR_RGB(234, 230, 221), 	/* GR_COLOR_BTNHIGHLIGHT        */

	/* top level application window backgrounds/text (FIXME REMOVE?)*/
	GR_RGB( 45,  45,  45),	/* GR_COLOR_APPWINDOW           */
	GR_RGB(175, 175, 175),  /* GR_COLOR_APPTEXT             */

	/* button control backgrounds/text (usually same as app window colors)*/
	GR_RGB( 50,  50,  50),	/* GR_COLOR_BTNFACE             */
	GR_RGB(175, 175, 175),  /* GR_COLOR_BTNTEXT             */

	/* edit/listbox control backgrounds/text, selected highlights*/
	GR_RGB( 45,  45,  45),  /* GR_COLOR_WINDOW              */
	GR_RGB(175, 175, 175),  /* GR_COLOR_WINDOWTEXT          */
	GR_RGB(128,   0,   0),  /* GR_COLOR_HIGHLIGHT           */
	GR_RGB(175, 175, 175),  /* GR_COLOR_HIGHLIGHTTEXT       */
	GR_RGB(175, 175, 175),  /* GR_COLOR_GRAYTEXT            */

	/* menu backgrounds/text*/
	GR_RGB( 40,  40,  40),	/* GR_COLOR_MENU                */
	GR_RGB(175, 175, 175),  /* GR_COLOR_MENUTEXT            */

	/* window border and interior line under caption*/
	GR_RGB( 65,  65,  65),	/* GR_COLOR_WINDOWFRAME         */
	GR_RGB( 65,  65,  65)   /* GR_COLOR_WINDOWFRAMELT       */
};
#endif

#ifdef SCHEME_TAN
const GR_COLOR nxSysColors[MAXSYSCOLORS] = {
	/* desktop background*/
	GR_RGB(  0, 128, 128),  /* GR_COLOR_DESKTOP             */

	/* caption colors*/
	GR_RGB(128,   0,   0),	/* GR_COLOR_ACTIVECAPTION       */
	GR_RGB(255, 255, 255),  /* GR_COLOR_ACTIVECAPTIONTEXT   */
	GR_RGB(162, 141, 104),	/* GR_COLOR_INACTIVECAPTION     */
	GR_RGB(192, 192, 192),  /* GR_COLOR_INACTIVECAPTIONTEXT */

	/* 3d border shades*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_3DFRAME             */
	GR_RGB(162, 141, 104),	/* GR_COLOR_BTNSHADOW           */
	GR_RGB(213, 204, 187),	/* GR_COLOR_3DLIGHT             */
	GR_RGB(234, 230, 221), 	/* GR_COLOR_BTNHIGHLIGHT        */

	/* top level application window backgrounds/text*/
	GR_RGB(255, 255, 255),	/* GR_COLOR_APPWINDOW           */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_APPTEXT             */

	/* button control backgrounds/text (usually same as app window colors)*/
	GR_RGB(213, 204, 187),	/* GR_COLOR_BTNFACE             */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_BTNTEXT             */

	/* edit/listbox control backgrounds/text, selected highlights*/
	GR_RGB(255, 255, 255),  /* GR_COLOR_WINDOW              */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWTEXT          */
	GR_RGB(128,   0,   0),  /* GR_COLOR_HIGHLIGHT           */
	GR_RGB(255, 255, 255),  /* GR_COLOR_HIGHLIGHTTEXT       */
	GR_RGB( 64,  64,  64),  /* GR_COLOR_GRAYTEXT            */

	/* menu backgrounds/text*/
	GR_RGB(213, 204, 187),	/* GR_COLOR_MENU                */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_MENUTEXT            */

	/* window border and interior line under caption*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWFRAME         */
	GR_RGB(213, 204, 187)	/* GR_COLOR_WINDOWFRAMELT       */
};
#endif

#ifdef SCHEME_WINSTD
const GR_COLOR nxSysColors[MAXSYSCOLORS] = {
	/* desktop background*/
	GR_RGB(  0, 128, 128),  /* GR_COLOR_DESKTOP             */

	/* caption colors*/
	GR_RGB(128,   0, 128),	/* GR_COLOR_ACTIVECAPTION       */
	GR_RGB(255, 255, 255),  /* GR_COLOR_ACTIVECAPTIONTEXT   */
	GR_RGB(128, 128, 128), 	/* GR_COLOR_INACTIVECAPTION     */
	GR_RGB(192, 192, 192),  /* GR_COLOR_INACTIVECAPTIONTEXT */

	/* 3d border shades*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_3DFRAME             */
	GR_RGB(128, 128, 128),	/* GR_COLOR_BTNSHADOW           */
	GR_RGB(223, 223, 223),	/* GR_COLOR_3DLIGHT             */
	GR_RGB(255, 255, 255), 	/* GR_COLOR_BTNHIGHLIGHT        */

	/* top level application window backgrounds/text*/
	GR_RGB(192, 192, 192),	/* GR_COLOR_APPWINDOW           */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_APPTEXT             */

	/* button control backgrounds/text (usually same as app window colors)*/
	GR_RGB(192, 192, 192),	/* GR_COLOR_BTNFACE             */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_BTNTEXT             */

	/* edit/listbox control backgrounds/text, selected highlights*/
	GR_RGB(255, 255, 255),  /* GR_COLOR_WINDOW              */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWTEXT          */
	GR_RGB(128,   0,   0),  /* GR_COLOR_HIGHLIGHT           */
	GR_RGB(255, 255, 255),  /* GR_COLOR_HIGHLIGHTTEXT       */
	GR_RGB( 64,  64,  64),  /* GR_COLOR_GRAYTEXT            */

	/* menu backgrounds/text*/
	GR_RGB(192, 192, 192),	/* GR_COLOR_MENU                */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_MENUTEXT            */

	/* window border and interior line under caption*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWFRAME         */
	GR_RGB(192, 192, 192)	/* GR_COLOR_WINDOWFRAMELT       */
};
#endif

#ifdef SCHEME_OLD
static const GR_COLOR sysColors[MAXSYSCOLORS] = {
	/* desktop background*/
	GR_RGB(  0, 128, 128),  /* GR_COLOR_DESKTOP             */

	/* caption colors*/
	GR_RGB(128,   0, 128),	/* GR_COLOR_ACTIVECAPTION       */
	GR_RGB(255, 255, 255),  /* GR_COLOR_ACTIVECAPTIONTEXT   */
	GR_RGB(  0,  64, 128),	/* GR_COLOR_INACTIVECAPTION     */
	GR_RGB(192, 192, 192),  /* GR_COLOR_INACTIVECAPTIONTEXT */

	/* 3d border shades*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_3DFRAME         */
	GR_RGB(128, 128, 128),	/* GR_COLOR_BTNSHADOW           */
	GR_RGB(192, 192, 192),	/* GR_COLOR_3DLIGHT             */
	GR_RGB(223, 223, 223), 	/* GR_COLOR_BTNHIGHLIGHT        */

	/* top level application window backgrounds/text*/
	GR_RGB(160, 160, 160),	/* GR_COLOR_APPWINDOW           */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_APPTEXT             */

	/* button control backgrounds/text (usually same as app window colors)*/
	GR_RGB(160, 160, 160),	/* GR_COLOR_BTNFACE             */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_BTNTEXT             */

	/* edit/listbox control backgrounds/text, selected highlights*/
	GR_RGB(255, 255, 255),  /* GR_COLOR_WINDOW              */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWTEXT          */
	GR_RGB(128,   0,   0),  /* GR_COLOR_HIGHLIGHT           */
	GR_RGB(255, 255, 255),  /* GR_COLOR_HIGHLIGHTTEXT       */
	GR_RGB( 64,  64,  64),  /* GR_COLOR_GRAYTEXT            */

	/* menu backgrounds/text*/
	GR_RGB(160, 160, 160),	/* GR_COLOR_MENU                */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_MENUTEXT            */

	/* window border and interior line under caption*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWFRAME         */
	GR_RGB(160, 160, 160)	/* GR_COLOR_WINDOWFRAMELT        */
};
#endif

void
nxPaintNCArea(GR_DRAW_ID id, int w, int h, char *title, GR_BOOL active, GR_WM_PROPS props)
{
	int		x = 0;
	int		y = 0;
	GR_GC_ID	gc = GrNewGC();
#if NUKLEARUI
	static GR_FONT_ID fontid = 0;
	if (!fontid)
		fontid = GrCreateFont(GR_FONT_SYSTEM_VAR, 0, NULL);
	GrSetGCFont(gc, fontid);
#endif

	if (props & GR_WM_PROPS_APPFRAME) {
#if NUKLEARUI
		/* draw 1-line black border around window*/
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_WINDOWFRAME));
		GrRect(id, gc, x, y, w, h);
		x += 1; y += 1; w -= 2; h -= 2;
#else
		/* draw 2-line 3d border around window*/
		nxDraw3dOutset(id, x, y, w, h);
		x += 2; y += 2; w -= 4; h -= 4;

		/* draw 1-line inset inside border*/
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_WINDOWFRAMELT));
		GrRect(id, gc, x, y, w, h);
		x += 1; y += 1; w -= 2; h -= 2;
#endif
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
		GrGetSysColor(active? GR_COLOR_ACTIVECAPTION: GR_COLOR_INACTIVECAPTION));
	GrFillRect(id, gc, x, y, w, CYCAPTION);

	/* draw caption text*/
	if (title) {
		GrSetGCForeground(gc,
			GrGetSysColor(active? GR_COLOR_ACTIVECAPTIONTEXT: GR_COLOR_INACTIVECAPTIONTEXT));
		GrSetGCUseBackground(gc, GR_FALSE);
#if NUKLEARUI
		/* X = 2 times padding (4)*/
		/* Y = 2 times padding (4) + font ascent+descent (11)*/
		GrText(id, gc, x+2*4, y+2*4+11, title, -1, GR_TFASCII|GR_TFBASELINE);
#else
		GrText(id, gc, x+4, y-1, title, -1, GR_TFASCII|GR_TFTOP);
#endif
	}
	y += CYCAPTION;

	/* draw one line under caption*/
	if (props & GR_WM_PROPS_APPFRAME) {
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_WINDOWFRAMELT));
		GrLine(id, gc, x, y, x+w-1, y);
	}

	if (props & GR_WM_PROPS_CLOSEBOX) {
#if NUKLEARUI
		GrSetGCForeground(gc,
			GrGetSysColor(active? GR_COLOR_ACTIVECAPTIONTEXT: GR_COLOR_INACTIVECAPTIONTEXT));
		GrSetGCUseBackground(gc, GR_FALSE);
		/* X = width - 3 - "x" width (5) - 2 times padding (4)*/
		/* Y = 2 times padding (4) + font ascent+descent (11)*/
		GrText(id, gc, x+w-3-5-8, y-CYCAPTION+8+11, "x", 1, GR_TFASCII|GR_TFBASELINE);
#else
		GR_RECT		r;
		/* draw close box*/
		r.x = x + w - CXCLOSEBOX - 2;
		r.y = y - CYCAPTION + 2;
		r.width = CXCLOSEBOX;
		r.height = CYCLOSEBOX;

		nxDraw3dBox(id, r.x, r.y, r.width, r.height,
			GrGetSysColor(GR_COLOR_BTNHIGHLIGHT),
			GrGetSysColor(GR_COLOR_3DFRAME));
		nxInflateRect(&r, -1, -1);
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_3DLIGHT));
		GrFillRect(id, gc, r.x, r.y, r.width, r.height);

		nxInflateRect(&r, -1, -1);
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_BTNTEXT));
		GrLine(id, gc, r.x, r.y, r.x+r.width-1, r.y+r.height-1);
		GrLine(id, gc, r.x, r.y+r.height-1, r.x+r.width-1, r.y);
#endif
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
		GrGetSysColor(GR_COLOR_3DFRAME),
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
		GrGetSysColor(GR_COLOR_3DFRAME));
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
		GrGetSysColor(GR_COLOR_3DFRAME));
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
