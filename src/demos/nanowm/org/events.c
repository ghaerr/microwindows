/*
 * NanoWM- the NanoGUI window manager.
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

#include <stdio.h>
#include <stdlib.h>

#define MWINCLUDECOLORS
#include <nano-X.h>

/* Uncomment this if you want debugging output from this file */
/* #define DEBUG */
#include "nanowm.h"

void do_exposure(GR_EVENT_EXPOSURE *event)
{
	win *window;

	Dprintf("do_exposure: wid %d, x %d, y %d, width %d, height %d\n",
		event->wid, event->x, event->y, event->width, event->height);

	if(!(window = find_window(event->wid))) return;

	switch(window->type) {
		case WINDOW_TYPE_ROOT:
			rootwindow_exposure(window, event);
			break;
		case WINDOW_TYPE_TOPBAR:
			topbar_exposure(window, event);
			break;
		case WINDOW_TYPE_LEFTBAR:
		case WINDOW_TYPE_LEFTRESIZE:
		case WINDOW_TYPE_BOTTOMBAR:
		case WINDOW_TYPE_RIGHTRESIZE:
		case WINDOW_TYPE_RIGHTBAR:
			break;		/* Currently nothing to redraw */
		case WINDOW_TYPE_CLOSEBUTTON:
			closebutton_exposure(window, event);
			break;
		case WINDOW_TYPE_MAXIMISEBUTTON:
			maximisebutton_exposure(window, event);
			break;
		case WINDOW_TYPE_RESTOREBUTTON:
			restorebutton_exposure(window, event);
			break;
		case WINDOW_TYPE_ICONISEBUTTON:
			iconisebutton_exposure(window, event);
			break;
		case WINDOW_TYPE_UTILITYBUTTON:
			utilitybutton_exposure(window, event);
			break;
		case WINDOW_TYPE_UTILITYMENU:
			utilitymenu_exposure(window, event);
			break;
		case WINDOW_TYPE_UTILITYMENUENTRY:
			utilitymenuentry_exposure(window, event);
			break;
		case WINDOW_TYPE_ROOTMENU:
			rootmenu_exposure(window, event);
			break;
		case WINDOW_TYPE_ROOTMENUENTRY:
			rootmenuentry_exposure(window, event);
			break;
		case WINDOW_TYPE_ICON:
			icon_exposure(window, event);
			break;
		default:
			Dprintf("Unhandled exposure on window %d "
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
		case WINDOW_TYPE_ROOT:
			rootwindow_buttondown(window, event);
			break;
		case WINDOW_TYPE_CONTAINER:
			container_buttondown(window, event);
			break;
		case WINDOW_TYPE_TOPBAR:
			topbar_buttondown(window, event);
			break;
		case WINDOW_TYPE_LEFTBAR:
		case WINDOW_TYPE_LEFTRESIZE:
		case WINDOW_TYPE_BOTTOMBAR:
		case WINDOW_TYPE_RIGHTRESIZE:
		case WINDOW_TYPE_RIGHTBAR:
			resizebar_buttondown(window, event);
			break;
		case WINDOW_TYPE_CLOSEBUTTON:
			closebutton_buttondown(window, event);
			break;
		case WINDOW_TYPE_MAXIMISEBUTTON:
			maximisebutton_buttondown(window, event);
			break;
		case WINDOW_TYPE_RESTOREBUTTON:
			restorebutton_buttondown(window, event);
			break;
		case WINDOW_TYPE_ICONISEBUTTON:
			iconisebutton_buttondown(window, event);
			break;
		case WINDOW_TYPE_UTILITYBUTTON:
			utilitybutton_buttondown(window, event);
			break;
		case WINDOW_TYPE_ICON:
			icon_buttondown(window, event);
			break;
		default:
			Dprintf("Unhandled button down on window %d "
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
		case WINDOW_TYPE_ROOT:
			rootwindow_buttonup(window, event);
			break;
		case WINDOW_TYPE_CONTAINER:
			container_buttonup(window, event);
			break;
		case WINDOW_TYPE_TOPBAR:
			topbar_buttonup(window, event);
			break;
		case WINDOW_TYPE_LEFTBAR:
		case WINDOW_TYPE_LEFTRESIZE:
		case WINDOW_TYPE_BOTTOMBAR:
		case WINDOW_TYPE_RIGHTRESIZE:
		case WINDOW_TYPE_RIGHTBAR:
			resizebar_buttonup(window, event);
			break;
		case WINDOW_TYPE_CLOSEBUTTON:
			closebutton_buttonup(window, event);
			break;
		case WINDOW_TYPE_MAXIMISEBUTTON:
			maximisebutton_buttonup(window, event);
			break;
		case WINDOW_TYPE_RESTOREBUTTON:
			restorebutton_buttonup(window, event);
			break;
		case WINDOW_TYPE_ICONISEBUTTON:
			iconisebutton_buttonup(window, event);
			break;
		case WINDOW_TYPE_UTILITYBUTTON:
			utilitybutton_buttonup(window, event);
			break;
		case WINDOW_TYPE_ICON:
			icon_buttonup(window, event);
			break;
		case WINDOW_TYPE_UTILITYMENUENTRY:
			utilitymenuentry_buttonup(window, event);
			break;
		case WINDOW_TYPE_ROOTMENUENTRY:
			rootmenuentry_buttonup(window, event);
			break;
		default:
			Dprintf("Unhandled button up on window %d "
				"(type %d)\n", window->wid, window->type);
			break;
	}
}

void do_mouse_enter(GR_EVENT_GENERAL *event)
{
	win *window;

	Dprintf("do_mouse_enter: wid %d\n", event->wid);

	if(!(window = find_window(event->wid))) return;

	if(window->type == WINDOW_TYPE_CONTAINER) {
		setfocus(window);
		return;
	}

	if((window = find_window(window->pid))) {
		if(window->type == WINDOW_TYPE_CONTAINER) {
			setfocus(window);
			return;
		}
	}

	fprintf(stderr, "Can't find container to set focus on window %d\n",
							event->wid);
	
}

void do_mouse_exit(GR_EVENT_GENERAL *event)
{
	win *window;

	Dprintf("do_mouse_exit: wid %d\n", event->wid);

	if(!(window = find_window(event->wid))) return;

	switch(window->type) {
		case WINDOW_TYPE_CLOSEBUTTON:
			closebutton_mouseexit(window, event);
			break;
		case WINDOW_TYPE_MAXIMISEBUTTON:
			maximisebutton_mouseexit(window, event);
			break;
		case WINDOW_TYPE_RESTOREBUTTON:
			restorebutton_mouseexit(window, event);
			break;
		case WINDOW_TYPE_ICONISEBUTTON:
			iconisebutton_mouseexit(window, event);
			break;
		case WINDOW_TYPE_UTILITYBUTTON:
			utilitybutton_mouseexit(window, event);
			break;
		case WINDOW_TYPE_UTILITYMENU:
			utilitymenu_mouseexit(window, event);
			break;
		case WINDOW_TYPE_UTILITYMENUENTRY:
			utilitymenuentry_mouseexit(window, event);
			break;
		case WINDOW_TYPE_ROOTMENU:
			rootmenu_mouseexit(window, event);
			break;
		case WINDOW_TYPE_ROOTMENUENTRY:
			rootmenuentry_mouseexit(window, event);
			break;
		default:
			Dprintf("Unhandled mouse exit from window %d "
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
		case WINDOW_TYPE_TOPBAR:
			topbar_mousemoved(window, event);
			break;
		case WINDOW_TYPE_LEFTBAR:
			leftbar_mousemoved(window, event);
			break;
		case WINDOW_TYPE_LEFTRESIZE:
			leftresize_mousemoved(window, event);
			break;
		case WINDOW_TYPE_BOTTOMBAR:
			bottombar_mousemoved(window, event);
			break;
		case WINDOW_TYPE_RIGHTBAR:
			rightbar_mousemoved(window, event);
			break;
		case WINDOW_TYPE_RIGHTRESIZE:
			rightresize_mousemoved(window, event);
			break;
		default:
			Dprintf("Unhandled mouse movement in window %d "
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

	Dprintf("do_update: wid %d, x %d, y %d, width %d, height %d, "
		"utype %d\n", event->wid, event->x, event->y, event->width,
		event->height, event->utype);

	if(!(window = find_window(event->wid))) {
		new_client_window(event->wid);
		return;
	}

	if(window->type != WINDOW_TYPE_CLIENT) return;

	if(event->utype == GR_UPDATE_UNMAP) client_window_unmapped(window);
}

void do_chld_update(GR_EVENT_UPDATE *event)
{
	win *window;

	Dprintf("do_chld_update: wid %d, x %d, y %d, width %d, height %d, "
		"utype %d\n", event->wid, event->x, event->y, event->width,
		event->height, event->utype);

	if(!(window = find_window(event->wid))) return;

	if(window->type != WINDOW_TYPE_CLIENT) return;

	if(event->utype == GR_UPDATE_UNMAP) client_window_unmapped(window);
}
