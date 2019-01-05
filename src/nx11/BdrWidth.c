#include "nxlib.h"

int
XSetWindowBorderWidth(Display *dpy, Window w, unsigned int width)
{
	GrSetWindowBorderSize(w, width);
	return 1;
}
