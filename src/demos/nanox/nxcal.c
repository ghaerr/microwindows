/*
 * nxcal: Standard screen calibrator for Nano-X
 * 
 * Copyright (c) 2003 Century Embedded Technologies <http://embedded.censoft.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "nano-X.h"
#include "nxcolors.h"

#define DEFAULT_DATA_FILE "nxcal.dat"

struct point {
	int x;
	int y;
};

#define TARGET_DIST 15
#define CROSS_SIZE 15

static struct point input[4];
static struct point target[4];

static GR_WINDOW_ID g_wid;
static GR_SCREEN_INFO g_si;

static int g_rotate = MWPORTRAIT_NONE;

static int state = 0;
void
usage(void)
{
	printf("Usage: nxcal [-f] [-d <datafile>]\n");
	exit(0);
}

void
swapem(int *a, int *b)
{
	int t = *a;
	*a = *b;
	*b = t;
}

void
calculate_transform(GR_TRANSFORM * trans)
{

	GR_CAL_DATA data;
	double xdiff = 0, ydiff = 0;
	int xyswap = 0;

	/* Check to see if the touchscreen has been rotated or not */
	/* Zaurus:  I'm looking at you */

	switch (g_rotate) {
	case MWPORTRAIT_NONE:
	case MWPORTRAIT_DOWN:
		if (abs(input[0].x - input[1].x) < 50)
			xyswap = 1;
		break;

	case MWPORTRAIT_RIGHT:
	case MWPORTRAIT_LEFT:
		if (abs(input[1].x - input[2].x) < 50)
			xyswap = 1;
		break;
	}

	//printf("DEBUG:  xyswap is %d\n", xyswap);
	//printf("INCOMING: ");

	//for(i = 0; i < 4; i++) 
	//printf("(%d,%d) ", input[i].x, input[i].y);
	//printf("\n");

	switch (g_rotate) {
	case MWPORTRAIT_NONE:
	case MWPORTRAIT_DOWN:

		if (!xyswap) {
			data.minx = (input[0].x + input[3].x) / 2;
			data.maxx = (input[1].x + input[2].x) / 2;

			data.miny = (input[0].y + input[1].y) / 2;
			data.maxy = (input[3].y + input[2].y) / 2;

			xdiff = (double) (data.maxx -
					  data.minx) / (target[1].x -
							target[0].x);
			ydiff = (double) (data.maxy -
					  data.miny) / (target[2].y -
							target[1].y);

			data.xres = g_si.cols;
			data.yres = g_si.rows;
		} else {
			data.miny = (input[0].y + input[3].y) / 2;
			data.maxy = (input[1].y + input[2].y) / 2;

			data.minx = (input[0].x + input[1].x) / 2;
			data.maxx = (input[3].x + input[2].x) / 2;

			xdiff = (double) (data.maxx -
					  data.minx) / (target[2].y -
							target[1].y);
			ydiff = (double) (data.maxy -
					  data.miny) / (target[1].x -
							target[0].x);
			data.xres = g_si.rows;
			data.yres = g_si.cols;
		}

		break;

	case MWPORTRAIT_RIGHT:
	case MWPORTRAIT_LEFT:

		if (!xyswap) {
			data.minx = (input[2].x + input[3].x) / 2;
			data.maxx = (input[0].x + input[1].x) / 2;

			data.miny = (input[0].y + input[3].y) / 2;
			data.maxy = (input[1].y + input[2].y) / 2;

			ydiff = (double) (data.maxy -
					  data.miny) / (target[1].x -
							target[0].x);
			xdiff = (double) (data.maxx -
					  data.minx) / (target[2].y -
							target[1].y);

			//ydiff = (double) (data.maxx - data.minx) / (target[1].x - target[0].x);
			//xdiff = (double) (data.maxy - data.miny) / (target[2].y - target[1].y);

			data.xres = g_si.rows;
			data.yres = g_si.cols;
		} else {
			data.maxy = (input[2].y + input[3].y) / 2;
			data.miny = (input[0].y + input[1].y) / 2;

			data.minx = (input[0].x + input[3].x) / 2;
			data.maxx = (input[1].x + input[2].x) / 2;

			ydiff = (double) (data.maxy -
					  data.miny) / (target[2].y -
							target[1].y);
			xdiff = (double) (data.maxx -
					  data.minx) / (target[1].x -
							target[0].x);

			data.xres = g_si.cols;
			data.yres = g_si.rows;
		}

		break;
	}

	data.minx -= (int) (xdiff * TARGET_DIST);
	data.miny -= (int) (ydiff * TARGET_DIST);

	data.maxx += (int) (xdiff * TARGET_DIST);
	data.maxy += (int) (ydiff * TARGET_DIST);

	//printf("DEBUG:  %d,%d,%d,%d\n", 
	//data.minx, data.maxx, data.miny, data.maxy);

	/* Here's a dirty little secret - xswap and yswap don't work.  */

	if (g_rotate == MWPORTRAIT_DOWN || g_rotate == MWPORTRAIT_LEFT) {
		swapem(&data.minx, &data.maxx);
		swapem(&data.miny, &data.maxy);
	}
	//printf("TRANSFORM:  %d,%d,%d,%d\n", 
	//data.minx, data.maxx, data.miny, data.maxy);

	GrCalcTransform(&data, trans);
}

void
draw_target(GR_WINDOW_ID wid, GR_GC_ID gc, int x, int y)
{

	GrSetGCForeground(gc, GR_COLOR_WHITE);

	GrFillRect(wid, gc, x - 1, y - (CROSS_SIZE / 2) - 1, 2,
		   CROSS_SIZE / 2);
	GrFillRect(wid, gc, x - 1, y + 1, 2, CROSS_SIZE / 2);

	GrFillRect(wid, gc, x - (CROSS_SIZE / 2) - 1, y - 1, CROSS_SIZE / 2, 2);
	GrFillRect(wid, gc, x + 1, y - 1, CROSS_SIZE / 2, 2);
}

void
draw_text(char *str, int row, GR_GC_ID gc)
{
	int tw, th, tb;

	GrGetGCTextSize(gc, str, -1, GR_TFTOP, &tw, &th, &tb);
	GrText(g_wid, gc, (g_si.cols - tw) / 2, row, str, -1, GR_TFTOP);
}

void
redraw(void)
{
	GR_GC_ID gc = GrNewGC();
	GrSetGCForeground(gc, MWRGB(30, 30, 30));
	GrFillRect(g_wid, gc, 0, 0, g_si.cols, g_si.rows);

	GrSetGCForeground(gc, GR_COLOR_WHITE);
	GrSetGCBackground(gc, MWRGB(30, 30, 30));

	draw_text("Microwindows Calibration", 95, gc);
	draw_text("Touch the cross hairs firmly to", 120, gc);
	draw_text("calibrate your handheld", 140, gc);

	draw_target(g_wid, gc, target[state].x, target[state].y);
	GrDestroyGC(gc);
}

int
handle_pos(GR_EVENT_MOUSE * raw)
{

	if (raw->buttons & GR_BUTTON_L) {
		input[state].x = raw->rootx;
		input[state].y = raw->rooty;
		return 0;
	}

	if (!state && !input[state].x && !input[state].y)
		return 0;

	if (state == 3)
		return 1;

	state++;
	redraw();

	return 0;
}

/* The main calibration loop - this handles all the wierd stuff */

void
calibrate(GR_TRANSFORM * trans)
{

	GrGetScreenInfo(&g_si);

	target[0].x = TARGET_DIST;
	target[0].y = TARGET_DIST;

	target[1].x = g_si.cols - TARGET_DIST;
	target[1].y = TARGET_DIST;

	target[2].x = g_si.cols - TARGET_DIST;
	target[2].y = g_si.rows - TARGET_DIST;

	target[3].x = TARGET_DIST;
	target[3].y = g_si.rows - TARGET_DIST;

	GrSetTransform(NULL);

	g_wid = GrNewWindowEx(GR_WM_PROPS_NODECORATE, "nxcal", GR_ROOT_WINDOW_ID,
			0, 0, g_si.cols, g_si.rows, GR_COLOR_BLACK);

	GrSelectEvents(g_wid,
		       GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(g_wid);
	GrSetFocus(g_wid);	/* Force the focus */

	while (1) {
		GR_EVENT event;
		GrGetNextEvent(&event);

		if (event.type == GR_EVENT_TYPE_EXPOSURE)
			redraw();
		else if (event.type == GR_EVENT_TYPE_MOUSE_POSITION)
			if (handle_pos(&event.mouse)) {
				calculate_transform(trans);
				return;
			}
	}
}

int
main(int argc, char **argv)
{
	int ret = 0;

	int force_calibrate = 0;
	char datafile[128];

	GR_TRANSFORM trans;
	GR_SCREEN_INFO si;

	datafile[0] = 0;

	if (GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	GrGetScreenInfo(&si);
	g_rotate = si.portrait;

	while (1) {
		extern char *optarg;

		signed ch = getopt(argc, argv, "fd:");
		if (ch == -1)
			break;
		switch (ch) {
		case 'f':
			force_calibrate = 1;
			break;
		case 'd':
			strncpy(datafile, optarg, sizeof(datafile) - 1);
			break;
		default:
			usage();
		}
	}

	if (!strlen(datafile))
		strcpy(datafile, DEFAULT_DATA_FILE);

	ret = GrLoadTransformData(datafile, &trans);

	if (!ret && !force_calibrate)
		GrSetTransform(&trans);
	else {
		calibrate(&trans);
		GrSetTransform(&trans);
		GrSaveTransformData(&trans, datafile);
	}

	GrClose();
	return 0;
}
