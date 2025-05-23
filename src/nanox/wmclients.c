/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
#include "nxdraw.h"
#include "nanowm.h"

/*
 * A new client window has been mapped, so we need to reparent and decorate it.
 * Returns 0 if window not handled by window manager, otherwise 1.
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
		return 1;
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

#if NO_AUTO_MOVE
	x = MWABS(winfo.x - xoffset);
	y = MWABS(winfo.y - yoffset);
#else
	/* determine x,y window location*/
	if (style & GR_WM_PROPS_NOAUTOMOVE) {
		x = winfo.x;
		y = winfo.y;
	} else {
		GR_SCREEN_INFO si;
		static GR_COORD lastx = 0;
		static GR_COORD lasty = 0;

		/* We could proably use a more intelligent algorithm here */
		GrGetScreenInfo(&si);
		x = lastx + WINDOW_STEP;
		if((x + width) > si.cols)
			x = 0;
		lastx = x;
		y = lasty + WINDOW_STEP;
		if((y + height) > si.rows)
			y = 0;
		lasty = y;
	}
#endif
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

	Dprintf("New client window %d container %d\n", wid, pid);

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

#if 0000
	/* add system utility button*/
	nid = GrNewWindow(pid, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0,
							LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_UTILITYBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	wm_add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
						utilitybutton_notpressed);

	nid = GrNewWindow(pid, TITLE_BAR_HEIGHT + 1, 1,
		width - (4 * TITLE_BAR_HEIGHT) - 3, TITLE_BAR_HEIGHT - 3,
		1, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_TOPBAR;
	window.active = GR_FALSE;
	window.data = NULL;

	wm_add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(nid);

	nid = GrNewWindow(pid, width - (3 * TITLE_BAR_HEIGHT), 0,
			TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_ICONISEBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	wm_add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
						iconisebutton_notpressed);

	nid = GrNewWindow(pid, width - (2 * TITLE_BAR_HEIGHT), 0,
			TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_MAXIMISEBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	wm_add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE
				| GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
						maximisebutton_notpressed);

	nid = GrNewWindow(pid, width - TITLE_BAR_HEIGHT, 0,
			TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_CLOSEBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	wm_add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
						closebutton_notpressed);

	nid = GrNewWindow(pid, 1, TITLE_BAR_HEIGHT + 1, BORDER_WIDTHS - 2,
				height - TITLE_BAR_HEIGHT - BORDER_WIDTHS - 1,
				1, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_LEFTBAR;
	window.active = GR_FALSE;
	window.data = NULL;

	wm_add_window(&window);

	GrSetCursor(nid, horizontal_resize_columns, horizontal_resize_rows,
			horizontal_resize_hotx, horizontal_resize_hoty,
			BLACK, WHITE, horizontal_resize_fg,
			horizontal_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindow(pid, 1, height - BORDER_WIDTHS + 1, BORDER_WIDTHS - 2,
					BORDER_WIDTHS - 2, 1, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_LEFTRESIZE;
	window.active = GR_FALSE;
	window.data = NULL;

	wm_add_window(&window);

	GrSetCursor(nid, lefthand_resize_columns, lefthand_resize_rows,
			lefthand_resize_hotx, lefthand_resize_hoty,
			BLACK, WHITE, lefthand_resize_fg, lefthand_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindow(pid, BORDER_WIDTHS, height - BORDER_WIDTHS + 1,
			width - (2 * BORDER_WIDTHS), BORDER_WIDTHS - 2, 1,
							LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_BOTTOMBAR;
	window.active = GR_FALSE;
	window.data = NULL;
	wm_add_window(&window);

	GrSetCursor(nid, vertical_resize_columns, vertical_resize_rows,
			vertical_resize_hotx, vertical_resize_hoty,
			BLACK, WHITE, vertical_resize_fg, vertical_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindow(pid, width - BORDER_WIDTHS + 1,
			height - BORDER_WIDTHS + 1, BORDER_WIDTHS - 2,
					BORDER_WIDTHS - 2, 1, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_RIGHTRESIZE;
	window.active = GR_FALSE;
	window.data = NULL;

	wm_add_window(&window);

	GrSetCursor(nid, righthand_resize_columns, righthand_resize_rows,
			righthand_resize_hotx, righthand_resize_hoty,
			BLACK, WHITE, righthand_resize_fg, righthand_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindow(pid, width - BORDER_WIDTHS + 1, TITLE_BAR_HEIGHT + 1,
		BORDER_WIDTHS - 2, height - TITLE_BAR_HEIGHT - BORDER_WIDTHS -1,
							 1, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_RIGHTBAR;
	window.active = GR_FALSE;
	window.data = NULL;

	wm_add_window(&window);

	GrSetCursor(nid, horizontal_resize_columns, horizontal_resize_rows,
			horizontal_resize_hotx, horizontal_resize_hoty,
			BLACK, WHITE, horizontal_resize_fg,
			horizontal_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(nid);
#endif
	return 1;
}

void wm_client_window_remap(win *window)
{
	GR_WINDOW_INFO winfo;
	win *pwin;

	if(!(pwin = wm_find_window(window->pid))) {
		Dprintf("nanowm: Couldn't find parent of remapped window " "%d\n", window->wid);
		return;
 	}
	Dprintf("client_window_remap %d (parent %d)\n", window->wid, window->pid);

	GrGetWindowInfo(pwin->wid, &winfo);
	if (winfo.mapped == GR_FALSE)
		GrMapWindow(pwin->wid);
}

/* If the client chooses to unmap the window, then we should also unmap the container */
void wm_client_window_unmap(win *window)
{
	win *pwin;

	if(!(pwin = wm_find_window(window->pid))) {
    	Dprintf("nanowm: Couldn't find parent of unmapped window %d\n", window->wid);
		return;
	}

	if(pwin->active) {
	  struct pos_size * pos = (struct pos_size *)pwin->data;
#if OUTLINE_MOVE
	  GR_GC_ID gc;	  
	  gc = GrNewGC();
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	  GrRect(GR_ROOT_WINDOW_ID,gc,pos->xorig, pos->yorig, pos->width, pos->height);
	  GrDestroyGC(gc);
#endif
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

	Dprintf("client_window_resize %d (parent %d)\n", window->wid, window->pid);
	if(!(pwin = wm_find_window(window->pid))) {
		Dprintf("nanowm: Couldn't find parent of resized window %d\n", window->wid);
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

	Dprintf("nanowm: Client window %d has been destroyed\n", window->wid);

	if(!(pwin = wm_find_window(window->pid))) {
		Dprintf("nanowm: Couldn't find parent of destroyed window %d\n", window->wid);
		return;
	}

	if(pwin->active) {
	  struct pos_size * pos = (struct pos_size *)pwin->data;
#if OUTLINE_MOVE
	  GR_GC_ID gc;	  
	  gc = GrNewGC();
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	  GrRect(GR_ROOT_WINDOW_ID,gc,pos->xorig, pos->yorig, pos->width, pos->height);
	  GrDestroyGC(gc);
#endif
	  free(pos);
	  pwin->active = GR_FALSE;
	  pwin->data = 0;
	}

	/* Do it this way around so we don't handle events after destroying */
	pid = pwin->wid;
	wm_remove_window_and_children(pwin);

	Dprintf("Destroying container %d\n", pid);
	GrDestroyWindow(pid);
}
