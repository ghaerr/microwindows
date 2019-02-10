#include "nxlib.h"
#include <string.h>
#include <stdlib.h>
/*
 * XSetGCxxx routines
 *
 * Many of these routines save a local copy of the value
 * so that the value can be later queried or used by other
 * routines that need it without querying the server, which
 * may not support a query function.  This means that shared
 * GCs between processes may not work as expected...
 *
 * This also means that this library shouldn't call XSetGCxxx
 * routines to get work done, since the GC save value will
 * be incorrect.
 */

/* FIXME if these differ from NX defaults, must set at create time*/
static XGCValues initial_GC = {
	GXcopy,			/* function */
	AllPlanes,		/* plane_mask */
	0L,			/* foreground */
	1L,			/* background */
	0,			/* line_width */
	LineSolid,		/* line_style */
	CapButt,		/* cap_style */
	JoinMiter,		/* join_style */
	FillSolid,		/* fill_style */
	EvenOddRule,		/* fill_rule */
	ArcPieSlice,		/* arc_mode */
	(Pixmap)0,		/* tile, impossible (unknown) resource */
	(Pixmap)0,		/* stipple, impossible (unknown) resource */
	0,			/* ts_x_origin */
	0,			/* ts_y_origin */
	(Font)0,		/* font, impossible (unknown) resource */
	ClipByChildren,		/* subwindow_mode */
	True,			/* graphics_exposures */
	0,			/* clip_x_origin */
	0,			/* clip_y_origin */
	None,			/* clip_mask */
	0,			/* dash_offset */
	4			/* dashes (list [4,4]) */
};

static void
setupGC(Display * dpy, GC gc, unsigned long valuemask, XGCValues * values)
{
	if (valuemask & GCFunction)
		XSetFunction(dpy, gc, values->function);

	if (valuemask & GCForeground)
		XSetForeground(dpy, gc, values->foreground);

	if (valuemask & GCBackground)
		XSetBackground(dpy, gc, values->background);

	//FIXME add save gc->ext_data values for each of these...
	if (valuemask & GCFont)
		XSetFont(dpy, gc, values->font);

	if (valuemask & GCGraphicsExposures)
		XSetGraphicsExposures(dpy, gc, values->graphics_exposures);

	if ((valuemask & GCClipXOrigin) && (valuemask & GCClipYOrigin))
		XSetClipOrigin(dpy, gc, values->clip_x_origin,
		    values->clip_y_origin);

	if (valuemask & GCClipMask)
		XSetClipMask(dpy, gc, values->clip_mask);

	if (valuemask & GCFillStyle)
		XSetFillStyle(dpy, gc, values->fill_style);

	if ((valuemask & GCTileStipXOrigin) && (valuemask & GCTileStipYOrigin))
		XSetTSOrigin(dpy, gc, values->ts_x_origin, values->ts_y_origin);

	// FIXME
	if (valuemask & (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle))
		XSetLineAttributes(dpy, gc, values->line_width,
		    values->line_style, values->cap_style, values->join_style);

	if (valuemask & GCFillRule)
		XSetFillStyle(dpy, gc, values->fill_rule);

	if (valuemask & GCTile)
		XSetTile(dpy, gc, values->tile);

	if (valuemask & GCStipple)
		XSetStipple(dpy, gc, values->stipple);

	if (valuemask & (GCDashOffset | GCDashList)) {
		// FIXME is this correct for values->dashes?
		if (values->dashes) {
			char d[2];
			d[0] = d[1] = values->dashes;
			XSetDashes(dpy, gc, values->dash_offset, d, 2);
		}
	}

	if (valuemask & GCSubwindowMode)
		XSetSubwindowMode(dpy, gc, values->subwindow_mode);

	if (valuemask & GCPlaneMask)
		DPRINTF("XCreateGC: GCPlaneMask not implemented\n");

	if (valuemask & GCArcMode)
		DPRINTF("XCreateGC: GCArcMode not implemented\n");
}

/* note: unused Drawable d */
GC
XCreateGC(Display *dpy, Drawable d, unsigned long valuemask, XGCValues *values)
{
	GC gc;
	XGCValues *vp;

	if ((gc = (GC) Xmalloc(sizeof(struct _XGC))) == NULL)
		return NULL;
	if ((vp = (XGCValues *)Xmalloc(sizeof(XGCValues))) == NULL) {
		Xfree(gc);
		return NULL;
	}
	gc->ext_data = (XExtData *)vp;
	memcpy(vp, &initial_GC, sizeof(initial_GC));
	gc->gid = GrNewGC();

	/* X11 doesn't draw background, must set on all GrNewGC's*/
	GrSetGCUseBackground(gc->gid, GR_FALSE);

	/* X11 defaults to fg=black, bg=white, NX is opposite...*/
	if (!(valuemask & GCForeground))
		XSetForeground(dpy, gc, 0L);	/* black*/
	if (!(valuemask & GCBackground))
		XSetBackground(dpy, gc, ~0L);	/* white*/

	setupGC(dpy, gc, valuemask, values);
	return gc;
}

int
XChangeGC(Display *display, GC gc, unsigned long valuemask, XGCValues *values)
{
	setupGC(display, gc, valuemask, values);
	return 1;
}

void
XFlushGC(Display * dpy, GC gc)
{
	DPRINTF("XFlushGC called, not synced\n");
}

GContext
XGContextFromGC(GC gc)
{
	return gc->gid;
}

/*** XSetGCxxx routines ***/

/* convert an X11 rop to NX rop*/
int
_nxConvertROP(int Xrop)
{
	switch (Xrop) {
	case GXclear:
		return GR_MODE_CLEAR;
	case GXand:
		return GR_MODE_AND;
	case GXandReverse:
		return GR_MODE_ANDREVERSE;
	case GXcopy:
		return GR_MODE_COPY;
	case GXandInverted:
		return GR_MODE_ANDINVERTED;
	case GXnoop:
		return GR_MODE_NOOP;
	case GXxor:
		return GR_MODE_XOR;
	case GXor:
		return GR_MODE_OR;
	case GXequiv:
		return GR_MODE_EQUIV;
	case GXinvert:
		return GR_MODE_INVERT;
	case GXcopyInverted:
		return GR_MODE_COPYINVERTED;
	case GXorInverted:
		return GR_MODE_ORINVERTED;
	case GXnand:
		return GR_MODE_NAND;
	case GXset:
	default:
		return GR_MODE_COPY;
	}
}

int
XSetFunction(Display *dpy, GC gc, int function)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;
	int mode = _nxConvertROP(function);

	vp->function = function;

	/* must OR in clip mode when GrSetGCMode called*/
	if (vp->subwindow_mode == IncludeInferiors)
		mode |= GR_MODE_EXCLUDECHILDREN;

	GrSetGCMode(gc->gid, mode);
	return 1;
}

int
XSetSubwindowMode(Display *dpy, GC gc, int subwindow_mode)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;
	int mode;

	vp->subwindow_mode = subwindow_mode;
	mode = (subwindow_mode == IncludeInferiors)? GR_MODE_EXCLUDECHILDREN: 0;

	/* must OR in draw mode when GrSetGCMode called*/
	mode |= _nxConvertROP(vp->function);

	GrSetGCMode(gc->gid, mode);
	return 1;
}

int
XSetForeground(Display *dpy, GC gc, unsigned long foreground)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;
	GR_COLOR c = _nxColorvalFromPixelval(dpy, foreground);

	vp->foreground = foreground;
	GrSetGCForeground(gc->gid, c);

//DPRINTF("XSetForeground clr %x pix %x\n", (int)c, (int)foreground);
	return 1;
}

int
XSetBackground(Display *dpy, GC gc, unsigned long background)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;
	GR_COLOR c = _nxColorvalFromPixelval(dpy, background);

	vp->background = background;
	GrSetGCBackground(gc->gid, c);
	return 1;
}

int
XSetFont(Display *dpy, GC gc, Font font)
{
	GrSetGCFont(gc->gid, font);
	return 1;
}

int
XSetFillStyle(Display * dpy, GC gc, int fill_style)
{
	unsigned long mode = 0;

	switch (fill_style) {
	case FillTiled:
		mode = GR_FILL_TILE;
		break;

	case FillStippled:
		mode = GR_FILL_STIPPLE;
		break;

	case FillOpaqueStippled:
		mode = GR_FILL_OPAQUE_STIPPLE;
		break;

	default:
		mode = GR_FILL_SOLID;
		break;
	}

	GrSetGCFillMode(gc->gid, mode);
	return 1;
}

int
XSetGraphicsExposures(Display * display, GC gc, int graphics)
{
	GrSetGCGraphicsExposure(gc->gid, graphics);
	return 1;
}

int
XSetTSOrigin(Display * display, GC gc, int ts_x_origin, int ts_y_origin)
{
	/* FIXME REMOVED for FLTK test/bitmap*/
	/* it seems that XSetTSOrigin works differently than nano-X?*/
	/**GrSetGCTSOffset(gc->gid, ts_x_origin, ts_y_origin);**/
	return 1;
}

int
XSetTile(Display * display, GC gc, Pixmap tile)
{
	GR_WINDOW_INFO wi;

	GrGetWindowInfo(tile, &wi);
	GrSetGCTile(gc->gid, tile, wi.width, wi.height);
	return 1;
}

int
XSetStipple(Display * display, GC gc, Pixmap stipple)
{
	GR_WINDOW_INFO wi;
	GR_BITMAP *bitmap;

	GrGetWindowInfo(stipple, &wi);

	bitmap = GrNewBitmapFromPixmap(stipple, 0, 0, wi.width, wi.height);
	GrSetGCStipple(gc->gid, bitmap, wi.width, wi.height);
	free(bitmap);

	return 1;
}

int
XSetDashes(Display * display, GC gc, int dash_offset,
	   _Xconst char *dash_list, int n)
{
	if (dash_offset)
		DPRINTF("XSetDashes: dash offset not implemented\n"); 
	GrSetGCDash(gc->gid, (char *)dash_list, n);
	return 1;
}

#if 0
#define	GXclear			0x0		/* 0 */
#define GXand			0x1		/* src AND dst */
#define GXandReverse		0x2		/* src AND NOT dst */
#define GXcopy			0x3		/* src */
#define GXandInverted		0x4		/* NOT src AND dst */
#define	GXnoop			0x5		/* dst */
#define GXxor			0x6		/* src XOR dst */
#define GXor			0x7		/* src OR dst */
#define GXnor			0x8		/* NOT src AND NOT dst */
#define GXequiv			0x9		/* NOT src XOR dst */
#define GXinvert		0xa		/* NOT dst */
#define GXorReverse		0xb		/* src OR NOT dst */
#define GXcopyInverted		0xc		/* NOT src */
#define GXorInverted		0xd		/* NOT src OR dst */
#define GXnand			0xe		/* NOT src OR NOT dst */
#define GXset			0xf		/* 1 */
#endif
