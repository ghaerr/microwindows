#include "nxlib.h"

Window
XCreateSimpleWindow(Display * dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height,
	unsigned int borderWidth, unsigned long border,
	unsigned long background)
{
	if (parent == 0)
		parent = GR_ROOT_WINDOW_ID;

	return GrNewWindow(parent, x, y, width, height, borderWidth,
		_nxColorvalFromPixelval(dpy, background),
		_nxColorvalFromPixelval(dpy, border));
}
