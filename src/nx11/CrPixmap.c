#include "nxlib.h"

Pixmap
XCreatePixmap(Display * dpy, Drawable d, unsigned int width,
	unsigned int height, unsigned int depth)
{
	/* Drawable ignored with nano-X */
	// FIXME depth ignored in pixmap
	if (depth == 1) DPRINTF("XCreatePixmap created with depth 1\n");
	return GrNewPixmapEx(width, height, 0, NULL);
}

int
XFreePixmap(Display * display, Pixmap pixmap)
{
	GrDestroyWindow(pixmap);
	return 1;
}
