/*
 * Copyright (c) 1999, 2000, 2002 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2002 Alex Holden <alex@alexholden.net>
 *
 * 4 planes EGA/VGA Memory (blitting) Video Driver for Microwindows
 * Included with #define HAVEBLIT in vgaplan4.h
 * 
 * 4bpp colors are stored in linear 4pp format in memory dc's
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "device.h"
#include "vgaplan4.h"
#include "fb.h"

#if HAVEBLIT

#if MSDOS | ELKS | __rtems__
/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENBASE(psd) 	EGA_BASE
#define BYTESPERLINE(psd)	80
#else
/* run on top of framebuffer*/
#define SCREENBASE(psd) 	((char *)((psd)->addr))
#define BYTESPERLINE(psd)	((psd)->linelen)
#endif

/* extern data*/
extern int gr_mode;	/* temp kluge*/

static unsigned char notmask[2] = { 0x0f, 0xf0};
static unsigned char mask[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/* Calc linelen and mmap size, return 0 on fail*/
static int
mempl4_init(PSD psd)
{
	if (!psd->size)
		psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
mempl4_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	addr += (x>>1) + y * psd->linelen;
	if(gr_mode == MWMODE_XOR)
		*addr ^= c << ((1-(x&1))<<2);
	else
		*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
}

/* Read pixel at x, y*/
static MWPIXELVAL
mempl4_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[(x>>1) + y * psd->linelen] >> ((1-(x&1))<<2) ) & 0x0f;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
mempl4_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	addr += (x1>>1) + y * psd->linelen;
	if(gr_mode == MWMODE_XOR) {
		while(x1 <= x2) {
			*addr ^= c << ((1-(x1&1))<<2);
			if((++x1 & 1) == 0)
				++addr;
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))<<2));
			if((++x1 & 1) == 0)
				++addr;
		}
	}
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
mempl4_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	addr += (x>>1) + y1 * linelen;
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
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from memdc to memdc*/
static void
mempl4_to_mempl4_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
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

	dst = (char *)dstpsd->addr + (dstx>>1) + dsty * dlinelen;
	src = (char *)srcpsd->addr + (srcx>>1) + srcy * slinelen;
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
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from vga memory to vga memory, psd's ignored for speed*/
static void
vga_to_vga_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	FARADDR	dst;
	FARADDR	src;
	int	i, plane;
	int	x1, x2;

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

	DRAWON;
	set_enable_sr(0);
	dst = SCREENBASE(dstpsd) + (dstx>>3) + dsty * BYTESPERLINE(dstpsd);
	src = SCREENBASE(srcpsd) + (srcx>>3) + srcy * BYTESPERLINE(srcpsd);
	x1 = dstx>>3;
	x2 = (dstx + w - 1) >> 3;
	while(--h >= 0) {
		for(plane=0; plane<4; ++plane) {
			FARADDR d = dst;
			FARADDR s = src;

	    		set_read_plane(plane);
			set_write_planes(1 << plane);

			/* FIXME: only works if srcx and dstx are same modulo*/
			if(x1 == x2) {
		  		select_and_set_mask((0xff >> (x1 & 7)) & (0xff << (7 - (x2 & 7))));
				PUTBYTE_FP(d, GETBYTE_FP(s));
			} else {
				select_and_set_mask(0xff >> (x1 & 7));
				PUTBYTE_FP(d++, GETBYTE_FP(s++));

				set_mask(0xff);
		  		for(i=x1+1; i<x2; ++i)
					PUTBYTE_FP(d++, GETBYTE_FP(s++));

		  		set_mask(0xff << (7 - (x2 & 7)));
				PUTBYTE_FP(d, GETBYTE_FP(s));
			}
		}
		dst += BYTESPERLINE(dstpsd);
		src += BYTESPERLINE(srcpsd);
	}
	set_write_planes(0x0f);
	set_enable_sr(0x0f);
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from memdc to vga memory*/
static void
mempl4_to_vga_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	FARADDR	dst;
	ADDR8	src;
	int	i;
	int	slinelen = srcpsd->linelen;
	int	color, lastcolor = -1;

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
	set_op(0);		/* modetable[MWMODE_COPY]*/
	dst = SCREENBASE(dstpsd) + (dstx>>3) + dsty * BYTESPERLINE(dstpsd);
	src = (char *)srcpsd->addr + (srcx>>1) + srcy * slinelen;
	while(--h >= 0) {
		FARADDR d = dst;
		ADDR8	s = src;
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		for(i=0; i<w; ++i) {
			color = *s >> ((1-(sx&1))<<2) & 0x0f;
			if(color != lastcolor)
				set_color(lastcolor = color);
			select_and_set_mask (mask[dx&7]);
			RMW_FP(d);

			if((++dx & 7) == 0)
				++d;
			if((++sx & 1) == 0)
				++s;
		}
		dst += BYTESPERLINE(dstpsd);
		src += slinelen;
	}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from vga memory to memdc*/
static void
vga_to_mempl4_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	int x, y;
	int plane;
	ADDR8 d, dst;
	FARADDR s, src;
	MWCOORD sx, dx;
	unsigned char color;
	int dlinelen = dstpsd->linelen;

	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (dstpsd->addr != 0);
	assert (w > 0);
	assert (h > 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	src = SCREENBASE(srcpsd) + (srcx >> 3) + srcy * BYTESPERLINE(srcpsd);
	dst = (char *)dstpsd->addr + (dstx >> 1) + dsty * dlinelen;

	for(y = 0; y < h; y++) {
		s = src;
		d = dst;
		dx = dstx;
		sx = srcx;
		color = 0;
		for(x = 0; x < w; x++) {
			for(plane = 0; plane < 4; ++plane) {
				set_read_plane(plane);
				if(GETBYTE_FP(s) & mask[sx & 7])
					color |= 1 <<
						(plane + ((dx & 1) ? 0 : 4));
			}
			if((++sx & 7) == 0) ++s;
			if((++dx & 1) == 0) {
				*d++ = color;
				color = 0;
			}
		}
		dst += dlinelen;
		src += BYTESPERLINE(srcpsd);
	}
	DRAWOFF;
}

void
ega_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	MWBOOL	srcvga, dstvga;

	/* decide which blit algorithm to use*/
	srcvga = srcpsd->flags & PSF_SCREEN;
	dstvga = dstpsd->flags & PSF_SCREEN;

	if(srcvga) {
		if(dstvga)
			vga_to_vga_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
		else
			vga_to_mempl4_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
	} else {
		if(dstvga)
			mempl4_to_vga_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
		else
			mempl4_to_mempl4_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
	}
}

SUBDRIVER memplan4 = {
	mempl4_init,
	mempl4_drawpixel,
	mempl4_readpixel,
	mempl4_drawhorzline,
	mempl4_drawvertline,
	gen_fillrect,
	ega_blit
};

#endif /* HAVEBLIT*/
