/*
 * Copyright (c) 2000, 2001 Greg Haerr <greg@censoft.com>
 *
 * 24bpp Linear Video Driver for Microwindows
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear24_init(PSD psd)
{
	if (!psd->size) {
		psd->size = psd->yres * psd->linelen;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32*/
		psd->linelen /= 3;
	}
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear24_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	MWUCHAR	r, g, b;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);
	addr += (x + y * psd->linelen) * 3;
	DRAWON;
	if(gr_mode == MWMODE_COPY) {
		*addr++ = b;
		*addr++ = g;
		*addr = r;
	} else {
		applyOp(gr_mode, b, addr, ADDR8); ++addr;
		applyOp(gr_mode, g, addr, ADDR8); ++addr;
		applyOp(gr_mode, r, addr, ADDR8);
	}
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear24_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	addr += (x + y * psd->linelen) * 3;
	return RGB2PIXEL888(addr[2], addr[1], addr[0]);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear24_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	MWUCHAR	r, g, b;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);
	addr += (x1 + y * psd->linelen) * 3;
	DRAWON;
	if(gr_mode == MWMODE_COPY) {
		while(x1++ <= x2) {
			*addr++ = b;
			*addr++ = g;
			*addr++ = r;
		}
	} else {
		while (x1++ <= x2) {
			applyOp(gr_mode, b, addr, ADDR8); ++addr;
			applyOp(gr_mode, g, addr, ADDR8); ++addr;
			applyOp(gr_mode, r, addr, ADDR8); ++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear24_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen * 3;
	MWUCHAR	r, g, b;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);
	addr += (x + y1 * psd->linelen) * 3;
	DRAWON;
	if(gr_mode == MWMODE_COPY) {
		while(y1++ <= y2) {
			addr[0] = b;
			addr[1] = g;
			addr[2] = r;
			addr += linelen;
		}
	} else {
		while (y1++ <= y2) {
			applyOp(gr_mode, b, &addr[0], ADDR8);
			applyOp(gr_mode, g, &addr[1], ADDR8);
			applyOp(gr_mode, r, &addr[2], ADDR8);
			addr += linelen;
		}
	}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear24_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8	dst = dstpsd->addr;
	ADDR8	src = srcpsd->addr;
	int	i;
	int	dlinelen = dstpsd->linelen * 3;
	int	slinelen = srcpsd->linelen * 3;
	int	dlinelen_minus_w = (dstpsd->linelen - w) * 3;
	int	slinelen_minus_w = (srcpsd->linelen - w) * 3;
#if ALPHABLEND
	unsigned int alpha;
#endif

	assert (dst != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (src != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	dst += (dstx + dsty * dstpsd->linelen) * 3;
	src += (srcx + srcy * srcpsd->linelen) * 3;

#if ALPHABLEND
	if((op & MWROP_EXTENSION) != MWROP_BLENDCONSTANT)
		goto stdblit;
	alpha = op & 0xff;

	while(--h >= 0) {
		for(i=0; i<w; ++i) {
			unsigned long s = *src++;
			unsigned long d = *dst;
			*dst++ = (unsigned char)(((s - d)*alpha)>>8) + d;
			s = *src++;
			d = *dst;
			*dst++ = (unsigned char)(((s - d)*alpha)>>8) + d;
			s = *src++;
			d = *dst;
			*dst++ = (unsigned char)(((s - d)*alpha)>>8) + d;
		}
		dst += dlinelen_minus_w;
		src += slinelen_minus_w;
	}
	DRAWOFF;
	return;
stdblit:
#endif

	if (op == MWROP_COPY) {
		/* copy from bottom up if dst in src rectangle*/
		/* memmove is used to handle x case*/
		if (srcy < dsty) {
			src += (h-1) * slinelen;
			dst += (h-1) * dlinelen;
			slinelen *= -1;
			dlinelen *= -1;
		}
		while(--h >= 0) {
			/* a _fast_ memcpy is a _must_ in this routine*/
			memmove(dst, src, w*3);
			dst += dlinelen;
			src += slinelen;
		}
	} else {
		for(i=w*3; i>=0; --i) {
			applyOp(MWROP_TO_MODE(op), *src, dst, ADDR8);
			++src;
			++dst;
		}
		dst += dlinelen_minus_w;
		src += slinelen_minus_w;
	}
	DRAWOFF;
}

/* srccopy stretchblt*/
static void
linear24_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, long op)
{
	ADDR8	dst;
	ADDR8	src;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	i, ymax;
	int	row_pos, row_inc;
	int	col_pos, col_inc;
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	assert (dstpsd->addr != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (dstw > 0);
	assert (dsth > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (srcw > 0);
	assert (srch > 0);
	assert (dstx+dstw <= dstpsd->xres);
	assert (dsty+dsth <= dstpsd->yres);
	assert (srcx+srcw <= srcpsd->xres);
	assert (srcy+srch <= srcpsd->yres);

	DRAWON;
	row_pos = 0x10000;
	row_inc = (srch << 16) / dsth;

	/* stretch blit using integer ratio between src/dst height/width*/
	for (ymax = dsty+dsth; dsty<ymax; ++dsty) {

		/* find source y position*/
		while (row_pos >= 0x10000L) {
			++srcy;
			row_pos -= 0x10000L;
		}

		dst = dstpsd->addr + (dstx + dsty*dlinelen) * 3;
		src = srcpsd->addr + (srcx + (srcy-1)*slinelen) * 3;

		/* copy a row of pixels*/
		col_pos = 0x10000;
		col_inc = (srcw << 16) / dstw;
		for (i=0; i<dstw; ++i) {
			/* get source x pixel*/
			while (col_pos >= 0x10000L) {
				b = *src++;
				g = *src++;
				r = *src++;
				col_pos -= 0x10000L;
			}
			*dst++ = b;
			*dst++ = g;
			*dst++ = r;
			col_pos += col_inc;
		}

		row_pos += row_inc;
	}
	DRAWOFF;
}

SUBDRIVER fblinear24 = {
	linear24_init,
	linear24_drawpixel,
	linear24_readpixel,
	linear24_drawhorzline,
	linear24_drawvertline,
	gen_fillrect,
	linear24_blit,
	NULL,
	linear24_stretchblit
};
