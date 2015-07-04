#include "nxlib.h"

Bool
XQueryPointer(Display * display, Window w, Window *root, Window *child,
	int *root_x, int *root_y, int *win_x, int *win_y, unsigned int *mask)
{
	int x, y;
	GR_WINDOW_INFO winfo;

	*root = GR_ROOT_WINDOW_ID;

	/* FIXME: child should be None if not descendant of pointer window*/
	GrQueryPointer((GR_WINDOW_ID *) child, root_x, root_y, mask);

	/* convert window coords to absolute*/
	GrGetWindowInfo(w, &winfo);
	x = winfo.x;
	y = winfo.y;
	while (winfo.parent != 0) {
		GrGetWindowInfo(winfo.parent, &winfo);
		x += winfo.x;
		y += winfo.y;
	}

	/* return pointer as window relative coords*/
	*win_x = *root_x - x;
	*win_y = *root_y - y;

	return True;
}

static GR_WINDOW_ID
_checkWindowCoords(GR_WINDOW_ID win, int x, int y)
{
	GR_WINDOW_ID cwin;
	GR_WINDOW_INFO winfo;

	/* Go through each of the children and compare them */
	GrGetWindowInfo(win, &winfo);
	if (!winfo.child)
		return (win);

	cwin = winfo.child;

	do {
		GR_WINDOW_ID ret;
		GrGetWindowInfo(cwin, &winfo);

		if (x >= winfo.x && x <= winfo.x + winfo.width &&
		    y >= winfo.y && y <= winfo.x + winfo.height
		    && winfo.mapped) {
			ret = _checkWindowCoords(cwin, x - winfo.x,
						 y - winfo.y);
			if (ret)
				return (ret);
		}

		cwin = winfo.sibling;
	} while (cwin);

	return win;
}

Bool
XTranslateCoordinates(Display * display, Window src_w, Window dest_w,
		      int src_x, int src_y, int *dest_x_return,
		      int *dest_y_return, Window * child_return)
{
	int rx = src_x, ry = src_y;
	int dx = 0, dy = 0;
	GR_WINDOW_ID parent = (GR_WINDOW_ID) src_w;

	/* Get the root x and y of the src values */
	do {
		GR_WINDOW_INFO winfo;
		GrGetWindowInfo(parent, &winfo);

		rx += winfo.x;
		ry += winfo.y;

		if (parent == GR_ROOT_WINDOW_ID)
			break;
		parent = winfo.parent;
	} while (1);

	/* Now find the destination window absolute coordinates */
	parent = (GR_WINDOW_ID) dest_w;
	do {
		GR_WINDOW_INFO winfo;
		GrGetWindowInfo(parent, &winfo);

		dx += winfo.x;
		dy += winfo.y;

		if (parent == GR_ROOT_WINDOW_ID)
			break;
		parent = winfo.parent;
	} while (1);

	*dest_x_return = rx - dx;
	*dest_y_return = ry - dy;

	/* Now, see if this window is actually within any of our children */
	*child_return = _checkWindowCoords((GR_WINDOW_ID) dest_w,
				*dest_x_return, *dest_y_return);
	if (*child_return == dest_w)
		*child_return = None;

	return True;
}
