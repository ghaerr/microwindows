/*
 * nano-X launcher/window manager program
 * (C) 1999 Alistair Riddoch <ajr@ecs.soton.ac.uk>
 * (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

/*
   Undefine this if solid window moves are incredibly slow on your hardware.
   Unfortunately since outline moves are not supported yet, the only
   alternative is "invisible" moving.
*/ 
#define SHOW_WINDOW_MOTION

/*
   Define this if you want the mouse pointer to become bell shaped when over
   the launcher window.
*/
#undef USE_WEIRD_POINTER

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
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
static	GR_GC_ID	bgc;		/* graphics context for rectangle */
static	GR_SCREEN_INFO	si;		/* information about screen */
static	int		fwidth, fheight;
static	int		fbase;
static	int		num_apps = 0;

void do_exposure();
void do_buttondown();
void do_buttonup();
void do_update();
void do_mouse();

struct app_info {
	char		app_id[10];
	char		app_path[64];
} Apps[] = {
#if ELKS
	{"clock", "/root/nclock"},
	{"term", "/root/nterm"},
	{"demo", "/root/demo"},
	{"demo2", "/root/demo2"},
#else
	{"clock", "bin/nclock"},
	{"term", "bin/nterm"},
	{"demo", "bin/demo"},
	{"demo2", "bin/demo2"},
	{"ntest", "bin/ntest"},
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

/*
 * Reap the dead children whenever we get a SIGCHLD.
 */
void reaper(int signum) { while(waitpid(WAIT_ANY, NULL, WNOHANG) > 0); }

int
main(int argc,char **argv)
{
	GR_EVENT	event;		/* current event */
	struct app_info	* act;
	int		width, height;

#ifdef USE_WEIRD_POINTER
	GR_BITMAP	bitmap1fg[7];	/* bitmaps for first cursor */
	GR_BITMAP	bitmap1bg[7];
#endif

	for(act = Apps; act->app_id[0] != '\0'; act++, num_apps++);

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
	
	GrGetScreenInfo(&si);

	signal(SIGCHLD, &reaper);

	gc = GrNewGC();
	bgc = GrNewGC();

	GrSetGCForeground(bgc, GRAY);
	GrSetGCFont(gc, GrCreateFont(GR_FONT_OEM_FIXED, 0, NULL));

	GrGetGCTextSize(gc, "A", 1, GR_TFASCII, &fwidth, &fheight, &fbase);
	width = fwidth * 8 + 4;
	height = (fheight) * num_apps + 4;

	w1 = GrNewWindow(GR_ROOT_WINDOW_ID, 5, 5, width,
		height, 1, WHITE, BLACK);

	GrSelectEvents(w1, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_DOWN
			| GR_EVENT_MASK_CLOSE_REQ);
	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE |
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

	GrFillRect(GR_ROOT_WINDOW_ID, bgc, 0, 0, si.cols, si.rows);

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
				exit(0);
		}
	}
}

mwin * IsDecoration(GR_WINDOW_ID wid)
{
	mwin * mwp;
	for(mwp = mwins; mwp; mwp = mwp->next) {
		if (mwp->fid == wid) {
			return mwp;
		}
	}
	return NULL;
}
	
mwin * FindWindow(GR_WINDOW_ID wid)
{
	mwin * mwp;
	for(mwp = mwins; mwp; mwp = mwp->next) {
		if (mwp->wid == wid) {
			return mwp;
		}
	}
	return NULL;
}

mwin * NewWindow(GR_WINDOW_ID wid)
{
	mwin * mwp = malloc(sizeof(mwin));

	if (mwp) {
		mwp->wid = wid;
		mwp->next = mwins;
		mwins = mwp;
	}
	return mwp;
}

void
do_update(ep)
	GR_EVENT_UPDATE	*ep;
{
	mwin *	mwp;
	mwin *	tmwp;
	GR_WINDOW_INFO winfo;

	if (IsDecoration(ep->wid)) return;

	if ((mwp = FindWindow(ep->wid)) == NULL) {
		/* We have a new window */
		if (ep->utype != GR_UPDATE_MAP) return;
		if ((mwp = NewWindow(ep->wid)) == NULL) {
			printf("malloc failed\n");
			return;
		}
		GrGetWindowInfo(ep->wid, &winfo);
		mwp->x = ep->x - winfo.bordersize;
		mwp->y = ep->y - winfo.bordersize;
		mwp->width = ep->width + 2 * winfo.bordersize;
		GrMoveWindow(mwp->wid, mwp->x + winfo.bordersize,
					mwp->y + DEC_HEIGHT +
						2 * winfo.bordersize);
		mwp->fid = GrNewWindow(GR_ROOT_WINDOW_ID, mwp->x + 1,
				mwp->y + 1, mwp->width - 2,
				DEC_HEIGHT - 2, 1, BLUE, BLACK);
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
					(ep->y == (mwp->y + winfo.bordersize
							+ DEC_HEIGHT))) {
					return;
				}
				mwp->x = ep->x - winfo.bordersize;
				mwp->y = ep->y - winfo.bordersize - DEC_HEIGHT;
				GrMoveWindow(mwp->fid, mwp->x + 1, mwp->y + 1);
			default:
				break;
		}
	}
}

/*
 * Handle mouse position events
 */
void do_mouse(ep)
	GR_EVENT_MOUSE *ep;
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
void
do_exposure(ep)
	GR_EVENT_EXPOSURE	*ep;
{
	struct app_info	* act;
	int app_no;

	if (ep->wid == w1) {
		for(act=Apps,app_no=0;act->app_id[0]!='\0';act++,app_no++) {
			GrText(w1, gc, 2, 2 + fheight * (app_no + 1),
				act->app_id, -1, GR_TFBOTTOM);
		}
	} else if (ep->wid == GR_ROOT_WINDOW_ID) {
		GrFillRect(GR_ROOT_WINDOW_ID, bgc, ep->x, ep->y,
				ep->width, ep->height);
	}
}

extern char ** environ;

void
do_buttondown(ep)
	GR_EVENT_BUTTON	*ep;
{
	mwin *	mwp;
	static int app_no;

	if (ep->wid == w1) {
		app_no = ep->y / fheight;
		if (app_no >= num_apps) {
			app_no = num_apps - 1;
		}
		
		if (!vfork()) {
			char * nargv[2];

			nargv[0] = Apps[app_no].app_path;
			nargv[1] = 0;
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
 
void
do_buttonup(ep)
GR_EVENT_BUTTON	*ep;
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
