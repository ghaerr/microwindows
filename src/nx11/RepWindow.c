#include "nxlib.h"

int
XReparentWindow(Display *dpy, Window w, Window p, int x, int y)
{
	GrReparentWindow(w, p, x, y);
	return 1;
}
