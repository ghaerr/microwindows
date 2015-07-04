#define MWINCLUDECOLORS
#include "nxlib.h"

/*
 * XCreatePixmapFromBitmapData: Routine to make a pixmap from 
 * user supplied bitmap data.
 *
 * 	D is any drawable on the same screen that the pixmap will be used in.
 *	Data is a pointer to the bit data, and 
 *	width & height give the size in bits of the pixmap.
 *	Fg and Bg are the pixel values to use for the two colors.
 *	Depth is the depth of the pixmap to create.
 *
 * The following format is assumed for data:
 *
 *    format=XYPixmap
 *    bit_order=LSBFirst
 *    byte_order=LSBFirst
 *    padding=8
 *    bitmap_unit=8
 *    xoffset=0
 *    no extra bytes per line
 */  
Pixmap
XCreatePixmapFromBitmapData(Display *display, Drawable d, char *data,
	unsigned int width, unsigned int height,
	unsigned long fg, unsigned long bg, unsigned int depth)
{
	GR_COLOR fc, bc;

	// FIXME depth 1 bitmaps don't work, must store just 1 or 0
	if (depth == 1) {
		/* NOTE: colors reversed in mono bitmaps...*/
		fc = bg? WHITE: BLACK;
		bc = fg? WHITE: BLACK;
		//FIXME sunclock -fg/bg doesn't work...
		//fc = _nxColorvalFromPixelval(display, bg);
		//bc = _nxColorvalFromPixelval(display, fg);
	} else {
		fc = _nxColorvalFromPixelval(display, fg);
		bc = _nxColorvalFromPixelval(display, bg);
	}
DPRINTF("XCreatePixmapFromBitmapData %x,%x\n", (int)fc, (int)bc);

#if CPU_BIG_ENDIAN
	return GrNewPixmapFromData(width, height, fc, bc, (void *)data,
		GR_BMDATA_BYTEREVERSE);
#else
	return GrNewPixmapFromData(width, height, fc, bc, (void *)data,
		GR_BMDATA_BYTEREVERSE|GR_BMDATA_BYTESWAP);
#endif

#if 0
    XImage ximage;
    GC gc;
    XGCValues gcv;
    Pixmap pix;

    pix = XCreatePixmap(display, d, width, height, depth);
    gcv.foreground = fg;
    gcv.background = bg;
    if (! (gc = XCreateGC(display, pix, GCForeground|GCBackground, &gcv)))
	return (Pixmap) NULL;
    ximage.height = height;
    ximage.width = width;
    ximage.depth = 1;
    ximage.bits_per_pixel = 1;
    ximage.xoffset = 0;
    ximage.format = XYBitmap;
    ximage.data = data;
    ximage.byte_order = LSBFirst;
    ximage.bitmap_unit = 8;
    ximage.bitmap_bit_order = LSBFirst;
    ximage.bitmap_pad = 8;
    ximage.bytes_per_line = (width+7)/8;

    XPutImage(display, pix, gc, &ximage, 0, 0, 0, 0, width, height);
    XFreeGC(display, gc);
    return(pix);
#endif
}
