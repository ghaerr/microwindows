#include "nxlib.h"

int
XSetInputFocus(Display *dpy, Window focus, int revert_to, Time time)
{
	GrSetFocus(focus);
	return 1;
}
