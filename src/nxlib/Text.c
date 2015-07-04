#include "nxlib.h"

int
XDrawString(Display *dpy, Drawable d, GC gc, int x, int y,
	_Xconst char *string, int length)
{   
	if (length > 0)
		GrText(d, gc->gid, x, y, (char *)string, length,
		       GR_TFASCII|GR_TFBASELINE);
	return 0;
}

int
XDrawImageString(Display *dpy, Drawable d, GC gc, int x, int y,
	_Xconst char *string, int length)
{
	if (length > 0) {
		GrSetGCUseBackground(gc->gid, GR_TRUE);
		GrText(d, gc->gid, x, y, (char *)string, length,
		       GR_TFASCII|GR_TFBASELINE);
		GrSetGCUseBackground(gc->gid, GR_FALSE);
	}
	return 0;
}
