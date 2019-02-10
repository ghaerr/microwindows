#include "nxlib.h"
#include <stdlib.h>

/* Calculate the cursor from the given pixmap */
GR_CURSOR_ID
_nxCreateCursor(GR_WINDOW_ID cursor, GR_RECT * cbb,
		 GR_WINDOW_ID mask, GR_RECT * mbb,
		 int hotx, int hoty, GR_COLOR fg, GR_COLOR bg)
{
	int w, h;
	GR_BITMAP *bcursor, *bmask;
	GR_CURSOR_ID cursorid;

	/* force cursor size to max Microwindows size*/
	w = MWMIN(cbb->width, MWMAX_CURSOR_SIZE);
	h = MWMIN(cbb->height, MWMAX_CURSOR_SIZE);
	bcursor = GrNewBitmapFromPixmap(cursor, cbb->x, cbb->y, w, h);
	if (!bcursor)
		return 0;

	w = MWMIN(mbb->width, MWMAX_CURSOR_SIZE);
	h = MWMIN(mbb->height, MWMAX_CURSOR_SIZE);
	bmask = GrNewBitmapFromPixmap(mask, mbb->x, mbb->y, w, h);
	if (!bmask) {
		free(bcursor);
		return 0;
	}

	if (cbb->width > w || cbb->height > h)
		DPRINTF("nxCreateCursor: truncating original cursor (%d x %d)\n", cbb->width, cbb->height);

	/*
	 * Foreground/background reversed from nano-X !!!
	 */
	cursorid = GrNewCursor(w, h, hotx, hoty, bg, fg, bcursor, bmask);

	free(bcursor);
	free(bmask);

	return cursorid;
}

Cursor
XCreatePixmapCursor(Display * display, Pixmap source, Pixmap mask,
		    XColor * fg, XColor * bg, unsigned int x, unsigned y)
{
	Cursor ret;
	GR_RECT cbb, mbb;
	GR_WINDOW_INFO winfo;

	GrGetWindowInfo(source, &winfo);
	cbb.x = cbb.y = 0;
	cbb.width = winfo.width;
	cbb.height = winfo.height;

	GrGetWindowInfo(mask, &winfo);
	mbb.x = mbb.y = 0;
	mbb.width = winfo.width;
	mbb.height = winfo.height;

	ret = _nxCreateCursor(source, &cbb, mask, &mbb, x, y,
		       GR_RGB(fg->red >> 8, fg->green >> 8, fg->blue >> 8),
		       GR_RGB(bg->red >> 8, bg->green >> 8, bg->blue >> 8));

	return ret;
}

int
XFreeCursor(Display * display, Cursor cursor)
{
	GrDestroyCursor(cursor);
	return 1;
}
