/*
 * NanoWM - The Nano-X window manager.
 * Copyright (C) 2000, 2010, 2019 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

#ifndef __NANOWM_H
#define __NANOWM_H

#include "mwconfig.h"

/* configurable options*/
#define OUTLINE_MOVE		0		/* draw outline only during window move*/
#define NO_CORNER_RESIZE	0		/* don't resize windows on corner drag*/
#define NO_AUTO_MOVE		0		/* don't auto position window on new windows*/
#define FIRST_WINDOW_LOCATION 2		/* x,y for first window*/
#define WINDOW_STEP			20	 	/* x,y step between new window placements*/
#define MAXSYSCOLORS		22		/* # of GR_COLOR_* system colors*/

/* default window style for GR_WM_PROPS_APPWINDOW*/
#define DEFAULT_WINDOW_STYLE	(GR_WM_PROPS_APPFRAME | GR_WM_PROPS_CAPTION | GR_WM_PROPS_CLOSEBOX)

/* nxPaintNCArea window drawing and color scheme*/
#if NUKLEARUI						/* draw window frames/colors to match Nuklear style*/
#define SCHEME_NUKLEAR				/* nuklear color scheme*/
#define CXBORDER	1				/* 3d border width*/
#define CYBORDER	1				/* 3d border height*/
#define CYCAPTION	29				/* height of caption*/
#define CXCLOSEBOX	20				/* width of closebox*/
#define CYCLOSEBOX	20				/* height of closebox*/
#define CXFRAME		(CXBORDER*2)	/* width of frame*/
#define CYFRAME		(CYBORDER*2)	/* height of frame*/
#else
#define SCHEME_TAN					/* tan color scheme*/
#define CXBORDER	3				/* 3d border width*/
#define CYBORDER	3				/* 3d border height*/
#define CYCAPTION	12				/* height of caption*/
#define CXCLOSEBOX	9				/* width of closebox*/
#define CYCLOSEBOX	9				/* height of closebox*/
#define CXFRAME		(CXBORDER*2)	/* width of frame*/
#define CYFRAME		(CYBORDER*2)	/* height of frame*/
#endif

#if DEBUG_NANOWM
#define Dprintf printf
#else
#define Dprintf(...)
#endif

/* GrGetSystemColor color scheme definitions*/
extern const GR_COLOR nxSysColors[MAXSYSCOLORS];

/* The different window types which can be used in windowlist->type */
enum {
	WINDOW_TYPE_ROOT,
	WINDOW_TYPE_CONTAINER,
	WINDOW_TYPE_CLIENT
	/*
	WINDOW_TYPE_TOPBAR,
	WINDOW_TYPE_LEFTBAR,
	WINDOW_TYPE_RIGHTBAR,
	WINDOW_TYPE_BOTTOMBAR,
	WINDOW_TYPE_LEFTRESIZE,
	WINDOW_TYPE_RIGHTRESIZE,
	WINDOW_TYPE_CLOSEBUTTON,
	WINDOW_TYPE_MAXIMISEBUTTON,
	WINDOW_TYPE_RESTOREBUTTON,
	WINDOW_TYPE_ICONISEBUTTON,
	WINDOW_TYPE_ICON,
	WINDOW_TYPE_UTILITYBUTTON,
	WINDOW_TYPE_UTILITYMENU,
	WINDOW_TYPE_UTILITYMENUENTRY,
	WINDOW_TYPE_ROOTMENU,
	WINDOW_TYPE_ROOTMENUENTRY
	*/
};

/* 
 * Used to keep a list of all the windows we know about so we can quickly
 * find out whether a window is "one of ours", and if it is, what kind of
 * window it is (title bar, side bar, button, icon, root menu, etc.), who
 * it's a child of, and any special data associated with it (the title
 * used in the title, the text of a root menu entry, the pixmap of an
 * icon, etc.).
 */
struct windowlist {
	GR_WINDOW_ID wid;	/* The ID of this window */
	GR_WINDOW_ID pid;	/* The ID of this window's parent */
	GR_WINDOW_ID clientid;	/* clientid for container window*/
	int type;			/* What kind of window this is */
	int sizing;			/* True if in the middle of a sizing request */
	int close;			/* Close button pressed*/
	int active;			/* Whether this window is active or not */
	void *data;			/* Data associated with this window */
	struct windowlist *next; /* The next window in the list */
};
typedef struct windowlist win;

/*
 * Used to record the offset position when performing a move.
 */
struct position {
	GR_COORD x;
	GR_COORD y;
};

/*
 * Used to record the original position, original size, and offset position
 * when performing a resize.
 */
struct pos_size {
	GR_COORD xoff;
	GR_COORD yoff;
	GR_COORD xorig;
	GR_COORD yorig;
	GR_SIZE width;
	GR_SIZE height;
};

/* Function prototypes */
win *wm_find_window(GR_WINDOW_ID wid);
int wm_add_window(win *window);
int wm_remove_window(win *window);
int wm_remove_window_and_children(win *window);

int wm_new_client_window(GR_WINDOW_ID wid);
void wm_client_window_resize(win *window);
void wm_client_window_destroy(win *window);
void wm_client_window_remap(win *window);
void wm_client_window_unmap(win *window);
void wm_redraw_ncarea(win *window);

int wm_exposure(GR_EVENT_EXPOSURE *event);
int wm_button_down(GR_EVENT_BUTTON *event);
int wm_button_up(GR_EVENT_BUTTON *event);
int wm_mouse_enter(GR_EVENT_GENERAL *event);
int wm_mouse_exit(GR_EVENT_GENERAL *event);
int wm_mouse_moved(GR_EVENT_MOUSE *event);
int wm_focus_in(GR_EVENT_GENERAL *event);
int wm_key_down(GR_EVENT_KEYSTROKE *event);
int wm_key_up(GR_EVENT_KEYSTROKE *event);
int wm_focus_in(GR_EVENT_GENERAL *event);
int wm_focus_out(GR_EVENT_GENERAL *event);
int wm_chld_update(GR_EVENT_UPDATE *event);
int wm_chld_update(GR_EVENT_UPDATE *event);

void wm_container_exposure(win *window, GR_EVENT_EXPOSURE *event);
void wm_container_buttondown(win *window, GR_EVENT_BUTTON *event);
void wm_container_buttonup(win *window, GR_EVENT_BUTTON *event);
void wm_container_mousemoved(win *window, GR_EVENT_MOUSE *event);
void wm_container_mouse_enter(win *window, GR_EVENT_GENERAL *event);
void wm_container_mouse_exit(win *window, GR_EVENT_GENERAL *event);

#endif /* __NANOWM_H*/
