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

extern int 	  gr_mode; 	      /* drawing mode */
//extern MWPALENTRY gr_palette[256];    /* current palette*/
//extern int	  gr_firstuserpalentry;/* first user-changable palette entry*/
//extern int 	  gr_nextpalentry;    /* next available palette entry*/

/* These support drawing dashed lines */
extern uint32_t gr_dashmask;     /* An actual bitmask of the dash values */
extern uint32_t gr_dashcount;    /* The number of bits defined in the dashmask */

extern int        gr_fillmode;

/*static*/ void drawpoint(PSD psd,MWCOORD x, MWCOORD y);

/*static*/ void drawrow(PSD psd,MWCOORD x1,MWCOORD x2,MWCOORD y);
/*static*/ void drawcol(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2);

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

/* slow draw a mono word msb bitmap, use precalced clipresult if passed*/
void
GdBitmapByPoint(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	const MWIMAGEBITS *imagebits, int clipresult)
{
	MWCOORD minx;
	MWCOORD maxx;
	MWIMAGEBITS bitvalue = 0;	/* bitmap word value */
	int bitcount;			/* number of bits left in bitmap word */

	DPRINTF("Using slow GdBitmapByPoint\n");
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
		if ((bitvalue & 0x8000) && (clipresult == CLIP_VISIBLE || GdClipPoint(psd, x, y)))
			psd->DrawPixel(psd, x, y, gr_foreground);
		bitvalue = bitvalue << 1;
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
 * MWPF_TRUECOLOR8888	uint32_t
 * MWPF_TRUECOLORABGR	uint32_t
 * MWPF_TRUECOLOR888	packed struct {char r,char g,char b} (24 bits)
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
	case MWPF_TRUECOLOR8888:
		data_format = MWIF_BGRA8888;
		break;
	default:
		/* no convblit supported*/
		break;
	}

	/* find conversion blit based on data format*/
	if (data_format)
		convblit = GdFindConvBlit(psd, data_format, MWROP_COPY);

	if (!convblit) {
		DPRINTF("GdArea: no convblit or format not supported, using slow GdAreaByPoint fallback\n");
		//GdAreaByPoint(psd, x, y, width, height, pixels, pixtype);	/* old pixel by pixel*/
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
	GdConvBlitInternal(psd, &parms, convblit);
}
