/*
 * Copyright (c) 1999, 2010 Greg Haerr <greg@censoft.com>
 *
 * 2bpp Packed Linear Video Driver for Microwindows (LSB first bit order)
 * For Psion S5
 */
/*#define NDEBUG*/
#include <assert.h>
#include <stdlib.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"

static const unsigned char notmask[4] = { 0xfc, 0xf3, 0xcf, 0x3f };

/* Set pixel at x, y, to pixelval c*/
static void
linear2_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x >> 2);
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_XOR)
		*addr ^= c << ((x&3)<<1);
	else
		*addr = (*addr & notmask[x&3]) | (c << ((x&3)<<1));
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear2_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x >> 2);
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return ( *addr >> ((x&3)<<1) ) & 0x03;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear2_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x1 >> 2);
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
			*addr ^= c << ((x1&3)<<1);
			if((++x1 & 3) == 0)
				++addr;
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&3]) | (c << ((x1&3)<<1));
			if((++x1 & 3) == 0)
				++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear2_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	pitch = psd->pitch;
	register unsigned char *addr = psd->addr + y1 * pitch + (x >> 2);
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
			*addr ^= c << ((x&3)<<1);
			addr += pitch;
		}
	else
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&3]) | (c << ((x&3)<<1));
			addr += pitch;
		}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear2_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	int		i;
	int		dpitch = dstpsd->pitch;
	int		spitch = srcpsd->pitch;
	/* src is LSB 2bpp, dst is LSB 2bpp*/
	ADDR8 dst = ((ADDR8)dstpsd->addr) + (dstx>>2) + dsty * dpitch;
	ADDR8 src = ((ADDR8)srcpsd->addr) + (srcx>>2) + srcy * spitch;
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
			*d = (*d & notmask[dx&3]) | ((*s >> ((sx&3)<<1) & 0x03) << ((dx&3)<<1));
			if((++dx & 3) == 0)
				++d;
			if((++sx & 3) == 0)
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
linear2_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc)
{
	int		i;
	int		dpitch = gc->dst_pitch;
	int		spitch = gc->src_pitch;
	/* src is MSB 1bpp, dst is LSB 2bpp*/
	ADDR8 dst = ((ADDR8)gc->data_out) + (gc->dstx>>2) + gc->dsty * dpitch;
	ADDR8 src = ((ADDR8)gc->data) + (gc->srcx>>2) + gc->srcy * spitch;
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
				/* check if src bit set, write fg color else bg color*/
				if ((*s >> (7-(sx&7))) & 01)
					*d = (*d & notmask[dx&3]) | (fg << ((dx&3)<<1));
				else
					*d = (*d & notmask[dx&3]) | (bg << ((dx&3)<<1));
				if((++dx & 3) == 0)
					++d;
				if((++sx & 7) == 0)
					++s;
			}
		} else {
			for(i=0; i<w; ++i) {
				/* check if src bit set, and write fg color*/
				if ((*s >> (7-(sx&7))) & 01)
					*d = (*d & notmask[dx&3]) | (fg << ((dx&3)<<1));
				if((++dx & 3) == 0)
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

static SUBDRIVER fblinear2_none = {
	linear2_drawpixel,
	linear2_readpixel,
	linear2_drawhorzline,
	linear2_drawvertline,
	gen_fillrect,
	linear2_blit,
	NULL,		/* FrameBlit*/
	NULL,		/* FrameStretchBlit*/
	linear2_convblit_copy_mask_mono_byte_msb,
	NULL,		/* BlitCopyMaskMonoByteLSB*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	NULL,		/* BlitBlendMaskAlphaByte*/
	NULL,		/* BlitCopyRGBA8888*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL		/* BlitCopyRGB888*/
};

PSUBDRIVER fblinear2[4] = {
	&fblinear2_none, &fbportrait_left, &fbportrait_right, &fbportrait_down
};
