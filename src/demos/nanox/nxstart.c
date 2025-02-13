/*
 * nano-X launcher/window manager program
 * (C) 1999 Alistair Riddoch <ajr@ecs.soton.ac.uk>
 * (C) 2000 Alex Holden <alex@alexholden.net>
 */

/*
   Undefine this if solid window moves are incredibly slow on your hardware.
   Unfortunately since outline moves are not supported yet, the only
   alternative is "invisible" moving.
*/ 
#define WINDOW_MANAGER      0   /* = 1 for window manager code */
#define ROOT_WIN_RECOLOR    0   /* =1 to recolor desktop to gray */
#define SHOW_WINDOW_MOTION  1

/*
   Define this if you want the mouse pointer to become bell shaped when over
   the launcher window.
*/
#undef USE_WEIRD_POINTER

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "uni_std.h"
#define MWINCLUDECOLORS
#include "nano-X.h"

/*
 * Definitions to make it easy to define cursors
 */
#define	_	((unsigned) 0)		/* off bits */
#define	X	((unsigned) 1)		/* on bits */
#define	MASK(a,b,c,d,e,f,g) \
	(((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
	+ e) * 2) + f) * 2) + g) << 9)

#define DEC_HEIGHT 7
#define IDLE_DELAY 100000

static	GR_WINDOW_ID	w1;		/* id for launcher window */
static	GR_GC_ID	gc;		/* graphics context for rectangle */
static	GR_SCREEN_INFO	si;		/* information about screen */
static	int		fwidth, fheight;
static	int		fbase;
static	int		num_apps = 0;
#if ROOT_WIN_RECOLOR
static	GR_GC_ID	bgc;		/* graphics context for rectangle */
#endif

static void do_exposure(GR_EVENT_EXPOSURE *ep);
static void do_buttondown(GR_EVENT_BUTTON *ep);
static void do_buttonup(GR_EVENT_BUTTON *ep);
static void do_update(GR_EVENT_UPDATE *ep);
static void do_mouse(GR_EVENT_MOUSE *ep);

#if ELKS
#define PATH    "./"
#else
#define PATH    "bin/"
#endif

struct app_info {
	char		app_id[12];
	char		app_path[32];
} Apps[] = {
	{"clock",   PATH "nxclock"},
	{"term",    PATH "nxterm"},
	{"tetris",  PATH "nxtetris"},
	{"world",   PATH "nxworld"},
	{"landmine",PATH "nxlandmine"},
#if !ELKS
	{"aafont",    PATH "demo-aafont"},
	{"agg",       PATH "demo-agg"},
	{"calculator",PATH "demo-nuklear-calculator"},
	{"chess",     PATH "nxchess"},
	{"composite", PATH "demo-composite"},
	{"eyes",      PATH "nxeyes"},
	{"keyboard",  PATH "nxkbd"},
	{"magnifier", PATH "nxmag"},
	{"nuklear",   PATH "demo-nuklear-node_editor"},
	{"nuklear2",  PATH "demo-nuklear-overview"},
	{"roaches",   PATH "nxroach"},
	{"scribble",  PATH "nxscribble"},
	{"slider",    PATH "nxslider"},
#endif
	{"", ""}
};

typedef struct managed_window mwin;
struct managed_window {
	GR_WINDOW_ID	wid;	/* Application's window */
	GR_WINDOW_ID	fid;	/* Title bar */
	GR_COORD	x;	/* Overall window X origin */
	GR_COORD	y;	/* Overall window Y origin */
	GR_SIZE		width;	/* Overall width of window */
	mwin		* next;
};

mwin * mwins = NULL;
mwin * in_motion = NULL;
GR_COORD	move_xoff;
GR_COORD	move_yoff;

#ifndef WAIT_ANY
/* For Cygwin.  See:
 * http://www.opengroup.org/onlinepubs/007908799/xsh/wait.html
 */
#define WAIT_ANY (pid_t)-1
#endif

/*
 * Reap the dead children whenever we get a SIGCHLD.
 */
static void reaper(int signum) { while(waitpid(WAIT_ANY, NULL, WNOHANG) > 0); }

int
main(int argc,char **argv)
{
	GR_EVENT	event;		/* current event */
	struct app_info	* act;
	int		width = 8, height;

#ifdef USE_WEIRD_POINTER
	GR_BITMAP	bitmap1fg[7];	/* bitmaps for first cursor */
	GR_BITMAP	bitmap1bg[7];
#endif

	for(act = Apps; act->app_id[0] != '\0'; act++) {
		width = MWMAX(width, strlen(act->app_id));
		num_apps++;
	}

	if (GrOpen() < 0) {
		GrError("cannot open graphics\n");
		return 1;
	}
	
	GrGetScreenInfo(&si);

	signal(SIGCHLD, &reaper);
#if ROOT_WIN_RECOLOR
	bgc = GrNewGC();
	GrSetGCForeground(bgc, GRAY);
#endif
	gc = GrNewGC();
	GrSetGCFont(gc, GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL));
	GrGetGCTextSize(gc, "A", 1, GR_TFASCII, &fwidth, &fheight, &fbase);
	width = fwidth * width + 4;
	height = fheight * num_apps + 4;

	w1 = GrNewWindowEx(GR_WM_PROPS_NOAUTOMOVE, NULL, GR_ROOT_WINDOW_ID,
		si.cols-width-10, 5, width, height, WHITE);

	GrSelectEvents(w1, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_DOWN |
			GR_EVENT_MASK_CLOSE_REQ);
	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE|
			GR_EVENT_MASK_CHLD_UPDATE);

	GrMapWindow(w1);

#ifdef USE_WEIRD_POINTER
	bitmap1bg[0] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[1] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[2] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[3] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[4] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[5] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[6] = MASK(X,X,X,X,X,X,X);

	bitmap1fg[0] = MASK(_,_,_,_,_,_,_);
	bitmap1fg[1] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[2] = MASK(_,_,X,X,X,_,_);
	bitmap1fg[3] = MASK(_,_,X,X,X,_,_);
	bitmap1fg[4] = MASK(_,_,X,X,X,_,_);
	bitmap1fg[5] = MASK(_,_,X,X,X,_,_);
	bitmap1fg[6] = MASK(_,X,X,X,X,X,_);

	GrSetCursor(w1, 7, 7, 3, 3, WHITE, BLACK, bitmap1fg, bitmap1bg);
#endif

#if ROOT_WIN_RECOLOR
	GrFillRect(GR_ROOT_WINDOW_ID, bgc, 0, 0, si.cols, si.rows);
#endif
	GrSetGCForeground(gc, BLACK);
	GrSetGCBackground(gc, WHITE);

	while (1) {
		GrGetNextEvent(&event);

		switch (event.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				do_exposure(&event.exposure);
				break;
			case GR_EVENT_TYPE_BUTTON_DOWN:
				do_buttondown(&event.button);
				break;
			case GR_EVENT_TYPE_BUTTON_UP:
				do_buttonup(&event.button);
				break;
			case GR_EVENT_TYPE_UPDATE:
				do_update(&event.update);
				break;
			case GR_EVENT_TYPE_MOUSE_POSITION:
				do_mouse(&event.mouse);
				break;
			case GR_EVENT_TYPE_CLOSE_REQ:
				GrClose();
				return 0;
		}
	}
}

static mwin * IsDecoration(GR_WINDOW_ID wid)
{
	mwin * mwp;
	for(mwp = mwins; mwp; mwp = mwp->next) {
		if (mwp->fid == wid) {
			return mwp;
		}
	}
	return NULL;
}

#if WINDOW_MANAGER
static mwin * FindWindow(GR_WINDOW_ID wid)
{
	mwin * mwp;
	for(mwp = mwins; mwp; mwp = mwp->next) {
		if (mwp->wid == wid) {
			return mwp;
		}
	}
	return NULL;
}

static mwin * NewWindow(GR_WINDOW_ID wid)
{
	mwin * mwp = malloc(sizeof(mwin));

	if (mwp) {
		mwp->wid = wid;
		mwp->next = mwins;
		mwins = mwp;
	}
	return mwp;
}
#endif

static void
do_update(GR_EVENT_UPDATE *ep)
{
#if WINDOW_MANAGER
	mwin *	mwp;
	mwin *	tmwp;
	GR_WINDOW_INFO winfo;

	if (IsDecoration(ep->wid)) return;

	if ((mwp = FindWindow(ep->wid)) == NULL) {
		/* We have a new window */
		if (ep->utype != GR_UPDATE_MAP) return;
		if ((mwp = NewWindow(ep->wid)) == NULL) {
			GrError("malloc failed\n");
			return;
		}
		GrGetWindowInfo(ep->wid, &winfo);
		mwp->x = ep->x - winfo.bordersize;
		mwp->y = ep->y - winfo.bordersize;
		mwp->width = ep->width + 2 * winfo.bordersize;
		GrMoveWindow(mwp->wid, mwp->x + winfo.bordersize,
			mwp->y + DEC_HEIGHT + 2 * winfo.bordersize);
		mwp->fid = GrNewWindow(GR_ROOT_WINDOW_ID, mwp->x + 1,
			mwp->y + 1, mwp->width - 2, DEC_HEIGHT - 2, 1, BLUE, BLACK);
		GrSelectEvents(mwp->fid, GR_EVENT_MASK_BUTTON_DOWN |
			GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_MOUSE_POSITION);
		GrMapWindow(mwp->fid);
	} else {
		switch (ep->utype) {
			case GR_UPDATE_UNMAP:
				GrUnmapWindow(mwp->fid);
				GrDestroyWindow(mwp->fid);
				if (mwins == mwp) {
					mwins = mwp->next;
				} else for(tmwp = mwins; tmwp; tmwp = tmwp->next) {
					if (tmwp->next == mwp) {
						tmwp->next = mwp->next;
					}
				}
				free(mwp);
				break;
			case GR_UPDATE_MOVE:
				GrGetWindowInfo(ep->wid, &winfo);
				if ((ep->x == (mwp->x + winfo.bordersize)) &&
					(ep->y == (mwp->y + winfo.bordersize + DEC_HEIGHT))) {
					return;
				}
				mwp->x = ep->x - winfo.bordersize;
				mwp->y = ep->y - winfo.bordersize - DEC_HEIGHT;
				GrMoveWindow(mwp->fid, mwp->x + 1, mwp->y + 1);
			default:
				break;
		}
	}
#endif
}

/*
 * Handle mouse position events
 */
static void
do_mouse(GR_EVENT_MOUSE *ep)
{
#ifdef SHOW_WINDOW_MOTION
	GR_WINDOW_INFO winfo;

	if(!in_motion) return;

	in_motion->x = ep->rootx - move_xoff - 1;
	in_motion->y = ep->rooty - move_yoff - 1;
	GrMoveWindow(in_motion->fid, in_motion->x + 1, in_motion->y + 1);
	GrGetWindowInfo(in_motion->wid, &winfo);
	GrMoveWindow(in_motion->wid, in_motion->x + winfo.bordersize - 1,
			in_motion->y + 2 * winfo.bordersize + DEC_HEIGHT - 1);
#endif
}

/*
 * Here when an exposure event occurs.
 */
static void
do_exposure(GR_EVENT_EXPOSURE *ep)
{
	struct app_info	* act;
	int app_no;

	if (ep->wid == w1) {
		for(act=Apps,app_no=0;act->app_id[0]!='\0';act++,app_no++) {
			GrText(w1, gc, 2, 2 + fheight * (app_no + 1),
				act->app_id, -1, GR_TFBOTTOM);
		}
	}
#if ROOT_WIN_RECOLOR
	else if (ep->wid == GR_ROOT_WINDOW_ID) {
		GrFillRect(GR_ROOT_WINDOW_ID, bgc, ep->x, ep->y, ep->width, ep->height);
	}
#endif
}

extern char ** environ;

static void
do_buttondown(GR_EVENT_BUTTON *ep)
{
	mwin *	mwp;
	char * nargv[2];
	static int app_no;

	if (ep->wid == w1) {
		int y = MWMAX(ep->y - 2, 0);        /* FIXME fudge */
		app_no = y / fheight;
		if (app_no >= num_apps) {
			app_no = num_apps - 1;
		}
		nargv[0] = Apps[app_no].app_path;
		nargv[1] = 0;
#if ELKS
		waitpid(-1, NULL, WNOHANG);
#endif
		if (!vfork()) {
			execve(nargv[0], nargv, environ);
			/* write(1, "\7", 1); */
			exit(1);
		}
	} else if ((mwp = IsDecoration(ep->wid)) != NULL) {
		GrRaiseWindow(mwp->wid);
		GrRaiseWindow(mwp->fid);
		in_motion = mwp;
		move_xoff = ep->x;
		move_yoff = ep->y;
 	}
}
 
static void
do_buttonup(GR_EVENT_BUTTON *ep)
{
#ifdef SHOW_WINDOW_MOTION
	in_motion = NULL;
#else
	mwin *	mwp;
	GR_WINDOW_INFO winfo;

	if ((mwp = IsDecoration(ep->wid)) != NULL) {
		if (mwp == in_motion) {
			mwp->x = ep->rootx - 1 - move_xoff;
			mwp->y = ep->rooty - 1 - move_yoff;
			GrMoveWindow(mwp->fid, mwp->x + 1, mwp->y + 1);
			GrGetWindowInfo(mwp->wid, &winfo);
			GrMoveWindow(mwp->wid, mwp->x + winfo.bordersize,
				mwp->y + 2 * winfo.bordersize + DEC_HEIGHT);
			in_motion = NULL;
		}
	}
#endif
}
