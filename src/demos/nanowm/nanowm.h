/*
 * NanoWM- the NanoGUI window manager.
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

#ifndef __NANOWM_H
#define __NANOWM_H

#ifdef DEBUG
#define Dprintf printf
#else
#define Dprintf(ignore...)
#endif

/* Where to place the first window on the screen */
#define FIRST_WINDOW_LOCATION 2

/* The distance to leave between windows when deciding where to place */
#define WINDOW_STEP 20

/* The different window types which can be used in windowlist->type */
enum {
	WINDOW_TYPE_ROOT,
	WINDOW_TYPE_CONTAINER,
	WINDOW_TYPE_CLIENT
	/***WINDOW_TYPE_TOPBAR,
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
	WINDOW_TYPE_ROOTMENUENTRY**/
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
	int type;		/* What kind of window this is */
  int sizing;                   /* True if in the middle of a sizing request */
  int active;		/* Whether this window is active or not */
	void *data;		/* Data associated with this window */
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

#if 0000
/*
 * Used to record some general information about the client.
 */
struct clientinfo {
	GR_WINDOW_ID cid;
};
#endif

/* Function prototypes */
win *find_window(GR_WINDOW_ID wid);
int add_window(win *window);
int remove_window(win *window);
int remove_window_and_children(win *window);
int new_client_window(GR_WINDOW_ID wid);
void client_window_destroy(win *window);
void redraw_ncarea(win *window);
void do_exposure(GR_EVENT_EXPOSURE *event);
void do_button_down(GR_EVENT_BUTTON *event);
void do_button_up(GR_EVENT_BUTTON *event);
void do_mouse_enter(GR_EVENT_GENERAL *event);
void do_mouse_exit(GR_EVENT_GENERAL *event);
void do_mouse_moved(GR_EVENT_MOUSE *event);
void do_focus_in(GR_EVENT_GENERAL *event);
void do_key_down(GR_EVENT_KEYSTROKE *event);
void do_key_up(GR_EVENT_KEYSTROKE *event);
void do_focus_in(GR_EVENT_GENERAL *event);
void do_focus_out(GR_EVENT_GENERAL *event);
void do_update(GR_EVENT_UPDATE *event);
void do_chld_update(GR_EVENT_UPDATE *event);
void rootwindow_exposure(win *window, GR_EVENT_EXPOSURE *event);
void container_exposure(win *window, GR_EVENT_EXPOSURE *event);
void topbar_exposure(win *window, GR_EVENT_EXPOSURE *event);
void closebutton_exposure(win *window, GR_EVENT_EXPOSURE *event);
void maximisebutton_exposure(win *window, GR_EVENT_EXPOSURE *event);
void restorebutton_exposure(win *window, GR_EVENT_EXPOSURE *event);
void iconisebutton_exposure(win *window, GR_EVENT_EXPOSURE *event);
void utilitybutton_exposure(win *window, GR_EVENT_EXPOSURE *event);
void utilitymenu_exposure(win *window, GR_EVENT_EXPOSURE *event);
void utilitymenuentry_exposure(win *window, GR_EVENT_EXPOSURE *event);
void rootmenu_exposure(win *window, GR_EVENT_EXPOSURE *event);
void rootmenuentry_exposure(win *window, GR_EVENT_EXPOSURE *event);
void icon_exposure(win *window, GR_EVENT_EXPOSURE *event);
void rootwindow_buttondown(win *window, GR_EVENT_BUTTON *event);
void container_buttondown(win *window, GR_EVENT_BUTTON *event);
void topbar_buttondown(win *window, GR_EVENT_BUTTON *event);
void resizebar_buttondown(win *window, GR_EVENT_BUTTON *event);
void closebutton_buttondown(win *window, GR_EVENT_BUTTON *event);
void maximisebutton_buttondown(win *window, GR_EVENT_BUTTON *event);
void restorebutton_buttondown(win *window, GR_EVENT_BUTTON *event);
void iconisebutton_buttondown(win *window, GR_EVENT_BUTTON *event);
void utilitybutton_buttondown(win *window, GR_EVENT_BUTTON *event);
void icon_buttondown(win *window, GR_EVENT_BUTTON *event);
void rootwindow_buttonup(win *window, GR_EVENT_BUTTON *event);
void container_buttonup(win *window, GR_EVENT_BUTTON *event);
void topbar_buttonup(win *window, GR_EVENT_BUTTON *event);
void resizebar_buttonup(win *window, GR_EVENT_BUTTON *event);
void closebutton_buttonup(win *window, GR_EVENT_BUTTON *event);
void maximisebutton_buttonup(win *window, GR_EVENT_BUTTON *event);
void restorebutton_buttonup(win *window, GR_EVENT_BUTTON *event);
void iconisebutton_buttonup(win *window, GR_EVENT_BUTTON *event);
void utilitybutton_buttonup(win *window, GR_EVENT_BUTTON *event);
void icon_buttonup(win *window, GR_EVENT_BUTTON *event);
void utilitymenuentry_buttonup(win *window, GR_EVENT_BUTTON *event);
void rootmenuentry_buttonup(win *window, GR_EVENT_BUTTON *event);
void closebutton_mouseexit(win *window, GR_EVENT_GENERAL *event);
void maximisebutton_mouseexit(win *window, GR_EVENT_GENERAL *event);
void restorebutton_mouseexit(win *window, GR_EVENT_GENERAL *event);
void iconisebutton_mouseexit(win *window, GR_EVENT_GENERAL *event);
void utilitybutton_mouseexit(win *window, GR_EVENT_GENERAL *event);
void utilitymenu_mouseexit(win *window, GR_EVENT_GENERAL *event);
void utilitymenuentry_mouseexit(win *window, GR_EVENT_GENERAL *event);
void rootmenu_mouseexit(win *window, GR_EVENT_GENERAL *event);
void rootmenuentry_mouseexit(win *window, GR_EVENT_GENERAL *event);
void container_mousemoved(win *window, GR_EVENT_MOUSE *event);
void topbar_mousemoved(win *window, GR_EVENT_MOUSE *event);
void leftbar_mousemoved(win *window, GR_EVENT_MOUSE *event);
void leftresize_mousemoved(win *window, GR_EVENT_MOUSE *event);
void bottombar_mousemoved(win *window, GR_EVENT_MOUSE *event);
void rightresize_mousemoved(win *window, GR_EVENT_MOUSE *event);
void rightbar_mousemoved(win *window, GR_EVENT_MOUSE *event);

extern GR_SCREEN_INFO si;
extern win *windows;
extern GR_BITMAP utilitybutton_notpressed[];
extern GR_BITMAP utilitybutton_pressed[];
extern GR_BITMAP maximisebutton_notpressed[];
extern GR_BITMAP maximisebutton_pressed[];
extern GR_BITMAP iconisebutton_notpressed[];
extern GR_BITMAP iconisebutton_pressed[];
extern GR_BITMAP closebutton_notpressed[];
extern GR_BITMAP closebutton_pressed[];
extern GR_BITMAP restorebutton_notpressed[];
extern GR_BITMAP restorebutton_pressed[];
extern GR_BITMAP horizontal_resize_fg[];
extern GR_BITMAP horizontal_resize_bg[];
extern GR_BITMAP vertical_resize_fg[];
extern GR_BITMAP vertical_resize_bg[];
extern GR_BITMAP righthand_resize_fg[];
extern GR_BITMAP righthand_resize_bg[];
extern GR_BITMAP lefthand_resize_fg[];
extern GR_BITMAP lefthand_resize_bg[];
extern int horizontal_resize_columns, horizontal_resize_rows;
extern int horizontal_resize_hotx, horizontal_resize_hoty;
extern int vertical_resize_columns, vertical_resize_rows;
extern int vertical_resize_hotx, vertical_resize_hoty;
extern int lefthand_resize_columns, lefthand_resize_rows;
extern int lefthand_resize_hotx, lefthand_resize_hoty;
extern int righthand_resize_columns, righthand_resize_rows;
extern int righthand_resize_hotx, righthand_resize_hoty;

#endif
