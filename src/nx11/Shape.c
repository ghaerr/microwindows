#include "nxlib.h"
#include "X11/Xutil.h"
#include "X11/extensions/shape.h"

void
XShapeCombineMask(Display *dpy, Window dest, int destKind, int xOff, int yOff,
	Pixmap src, int op)
{
	GR_REGION_ID	mask;
	GR_WINDOW_INFO	info;

	if (destKind != ShapeBounding || op != ShapeSet)
	//if (destKind != ShapeClip || op != ShapeSet)
		return;

	if (src == None) {
		GrSetWindowRegion(dest, 0, 0);
		return;
	}

	GrGetWindowInfo(src, &info);
	mask = GrNewRegionFromPixmap(src, 0, 0, info.width, info.height);
	GrSetWindowRegion(dest, mask, mask);
	GrDestroyRegion(mask);
}

// from /usr/include/X11/extensions/shapestr.h
#define SHAPENAME "SHAPE"
#define SHAPE_MAJOR_VERSION	1	/* current version numbers */
#define SHAPE_MINOR_VERSION	1

Bool XShapeQueryExtension(Display *dpy, int *event_base, int *error_base)
{
/*	*event_base = LASTEvent +1;
	*error_base = 0;
	return 1;*/
	return XQueryExtension(dpy, SHAPENAME, NULL, event_base, error_base);
}

Status XShapeQueryVersion(Display *dpy, int *major, int *minor)
{
	*major = SHAPE_MAJOR_VERSION;
	*minor = SHAPE_MINOR_VERSION;
	return 1;
}
