#include "nxlib.h"

Status
XGetGeometry(Display *dpy, Drawable d, Window *root, int *x, int *y,
	unsigned int *width, unsigned int *height, unsigned int *borderWidth,
	unsigned int *depth)
{
	GR_WINDOW_INFO info;
	GR_SCREEN_INFO sinfo;

	GrGetWindowInfo(d, &info);
	GrGetScreenInfo(&sinfo);

	*root = GR_ROOT_WINDOW_ID;
	*x = info.x;
	*y = info.y;
	*width = info.width;
	*height = info.height;
	*borderWidth = info.bordersize;
	*depth = sinfo.bpp;

	return 1;
}
