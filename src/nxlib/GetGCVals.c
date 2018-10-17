//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"

// All gc fields except GCClipMask and GCDashList
#define ValidGCValuesBits (GCFunction | GCPlaneMask | GCForeground | \
			   GCBackground | GCLineWidth | GCLineStyle | \
			   GCCapStyle | GCJoinStyle | GCFillStyle | \
			   GCFillRule | GCTile | GCStipple | \
			   GCTileStipXOrigin | GCTileStipYOrigin | \
			   GCFont | GCSubwindowMode | GCGraphicsExposures | \
			   GCClipXOrigin | GCClipYOrigin | GCDashOffset | \
			   GCArcMode)

Status XGetGCValues(Display *dpy, GC gc, unsigned long valuemask, XGCValues *values)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;

	if (valuemask == ValidGCValuesBits) {
		char dashes = values->dashes;
		Pixmap clip_mask = values->clip_mask;
		*values = *vp;
		values->dashes = dashes;
		values->clip_mask = clip_mask;
		return True;
	}
	if (valuemask & ~ValidGCValuesBits) return False;

	if (valuemask & GCFunction)
		values->function = vp->function;

	if (valuemask & GCPlaneMask)
		values->plane_mask = vp->plane_mask;

	if (valuemask & GCForeground)
		values->foreground = vp->foreground;
	if (valuemask & GCBackground)
		values->background = vp->background;

	if (valuemask & GCLineWidth)
		values->line_width = vp->line_width;
	if (valuemask & GCLineStyle)
		values->line_style = vp->line_style;
	if (valuemask & GCCapStyle)
		values->cap_style = vp->cap_style;
	if (valuemask & GCJoinStyle)
		values->join_style = vp->join_style;
	if (valuemask & GCFillStyle)
		values->fill_style = vp->fill_style;
	if (valuemask & GCFillRule)
		values->fill_rule = vp->fill_rule;

	if (valuemask & GCTile)
		values->tile = vp->tile;
	if (valuemask & GCStipple)
		values->stipple = vp->stipple;

	if (valuemask & GCTileStipXOrigin)
		values->ts_x_origin = vp->ts_x_origin;
	if (valuemask & GCTileStipYOrigin)
		values->ts_y_origin = vp->ts_y_origin;

	if (valuemask & GCFont)
		values->font = vp->font;
	if (valuemask & GCSubwindowMode)
		values->subwindow_mode = vp->subwindow_mode;
	if (valuemask & GCGraphicsExposures)
		values->graphics_exposures = vp->graphics_exposures;

	if (valuemask & GCClipXOrigin)
		values->clip_x_origin = vp->clip_x_origin;
	if (valuemask & GCClipYOrigin)
		values->clip_y_origin = vp->clip_y_origin;

	if (valuemask & GCDashOffset)
		values->dash_offset = vp->dash_offset;
	if (valuemask & GCArcMode)
		values->arc_mode = vp->arc_mode;

	return True;
}
