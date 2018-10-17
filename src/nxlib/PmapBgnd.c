#include "nxlib.h"

int
XSetWindowBackgroundPixmap(Display * dpy, Window w, Pixmap pixmap)
{
	/* FIXME... if ParentRelative is specified, then we should use our */
	/* parent's background pixmap.  That would involve a copy???      */

DPRINTF("XSetWindowBackgroundPixmap %d %d\n", (int)w, (int)pixmap);
	if (pixmap == ParentRelative)
		return 1;
		/*GR_WINDOW_INFO wp;
		GrGetWindowInfo(w, &wp);
		GrGetWindowInfo(wp.parent, &wp);
		pixmap = wp.bgpixid;*/

	GrSetBackgroundPixmap(w, pixmap, GR_BACKGROUND_TILE);
	return 1;
}
