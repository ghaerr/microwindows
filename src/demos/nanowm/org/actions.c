/*
 * NanoWM- the NanoGUI window manager.
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

#include <stdio.h>
#include <stdlib.h>

#define MWINCLUDECOLORS
#include <nano-X.h>

/* Uncomment this if you want debugging output from this file */
//#define DEBUG
#include "nanowm.h"

void rootwindow_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("rootwindow_exposure window %d\n", window->wid);
}

void topbar_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	win *pwin = find_window(window->pid);
	struct clientinfo *ci = pwin->data;
	GR_WM_PROPERTIES prop;

	Dprintf("topbar_exposure window %d\n", window->wid);

	GrGetWMProperties(ci->cid, &prop);
	if (prop.title)
		GrText(window->wid, buttonsgc, 0, 0, prop.title, -1,
			GR_TFASCII|GR_TFTOP);
}

void closebutton_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("closebutton_exposure window %d\n", window->wid);

	GrBitmap(window->wid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT,
		TITLE_BAR_HEIGHT, window->active ? closebutton_pressed :
						closebutton_notpressed);
}

void maximisebutton_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("maximisebutton_exposure window %d\n", window->wid);

	GrBitmap(window->wid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT,
		TITLE_BAR_HEIGHT, window->active ? maximisebutton_pressed :
						maximisebutton_notpressed);
}

void restorebutton_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("restorebutton_exposure window %d\n", window->wid);

	GrBitmap(window->wid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT,
		TITLE_BAR_HEIGHT, window->active ? restorebutton_pressed :
						restorebutton_notpressed);
}

void iconisebutton_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("iconisebutton_exposure window %d\n", window->wid);

	GrBitmap(window->wid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT,
		TITLE_BAR_HEIGHT, window->active ? iconisebutton_pressed :
						iconisebutton_notpressed);
}

void utilitybutton_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("utilitybutton_exposure window %d\n", window->wid);

	GrBitmap(window->wid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT,
		TITLE_BAR_HEIGHT, window->active ? utilitybutton_pressed :
						utilitybutton_notpressed);
}

void utilitymenu_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("utilitymenu_exposure window %d\n", window->wid);
}

void utilitymenuentry_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("utilitymenuentry_exposure window %d\n", window->wid);
}

void rootmenu_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("rootmenu_exposure window %d\n", window->wid);
}

void rootmenuentry_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("rootmenuentry_exposure window %d\n", window->wid);
}

void icon_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("icon_exposure window %d\n", window->wid);
}

void rootwindow_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("rootwindow_buttondown window %d\n", window->wid);
}

void container_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	struct position *pos;

	Dprintf("container_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	if(window->active) return;

	if(!window->data)
		if(!(window->data = malloc(sizeof(struct position)))) return;

	pos = (struct position *) window->data;

	pos->x = event->x;
	pos->y = event->y;

	window->active = GR_TRUE;
}

void topbar_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	struct position *pos;

	Dprintf("topbar_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	if(window->active) return;

	if(!window->data)
		if(!(window->data = malloc(sizeof(struct position)))) return;

	pos = (struct position *) window->data;

	pos->x = event->x + TITLE_BAR_HEIGHT;	/* actually width*/
	pos->y = event->y;

	window->active = GR_TRUE;
}

void resizebar_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	GR_WINDOW_INFO wi;
	struct pos_size *pos;

	Dprintf("resizebar_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	if(window->active) return;

	if(!window->data)
		if(!(window->data = malloc(sizeof(struct pos_size)))) return;

	pos = (struct pos_size *) window->data;

	GrGetWindowInfo(window->pid, &wi);

	pos->xoff = event->x;
	pos->yoff = event->y;
	pos->xorig = wi.x;
	pos->yorig = wi.y;
	pos->width = wi.width;
	pos->height = wi.height;

	window->active = GR_TRUE;
}

void closebutton_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("closebutton_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	window->active = GR_TRUE;
	closebutton_exposure(window, NULL);
}

void maximisebutton_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("maximisebutton_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	window->active = GR_TRUE;
	maximisebutton_exposure(window, NULL);
}

void restorebutton_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("restorebutton_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	window->active = GR_TRUE;
	restorebutton_exposure(window, NULL);
}

void iconisebutton_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("iconisebutton_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	window->active = GR_TRUE;
	iconisebutton_exposure(window, NULL);
}

void utilitybutton_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("utilitybutton_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	window->active = GR_TRUE;
	utilitybutton_exposure(window, NULL);
}

void icon_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("icon_buttondown window %d\n", window->wid);
}

void rootwindow_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("rootwindow_buttonup window %d\n", window->wid);
}

void container_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("container_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
}

void topbar_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("topbar_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
}

void resizebar_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("resizebar_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
}

void closebutton_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	win *pwin = find_window(window->pid);
	struct clientinfo *ci = pwin->data;

	Dprintf("closebutton_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
	closebutton_exposure(window, NULL);

	GrCloseWindow(ci->cid);
}

void maximisebutton_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("maximisebutton_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
	maximisebutton_exposure(window, NULL);
}

void restorebutton_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("restorebutton_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
	restorebutton_exposure(window, NULL);
}

void iconisebutton_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("iconisebutton_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
	iconisebutton_exposure(window, NULL);
}

void utilitybutton_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("utilitybutton_buttonup window %d\n", window->wid);

	window->active = GR_FALSE;
	utilitybutton_exposure(window, NULL);
}

void icon_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("icon_buttonup window %d\n", window->wid);
}

void utilitymenuentry_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("utilitymenuentry_buttonup window %d\n", window->wid);
}

void rootmenuentry_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("rootmenuentry_buttonup window %d\n", window->wid);
}

void closebutton_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("closebutton_mouseexit window %d\n", window->wid);
}

void maximisebutton_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("maximisebutton_mouseexit window %d\n", window->wid);
}

void restorebutton_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("restorebutton_mouseexit window %d\n", window->wid);
}

void iconisebutton_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("iconisebutton_mouseexit window %d\n", window->wid);
}

void utilitybutton_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("utilitybutton_mouseexit window %d\n", window->wid);
}

void utilitymenu_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("utilitymenu_mouseexit window %d\n", window->wid);
}

void utilitymenuentry_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("utilitymenuentry_mouseexit window %d\n", window->wid);
}

void rootmenu_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("rootmenu_mouseexit window %d\n", window->wid);
}

void rootmenuentry_mouseexit(win *window, GR_EVENT_GENERAL *event)
{
	Dprintf("rootmenuentry_mouseexit window %d\n", window->wid);
}

void container_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	struct position *pos;

	Dprintf("container_mousemoved window %d\n", window->wid);

	if(!window->active) return;

	pos = (struct position *) window->data;

	GrMoveWindow(window->wid, event->rootx - pos->x,
			event->rooty - pos->y);
}

void topbar_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	struct position *pos;
	GR_WM_PROPERTIES props;

	Dprintf("topbar_mousemoved window %d\n", window->wid);

	if(!window->active) return;

	pos = (struct position *) window->data;

	/* turn off background erase draw while moving*/
	GrGetWMProperties(window->pid, &props);
	props.flags = GR_WM_FLAGS_PROPS;
	props.props |= GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties(window->pid, &props);

	GrMoveWindow(window->pid, event->rootx - pos->x,
			event->rooty - pos->y);

	props.props &= ~GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties(window->pid, &props);
}

void leftbar_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	GR_COORD newx;
	GR_SIZE newwidth;
	struct pos_size *pos;

	Dprintf("leftbar_mousemoved window %d\n", window->wid);

	if(!window->active) return;

	pos = (struct pos_size *) window->data;

	newx = event->rootx - pos->xoff;
	newwidth = pos->width + pos->xorig - event->rootx - pos->xoff;

	GrMoveWindow(window->pid, newx, pos->yorig);
	GrResizeWindow(window->pid, newwidth, pos->height);
}

void leftresize_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	GR_COORD newx;
	GR_SIZE newwidth, newheight;
	struct pos_size *pos;

	Dprintf("leftresize_mousemoved window %d\n", window->wid);

	if(!window->active) return;

	pos = (struct pos_size *) window->data;

	newx = event->rootx - pos->xoff;
	newheight = event->rooty - pos->yorig;
	newwidth = pos->width + pos->xorig - event->rootx - pos->xoff;

	GrMoveWindow(window->pid, newx, pos->yorig);
	GrResizeWindow(window->pid, newwidth, newheight);
}

void bottombar_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	GR_SIZE newheight;
	struct pos_size *pos;

	Dprintf("bottombar_mousemoved window %d\n", window->wid);

	if(!window->active) return;

	pos = (struct pos_size *) window->data;

	newheight = event->rooty - pos->yorig;

	GrResizeWindow(window->pid, pos->width, newheight);
}

void rightresize_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	GR_SIZE newwidth, newheight;
	struct pos_size *pos;

	Dprintf("rightresize_mousemoved window %d\n", window->wid);

	if(!window->active) return;

	pos = (struct pos_size *) window->data;

	newheight = event->rooty - pos->yorig;
	newwidth = event->rootx - pos->xorig;

	GrResizeWindow(window->pid, newwidth, newheight);
}

void rightbar_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	GR_SIZE newwidth;
	struct pos_size *pos;

	Dprintf("rightbar_mousemoved window %d\n", window->wid);

	if(!window->active) return;

	pos = (struct pos_size *) window->data;

	newwidth = event->rootx - pos->xorig;

	GrResizeWindow(window->pid, newwidth, pos->height);

}
