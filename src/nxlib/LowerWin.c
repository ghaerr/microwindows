#include "nxlib.h"

int
XLowerWindow(Display *dpy, Window w)
{
	GrLowerWindow(w);
	return 1;
}
