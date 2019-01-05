#include "nxlib.h"

int
XDrawLine (Display *dpy, Drawable d, GC gc, int x1, int y1, int x2, int y2)
{
	GrLine(d, gc->gid, x1, y1, x2, y2);
	return 1;
}
