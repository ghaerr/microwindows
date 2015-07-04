#include "nxlib.h"

int
XSetWindowBackground(Display *dpy, Window w, unsigned long pixel)
{
	GR_COLOR c = _nxColorvalFromPixelval(dpy, pixel);

	GrSetWindowBackgroundColor(w, c);
	return 1;
}
