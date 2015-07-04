#include "nxlib.h"

int
XDrawString16(Display *dpy, Drawable d, GC gc, int x, int y,
	_Xconst XChar2b *string, int length)
{   
printf("XDrawString16 %d %x %x\n", length, string->byte1, string->byte2);
	if (length > 0)
		GrText(d, gc->gid, x, y, (void *)string, length, 
		       GR_TFXCHAR2B|GR_TFBASELINE);
	return 0;
}

int
XDrawImageString16(Display *dpy, Drawable d, GC gc, int x, int y,
	_Xconst XChar2b *string, int length)
{
	if (length > 0) {
		GrSetGCUseBackground(gc->gid, GR_TRUE);
		GrText(d, gc->gid, x, y, (void *)string, length, 
		       GR_TFXCHAR2B|GR_TFBASELINE);
		GrSetGCUseBackground(gc->gid, GR_FALSE);
	}
	return 0;
}

