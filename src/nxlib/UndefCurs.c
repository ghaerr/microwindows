#include "nxlib.h"

int
XUndefineCursor(Display *dpy, Window w)
{
	GrSetWindowCursor(w, 0);
	return 1;
}
