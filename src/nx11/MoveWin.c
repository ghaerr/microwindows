#include "nxlib.h"

int
XMoveWindow(Display * dpy, Window w, int x, int y)
{
	GrMoveWindow(w, x, y);
	return 1;
}

int
XMoveResizeWindow(Display * display, Window w, int x, int y,
		  unsigned int width, unsigned int height)
{
	GrMoveWindow(w, x, y);
	GrResizeWindow(w, width, height);
	return 1;
}
