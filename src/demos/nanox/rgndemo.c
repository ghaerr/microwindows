/* 
 * rgndemo - A program to demonstrate the use of various different
 * types of regions for Nano-X.
 *
 * Copyright (c) 2002 Alex Holden.
 */

#include <stdio.h>
#include <nano-X.h>
#include <nxcolors.h>

#include "tuxmask.h"

#define WINDOW_WIDTH 190
#define WINDOW_HEIGHT 305
#define NUMGCS 5


void redraw(GR_WINDOW_ID wid, GR_GC_ID *gc);
GR_GC_ID setup_single_rect_region(void);
GR_GC_ID setup_multi_rect_region(void);
GR_GC_ID setup_simple_poly_region(void);
GR_GC_ID setup_bitmap_region(void);
GR_GC_ID setup_complex_poly_region(void);


void redraw(GR_WINDOW_ID wid, GR_GC_ID *gc)
{
	int i, y;

	for(i = 0; i < NUMGCS; i++)
		for(y = 40; y < WINDOW_HEIGHT; y += 3)
			GrLine(wid, gc[i], 0, y, WINDOW_WIDTH - 1, y - 40);
}

GR_GC_ID setup_single_rect_region(void)
{
	GR_GC_ID gc;
	GR_RECT rect;
	GR_REGION_ID rg;

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_BLACK);
	GrSetGCBackground(gc, GR_COLOR_WHITE);
	rg = GrNewRegion();
	rect.x = 0;
	rect.y = 0;
	rect.width = 60;
	rect.height = 40;
	GrUnionRectWithRegion(rg, &rect);
	GrSetGCRegion(gc, rg);
	GrSetGCClipOrigin(gc, 25, 35);

	return gc;
}

GR_GC_ID setup_multi_rect_region(void)
{
	GR_GC_ID gc;
	GR_RECT rect;
	GR_REGION_ID rg;

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_BLUE);
	GrSetGCBackground(gc, GR_COLOR_WHITE);
	rg = GrNewRegion();
	rect.x = 10;
	rect.y = 0;
	rect.width = 10;
	rect.height = 60;
	GrUnionRectWithRegion(rg, &rect);
	rect.x = 40;
	rect.y = 0;
	rect.width = 10;
	rect.height = 60;
	GrUnionRectWithRegion(rg, &rect);
	rect.x = 0;
	rect.y = 10;
	rect.width = 60;
	rect.height = 10;
	GrUnionRectWithRegion(rg, &rect);
	rect.x = 0;
	rect.y = 40;
	rect.width = 60;
	rect.height = 10;
	GrUnionRectWithRegion(rg, &rect);
	GrSetGCRegion(gc, rg);
	GrSetGCClipOrigin(gc, 105, 15);

	return gc;
}

GR_GC_ID setup_simple_poly_region(void)
{
	GR_GC_ID gc;
	GR_REGION_ID rg;
	GR_POINT points[] = { {15, 0}, {45, 0}, {60, 15}, {60, 45}, {45, 60},
		{15, 60}, {0, 45}, {0, 15} };

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_PURPLE);
	GrSetGCBackground(gc, GR_COLOR_WHITE);
	rg = GrNewPolygonRegion(GR_POLY_WINDING, 8, points);
	GrSetGCRegion(gc, rg);
	GrSetGCClipOrigin(gc, 25, 95);

	return gc;
}

GR_GC_ID setup_bitmap_region(void)
{
	GR_GC_ID gc;
	GR_REGION_ID rg;

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_FORESTGREEN);
	GrSetGCBackground(gc, GR_COLOR_WHITE);
	rg = GrNewBitmapRegion(tuxmask_bits, TUXMASK_WIDTH, TUXMASK_HEIGHT);
	GrSetGCRegion(gc, rg);
	GrSetGCClipOrigin(gc, 100, 80);

	return gc;
}

GR_GC_ID setup_complex_poly_region(void)
{
	GR_GC_ID gc;
	GR_REGION_ID rg;
	GR_POINT points[] = { {0, 0}, {99, 0}, {99, 99}, {0, 99}, {0, 19},
		{79, 19}, {79, 79}, {19, 79}, {19, 39}, {59, 39}, {59, 49},
		{29, 49}, {29, 69}, {69, 69}, {69, 29}, {9, 29}, {9, 89},
		{89, 89}, {89, 9}, {0, 9} };

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_ORANGE);
	GrSetGCBackground(gc, GR_COLOR_WHITE);
	rg = GrNewPolygonRegion(GR_POLY_EVENODD, 20, points);
	GrSetGCRegion(gc, rg);
	GrSetGCClipOrigin(gc, 25, 180);

	return gc;
}

int main(void)
{
	GR_WINDOW_ID wid;
	GR_GC_ID gc[NUMGCS];
	GR_EVENT event;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to the Nano-X server\n");
		return 1;
	}

	wid = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "rgndemo",
		GR_ROOT_WINDOW_ID, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		GR_COLOR_GRAY80);

	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_EXPOSURE);

	gc[0] = setup_single_rect_region();
	gc[1] = setup_multi_rect_region();
	gc[2] = setup_simple_poly_region();
	gc[3] = setup_bitmap_region();
	gc[4] = setup_complex_poly_region();

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
