#include "nxlib.h"

int
XClearWindow(Display *dpy, Window w)
{
	GrClearArea(w, 0, 0, 0, 0, GR_FALSE);
	return 1;
}
