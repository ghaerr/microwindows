/*
 * Copyright (c) 2001, 2002 Century Embedded Technologies
 *
 * Written by Jordan Crouse
 * 
 * Stipple and tile engine routines
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"

extern MWPIXELVAL gr_foreground;	/* current foreground color */
extern MWPIXELVAL gr_background;	/* current background color */
extern int gr_fillmode;			/* current fill mode        */
extern MWSTIPPLE gr_stipple;		/* The current stipple as set by the GC */
extern MWTILE gr_tile;			/* The current tile as set by the GC */
extern MWPOINT gr_ts_offset;		/* The x and y offset of the tile / stipple */

static int ts_origin_x = 0;
static int ts_origin_y = 0;

/* Some useful macros */
#define SPITCH ((gr_stipple.width + (MWIMAGE_BITSPERIMAGE - 1)) / MWIMAGE_BITSPERIMAGE)

/* This is when the bits are least significantly aligned */
/* #define BIT_SET(data, w, h) \
	(data[(h * SPITCH) + (w / MWIMAGE_BITSPERIMAGE)] & (1 << (w % MWIMAGE_BITSPERIMAGE))) */

/* This is when the bits are most significantly aligned */
#define BIT_SET(data, w, h) \
	(data[(h * SPITCH) + (w / MWIMAGE_BITSPERIMAGE)] & (1 << ((MWIMAGE_BITSPERIMAGE - 1) - (w % MWIMAGE_BITSPERIMAGE))))

void
GdSetStippleBitmap(MWIMAGEBITS *stipple, MWCOORD width, MWCOORD height)
{
	int x, y;
	int size;

	if (gr_stipple.bitmap)
		free(gr_stipple.bitmap);

	gr_stipple.width = 0;
	gr_stipple.height = 0;

	if (!stipple) {
		gr_stipple.bitmap = 0;
		return;
	}

	size = MWIMAGE_SIZE(width, height) * sizeof(MWIMAGEBITS);
	gr_stipple.bitmap = malloc(size);
	if (!gr_stipple.bitmap)
		return;
	gr_stipple.width = width;
	gr_stipple.height = height;
	memcpy(gr_stipple.bitmap, stipple, size);

	/* debug output*/
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if (BIT_SET(gr_stipple.bitmap, x, y))
				DPRINTF("X");
			else
				DPRINTF("_");
		}
		DPRINTF("\n");
	}

}

void
GdSetTilePixmap(PSD src, MWCOORD width, MWCOORD height)
{
	gr_tile.psd = src;
	if (!src) {
		gr_tile.width = 0;
		gr_tile.height = 0;
	} else {
		gr_tile.width = width;
		gr_tile.height = height;
	}
}

/* This sets the stipple offset to the specified offset */
void
GdSetTSOffset(int x, int y)
{
	gr_ts_offset.x = x;
	gr_ts_offset.y = y;
}

/* Set the bounding rect for the stipple.  This also constructs a bitmap that gives us an easy
   lookup when the time comes */

/* This only works for tiles */
static void
tile_drawrect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
{
	int sx = x, sy = y;
	int px = 0, py = 0;
	int dw = w;
	int dh = h;

	/* This is where the tile starts */
	int tilex = x - ts_origin_x;
	int tiley = y - ts_origin_y;

	/* Sanity check */
	if (!gr_tile.psd || !gr_tile.width || !gr_tile.height)
		return;

	/* Adjust the starting point to correspond with the tile offset */
	if (tilex < 0) {
		sx += -tilex;
		dw -= -tilex;

		if (sx > (x + w - 1))
			return;
		tilex = sx - ts_origin_x;
	}

	if (tiley < 0) {
		sy += -tiley;
		dh -= -tiley;

		if (sy > (y + h - 1))
			return;
		tiley = sy - ts_origin_y;
	}

	while (dh) {
		int ch = (gr_tile.height - ((tiley + py) % gr_tile.height));
		if (ch > dh)
			ch = dh;

		dw = w;
		px = 0;

		while (dw) {
			int cw = (gr_tile.width -
				 ((tilex + px) % gr_tile.width));
			if (cw > dw)
				cw = dw;

			GdBlit(psd, sx + px, sy + py, cw, ch, gr_tile.psd,
			       ((tilex + px) % gr_tile.width),
			       ((tiley + py) % gr_tile.height), MWROP_SRCCOPY);
			dw -= cw;
			px += cw;
		}
		dh -= ch;
		py += ch;
	}
}

/* This sets the origin of the stipple (we add the offset) */
/* We use this in the following functions                  */
void
set_ts_origin(int x, int y)
{
	ts_origin_x = x + gr_ts_offset.x;
	ts_origin_y = y + gr_ts_offset.y;
}

/* For these, we need to ensure that the points fall within the stipple box */
void
ts_drawpoint(PSD psd, MWCOORD x, MWCOORD y)
{
	int bx = x - ts_origin_x;
	int by = y - ts_origin_y;

	/* Sanity check - If no stipple / tile is set, then just ignore the request */
	/* FIXME:  X returns an error - Should we too?                              */
	if (gr_fillmode == MWFILL_STIPPLE
	    || gr_fillmode == MWFILL_OPAQUE_STIPPLE) {
		if (!gr_stipple.bitmap || !gr_stipple.width
		    || !gr_stipple.height)
		    	return;
	} else {
		if (!gr_tile.psd || !gr_tile.width || !gr_tile.height)
			return;
	}


	if (!GdClipPoint(psd, x, y))
		return;

	/* If the bit offset is less than zero    */
	/* Meaning that the pixel in question     */
	/* is to the left or above the current    */
	/* offset - Then just draw the foreground */
	/* FIXME:  Should we just skip the pixel instead? */
	if (bx < 0 || by < 0) {
		psd->DrawPixel(psd, x, y, gr_foreground);
		return;
	}

	switch (gr_fillmode) {
	case MWFILL_OPAQUE_STIPPLE:
		/* FIXME: precompute bitset, don't do it twice */
		if (!BIT_SET(gr_stipple.bitmap, (bx % gr_stipple.width), (by % gr_stipple.height)))
		     	psd->DrawPixel(psd, x, y, gr_background);

		/* Fall through */
	case MWFILL_STIPPLE:
		if (BIT_SET(gr_stipple.bitmap, (bx % gr_stipple.width), (by % gr_stipple.height)))
			psd->DrawPixel(psd, x, y, gr_foreground);
		break;

	case MWFILL_TILE:
		/* Read the bit from the PSD and write it to the current PSD */
		/* FIXME:  This does no checks for depth correctness         */
		psd->DrawPixel(psd, x, y,
			gr_tile.psd->ReadPixel(gr_tile.psd,
				(bx % gr_tile.width), (by % gr_tile.height)));
		break;
	}
}

/* FIXME:  Optimize the stipple so it uses more bliting and less pixel by pixel stuff */
void
ts_drawrow(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y)
{
	int x;
	int dstwidth = x2 - x1 + 1;

	switch (gr_fillmode) {
	case MWFILL_STIPPLE:
	case MWFILL_OPAQUE_STIPPLE:
		for (x = x1; x <= x2; x++)
			ts_drawpoint(psd, x, y);
		break;
	case MWFILL_TILE:
		tile_drawrect(psd, x1, y, dstwidth, 1);
	}
}

void
ts_fillrect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
{
	int x1 = x;
	int x2 = x + w - 1;
	int y1 = y;
	int y2 = y + h - 1;

	if (GdClipArea(psd, x1, y1, x2, y2) == CLIP_INVISIBLE)
		return;

	if (gr_fillmode == MWFILL_TILE)
		tile_drawrect(psd, x, y, w, h);
	else
		for (; y1 <= y2; y1++)
			ts_drawrow(psd, x1, x2, y1);
}
