/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2000 Century Software <embedded.centurysoftware.com>
 *
 * Scribble Handwriting Recognition for Nano-X!
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
#include "scrib.h"

#define TEXTWIN_WIDTH	200		/* text window width/height*/
#define TEXTWIN_HEIGHT	150

static ScribbleWidget	w;

static GR_BOOL		bTextwin = GR_FALSE;
static GR_WINDOW_ID	wt = 0;
static GR_GC_ID		gct = 0;
static GR_GC_ID		gctb = 0;
static GR_COORD		xpos = 0;
static GR_COORD		ypos = 0;
static GR_SIZE		width;		/* width of character */
static GR_SIZE		height;		/* height of character */
static GR_SIZE		base;		/* height of baseline */
static void char_out(GR_CHAR ch);
static void char_del(GR_COORD x, GR_COORD y);

void do_buttondown(GR_EVENT_BUTTON	*bp);
void do_buttonup(GR_EVENT_BUTTON	*bp);
void do_motion(GR_EVENT_MOUSE		*mp);
void do_focusin(GR_EVENT_GENERAL	*gp);
void do_keystroke(GR_EVENT_KEYSTROKE	*kp);
void do_exposure(GR_EVENT_EXPOSURE	*ep);

int
main(int argc, char **argv)
{
	int		t = 1;
	GR_EVENT	event;		/* current event */

	while (t < argc) {
		if (!strcmp("-t", argv[t])) {
			bTextwin = GR_TRUE;
			++t;
			continue;
		}
	}

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}

	if (bTextwin) {
		/* create text output window for debugging*/
		wt = GrNewWindow(GR_ROOT_WINDOW_ID, 50, 20,
				TEXTWIN_WIDTH, TEXTWIN_HEIGHT, 5, BLACK, GREEN);
		GrSelectEvents(wt, 
			GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_KEY_DOWN
			| GR_EVENT_MASK_EXPOSURE);
		GrMapWindow(wt);
		gct = GrNewGC();
		GrSetGCForeground(gct, GREEN);
		GrGetGCTextSize(gct, "A",1, GR_TFASCII, &width, &height, &base);
		GrSetGCFont(gct, GrCreateFont(GR_FONT_OEM_FIXED, 0, NULL));
		gctb = GrNewGC();
		GrSetGCForeground(gctb, BLACK);
	}

	/* create scribble input window*/
	w = create_scribble();

	while (1) {
		GrGetNextEvent(&event);

		switch (event.type) {
			case GR_EVENT_TYPE_BUTTON_DOWN:
				do_buttondown(&event.button);
				break;

			case GR_EVENT_TYPE_BUTTON_UP:
				do_buttonup(&event.button);
				break;

			case GR_EVENT_TYPE_MOUSE_POSITION:
			case GR_EVENT_TYPE_MOUSE_MOTION:
				do_motion(&event.mouse);
				break;

			case GR_EVENT_TYPE_FOCUS_IN:
				do_focusin(&event.general);
				break;

			case GR_EVENT_TYPE_KEY_DOWN:
				do_keystroke(&event.keystroke);
				break;

			case GR_EVENT_TYPE_EXPOSURE:
				do_exposure(&event.exposure);
				break;

			case GR_EVENT_TYPE_CLOSE_REQ:
				GrClose();
				exit(0);
		}
	}
}


/*
 * Here when a button is pressed.
 */
void
do_buttondown(GR_EVENT_BUTTON	*bp)
{
	ActionStart(w, bp->x, bp->y);
}


/*
 * Here when a button is released.
 */
void
do_buttonup(GR_EVENT_BUTTON	*bp)
{
	ActionEnd(w, bp->x, bp->y);
}


/*
 * Here when the mouse has a motion event.
 */
void
do_motion(GR_EVENT_MOUSE	*mp)
{
	ActionMove(w, mp->x, mp->y);
}


/*
 * Here when our window gets focus
 */
void
do_focusin(GR_EVENT_GENERAL	*gp)
{
#if 0
	/* if the window receiving focus is scribble, remember last window*/
	if (gp->wid == w->win && gp->wid != 1)
		w->lastfocusid = gp->otherid;
#endif
}


/*
 * Here when an exposure event occurs.
 */
void
do_exposure(GR_EVENT_EXPOSURE	*ep)
{
	if (ep->wid == w->win)
		Redisplay(w);
}


/*
 * Here when a keyboard press or injection occurs.
 */
void
do_keystroke(GR_EVENT_KEYSTROKE	*kp)
{
	if (bTextwin)
		char_out(kp->ch);
}

static void
char_del(GR_COORD x, GR_COORD y)
{
	xpos -= width;
	GrFillRect(wt, gctb, x+1, y /*- height*/ /*+ base*/, width, height);
}

static void
char_out(GR_CHAR ch)
{
	switch(ch) {
	case '\r':
	case '\n':
		xpos = 0;
		ypos += height;
		if(ypos >= TEXTWIN_HEIGHT - height) {
			ypos -= height;

			/* FIXME: changing FALSE to TRUE crashes nano-X*/
			/* clear screen, no scroll*/
			ypos = 0;
			GrClearWindow(wt, GR_FALSE);
		}
		return;
	case '\007':			/* bel*/
		return;
	case '\t':
		xpos += width;
		while((xpos/width) & 7)
			char_out(' ');
		return;
	case '\b':			/* assumes fixed width font!!*/
		if (xpos <= 0)
			return;
		char_del(xpos, ypos);
		char_out(' ');
		char_del(xpos, ypos);
		return;
	}
	GrText(wt, gct, xpos+1, ypos, &ch, 1, GR_TFTOP);
	xpos += width;

	if (xpos >= TEXTWIN_WIDTH-width)
		char_out('\n');
}
