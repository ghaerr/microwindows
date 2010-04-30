/*
 * Copyright (c) 1999-2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * 1bpp Packed Linear Video Driver for Microwindows (MSB first bit order)
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

/* This driver doesn't have full drawing mode functionality using
 * the applyOp() macro from fb.h
 */

static const unsigned char notmask[8] = {
	0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear1_init(PSD psd)
{
	if (!psd->size)
		psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear1_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register ADDR8 addr = ((ADDR8)psd->addr) + (x>>3) + y * psd->linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWMODE_XOR)
		*addr ^= c << (7-(x&7));
	else
		*addr = (*addr & notmask[x&7]) | (c << (7-(x&7)));
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear1_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return (((ADDR8)psd->addr)[(x>>3) + y * psd->linelen] >> (7-(x&7)) ) & 0x01;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear1_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register ADDR8 addr = ((ADDR8)psd->addr) + (x1>>3) + y * psd->linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWMODE_XOR) {
		while(x1 <= x2) {
			*addr ^= c << (7-(x1&7));
			if((++x1 & 7) == 0)
				++addr;
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&7]) | (c << (7-(x1&7)));
			if((++x1 & 7) == 0)
				++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear1_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	linelen = psd->linelen;
	register ADDR8 addr = ((ADDR8)psd->addr) + (x>>3) + y1 * linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWMODE_XOR)
		while(y1++ <= y2) {
			*addr ^= c << (7-(x&7));
			addr += linelen;
		}
	else
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&7]) | (c << (7-(x&7)));
			addr += linelen;
		}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear1_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8	dst, src;
	int		i;
	int		dlinelen = dstpsd->linelen;
	int		slinelen = srcpsd->linelen;

	assert (dstpsd->addr != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	/* src is MSB 1bpp, dst is MSB 1bpp*/
	dst = ((ADDR8)dstpsd->addr) + (dstx>>3) + dsty * dlinelen;
	src = ((ADDR8)srcpsd->addr) + (srcx>>3) + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&7]) | ((*s >> (7-(sx&7)) & 0x01) << (7-(dx&7)));
			if((++dx & 7) == 0)
				++d;
			if((++sx & 7) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
	DRAWOFF;
}

#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
/* psd->DrawArea operation PSDOP_BITMAP_BYTES_MSB_FIRST which
 * takes a pixmap, each line is byte aligned, and copies it
 * to the screen using fg_color and bg_color to replace a 1
 * and 0 in the pixmap.  
 *
 * The bitmap is ordered how you'd expect, with the MSB used
 * for the leftmost of the 8 pixels controlled by each byte.
 *
 * Variables used in the gc:
 *       dstx, dsty, dsth, dstw   Destination rectangle
 *       srcx, srcy               Source rectangle
 *       src_linelen              Linesize in bytes of source
 *       pixels                   Pixmap data
 *       fg_color                 Color of a '1' bit
 *       bg_color                 Color of a '0' bit
 *       gr_usebg                 If set, bg_color is used.  If zero,
 *                                then '0' bits are transparentz.
 */
static void
linear1_drawarea_bitmap_bytes_msb_first(PSD psd, driver_gc_t * gc)
{
	ADDR8	dst, src;
	int		i;
	int		dlinelen = psd->linelen;
	int		slinelen = gc->src_linelen;
	MWCOORD	h = gc->dsth;
	MWCOORD	w = gc->dstw;
	MWPIXELVAL fg = gc->fg_color;
	MWPIXELVAL bg = gc->bg_color;
#if DEBUG
	assert (psd->addr != 0);
	assert (gc->dstx >= 0 && gc->dstx < psd->xres);
	assert (gc->dsty >= 0 && gc->dsty < psd->yres);
	assert (gc->dstw > 0);
	assert (gc->dsth > 0);
	assert (gc->dstx+w <= psd->xres);
	assert (gc->dsty+h <= psd->yres);
#endif
	DRAWON;
	/* src is MSB 1bpp, dst is MSB 1bpp*/
	dst = ((ADDR8)psd->addr) + (gc->dstx>>3) + gc->dsty * dlinelen;
	src = ((ADDR8)gc->pixels) + (gc->srcx>>3) + gc->srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = gc->dstx;
		MWCOORD	sx = gc->srcx;

		if (gc->gr_usebg) {
			for(i=0; i<w; ++i) {
				if ((*s >> (7-(sx&7))) & 01)
					*d = (*d & notmask[dx&7]) | (fg << (7-(dx&7)));
				else
					*d = (*d & notmask[dx&7]) | (bg << (7-(dx&7)));
				if((++dx & 7) == 0)
					++d;
				if((++sx & 7) == 0)
					++s;
			}
		} else {
			for(i=0; i<w; ++i) {
				if ((*s >> (7-(sx&7))) & 01)
					*d = (*d & notmask[dx&7]) | (fg << (7-(dx&7)));
				if((++dx & 7) == 0)
					++d;
				if((++sx & 7) == 0)
					++s;
			}
		}
		dst += dlinelen;
		src += slinelen;
	}
	DRAWOFF;
}
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST */

static void
linear1_drawarea(PSD psd, driver_gc_t * gc, int op)
{
#if DEBUG
	assert(psd->addr != 0);
	/*assert(gc->dstw <= gc->srcw); */
	assert(gc->dstx >= 0 && gc->dstx + gc->dstw <= psd->xres);
	/*assert(gc->dsty >= 0 && gc->dsty+gc->dsth <= psd->yres); */
	/*assert(gc->srcx >= 0 && gc->srcx+gc->dstw <= gc->srcw); */
	assert(gc->srcy >= 0);
	/*DPRINTF("linear1_drawarea op=%d dstx=%d dsty=%d\n", op, gc->dstx, gc->dsty);*/
#endif
	switch (op) {
#if MW_FEATURE_PSDOP_ALPHACOL
	case PSDOP_ALPHACOL:
		DPRINTF("linear1_drawarea: PSDOP_ALPHACOL not supported\n");
		break;
#endif
	
#if MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST
	case PSDOP_BITMAP_BYTES_LSB_FIRST:
		DPRINTF("linear1_drawarea: PSDOP_BITMAP_BYTES_LSB_FIRST not supported\n");
		break;
#endif
	
#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
	case PSDOP_BITMAP_BYTES_MSB_FIRST:
		linear1_drawarea_bitmap_bytes_msb_first(psd, gc);
		break;
#endif
	
	  }
}

SUBDRIVER fblinear1 = {
	linear1_init,
	linear1_drawpixel,
	linear1_readpixel,
	linear1_drawhorzline,
	linear1_drawvertline,
	gen_fillrect,
	linear1_blit,
	linear1_drawarea
};
