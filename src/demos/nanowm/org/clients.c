/*
 * NanoWM- the NanoGUI window manager.
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include <nano-X.h>
/* Uncomment this if you want debugging output from this file */
#define DEBUG
#include "nanowm.h"

#define CONTAINER_MOVES	   0	/* mousedn on container moves*/

GR_COORD lastx = FIRST_WINDOW_LOCATION;
GR_COORD lasty = FIRST_WINDOW_LOCATION;

/*
 * A new client window has been mapped, so we need to reparent and decorate it.
 * Returns -1 on failure or 0 on success.
 */
int new_client_window(GR_WINDOW_ID wid)
{
	win window;
	GR_WINDOW_ID pid, nid;
	GR_WINDOW_INFO winfo;
	GR_COORD x, y, width, height;
	GR_WM_PROPERTIES props;
	struct clientinfo *ci;

	Dprintf("New client window %d\n", wid);

	/* get client window information*/
	GrGetWindowInfo(wid, &winfo);

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
		props.flags = GR_WM_FLAGS_BORDERSIZE;
		props.bordersize = 0;
		GrSetWMProperties(wid, &props);

		/* remap the window without borders, call this routine again*/
		GrMapWindow(wid);
		return 0;
	}
	
	width = (2 * BORDER_WIDTHS) + winfo.width;
	height = TITLE_BAR_HEIGHT + winfo.height + BORDER_WIDTHS;

	/* FIXME: We could proably use a more intelligent algorithm here */
	x = lastx + WINDOW_STEP;
	if((x + width) > si.cols)
		x = FIRST_WINDOW_LOCATION;
	lastx = x;
	y = lasty + WINDOW_STEP;
	if((y + height) > si.rows)
		y = FIRST_WINDOW_LOCATION;
	lasty = y;

	/* create container window*/
	pid = GrNewWindow(GR_ROOT_WINDOW_ID, x, y, width, height,
		0, LTGRAY, BLACK);
	window.wid = pid;
	window.pid = GR_ROOT_WINDOW_ID;
	window.type = WINDOW_TYPE_CONTAINER;
	window.active = 0;
	ci = malloc(sizeof(struct clientinfo));
	ci->cid = wid;
	window.data = ci;
	add_window(&window);

	GrSelectEvents(pid, GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_CHLD_UPDATE
#if CONTAINER_MOVES
			| GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_BUTTON_DOWN |
			GR_EVENT_MASK_MOUSE_POSITION
#endif
			);

	/* reparent client to container window*/
	GrReparentWindow(wid, pid, BORDER_WIDTHS, TITLE_BAR_HEIGHT);

	GrMapWindow(pid);

	/* add client window*/
	window.wid = wid;
	window.pid = pid;
	window.type = WINDOW_TYPE_CLIENT;
	window.active = 0;
	window.data = NULL;
	add_window(&window);

	/* add system utility button*/
	nid = GrNewWindow(pid, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0,
							LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_UTILITYBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE
				| GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
						utilitybutton_notpressed);

#if !CONTAINER_MOVES
	nid = GrNewWindow(pid, TITLE_BAR_HEIGHT + 1, 1, width - (4 *
			TITLE_BAR_HEIGHT) - 3, TITLE_BAR_HEIGHT - 3, 1, LTGRAY,
								BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_TOPBAR;
	window.active = GR_FALSE;
	window.data = NULL;

	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE
				| GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(nid);
#endif

	nid = GrNewWindow(pid, width - (3 * TITLE_BAR_HEIGHT), 0,
			TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0, LTGRAY, BLACK);
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_ICONISEBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE
				| GR_EVENT_MASK_MOUSE_EXIT);
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
	add_window(&window);

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
	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_EXPOSURE
				| GR_EVENT_MASK_MOUSE_EXIT);
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

	add_window(&window);

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


	add_window(&window);

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
	add_window(&window);

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

	add_window(&window);

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

	add_window(&window);

	GrSetCursor(nid, horizontal_resize_columns, horizontal_resize_rows,
			horizontal_resize_hotx, horizontal_resize_hoty,
			BLACK, WHITE, horizontal_resize_fg,
			horizontal_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
				| GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(nid);

	return 0;
}

/*
 * We've just received an event notifying us that a client window has been
 * unmapped, so we need to destroy all of the decorations.
 */
void client_window_unmapped(win *window)
{
	win *pwin;
	GR_WINDOW_ID pid;

	Dprintf("Client window %d has been unmapped\n", window->wid);

	if(!(pwin = find_window(window->pid))) {
		fprintf(stderr, "Couldn't find parent of unmapped window "
				"%d\n", window->wid);
		return;
	}

	/* Do it this way around so we don't handle events after destroying */
	pid = pwin->wid;
	remove_window_and_children(pwin);
	GrDestroyWindow(pid);
}

/*
 * The mouse has entered a window, so give it the focus.
 */
void setfocus(win *window)
{
}
