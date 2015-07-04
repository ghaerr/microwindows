#include "nxlib.h"

XPixmapFormatValues *
XListPixmapFormats(Display *dpy, int *count)
{
	XPixmapFormatValues *formats =
		(XPixmapFormatValues *)Xmalloc(sizeof(XPixmapFormatValues));

	if (formats) {
		ScreenFormat *sf = dpy->pixmap_format;
		formats->depth = sf->depth;
		formats->bits_per_pixel = sf->bits_per_pixel;
		formats->scanline_pad = sf->scanline_pad;
		*count = 1;
    	}
	return formats;
}
