#include "nxlib.h"

int
XRaiseWindow (Display *dpy, Window w)
{
	GrRaiseWindow(w);
	return 1;
}
