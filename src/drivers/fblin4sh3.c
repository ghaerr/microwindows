/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 4bpp Packed Linear SH3 Video Driver for Microwindows
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/* 
	This is modified for SH3 LittleEndian system.
	4bpp linear video driver, 
	Frame buffer sturcture and LCD Pixel is 

	32	           16 15		0
	-----------------------------------------    
	| P0 | P1 | P2 | P3 | P4 | P5 | P6 | P7 |
	-----------------------------------------    

 */

/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

static unsigned char notmask[2] = { 0x0f, 0xf0}; 

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear4_init(PSD psd)
{
	if (!psd->size)
		psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear4_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += ((x>>1)^3) + y * psd->linelen;
	if(gr_mode == MWMODE_XOR)
		*addr ^= c << ((1-(x&1))<<2);
	else
		*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear4_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[((x>>1)^3) + y * psd->linelen] >> ((1-(x&1))<<2) ) & 0x0f;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear4_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += ( (x1>>1)^3 ) + y * psd->linelen;
	if(gr_mode == MWMODE_XOR) {
		while(x1 <= x2) {
			*addr ^= c << ((1-(x1&1))<<2);
			if((++x1 & 1) == 0) {
			/* 	++addr;    */
			addr = psd->addr + ( (x1>>1)^3 ) + y * psd->linelen;
			}
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))<<2));
			if((++x1 & 1) == 0) {
			/* 	++addr;    */
			addr = psd->addr + ( (x1>>1)^3 ) + y * psd->linelen;
			}
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear4_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
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
	addr += ( (x>>1)^3 ) + y1 * linelen;
	if(gr_mode == MWMODE_XOR)
		while(y1++ <= y2) {
			*addr ^= c << ((1-(x&1))<<2);
			addr += linelen;
		}
	else
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
			addr += linelen;
		}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear4_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
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
#ifdef SH3_LITTLE_ENDIAN   /* not used */
	dst = dstpsd->addr + ( (dstx>>1)^3 ) + dsty * dlinelen;
	src = srcpsd->addr + ( (srcx>>1)^3 ) + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&1]) |
			   ((*s >> ((1-(sx&1))<<2) & 0x0f) << ((1-(dx&1))<<2));
			if((++dx & 1) == 0)
				++d;
			if((++sx & 1) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
#else
	dst = dstpsd->addr + dsty * dlinelen;
	src = srcpsd->addr + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst + ( (dstx>>1)^3 );
		ADDR8	s = src + ( (srcx>>1)^3 );
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&1]) |
			   ((*s >> ((1-(sx&1))<<2) & 0x0f) << ((1-(dx&1))<<2));
			if((++dx & 1) == 0)
				d = dst + ( (dx >> 1)^3 );
			if((++sx & 1) == 0)
				s = src + ( (sx >> 1)^3 );
		}
		dst += dlinelen;
		src += slinelen;
	}
#endif
	DRAWOFF;
}

SUBDRIVER fblinear4 = {
	linear4_init,
	linear4_drawpixel,
	linear4_readpixel,
	linear4_drawhorzline,
	linear4_drawvertline,
	gen_fillrect,
	linear4_blit
};
