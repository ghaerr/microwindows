/*
 * Copyright (c) 1999,2000,2001,2003,2005,2007,2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Device-independent mid level blit routines.
 *
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 */
/*#define NDEBUG*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "device.h"
#include "convblit.h"

/* find a conversion blit based on data format and blit op*/
/* used by GdBitmap, GdArea and GdDrawImage*/
MWBLITFUNC
GdFindConvBlit(PSD psd, int data_format, int op)
{
	MWBLITFUNC convblit = NULL;

	/*DPRINTF("GdFindConvBlit format %x, op %d\n", data_format, op);*/

#if SWIEROS
	convblit = psd->BlitCopyMaskMonoWordMSB;	/* conv mono word MSBFirst*/
#else
	/* determine which blit to use*/
	switch (data_format) {
	case MWIF_ALPHABYTE:			/* ft2 alias, t1lib alias*/
		convblit = psd->BlitBlendMaskAlphaByte;		/* conv 8bpp alpha with fg/bg*/
		break;

	case MWIF_MONOBYTEMSB:			/* ft2 non-alias*/
		convblit = psd->BlitCopyMaskMonoByteMSB;	/* conv mono byte MSBFirst*/
		break;

	case MWIF_MONOWORDMSB:			/* core mwcfont, pcf*/
		convblit = psd->BlitCopyMaskMonoWordMSB;	/* conv mono word MSBFirst*/
		break;

	case MWIF_MONOBYTELSB:			/* t1lib non-alias*/
		convblit = psd->BlitCopyMaskMonoByteLSB;	/* conv mono byte LSBFirst*/
		break;

	case MWIF_RGBA8888:				/* png 32bpp w/alpha, GdArea MWPF_RGB/MWPF_TRUECOLORABGR*/
		if (op == MWROP_SRC_OVER) {
			convblit = psd->BlitSrcOverRGBA8888;	/* image, src 32bpp w/alpha - srcover*/
			break;
		}
		/* assume copy*/
		convblit = psd->BlitCopyRGBA8888;			/* GdArea MWPF_RGB*/
		break;

	case MWIF_BGRA8888:				/* GdArea MWPF_TRUECOLOR8888*/
		/* assume copy*/
		if (psd->data_format == MWIF_BGRA8888)
			convblit = convblit_copy_8888_8888;		/* 32bpp to 32bpp copy*/
		else if (psd->data_format == MWIF_BGR888)	/* GdArea MWPF_PIXELVAL conversion*/
			convblit = convblit_copy_bgra8888_bgr888; /* 32bpp BGRX to 24bpp BGR copy*/
		break;

	case MWIF_RGB888:				/* png 24bpp no alpha*/
		convblit = psd->BlitCopyRGB888;				/* image, src 24bpp - copy*/
		break;

	case MWIF_BGR888:				/* GdArea MWPF_TRUECOLOR888*/
		if (psd->data_format == MWIF_BGR888)
			convblit = convblit_copy_888_888;		/* 24bpp to 24bpp copy*/
		break;

	case MWIF_RGB565:				/* GdArea MWPF_TRUECOLOR565*/
	case MWIF_RGB555:				/* GdArea MWPF_TRUECOLOR555*/
	case MWIF_RGB1555:                              /* GdArea MWPF_TRUECOLOR1555*/
		if (psd->data_format == data_format)
			convblit = convblit_copy_16bpp_16bpp;	/* 16bpp to 16bpp copy*/
		break;
	}
#endif
	return convblit;
}

/* blit from non-psd source to destination*/
/* used by gen_drawtext, FT2 and T1LIB*/
void
GdConversionBlit(PSD psd, PMWBLITPARMS parms)
{
	/* first find blit based on data format and blit op*/
	MWBLITFUNC convblit = GdFindConvBlit(psd, parms->data_format, parms->op);

	/* call conversion blit routine with clipping*/
	if (convblit) {
		/* setup destination parms*/
		parms->dst_pitch = psd->pitch;
		parms->data_out = psd->addr;
		parms->srcpsd = NULL;		/* used in frameblits only*/

		GdConvBlitInternal(psd, parms, convblit);
		return;
	}
#if LATER
	/* check for fallback routines*/
	if (parms->data_format == MWIF_MONOWORDMSB) {			/* core mwcfont, pcf*/
		DPRINTF("GdConversionBlit: no convblit, using GdBitmap fallback\n");
		GdBitmap(psd, parms->dstx, parms->dsty, parms->width, parms->height, parms->data);
		return;
	}
#endif
	DPRINTF("GdConversionBlit: No convblit available\n");
}

/* fallback blitter wrapper for older 1,2,4 bpp subdrivers*/
static void
BlitFallback(PSD psd, PMWBLITPARMS gc)
{
	DPRINTF("GdBlit: No frameblit, using psd->BlitFallBack\n");
	psd->BlitFallback(psd, gc->dstx, gc->dsty, gc->width, gc->height,
		gc->srcpsd, gc->srcx, gc->srcy, gc->op);
}

/* find a framebuffer blit based on source data format and blit op*/
/* used by GdBlit*/
static MWBLITFUNC
GdFindFrameBlit(PSD psd, int src_data_format, int op)
{
	/*DPRINTF("GdFindFrameBlit format %x, op %d\n", src_data_format, op);*/

	/* use frameblit if same format, conversion will fail on same surface*/
	if (psd->data_format != src_data_format || !psd->FrameBlit)
	  switch (src_data_format) {	/* try conversion blits if possible*/
	case MWIF_RGBA8888:
		if (op == MWROP_SRC_OVER) {
			if (psd->BlitSrcOverRGBA8888)
				return psd->BlitSrcOverRGBA8888;
		}
		if (psd->BlitCopyRGBA8888)
			return psd->BlitCopyRGBA8888;
		break;

	case MWIF_MONOBYTEMSB:
		/* use conversion blit if destination not palette*/
		//FIXME this won't work if this function merged with GdFindConvBlit
		if (psd->BlitCopyMaskMonoByteMSB && psd->bpp >= 8)
			return psd->BlitCopyMaskMonoByteMSB;
		break;
	}

	/* try fallback blit if no frameblit*/
	if (!psd->FrameBlit) {
		if (!psd->BlitFallback)
			return NULL;
		return BlitFallback;		/* wrapper function to reorder parameters*/
	}

	/* BGRA->BGRA and RGBA->RGBA are handled properly with frameblit_xxxa in fblin32.c*/

	/* use frameblit*/
	return psd->FrameBlit;
}

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
/* Copy from srcpsd to dstpsd. Source and/or dest may be rotated, and/or same psd.*/
void
GdBlit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD width, MWCOORD height,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int rop)
{
	MWBLITFUNC	frameblit;
	MWBLITPARMS parms;

	/* Find appropriate blitter based on source data format and rop*/
	frameblit = GdFindFrameBlit(dstpsd, srcpsd->data_format, rop);
	if (!frameblit) {
		DPRINTF("GdBlit: No frameblit found for op %d\n", rop);
		return;
	}
	
	/* clip blit rectangle to source screen/bitmap size*/
	/* we must do this because there isn't any source clipping setup*/
	if (srcx < 0) {
		width += srcx;
		dstx -= srcx;
		srcx = 0;
	}
	if (srcy < 0) {
		height += srcy;
		dsty -= srcy;
		srcy = 0;
	}
	if (srcx + width > srcpsd->xvirtres)
		width = srcpsd->xvirtres - srcx;
	if (srcy + height > srcpsd->yvirtres)
		height = srcpsd->yvirtres - srcy;

	parms.op = rop;
	parms.data_format = dstpsd->data_format;
	parms.width = width;
	parms.height = height;
	parms.dstx = dstx;
	parms.dsty = dsty;
	parms.srcx = srcx;
	parms.srcy = srcy;
	parms.src_pitch = srcpsd->pitch;

	parms.fg_colorval = gr_foreground_rgb;	/* for mask convblit*/
	parms.bg_colorval = gr_background_rgb;
	parms.fg_pixelval = gr_foreground;		/* for palette mask convblit*/
	parms.bg_pixelval = gr_background;
	parms.usebg = gr_usebg;

	parms.data = srcpsd->addr;
	parms.dst_pitch = dstpsd->pitch;		/* usually set in GdConversionBlit*/
	parms.data_out = dstpsd->addr;
	parms.srcpsd = srcpsd;					/* for GdCheckCursor/GdFixCursor*/
	parms.src_xvirtres = srcpsd->xvirtres;	/* used in frameblit for src rotation*/
	parms.src_yvirtres = srcpsd->yvirtres;

	GdConvBlitInternal(dstpsd, &parms, frameblit);
}

/**
 * A proper stretch blit.  Supports flipping the image.
 * Parameters are co-ordinates of two points in the source, and
 * two corresponding points in the destination.  The image is scaled
 * and flipped as needed to make the two points correspond.  The
 * top-left corner is drawn, the bottom right one isn't [i.e.
 * (0,0)-(2,2) specifies a 2x2 rectangle consisting of the points
 * at (0,0), (0,1), (1,0), (1,1).  It does not include the points
 * where x=2 or y=2.]
 *
 * Can stretch the image by any X and/or Y scale factor.
 * Can flip the image in the X and/or Y axis.
 *
 * This is the faster version with no per-pixel multiply and a single
 * decision tree for the inner loop, by Jon.  Based on Alex's original
 * all-integer version.
 *
 * Raster ops are not yet fully implemented - see the low-level
 * drivers for details.
 *
 * Note that we do not support overlapping blits.
 *
 * @param dstpsd Drawing surface to draw to.
 * @param dx1 Destination X co-ordinate of first corner.
 * @param dy1 Destination Y co-ordinate of first corner.
 * @param dx2 Destination X co-ordinate of second corner.
 * @param dy2 Destination Y co-ordinate of second corner.
 * @param srcpsd Drawing surface to copy from.
 * @param sx1 Source X co-ordinate of first corner.
 * @param sy1 Source Y co-ordinate of first corner.
 * @param sx2 Source X co-ordinate of second corner.
 * @param sy2 Source Y co-ordinate of second corner.
 * @param rop Raster operation.
 */

void
GdStretchBlit(PSD dstpsd, MWCOORD dx1, MWCOORD dy1, MWCOORD dx2,
	MWCOORD dy2, PSD srcpsd, MWCOORD sx1, MWCOORD sy1, MWCOORD sx2,
	MWCOORD sy2, int rop)
{
	int x_numerator;		/* Scale factors (as fractions, numerator/denominator) */
	int x_denominator;
	int y_numerator;
	int y_denominator;
	int x_start_fraction;	/* Initial source co-ordinates, as a fraction (denominators as above) */
	int y_start_fraction;
	MWCOORD cx1;			/* Clipped dest co-ords */
	MWCOORD cy1;
	MWCOORD cx2;
	MWCOORD cy2;
	int clipresult;
	MWBLITFUNC convblit;
	MWBLITPARMS parms;
#if DYNAMICREGIONS
	int 		count;
	MWRECT *	prc;
#else
	int 		count;
	MWCLIPRECT *prc;
#endif

	/* frame->frame stretchblits not yet implemented*/
	assert(dstpsd != srcpsd);

	/* DPRINTF("Nano-X: GdStretchBlit(dst=%x (%d,%d)-(%d,%d), src=%x (%d,%d)-(%d,%d), op=%d\n",
	           (int) dstpsd, dx1, dy1, dx2, dy2, (int) srcpsd, sx1, sy1, sx2, sy2, rop);*/

	/* Sort co-ordinates so d1 is top left, d2 is bottom right. */
	if (dx1 > dx2) {
		MWCOORD tmp = dx2;
		dx2 = dx1;
		dx1 = tmp;
		tmp = sx2;
		sx2 = sx1;
		sx1 = tmp;
	}

	if (dy1 > dy2) {
		MWCOORD tmp = dy2;
		dy2 = dy1;
		dy1 = tmp;
		tmp = sy2;
		sy2 = sy1;
		sy1 = tmp;
	}

	/* Destination rectangle is entirely off screen, or is zero-sized*/
	if (dx2 < 0 || dy2 < 0 || dx1 > dstpsd->xvirtres || dy1 > dstpsd->yvirtres ||
		dx1 == dx2 || dy1 == dy2) {
		return;
	}

	/* If we're not stretching or flipping, use the standard blit (faster)*/
	if ((dx2 - dx1 == sx2 - sx1) && (dy2 - dy1 == sy2 - sy1)) {
		GdBlit(dstpsd, dx1, dy1, dx2 - dx1, dy2 - dy1, srcpsd, sx1, sy1, rop);
		return;
	}

	/* check for driver, there's no fallback*/
	if (!dstpsd->FrameStretchBlit) {
		DPRINTF("GdStretchBlit: no FrameStretchBlit (op %d)\n", rop);
		return;
	}

	/* special case RGBA source and src_over/copy*/
	if (srcpsd->data_format == MWIF_RGBA8888 && (rop == MWROP_SRC_OVER || rop == MWROP_COPY)) {
		convblit = dstpsd->BlitStretchRGBA8888;
		if (!convblit) {
			DPRINTF("GdStretchblit: no convblit for RGBA8888 src_over/copy\n");
			return;
		}
	} else
		convblit = dstpsd->FrameStretchBlit;

	/* Need to preserve original values, so make a copy we can clip. */
	cx1 = dx1;
	cy1 = dy1;
	cx2 = dx2;
	cy2 = dy2;

 	/*
	 * Calculate how far in source co-ordinates is equivalent to one pixel in dest co-ordinates.
	 *
 	 * x_denominator   -
 	 * y_denominator   - Denominator for source X or Y value fractions.  Note that
 	 *                   this must be even, and all the numerators must also be
 	 *                   even, so we can easily divide by 2.
 	 * x_fraction  	   -
 	 * y_fraction      - Point in source that corresponds to the top left corner
 	 *                   of the pixel (dstx, dsty).  This is a fraction - to get
 	 *					 a float, divide by y_denominator.
 	 * x_numerator     -
 	 * y_numerator     - X step in src for an x++/y++ step in dest.  May be negative
 	 *                   (for a flip).  Expressed as a fraction - divide it by
 	 *                   x/y_denominator for a float.
	 *
	 * This is stored as a fraction (numerator/denominator).
	 * The numerator may be > denominator.  The numerator
	 * may be negative, the denominator is always positive.
	 *
	 * We need half this distance for some purposes, hence the *2.
	 *
	 * The +1s are because we care about *sizes*, not deltas.  
	 * (Without the +1s it just doesn't work properly.)
	 */
	x_numerator = (sx2 - sx1 + 1) << 1;
	x_denominator = (dx2 - dx1 + 1) << 1;
	y_numerator = (sy2 - sy1 + 1) << 1;
	y_denominator = (dy2 - dy1 + 1) << 1;

	/* Clip the image so that the destination X co-ordinates
	 * in cx1 and cx2 map to a point on the source image.
	 */
	if (sx1 < 0 || sx1 > srcpsd->xvirtres || sx2 < 0 || sx2 > srcpsd->xvirtres) {
		/* Calculate where the left of the source image will end up, in dest co-ordinates*/
		int ix1 = dx1 - (sx1 * x_denominator) / x_numerator;

		/* Calculate where the right of the source image will end up, in dest co-ordinates*/
		int ix2 = dx1 + ((srcpsd->xvirtres - sx1) * x_denominator + x_denominator - 1) / x_numerator;

		/* Since we may be doing a flip, "left" and "right" in the statements
		 * above do not necessarily correspond to "left" and "right" in the
		 * destination image, which is where we're clipping.  So sort the
		 * X co-ordinates.
		 */
		if (ix1 > ix2) {
			int temp = ix1;
			ix1 = ix2;
			ix2 = temp;
		}

		/* Check for total invisibility - source X checks*/
		if (cx2 < ix1 || cx1 > ix2)
			return;

		/* Perform partial clip */
		if (cx1 < ix1) cx1 = ix1;
		if (cx2 > ix2) cx2 = ix2;
	}

	/* Clip the image so that the destination Y co-ordinates
	 * in cy1 and cy2 map to a point on the source image.
	 */
	if (sy1 < 0 || sy1 > srcpsd->yvirtres || sy2 < 0 || sy2 > srcpsd->yvirtres) {
		/* Calculate where the top of the source image will end up, in dest co-ordinates*/
		int iy1 = dy1 - (sy1 * y_denominator) / y_numerator;

		/* Calculate where the bottom of the source image will end up, in dest co-ordinates*/
		int iy2 = dy1 + ((srcpsd->yvirtres - sy1) * y_denominator + y_denominator - 1) / y_numerator;

		/* Since we may be doing a flip, "top" and bottom" in the statements
		 * above do not necessarily correspond to "top" and bottom" in the
		 * destination image, which is where we're clipping.  So sort the
		 * Y co-ordinates.
		 */
		if (iy1 > iy2) {
			int temp = iy1;
			iy1 = iy2;
			iy2 = temp;
		}

		/* Check for total invisibility - source Y checks*/
		if (cy2 < iy1 || cy1 > iy2)
			return;

		/* Perform partial clip */
		if (cy1 < iy1) cy1 = iy1;
		if (cy2 > iy2) cy2 = iy2;
	}

	/* Clip against physical screen*/
	if (cx1 < 0) cx1 = 0;
	if (cy1 < 0) cy1 = 0;
	if (cx2 > dstpsd->xvirtres) cx2 = dstpsd->xvirtres;
	if (cy2 > dstpsd->yvirtres) cy2 = dstpsd->yvirtres;

	/* Final fully-offscreen clip check */
	if (cx1 >= cx2 || cy1 >= cy2)
		return;

	/* We now have a destination rectangle defined in (cx1,cy1)-(cx2,cy2)*/

	/* DPRINTF("Nano-X: GdStretchBlit: Clipped rect: (%d,%d)-(%d,%d)\n",
	       (int) cx1, (int) cy1, (int) cx2, (int) cy2); */

	/* clip against other windows*/
	clipresult = GdClipArea(dstpsd, cx1, cy1, cx2 - 1, cy2 - 1);
	if (clipresult == CLIP_INVISIBLE)
		return;

	/* check cursor in src region of both devices*/
	GdCheckCursor(dstpsd, sx1, sy1, sx2 - 1, sy2 - 1);
	if (srcpsd != dstpsd)
		GdCheckCursor(srcpsd, sx1, sy1, sx2 - 1, sy2 - 1);

	/* if partially clipped, must check in dst region as GdClipArea didn't if not visible*/
	if (clipresult != CLIP_VISIBLE)
		GdCheckCursor(dstpsd, cx1, cy1, cx2-1, cy2-1);	/* onetime cursor check in dst region*/

	/* Calculate the starting position (fraction) in the source rectange
	 * that is equivalent to the top-left of the destination rectangle.
	 */
	x_start_fraction = sx1 * x_denominator + (cx1 - dx1) * x_numerator;
	y_start_fraction = sy1 * y_denominator + (cy1 - dy1) * y_numerator;

	/* set invariant parameters*/
	parms.op = rop;
	parms.data_format = dstpsd->data_format;
	parms.data = srcpsd->addr;
	parms.src_pitch = srcpsd->pitch;
	parms.dst_pitch = dstpsd->pitch;		/* usually set in GdConversionBlit*/
	parms.data_out = dstpsd->addr;
	parms.srcpsd = srcpsd;
	parms.src_xvirtres = srcpsd->xvirtres;	/* used in frameblit for src rotation*/
	parms.src_yvirtres = srcpsd->yvirtres;
	parms.x_denominator = x_denominator;	/* stretchblit invariant parms*/
	parms.y_denominator = y_denominator;

	/* We'll blit using destination clip rectangles, and offset the blit accordingly.
	 * Since the destination is already clipped, we only need to clip the source here.
	 */
#if DYNAMICREGIONS
	prc = clipregion->rects;
	count = clipregion->numRects;
#else
	prc = cliprects;
	count = clipcount;
#endif
	while (--count >= 0) {
		int rx1, rx2, ry1, ry2;
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
		/* Clip r1-r2 so it's inside c1-c2 */
		if (rx1 < cx1) rx1 = cx1;
		if (ry1 < cy1) ry1 = cy1;
		if (rx2 > cx2) rx2 = cx2;
		if (ry2 > cy2) ry2 = cy2;

		/* So we're drawing to destination rectangle (rx1, ry1) - (rx2, ry2)
		 * x_fraction = x_start_fraction + (rx1 - cx1)*x_numerator
		 * y_fraction = y_start_fraction + (ry1 - cy1)*y_numerator
		 */
		if (rx1 < rx2 && ry1 < ry2) {
			/* Source starting point (fraction).  Add half a pixel here so we're
			 * sampling from the middle of the pixel, not the top left corner.
	 		 */
			int x_fraction = (x_start_fraction + (rx1 - cx1) * x_numerator) + (x_numerator >> 1);
			int y_fraction = (y_start_fraction + (ry1 - cy1) * y_numerator) + (y_numerator >> 1);

			/* set src/dst starting points (pixel)*/
			parms.dstx = rx1;
			parms.dsty = ry1;
			parms.width = rx2 - rx1;
			parms.height = ry2 - ry1;
			parms.srcx = x_fraction / x_denominator;	/* seperate whole part from fraction*/
			parms.srcy = y_fraction / y_denominator;

			/* We need to do lots of comparisons to see if error values
	 		 * are >= x_denominator.  So subtract an extra x_denominator for speed,
	 		 * then we can just check if it's >= 0.
	 		 */
			parms.err_x = x_fraction - (parms.srcx + 1) * x_denominator;
			parms.err_y = y_fraction - (parms.srcy + 1) * y_denominator;

			/* calculate various deltas for fast blitter source stepping*/
			parms.src_x_step = x_numerator / x_denominator;
			parms.src_x_step_one = MWSIGN(x_numerator);
			parms.err_x_step = MWABS(x_numerator) - MWABS(parms.src_x_step) * x_denominator;
			parms.src_y_step = y_numerator / y_denominator;
			parms.src_y_step_one = MWSIGN(y_numerator);
			parms.err_y_step = MWABS(y_numerator) - MWABS(parms.src_y_step) * y_denominator;

			convblit(dstpsd, &parms);
		}
		++prc;
	}
	GdFixCursor(dstpsd);
	if (srcpsd != dstpsd)
		GdFixCursor(srcpsd);
}

/* call conversion blit with clipping and cursor fix*/
void
GdConvBlitInternal(PSD psd, PMWBLITPARMS gc, MWBLITFUNC convblit)
{
	MWCOORD dstx = gc->dstx;
	MWCOORD dsty = gc->dsty;
	MWCOORD width = gc->width;
	MWCOORD height = gc->height;
	MWCOORD srcx = gc->srcx;
	MWCOORD srcy = gc->srcy;
	int count, clipresult, checksrc;
#if DYNAMICREGIONS
	MWRECT *prc;
#else
	MWCLIPRECT *prc;
#endif

	/* check clipping region*/
	clipresult = GdClipArea(psd, dstx, dsty, dstx + width - 1, dsty + height - 1);
	if (clipresult == CLIP_INVISIBLE)
		return;

	/* check cursor in src region of both screen devices*/
	GdCheckCursor(psd, srcx, srcy, srcx + width - 1, srcy + height - 1);
	if ((checksrc = (gc->srcpsd != NULL && gc->srcpsd != psd)) != 0)
		GdCheckCursor(gc->srcpsd, srcx, srcy, srcx + width - 1, srcy + height - 1);

	if (clipresult == CLIP_VISIBLE) {
#if DEBUG_BLIT
GdSetFillMode(MWFILL_SOLID);
GdSetMode(MWROP_COPY);
GdSetForegroundColor(psd, MWRGB(128,64,0));	/* brown*/
GdFillRect(psd, gc->dstx, gc->dsty, gc->width, gc->height);
usleep(200000);
#endif
		convblit(psd, gc);
		GdFixCursor(psd);
		if (checksrc)
			GdFixCursor(gc->srcpsd);
		return;
	} else	/* partially clipped, check cursor in dst region once*/
		GdCheckCursor(psd, dstx, dsty, dstx + width - 1, dsty + height - 1);

	/* we'll traverse visible region and draw*/
#if DYNAMICREGIONS
	prc = clipregion->rects;
	count = clipregion->numRects;
#else
	prc = cliprects;
	count = clipcount;
#endif

	while (count-- > 0) {
		MWCOORD rx1, rx2, ry1, ry2, rw, rh;
#if DYNAMICREGIONS
		rx1 = prc->left;
		ry1 = prc->top;
		rx2 = prc->right;
		ry2 = prc->bottom;
#else
		rx1 = prc->x;		/* old clip-code*/
		ry1 = prc->y;
		rx2 = prc->x + prc->width;
		ry2 = prc->y + prc->height;
#endif

		/* Check if this rect intersects with the one we draw */
		if (rx1 < dstx) rx1 = dstx;
		if (ry1 < dsty) ry1 = dsty;
		if (rx2 > dstx + width) rx2 = dstx + width;
		if (ry2 > dsty + height) ry2 = dsty + height;

		rw = rx2 - rx1;
		rh = ry2 - ry1;

		if (rw > 0 && rh > 0) {
			gc->dstx = rx1;
			gc->dsty = ry1;
			gc->width = rw;
			gc->height = rh;
			gc->srcx = srcx + rx1 - dstx;
			gc->srcy = srcy + ry1 - dsty;
#if DEBUG_BLIT
GdSetFillMode(MWFILL_SOLID);
GdSetMode(MWROP_COPY);
GdSetForegroundColor(psd, MWRGB(128,64,0));	/* brown*/
GdFillRect(psd, gc->dstx, gc->dsty, gc->width, gc->height);
usleep(200000);
#endif
			convblit(psd, gc);
		}
		prc++;
	}
	GdFixCursor(psd);
	if (checksrc)
		GdFixCursor(gc->srcpsd);

	/* Reset everything, in case the caller re-uses it. */
	gc->dstx = dstx;
	gc->dsty = dsty;
	gc->width = width;
	gc->height = height;
	gc->srcx = srcx;
	gc->srcy = srcy;
}
