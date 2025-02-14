/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

static GR_COORD lastx = FIRST_WINDOW_LOCATION;
static GR_COORD lasty = FIRST_WINDOW_LOCATION;

/*
 * A new client window has been mapped, so we need to reparent and decorate it.
 * Returns -1 on failure or 0 on success.
 */
int wm_new_client_window(GR_WINDOW_ID wid)
{
	win window;
	GR_WINDOW_ID pid;
	GR_WINDOW_INFO winfo;
	GR_COORD x, y, width, height, xoffset, yoffset;
	GR_WM_PROPS style;
	GR_WM_PROPERTIES props;

	/* get client window information*/
	GrGetWindowInfo(wid, &winfo);
	style = winfo.props;

	/* if not redecorating or not child of root window, return*/
	if (winfo.parent != GR_ROOT_WINDOW_ID || (style & GR_WM_PROPS_NODECORATE))
		return 0;

	/* deal with replacing borders with window decorations*/
	if (winfo.bordersize) {
		/*
		 * For complex reasons, it's easier to unmap,
		 * remove the borders, and then map again,
		 * rather than try to recalculate the window
		 * position in the server w/o borders.  By
		 * the time we get this event, the window has
		 * already been painted with borders...
		 * This currently causes a screen flicker as
		 * the window is painted twice.  The workaround
		 * is to create the window without borders in
		 * the first place.
		 */
		GrUnmapWindow(wid);

		/* remove client borders, if any*/
		props.flags = style | GR_WM_FLAGS_BORDERSIZE;
		props.bordersize = 0;
		GrSetWMProperties(wid, &props);

		/* remap the window without borders, call this routine again*/
		GrMapWindow(wid);
		return 0;
	}
	
	/* if default decoration style asked for, set real draw bits*/
	if ((style & GR_WM_PROPS_APPMASK) == GR_WM_PROPS_APPWINDOW) {
		GR_WM_PROPERTIES pr;

		style = style | DEFAULT_WINDOW_STYLE;
		pr.flags = GR_WM_FLAGS_PROPS;
		pr.props = style;
		GrSetWMProperties(wid, &pr);
	}

	/* determine container widths and client child window offsets*/
	if (style & GR_WM_PROPS_APPFRAME) {
		width = winfo.width + CXFRAME;
		height = winfo.height + CYFRAME;
		xoffset = CXBORDER;
		yoffset = CYBORDER;
	} else if (style & GR_WM_PROPS_BORDER) {
		width = winfo.width + 2;
		height = winfo.height + 2;
		xoffset = 1;
		yoffset = 1;
	} else {
		width = winfo.width;
		height = winfo.height;
		xoffset = 0;
		yoffset = 0;
	}
	if (style & GR_WM_PROPS_CAPTION) {
		height += CYCAPTION;
		yoffset += CYCAPTION;
		if (style & GR_WM_PROPS_APPFRAME) {
			/* extra line under caption with appframe*/
			++height;
			++yoffset;
		}
	}

	/* determine x,y window location*/
	if (style & GR_WM_PROPS_NOAUTOMOVE) {
		x = winfo.x;
		y = winfo.y;
	} else {
		GR_SCREEN_INFO si;

		/* We could proably use a more intelligent algorithm here */
		GrGetScreenInfo(&si);
		x = lastx + WINDOW_STEP;
		if((x + width) > si.cols)
			x = FIRST_WINDOW_LOCATION;
		lastx = x;
		y = lasty + WINDOW_STEP;
		if((y + height) > si.rows)
			y = FIRST_WINDOW_LOCATION;
		lasty = y;
	}

	/* create container window*/
	pid = GrNewWindow(GR_ROOT_WINDOW_ID, x, y, width, height,
		0, LTGRAY, BLACK);
	window.wid = pid;
	window.pid = GR_ROOT_WINDOW_ID;
	window.type = WINDOW_TYPE_CONTAINER;
	window.sizing = GR_FALSE;
	window.active = 0;
	window.data = NULL;
	window.clientid = wid;
	wm_add_window(&window);

	/* don't erase background of container window*/
	props.flags = GR_WM_FLAGS_PROPS;
	props.props = (style & ~GR_WM_PROPS_BUFFERED) | GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties(pid, &props);


	GrSelectEvents(pid, GR_EVENT_MASK_CHLD_UPDATE
		| GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_BUTTON_DOWN
		| GR_EVENT_MASK_MOUSE_POSITION
		| GR_EVENT_MASK_MOUSE_ENTER | GR_EVENT_MASK_MOUSE_EXIT
		| GR_EVENT_MASK_EXPOSURE);


	/* add client window*/
	window.wid = wid;
	window.pid = pid;
	window.type = WINDOW_TYPE_CLIENT;
	window.sizing = GR_FALSE;
	window.active = 0;
	window.clientid = 0;
	window.data = NULL;
	wm_add_window(&window);

	/* reparent client to container window (child is already mapped)*/
	GrReparentWindow(wid, pid, xoffset, yoffset);

	/* map container window*/
	GrMapWindow(pid);

	GrSetFocus(wid);	/* force fixed focus*/

	return 0;
}

void wm_client_window_remap(win *window)
{
	GR_WINDOW_INFO winfo;
	win *pwin;

	if(!(pwin = wm_find_window(window->pid))) {
		return;
 	}

	GrGetWindowInfo(pwin->wid, &winfo);
	if (winfo.mapped == GR_FALSE)
		GrMapWindow(pwin->wid);
}

/* If the client chooses to unmap the window, then we should also unmap the container */
void wm_client_window_unmap(win *window)
{
	win *pwin;

	if(!(pwin = wm_find_window(window->pid))) {
		return;
	}

	if(pwin->active) {
	  struct pos_size * pos = (struct pos_size *)pwin->data;
	  free(pos);
	  pwin->active = GR_FALSE;
	  pwin->data = 0;
	}
	GrUnmapWindow(pwin->wid);
}

void
wm_client_window_resize(win *window)
{
	win *pwin;
	GR_COORD width, height;
	GR_WM_PROPS style;
	GR_WINDOW_INFO winfo;

	if(!(pwin = wm_find_window(window->pid))) {
		return;
	}

	/* get client window style and size, determine new container size*/
	GrGetWindowInfo(pwin->clientid, &winfo);
	style = winfo.props;

	if (style & GR_WM_PROPS_APPFRAME) {
		width = winfo.width + CXFRAME;
		height = winfo.height + CYFRAME;
	} else if (style & GR_WM_PROPS_BORDER) {
		width = winfo.width + 2;
		height = winfo.height + 2;
	} else {
		width = winfo.width;
		height = winfo.height;
	}
	if (style & GR_WM_PROPS_CAPTION) {
		height += CYCAPTION;
		if (style & GR_WM_PROPS_APPFRAME) {
			/* extra line under caption with appframe*/
			++height;
		}
	}

	/* resize container window based on client size*/
	GrResizeWindow(pwin->wid, width, height);
}

/*
 * We've just received an event notifying us that a client window has been
 * unmapped, so we need to destroy all of the decorations.
 */
void wm_client_window_destroy(win *window)
{
	win *pwin;
	GR_WINDOW_ID pid;


	if(!(pwin = wm_find_window(window->pid))) {
		return;
	}

	if(pwin->active) {
	  struct pos_size * pos = (struct pos_size *)pwin->data;
	  free(pos);
	  pwin->active = GR_FALSE;
	  pwin->data = 0;
	}

	/* Do it this way around so we don't handle events after destroying */
	pid = pwin->wid;
	wm_remove_window_and_children(pwin);

	GrDestroyWindow(pid);
}
