#define MWINCLUDECOLORS
#include "nxlib.h"

#define AllMaskBits (CWBackPixmap|CWBackPixel|CWBorderPixmap|\
		     CWBorderPixel|CWBitGravity|CWWinGravity|\
		     CWBackingStore|CWBackingPlanes|CWBackingPixel|\
		     CWOverrideRedirect|CWSaveUnder|CWEventMask|\
		     CWDontPropagate|CWColormap|CWCursor)

Window
XCreateWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int borderWidth,
	int depth, unsigned int class, Visual *visual, unsigned long valuemask,
	XSetWindowAttributes *attributes)
{
	Window		wid;
	GR_COLOR	bkgnd = WHITE;
	GR_COLOR	border = BLACK;

	// FIXME: handle CopyFromParent for depth, class, visual

	if (parent == 0)
		parent = GR_ROOT_WINDOW_ID;

	if (class == InputOnly)
		wid = GrNewInputWindow((GR_WINDOW_ID)parent, x, y, width, height);
	else {
		if (valuemask & CWBackPixel)
			bkgnd = _nxColorvalFromPixelval(dpy, attributes->background_pixel);

		if (valuemask & CWBorderPixel)
			border = _nxColorvalFromPixelval(dpy, attributes->border_pixel);

		wid = GrNewWindow((GR_WINDOW_ID)parent, x, y, width, height, borderWidth,
			bkgnd, border);
	}
	if (!wid)
		return 0;

	/* if override_redirect set, assume popup-style window*/
	if ((valuemask & CWOverrideRedirect) && attributes->override_redirect) {
		GR_WM_PROPERTIES props;

		props.props = GR_WM_PROPS_NODECORATE;
		props.flags = GR_WM_FLAGS_PROPS;
		GrSetWMProperties(wid, &props);
	}

	if (valuemask & CWEventMask)
		XSelectInput(dpy, wid, attributes->event_mask);

	if (valuemask & CWCursor)
		XDefineCursor(dpy, wid, attributes->cursor);

    // FIXME add XCreateWindow valuemask attributes
    //if (valuemask & CWBackPixmap)
	//*value++ = attributes->background_pixmap;
    //if (vali uemask & CWBorderPixmap)
    	//*value++ = attributes->border_pixmap;
    //if (valuemask & CWBitGravity)
    	//*value++ = attributes->bit_gravity;
    //if (valuemask & CWWinGravity)
	//*value++ = attributes->win_gravity;
    //if (valuemask & CWBackingStore)
        //*value++ = attributes->backing_store;
    //if (valuemask & CWBackingPlanes)
	//*value++ = attributes->backing_planes;
    //if (valuemask & CWBackingPixel)
    	//*value++ = attributes->backing_pixel;
    //if (valuemask & CWSaveUnder)
    	//*value++ = attributes->save_under;
    //if (valuemask & CWDontPropagate)
	//*value++ = attributes->do_not_propagate_mask;
    //if (valuemask & CWColormap)
	//*value++ = attributes->colormap;

	return wid;
}
