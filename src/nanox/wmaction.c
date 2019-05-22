/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000, 2010 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
#include "nxdraw.h"
#include "nanowm.h"

static void set_resize_cursor(int wid);

/*
 * This function performs a depth first search on window 
 * and finds if any of its children has focus. 
 * i.e. wid equal to ref 
 * -- Amit Kulkarni
 */
static GR_BOOL
checkHasFocus(GR_WINDOW_ID wid,GR_WINDOW_ID ref)
{
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID childwid;
	if(wid==ref)
		return GR_TRUE;
	GrGetWindowInfo(wid, &winfo);

	/*Go down*/
	childwid = winfo.child;
	while (childwid)
	{
		if(checkHasFocus(childwid,ref)) 
			return GR_TRUE;
		GrGetWindowInfo (childwid,&winfo);

		/*Go right*/
		childwid=winfo.sibling;
	}
	/*Nah.. no match here*/
	return GR_FALSE;
}

void wm_redraw_ncarea(win *window)
{
	GR_WINDOW_INFO info;
	GR_WM_PROPERTIES props;
	GR_BOOL active;
	GR_WINDOW_ID focusedwindow;

	Dprintf("wm_container_exposure window %d\n", window->wid);

	GrGetWindowInfo(window->wid, &info);
	GrGetWMProperties(window->clientid, &props);

	/*
	 * Check for invalid window.  This will be the
	 * case if the client exited, and we're just
	 * getting the paint notification for our parent.
	 */
	if (props.flags) {
 		/* Get window with keyboard focus*/
 		focusedwindow = GrGetFocus();
 		/*
 	 	 * Check if the current window or any of its ancestors has the focus 
 	 	 * A window is active even if any of its ancestors has focus
 	 	 * --Amit Kulkarni
 	 	 */	 
 		active = checkHasFocus(window->clientid,focusedwindow);
 		/*
 	 	 * Note that we should also raise the window if its active and behind.
 	 	 * an active window deserves to be on the top :-)
 	 	 */
 		//if(active)
 			//GrRaiseWindow(window->wid);			// FIXME
		nxPaintNCArea(window->wid, info.width, info.height, props.title, active, props.props);
	}

	/* free title returned from GrGetWMProperties*/
	if (props.title)
		free(props.title);
}

void wm_container_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("wm_container_exposure window %d\n", window->wid);

	wm_redraw_ncarea(window);
}

static GR_BOOL
PtInRect(GR_RECT *prc, GR_SIZE x, GR_SIZE y)
{
	return (x >= prc->x && x < (prc->x+prc->width) &&
		y >= prc->y && y < (prc->y+prc->height));
}

void wm_container_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	struct pos_size *pos;
	GR_RECT		r;
	GR_COORD	cxborder = 0, cyborder = 0;
	GR_WINDOW_INFO	info;
	GR_WINDOW_INFO	cinfo;
	GR_GC_ID        gc;
	Dprintf("wm_container_buttondown window %d\n", window->wid);

	if(window->active)
		return;

	GrGetWindowInfo(window->wid, &info);
	GrGetWindowInfo(window->clientid, &cinfo);

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
			/* close on button up*/
			window->close = GR_TRUE;
      		return;
		}
	}

	/* 
	 * Set focus to the window only if client 
	 * or the container itself is clicked.
	 * if any of the  children (of the client)
	 * are clicked better not take the focus 
	 * away from them. They might require handling
	 * the focus themself.
	 * -- Amit Kulkarni
	 */
	if(window->wid==event->subwid)
		GrSetFocus(window->wid);

	/* check for corner resize */
	r.x = info.width - 5;
	r.y = info.height - 5;
	r.width = 5;
	r.height = 5;

	if (PtInRect(&r,event->x, event->y)
	   && !(info.props & GR_WM_PROPS_NORESIZE) && !(cinfo.props & GR_WM_PROPS_NORESIZE)) {
#if !NO_CORNER_RESIZE
	  struct pos_size * pos;

	  if(!window->data)
	    if(!(window->data = malloc(sizeof(struct pos_size))))
			return;

	  window->sizing = GR_TRUE;
	  
	  /* save off the width/height offset from the window manager */
	  GrGetWindowInfo(window->clientid,&info);
	  pos = (struct pos_size*)window->data;
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
      pos->xorig = event->x;
      pos->yorig = event->y;

	  /*
	   * This window is being resized.
	   * The client should have focus now.
	   * -- Amit Kulkarni
	   */
	  GrSetFocus(window->clientid);	  

	  return;
#endif /* !NO_CORNER_RESIZE*/
	} else
		GrSetWindowCursor(window->wid, 0);

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
	
	/*
	 * Now we have a click on the caption. 
	 * So window is active. 
	 * Set focus to the client
	 * --Amit Kulkarni
	 */	
	GrSetFocus(window->clientid);

	/* Raise window if mouse down and allowed*/
	if (!(info.props & GR_WM_PROPS_NORAISE) && !(cinfo.props & GR_WM_PROPS_NORAISE))
		GrRaiseWindow(window->wid);

	/* Don't allow window move if NOMOVE property set*/
	if ((info.props & GR_WM_PROPS_NOMOVE) || (cinfo.props & GR_WM_PROPS_NOMOVE))
		return;

	if(!window->data)
		if(!(window->data = malloc(sizeof(struct pos_size))))
			return;

	GrGetWindowInfo(window->wid,&info);
	pos = (struct pos_size *)window->data;
	pos->xoff = event->x;
	pos->yoff = event->y;

#if OUTLINE_MOVE
	pos->xorig = info.x;
	pos->yorig = info.y;
	pos->width = info.width;
	pos->height = info.height;

	gc = GrNewGC();
	GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	GrRect(GR_ROOT_WINDOW_ID, gc,info.x, info.y, info.width, info.height);
	GrDestroyGC(gc);
#endif	
	window->active = GR_TRUE;
}

void wm_container_buttonup(win *window, GR_EVENT_BUTTON *event)
{
	GR_RECT		r;
	GR_COORD	cxborder = 0, cyborder = 0;
	GR_WINDOW_INFO	info;

	Dprintf("wm_container_buttonup window %d\n", window->wid);

	GrGetWindowInfo(window->wid, &info);

	/* Check for close box press*/
	if ((info.props & (GR_WM_PROPS_CAPTION|GR_WM_PROPS_CLOSEBOX)) ==
	    (GR_WM_PROPS_CAPTION|GR_WM_PROPS_CLOSEBOX)) {

		/* calc border sizes*/
		if (info.props & GR_WM_PROPS_BORDER) {
			cxborder = 1;
			cyborder = 1;
		}
		if (info.props & GR_WM_PROPS_APPFRAME) {
			cxborder = CXBORDER;
			cyborder = CYBORDER;
		}

		/* Get close box rect*/
		r.x = info.width - CXCLOSEBOX - cxborder - 2;
		r.y = cyborder + 2;
		r.width = CXCLOSEBOX;
		r.height = CYCLOSEBOX;

		/* Check mouseup in close box*/
		if (PtInRect(&r, event->x, event->y)) {
			if(window->close == GR_TRUE) {
				/*
				 * This sends a CLOSE_REQ event to the window.
				 * NXLIB clients catch this and convert it
				 * to an X11 ClientMessage with a WM_DELETE_WINDOW
				 * atom, causing the window to close.
				 */
				GrCloseWindow(window->clientid);
        		window->close = GR_FALSE;
        		return;
      		}
		}
	}
	window->close = GR_FALSE;

	if(window->active) {
#if OUTLINE_MOVE
	  struct pos_size * pos = (struct pos_size *)window->data;
	  GR_GC_ID gc = GrNewGC();

	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	  GrRect(GR_ROOT_WINDOW_ID, gc, pos->xorig, pos->yorig, pos->width, pos->height);
	  GrMoveWindow(window->wid, pos->xorig, pos->yorig);
	  GrDestroyGC(gc);
#endif
	  free(window->data);
	  window->active = GR_FALSE;
	  window->data = 0;
	}
	
	if(window->sizing) {
	  struct pos_size * pos = (struct pos_size *)window->data;
	  GR_GC_ID gc = GrNewGC();
	  GR_WINDOW_INFO info;
	  GR_SIZE w, h;

	  GrGetWindowInfo(window->wid, &info);
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	  GrRect(GR_ROOT_WINDOW_ID, gc, info.x, info.y, pos->width, pos->height);
	  GrDestroyGC(gc);

	  w = info.width + (event->x - pos->xorig);
	  h = info.height + (event->y - pos->yorig);
	  GrResizeWindow(window->wid, w, h);
	  GrResizeWindow(window->clientid, w - pos->xoff, h - pos->yoff);

	  free(window->data);
	  window->sizing = GR_FALSE;
	  window->data = 0;
	}
}

void wm_container_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	struct pos_size *pos;
	GR_GC_ID gc;
	GR_RECT r;
	GR_WINDOW_INFO info;

	Dprintf("wm_container_mousemoved window %d\n", window->wid);

	GrGetWindowInfo(window->wid, &info);

	if(window->sizing) {
	  struct pos_size * pos = (struct pos_size*)window->data;

	  /* erase old rectangle */
	  gc = GrNewGC();
	  GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	  GrRect(GR_ROOT_WINDOW_ID, gc, info.x, info.y, pos->width, pos->height);

	  /* draw new one */
	  GrRect(GR_ROOT_WINDOW_ID,gc,info.x, info.y, event->rootx - info.x, event->rooty - info.y);
	  GrDestroyGC(gc);

	  /* save this new rectangle's width, height */
	  /* I know, this shouldn't be stored in x/y, but... */
	  pos->width = event->rootx - info.x;
	  pos->height = event->rooty - info.y;
	  return;
	}

#if !NO_CORNER_RESIZE
	/* check corner resize cursor on/off*/
	r.x = info.width - 5;
	r.y = info.height - 5;
	r.width = 5;
	r.height = 5;
	if (PtInRect(&r,event->x, event->y))
		set_resize_cursor(window->wid);
	else
		GrSetWindowCursor(window->wid, 0);
#endif

	if(!window->active)
		return;

	pos = (struct pos_size *)window->data;
#if OUTLINE_MOVE
	gc = GrNewGC();
	GrSetGCMode(gc, GR_MODE_XOR|GR_MODE_EXCLUDECHILDREN);
	GrRect(GR_ROOT_WINDOW_ID,gc,pos->xorig, pos->yorig, pos->width, pos->height);
	GrRect(GR_ROOT_WINDOW_ID,gc,event->rootx - pos->xoff, event->rooty - pos->yoff,
	       pos->width, pos->height);

	pos->xorig = event->rootx - pos->xoff;
	pos->yorig = event->rooty - pos->yoff;
	
	GrDestroyGC(gc);
#else	
	GrMoveWindow(window->wid, event->rootx - pos->xoff, event->rooty - pos->yoff);
#endif
}

void wm_container_mouse_enter(win *window, GR_EVENT_GENERAL *event)
{
	GR_RECT		r;
	GR_WINDOW_INFO info, cinfo;

#if !NO_CORNER_RESIZE
	/* Don't allow window move if NORESIZE property set*/
	GrGetWindowInfo(window->wid, &info);
	if (info.props & GR_WM_PROPS_NORESIZE)
		return;
	
	GrGetWindowInfo(window->clientid, &cinfo);
	if (cinfo.props & GR_WM_PROPS_NORESIZE)
		return;

	/* check for corner resize */
	r.x = info.width - 5;
	r.y = info.height - 5;
	r.width = 5;
	r.height = 5;
	if (PtInRect(&r,event->x, event->y))
		set_resize_cursor(window->wid);
#endif
}

void wm_container_mouse_exit(win *window, GR_EVENT_GENERAL *event)
{
	if (!window->sizing)
		GrSetWindowCursor(window->wid, 0);
}

/* cursor definition macros*/
#define	_	((unsigned) 0)		/* off bits */
#define	X	((unsigned) 1)		/* on bits */
#define	MASK(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
	((((((((((((((((((((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) + e) * 2) + f) * 2) + g) * 2)\
	       + h) * 2) + i) * 2) + j) * 2) + k) * 2) + l) * 2) + m) * 2) + n) * 2) + o) * 2) + p)

static void set_resize_cursor(int wid)
{
	static int resize_cursor = 0;

	if (!resize_cursor) {
		GR_BITMAP	resize_fg[16];
		GR_BITMAP	resize_bg[16];

		resize_fg[0] =  MASK(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_);
		resize_fg[1] =  MASK(_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_);
		resize_fg[2] =  MASK(_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_);
		resize_fg[3] =  MASK(_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_);
		resize_fg[4] =  MASK(_,X,X,_,X,_,_,_,_,_,_,_,_,_,_,_);
		resize_fg[5] =  MASK(_,X,_,_,_,X,_,_,_,_,_,_,_,_,_,_);
		resize_fg[6] =  MASK(_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_);
		resize_fg[7] =  MASK(_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_);
		resize_fg[8] =  MASK(_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_);
		resize_fg[9] =  MASK(_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_);
		resize_fg[10] = MASK(_,_,_,_,_,_,_,_,_,_,X,_,_,_,X,_);
		resize_fg[11] = MASK(_,_,_,_,_,_,_,_,_,_,_,X,_,X,X,_);
		resize_fg[12] = MASK(_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_);
		resize_fg[13] = MASK(_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_);
		resize_fg[14] = MASK(_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_);
		resize_fg[15] = MASK(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_);

		resize_bg[0] =  MASK(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_);
		resize_bg[1] =  MASK(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_);
		resize_bg[2] =  MASK(X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_);
		resize_bg[3] =  MASK(X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_);
		resize_bg[4] =  MASK(X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_);
		resize_bg[5] =  MASK(X,X,X,_,X,X,X,_,_,_,_,_,_,_,_,_);
		resize_bg[6] =  MASK(X,X,_,_,_,X,X,X,_,_,_,_,_,_,_,_);
		resize_bg[7] =  MASK(_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_);
		resize_bg[8] =  MASK(_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_);
		resize_bg[9] =  MASK(_,_,_,_,_,_,_,_,X,X,X,_,_,_,X,X);
		resize_bg[10] = MASK(_,_,_,_,_,_,_,_,_,X,X,X,_,X,X,X);
		resize_bg[11] = MASK(_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X);
		resize_bg[12] = MASK(_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X);
		resize_bg[13] = MASK(_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X);
		resize_bg[14] = MASK(_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X);
		resize_bg[15] = MASK(_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X);

		resize_cursor = GrNewCursor(16, 16, 8, 8, WHITE, BLACK,
			(MWIMAGEBITS *)resize_fg, (MWIMAGEBITS *)resize_bg);
	}

	GrSetWindowCursor(wid, resize_cursor);
}

#if 0000
void topbar_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	win *pwin = wm_find_window(window->pid);
	struct clientinfo *ci = pwin->data;
	GR_WM_PROPERTIES prop;

	Dprintf("topbar_exposure window %d\n", window->wid);

	GrGetWMProperties(ci->cid, &prop);
	if (prop.title) {
		GrText(window->wid, buttonsgc, 0, 0, prop.title, -1, GR_TFASCII|GR_TFTOP);
		free(prop.title);
	}
}

void closebutton_exposure(win *window, GR_EVENT_EXPOSURE *event)
{
	Dprintf("closebutton_exposure window %d\n", window->wid);

	GrBitmap(window->wid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
		window->active ? closebutton_pressed : closebutton_notpressed);
}

void topbar_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	struct position *pos;

	Dprintf("topbar_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	if(window->active)
		return;

	if(!window->data)
		if(!(window->data = malloc(sizeof(struct position))))
			return;

	pos = (struct position *) window->data;
	pos->x = event->x; //+ TITLE_BAR_HEIGHT;	/* actually width*/
	pos->y = event->y;

	window->active = GR_TRUE;
}

void resizebar_buttondown(win *window, GR_EVENT_BUTTON *event)
{
	GR_WINDOW_INFO wi;
	struct pos_size *pos;

	Dprintf("resizebar_buttondown window %d\n", window->wid);

	GrRaiseWindow(window->pid);

	if(window->active)
		return;

	if(!window->data)
		if(!(window->data = malloc(sizeof(struct pos_size))))
			return;

	GrGetWindowInfo(window->pid, &wi);
	pos = (struct pos_size *)window->data;
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
	win *pwin = wm_find_window(window->pid);
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

	if(!window->active)
		return;

	pos = (struct position *) window->data;

	/* turn off background erase draw while moving*/
	GrGetWMProperties(window->pid, &props);
	if (props.title)
		free(props.title);
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

	if(!window->active)
		return;

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
	if(!pos)return;

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

	if(!window->active)
		return;

	pos = (struct pos_size *)window->data;
	newheight = event->rooty - pos->yorig;
	GrResizeWindow(window->pid, pos->width, newheight);
}

void rightresize_mousemoved(win *window, GR_EVENT_MOUSE *event)
{
	GR_SIZE newwidth, newheight;
	struct pos_size *pos;

	Dprintf("rightresize_mousemoved window %d\n", window->wid);

	if(!window->active)
		return;

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

	if(!window->active)
		return;

	pos = (struct pos_size *) window->data;
	newwidth = event->rootx - pos->xorig;
	GrResizeWindow(window->pid, newwidth, pos->height);
}
#endif /* 0000*/
