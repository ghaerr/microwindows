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
#include "nxdraw.h"
/* Uncomment this if you want debugging output from this file */
/*#define DEBUG*/

#include "nanowm.h"

/* uncomment this line to perform outline move operations*/
/*#define OUTLINE_MOVE*/

void redraw_ncarea(win *window)
{
	GR_WINDOW_INFO info;
	GR_WM_PROPERTIES props;
	GR_BOOL active;

	Dprintf("container_exposure window %d\n", window->wid);

	GrGetWindowInfo(window->wid, &info);

	GrGetWMProperties(window->clientid, &props);

	/*
	 * Check for invalid window.  This will be the
	 * case if the client exited, and we're just
	 * getting the paint notification for our parent.
	 */
	if (props.flags == 0)
		return;

	active = (window->clientid == GrGetFocus());
	nxPaintNCArea(window->wid, info.width, info.height, props.title,
		active, props.props);
}

void container_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("container_exposure window %d\n", window->wid);

	redraw_ncarea(window);
}

static GR_BOOL
PtInRect(GR_RECT *prc, GR_SIZE x, GR_SIZE y)
{
	return (x >= prc->x && x < (prc->x+prc->width) &&
		y >= prc->y && y < (prc->y+prc->height));
}

void container_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	struct pos_size *pos;
	GR_RECT		r;
	GR_COORD	cxborder = 0, cyborder = 0;
	GR_WINDOW_INFO	info;
	GR_GC_ID        gc;
	Dprintf("container_buttondown window %d\n", window->wid);

	if(window->active) return;

	GrGetWindowInfo(window->wid, &info);

	/* calc border sizes*/
	if (info.props & GR_WM_PROPS_BORDER) {
		cxborder = 1;
		cyborder = 1;
	}
	if (info.props & GR_WM_PROPS_APPFRAME) {
		cxborder = CXBORDER;
		cyborder = CYBORDER;
	}

	/* Check for close box press*/
	if ((info.props & (GR_WM_PROPS_CAPTION|GR_WM_PROPS_CLOSEBOX)) ==
	    (GR_WM_PROPS_CAPTION|GR_WM_PROPS_CLOSEBOX)) {

		/* Get close box rect*/
		r.x = info.width - CXCLOSEBOX - cxborder - 2;
		r.y = cyborder + 2;
		r.width = CXCLOSEBOX;
		r.height = CYCLOSEBOX;

		/* Check mousedn in close box*/
		if (PtInRect(&r, event->x, event->y)) {
			/* this may or not close the window...*/
			GrCloseWindow(window->clientid);
			return;
		}
	}

	/* Set focus on button down*/
	GrSetFocus(window->clientid);
/*
 * Note: Resize seems to cause lots of trouble since the resize "handle"
 * does not seem to be visible/advertized.  Thus at any touch, the window
 * may get resized and it is often impossible to recover
 */

	/* check for corner resize */
	r.x = info.width - 5;
	r.y = info.height - 5;
	r.width = 5;
	r.height = 5;

	if(PtInRect(&r,event->x, event->y)) {

	  struct pos_size * pos;

	  if(!window->data)
	    if(!(window->data = malloc(sizeof(struct pos_size)))) return;

	  window->sizing = GR_TRUE;
	  pos = (struct pos_size*)window->data;
	  
	  /* save off the width/height offset from the window manager */
	  GrGetWindowInfo(window->clientid,&info);
	  pos->xoff = -info.width;
	  pos->yoff = -info.height;

	  GrGetWindowInfo(window->wid,&info);
	  pos->xoff += info.width;
	  pos->yoff += info.height;

	  gc = GrNewGC();
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	  GrRect(GR_ROOT_WINDOW_ID,gc,info.x, info.y, info.width, info.height);
	  GrDestroyGC(gc);

	  /* save this rectangle's width/height so we can erase it later */
	  pos->width = info.width;
	  pos->height = info.height;

	  return;
	}

	/* if not in caption, return (FIXME, not calc'd exactly)*/
	if (!(info.props & GR_WM_PROPS_CAPTION))
		return;

	/* Get caption box rect*/
	r.x = cxborder;
	r.y = cyborder;
	r.width = info.width - cxborder*2;
	r.height = CYCAPTION;

	/* Check for mousedn in caption box*/
	if (!PtInRect(&r, event->x, event->y))
		return;

	/* Raise window if mouse down and allowed*/
	if (!(info.props & GR_WM_PROPS_NORAISE))
		GrRaiseWindow(window->wid);

	/* Don't allow window move if NOMOVE property set*/
	if (info.props & GR_WM_PROPS_NOMOVE)
		return;

	if(!window->data)
		if(!(window->data = malloc(sizeof(struct pos_size)))) return;

	pos = (struct pos_size *) window->data;

	GrGetWindowInfo(window->wid,&info);
	
	pos->xoff = event->x;
	pos->yoff = event->y;

#ifdef OUTLINE_MOVE
	pos->xorig = info.x;
	pos->yorig = info.y;
	pos->width = info.width;
	pos->height = info.height;

	gc = GrNewGC();
	GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	GrRect(GR_ROOT_WINDOW_ID,gc,info.x, info.y, info.width, info.height);
	GrDestroyGC(gc);
#endif	
	window->active = GR_TRUE;
}

void container_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("container_buttonup window %d\n", window->wid);

	if(window->active) {
	  struct pos_size * pos = (struct pos_size *)window->data;
#ifdef OUTLINE_MOVE
	  GR_GC_ID gc;	  
	  gc = GrNewGC();
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	  GrRect(GR_ROOT_WINDOW_ID,gc,pos->xorig, pos->yorig, pos->width, pos->height);

	  GrMoveWindow(window->wid, pos->xorig, pos->yorig);

#endif
	  free(pos);
	  window->active = GR_FALSE;
	  window->data = 0;
	}
	
	if(window->sizing) {
	  GR_WINDOW_INFO info;
	  GR_GC_ID gc;

	  struct pos_size * pos = (struct pos_size *)window->data;

	  gc = GrNewGC();
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);

	  GrGetWindowInfo(window->wid, &info);

	  GrRect(GR_ROOT_WINDOW_ID,gc,info.x, info.y, pos->width, pos->height);

	  GrResizeWindow(window->wid,event->rootx - info.x, event->rooty - info.y);
	  GrResizeWindow(window->clientid,event->rootx - info.x - pos->xoff, 
			 event->rooty - info.y - pos->yoff);
	  GrDestroyGC(gc);
	  free(window->data);
	  window->sizing = GR_FALSE;
	  window->data = 0;
	}
}

void container_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	struct pos_size *pos;
	GR_WINDOW_INFO info;
	GR_GC_ID gc;

	Dprintf("container_mousemoved window %d\n", window->wid);

	if(window->sizing) {

	  struct pos_size * pos = (struct pos_size*)window->data;
	  GrGetWindowInfo(window->wid, &info);

	  gc = GrNewGC();
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);

	  /* erase old rectangle */
	  GrRect(GR_ROOT_WINDOW_ID,gc,info.x, info.y, pos->width, pos->height);
	  /* draw new one */
	  GrRect(GR_ROOT_WINDOW_ID,gc,info.x, info.y, 
		 event->rootx - info.x, event->rooty - info.y);
	  GrDestroyGC(gc);

	  /* save this new rectangle's width, height */
	  /* I know, this shouldn't be stored in x/y, but... */
	  pos->width = event->rootx - info.x;
	  pos->height = event->rooty - info.y;

	  return;
	}

	if(!window->active) return;

	pos = (struct pos_size *) window->data;

#ifdef OUTLINE_MOVE
	gc = GrNewGC();
	GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	GrRect(GR_ROOT_WINDOW_ID,gc,pos->xorig, pos->yorig, pos->width, pos->height);
	GrRect(GR_ROOT_WINDOW_ID,gc,event->rootx - pos->xoff, event->rooty - pos->yoff,
	       pos->width, pos->height);

	pos->xorig = event->rootx - pos->xoff;
	pos->yorig = event->rooty - pos->yoff;
	
	GrDestroyGC(gc);
#else	
	GrMoveWindow(window->wid, event->rootx - pos->xoff,
		event->rooty - pos->yoff);
#endif
}

#if 0000
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


void topbar_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	Dprintf("topbar_buttonup window %d\n", window->wid);

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
#endif /* 0000*/
