/*
 * Copyright (c) 1999, 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Device-independent mid level drawing and color routines.
 *
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "device.h"

extern MWPIXELVAL gr_foreground;      /* current foreground color */
extern MWPIXELVAL gr_background;      /* current background color */
extern MWBOOL 	  gr_usebg;    	      /* TRUE if background drawn in pixmaps */
extern int 	  gr_mode; 	      /* drawing mode */
extern MWPALENTRY gr_palette[256];    /* current palette*/
extern int	  gr_firstuserpalentry;/* first user-changable palette entry*/
extern int 	  gr_nextpalentry;    /* next available palette entry*/

/* These support drawing dashed lines */
extern unsigned long gr_dashmask;     /* An actual bitmask of the dash values */
extern unsigned long gr_dashcount;    /* The number of bits defined in the dashmask */

extern int        gr_fillmode;

/*static*/ void drawpoint(PSD psd,MWCOORD x, MWCOORD y);

/*static*/ void drawrow(PSD psd,MWCOORD x1,MWCOORD x2,MWCOORD y);
static void drawcol(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2);

/**
 * Set the drawing mode for future calls.
 *
 * @param mode New drawing mode.
 * @return Old drawing mode.
 */
int
GdSetMode(int mode)
{
	int oldmode = gr_mode;

	gr_mode = mode;
	return oldmode;
}

/**
 * Set the fill mode for future calls.
 *
 * @param mode New fill mode.
 * @return Old fill mode.
 */
int
GdSetFillMode(int mode)
{
	int oldmode = gr_fillmode;

	gr_fillmode = mode;
	return oldmode;
}

/**
 * Set whether or not the background is used for drawing pixmaps and text.
 *
 * @param flag Flag indicating whether or not to draw the background.
 * @return Old value of flag.
 */
MWBOOL
GdSetUseBackground(MWBOOL flag)
{
	MWBOOL oldusebg = gr_usebg;

	gr_usebg = flag;
	return oldusebg;
}

/*
 * Set the foreground color for drawing from passed pixel value.
 *
 * @param psd Screen device.
 * @param bg Background pixel value.
 */
MWPIXELVAL
GdSetForegroundPixelVal(PSD psd, MWPIXELVAL fg)
{
	MWPIXELVAL oldfg = gr_foreground;

	gr_foreground = fg;
	return oldfg;
}

/*
 * Set the background color for bitmap and text backgrounds
 * from passed pixel value.
 *
 * @param psd Screen device.
 * @param bg Background pixel value.
 */
MWPIXELVAL
GdSetBackgroundPixelVal(PSD psd, MWPIXELVAL bg)
{
	MWPIXELVAL oldbg = gr_background;

	gr_background = bg;
	return oldbg;
}

/**
 * Set the foreground color for drawing from passed RGB color value.
 *
 * @param psd Screen device.
 * @param fg Foreground RGB color to use for drawing.
 * @return Old foreground color.
 */
MWPIXELVAL
GdSetForegroundColor(PSD psd, MWCOLORVAL fg)
{
	MWPIXELVAL oldfg = gr_foreground;

	gr_foreground = GdFindColor(psd, fg);
	return oldfg;
}

/**
 * Set the background color for bitmap and text backgrounds
 * from passed RGB color value.
 *
 * @param psd Screen device.
 * @param bg Background color to use for drawing.
 * @return Old background color.
 */
MWPIXELVAL
GdSetBackgroundColor(PSD psd, MWCOLORVAL bg)
{
	MWPIXELVAL oldbg = gr_background;

	gr_background = GdFindColor(psd, bg);
	return oldbg;
}

/**
 * Set the dash mode for future drawing
 */
void
GdSetDash(unsigned long *mask, int *count)
{
	int oldm = gr_dashmask;
	int oldc = gr_dashcount;

	if (!mask || !count)
		return;

	gr_dashmask = *mask;
	gr_dashcount = *count;

	*mask = oldm;
	*count = oldc;
}

/**
 * Draw a point using the current clipping region and foreground color.
 *
 * @param psd Drawing surface.
 * @param x X co-ordinate to draw point.
 * @param y Y co-ordinate to draw point.
 */
void
GdPoint(PSD psd, MWCOORD x, MWCOORD y)
{
	if (GdClipPoint(psd, x, y)) {
		psd->DrawPixel(psd, x, y, gr_foreground);
		GdFixCursor(psd);
	}
}

/**
 * Draw an arbitrary line using the current clipping region and foreground color
 * If bDrawLastPoint is FALSE, draw up to but not including point x2, y2.
 *
 * This routine is the only routine that adjusts coordinates for supporting
 * two different types of upper levels, those that draw the last point
 * in a line, and those that draw up to the last point.  All other local
 * routines draw the last point.  This gives this routine a bit more overhead,
 * but keeps overall complexity down.
 *
 * @param psd Drawing surface.
 * @param x1 Start X co-ordinate
 * @param y1 Start Y co-ordinate
 * @param x2 End X co-ordinate
 * @param y2 End Y co-ordinate
 * @param bDrawLastPoint TRUE to draw the point at (x2, y2).  FALSE to omit it.
 */
void
GdLine(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
       MWBOOL bDrawLastPoint) 
{
	int xdelta;		/* width of rectangle around line */
	int ydelta;		/* height of rectangle around line */
	int xinc;		/* increment for moving x coordinate */
	int yinc;		/* increment for moving y coordinate */
	int rem;		/* current remainder */
	unsigned int bit = 0;	/* used for dashed lines */
	MWCOORD temp;

	/* See if the line is horizontal or vertical. If so, then call
	 * special routines.
	 */
	if (y1 == y2) {
		/*
		 * Adjust coordinates if not drawing last point.  Tricky.
		 */
		if (!bDrawLastPoint) {
			if (x1 > x2) {
				temp = x1;
				x1 = x2 + 1;
				x2 = temp;
			} else
				--x2;
		}

		/* call faster line drawing routine */
		drawrow(psd, x1, x2, y1);
		GdFixCursor(psd);
		return;
	}
	if (x1 == x2) {
		/*
		 * Adjust coordinates if not drawing last point.  Tricky.
		 */
		if (!bDrawLastPoint) {
			if (y1 > y2) {
				temp = y1;
				y1 = y2 + 1;
				y2 = temp;
			} else
				--y2;
		}

		/* call faster line drawing routine */
		drawcol(psd, x1, y1, y2);
		GdFixCursor(psd);
		return;
	}

	/* See if the line is either totally visible or totally invisible. If
	 * so, then the line drawing is easy.
	 */
	switch (GdClipArea(psd, x1, y1, x2, y2)) {
	case CLIP_VISIBLE:
		/*
		 * For size considerations, there's no low-level bresenham
		 * line draw, so we've got to draw all non-vertical
		 * and non-horizontal lines with per-point
		 * clipping for the time being
		 psd->Line(psd, x1, y1, x2, y2, gr_foreground);
		 GdFixCursor(psd);
		 return;
		 */
		break;
	case CLIP_INVISIBLE:
		return;
	}

	/* The line may be partially obscured. Do the draw line algorithm
	 * checking each point against the clipping regions.
	 */
	xdelta = x2 - x1;
	ydelta = y2 - y1;
	if (xdelta < 0)
		xdelta = -xdelta;
	if (ydelta < 0)
		ydelta = -ydelta;
	xinc = (x2 > x1)? 1 : -1;
	yinc = (y2 > y1)? 1 : -1;

	/* draw first point*/
	if (GdClipPoint(psd, x1, y1))
		psd->DrawPixel(psd, x1, y1, gr_foreground);

	if (xdelta >= ydelta) {
		rem = xdelta / 2;
		for (;;) {
			if (!bDrawLastPoint && x1 == x2)
				break;
			x1 += xinc;
			rem += ydelta;
			if (rem >= xdelta) {
				rem -= xdelta;
				y1 += yinc;
			}

			if (gr_dashcount) {
				if ((gr_dashmask & (1 << bit)) && GdClipPoint(psd, x1, y1))
					psd->DrawPixel(psd, x1, y1, gr_foreground);

				bit = (bit + 1) % gr_dashcount;
			} else {	/* No dashes */
				if (GdClipPoint(psd, x1, y1))
					psd->DrawPixel(psd, x1, y1, gr_foreground);
			}

			if (bDrawLastPoint && x1 == x2)
				break;

		}
	} else {
		rem = ydelta / 2;
		for (;;) {
			if (!bDrawLastPoint && y1 == y2)
				break;
			y1 += yinc;
			rem += xdelta;
			if (rem >= ydelta) {
				rem -= ydelta;
				x1 += xinc;
			}

			/* If we are trying to draw to a dash mask */
			if (gr_dashcount) {
				if ((gr_dashmask & (1 << bit)) && GdClipPoint(psd, x1, y1))
					psd->DrawPixel(psd, x1, y1, gr_foreground);

				bit = (bit + 1) % gr_dashcount;
			} else {	/* No dashes */
				if (GdClipPoint(psd, x1, y1))
					psd->DrawPixel(psd, x1, y1, gr_foreground);
			}

			if (bDrawLastPoint && y1 == y2)
				break;
		}
	}
	GdFixCursor(psd);
}

/* Draw a point in the foreground color, applying clipping if necessary*/
/*static*/ void
drawpoint(PSD psd, MWCOORD x, MWCOORD y)
{
	if (GdClipPoint(psd, x, y))
		psd->DrawPixel(psd, x, y, gr_foreground);
}

/* Draw a horizontal line from x1 to and including x2 in the
 * foreground color, applying clipping if necessary.
 */
/*static*/ void
drawrow(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y)
{
	MWCOORD temp;

	/* reverse endpoints if necessary */
	if (x1 > x2) {
		temp = x1;
		x1 = x2;
		x2 = temp;
	}

	/* clip to physical device */
	if (x1 < 0)
		x1 = 0;
	if (x2 >= psd->xvirtres)
		x2 = psd->xvirtres - 1;

	/* check cursor intersect once for whole line */
	GdCheckCursor(psd, x1, y, x2, y);

	/* If aren't trying to draw a dash, then head for the speed */

	if (!gr_dashcount) {
		while (x1 <= x2) {
			if (GdClipPoint(psd, x1, y)) {
				temp = MWMIN(clipmaxx, x2);
				psd->DrawHorzLine(psd, x1, temp, y, gr_foreground);
			} else
				temp = MWMIN(clipmaxx, x2);
			x1 = temp + 1;
		}
	} else {
		unsigned int p, bit = 0;

		/* We want to draw a dashed line instead */
		for (p = x1; p <= x2; p++) {
			if ((gr_dashmask & (1 << bit)) && GdClipPoint(psd, p, y))
				psd->DrawPixel(psd, p, y, gr_foreground);

			bit = (bit + 1) % gr_dashcount;
		}
	}
}

/* Draw a vertical line from y1 to and including y2 in the
 * foreground color, applying clipping if necessary.
 */
static void
drawcol(PSD psd, MWCOORD x,MWCOORD y1,MWCOORD y2)
{
	MWCOORD temp;

	/* reverse endpoints if necessary */
	if (y1 > y2) {
		temp = y1;
		y1 = y2;
		y2 = temp;
	}

	/* clip to physical device */
	if (y1 < 0)
		y1 = 0;
	if (y2 >= psd->yvirtres)
		y2 = psd->yvirtres - 1;

	/* check cursor intersect once for whole line */
	GdCheckCursor(psd, x, y1, x, y2);

	if (!gr_dashcount) {
		while (y1 <= y2) {
			if (GdClipPoint(psd, x, y1)) {
				temp = MWMIN(clipmaxy, y2);
				psd->DrawVertLine(psd, x, y1, temp, gr_foreground);
			} else
				temp = MWMIN(clipmaxy, y2);
			y1 = temp + 1;
		}
	} else {
		unsigned int p, bit = 0;

		/* We want to draw a dashed line instead */
		for (p = y1; p <= y2; p++) {
			if ((gr_dashmask & (1<<bit)) && GdClipPoint(psd, x, p))
				psd->DrawPixel(psd, x, p, gr_foreground);

			bit = (bit + 1) % gr_dashcount;
		}
	}
}

/**
 * Draw a rectangle in the foreground color, applying clipping if necessary.
 * This is careful to not draw points multiple times in case the rectangle
 * is being drawn using XOR.
 *
 * @param psd Drawing surface.
 * @param x Left edge of rectangle.
 * @param y Top edge of rectangle.
 * @param width Width of rectangle.
 * @param height Height of rectangle.
 */
void
GdRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	MWCOORD maxx;
	MWCOORD maxy;

	if (width <= 0 || height <= 0)
		return;
	maxx = x + width - 1;
	maxy = y + height - 1;
	drawrow(psd, x, maxx, y);
	if (height > 1)
		drawrow(psd, x, maxx, maxy);
	if (height < 3)
		return;
	++y;
	--maxy;
	drawcol(psd, x, y, maxy);
	if (width > 1)
		drawcol(psd, maxx, y, maxy);
	GdFixCursor(psd);
}

/**
 * Draw a filled in rectangle in the foreground color, applying
 * clipping if necessary.
 *
 * @param psd Drawing surface.
 * @param x1 Left edge of rectangle.
 * @param y1 Top edge of rectangle.
 * @param width Width of rectangle.
 * @param height Height of rectangle.
 */
void
GdFillRect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD width, MWCOORD height)
{
	unsigned long dm = 0, dc = 0;

	MWCOORD x2 = x1 + width - 1;
	MWCOORD y2 = y1 + height - 1;

	if (width <= 0 || height <= 0)
		return;

	/* Stipples and tiles have their own drawing routines */
	if (gr_fillmode != MWFILL_SOLID) {
		set_ts_origin(x1, y1);

		ts_fillrect(psd, x1, y1, width, height);
		GdFixCursor(psd);
		return;
	}

	/* See if the rectangle is either totally visible or totally
	 * invisible. If so, then the rectangle drawing is easy.
	 */
	switch (GdClipArea(psd, x1, y1, x2, y2)) {
	case CLIP_VISIBLE:
		psd->FillRect(psd, x1, y1, x2, y2, gr_foreground);
		GdFixCursor(psd);
		return;

	case CLIP_INVISIBLE:
		return;
	}


	/* Quickly save off the dash settings to avoid problems with drawrow */
	GdSetDash(&dm, (int *) &dc);

	/* The rectangle may be partially obstructed. So do it line by line. */
	while (y1 <= y2)
		drawrow(psd, x1, x2, y1++);

	/* Restore the dash settings */
	GdSetDash(&dm, (int *) &dc);

	GdFixCursor(psd);
}

/**
 * Draw a rectangular area using the current clipping region and the
 * specified bit map.  This differs from rectangle drawing in that the
 * rectangle is drawn using the foreground color and possibly the background
 * color as determined by the bit map.  Each row of bits is aligned to the
 * next bitmap word boundary (so there is padding at the end of the row).
 * (I.e. each row begins at the start of a new MWIMAGEBITS value).
 * The background bit values are only written if the gr_usebg flag
 * is set.
 * The function drawbitmap performs no clipping, GdBitmap clips.
 *
 * @param psd Drawing surface.
 * @param x Left edge of destination rectangle.
 * @param y Top edge of destination rectangle.
 * @param width Width of bitmap.  Equal to width of destination rectangle.
 * @param height Height of bitmap.  Equal to height of destination rectangle.
 * @param imagebits The bitmap to draw.
 */
void
drawbitmap(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	const MWIMAGEBITS *imagebits)
{
	MWCOORD minx;
	MWCOORD maxx;
	MWIMAGEBITS bitvalue = 0;	/* bitmap word value */
	int bitcount;			/* number of bits left in bitmap word */

	if (width <= 0 || height <= 0)
		return;

	if (gr_usebg)
		psd->FillRect(psd, x, y, x + width - 1, y + height - 1,
			gr_background);

	/* FIXME think of the speedups if this existed...
	psd->DrawBitmap(psd, x, y, width, height, imagebits, gr_foreground);
	return;
	*/

	minx = x;
	maxx = x + width - 1;
	bitcount = 0;
	while (height > 0) {
		if (bitcount <= 0) {
			bitcount = MWIMAGE_BITSPERIMAGE;
			bitvalue = *imagebits++;
		}
		/* draw without clipping*/
		if (MWIMAGE_TESTBIT(bitvalue))
			psd->DrawPixel(psd, x, y, gr_foreground);
		bitvalue = MWIMAGE_SHIFTBIT(bitvalue);
		bitcount--;
		if (x++ == maxx) {
			x = minx;
			y++;
			--height;
			bitcount = 0;
		}
	}
}

void
GdBitmap(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	const MWIMAGEBITS *imagebits)
{
	MWCOORD minx;
	MWCOORD maxx;
	MWPIXELVAL savecolor;		/* saved foreground color */
	MWIMAGEBITS bitvalue = 0;	/* bitmap word value */
	int bitcount;			/* number of bits left in bitmap word */

	if (width <= 0 || height <= 0)
		return;

	switch (GdClipArea(psd, x, y, x + width - 1, y + height - 1)) {
	case CLIP_VISIBLE:
		drawbitmap(psd, x, y, width, height, imagebits);
		GdFixCursor(psd);
		return;

	case CLIP_INVISIBLE:
		return;
	}

	/* The rectangle is partially visible, so must do clipping. First
	 * fill a rectangle in the background color if necessary.
	 */
	if (gr_usebg) {
		savecolor = gr_foreground;
		gr_foreground = gr_background;
		/* note: change to fillrect*/
		GdFillRect(psd, x, y, width, height);
		gr_foreground = savecolor;
	}
	minx = x;
	maxx = x + width - 1;
	bitcount = 0;
	while (height > 0) {
		if (bitcount <= 0) {
			bitcount = MWIMAGE_BITSPERIMAGE;
			bitvalue = *imagebits++;
		}
		if (MWIMAGE_TESTBIT(bitvalue) && GdClipPoint(psd, x, y))
			psd->DrawPixel(psd, x, y, gr_foreground);
			bitvalue = MWIMAGE_SHIFTBIT(bitvalue);
			bitcount--;
		if (x++ == maxx) {
			x = minx;
			y++;
			--height;
			bitcount = 0;
		}
	}
	GdFixCursor(psd);
}

/**
 * Return true if color is in palette
 *
 * @param cr Color to look for.
 * @param palette Palette to look in.
 * @param palsize Size of the palette.
 * @return TRUE iff the color is in palette.
 */
MWBOOL
GdColorInPalette(MWCOLORVAL cr,MWPALENTRY *palette,int palsize)
{
	int	i;

	for(i=0; i<palsize; ++i)
		if(GETPALENTRY(palette, i) == cr)
			return TRUE;
	return FALSE;
}

/**
 * Create a MWPIXELVAL conversion table between the passed palette
 * and the in-use palette.  The system palette is loaded/merged according
 * to fLoadType.
 *
 * FIXME: LOADPALETTE and MERGEPALETTE are defined in "device.h"
 *
 * @param psd Drawing surface.
 * @param palette Palette to look in.
 * @param palsize Size of the palette.
 * @param convtable Destination for the conversion table.  Will hold palsize
 * entries.
 * @param fLoadType LOADPALETTE to set the surface's palette to the passed
 * palette, MERGEPALETTE to add the passed colors to the surface
 * palette without removing existing colors, or 0.
 */
void
GdMakePaletteConversionTable(PSD psd,MWPALENTRY *palette,int palsize,
	MWPIXELVAL *convtable,int fLoadType)
{
	int		i;
	MWCOLORVAL	cr;
	int		newsize, nextentry;
	MWPALENTRY	newpal[256];

	/*
	 * Check for load palette completely, or add colors
	 * from passed palette to system palette until full.
	 */
	if(psd->pixtype == MWPF_PALETTE) {
	    switch(fLoadType) {
	    case LOADPALETTE:
		/* Load palette from beginning with image's palette.
		 * First palette entries are Microwindows colors
		 * and not changed.
		 */
		GdSetPalette(psd, gr_firstuserpalentry, palsize, palette);
		break;

	    case MERGEPALETTE:
		/* get system palette*/
		for(i=0; i<(int)psd->ncolors; ++i)
			newpal[i] = gr_palette[i];

		/* merge passed palette into system palette*/
		newsize = 0;
		nextentry = gr_nextpalentry;

		/* if color missing and there's room, add it*/
		for(i=0; i<palsize && nextentry < (int)psd->ncolors; ++i) {
			cr = GETPALENTRY(palette, i);
			if(!GdColorInPalette(cr, newpal, nextentry)) {
				newpal[nextentry++] = palette[i];
				++newsize;
			}
		}

		/* set the new palette if any color was added*/
		if(newsize) {
			GdSetPalette(psd, gr_nextpalentry, newsize,
				&newpal[gr_nextpalentry]);
			gr_nextpalentry += newsize;
		}
		break;
	    }
	}

	/*
	 * Build conversion table from inuse system palette and
	 * passed palette.  This will load RGB values directly
	 * if running truecolor, otherwise it will find the
	 * nearest color from the inuse palette.
	 * FIXME: tag the conversion table to the bitmap image
	 */
	for(i=0; i<palsize; ++i) {
		cr = GETPALENTRY(palette, i);
		convtable[i] = GdFindColor(psd, cr);
	}
}

/**
 * Draw a color bitmap image in 1, 4, 8, 24 or 32 bits per pixel.  The
 * Microwindows color image format is DWORD padded bytes, with
 * the upper bits corresponding to the left side (identical to
 * the MS Windows format).  This format is currently different
 * than the MWIMAGEBITS format, which uses word-padded bits
 * for monochrome display only, where the upper bits in the word
 * correspond with the left side.
 *
 * @param psd Drawing surface.
 * @param x Destination X co-ordinate for left of image.
 * @param y Destination Y co-ordinate for top of image.
 * @param pimage Structure describing the image.
 */
void
GdDrawImage(PSD psd, MWCOORD x, MWCOORD y, PMWIMAGEHDR pimage)
{
	MWCOORD minx;
	MWCOORD maxx;
	MWUCHAR bitvalue = 0;
	int bitcount;
	MWUCHAR *imagebits;
	MWCOORD height, width;
	int bpp;
	MWPIXELVAL pixel;
	int clip;
	int extra, linesize;
	int rgborder;
	MWCOLORVAL cr;
	MWCOORD yoff;
	unsigned long transcolor;
	MWPIXELVAL convtable[256];

	assert(pimage);

	height = pimage->height;
	width = pimage->width;

	/* determine if entire image is clipped out, save clipresult for later*/
	clip = GdClipArea(psd, x, y, x + width - 1, y + height - 1);
	if(clip == CLIP_INVISIBLE)
		return;

	transcolor = pimage->transcolor;
	bpp = pimage->bpp;

	/*
	 * Merge the images's palette and build a palette index conversion table.
	 */
	if (pimage->bpp <= 8) {
		if(!pimage->palette) {
			/* for jpeg's without a palette*/
			for(yoff=0; yoff<pimage->palsize; ++yoff)
				convtable[yoff] = yoff;
		} else GdMakePaletteConversionTable(psd, pimage->palette,
			pimage->palsize, convtable, MERGEPALETTE);

		/* The following is no longer used.  One reason is that it required */
		/* the transparent color to be unique, which was unnessecary        */

		/* convert transcolor to converted palette index for speed*/
		/* if (transcolor != -1L)
		   transcolor = (unsigned long) convtable[transcolor];  */
	}

	minx = x;
	maxx = x + width - 1;
	imagebits = pimage->imagebits;

	/* check for bottom-up image*/
	if(pimage->compression & MWIMAGE_UPSIDEDOWN) {
		y += height - 1;
		yoff = -1;
	} else
		yoff = 1;

#define PIX2BYTES(n)	(((n)+7)/8)
	/* imagebits are dword aligned*/
	switch(pimage->bpp) {
	default:
	case 8:
		linesize = width;
		break;
	case 32:
		linesize = width*4;
		break;
	case 24:
		linesize = width*3;
		break;
	case 4:
		linesize = PIX2BYTES(width<<2);
		break;
	case 1:
		linesize = PIX2BYTES(width);
		break;
	}
	extra = pimage->pitch - linesize;

	/* 24bpp RGB rather than BGR byte order?*/
	rgborder = pimage->compression & MWIMAGE_RGB; 

	if ((bpp == 32)
	    && ((pimage->compression & MWIMAGE_ALPHA_CHANNEL) != 0)) {
		long *data = (long *) imagebits;

		/* DPRINTF("Nano-X: GdDrawImage (%d,%d) %dx%d x=%d-%d\n  ",
		   x,y,width,height,minx,maxx); */

		while (height > 0) {

			cr = *data++;
#if MW_CPU_BIG_ENDIAN
			if (rgborder)
			{
				/* Fix endian and swap R/B order */
				cr =  ((cr & 0xFFFFFF00UL) >> 8)
					| ((cr & 0x000000FFUL) << 24);
			}
			else
			{
				/* Fix endian */
				cr =  ((cr & 0xFF000000UL) >> 24)
					| ((cr & 0x00FF0000UL) >> 8)
					| ((cr & 0x0000FF00UL) << 8)
					| ((cr & 0x000000FFUL) << 24);
			}
#else /* little endian*/
			if (rgborder) {
				/* Swap R/B order */
				cr = (cr & 0xFF00FF00UL)
					| ((cr & 0x00FF0000UL) >> 16)
					| ((cr & 0x000000FFUL) << 16);
			}
#endif

			switch (psd->pixtype) {
			case MWPF_PALETTE:
			default:
				pixel = GdFindColor(psd, cr);
				break;
			case MWPF_TRUECOLOR8888:
				pixel = COLOR2PIXEL8888(cr);
				break;
			case MWPF_TRUECOLOR0888:
			case MWPF_TRUECOLOR888:
				pixel = COLOR2PIXEL888(cr);
				break;
			case MWPF_TRUECOLOR565:
				pixel = COLOR2PIXEL565(cr);
				break;
			case MWPF_TRUECOLOR555:
				pixel = COLOR2PIXEL555(cr);
				break;
			case MWPF_TRUECOLOR332:
				pixel = COLOR2PIXEL332(cr);
				break;
			}

			if (clip == CLIP_VISIBLE || GdClipPoint(psd, x, y))
				psd->DrawPixel(psd, x, y, pixel);

			if (x++ == maxx) {
				/* printf("EOL\n  "); */
				x = minx;
				y += yoff;
				height--;
				data = (long *) (((char *) data) + extra);
			}
		}
		/* printf("End of image\n"); */
	} else if ((bpp == 24) || (bpp == 32)) {
		long trans;

		while (height > 0) {
			/* RGB rather than BGR byte order? */
			trans = cr = rgborder
				? MWRGB(imagebits[0], imagebits[1],
					imagebits[2])
				: MWRGB(imagebits[2], imagebits[1],
					imagebits[0]);

			imagebits += 3;

			if (bpp == 32) {
				/*
				 * FIXME Currently, XPM is the only image
				 * decoder that creates 32bpp images with
				 * transparency. This is done specifying the
				 * transparent color 0x01000000, using 0x01
				 * in the alpha channel as the indicator.
				 */
				if (*imagebits++ == 0x01)
					trans = 0x01000000;
			}

			/* handle transparent color */
			if (transcolor != trans) {

				switch (psd->pixtype) {
				case MWPF_PALETTE:
				default:
					pixel = GdFindColor(psd, cr);
					break;
				case MWPF_TRUECOLOR8888:
					pixel = COLOR2PIXEL8888(cr);
					break;
				case MWPF_TRUECOLOR0888:
				case MWPF_TRUECOLOR888:
					pixel = COLOR2PIXEL888(cr);
					break;
				case MWPF_TRUECOLOR565:
					pixel = COLOR2PIXEL565(cr);
					break;
				case MWPF_TRUECOLOR555:
					pixel = COLOR2PIXEL555(cr);
					break;
				case MWPF_TRUECOLOR332:
					pixel = COLOR2PIXEL332(cr);
					break;
				}

				if (clip == CLIP_VISIBLE
				    || GdClipPoint(psd, x, y))
					psd->DrawPixel(psd, x, y, pixel);
#if 0
				/* fix: use clipmaxx to clip quicker */
				else if (clip != CLIP_VISIBLE && !clipresult
					 && x > clipmaxx) {
					x = maxx;
				}
#endif
			}

			if (x++ == maxx) {
				x = minx;
				y += yoff;
				height--;
				imagebits += extra;
			}
		}
	} else {  /* bpp == 8, 4, or 1, palettized image. */

		bitcount = 0;
		while (height > 0) {
			if (bitcount <= 0) {
				bitcount = sizeof(MWUCHAR) * 8;
				bitvalue = *imagebits++;
			}
			switch (bpp) {
			default:
			case 8:
				bitcount = 0;
				if (bitvalue == transcolor)
					goto next;

				pixel = convtable[bitvalue];
				break;
			case 4:
				if (((bitvalue & 0xf0) >> 4) == transcolor) {
					bitvalue <<= 4;
					bitcount -= 4;
					goto next;
				}

				pixel = convtable[(bitvalue & 0xf0) >> 4];
				bitvalue <<= 4;
				bitcount -= 4;
				break;
			case 1:
				--bitcount;
				if (((bitvalue & 0x80) ? 1 : 0) == transcolor) {
					bitvalue <<= 1;
					goto next;
				}

				pixel = convtable[(bitvalue & 0x80) ? 1 : 0];
				bitvalue <<= 1;
				break;
			}

			if (clip == CLIP_VISIBLE || GdClipPoint(psd, x, y))
				psd->DrawPixel(psd, x, y, pixel);
#if 0
			/* fix: use clipmaxx to clip quicker */
			else if (clip != CLIP_VISIBLE && !clipresult
				 && x > clipmaxx) {
				x = maxx;
			}
#endif
		      next:
			if (x++ == maxx) {
				x = minx;
				y += yoff;
				height--;
				bitcount = 0;
				imagebits += extra;
			}
		}
	}
	GdFixCursor(psd);
}

/**
 * Read a rectangular area of the screen.
 * The color table is indexed row by row.
 *
 * @param psd Drawing surface.
 * @param x Left edge of rectangle to read.
 * @param y Top edge of rectangle to read.
 * @param width Width of rectangle to read.
 * @param height Height of rectangle to read.
 * @param pixels Destination for screen grab.
 */
void
GdReadArea(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	MWPIXELVAL *pixels)
{
	MWCOORD 		row;
	MWCOORD 		col;

	if (width <= 0 || height <= 0)
		return;

	GdCheckCursor(psd, x, y, x+width-1, y+height-1);
	for (row = y; row < height+y; row++)
		for (col = x; col < width+x; col++)
			if (row < 0 || row >= psd->yvirtres ||
			    col < 0 || col >= psd->xvirtres)
				*pixels++ = 0;
			else *pixels++ = psd->ReadPixel(psd, col, row);

	GdFixCursor(psd);
}

/*
 * A wrapper for psd->DrawArea which performs clipping.
 * The gc->dst[x,y,w,h] values are clipped.  The gc->src[x,y]
 * values are adjusted accordingly.
 *
 * This function does NOT have a fallback implementation
 * if the function is not supported by the driver.
 *
 * It is the caller's responsibility to GdFixCursor(psd).
 *
 * This is a low-level function.
 */
void
GdDrawAreaInternal(PSD psd, driver_gc_t * gc, int op)
{
	MWCOORD x = gc->dstx;
	MWCOORD y = gc->dsty;
	MWCOORD width = gc->dstw;
	MWCOORD height = gc->dsth;
	MWCOORD srcx;
	MWCOORD srcy;
	int clipped;
	int rx1, rx2, ry1, ry2, rw, rh;
	int count;
#if DYNAMICREGIONS
	MWRECT *prc;
	extern MWCLIPREGION *clipregion;
#else
	MWCLIPRECT *prc;
	extern MWCLIPRECT cliprects[];
	extern int clipcount;
#endif

	if (!psd->DrawArea)
		return;

	/* Set up area clipping, and just return if nothing is visible */
	clipped = GdClipArea(psd, x, y, x + width - 1, y + height - 1);
	if (clipped == CLIP_INVISIBLE) {
		return;
	} else if (clipped == CLIP_VISIBLE) {
		psd->DrawArea(psd, gc, op);
		return;
	}
	/* Partially clipped. */

	/* Save srcX/Y so we can change the originals. */
	srcx = gc->srcx;
	srcy = gc->srcy;

#if DYNAMICREGIONS
	prc = clipregion->rects;
	count = clipregion->numRects;
#else
	prc = cliprects;
	count = clipcount;
#endif

	while (count-- > 0) {
#if DYNAMICREGIONS
		rx1 = prc->left;
		ry1 = prc->top;
		rx2 = prc->right;
		ry2 = prc->bottom;
#else
		/* New clip-code by Morten */
		rx1 = prc->x;
		ry1 = prc->y;
		rx2 = prc->x + prc->width;
		ry2 = prc->y + prc->height;
#endif

		/* Check if this rect intersects with the one we draw */
		if (rx1 < x)
			rx1 = x;
		if (ry1 < y)
			ry1 = y;
		if (rx2 > x + width)
			rx2 = x + width;
		if (ry2 > y + height)
			ry2 = y + height;

		rw = rx2 - rx1;
		rh = ry2 - ry1;

		if (rw > 0 && rh > 0) {
			gc->dstx = rx1;
			gc->dsty = ry1;
			gc->dstw = rw;
			gc->dsth = rh;
			gc->srcx = srcx + rx1 - x;
			gc->srcy = srcy + ry1 - y;
			GdCheckCursor(psd, rx1, ry1, rx2 - 1, ry2 - 1);
			psd->DrawArea(psd, gc, op);
		}
		prc++;
	}

	/* Reset everything, in case the caller re-uses it. */
	gc->dstx = x;
	gc->dsty = y;
	gc->dstw = width;
	gc->dsth = height;
	gc->srcx = srcx;
	gc->srcy = srcy;
	return;
}

/**
 * Draw a rectangle of color values, clipping if necessary.
 * If a color matches the background color,
 * then that pixel is only drawn if the gr_usebg flag is set.
 *
 * The pixels are packed according to pixtype:
 *
 * pixtype		array of
 * MWPF_RGB		MWCOLORVAL (unsigned long)
 * MWPF_PIXELVAL	MWPIXELVAL (compile-time dependent)
 * MWPF_PALETTE		unsigned char
 * MWPF_TRUECOLOR8888	unsigned long
 * MWPF_TRUECOLOR0888	unsigned long
 * MWPF_TRUECOLOR888	packed struct {char r,char g,char b} (24 bits)
 * MWPF_TRUECOLOR565	unsigned short
 * MWPF_TRUECOLOR555	unsigned short
 * MWPF_TRUECOLOR332	unsigned char
 *
 * NOTE: Currently, no translation is performed if the pixtype
 * is not MWPF_RGB.  Pixtype is only then used to determine the
 * packed size of the pixel data, and is then stored unmodified
 * in a MWPIXELVAL and passed to the screen driver.  Virtually,
 * this means there's only three reasonable options for client
 * programs: (1) pass all data as RGB MWCOLORVALs, (2) pass
 * data as unpacked 32-bit MWPIXELVALs in the format the current
 * screen driver is running, or (3) pass data as packed values
 * in the format the screen driver is running.  Options 2 and 3
 * are identical except for the packing structure.
 *
 * @param psd Drawing surface.
 * @param x Left edge of rectangle to blit to.
 * @param y Top edge of rectangle to blit to.
 * @param width Width of image to blit.
 * @param height Height of image to blit.
 * @param pixels Image data.
 * @param pixtype Format of pixels.
 */
void
GdArea(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height, void *pixels,
	int pixtype)
{
	unsigned char *PIXELS = pixels;	/* for ANSI compilers, can't use void*/
	long cellstodo;			/* remaining number of cells */
	long count;			/* number of cells of same color */
	long cc;			/* current cell count */
	long rows;			/* number of complete rows */
	MWCOORD minx;			/* minimum x value */
	MWCOORD maxx;			/* maximum x value */
	MWPIXELVAL savecolor;		/* saved foreground color */
	MWBOOL dodraw;			/* TRUE if draw these points */
	MWCOLORVAL rgbcolor = 0L;
	int pixsize;
	unsigned char r, g, b;

	minx = x;
	maxx = x + width - 1;

	/* Set up area clipping, and just return if nothing is visible */
	if ( GdClipArea(psd, minx, y, maxx, y + height - 1) == CLIP_INVISIBLE )
		return;

/* psd->DrawArea driver call temp removed, hasn't been tested with new drawarea routines*/
#if 0000
	if (pixtype == MWPF_PIXELVAL) {
		driver_gc_t hwgc;

		if (!(psd->flags & PSF_HAVEOP_COPY))
			goto fallback;

		hwgc.pixels = PIXELS;
		hwgc.src_linelen = width;
		hwgc.gr_usebg = gr_usebg;
		hwgc.bg_color = gr_background;
		hwgc.dstx = x;
		hwgc.dsty = y;
		hwgc.dstw = width;
		hwgc.dsth = height;
		hwgc.srcx = 0;
		hwgc.srcy = 0;
		GdDrawAreaInternal(psd, &hwgc, PSDOP_COPY);
		GdFixCursor(psd);
		return;
	      fallback:
	}
	GdFixCursor(psd);
	return;
 fallback:
#endif /* if 0000 temp removed*/

	/* Calculate size of packed pixels*/
	switch(pixtype) {
	case MWPF_RGB:
		pixsize = sizeof(MWCOLORVAL);
		break;
	case MWPF_PIXELVAL:
		pixsize = sizeof(MWPIXELVAL);
		break;
	case MWPF_PALETTE:
	case MWPF_TRUECOLOR332:
		pixsize = sizeof(unsigned char);
		break;
	case MWPF_TRUECOLOR8888:
	case MWPF_TRUECOLOR0888:
		pixsize = sizeof(unsigned long);
		break;
	case MWPF_TRUECOLOR888:
		pixsize = 3;
		break;
	case MWPF_TRUECOLOR565:
	case MWPF_TRUECOLOR555:
		pixsize = sizeof(unsigned short);
		break;
	default:
		return;
	}

  savecolor = gr_foreground;
  cellstodo = (long)width * height;
  while (cellstodo > 0) {
	/* read the pixel value from the pixtype*/
	switch(pixtype) {
	case MWPF_RGB:
		rgbcolor = *(MWCOLORVAL *)PIXELS;
		PIXELS += sizeof(MWCOLORVAL);
		gr_foreground = GdFindColor(psd, rgbcolor);
		break;
	case MWPF_PIXELVAL:
		gr_foreground = *(MWPIXELVAL *)PIXELS;
		PIXELS += sizeof(MWPIXELVAL);
		break;
	case MWPF_PALETTE:
	case MWPF_TRUECOLOR332:
		gr_foreground = *PIXELS++;
		break;
	case MWPF_TRUECOLOR8888:
	case MWPF_TRUECOLOR0888:
		gr_foreground = *(unsigned long *)PIXELS;
		PIXELS += sizeof(unsigned long);
		break;
	case MWPF_TRUECOLOR888:
		r = *PIXELS++;
		g = *PIXELS++;
		b = *PIXELS++;
		gr_foreground = RGB2PIXEL888(r, g, b);
		break;
	case MWPF_TRUECOLOR565:
	case MWPF_TRUECOLOR555:
		gr_foreground = *(unsigned short *)PIXELS;
		PIXELS += sizeof(unsigned short);
		break;
	}
	dodraw = (gr_usebg || (gr_foreground != gr_background));
	count = 1;
	--cellstodo;

	/* See how many of the adjacent remaining points have the
	 * same color as the next point.
	 *
	 * NOTE: Yes, with the addition of the pixel unpacking,
	 * it's almost slower to look ahead than to just draw
	 * the pixel...  FIXME
	 */
	while (cellstodo > 0) {
		switch(pixtype) {
		case MWPF_RGB:
			if(rgbcolor != *(MWCOLORVAL *)PIXELS)
				goto breakwhile;
			PIXELS += sizeof(MWCOLORVAL);
			break;
		case MWPF_PIXELVAL:
			if(gr_foreground != *(MWPIXELVAL *)PIXELS)
				goto breakwhile;
			PIXELS += sizeof(MWPIXELVAL);
			break;
		case MWPF_PALETTE:
		case MWPF_TRUECOLOR332:
			if(gr_foreground != *(unsigned char *)PIXELS)
				goto breakwhile;
			++PIXELS;
			break;
		case MWPF_TRUECOLOR8888:
		case MWPF_TRUECOLOR0888:
			if(gr_foreground != *(unsigned long *)PIXELS)
				goto breakwhile;
			PIXELS += sizeof(unsigned long);
			break;
		case MWPF_TRUECOLOR888:
			r = *(unsigned char *)PIXELS;
			g = *(unsigned char *)(PIXELS + 1);
			b = *(unsigned char *)(PIXELS + 2);
			if(gr_foreground != RGB2PIXEL888(r, g, b))
				goto breakwhile;
			PIXELS += 3;
			break;
		case MWPF_TRUECOLOR565:
		case MWPF_TRUECOLOR555:
			if(gr_foreground != *(unsigned short *)PIXELS)
				goto breakwhile;
			PIXELS += sizeof(unsigned short);
			break;
		}
		++count;
		--cellstodo;
	}
breakwhile:

	/* If there is only one point with this color, then draw it
	 * by itself.
	 */
	if (count == 1) {
		if (dodraw)
			drawpoint(psd, x, y);
		if (++x > maxx) {
			x = minx;
			y++;
		}
		continue;
	}

	/* There are multiple points with the same color. If we are
	 * not at the start of a row of the rectangle, then draw this
	 * first row specially.
	 */
	if (x != minx) {
		cc = count;
		if (x + cc - 1 > maxx)
			cc = maxx - x + 1;
		if (dodraw)
			drawrow(psd, x, x + cc - 1, y);
		count -= cc;
		x += cc;
		if (x > maxx) {
			x = minx;
			y++;
		}
	}

	/* Now the x value is at the beginning of a row if there are
	 * any points left to be drawn.  Draw all the complete rows
	 * with one call.
	 */
	rows = count / width;
	if (rows > 0) {
		if (dodraw) {
			/* note: change to fillrect, (parm types changed)*/
			/*GdFillRect(psd, x, y, maxx, y + rows - 1);*/
			GdFillRect(psd, x, y, maxx - x + 1, rows);
		}
		count %= width;
		y += rows;
	}

	/* If there is a final partial row of pixels left to be
	 * drawn, then do that.
	 */
	if (count > 0) {
		if (dodraw)
			drawrow(psd, x, x + count - 1, y);
		x += count;
	}
  }
  gr_foreground = savecolor;
  GdFixCursor(psd);
}

#if NOTYET
/* Copy a rectangular area from one screen area to another.
 * This bypasses clipping.
 */
void
GdCopyArea(PSD psd, MWCOORD srcx, MWCOORD srcy, MWCOORD width, MWCOORD height,
	MWCOORD destx, MWCOORD desty)
{
	if (width <= 0 || height <= 0)
		return;

	if (srcx == destx && srcy == desty)
		return;
	GdCheckCursor(psd, srcx, srcy, srcx + width - 1, srcy + height - 1);
	GdCheckCursor(psd, destx, desty, destx + width - 1, desty + height - 1);
	psd->CopyArea(psd, srcx, srcy, width, height, destx, desty);
	GdFixCursor(psd);
}
#endif

extern MWCLIPREGION *clipregion;

/**
 * Copy source rectangle of pixels to destination rectangle quickly
 *
 * @param dstpsd Drawing surface to draw to.
 * @param dstx Destination X co-ordinate.
 * @param dsty Destination Y co-ordinate.
 * @param width Width of rectangle to copy.
 * @param height Height of rectangle to copy.
 * @param srcpsd Drawing surface to copy from.
 * @param srcx Source X co-ordinate.
 * @param srcy Source Y co-ordinate.
 * @param rop Raster operation.
 */
void
GdBlit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD width, MWCOORD height,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long rop)
{
	int rx1, rx2, ry1, ry2;
	int px1, px2, py1, py2;
	int pw, ph;
	int count;
#if DYNAMICREGIONS
	MWRECT *	prc;
#else
	MWCLIPRECT *	prc;
	extern MWCLIPRECT cliprects[];
	extern int clipcount;
#endif

	/*FIXME: compare bpp's and convert if necessary*/
	assert(dstpsd->planes == srcpsd->planes);
	assert(dstpsd->bpp == srcpsd->bpp);

	/* temporary assert() until rotation blits completed*/
	assert(dstpsd->portrait == srcpsd->portrait);
	
	/* clip blit rectangle to source screen/bitmap size*/
	/* we must do this because there isn't any source clipping setup*/
	if(srcx < 0) {
		width += srcx;
		dstx -= srcx;
		srcx = 0;
	}
	if(srcy < 0) {
		height += srcy;
		dsty -= srcy;
		srcy = 0;
	}
	if(srcx+width > srcpsd->xvirtres)
		width = srcpsd->xvirtres - srcx;
	if(srcy+height > srcpsd->yvirtres)
		height = srcpsd->yvirtres - srcy;

	switch(GdClipArea(dstpsd, dstx, dsty, dstx+width-1, dsty+height-1)) {
	case CLIP_VISIBLE:
		/* check cursor in src region of both screen devices*/
		GdCheckCursor(dstpsd, srcx, srcy, srcx+width-1, srcy+height-1);
		if (dstpsd != srcpsd)
			GdCheckCursor(srcpsd, srcx, srcy, srcx+width-1,
				srcy+height-1);
		dstpsd->Blit(dstpsd, dstx, dsty, width, height,
			srcpsd, srcx, srcy, rop);
		GdFixCursor(dstpsd);
		if (dstpsd != srcpsd)
			GdFixCursor(srcpsd);
		return;

	case CLIP_INVISIBLE:
		return;
	}

	/* Partly clipped, we'll blit using destination clip
	 * rectangles, and offset the blit accordingly.
	 * Since the destination is already clipped, we
	 * only need to clip the source here.
	 */
#if DYNAMICREGIONS
	prc = clipregion->rects;
	count = clipregion->numRects;
#else
	prc = cliprects;
	count = clipcount;
#endif
	while(--count >= 0) {
#if DYNAMICREGIONS
		rx1 = prc->left;
		ry1 = prc->top;
		rx2 = prc->right;
		ry2 = prc->bottom;
#else
		rx1 = prc->x;
		ry1 = prc->y;
		rx2 = prc->x + prc->width;
		ry2 = prc->y + prc->height;
#endif
		/* Check:  does this rect intersect the one we want to draw? */
		px1 = dstx;
		py1 = dsty;
		px2 = dstx + width;
		py2 = dsty + height;
		if (px1 < rx1) px1 = rx1;
		if (py1 < ry1) py1 = ry1;
		if (px2 > rx2) px2 = rx2;
		if (py2 > ry2) py2 = ry2;

		pw = px2 - px1;
		ph = py2 - py1;
		if(pw > 0 && ph > 0) {
			/* check cursor in dest and src regions*/
			GdCheckCursor(dstpsd, px1, py1, px2-1, py2-1);
			GdCheckCursor(dstpsd, srcx, srcy,
				srcx+width, srcy+height);
			dstpsd->Blit(dstpsd, px1, py1, pw, ph, srcpsd,
				srcx + (px1-dstx), srcy + (py1-dsty), rop);
		}
		++prc;
	}
	GdFixCursor(dstpsd);
}

/* experimental globals for ratio bug when src != 0*/
/* Only used by fblin16.c */
int g_row_inc, g_col_inc;

/**
 * Stretch source rectangle of pixels to destination rectangle quickly
 *
 * @param dstpsd Drawing surface to draw to.
 * @param dstx Destination X co-ordinate.
 * @param dsty Destination Y co-ordinate.
 * @param dstw Width of destination rectangle.
 * @param dsth Height of destination rectangle.
 * @param srcpsd Drawing surface to copy from.
 * @param srcx Source X co-ordinate.
 * @param srcy Source Y co-ordinate.
 * @param srcw Width of source rectangle.
 * @param srch Height of source rectangle.
 * @param rop Raster operation.
 */
void
GdStretchBlit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, long rop)
{
	int count;
#if DYNAMICREGIONS
	MWRECT *	prc;
	extern MWCLIPREGION *clipregion;
#else
	MWCLIPRECT *	prc;
	extern MWCLIPRECT cliprects[];
	extern int clipcount;
#endif

#if 1 /* FIXME*/
	/* Use new improved stretchblit if the driver supports it */
	if (dstpsd->StretchBlitEx) {
		GdStretchBlitEx(dstpsd, dstx, dsty,
				dstx + dstw, dsty + dsth,
				srcpsd, srcx, srcy,
				srcx + srcw, srcy + srch,
				rop);
		return;
	}
#endif

g_row_inc = g_col_inc = 0;

	/* check for driver stretch blit implementation*/
	if (!dstpsd->StretchBlit)
		return;

	/*FIXME: compare bpp's and convert if necessary*/
	assert(dstpsd->planes == srcpsd->planes);
	assert(dstpsd->bpp == srcpsd->bpp);
	
	/* clip blit rectangle to source screen/bitmap size*/
	/* we must do this because there isn't any source clipping setup*/
	if(srcx < 0) {
		srcw += srcx;
		/*dstx -= srcx;*/
		srcx = 0;
	}
	if(srcy < 0) {
		srch += srcy;
		/*dsty -= srcy;*/
		srcy = 0;
	}
	if(srcx+srcw > srcpsd->xvirtres)
		srcw = srcpsd->xvirtres - srcx;
	if(srcy+srch > srcpsd->yvirtres)
		srch = srcpsd->yvirtres - srcy;

	/* temp dest clipping for partially visible case*/
	if(dstx+dstw > dstpsd->xvirtres)
		dstw = dstpsd->xvirtres - dstx;
	if(dsty+dsth > dstpsd->yvirtres)
		dsth = dstpsd->yvirtres - dsty;

	switch(GdClipArea(dstpsd, dstx, dsty, dstx+dstw-1, dsty+dsth-1)) {
	case CLIP_VISIBLE:
		/* check cursor in src region*/
		GdCheckCursor(dstpsd, srcx, srcy, srcx+srcw-1, srcy+srch-1);
		dstpsd->StretchBlit(dstpsd, dstx, dsty, dstw, dsth,
			srcpsd, srcx, srcy, srcw, srch, rop);
		GdFixCursor(dstpsd);
		return;

	case CLIP_INVISIBLE:
		return;
	}

	/* Partly clipped, we'll blit using destination clip
	 * rectangles, and offset the blit accordingly.
	 * Since the destination is already clipped, we
	 * only need to clip the source here.
	 */
#if DYNAMICREGIONS
	prc = clipregion->rects;
	count = clipregion->numRects;
#else
	prc = cliprects;
	count = clipcount;
#endif
	while(--count >= 0) {
		int rx1, rx2, ry1, ry2;
		int px1, px2, py1, py2;
		int pw, ph;
		int sx, sy, sw, sh;
#if DYNAMICREGIONS
		rx1 = prc->left;
		ry1 = prc->top;
		rx2 = prc->right;
		ry2 = prc->bottom;
#else
		rx1 = prc->x;
		ry1 = prc->y;
		rx2 = prc->x + prc->width;
		ry2 = prc->y + prc->height;
#endif
		/* Check:  does this rect intersect the one we want to draw? */
		px1 = dstx;
		py1 = dsty;
		px2 = dstx + dstw;
		py2 = dsty + dsth;
		if (px1 < rx1) px1 = rx1;
		if (py1 < ry1) py1 = ry1;
		if (px2 > rx2) px2 = rx2;
		if (py2 > ry2) py2 = ry2;

		pw = px2 - px1;
		ph = py2 - py1;
		if(pw > 0 && ph > 0) {
			/* calc proper src/dst offset for stretch rect*/
g_row_inc = (srch << 16) / dsth;
g_col_inc = (srcw << 16) / dstw;
			sw = pw * srcw / dstw;
			sh = ph * srch / dsth;

			if (sw > 0 && sh > 0) {
				sx = srcx + (px1-dstx) * srcw / dstw;
				sy = srcy + (py1-dsty) * srch / dsth;
/*printf("P %d,%d,%d,%d   %d,%d\n", sx, sy, sw, sh, g_row_inc, g_col_inc);*/

				/* check cursor in dest and src regions*/
				GdCheckCursor(dstpsd, px1, py1, px2-1, py2-1);
				GdCheckCursor(dstpsd, srcx, srcy, srcx+srcw, srcy+srch);
				dstpsd->StretchBlit(dstpsd, px1, py1, pw, ph, srcpsd,
					sx, sy, sw, sh, rop);
			}
		}
		++prc;
	}
	GdFixCursor(dstpsd);
}

/**
 * A proper stretch blit.  Supports flipping the image.
 * Paramaters are co-ordinates of two points in the source, and
 * two corresponding points in the destination.  The image is scaled
 * and flipped as needed to make the two points correspond.  The
 * top-left corner is drawn, the bottom right one isn't [i.e.
 * (0,0)-(2,2) specifies a 2x2 rectangle consisting of the points
 * at (0,0), (0,1), (1,0), (1,1).  It does not include the points
 * where x=2 or y=2.]
 *
 * Raster ops are not yet fully implemented - see the low-level
 * drivers for details.
 *
 * Note that we do not support overlapping blits.
 *
 * @param dstpsd Drawing surface to draw to.
 * @param d1_x Destination X co-ordinate of first corner.
 * @param d1_y Destination Y co-ordinate of first corner.
 * @param d2_x Destination X co-ordinate of second corner.
 * @param d2_y Destination Y co-ordinate of second corner.
 * @param srcpsd Drawing surface to copy from.
 * @param s1_x Source X co-ordinate of first corner.
 * @param s1_y Source Y co-ordinate of first corner.
 * @param s2_x Source X co-ordinate of second corner.
 * @param s2_y Source Y co-ordinate of second corner.
 * @param rop Raster operation.
 */
void
GdStretchBlitEx(PSD dstpsd, MWCOORD d1_x, MWCOORD d1_y, MWCOORD d2_x,
	MWCOORD d2_y, PSD srcpsd, MWCOORD s1_x, MWCOORD s1_y, MWCOORD s2_x,
	MWCOORD s2_y, long rop)
{
	/* Scale factors (as fractions, numerator/denominator) */
	int src_x_step_numerator;
	int src_x_step_denominator;
	int src_y_step_numerator;
	int src_y_step_denominator;

	/* Clipped dest co-ords */
	MWCOORD c1_x;
	MWCOORD c1_y;
	MWCOORD c2_x;
	MWCOORD c2_y;

	/* Initial source co-ordinates, as a fraction (denominators as above) */
	int src_x_start_exact;
	int src_y_start_exact;

	/* Used by the clipping code. */
#if DYNAMICREGIONS
	int count;
	MWRECT *prc;

	extern MWCLIPREGION *clipregion;
#else
	int count;
	MWCLIPRECT *prc;

	extern MWCLIPRECT cliprects[];
	extern int clipcount;
#endif

	assert(srcpsd);
	assert(dstpsd);

	/* DPRINTF("Nano-X: GdStretchBlitEx(dst=%x (%d,%d)-(%d,%d), src=%x (%d,%d)-(%d,%d), op=0x%lx\n",
	           (int) dstpsd, (int) d1_x, (int) d1_y, (int) d2_x, (int) d2_y,
	           (int) srcpsd, (int) s1_x, (int) s1_y, (int) s2_x, (int) s2_y, rop); */

	/* Sort co-ordinates so d1 is top left, d2 is bottom right. */
	if (d1_x > d2_x) {
		register MWCOORD tmp;
		tmp = d2_x;
		d2_x = d1_x;
		d1_x = tmp;
		tmp = s2_x;
		s2_x = s1_x;
		s1_x = tmp;
	}

	if (d1_y > d2_y) {
		register MWCOORD tmp;
		tmp = d2_y;
		d2_y = d1_y;
		d1_y = tmp;
		tmp = s2_y;
		s2_y = s1_y;
		s1_y = tmp;
	}

	if ((d2_x < 0)
	    || (d2_y < 0)
	    || (d1_x > dstpsd->xvirtres)
	    || (d1_y > dstpsd->yvirtres)
	    || (d1_x == d2_x)
	    || (d1_y == d2_y)) {
		/* Destination rectangle is entirely off screen,
		 * or is zero-sized - bail ASAP
		 */
		/* DPRINTF("Nano-X: GdStretchBlitEx: CLIPPED OFF (dest rect offscreen or 0)\n"); */
		return;
	}

	/* If we're not stretching or flipping, use the standard blit
	 * (faster).
	 */
	if ((d2_x - d1_x == s2_x - s1_x) && (d2_y - d1_y == s2_y - s1_y)) {
		GdBlit(dstpsd, d1_x, d1_y, d2_x - d1_x, d2_y - d1_y,
		       srcpsd, s1_x, s1_y, rop);
		return;
	}

	if (!dstpsd->StretchBlitEx) {
		EPRINTF("GdStretchBlitEx NOT SUPPORTED on this target\n");
		return;
	}

	/* Need to preserve original values, so make a copy we can clip. */
	c1_x = d1_x;
	c1_y = d1_y;
	c2_x = d2_x;
	c2_y = d2_y;

	/* Calculate how far in source co-ordinates is
	 * equivalent to one pixel in dest co-ordinates.
	 * This is stored as a fraction (numerator/denominator).
	 * The numerator may be > denominator.  The numerator
	 * may be negative, the denominator is always positive.
	 *
	 * We need half this distance for some purposes,
	 * hence the *2.
	 *
	 * The +1s are because we care about *sizes*, not
	 * deltas.  (Without the +1s it just doesn't
	 * work properly.)
	 */
	src_x_step_numerator = (s2_x - s1_x + 1) << 1;
	src_x_step_denominator = (d2_x - d1_x + 1) << 1;
	src_y_step_numerator = (s2_y - s1_y + 1) << 1;
	src_y_step_denominator = (d2_y - d1_y + 1) << 1;

	/* Clip the image so that the destination X co-ordinates
	 * in c1_x and c2_x map to a point on the source image.
	 */
	if ((s1_x < 0) || (s1_x > srcpsd->xvirtres)
	    || (s2_x < 0) || (s2_x > srcpsd->xvirtres)) {
		/* Calculate where the left of the source image will end up,
		 * in dest co-ordinates.
		 */
		int i1_x = d1_x - (s1_x * src_x_step_denominator) / src_x_step_numerator;

		/* Calculate where the right of the source image will end up,
		 * in dest co-ordinates.
		 */
		int i2_x = d1_x + ((srcpsd->xvirtres - s1_x) * src_x_step_denominator + src_x_step_denominator - 1) / src_x_step_numerator;

		/* Since we may be doing a flip, "left" and "right" in the statements
		 * above do not necessarily correspond to "left" and "right" in the
		 * destination image, which is where we're clipping.  So sort the
		 * X co-ordinates.
		 */
		if (i1_x > i2_x) {
			register int temp;
			temp = i1_x;
			i1_x = i2_x;
			i2_x = temp;
		}

		/* Check for total invisibility */
		if ((c2_x < i1_x) || (c1_x > i2_x)) {
			/* DPRINTF("Nano-X: GdStretchBlitEx: CLIPPED OFF (source X checks)\n"); */
			return;
		}

		/* Perform partial clip */
		if (c1_x < i1_x)
			c1_x = i1_x;
		if (c2_x > i2_x)
			c2_x = i2_x;
	}

	/* Clip the image so that the destination Y co-ordinates
	 * in c1_y and c2_y map to a point on the source image.
	 */
	if ((s1_y < 0) || (s1_y > srcpsd->yvirtres)
	    || (s2_y < 0) || (s2_y > srcpsd->yvirtres)) {
		/* Calculate where the top of the source image will end up,
		 * in dest co-ordinates.
		 */
		int i1_y = d1_y - (s1_y * src_y_step_denominator) / src_y_step_numerator;

		/* Calculate where the bottom of the source image will end up,
		 * in dest co-ordinates.
		 */
		int i2_y = d1_y + ((srcpsd->yvirtres - s1_y) * src_y_step_denominator + src_y_step_denominator - 1) / src_y_step_numerator;

		/* Since we may be doing a flip, "top" and bottom" in the statements
		 * above do not necessarily correspond to "top" and bottom" in the
		 * destination image, which is where we're clipping.  So sort the
		 * Y co-ordinates.
		 */
		if (i1_y > i2_y) {
			register int temp;
			temp = i1_y;
			i1_y = i2_y;
			i2_y = temp;
		}

		/* Check for total invisibility */
		if ((c2_y < i1_y) || (c1_y > i2_y)) {
			/* DPRINTF("Nano-X: GdStretchBlitEx: CLIPPED OFF (source Y checks)\n"); */
			return;
		}

		/* Perform partial clip */
		if (c1_y < i1_y)
			c1_y = i1_y;
		if (c2_y > i2_y)
			c2_y = i2_y;
	}

	/* Clip against dest window (NOT dest clipping region). */
	if (c1_x < 0)
		c1_x = 0;
	if (c1_y < 0)
		c1_y = 0;
	if (c2_x > dstpsd->xvirtres)
		c2_x = dstpsd->xvirtres;
	if (c2_y > dstpsd->yvirtres)
		c2_y = dstpsd->yvirtres;

	/* Final fully-offscreen check */
	if ((c1_x >= c2_x)
	    || (c1_y >= c2_y)) {
		/* DPRINTF("Nano-X: GdStretchBlitEx: CLIPPED OFF (final check)\n"); */
		return;
	}

	/* Well, if we survived that lot, then we now have a destination
	 * rectangle defined in (c1_x,c1_y)-(c2_x,c2_y).
	 */

	/* DPRINTF("Nano-X: GdStretchBlitEx: Clipped rect: (%d,%d)-(%d,%d)\n",
	       (int) c1_x, (int) c1_y, (int) c2_x, (int) c2_y); */

	/* Calculate the position in the source rectange that is equivalent
	 * to the top-left of the destination rectangle.
	 */
	src_x_start_exact = s1_x * src_x_step_denominator
		+ (c1_x - d1_x) * src_x_step_numerator;
	src_y_start_exact = s1_y * src_y_step_denominator
		+ (c1_y - d1_y) * src_y_step_numerator;

	/* OK, clipping so far has been against physical bounds, we now have
	 * to worry about user defined clip regions.
	 */
	switch (GdClipArea(dstpsd, c1_x, c1_y, c2_x - 1, c2_y - 1)) {
	case CLIP_INVISIBLE:
		/* DPRINTF("Nano-X: GdStretchBlitEx: CLIPPED OFF (GdClipArea check)\n"); */
		return;
	case CLIP_VISIBLE:
		/* FIXME: check cursor in src region */
		/* GdCheckCursor(srcpsd, c1_x, c1_y, c2_x-1, c2_y-1); */
		/* DPRINTF("Nano-X: GdStretchBlitEx: no more clipping needed\n"); */
		dstpsd->StretchBlitEx(dstpsd,
					srcpsd,
					c1_x,
					c1_y,
					c2_x - c1_x,
					c2_y - c1_y,
					src_x_step_denominator,
					src_y_step_denominator,
					src_x_start_exact,
					src_y_start_exact,
					src_x_step_numerator,
					src_y_step_numerator, rop);
		/* GdFixCursor(srcpsd); */
		GdFixCursor(dstpsd);
		return;

	}
	/* DPRINTF("Nano-X: GdStretchBlitEx: complex clipping needed\n"); */

	/* FIXME: check cursor in src region */
	/* GdCheckCursor(srcpsd, c1_x, c1_y, c2_x-1, c2_y-1); */


	/* Partly clipped, we'll blit using destination clip
	 * rectangles, and offset the blit accordingly.
	 * Since the destination is already clipped, we
	 * only need to clip the source here.
	 */
#if DYNAMICREGIONS
	prc = clipregion->rects;
	count = clipregion->numRects;
#else
	prc = cliprects;
	count = clipcount;
#endif
	while (--count >= 0) {
		int r1_x, r2_x, r1_y, r2_y;

#if DYNAMICREGIONS
		r1_x = prc->left;
		r1_y = prc->top;
		r2_x = prc->right;
		r2_y = prc->bottom;
#else
		r1_x = prc->x;
		r1_y = prc->y;
		r2_x = prc->x + prc->width;
		r2_y = prc->y + prc->height;
#endif

		/* Check:  does this rect intersect the one we want to draw? */
		/* Clip r1-r2 so it's inside c1-c2 */
		if (r1_x < c1_x)
			r1_x = c1_x;
		if (r1_y < c1_y)
			r1_y = c1_y;
		if (r2_x > c2_x)
			r2_x = c2_x;
		if (r2_y > c2_y)
			r2_y = c2_y;

		if ((r1_x < r2_x) && (r1_y < r2_y)) {
			/* So we're drawing to:
			 * destination rectangle (r1_x, r1_y) - (r2_x, r2_y)
			 * source start co-ords:
			 * x = src_x_start_exact + (r1_x - c1_x)*src_x_step_numerator
			 * y = src_y_start_exact + (r1_y - c1_y)*src_y_step_numerator
			 */

			/* check cursor in dest region */
			GdCheckCursor(dstpsd, r1_x, r1_y, r2_x - 1,
					  r2_y - 1);
			dstpsd->StretchBlitEx(dstpsd,
						srcpsd,
						r1_x,
						r1_y,
						r2_x - r1_x,
						r2_y - r1_y,
						src_x_step_denominator,
						src_y_step_denominator,
						src_x_start_exact + (r1_x - c1_x) * src_x_step_numerator,
						src_y_start_exact + (r1_y - c1_y) * src_y_step_numerator,
						src_x_step_numerator,
						src_y_step_numerator,
						rop);
		}
		++prc;
	}
	GdFixCursor(dstpsd);
	/* GdFixCursor(srcpsd); */
}

/*
 * Calculate size and linelen of memory gc.
 * If bpp or planes is 0, use passed psd's bpp/planes.
 * Note: linelen is calculated to be DWORD aligned for speed
 * for bpp <= 8.  Linelen is converted to bytelen for bpp > 8.
 */
int
GdCalcMemGCAlloc(PSD psd, unsigned int width, unsigned int height, int planes,
	int bpp, int *psize, int *plinelen)
{
	int	bytelen, linelen, tmp;

	if(!planes)
		planes = psd->planes;
	if(!bpp)
		bpp = psd->bpp;
	/* 
	 * swap width and height in left/right portrait modes,
	 * so imagesize is calculated properly
	 */
	if(psd->portrait & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT)) {
		tmp = width;
		width = height;
		height = tmp;
	}

	/*
	 * use bpp and planes to create size and linelen.
	 * linelen is in bytes for bpp 1, 2, 4, 8, and pixels for bpp 16,24,32.
	 */
	if(planes == 1) {
		switch(bpp) {
		case 1:
			linelen = (width+7)/8;
			bytelen = linelen = (linelen+3) & ~3;
			break;
		case 2:
			linelen = (width+3)/4;
			bytelen = linelen = (linelen+3) & ~3;
			break;
		case 4:
			linelen = (width+1)/2;
			bytelen = linelen = (linelen+3) & ~3;
			break;
		case 8:
			bytelen = linelen = (width+3) & ~3;
			break;
		case 16:
			linelen = width;
			bytelen = width * 2;
			break;
		case 24:
			linelen = width;
			bytelen = width * 3;
			break;
		case 32:
			linelen = width;
			bytelen = width * 4;
			break;
		default:
			return 0;
		}
	} else if(planes == 4) {
		/* FIXME assumes VGA 4 planes 4bpp*/
		/* we use 4bpp linear for memdc format*/
		linelen = (width+1)/2;
		linelen = (linelen+3) & ~3;
		bytelen = linelen;
	} else {
		*psize = *plinelen = 0;
		return 0;
	}

	*plinelen = linelen;
	*psize = bytelen * height;
	return 1;
}

/**
 * Translate a rectangle of color values
 *
 * The pixels are packed according to inpixtype/outpixtype:
 *
 * pixtype		array of
 * MWPF_RGB		MWCOLORVAL (unsigned long)
 * MWPF_PIXELVAL	MWPIXELVAL (compile-time dependent)
 * MWPF_PALETTE		unsigned char
 * MWPF_TRUECOLOR8888	unsigned long
 * MWPF_TRUECOLOR0888	unsigned long
 * MWPF_TRUECOLOR888	packed struct {char r,char g,char b} (24 bits)
 * MWPF_TRUECOLOR565	unsigned short
 * MWPF_TRUECOLOR555	unsigned short
 * MWPF_TRUECOLOR332	unsigned char
 *
 * @param width Width of rectangle to translate.
 * @param height Height of rectangle to translate.
 * @param in Source pixels buffer.
 * @param inpixtype Source pixel type.
 * @param inpitch Source pitch.
 * @param out Destination pixels buffer.
 * @param outpixtype Destination pixel type.
 * @param outpitch Destination pitch.
 */
void
GdTranslateArea(MWCOORD width, MWCOORD height, void *in, int inpixtype,
	MWCOORD inpitch, void *out, int outpixtype, int outpitch)
{
	unsigned char *	inbuf = in;
	unsigned char *	outbuf = out;
	unsigned long	pixelval;
	MWCOLORVAL	colorval;
	MWCOORD		x, y;
	unsigned char 	r, g, b;
	extern MWPALENTRY gr_palette[256];
	int	  gr_palsize = 256;	/* FIXME*/

	for(y=0; y<height; ++y) {
	    for(x=0; x<width; ++x) {
		/* read pixel value and convert to BGR colorval (0x00BBGGRR)*/
		switch (inpixtype) {
		case MWPF_RGB:
			colorval = *(MWCOLORVAL *)inbuf;
			inbuf += sizeof(MWCOLORVAL);
			break;
		case MWPF_PIXELVAL:
			pixelval = *(MWPIXELVAL *)inbuf;
			inbuf += sizeof(MWPIXELVAL);
			/* convert based on compile-time MWPIXEL_FORMAT*/
#if MWPIXEL_FORMAT == MWPF_PALETTE
			colorval = GETPALENTRY(gr_palette, pixelval);
#else
			colorval = PIXELVALTOCOLORVAL(pixelval);
#endif
			break;
		case MWPF_PALETTE:
			pixelval = *inbuf++;
			colorval = GETPALENTRY(gr_palette, pixelval);
			break;
		case MWPF_TRUECOLOR332:
			pixelval = *inbuf++;
			colorval = PIXEL332TOCOLORVAL(pixelval);
			break;
		case MWPF_TRUECOLOR0888:
			pixelval = *(unsigned long *)inbuf;
			colorval = PIXEL888TOCOLORVAL(pixelval);
			inbuf += sizeof(unsigned long);
			break;
		case MWPF_TRUECOLOR8888:
			pixelval = *(unsigned long *) inbuf;
			colorval = PIXEL8888TOCOLORVAL(pixelval);
			inbuf += sizeof(unsigned long);
			break;
		case MWPF_TRUECOLOR888:
			r = *inbuf++;
			g = *inbuf++;
			b = *inbuf++;
			colorval = (MWPIXELVAL)MWRGB(r, g, b);
			break;
		case MWPF_TRUECOLOR565:
			pixelval = *(unsigned short *)inbuf;
			colorval = PIXEL565TOCOLORVAL(pixelval);
			inbuf += sizeof(unsigned short);
			break;
		case MWPF_TRUECOLOR555:
			pixelval = *(unsigned short *)inbuf;
			colorval = PIXEL555TOCOLORVAL(pixelval);
			inbuf += sizeof(unsigned short);
			break;
		default:
			return;
		}

		/* convert from BGR colorval to desired output pixel format*/
		switch (outpixtype) {
		case MWPF_RGB:
			*(MWCOLORVAL *)outbuf = colorval;
			outbuf += sizeof(MWCOLORVAL);
			break;
		case MWPF_PIXELVAL:
			/* convert based on compile-time MWPIXEL_FORMAT*/
#if MWPIXEL_FORMAT == MWPF_PALETTE
			*(MWPIXELVAL *)outbuf = GdFindNearestColor(gr_palette,
					gr_palsize, colorval);
#else
			*(MWPIXELVAL *)outbuf = COLORVALTOPIXELVAL(colorval);
#endif
			outbuf += sizeof(MWPIXELVAL);
			break;
		case MWPF_PALETTE:
			*outbuf++ = GdFindNearestColor(gr_palette, gr_palsize,
					colorval);
			break;
		case MWPF_TRUECOLOR332:
			*outbuf++ = COLOR2PIXEL332(colorval);
			break;
		case MWPF_TRUECOLOR0888:
			*(unsigned long *)outbuf = COLOR2PIXEL888(colorval);
			outbuf += sizeof(unsigned long);
			break;
		case MWPF_TRUECOLOR8888:
			*(unsigned long *) outbuf =
				COLOR2PIXEL8888(colorval);
			outbuf += sizeof(unsigned long);
			break;
		case MWPF_TRUECOLOR888:
			*outbuf++ = REDVALUE(colorval);
			*outbuf++ = GREENVALUE(colorval);
			*outbuf++ = BLUEVALUE(colorval);
			break;
		case MWPF_TRUECOLOR565:
			*(unsigned short *)outbuf = COLOR2PIXEL565(colorval);
			outbuf += sizeof(unsigned short);
			break;
		case MWPF_TRUECOLOR555:
			*(unsigned short *)outbuf = COLOR2PIXEL555(colorval);
			outbuf += sizeof(unsigned short);
			break;
		}
	    }

	    /* adjust line widths, if necessary*/
	    if(inpitch > width)
		    inbuf += inpitch - width;
	    if(outpitch > width)
		    outbuf += outpitch - width;
	}
}
