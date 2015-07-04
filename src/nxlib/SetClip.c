#include "nxlib.h"
#include <stdlib.h>

int
XSetClipOrigin(Display *display, GC gc, int clip_x_origin, int clip_y_origin)
{
	//GR_GC_INFO	gcip;

	GrSetGCClipOrigin(gc->gid, clip_x_origin, clip_y_origin);

	//NOTE: Don't offset region here, this is done in GsPrepareDrawing
	//GrGetGCInfo(gc->gid, &gcip);
	//GrOffsetRegion(gcip.region, clip_x_origin, clip_y_origin);
	return 1;
}

int
XSetClipMask(Display *display, GC gc, Pixmap mask)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;
	GR_WINDOW_INFO	info;

	/* avoid memory leak by deleting current gc region before new alloc*/
	if (vp->clip_mask != None)
		GrDestroyRegion(vp->clip_mask);

	if (mask == None) {
		GrSetGCRegion(gc->gid, 0);
		vp->clip_mask = None;
		return 1;
	}

	GrGetWindowInfo(mask, &info);
	vp->clip_mask = GrNewRegionFromPixmap(mask, 0, 0, info.width, info.height);
	GrSetGCRegion(gc->gid, vp->clip_mask);

	return 1;
}

int
XSetClipRectangles(Display *display, GC gc, int clip_x_origin, int clip_y_origin,
	XRectangle *rectangles, int n, int ordering)
{
	GR_REGION_ID	r;
	XGCValues *vp = (XGCValues *)gc->ext_data;

	GrSetGCClipOrigin(gc->gid, clip_x_origin, clip_y_origin);

	/* avoid memory leak by deleting current gc region before new alloc*/
	if (vp->clip_mask != None)
		GrDestroyRegion(vp->clip_mask);

	if (n == 0) {
		/* FIXME need to disable all output here...*/
		DPRINTF("XSetClipRectangles called with n=0\n");
		vp->clip_mask = None;	/* None for no output?*/
		return 1;
	}

	r = GrNewRegion();
	while (--n >= 0) {
		GR_RECT rc;

		rc.x = rectangles->x;
		rc.y = rectangles->y;
		rc.width = rectangles->width;
		rc.height = rectangles->height;
		GrUnionRectWithRegion(r, &rc);
		++rectangles;
	}
	GrSetGCRegion(gc->gid, r);
	vp->clip_mask = r;

	return 1;
}
