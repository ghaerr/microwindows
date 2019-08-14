#include <string.h>
#include "nxlib.h"

Status
XGetWindowAttributes(Display * display, Window w, XWindowAttributes * ret)
{
	GR_WINDOW_INFO info;
	Window parent = 0;

	memset(ret, 0, sizeof(*ret));

	GrGetWindowInfo(w, &info);
	ret->x = info.x;
	ret->y = info.y;
	ret->width = info.width;
	ret->height = info.height;
	ret->border_width = info.bordersize;
	ret->depth = XDefaultDepth(display, 0);
	ret->visual = XDefaultVisual(display, 0);
	ret->class = ret->visual->class;
	ret->root = GR_ROOT_WINDOW_ID;
	ret->colormap = XDefaultColormap(display, 0);
	if (info.mapped)
		ret->map_state = info.realized? IsViewable: IsUnviewable;
	else ret->map_state = IsUnmapped;
	//FIXME ret->override_redirect = (info.props & GR_WM_PROPS_NODECORATE) != 0;
	ret->screen = &display->screens[0];
	/* We need to check if any parents are unmapped,
	* or we will report a window as mapped when it is not.	*/
	parent = info.parent;
	while (parent)
	{
	    GrGetWindowInfo(parent, &info);
	    if (info.mapped == 0)
		ret->map_state = IsUnmapped;
	    parent = info.parent;
	}
	
	return 1;
}
