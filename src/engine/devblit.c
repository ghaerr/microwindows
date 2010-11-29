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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "swap.h"
#include "device.h"
#include "convblit.h"

extern MWPIXELVAL gr_foreground;      /* current foreground color */
extern MWPIXELVAL gr_background;      /* current background color */
extern MWBOOL 	  gr_usebg;    	      /* TRUE if background drawn in pixmaps */

/* call conversion blit with clipping and cursor fix*/
void
GdConvBlitInternal(PSD psd, PMWBLITPARMS gc, MWBLITFUNC convblit)
{
	MWCOORD x = gc->dstx;
	MWCOORD y = gc->dsty;
	MWCOORD width = gc->width;
	MWCOORD height = gc->height;
	MWCOORD srcx, srcy;
	MWCOORD rx1, rx2, ry1, ry2, rw, rh;
	int count;
#if DYNAMICREGIONS
	MWRECT *prc;
	extern MWCLIPREGION *clipregion;
#else
	MWCLIPRECT *prc;
	extern MWCLIPRECT cliprects[];
	extern int clipcount;
#endif

	/* check clipping region*/
	switch(GdClipArea(psd, x, y, x + width - 1, y + height - 1)) {
	case CLIP_VISIBLE:
		convblit(psd, gc);
		GdFixCursor(psd);
		return;

	case CLIP_INVISIBLE:
		return;
	}

	/* partially clipped, we'll traverse visible region and draw*/
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
		/* old clip-code*/
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
			gc->width = rw;
			gc->height = rh;
			gc->srcx = srcx + rx1 - x;
			gc->srcy = srcy + ry1 - y;
			GdCheckCursor(psd, rx1, ry1, rx2 - 1, ry2 - 1);
			convblit(psd, gc);
		}
		prc++;
	}
	GdFixCursor(psd);

	/* Reset everything, in case the caller re-uses it. */
	gc->dstx = x;
	gc->dsty = y;
	gc->width = width;
	gc->height = height;
	gc->srcx = srcx;
	gc->srcy = srcy;
}

/* find a conversion blit based on data format and blit op*/
MWBLITFUNC
GdFindConvBlit(PSD psd, int data_format, int op)
{
	MWBLITFUNC convblit = NULL;

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
		if (psd->data_format == data_format)
			convblit = convblit_copy_16bpp_16bpp;	/* 16bpp to 16bpp copy*/
		break;
	}

	return convblit;
}

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
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int32_t rop)
{
	int rx1, rx2, ry1, ry2;
	int px1, px2, py1, py2;
	int pw, ph;
	int count;
#if DYNAMICREGIONS
	MWRECT *	prc;
	extern MWCLIPREGION *clipregion;
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
	
//printf("GdBlit %d,%d %d,%d\n", dstx, dsty, width, height);
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
			GdCheckCursor(srcpsd, srcx, srcy, srcx+width-1, srcy+height-1);
		dstpsd->Blit(dstpsd, dstx, dsty, width, height, srcpsd, srcx, srcy, rop);
		GdFixCursor(dstpsd);
		if (dstpsd != srcpsd)
			GdFixCursor(srcpsd);
		return;

	case CLIP_INVISIBLE:
printf("GdBlit invis\n");
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
			GdCheckCursor(dstpsd, srcx, srcy, srcx+width, srcy+height);
			dstpsd->Blit(dstpsd, px1, py1, pw, ph, srcpsd,
				srcx + (px1-dstx), srcy + (py1-dsty), rop);
		}
		++prc;
	}
	GdFixCursor(dstpsd);
}

#if 0000 /* THIS FUNCTION IS DEPCRECATED, USE GdStretchBlitEx*/
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
	MWCOORD srch, int32_t rop)
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
				dstx + dstw - 1, dsty + dsth - 1,
				srcpsd, srcx, srcy,
				srcx + srcw - 1, srcy + srch - 1,
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
#endif /* DEPRECATED*/

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
	MWCOORD s2_y, int32_t rop)
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
		register MWCOORD tmp = d2_x;
		d2_x = d1_x;
		d1_x = tmp;
		tmp = s2_x;
		s2_x = s1_x;
		s1_x = tmp;
	}

	if (d1_y > d2_y) {
		register MWCOORD tmp = d2_y;
		d2_y = d1_y;
		d1_y = tmp;
		tmp = s2_y;
		s2_y = s1_y;
		s1_y = tmp;
	}

	if ((d2_x < 0) || (d2_y < 0)
	    || (d1_x > dstpsd->xvirtres)
	    || (d1_y > dstpsd->yvirtres)
	    || (d1_x == d2_x)
	    || (d1_y == d2_y)) {
		/* Destination rectangle is entirely off screen, or is zero-sized*/
		/* DPRINTF("Nano-X: GdStretchBlitEx: CLIPPED OFF (dest rect offscreen or 0)\n"); */
		return;
	}

	/* If we're not stretching or flipping, use the standard blit (faster)*/
	if ((d2_x - d1_x == s2_x - s1_x) && (d2_y - d1_y == s2_y - s1_y)) {
		GdBlit(dstpsd, d1_x, d1_y, d2_x - d1_x, d2_y - d1_y, srcpsd, s1_x, s1_y, rop);
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
		int i2_x = d1_x + ((srcpsd->xvirtres - s1_x) * src_x_step_denominator + src_x_step_denominator - 1)
			/ src_x_step_numerator;

		/* Since we may be doing a flip, "left" and "right" in the statements
		 * above do not necessarily correspond to "left" and "right" in the
		 * destination image, which is where we're clipping.  So sort the
		 * X co-ordinates.
		 */
		if (i1_x > i2_x) {
			register int temp = i1_x;
			i1_x = i2_x;
			i2_x = temp;
		}

		/* Check for total invisibility */
		if (c2_x < i1_x || c1_x > i2_x) {
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
		int i2_y = d1_y + ((srcpsd->yvirtres - s1_y) * src_y_step_denominator + src_y_step_denominator - 1)
			/ src_y_step_numerator;

		/* Since we may be doing a flip, "top" and bottom" in the statements
		 * above do not necessarily correspond to "top" and bottom" in the
		 * destination image, which is where we're clipping.  So sort the
		 * Y co-ordinates.
		 */
		if (i1_y > i2_y) {
			register int temp = i1_y;
			i1_y = i2_y;
			i2_y = temp;
		}

		/* Check for total invisibility */
		if (c2_y < i1_y || c1_y > i2_y) {
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
	if ((c1_x >= c2_x) || (c1_y >= c2_y)) {
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
	src_x_start_exact = s1_x * src_x_step_denominator + (c1_x - d1_x) * src_x_step_numerator;
	src_y_start_exact = s1_y * src_y_step_denominator + (c1_y - d1_y) * src_y_step_numerator;

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
		dstpsd->StretchBlitEx(dstpsd, srcpsd,
					c1_x, c1_y,
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
			GdCheckCursor(dstpsd, r1_x, r1_y, r2_x - 1, r2_y - 1);
			dstpsd->StretchBlitEx(dstpsd, srcpsd,
						r1_x, r1_y,
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

#if DEBUG
void GdPrintBitmap(PMWBLITPARMS gc, int SSZ)
{
	unsigned char *src;
	int height;
	unsigned int v;

	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + gc->srcx * SSZ;

	printf("Image %d,%d SSZ %d\n", gc->width, gc->height, SSZ);
	height = gc->height;
	while (--height >= 0)
	{
		register unsigned char *s = src;
		int w = gc->width;

		while (--w >= 0)
		{
			switch (SSZ) {
			case 2:
				v = s[0] | (s[1] << 8);
				v = PIXEL565RED(v) + PIXEL565GREEN(v) + PIXEL565BLUE(v);
				printf("%c", "_.:;oVM@X"[v]);
				break;
			case 3:
				v = (s[0] + s[1] + s[2]) / 3;
				printf("%c", "_.:;oVM@X"[v >> 5]);
				break;
			case 4:
				//if (s[4])
					v = (s[0] + s[1] + s[2]) / 3;
				//else v = 256;
				printf("%c", "_.:;oVM@X"[v >> 5]);
				break;
			}
			s += SSZ;				/* src: next pixel right*/
		}
		printf("\n");
		src += gc->src_pitch;		/* src: next line down*/
	}
}
#endif
