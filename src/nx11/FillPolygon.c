#include <stdio.h>
#include <stdlib.h>
#include "uni_std.h"
#include "nxlib.h"

int
XFillPolygon(Display * display, Drawable d, GC gc,
	XPoint * points, int npoints, int shape, int mode)
{
	int i;
	GR_POINT *gr_points;

	gr_points = ALLOCA(npoints * sizeof(GR_POINT));

	if (mode == CoordModeOrigin) {
		for (i = 0; i < npoints; i++) {
			gr_points[i].x = points[i].x;
			gr_points[i].y = points[i].y;
		}
	} else {		/* CoordModePrevious */
		int px = 0, py = 0;
		for (i = 0; i < npoints; i++) {
			gr_points[i].x = px + points[i].x;
			gr_points[i].y = py + points[i].y;

			px += points[i].x;
			py += points[i].y;
		}
	}

//	if (shape == Complex || shape == Convex)
//		DPRINTF("XFillPolygon: Complex/Convex\n");

	GrFillPoly(d, gc->gid, npoints, gr_points);

	FREEA(gr_points);
	return 1;
}
