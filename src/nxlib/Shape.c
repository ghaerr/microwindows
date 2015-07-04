#include "nxlib.h"
#include "X11/Xutil.h"
#include "X11/shape.h"	/* X11/extensions/shape.h*/

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
