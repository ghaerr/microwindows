/* 
 * nxclock - Nano-X clock program
 *
 * Copyright (C) 2000 by Greg Haerr <greg@censoft.com>
 * Copyright (C) 1999 Alistair Riddoch <ajr@ecs.soton.ac.uk>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

/* If you need a clock bigger than 200x200 you will need to re-write the trig *
 * to use longs. (Only applies under ELKS I think. */
#define CWIDTH		100		/* Max 200 */
#define CHEIGHT		100		/* Max 200 */

/*
 * Definitions to make it easy to define cursors
 */
#define	_	((unsigned) 0)		/* off bits */
#define	X	((unsigned) 1)		/* on bits */
#define	MASK(a,b,c,d,e,f,g) \
	(((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
	+ e) * 2) + f) * 2) + g) << 9)

static	GR_WINDOW_ID	w1;		/* id for window */
static	GR_GC_ID	gc1;		/* graphics context for text */
static	GR_GC_ID	gc2;		/* graphics context for rectangle */
static int lh = -1, lm = -1, ls = -1;
static time_t then;

static unsigned char trig[91] =
	{ 0, 4, 8, 13, 17, 22, 26, 31, 35, 40, 44, 48, 53, 57, 61, 66,
	70, 74, 79, 83, 87, 91, 95, 100, 104, 108, 112, 116, 120, 124, 128,
	131, 135, 139, 143, 146, 150, 154, 157, 161, 164, 167, 171, 174, 177,
	181, 184, 187, 190, 193, 196, 198, 201, 204, 207, 209, 212, 214, 217,
	219, 221, 223, 226, 228, 230, 232, 233, 235, 237, 238, 240, 242, 243,
	244, 246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255, 255,
	255, 255, 255, 255}; 

void do_exposure();
void do_clock();
void do_idle();
void errorcatcher();			/* routine to handle errors */

int
main(int ac, char **av)
{
	GR_EVENT	event;		/* current event */
	GR_BITMAP	bitmap1fg[7];	/* bitmaps for first cursor */
	GR_BITMAP	bitmap1bg[7];

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
	
	/* create window*/
	w1 = GrNewWindowEx(
		GR_WM_PROPS_NOAUTOMOVE|GR_WM_PROPS_BORDER|GR_WM_PROPS_CAPTION|
		GR_WM_PROPS_CLOSEBOX, "nxclock", GR_ROOT_WINDOW_ID, 
		10, 10, CWIDTH, CHEIGHT, GrGetSysColor(GR_COLOR_WINDOW));
		
	GrSelectEvents(w1, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);

	gc1 = GrNewGC();
	gc2 = GrNewGC();

	GrSetGCForeground(gc1, GrGetSysColor(GR_COLOR_WINDOW));
	GrSetGCBackground(gc1, GrGetSysColor(GR_COLOR_WINDOWTEXT));
	GrSetGCForeground(gc2, GrGetSysColor(GR_COLOR_WINDOWTEXT));
	GrSetGCBackground(gc2, GrGetSysColor(GR_COLOR_WINDOW));

	bitmap1bg[0] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[1] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[2] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[3] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[4] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[5] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[6] = MASK(_,_,X,X,X,_,_);

	bitmap1fg[0] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[1] = MASK(_,X,_,X,_,X,_);
	bitmap1fg[2] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[3] = MASK(X,_,_,X,X,_,X);
	bitmap1fg[4] = MASK(_,_,_,_,_,_,_);
	bitmap1fg[5] = MASK(_,X,_,_,_,X,_);
	bitmap1fg[6] = MASK(_,_,_,X,_,_,_);

	GrSetCursor(w1, 7, 7, 3, 3, WHITE, BLACK, bitmap1fg, bitmap1bg);
	GrMapWindow(w1);

	while (1) {
		GrGetNextEventTimeout(&event, 500L);

		switch (event.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				do_exposure(&event.exposure);
				break;

			case GR_EVENT_TYPE_CLOSE_REQ:
				GrClose();
				exit(0);

			case GR_EVENT_TYPE_TIMEOUT:
				do_clock();
				break;
		}
	}
}

int bsin(int angle)
{
	if(angle < 91) {
		return trig[angle];
	} else if (angle < 181) {
		return trig[180 - angle];
	} else if (angle < 271) {
		return -trig[angle - 180];
	} else if (angle < 361) {
		return -trig[360 - angle];
	} else {
		return 0;
	}
}

int bcos(int angle)
{
	if(angle < 91) {
		return trig[90 - angle];
	} else if (angle < 181) {
		return -trig[angle - 90];
	} else if (angle < 271) {
		return -trig[270 - angle];
	} else if (angle < 361) {
		return trig[angle - 270];
	} else {
		return 0;
	}
}

/*
 * Here when an exposure event occurs.
 */
void
do_exposure(ep)
	GR_EVENT_EXPOSURE	*ep;
{
	GR_COORD	midx = CWIDTH / 2;
	GR_COORD	midy = CHEIGHT / 2;
	int i, l;

/*	GrFillRect(w1, gc2, 0, 0, CWIDTH, CHEIGHT); */
/*	GrFillEllipse(w1, gc1, midx, midy, midx, midy); */
	GrEllipse(w1, gc2, midx, midy, midx - 1, midy - 1);
	for(i = 0; i < 60; i++) {
		if (i%5 == 0) {
			l = 5;
		} else {
			l = 0;
		}
		GrLine(w1, gc2,
			midx + (((midx - 3) * bsin(i * 6)) >> 8), 
			midy + (((midy - 3) * bcos(i * 6)) >> 8), 
			midx + (((midx - 3 - l) * bsin(i * 6)) >> 8), 
			midy + (((midy - 3 - l) * bcos(i * 6)) >> 8));
	}

	lh = -1; lm = -1; ls = -1;
	then = 0;
	do_clock();
}

void draw_time(int hour, int minutes, int sec, GR_GC_ID gc )
{
	GR_COORD	midx = CWIDTH / 2;
	GR_COORD	midy = CHEIGHT / 2;

	GrLine(w1, gc1,
		midx + (((midx - 8 - midx / 10) * bsin(ls)) >> 8), 
		midy - (((midy - 8 - midy / 10) * bcos(ls)) >> 8), 
		midx + (((midx - 8 - midx / 4) * bsin(ls)) >> 8), 
		midy - (((midy - 8 - midx / 4) * bcos(ls)) >> 8));
	GrLine(w1, gc2,
		midx + (((midx - 8 - midx / 10) * bsin(sec)) >> 8), 
		midy - (((midy - 8 - midy / 10) * bcos(sec)) >> 8), 
		midx + (((midx - 8 - midx / 4) * bsin(sec)) >> 8), 
		midy - (((midy - 8 - midx / 4) * bcos(sec)) >> 8));
	if ((lm != minutes) || (ls == minutes)) {
		GrLine(w1, gc1,
			midx + (((midx - 8 - midx / 10) * bsin(lm)) >> 8), 
			midy - (((midy - 8 - midy / 10) * bcos(lm)) >> 8), 
			midx + (((midx / 5) * bsin(lm)) >> 8), 
			midy - (((midy / 5) * bcos(lm)) >> 8));
		GrLine(w1, gc2,
			midx + (((midx - 8 - midx / 10) * bsin(minutes)) >> 8), 
			midy - (((midy - 8 - midy / 10) * bcos(minutes)) >> 8), 
			midx + (((midx / 5) * bsin(minutes)) >> 8), 
			midy - (((midy / 5) * bcos(minutes)) >> 8));
		GrLine(w1, gc1,
			midx + (((midx - 8 - midx / 2) * bsin(lh)) >> 8), 
			midy - (((midy - 8 - midy / 2) * bcos(lh)) >> 8), 
			midx + (((midx / 5) * bsin(lh)) >> 8), 
			midy - (((midy / 5) * bcos(lh)) >> 8));
		GrLine(w1, gc2,
			midx + (((midx - 8 - midx / 2) * bsin(hour)) >> 8), 
			midy - (((midy - 8 - midy / 2) * bcos(hour)) >> 8), 
			midx + (((midx / 5) * bsin(hour)) >> 8), 
			midy - (((midy / 5) * bcos(hour)) >> 8));
	}
	lh = hour;
	lm = minutes;
	ls = sec;
}

/*
 * Update the clock if the seconds have changed.
 */
void
do_clock()
{
	struct timeval tv;
	struct timezone tz;
	struct tm * tp;
	time_t now;
	int minutes, hour, sec;

	gettimeofday(&tv, &tz);
	now = tv.tv_sec - (60 * tz.tz_minuteswest);
	if (now == then) {
		return;
	}
	then = now;
	tp = gmtime(&now);
	minutes = tp->tm_min * 6;
	sec = tp->tm_sec * 6;
	hour = tp->tm_hour;
	if (hour > 12) {
		hour -= 12;
	}
	hour = hour*30 + minutes/12;
	draw_time(hour, minutes, sec, gc2);
}

#if 0
/*
 * Sleep a while to avoid using too much CPU time.
 */
void do_idle()
{
	struct timespec idletime;
	idletime.tv_sec = 0;
	idletime.tv_nsec = 100000;
	nanosleep(&idletime, NULL);
}
#endif
