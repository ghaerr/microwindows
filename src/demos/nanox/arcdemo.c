/*
 * A quick program to test GrArc() and GrArcAngle().
 * Copryight (C) 2002 Alex Holden <alex@alexholden.net>
 *
 * Should draw two quarter-pies next to each other, with the upper left
 * quadrant filled. The left hand one is drawn with GrArc() and the right hand
 * one is drawn with GrArcAngle().
 */
#include <stdlib.h>
#include <nano-X.h>
#include <nxcolors.h>

int main()
{
	GR_EVENT ev;
	GR_WINDOW_ID wid;
	GR_GC_ID gc;

	if(GrOpen() == -1) exit(-1);

	wid = GrNewWindowEx(GR_WM_PROPS_BORDER|GR_WM_PROPS_CAPTION|
		GR_WM_PROPS_CLOSEBOX, "arcdemo",
		GR_ROOT_WINDOW_ID, 0, 0, 200, 100, GR_COLOR_WHITE);

	GrSelectEvents(wid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(wid);

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_GREEN);

	while(1) {
		GrGetNextEvent(&ev);
		if(ev.type == GR_EVENT_TYPE_CLOSE_REQ)
			break;
		else if(ev.type == GR_EVENT_TYPE_EXPOSURE) {
			GrArc(wid, gc, 75, 60, 30, 30, 0, -30, -30, 0, GR_PIE);
			GrArcAngle(wid, gc, 150, 60, 30, 30, 5760, 11520, GR_PIE);
		}

	}

	GrClose();

	return 0;
}
