#include "nxlib.h"

int
XSetWindowBorder(Display *dpy, Window w, unsigned long pixel)
{
	GR_COLOR c = _nxColorvalFromPixelval(dpy, pixel);

	GrSetWindowBorderColor(w, c);
	return 1;
}
