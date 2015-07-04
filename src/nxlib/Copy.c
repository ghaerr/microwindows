#include "nxlib.h"

int
XCopyArea(Display * display, Drawable src, Drawable dest, GC gc,
	int src_x, int src_y, unsigned int width, unsigned int height,
	int dest_x, int dest_y)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;
	int rop = _nxConvertROP(vp->function);

	// FIXME - use GC fg/bg for depth == 1 pixmaps
	GrCopyArea(dest, gc->gid, dest_x, dest_y, width, height, src, src_x,
		   src_y, rop);
	return 1;
}

int
XCopyPlane(Display * display, Drawable src, Drawable dest, GC gc,
	  int src_x, int src_y, unsigned int width, unsigned int height,
	  int dest_x, int dest_y, unsigned long plane)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;
	int rop = _nxConvertROP(vp->function);

	// FIXME: plane ignored
	if (plane != 0x00000001)
		DPRINTF("XCopyPlane: plane %x ignored\n", (int)plane);

#if 0
	/* test code instead of 1bpp pixmaps for sunclock...*/
	{
	GR_BITMAP *bitmap;
	GR_GC_ID newgc;
	GR_WINDOW_ID pixmap;
	GR_WINDOW_INFO info;

	// FIXME required for depth(src) == 1 ...
	GrGetWindowInfo(src, &info);
	bitmap = GrNewBitmapFromPixmap(src, 0, 0, info.width, info.height);
	newgc = GrNewGC();
	pixmap = GrNewPixmap(info.width, info.height, NULL);
	GrSetGCForeground(newgc, _nxColorvalFromPixelval(display, vp->foreground));
	GrSetGCBackground(newgc, _nxColorvalFromPixelval(display, vp->background));
	GrBitmap(pixmap, newgc, 0, 0, info.width, info.height, bitmap);

	GrCopyArea(dest, newgc, dest_x, dest_y, width, height, pixmap, src_x,
		   src_y, rop);

	GrDestroyGC(newgc);
	GrDestroyWindow(pixmap);
	free(bitmap);
	}
#else
	GrCopyArea(dest, gc->gid, dest_x, dest_y, width, height, src, src_x,
		   src_y, rop);
#endif
	return 1;
}
