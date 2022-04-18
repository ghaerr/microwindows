/*
 * Copyright (c) 1999,2000,2001,2003,2005,2007,2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Device-independent mid level drawing and color routines.
 *
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 */
#include <string.h>
#include "swap.h"
#include "device.h"
#include "convblit.h"

extern int 	  gr_mode; 	      /* drawing mode */
extern MWPALENTRY gr_palette[256];    /* current palette*/
extern int	  gr_firstuserpalentry;/* first user-changable palette entry*/
extern int 	  gr_nextpalentry;    /* next available palette entry*/

/* These support drawing dashed lines */
extern uint32_t gr_dashmask;     /* An actual bitmask of the dash values */
extern uint32_t gr_dashcount;    /* The number of bits defined in the dashmask */

extern int        gr_fillmode;

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
	gr_foreground_rgb = fg;
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
	gr_background_rgb = bg;
	return oldbg;
}

/**
 * Set the dash mode for future drawing
 */
void
GdSetDash(uint32_t *mask, int *count)
{
	int oldm = gr_dashmask;
	int oldc = gr_dashcount;

	gr_dashmask = mask? *mask: 0;
	gr_dashcount = count? *count: 0;

	/* special case for solid line sets no dashcount for speed*/
	if (gr_dashcount == 32 && gr_dashmask == 0xFFFFFFFFL)
		gr_dashcount = 0;

	if (mask) *mask = oldm;
	if (count) *count = oldc;
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
		unsigned bit = 0;
		int p;				/* must use "int" to handle case when x2 < 0*/

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
/*static*/ void
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
	uint32_t dm = 0;
	int dc = 0;

	MWCOORD x2 = x1 + width - 1;
	MWCOORD y2 = y1 + height - 1;

	if (width <= 0 || height <= 0)
		return;

#if MW_FEATURE_SHAPES
	/* Stipples and tiles have their own drawing routines */
	if (gr_fillmode != MWFILL_SOLID) {
		set_ts_origin(x1, y1);

		ts_fillrect(psd, x1, y1, width, height);
		GdFixCursor(psd);
		return;
	}
#endif

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
	GdSetDash(&dm, &dc);

	/* The rectangle may be partially obstructed. So do it line by line. */
	while (y1 <= y2)
		drawrow(psd, x1, x2, y1++);

	/* Restore the dash settings */
	GdSetDash(&dm, &dc);

	GdFixCursor(psd);
}

#if MW_FEATURE_IMAGES
#if MW_FEATURE_PALETTE
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
	MWPIXELVALHW *convtable,int fLoadType)
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
			GdSetPalette(psd, gr_nextpalentry, newsize, &newpal[gr_nextpalentry]);
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
#endif /* MW_FEATURE_PALETTE*/

static void GdDrawImageByPoint(PSD psd, MWCOORD x, MWCOORD y, PMWIMAGEHDR pimage);
/**
 * Draw a color bitmap image in 1, 4, 8, 16, 24 or 32 bits per pixel.
 * The Microwindows color image format is DWORD padded bytes, with
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
	int			op = MWROP_COPY;
	MWBLITFUNC	convblit;
	MWBLITPARMS parms;

	/* use srcover for supported images with alpha*/
	if (pimage->data_format & MWIF_HASALPHA)
		op = MWROP_SRC_OVER;

	/* find conversion blit based on data format*/
	convblit = GdFindConvBlit(psd, pimage->data_format, op);

	/* if not using new MWIF_ format and convblit drivers, must draw pixel by pixel*/
	if (!convblit) {
		if (pimage->bpp != 4)		/* don't complain about win32 bmp format conversions*/
			DPRINTF("GdDrawImage: not RGBA/RGB format or no convblit, using slow GdDrawImageByPoint\n");
		GdDrawImageByPoint(psd, x, y, pimage);			/* old pixel-by-pixel drawing*/
		return;
	}

	/* use fast conversion blit*/
	parms.op = op;
	parms.data_format = pimage->data_format;
	parms.width = pimage->width;
	parms.height = pimage->height;
	parms.dstx = x;
	parms.dsty = y;
	parms.srcx = 0;
	parms.srcy = 0;
	parms.src_pitch = pimage->pitch;
	parms.fg_colorval = gr_foreground_rgb;		/* for convblit*/
	parms.bg_colorval = gr_background_rgb;
	parms.fg_pixelval = gr_foreground;			/* for palette mask convblit*/
	parms.bg_pixelval = gr_background;
	parms.usebg = gr_usebg;
	parms.data = pimage->imagebits;
	parms.dst_pitch = psd->pitch;		/* usually set in GdConversionBlit*/
	parms.data_out = psd->addr;
	parms.srcpsd = NULL;
	GdConvBlitInternal(psd, &parms, convblit);
}

/*
 * Alpha drawing using C bitfields.  Experimental,
 * uses bitfields rather than explicit bit-twiddling.
 */

/* ARGB8888 : 0xaarrggbb order (little endian frame buffer format BB GG RR AA)*/
typedef union {
	struct {
/* use processor byte endianness rather than bitfield order*/
#if !MW_CPU_BIG_ENDIAN
		unsigned char b; 
		unsigned char g;
		unsigned char r;
		unsigned char a; // MSByte on little endian
#else
		unsigned char a; // MSByte on big endian
		unsigned char r;
		unsigned char g;
		unsigned char b; 
#endif
	} f;
	unsigned int v; 
} ARGB8888;	

typedef union {
	/* bitfield order should use __BIG_ENDIAN_BITFIELDS, assuming so with big endian byte order*/
	struct {
#if !MW_CPU_BIG_ENDIAN	
		unsigned short b:5; 
		unsigned short g:6;
		unsigned short r:5; // MSBit on little endian
#else
		unsigned short r:5; // MSBit on big endian
		unsigned short g:6;
		unsigned short b:5;
#endif
	} f;
	unsigned short v; 
} RGB565;	

typedef union {
	/* bitfield order should use __BIG_ENDIAN_BITFIELDS, assuming so with big endian byte order*/
	struct {
#if !MW_CPU_BIG_ENDIAN	
		unsigned short b:5; 
		unsigned short g:5;
		unsigned short r:5;
		unsigned short a:1; // MSBit on little endian
#else
		unsigned short a:1; // MSBit on big endian
		unsigned short r:5;
		unsigned short g:5;
		unsigned short b:5; 
#endif		
	} f;
	unsigned short v; 
} RGB555;	

typedef union {
	/* bitfield order should use __BIG_ENDIAN_BITFIELDS, assuming so with big endian byte order*/
	struct {
#if !MW_CPU_BIG_ENDIAN	
		unsigned short r:5; 
		unsigned short g:5;
		unsigned short b:5;
		unsigned short a:1; // MSBit on little endian
#else
		unsigned short a:1; // MSBit on big endian
		unsigned short b:5;
		unsigned short g:5;
		unsigned short r:5; 
#endif		
	} f;
	unsigned short v; 
} RGB1555;	


/* slow draw point by point with clipping*/
static void
GdDrawImageByPoint(PSD psd, MWCOORD x, MWCOORD y, PMWIMAGEHDR pimage)
{
	MWCOORD minx;
	MWCOORD maxx;
	MWUCHAR bitvalue = 0;
	int bitcount;
	MWUCHAR *imagebits;
	MWCOORD height, width;
	int bpp;
	MWPIXELVALHW pixel;
	int clip;
	int extra, linesize;
	int rgborder, alpha;
	MWCOLORVAL cr = 0;
	MWCOORD yoff;
	uint32_t transcolor;
	MWPIXELVALHW convtable[256];

	height = pimage->height;
	width = pimage->width;

	/* determine if entire image is clipped out, save clipresult for later*/
	clip = GdClipArea(psd, x, y, x + width - 1, y + height - 1);
	if(clip == CLIP_INVISIBLE)
		return;

	transcolor = pimage->transcolor;
	bpp = pimage->bpp;

#if MW_FEATURE_PALETTE
	/*
	 * Merge the images's palette and build a palette index conversion table.
	 */
	if (pimage->bpp <= 8) {
		if(!pimage->palette) {			/* for jpeg's without a palette*/
			for(yoff=0; yoff<pimage->palsize; ++yoff)
				convtable[yoff] = yoff;
		} else
			GdMakePaletteConversionTable(psd, pimage->palette, pimage->palsize,
				convtable, MERGEPALETTE);
	}
#endif
	minx = x;
	maxx = x + width - 1;
	imagebits = pimage->imagebits;

//	/* check for bottom-up image*/
//	if(pimage->compression & MWIMAGE_UPSIDEDOWN) {
//		y += height - 1;
//		yoff = -1;
//	} else
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
	case 18:
		linesize = width*3;
		break;
	case 16:
		linesize = width*2;
		break;
	case 4:
		linesize = PIX2BYTES(width<<2);
		break;
	case 1:
		linesize = PIX2BYTES(width);
		break;
	}
	extra = pimage->pitch - linesize;

	/* Image format in RGB rather than BGR byte order?*/
	rgborder = (pimage->data_format == MWIF_RGB888 || pimage->data_format == MWIF_RGBA8888);

	/* handle 32bpp RGBA images with alpha channel*/
	if (pimage->data_format == MWIF_RGBA8888) {
		int32_t *data = (int32_t *)imagebits;

		while (height > 0) {
			/* 
			 * Grab 32 bits of data using processor endianness.
			 * For little endian:
			 *	MWIMAGE_RGB will be long word 0xAABBGGRR (ABGR)
			 *  MWIMAGE_BGR will be long word 0xAARRGGBB (ARGB)
			 *
			 * For big endian:
			 *	MWIMAGE_RGB will be long word 0xRRGGBBAA (RGBA)
			 *  MWIMAGE_BGR will be long word 0xBBGGRRAA (BGRA)
			 */
			cr = *data++;

			/*
			 * Convert to ARGB (MWPIXELVAL little endian framebuffer format)
			 */
#if MW_CPU_BIG_ENDIAN
			if (rgborder) {
				/*
				 * cr is RGBA, shift RGB >> 8 and A << 24 to ARGB.
				 */
				cr =  ((cr & 0xFFFFFF00UL) >> 8)
					| ((cr & 0x000000FFUL) << 24);
			} else {
				/*
				 * cr is BGRA, swap endianness to ARGB.
				 */
				cr =  ((cr & 0xFF000000UL) >> 24)
					| ((cr & 0x00FF0000UL) >> 8)
					| ((cr & 0x0000FF00UL) << 8)
					| ((cr & 0x000000FFUL) << 24);
			}
#else /* little endian*/
			if (rgborder) {
				/* 
				 * cr is ABGR, swap R/B to ARGB.
				 */
				cr = (cr & 0xFF00FF00UL)
					| ((cr & 0x00FF0000UL) >> 16)
					| ((cr & 0x000000FFUL) << 16);
			} else {
				/*
				 * cr is ARGB, no change needed.
				 */
			}
#endif
			/* cr is now in ARGB(0xaarrggbb) format*/
			alpha = (cr >> 24);
			if (alpha != 0) { /* skip if pixel is fully transparent*/
				if (clip == CLIP_VISIBLE || GdClipPoint(psd, x, y)) {
					switch (psd->pixtype) {
#if 1 /* alpha blending*/
					/* implement alpha blending image draw from image alpha channel*/
					case MWPF_TRUECOLOR8888:
					case MWPF_TRUECOLOR888:
						if (alpha == 255)
							pixel = cr;		/* both cr and pixel are ARGB (0xAARRGGBB)*/
						else {					
							/* ARGB8888   : 0xAARRGGBB*/
							/* MWPIXELVAL : 0x00RRGGBB*/
							ARGB8888 	  fg;
							ARGB8888 	  bg;

							fg.v = cr;
							bg.v = psd->ReadPixel(psd,x,y);

 							//bg +=muldiv255(a,fg-bg)
							bg.f.r += muldiv255(alpha, fg.f.r - bg.f.r);
							bg.f.g += muldiv255(alpha, fg.f.g - bg.f.g);
							bg.f.b += muldiv255(alpha, fg.f.b - bg.f.b);

							//d += muldiv255(a, 255 - d)
 							bg.f.a += muldiv255(alpha, 255 - bg.f.a);

 							//d = muldiv255(d, 255 - a) + a
 							//bg.f.a = muldiv255(bg.f.a, 255 - alpha) + alpha;

							pixel = bg.v;	/* endian swap handled with ARGB8888 struct*/
						}
						break;
					case MWPF_TRUECOLORABGR:
						if (alpha == 255)
							pixel = PIXEL888TOCOLORVAL(cr);	/* convert ARGB cr to ABGR pixel*/
						else {					
							/* ARGB8888   : 0xAARRGGBB*/
							/* MWPIXELVAL : 0xAABBGGRR*/
							ARGB8888 	  fg;
							ARGB8888 	  bg;

							/* tricky code: just swap red/blue from above case for bg pixel*/
							fg.v = cr;
							bg.v = psd->ReadPixel(psd,x,y);

 							//bg +=muldiv255(a,fg-bg)
							bg.f.b += muldiv255(alpha, fg.f.r - bg.f.b); /* actually bg red*/
							bg.f.g += muldiv255(alpha, fg.f.g - bg.f.g);
							bg.f.r += muldiv255(alpha, fg.f.b - bg.f.r); /* actually bg blue*/

							//d += muldiv255(a, 255 - d)
 							bg.f.a += muldiv255(alpha, 255 - bg.f.a);

							pixel = bg.v;	/* endian swap handled with ARGB8888 struct*/
						}
						break;
					case MWPF_TRUECOLOR565:
						if (alpha == 255)
							pixel = ARGB2PIXEL565(cr);
						else {
							/* ARGB565    : 0xAARRGGBB*/
							/* MWPIXELVAL : r/g/b 5/6/5*/
							ARGB8888  fg;							
							RGB565 	  bg;

							/*
							 * 1) case: DrawPixel(x,y,c)
							 * c=(b)rrrrrggg,gggbbbbb (MSB,LSB order)
							 * memory format at address fb(x,y). i.e. ADDR (lo,hi addr order)
							 *          ADDR[0],  ADDR[1]
							 * little : gggbbbbb,rrrrrggg 
							 * big    : rrrrrggg,gggbbbbb 
 							 * 
							 * 2) case: ushort c = ReadPixel(x,y)
							 * ushort c format. (MSB,LSB order)
							 * 			     MSB      LSB
							 * little : c=(b)rrrrrggg,gggbbbbb 
							 * big    : c=(b)rrrrrggg,gggbbbbb 
							 */
							fg.v = cr;
							bg.v = psd->ReadPixel(psd,x,y);

 							//bg +=mulscale(a,fg-bg)
							bg.f.r += mulscale(alpha, fg.f.r - (bg.f.r<<3), 11);
							bg.f.g += mulscale(alpha, fg.f.g - (bg.f.g<<2), 10);
							bg.f.b += mulscale(alpha, fg.f.b - (bg.f.b<<3), 11);

							//bg.f.a = 0;
							pixel = bg.v;
						}
						break;
					case MWPF_TRUECOLOR555:
						if (alpha == 255)
						  pixel = COLOR2PIXEL555(cr);
						else {
							/* ARGB8888   : 0xAARRGGBB*/
							/* MWPIXELVAL : r/g/b 5/5/5*/
							ARGB8888  fg;							
							RGB555 	  bg;
							
							fg.v = cr;
							bg.v = psd->ReadPixel(psd,x,y);

 							//bg +=mulscale(a,fg-bg)
							bg.f.r += mulscale(alpha, fg.f.r - (bg.f.r<<3), 11);
							bg.f.g += mulscale(alpha, fg.f.g - (bg.f.g<<3), 11);
							bg.f.b += mulscale(alpha, fg.f.b - (bg.f.b<<3), 11);

							//bg.f.a = 0;
							pixel = bg.v;
						}
						break;

					case MWPF_TRUECOLOR1555:
						if (alpha == 255)
						        pixel = COLOR2PIXEL1555(cr);
						else {
							/* ARGB8888   : 0xAARRGGBB*/
							/* MWPIXELVAL : r/g/b 5/5/5*/
							ARGB8888  fg;
							RGB1555   bg;
							
							fg.v = cr;
							bg.v = psd->ReadPixel(psd,x,y);

 							//bg +=mulscale(a,fg-bg)
							bg.f.r += mulscale(alpha, fg.f.r - PIXEL1555RED8(bg.v), 11);
							bg.f.g += mulscale(alpha, fg.f.g - PIXEL1555GREEN8(bg.v), 11);
							bg.f.b += mulscale(alpha, fg.f.b - PIXEL1555BLUE(bg.v), 11);

							//bg.f.a = 0;
							pixel = bg.v;
						}
						break;

#else /* !alpha blending*/
					/* implement image draw without alpha blending*/
					/*
					 * Draw without alpha blending.
					 * Must convert cr from ARGB format to ABGR colorval.
					 */
					case MWPF_TRUECOLOR8888:
						pixel = COLOR2PIXEL8888(ARGB2COLORVAL(cr));
						break;
					case MWPF_TRUECOLORABGR:
						pixel = COLOR2PIXELABGR(ARGB2COLORVAL(cr));
						break;
					case MWPF_TRUECOLOR888:
						pixel = COLOR2PIXEL888(ARGB2COLORVAL(cr));
						break;
					case MWPF_TRUECOLOR565:
						pixel = COLOR2PIXEL565(ARGB2COLORVAL(cr));
						break;
					case MWPF_TRUECOLOR555:
						pixel = COLOR2PIXEL555(ARGB2COLORVAL(cr));
						break;
					case MWPF_TRUECOLOR1555:
					        pixel = COLOR2PIXEL1555(ARGB2COLORVAL(cr));
#endif /* alpha blending*/

					case MWPF_PALETTE:
					default:
						pixel = GdFindColor(psd, ARGB2COLORVAL(cr));
						break;
					case MWPF_TRUECOLOR332:
						pixel = COLOR2PIXEL332(ARGB2COLORVAL(cr));
						break;
					case MWPF_TRUECOLOR233:
						pixel = COLOR2PIXEL233(ARGB2COLORVAL(cr));
						break;
					}
					psd->DrawPixel(psd, x, y, pixel);

				}
			}

			if (x++ == maxx) {
				x = minx;
				y += yoff;
				--height;
				data = (int32_t *) (((char *) data) + extra);
			}
		}
	} /* end of alpha channel image handling*/
	else if (bpp > 8) {
		/* handle non-alpha images of 16, 18, 24 or 32bpp*/
		while (height > 0) {
			/* get value in correct RGB or BGR byte order*/
			if (bpp == 24 || bpp == 18) {
				cr = rgborder
					? MWRGB(imagebits[0], imagebits[1], imagebits[2])
					: MWRGB(imagebits[2], imagebits[1], imagebits[0]);
				imagebits += 3;
			} else if (bpp == 32) {
				cr = rgborder
					? MWARGB(imagebits[3],imagebits[0], imagebits[1], imagebits[2])
					: MWARGB(imagebits[3],imagebits[2], imagebits[1], imagebits[0]);
				imagebits += 4;
			} else {	/* 16 bpp*/
#if MW_CPU_BIG_ENDIAN
				unsigned int pv = (imagebits[0] << 8) | imagebits[1];
#else
				unsigned int pv = (imagebits[1] << 8) | imagebits[0];
#endif
				switch (pimage->data_format)
				{
				  case MWIF_RGB555:
				    cr =  PIXEL555TOCOLORVAL(pv);
				    break;
				  case MWIF_RGB565:
				    cr = PIXEL565TOCOLORVAL(pv);
				    break;
				  case MWIF_RGB1555:
				    cr = PIXEL1555TOCOLORVAL(pv);
				    break;
				}
				imagebits += 2;
			}

			/* handle transparent color */
			if (transcolor != cr) {

				switch (psd->pixtype) {
				case MWPF_PALETTE:
				default:
					pixel = GdFindColor(psd, cr);
					break;
				case MWPF_TRUECOLOR8888:
					pixel = COLOR2PIXEL8888(cr);
					break;
				case MWPF_TRUECOLORABGR:
					pixel = COLOR2PIXELABGR(cr);
					break;
				case MWPF_TRUECOLOR888:
					pixel = COLOR2PIXEL888(cr);
					break;
				case MWPF_TRUECOLOR565:
					pixel = COLOR2PIXEL565(cr);
					break;
				case MWPF_TRUECOLOR555:
					pixel = COLOR2PIXEL555(cr);
					break;
				case MWPF_TRUECOLOR1555:
				        pixel = COLOR2PIXEL1555(cr);
					break;
				case MWPF_TRUECOLOR332:
					pixel = COLOR2PIXEL332(cr);
					break;
				case MWPF_TRUECOLOR233:
					pixel = COLOR2PIXEL233(cr);
					break;
				}

				if (clip == CLIP_VISIBLE || GdClipPoint(psd, x, y))
					psd->DrawPixel(psd, x, y, pixel);
#if 0
				/* fix: use clipmaxx to clip quicker */
				else if (clip != CLIP_VISIBLE && !clipresult && x > clipmaxx)
					x = maxx;
#endif
			}

			if (x++ == maxx) {
				x = minx;
				y += yoff;
				--height;
				imagebits += extra;
			}
		}
	} /* end of 16, 18, 24 or 32bpp non-alpha image handling*/
	else {
		/* handle palettized images of 8, 4 or 1bpp*/
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
			else if (clip != CLIP_VISIBLE && !clipresult && x > clipmaxx)
				x = maxx;
#endif
		next:
			if (x++ == maxx) {
				x = minx;
				y += yoff;
				--height;
				bitcount = 0;
				imagebits += extra;
			}
		}
	} /* end of palettized images of 8, 4 or 1bpp handling*/

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
 *
 * @param psd Drawing surface.
 * @param x Left edge of destination rectangle.
 * @param y Top edge of destination rectangle.
 * @param width Width of bitmap.  Equal to width of destination rectangle.
 * @param height Height of bitmap.  Equal to height of destination rectangle.
 * @param imagebits The bitmap to draw.
 */
void
GdBitmap(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height, const MWIMAGEBITS *imagebits)
{
	MWBLITFUNC	convblit;
	MWBLITPARMS parms;

	/* try finding blitter, else fallback to drawing point by point*/
	convblit = GdFindConvBlit(psd, MWIF_MONOWORDMSB, MWROP_COPY);
	if (!convblit) {
		GdBitmapByPoint(psd, x, y, width, height, imagebits, -1);
		return;
	}

	if (width <= 0 || height <= 0)
		return;

	parms.op = MWROP_COPY;					/* copy to dst, 1=fg (0=bg if usebg)*/
	parms.data_format = MWIF_MONOWORDMSB;	/* data is 1bpp words, msb first*/
	parms.width = width;
	parms.height = height;
	parms.dstx = x;
	parms.dsty = y;
	parms.srcx = 0;
	parms.srcy = 0;
	parms.src_pitch = ((width + 15) >> 4) << 1;	/* pad to WORD boundary*/
	parms.fg_colorval = gr_foreground_rgb;
	parms.bg_colorval = gr_background_rgb;
	parms.fg_pixelval = gr_foreground;			/* for palette mask convblit*/
	parms.bg_pixelval = gr_background;
	parms.usebg = gr_usebg;
	parms.data = (char *)imagebits;
	parms.dst_pitch = psd->pitch;			/* usually set in GdConversionBlit*/
	parms.data_out = psd->addr;
	parms.srcpsd = NULL;
	GdConvBlitInternal(psd, &parms, convblit);
}
#endif /* MW_FEATURE_IMAGES*/

/* slow draw a mono word msb bitmap, use precalced clipresult if passed*/
void
GdBitmapByPoint(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	const MWIMAGEBITS *imagebits, int clipresult)
{
	MWCOORD minx;
	MWCOORD maxx;
	MWIMAGEBITS bitvalue = 0;	/* bitmap word value */
	int bitcount;			/* number of bits left in bitmap word */

#if MWPIXEL_FORMAT != MWPF_PALETTE
	DPRINTF("Using slow GdBitmapByPoint\n");
#endif
	if (width <= 0 || height <= 0)
		return;

	/* get valid clipresult if required*/
	if (clipresult < 0)
		clipresult = GdClipArea(psd, x, y, x + width - 1, y + height - 1);

	if (clipresult == CLIP_INVISIBLE)
		return;

	/* fill background if necessary, use quick method if no clipping*/
	if (gr_usebg) {
		if (clipresult == CLIP_VISIBLE)
			psd->FillRect(psd, x, y, x + width - 1, y + height - 1, gr_background);
		else {
			MWPIXELVAL savefg = gr_foreground;
			gr_foreground = gr_background;
			GdFillRect(psd, x, y, width, height);
			gr_foreground = savefg;
		}
	}
	minx = x;
	maxx = x + width - 1;
	bitcount = 0;
	while (height > 0) {
		if (bitcount <= 0) {
			bitcount = MWIMAGE_BITSPERIMAGE;
			bitvalue = *imagebits++;
		}
		if (MWIMAGE_TESTBIT(bitvalue) && (clipresult == CLIP_VISIBLE || GdClipPoint(psd, x, y)))
			psd->DrawPixel(psd, x, y, gr_foreground);
		bitvalue = MWIMAGE_SHIFTBIT(bitvalue);
		bitcount--;
		if (x++ == maxx) {
			x = minx;
			++y;
			--height;
			bitcount = 0;
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
GdReadArea(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height, MWPIXELVALHW *pixels)
{
	MWCOORD 		row;
	MWCOORD 		col;

	if (width <= 0 || height <= 0)
		return;

	GdCheckCursor(psd, x, y, x+width-1, y+height-1);
	for (row = y; row < height+y; row++) {
		for (col = x; col < width+x; col++) {
			if (row < 0 || row >= psd->yvirtres || col < 0 || col >= psd->xvirtres) {
				*pixels++ = 0;
				//printf("_");
			} else {
				*pixels++ = psd->ReadPixel(psd, col, row);
				//v = ((v&255) + ((v>>8)&255) + ((v>>16)&255)) / 3;
				//printf("%c", "_.:;oVM@X"[v>>5]);
			}
		}
		//printf("\n");
	}
	GdFixCursor(psd);
}

static void GdAreaByPoint(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
				void *pixels, int pixtype);
/**
 * Draw a rectangle of color values, clipping if necessary.
 * If a color matches the background color,
 * then that pixel is only drawn if the gr_usebg flag is set.
 *
 * The pixels are packed according to pixtype:
 *
 * pixtype		array of
 * MWPF_RGB		MWCOLORVAL (uint32_t)
 * MWPF_PIXELVAL	MWPIXELVALHW (compile-time dependent)
 * MWPF_PALETTE		unsigned char
 * MWPF_TRUECOLORARGB	uint32_t
 * MWPF_TRUECOLORABGR	uint32_t
 * MWPF_TRUECOLORRGB	packed struct {char r,char g,char b} (24 bits)
 * MWPF_TRUECOLOR565	unsigned short
 * MWPF_TRUECOLOR555	unsigned short
 * MWPF_TRUECOLOR332	unsigned char
 * MWPF_TRUECOLOR233	unsigned char
 *
 * NOTE: Currently, no translation is performed if the pixtype
 * is not MWPF_RGB.  Pixtype is only then used to determine the
 * packed size of the pixel data, and is then stored unmodified
 * in a MWPIXELVALHW and passed to the screen driver.  Virtually,
 * this means there's only three reasonable options for client
 * programs: (1) pass all data as RGB MWCOLORVALs, (2) pass
 * data as unpacked 32-bit MWPIXELVALHWs in the format the current
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
GdArea(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height, void *pixels, int pixtype)
{
	int pixsize = 4;
	int	data_format = 0;
	MWBLITFUNC convblit = NULL;
	MWBLITPARMS parms;

	/* convert MWPF_HWPIXELVAL to real pixel type*/
	if (pixtype == MWPF_HWPIXELVAL)
		pixtype = psd->pixtype;

	/* Calculate size of packed pixels and possible fast blitter*/
	switch(pixtype) {
	case MWPF_RGB:
	case MWPF_TRUECOLORABGR:
		data_format = MWIF_RGBA8888;
		break;
	case MWPF_PIXELVAL:
		pixsize = sizeof(MWPIXELVALHW);
		switch (pixsize) {
		case 4:
			if (psd->bpp == 32)
				data_format = psd->data_format;		/* will use 32bpp copy*/
			else if (psd->data_format == MWIF_BGR888)
				data_format = MWIF_BGRA8888;		/* try 32bpp BGRA to 24bpp BGR copy*/
			break;
		case 2:
			if (psd->bpp == 16)
				data_format = psd->data_format;		/* will use 16bpp copy*/
			break;
		case 1:
			break;
		}
		break;
	case MWPF_TRUECOLORARGB:
		data_format = MWIF_BGRA8888;
		break;
	case MWPF_TRUECOLORRGB:
		data_format = MWIF_BGR888;
		pixsize = 3;
		break;
	case MWPF_TRUECOLOR565:
		data_format = MWIF_RGB565;
		pixsize = 2;
		break;
	case MWPF_TRUECOLOR555:
		data_format = MWIF_RGB555;
		pixsize = 2;
		break;
	case MWPF_TRUECOLOR1555:
	        data_format = MWIF_RGB1555;
		pixsize = 2;
                break;
	case MWPF_PALETTE:
	case MWPF_TRUECOLOR233:
	case MWPF_TRUECOLOR332:
	default:
		/* no convblit supported*/
		break;
	}

	/* find conversion blit based on data format*/
	if (data_format)
		convblit = GdFindConvBlit(psd, data_format, MWROP_COPY);

	if (!convblit) {
		DPRINTF("GdArea: no convblit or format not supported, using slow GdAreaByPoint fallback\n");
		GdAreaByPoint(psd, x, y, width, height, pixels, pixtype);	/* old pixel by pixel*/
		return;
	}

	/* prepare blit parameters*/
	parms.op = MWROP_COPY;
	parms.data_format = 0;
	parms.width = width;
	parms.height = height;
	parms.dstx = x;
	parms.dsty = y;
	parms.srcx = 0;
	parms.srcy = 0;
	parms.src_pitch = width * pixsize;
	parms.fg_colorval = gr_foreground_rgb;		/* for convblit*/
	parms.bg_colorval = gr_background_rgb;
	parms.fg_pixelval = gr_foreground;			/* for palette mask convblit*/
	parms.bg_pixelval = gr_background;
	parms.usebg = gr_usebg;
	parms.data = pixels;
	parms.dst_pitch = psd->pitch;		/* usually set in GdConversionBlit*/
	parms.data_out = psd->addr;
	parms.srcpsd = NULL;
//GdPrintBitmap(&parms, pixsize);
	GdConvBlitInternal(psd, &parms, convblit);
}

static void
GdAreaByPoint(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height, void *pixels, int pixtype)
{
	unsigned char *PIXELS = pixels;	/* for ANSI compilers, can't use void*/
	MWCOORD minx = x;
	MWCOORD maxx = x + width - 1;
	int32_t cellstodo;			/* remaining number of cells */
	int32_t count;			/* number of cells of same color */
	int32_t cc;			/* current cell count */
	int32_t rows;			/* number of complete rows */
	MWPIXELVAL savecolor;		/* saved foreground color */
	MWBOOL dodraw;			/* TRUE if draw these points */
	MWCOLORVAL rgbcolor = 0L;
	unsigned char r, g, b;

	/* Set up area clipping, and just return if nothing is visible */
	if (GdClipArea(psd, minx, y, maxx, y + height - 1) == CLIP_INVISIBLE )
		return;

	/* no fast low level routine, draw point-by-point...*/
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
		gr_foreground = *(MWPIXELVALHW *)PIXELS;
		PIXELS += sizeof(MWPIXELVALHW);
		break;
	case MWPF_PALETTE:
	case MWPF_TRUECOLOR233:
	case MWPF_TRUECOLOR332:
		gr_foreground = *PIXELS++;
		break;
	case MWPF_TRUECOLORABGR:
	case MWPF_TRUECOLOR8888:
		gr_foreground = *(uint32_t *)PIXELS;
		PIXELS += sizeof(uint32_t);
		break;
	case MWPF_TRUECOLOR888:
		r = *PIXELS++;
		g = *PIXELS++;
		b = *PIXELS++;
		gr_foreground = RGB2PIXEL888(r, g, b);
		break;
	case MWPF_TRUECOLOR565:
	case MWPF_TRUECOLOR555:
	case MWPF_TRUECOLOR1555:
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
		case MWPF_HWPIXELVAL:
			if(gr_foreground != *(MWPIXELVALHW *)PIXELS)
				goto breakwhile;
			PIXELS += sizeof(MWPIXELVALHW);
			break;
		case MWPF_PALETTE:
		case MWPF_TRUECOLOR233:
		case MWPF_TRUECOLOR332:
			if(gr_foreground != *(unsigned char *)PIXELS)
				goto breakwhile;
			++PIXELS;
			break;
		case MWPF_TRUECOLOR8888:
		case MWPF_TRUECOLORABGR:
			if(gr_foreground != *(uint32_t *)PIXELS)
				goto breakwhile;
			PIXELS += sizeof(uint32_t);
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
		case MWPF_TRUECOLOR1555:
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

#if LATER
/**
 * Translate a rectangle of color values
 *
 * The pixels are packed according to inpixtype/outpixtype:
 *
 * pixtype		array of
 * MWPF_RGB		MWCOLORVAL (uint32_t)
 * MWPF_PIXELVAL	MWPIXELVALHW (compile-time dependent)
 * MWPF_PALETTE		unsigned char
 * MWPF_TRUECOLOR8888	uint32_t
 * MWPF_TRUECOLORABGR	uint32_t
 * MWPF_TRUECOLOR888	packed struct {char r,char g,char b} (24 bits)
 * MWPF_TRUECOLOR565	unsigned short
 * MWPF_TRUECOLOR555	unsigned short
 * MWPF_TRUECOLOR332	unsigned char
 * MWPF_TRUECOLOR233	unsigned char
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
	uint32_t	pixelval;
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
			pixelval = *(MWPIXELVALHW *)inbuf;
			inbuf += sizeof(MWPIXELVALHW);
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
		case MWPF_TRUECOLOR233:
			pixelval = *inbuf++;
			colorval = PIXEL233TOCOLORVAL(pixelval);
			break;
		case MWPF_TRUECOLORABGR:
			pixelval = *(uint32_t *)inbuf;
			colorval = PIXELABGRTOCOLORVAL(pixelval);
			inbuf += sizeof(uint32_t);
			break;
		case MWPF_TRUECOLOR8888:
			pixelval = *(uint32_t *)inbuf;
			colorval = PIXEL8888TOCOLORVAL(pixelval);
			inbuf += sizeof(uint32_t);
			break;
		case MWPF_TRUECOLOR888:
			r = *inbuf++;
			g = *inbuf++;
			b = *inbuf++;
			colorval = MWRGB(r, g, b);
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
		case MWPF_TRUECOLOR1555:
		        pixelval = *(unsigned short *)inbuf;
			colorval = PIXEL1555TOCOLORVAL(pixelval);
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
			*(MWPIXELVALHW *)outbuf = GdFindNearestColor(gr_palette, gr_palsize, colorval);
#else
			*(MWPIXELVALHW *)outbuf = COLORVALTOPIXELVAL(colorval);
#endif
			outbuf += sizeof(MWPIXELVALHW);
			break;
		case MWPF_PALETTE:
			*outbuf++ = GdFindNearestColor(gr_palette, gr_palsize,
					colorval);
			break;
		case MWPF_TRUECOLOR332:
			*outbuf++ = COLOR2PIXEL332(colorval);
			break;
		case MWPF_TRUECOLOR233:
			*outbuf++ = COLOR2PIXEL233(colorval);
			break;
		case MWPF_TRUECOLOR8888:
			*(uint32_t *)outbuf = COLOR2PIXEL8888(colorval);
			outbuf += sizeof(uint32_t);
			break;
		case MWPF_TRUECOLORABGR:
			*(uint32_t *)outbuf = COLOR2PIXELABGR(colorval);
			outbuf += sizeof(uint32_t);
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
		case MWPF_TRUECOLOR1555:
		        *(unsigned short *)outbuf = COLOR2PIXEL1555(colorval);
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
#endif /* LATER*/
