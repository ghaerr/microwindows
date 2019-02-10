#include "nxlib.h"
#include <stdlib.h>

int
XChangeWindowAttributes(Display * display, Window w, unsigned long valuemask,
			XSetWindowAttributes * attributes)
{
DPRINTF("XChangeWindowAttributes: valuemask 0x%X\n", (int)valuemask);

	if (valuemask & CWBackPixmap)	// 1
		XSetWindowBackgroundPixmap(display, w, attributes->background_pixmap);
	if (valuemask & CWBackPixel)	// 2
		XSetWindowBackground(display, w, attributes->background_pixel);

	if (valuemask & CWBorderPixmap)	// 4
		XSetWindowBorderPixmap(display, w, attributes->border_pixmap);
	if (valuemask & CWBorderPixel)	// 8
		XSetWindowBorder(display, w, attributes->border_pixel);

//http://karel.tsuda.ac.jp/lec/x/c1/
//	if (valuemask & CWBitGravity)	// 0x10
//		XSetWindowBorder(display, w, attributes->border_pixel);

	//CWSaveUnder
	if (valuemask & CWEventMask)
		XSelectInput(display, w, attributes->event_mask);
	//CWDontPropagate

	if (valuemask & CWColormap)
		XSetWindowColormap(display, w, attributes->colormap);
	if (valuemask & CWCursor)
		XDefineCursor(display, w, attributes->cursor);

	if (valuemask & CWOverrideRedirect) {
		GR_WM_PROPERTIES props;

		GrGetWMProperties(w, &props);
		if (props.title)
			free(props.title);

		props.flags = GR_WM_FLAGS_PROPS;
		if (attributes->override_redirect)
			props.props |= GR_WM_PROPS_NODECORATE;
		else props.props &= ~GR_WM_PROPS_NODECORATE;

		GrSetWMProperties(w, &props);
	}

	// FIXME handle additional attributes
	return 1;
}

int
XSetLineAttributes(Display * display, GC gc, unsigned int line_width,
		   int line_style, int cap_style, int join_style)
{
	unsigned long ls;

	switch (line_style) {
	case LineOnOffDash:
	case LineDoubleDash:
		/*ls = GR_LINE_DOUBLE_DASH;*/ /* nyi*/
		ls = GR_LINE_ONOFF_DASH;
		break;
	default:
		ls = GR_LINE_SOLID;
		break;
	}

	if (line_width > 1)
		DPRINTF("XSetLineAttributes: width %d\n", line_width);

	if (join_style != JoinMiter)
		DPRINTF("XSetLineAttributes: We don't support join style yet\n");

	GrSetGCLineAttributes(gc->gid, ls);
	return 1;
}
