/* 
 * A quick program demonstrating creating and drawing a bitmap.
 *
 * Copyright (c) 2002 Alex Holden.
 */

#include <stdio.h>
#include <nano-X.h>
#include <nxcolors.h>

#define TUX

#ifdef TUX

#define BMAP(a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, \
		a1, a0) (GR_BITMAP)(a0 | (a1 << 1) | (a2 << 2) | (a3 << 3) \
		| (a4 << 4) | (a5 << 5) | (a6 << 6) | (a7 << 7) | (a8 << 8) \
		| (a9 << 9) | (a10 << 10) | (a11 << 11) | (a12 << 12) \
		| (a13 << 13) | (a14 << 14) | (a15 << 15))

#define TUX_WIDTH 32
#define TUX_HEIGHT 32
	
void redraw(GR_WINDOW_ID wid, GR_GC_ID gc);

static GR_BITMAP tux_bits[] = {
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1),BMAP(1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1),BMAP(1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1),BMAP(1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1),BMAP(1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1),BMAP(0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1),BMAP(0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1),BMAP(1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0),BMAP(0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0),BMAP(0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1),BMAP(0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1),BMAP(0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0),BMAP(0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0),BMAP(0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0),BMAP(0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0),BMAP(0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0),BMAP(0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0),BMAP(0,0,0,0,1,0,1,1,1,0,0,0,0,0,0,0),
BMAP(0,0,0,0,1,0,0,0,1,0,1,1,0,0,0,0),BMAP(0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,1,0,0,0,0,0,1,1,0,0,0),BMAP(0,0,0,0,1,0,0,0,0,1,0,1,0,0,0,0),
BMAP(0,0,0,1,0,0,0,1,0,1,0,0,0,0,0,0),BMAP(0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,1,0,0,1,0,0,1,0,1,0,0,0),BMAP(0,0,1,1,1,0,0,0,1,0,1,0,1,0,0,0),
BMAP(0,0,0,1,0,0,1,0,0,0,1,0,1,1,1,1),BMAP(1,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,1,0,1,0,1,0,0,0,1,1,1,1),BMAP(1,1,1,1,1,1,0,1,1,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,1,1,1,0,1,0,0,0),BMAP(0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0),
BMAP(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),BMAP(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
};

#define BITMAPWIDTH TUX_WIDTH
#define BITMAPHEIGHT TUX_HEIGHT
#define BITMAP tux_bits

#else
#include "tuxmask.h"
#define BITMAPWIDTH TUXMASK_WIDTH
#define BITMAPHEIGHT TUXMASK_HEIGHT
#define BITMAP tuxmask_bits
#endif

void redraw(GR_WINDOW_ID wid, GR_GC_ID gc)
{
	GrBitmap(wid, gc, 0, 0, BITMAPWIDTH, BITMAPHEIGHT, BITMAP);
}

int main(void)
{
	GR_WINDOW_ID wid;
	GR_GC_ID gc;
	GR_EVENT event;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to the Nano-X server\n");
		return 1;
	}

	wid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, BITMAPWIDTH, BITMAPHEIGHT,
			0, GR_COLOR_WHITE, 0);

	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_EXPOSURE);

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_BLACK);
	GrSetGCBackground(gc, GR_COLOR_WHITE);

	GrMapWindow(wid);

	while(1) {
		GrGetNextEvent(&event);
		switch(event.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				redraw(wid, gc);
				break;
			case GR_EVENT_TYPE_CLOSE_REQ:
				GrClose();
				return 0;
			default:
				break;
		}
	}
}
