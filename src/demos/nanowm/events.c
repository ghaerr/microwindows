/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
/* Uncomment this if you want debugging output from this file */
/*#define DEBUG*/

#include "nanowm.h"

void do_exposure(GR_EVENT_EXPOSURE *event)
{
	win *window;

	Dprintf("do_exposure: wid %d, x %d, y %d, width %d, height %d\n",
		event->wid, event->x, event->y, event->width, event->height);

	if(!(window = find_window(event->wid))) return;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			container_exposure(window, event);
			break;
		default:
			printf("Unhandled exposure on window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}

}

void do_button_down(GR_EVENT_BUTTON *event)
{
	win *window;

	Dprintf("do_button_down: wid %d, subwid %d, rootx %d, rooty %d, x %d, "
		"y %d, buttons %d, changebuttons %d, modifiers %d\n",
		event->wid, event->subwid, event->rootx, event->rooty, event->x,
		event->y, event->buttons, event->changebuttons,
		event->modifiers);

	if(!(window = find_window(event->wid))) return;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			container_buttondown(window, event);
			break;
		default:
			printf("Unhandled button down on window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}
}

void do_button_up(GR_EVENT_BUTTON *event)
{
	win *window;

	Dprintf("do_button_up: wid %d, subwid %d, rootx %d, rooty %d, x %d, "
		"y %d, buttons %d, changebuttons %d, modifiers %d\n",
		event->wid, event->subwid, event->rootx, event->rooty, event->x,
		event->y, event->buttons, event->changebuttons,
		event->modifiers);

	if(!(window = find_window(event->wid))) return;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			container_buttonup(window, event);
			break;
		default:
			printf("Unhandled button up on window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}
}

void do_mouse_enter(GR_EVENT_GENERAL *event)
{
	win *window;

	Dprintf("do_mouse_enter: wid %d\n", event->wid);

	if(!(window = find_window(event->wid)))
		return;

	switch(window->type) {
		default:
			printf("Unhandled mouse enter from window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}
}

void do_mouse_exit(GR_EVENT_GENERAL *event)
{
	win *window;

	Dprintf("do_mouse_exit: wid %d\n", event->wid);

	if(!(window = find_window(event->wid))) return;

	switch(window->type) {
		default:
			printf("Unhandled mouse exit from window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}
}

void do_mouse_moved(GR_EVENT_MOUSE *event)
{
	win *window;

	Dprintf("do_mouse_moved: wid %d, subwid %d, rootx %d, rooty %d, x %d, "
		"y %d, buttons %d, modifiers %d\n", event->wid, event->subwid,
		event->rootx, event->rooty, event->x, event->y, event->buttons,
		event->modifiers);

	if(!(window = find_window(event->wid))) return;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			container_mousemoved(window, event);
			break;
		default:
			printf("Unhandled mouse movement in window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}
}

void do_focus_in(GR_EVENT_GENERAL *event)
{
	win *window;

	printf("do_focus_in: wid %d\n", event->wid);

	if(!(window = find_window(event->wid)))
		return;

	switch(window->type) {
		default:
			printf("Unhandled focus in from window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}
}

void do_key_down(GR_EVENT_KEYSTROKE *event)
{
	Dprintf("do_key_down: wid %d, subwid %d, rootx %d, rooty %d, x %d, "
		"y %d, buttons %d, modifiers %d, uch %lu, special %d, "
		"ch %d, content %d\n", event->wid, event->subwid, event->rootx,
		event->rooty, event->x, event->y, event->buttons,
		event->modifiers, event->uch, event->special, event->ch,
		event->content);

	/* FIXME: Implement keyboard shortcuts */
}

void do_key_up(GR_EVENT_KEYSTROKE *event)
{
	Dprintf("do_key_up: wid %d, subwid %d, rootx %d, rooty %d, x %d, "
		"y %d, buttons %d, modifiers %d, uch %lu, special %d, "
		"ch %d, content %d\n", event->wid, event->subwid, event->rootx,
		event->rooty, event->x, event->y, event->buttons,
		event->modifiers, event->uch, event->special, event->ch,
		event->content);
}

void do_update(GR_EVENT_UPDATE *event)
{
	win *window;

	Dprintf("do_update: wid %d, subwid %d, x %d, y %d, width %d, height %d, "
		"utype %d\n", event->wid, event->subwid, event->x, event->y, event->width,
		event->height, event->utype);

	if(!(window = find_window(event->subwid))) {
		if (event->utype == GR_UPDATE_MAP)
			new_client_window(event->subwid);
		return;
	}

	if(window->type == WINDOW_TYPE_CONTAINER) {
		if (event->utype == GR_UPDATE_ACTIVATE)
			redraw_ncarea(window);
		return;
	}

	if(window->type != WINDOW_TYPE_CLIENT)
		return;

	if(event->utype == GR_UPDATE_DESTROY)
		client_window_destroy(window);
}
