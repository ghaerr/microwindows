/*
 * Copyright (c) 1999-2001 Greg Haerr <greg@censoft.com>
 *
 * 1bpp Packed Linear Video Driver for Microwindows
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

/* This file doesn't have full drawing mode functionality using
 * the applyOp() macro from fb.h
 */

static unsigned char notmask[8] = {
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
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x>>3) + y * psd->linelen;
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
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[(x>>3) + y * psd->linelen] >> (7-(x&7)) ) & 0x01;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear1_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x1>>3) + y * psd->linelen;
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
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x>>3) + y1 * linelen;
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
	ADDR8	dst;
	ADDR8	src;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;

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
	dst = dstpsd->addr + (dstx>>3) + dsty * dlinelen;
	src = srcpsd->addr + (srcx>>3) + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&7]) |
			   ((*s >> (7-(sx&7)) & 0x01) << (7-(dx&7)));
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

SUBDRIVER fblinear1 = {
	linear1_init,
	linear1_drawpixel,
	linear1_readpixel,
	linear1_drawhorzline,
	linear1_drawvertline,
	gen_fillrect,
	linear1_blit
};
