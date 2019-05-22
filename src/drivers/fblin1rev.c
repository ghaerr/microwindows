/*
 * Copyright (c) 1999, 2010 Greg Haerr <greg@censoft.com>
 *
 * 1bpp Packed Linear Video Driver for Microwindows
 * Writes MWIF_MONOBYTELSB data format (LSB first bit order)
 *
 * For Psion S5
 */
/*#define NDEBUG*/
#include <assert.h>
#include <stdlib.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"

/* This driver doesn't have full drawing mode functionality using
 * the applyOp() macro from fb.h
 */

static const unsigned char notmask[8] = {
	0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};

/* Set pixel at x, y, to pixelval c*/
static void
linear1_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x >> 3);
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_XOR)
		*addr ^= c << (x&7);
	else
		*addr = (*addr & notmask[x&7]) | (c << (x&7));
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear1_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x >> 3);
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return ( *addr >> (x&7) ) & 0x01;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear1_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x1 >> 3);
#if DEBUG
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_XOR) {
		while(x1 <= x2) {
			*addr ^= c << (x1&7);
			if((++x1 & 7) == 0)
				++addr;
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&7]) | (c << (x1&7));
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
	int	pitch = psd->pitch;
	register unsigned char *addr = psd->addr + y1 * pitch + (x >> 3);
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_XOR)
		while(y1++ <= y2) {
			*addr ^= c << (x&7);
			addr += pitch;
		}
	else
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&7]) | (c << (x&7));
			addr += pitch;
		}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear1_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	int		i;
	int		dpitch = dstpsd->pitch;
	int		spitch = srcpsd->pitch;
	/* src is LSB 1bpp, dst is LSB 1bpp*/
	ADDR8 dst = ((ADDR8)dstpsd->addr) + (dstx>>3) + dsty * dpitch;
	ADDR8 src = ((ADDR8)srcpsd->addr) + (srcx>>3) + srcy * spitch;
#if DEBUG
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);
#endif
	DRAWON;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;

		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&7]) | ((*s >> (sx&7) & 0x01) << (dx&7));
			if((++dx & 7) == 0)
				++d;
			if((++sx & 7) == 0)
				++s;
		}
		dst += dpitch;
		src += spitch;
	}
	DRAWOFF;
}

/*
 * Routine to draw mono 1bpp MSBFirst bitmap to LSB 1bpp
 * Bitmap is byte array.
 *
 * Used to draw FT2 non-antialiased glyphs.
 */
static void
linear1_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc)
{
	int		i;
	int		dpitch = gc->dst_pitch;
	int		spitch = gc->src_pitch;
	/* src is MSB 1bpp, dst is LSB 1bpp*/
	ADDR8 dst = ((ADDR8)psd->addr) + (gc->dstx>>3) + gc->dsty * dpitch;
	ADDR8 src = ((ADDR8)gc->data) + (gc->srcx>>3) + gc->srcy * spitch;
	MWCOORD	h = gc->height;
	MWCOORD	w = gc->width;
	MWPIXELVAL fg = gc->fg_pixelval;
	MWPIXELVAL bg = gc->bg_pixelval;
#if DEBUG
	assert (gc->dstx >= 0 && gc->dstx < psd->xres);
	assert (gc->dsty >= 0 && gc->dsty < psd->yres);
	assert (gc->width > 0);
	assert (gc->height > 0);
	assert (gc->dstx+w <= psd->xres);
	assert (gc->dsty+h <= psd->yres);
#endif
	DRAWON;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = gc->dstx;
		MWCOORD	sx = gc->srcx;

		if (gc->usebg) {
			for(i=0; i<w; ++i) {
				if ((*s >> (7-(sx&7))) & 01)
					*d = (*d & notmask[dx&7]) | (fg << (dx&7));
				else
					*d = (*d & notmask[dx&7]) | (bg << (dx&7));
				if((++dx & 7) == 0)
					++d;
				if((++sx & 7) == 0)
					++s;
			}
		} else {
			for(i=0; i<w; ++i) {
				if ((*s >> (7-(sx&7))) & 01)
					*d = (*d & notmask[dx&7]) | (fg << (dx&7));
				//*d = (*d & notmask[dx&7]) | ((sb >> (7 - (sx&7)) & 0x01) << (dx&7));
				if((++dx & 7) == 0)
					++d;
				if((++sx & 7) == 0)
					++s;
			}
		}
		dst += dpitch;
		src += spitch;
	}
	DRAWOFF;
}

static SUBDRIVER fblinear1_none = {
	linear1_drawpixel,
	linear1_readpixel,
	linear1_drawhorzline,
	linear1_drawvertline,
	gen_fillrect,
	linear1_blit,
	NULL,		/* FrameBlit*/
	NULL,		/* FrameStretchBlit*/
	linear1_convblit_copy_mask_mono_byte_msb,
	NULL,		/* BlitCopyMaskMonoByteLSB*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	NULL,		/* BlitBlendMaskAlphaByte*/
	NULL,		/* BlitCopyRGBA8888*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL		/* BlitCopyRGB888*/
};

PSUBDRIVER fblinear1[4] = {
	&fblinear1_none, &fbportrait_left, &fbportrait_right, &fbportrait_down
};
