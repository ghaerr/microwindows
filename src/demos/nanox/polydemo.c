/*
 * polytest - polygon fill test program for Nano-X
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

static void
draw(GR_EVENT * e)
{
	GR_WINDOW_ID wid = ((GR_EVENT_EXPOSURE *)e)->wid;
	GR_GC_ID gc = GrNewGC();
	GR_POINT points[4];
	int x = 10;
	int y = 10;
	int sz = 20;
	int sz2 = 5;

	GrSetGCBackground(gc, BLACK);

	/* fill poly #1*/
	points[0].x = x;
	points[0].y = y;
	points[1].x = x + sz;
	points[1].y = y;
	points[2].x = x + (sz / 2);
	points[2].y = y + sz;
	points[3].x = x;
	points[3].y = y;

	GrSetGCForeground(gc, WHITE);
	GrFillPoly(wid, gc, 3, points);

	/* outline poly #1*/
	GrSetGCForeground(gc, GREEN);
	GrPoly(wid, gc, 4, points);

	/* fill poly #2*/
	y += sz + 10;
	points[0].x = x;
	points[0].y = y;
	points[1].x = x + sz + 1;
	points[1].y = y;
	points[2].x = x + (sz / 2);
	points[2].y = y + sz;
	points[3].x = x;
	points[3].y = y;

	GrSetGCForeground(gc, WHITE);
	GrFillPoly(wid, gc, 3, points);

	/* outline poly #2*/
	GrSetGCForeground(gc, GREEN);
	GrPoly(wid, gc, 4, points);

	/* fill poly #3*/
	y += sz + 10;
	points[0].x = x;
	points[0].y = y;
	points[1].x = x + sz - 1;
	points[1].y = y;
	points[2].x = x + (sz / 2);
	points[2].y = y + sz;
	points[3].x = x;
	points[3].y = y;

	GrSetGCForeground(gc, WHITE);
	GrFillPoly(wid, gc, 3, points);

	/* outline poly #3*/
	GrSetGCForeground(gc, GREEN);
	GrPoly(wid, gc, 4, points);

	/* fill right arrow #1*/
	x = 60;
	y = 60;
	sz = 10;
	sz2 = 8;

	points[0].x = x;
	points[0].y = y;
	y -= sz;
	points[1].x = x + sz2;
	points[1].y = y;
	y -= sz;
	points[2].x = x;
	points[2].y = y;
	points[3].x = x;
	points[3].y = 60;

	GrSetGCForeground(gc, WHITE);
	GrFillPoly(wid, gc, 3, points);

	/* outline right arrow #1*/
	GrSetGCForeground(gc, GREEN);
	GrPoly(wid, gc, 4, points);

	/* fill right arrow #2*/
	x = 60;
	y = 90;
	points[0].x = x;
	points[0].y = y;
	y -= sz;
	points[1].x = x + sz2;
	points[1].y = y;
	y -= sz;
	points[2].x = x;
	points[2].y = y;
	points[3].x = x;
	points[3].y = 90;

	GrSetGCForeground(gc, WHITE);
	GrFillPoly(wid, gc, 3, points);

	/* concave polygon filling*/
	{
	static GR_POINT pt1[5] = {{10,120},{150,130},{120,220},{60,160},{15,200}};
	static GR_POINT pt2[5] = {{10,220},{150,230},{120,320},{60,360},{15,300}};
 
	/* concave poly fill #4 - fails with some algorithms*/
	GrSetGCForeground(gc, WHITE);
	GrFillPoly(wid,gc,5,pt1);
	 
	/* concave poly outline #4 - ok*/
	GrSetGCForeground(gc, GREEN);
	GrPoly(wid,gc,5,pt1);

	/* convex poly fill #5 - ok*/
	GrSetGCForeground(gc, WHITE);
	GrFillPoly(wid,gc,5,pt2);

	/* convex poly outline #5 - ok*/
	GrSetGCForeground(gc, GREEN);
	GrPoly(wid,gc,5,pt2);
	}

	GrDestroyGC(gc);
}

int
main(int ac, char **av)
{
	GR_EVENT event;
	GR_WINDOW_ID w;

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}

	/* create window */
	w = GrNewWindowEx(GR_WM_PROPS_NOAUTOMOVE | GR_WM_PROPS_BORDER |
			      GR_WM_PROPS_CAPTION | GR_WM_PROPS_CLOSEBOX,
			      "polydemo", GR_ROOT_WINDOW_ID, 
			      10, 10, 220, 362, GR_RGB(0, 0, 0));
	GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(w);

	while (1) {
		GrGetNextEvent(&event);

		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			draw(&event);
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
		}
	}

}
