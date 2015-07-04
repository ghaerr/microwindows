#include "nxlib.h"

int
XMapRaised (Display *dpy, Window w)
{
	GrMapWindow(w);
	GrRaiseWindow(w);
	return 1;
}
