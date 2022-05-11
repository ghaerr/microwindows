#include "nxlib.h"

int
XDrawString16(Display *dpy, Drawable d, GC gc, int x, int y,
	_Xconst XChar2b *string, int length)
{   
	/*DPRINTF("XDrawString16 %d %x %x\n", length, string->byte1, string->byte2);*/
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

//http://xjman.dsl.gr.jp/X11R6/X11/CH08.html
int XDrawText16(Display *dpy, Drawable d, GC gc, int x, int y, XTextItem16 *items, int nitems)
{
	DPRINTF("XDrawText16 called\n");
//	item = items;
//	for (i=0; i < nitems; i++) {
	return XDrawString16(dpy, d, gc, x, y, items->chars, items->nchars);
}
