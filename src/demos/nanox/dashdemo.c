#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

int COLS, ROWS;
GR_WINDOW_ID g_main;

static void
draw_screen(void)
{
	GR_POINT tri[4] = { {5, 115}, {105, 115}, {55, 200}, {5, 115} };
	GR_WINDOW_INFO winfo;
	GR_GC_ID gc;
	char dash1[2] = { 10, 5 };
	char dash2[4] = { 5, 2, 1, 2 };
	char dash3[4] = { 5, 2, 5, 5 };
	char dash4[2] = { 2, 2 };

	GrGetWindowInfo(g_main, &winfo);

	/* Draw several lines and a few boxes */
	gc = GrNewGC();
	GrSetGCLineAttributes(gc, GR_LINE_ONOFF_DASH);

	/* Draw a dashed box */

	GrSetGCDash(gc, dash1, 2);
	GrRect(g_main, gc, 5, 5, 100, 100);

	GrSetGCDash(gc, dash2, 4);
	GrLine(g_main, gc, 10, 10, 95, 95);

	GrSetGCDash(gc, dash3, 4);
	GrEllipse(g_main, gc, 160, 55, 50, 50);

	GrSetGCDash(gc, dash4, 2);
	GrPoly(g_main, gc, 4, tri);

	GrDestroyGC(gc);
}

int
main(int argc, char **argv)
{
	int COLS, ROWS;

	if (GrOpen() < 0) {
		fprintf(stderr, "Cannot open graphics\n");
		exit(1);
	}

	COLS = 350;
	ROWS = 300;

	g_main = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "dashdemo",
		GR_ROOT_WINDOW_ID, 100, 50, COLS - 120, ROWS - 60, BLUE);

	GrSelectEvents(g_main, GR_EVENT_MASK_EXPOSURE|GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(g_main);

	while (1) {
		GR_EVENT event;
		GrGetNextEvent(&event);

		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			draw_screen();
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
		}
	}
}
