#include "nxlib.h"

int
XUnmapWindow (Display *dpy, Window w)
{
	GrUnmapWindow(w);
	return 1;
}

Status
XWithdrawWindow(Display * display, Window w, int screen_number)
{
	GrUnmapWindow(w);
	return 1;
}
