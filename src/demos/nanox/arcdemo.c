/*
 * Arc drawing demo for Nano-X
 *
 * Copyright (C) 2002 Alex Holden <alex@alexholden.net>
 * Modified by G Haerr
 */
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

static void
draw(GR_EVENT *ep)
{
	GR_WINDOW_ID wid = ((GR_EVENT_EXPOSURE *)ep)->wid;
	GR_GC_ID gc = GrNewGC();
	int x = 40;
	int y = 40;
	int rx = 30;
	int ry = 30;
	int xoff = (rx + 10) * 2;

	GrSetGCForeground(gc, GREEN);

	/* filled arc*/
	GrArc(wid, gc, x, y, rx, ry, 0, -30, -30, 0, GR_PIE);
	GrArc(wid, gc, x+5, y, rx, ry, 30, 0, 0, -30, GR_PIE);
	GrArc(wid, gc, x, y+5, rx, ry, -30, 0, 0, 30, GR_PIE);
	GrArc(wid, gc, x+5, y+5, rx, ry, 0, 30, 30, 0, GR_PIE);

	/* outlined arc*/
	x += xoff;
	GrArc(wid, gc, x, y, rx, ry, 0, -30, -30, 0, GR_ARCOUTLINE);
	GrArc(wid, gc, x+5, y, rx, ry, 30, 0, 0, -30, GR_ARCOUTLINE);
	GrArc(wid, gc, x, y+5, rx, ry, -30, 0, 0, 30, GR_ARCOUTLINE);
	GrArc(wid, gc, x+5, y+5, rx, ry, 0, 30, 30, 0, GR_ARCOUTLINE);

	/* arc only*/
	x += xoff;
	GrArc(wid, gc, x, y, rx, ry, 0, -30, -30, 0, GR_ARC);
	GrArc(wid, gc, x+5, y, rx, ry, 30, 0, 0, -30, GR_ARC);
	GrArc(wid, gc, x, y+5, rx, ry, -30, 0, 0, 30, GR_ARC);
	GrArc(wid, gc, x+5, y+5, rx, ry, 0, 30, 30, 0, GR_ARC);

	GrDestroyGC(gc);
}

int
main(int ac, char **av)
{
	GR_EVENT ev;
	GR_WINDOW_ID wid;

	if (GrOpen() < 0)
		exit(-1);

	wid = GrNewWindowEx(GR_WM_PROPS_BORDER|GR_WM_PROPS_CAPTION|
		GR_WM_PROPS_CLOSEBOX, "arcdemo",
		GR_ROOT_WINDOW_ID, 0, 0, 250, 90, WHITE);

	GrSelectEvents(wid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(wid);

	while (1) {
		GrGetNextEvent(&ev);

		if (ev.type == GR_EVENT_TYPE_CLOSE_REQ)
			break;
		if (ev.type == GR_EVENT_TYPE_EXPOSURE)
			draw(&ev);
	}

	GrClose();

	return 0;
}
