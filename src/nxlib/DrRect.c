#include "nxlib.h"

int
XDrawRectangle(Display *display, Drawable d, GC gc, int x, int y,
	unsigned int width, unsigned int height)
{
	/* X11 width/height is one less than Nano-X width/height*/
	GrRect(d, gc->gid, x, y, width+1, height+1);
	return 1;
}

int
XDrawRectangles(Display *display, Drawable d, GC gc, XRectangle *rect,
	int nrect)
{
	int i;

	for (i = 0; i < nrect; i++) {
		/* X11 width/height is one less than Nano-X width/height*/
		GrRect(d, gc->gid, rect->x, rect->y, rect->width+1,
		       rect->height+1);
		++rect;
	}
	return 1;
}
