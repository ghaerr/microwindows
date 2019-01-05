#include "nxlib.h"

int
XDrawPoint(Display * display, Drawable d, GC gc, int x, int y)
{
	GrPoint(d, gc->gid, x, y);
	return 1;
}

int
XDrawPoints(Display * display, Drawable d, GC gc,
	    XPoint * points, int npoints, int mode)
{

	int i;

	if (mode == CoordModeOrigin) {
		for (i = 0; i < npoints; i++) {
			GrPoint(d, gc->gid, points->x, points->y);
			++points;
		}
	} else {
		int prevx = 0, prevy = 0;

		for (i = 0; i < npoints; i++) {
			GrPoint(d, gc->gid, prevx + points->x, prevy + points->y);
			prevx += points->x;
			prevy += points->y;
			++points;
		}
	}

	return 1;
}
