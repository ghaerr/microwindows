#include "nxlib.h"

int
XDrawSegments(Display * dpy,
	      Drawable d, GC gc, XSegment * segments, int nsegments)
{
	int i;

	for (i = 0; i < nsegments; i++) {
		GrLine(d, gc->gid, segments[i].x1, segments[i].y1,
			segments[i].x2, segments[i].y2);
	}

	return 1;
}

int
XDrawLines(Display * dpy, Drawable d, GC gc, XPoint * points, int npoints,
	int mode)	// FIXME: mode ignored
{
	GR_POINT beg, end;

	if (npoints < 1)
		return 1;
	if (npoints == 1) {
		GrPoint(d, gc->gid, points->x, points->y);
		return 1;
	}

	/* must copy since X points are shorts, Nano-X are MWCOORDs (int) */
	beg.x = points->x;
	beg.y = points->y;
	++points;
	while (--npoints > 0) {
		end.x = points->x;
		end.y = points->y;
		++points;
		GrLine(d, gc->gid, beg.x, beg.y, end.x, end.y);
		beg = end;
	}
	return 1;
}
