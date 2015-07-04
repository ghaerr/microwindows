#include "nxlib.h"

int
XResizeWindow(Display *dpy, Window w, unsigned int width, unsigned int height)
{
	GrResizeWindow(w, width, height);
	return 1;
}
