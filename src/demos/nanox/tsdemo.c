/*
 * tsdemo - tile/stipple demo program for Nano-X
 */
#define MWINCLUDECOLORS
#include <stdlib.h>
#include "nano-X.h"

#define	_	((unsigned) 0)	/* off bits */
#define	X	((unsigned) 1)	/* on bits */


static GR_BITMAP g_stipple1[] = {
	0x1000,			/* 00010000 */
	0x3800,			/* 00111000 */
	0x7C00,			/* 01111100 */
	0xFE00,			/* 11111110 */
	0x7C00,			/* 01111100 */
	0x3800,			/* 00111000 */
	0x1000			/* 00010000 */
};

static GR_BITMAP g_stipple2[] = {
	0x4000,
	0x8000
};

static GR_BITMAP g_stipple3[] = {
	0x4000,
	0x4000,
};

#ifdef NOTUSED
static GR_BITMAP g_stipple4[] = {
	0x8800,			/* 1000100000000000 */
	0x7000,			/* 0111000000000000 */
	0x2000,			/* 00100000000000000 */
	0x7000,			/* 1110000000000000 */
	0x8800,			/* 0001000100000000 */
};

static GR_BITMAP g_stipple1[] = {
	0x0008,			/* 0001000 */
	0x001C,			/* 0011100 */
	0x003E,			/* 0111110 */
	0x00FF,			/* 1111111 */
	0x003E,			/* 0111110 */
	0x001C,			/* 0011100 */
	0x0008			/* 0001000 */
};

static GR_BITMAP g_stipple2[] = {
	0x0002,
	0x0001
};

static GR_BITMAP g_stipple3[] = {
	0x0004,
	0x0004,
};

static GR_BITMAP g_stipple4[] = {
	0x0011,
	0x000E,
	0x0004,
	0x000E,
	0x0011
};
#endif

int g_x;
GR_WINDOW_ID g_main, g_pixmap;

void
draw_set(char *name, int mode)
{
	int x;
	int tw, th, tb;
	GR_POINT points[4];
	GR_GC_ID gc = GrNewGC();

	GrSetGCForeground(gc, WHITE);
	GrSetGCBackground(gc, GRAY);

	GrGetGCTextSize(gc, name, -1, GR_TFTOP, &tw, &th, &tb);

	x = g_x + (tw - 50) / 2;

	GrText(g_main, gc, g_x, 5, name, -1, GR_TFTOP);

	g_x += (tw + 10);

	GrSetGCFillMode(gc, mode);

	if (mode == GR_FILL_STIPPLE)
		GrSetGCForeground(gc, YELLOW);
	else {
		GrSetGCForeground(gc, WHITE);
		GrSetGCBackground(gc, BLUE);
	}

	if (mode == GR_FILL_TILE) {
		GrSetGCTile(gc, g_pixmap, 16, 16);
	}

	if (mode == GR_FILL_STIPPLE || mode == GR_FILL_OPAQUE_STIPPLE)
		GrSetGCStipple(gc, g_stipple2, 2, 2);

	GrFillRect(g_main, gc, x, 25, 50, 50);

	if (mode == GR_FILL_STIPPLE || mode == GR_FILL_OPAQUE_STIPPLE)
		GrSetGCStipple(gc, g_stipple1, 7, 7);

	GrFillEllipse(g_main, gc, x + 25, 105, 25, 25);

	if (mode == GR_FILL_STIPPLE || mode == GR_FILL_OPAQUE_STIPPLE)
		GrSetGCStipple(gc, g_stipple3, 3, 2);

	points[0].x = points[3].x = x;
	points[0].y = points[3].y = 165;

	points[1].x = x + 50;
	points[1].y = 165;

	points[2].x = x + 25;
	points[2].y = 215;

	GrFillPoly(g_main, gc, 4, points);

	GrDestroyGC(gc);
}

void
load_pixmap(void)
{
	GR_GC_ID gc = GrNewGC();

	g_pixmap = GrNewPixmap(16, 16, 0);
	GrDrawImageFromFile(g_pixmap, gc, 0, 0, -1, -1, "bin/nxroach.pgm", 0);

	GrDestroyGC(gc);
}

int
main(int argc, char **argv)
{
	int COLS, ROWS;

	if (GrOpen() == -1)
		exit(1);

	COLS = 480;
	ROWS = 300;

	load_pixmap();

	g_main = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "tsdemo",
		    GR_ROOT_WINDOW_ID, 100, 50, COLS - 120, ROWS - 60, GRAY);

	GrSelectEvents(g_main,
		       GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(g_main);

	while (1) {
		GR_EVENT event;

		GrGetNextEvent(&event);

		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			g_x = 5;
			draw_set("GR_FILL_STIPPLE", GR_FILL_STIPPLE);
			draw_set("GR_FILL_OPAQUE_STIPPLE", GR_FILL_OPAQUE_STIPPLE);
			draw_set("GR_FILL_TILE", GR_FILL_TILE);
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
		}
	}
}
