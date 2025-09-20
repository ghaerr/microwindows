/*
 * 4 planes EGA/VGA Memory (blitting) Video Driver for Microwindows
 * 
 * Copyright (c) 1999, 2000, 2002 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2002 Alex Holden <alex@alexholden.net>
 *
 * 4bpp colors are stored in linear 4pp format in memory
 * In this driver, psd->pitch is line byte length, not line pixel length
 *
 * Modified for PC-98
 * T. Yamada 2025
 */

/*#define NDEBUG*/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "device.h"
#include "genmem.h"
#include "vgaplan4.h"
#include "fb.h"

#if MSDOS | ELKS | RTEMS
/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENBASE0 	MK_FP(0xa800, 0)
#define SCREENBASE1 	MK_FP(0xb000, 0)
#define SCREENBASE2 	MK_FP(0xb800, 0)
#define SCREENBASE3 	MK_FP(0xe000, 0)
#define BYTESPERLINE(psd)	80

static FARADDR screenbase_table[4] = {
	SCREENBASE0, SCREENBASE2, SCREENBASE1, SCREENBASE3
};
#else
/* run on top of framebuffer*/
#define SCREENBASE(psd) 	((char *)((psd)->addr))
#define BYTESPERLINE(psd)	((psd)->pitch)
#endif

static unsigned char notmask[2] = { 0x0f, 0xf0};
static unsigned char mask[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

#if 0
/* Calc linelen and mmap size, return 0 on fail*/
static int
mempl4_init(PSD psd)    //FIXME unused
{
	if (!psd->size)
		psd->size = psd->yres * psd->pitch;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}
#endif

/* Set pixel at x, y, to pixelval c*/
static void
mempl4_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	addr += (x>>1) + y * psd->pitch;
	if(gr_mode == MWROP_XOR)
		*addr ^= (unsigned int)c << ((1-(x&1))<<2);
	else
		*addr = (*addr & notmask[x&1]) | ((unsigned int)c << ((1-(x&1))<<2));
}

/* Read pixel at x, y*/
static MWPIXELVAL
mempl4_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[(x>>1) + y * psd->pitch] >> ((1-(x&1))<<2) ) & 0x0f;
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

	addr += (x1>>1) + y * psd->pitch;
	if(gr_mode == MWROP_XOR) {
		while(x1 <= x2) {
			*addr ^= (unsigned int)c << ((1-(x1&1))<<2);
			if((++x1 & 1) == 0)
				++addr;
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&1]) | ((unsigned int)c << ((1-(x1&1))<<2));
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
	int	pitch = psd->pitch;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	addr += (x>>1) + y1 * pitch;
	if(gr_mode == MWROP_XOR)
		while(y1++ <= y2) {
			*addr ^= (unsigned int)c << ((1-(x&1))<<2);
			addr += pitch;
		}
	else
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&1]) | ((unsigned int)c << ((1-(x&1))<<2));
			addr += pitch;
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
	int	dpitch = dstpsd->pitch;
	int	spitch = srcpsd->pitch;

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

	dst = dstpsd->addr + (dstx>>1) + dsty * dpitch;
	src = srcpsd->addr + (srcx>>1) + srcy * spitch;
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
		dst += dpitch;
		src += spitch;
	}
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from vga memory to vga memory, psd's ignored for speed*/
static void
vga_to_vga_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	MWCOORD	i, x1, x2;
	int	plane;
	FARADDR	dst[4];
	FARADDR	src[4];

	unsigned int dst_b;
	unsigned int src_b;
	unsigned int mask_b;

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
	assert (srcx != dstx);			/* FIXME use EPRINTF */

	DRAWON;
	x2 = dstx + w - 1;
	x1 = dstx;
	for(plane=0; plane<4; ++plane) {
		dst[plane] = screenbase_table[plane] + (x1>>3) + dsty * BYTESPERLINE(dstpsd);
		src[plane] = screenbase_table[plane] + (srcx>>3) + srcy * BYTESPERLINE(srcpsd);
	}
	while(--h >= 0) {
		for(plane=0; plane<4; ++plane) {
			FARADDR d = dst[plane];
			FARADDR s = src[plane];

			/* FIXME: only works if srcx and dstx are same modulo*/
			if((x1>>3) == (x2>>3)) {
				mask_b = (0xff >> (x1 & 7)) & (0xff << (7 - (x2 & 7)));
				src_b = GETBYTE_FP(s) & mask_b;
				dst_b = GETBYTE_FP(d) & ~mask_b;
				PUTBYTE_FP(d, src_b | dst_b);
			} else {
				mask_b = (0xff >> (x1 & 7));
				src_b = GETBYTE_FP(s) & mask_b;
				dst_b = GETBYTE_FP(d) & ~mask_b;
				PUTBYTE_FP(d, src_b | dst_b);
				s++;
				d++;

				for (i = (x2 - (x1&~7)) >> 3; i > 1; i--)   /* while x1+1 < x2 */
					PUTBYTE_FP(d++, GETBYTE_FP(s++));

				mask_b = (0xff << (7 - (x2 & 7)));
				src_b = GETBYTE_FP(s) & mask_b;
				dst_b = GETBYTE_FP(d) & ~mask_b;
				PUTBYTE_FP(d, src_b | dst_b);
			}
			dst[plane] += BYTESPERLINE(dstpsd);
			src[plane] += BYTESPERLINE(srcpsd);
		}
	}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from memdc to vga memory*/
static void
mempl4_to_vga_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	int	plane;
	FARADDR	dst[4];
	ADDR8	src;
	int	i;
	int	spitch = srcpsd->pitch;
	int	color;

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
	for(plane=0; plane<4; ++plane)
		dst[plane] = screenbase_table[plane] + (dstx>>3) + dsty * BYTESPERLINE(dstpsd);

	src = srcpsd->addr + (srcx>>1) + srcy * spitch;
	while(--h >= 0) {
		for(plane=0; plane<4; ++plane) {
			FARADDR d = dst[plane];
			ADDR8	s = src;
			MWCOORD	dx = dstx;
			MWCOORD	sx = srcx;
			for(i=0; i<w; ++i) {
				color = *s >> ((1-(sx&1))<<2) & 0x0f;

				if  (color & (1 << plane)) {
					ORBYTE_FP (d,mask[dx&7]);
				} else {
					ANDBYTE_FP (d,~mask[dx&7]);
				}

				if((++dx & 7) == 0)
					++d;
				if((++sx & 1) == 0)
					++s;
			}
			dst[plane] += BYTESPERLINE(dstpsd);
		}
		src += spitch;
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
	FARADDR s[4], src[4];
	MWCOORD sx[4], dx;
	unsigned char color;
	int dpitch = dstpsd->pitch;

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
	for(plane=0; plane<4; ++plane)
		src[plane] = screenbase_table[plane] + (srcx >> 3) + srcy * BYTESPERLINE(srcpsd);

	dst = dstpsd->addr + (dstx >> 1) + dsty * dpitch;

	for(y = 0; y < h; y++) {
		for(plane = 0; plane < 4; ++plane) {
			s[plane] = src[plane];
			sx[plane] = srcx;
		}
		d = dst;
		dx = dstx;
		color = 0;
		for(x = 0; x < w; x++) {
			for(plane = 0; plane < 4; ++plane) {
				if(GETBYTE_FP(s[plane]) & mask[sx[plane] & 7])
					color |= 1 << (plane + ((dx & 1) ? 0 : 4));
				if((++sx[plane] & 7) == 0) ++s[plane];
			}
			if((++dx & 1) == 0) {
				*d++ = color;
				color = 0;
			}
		}
		dst += dpitch;
		for(plane=0; plane<4; ++plane)
			src[plane] += BYTESPERLINE(srcpsd);
	}
	DRAWOFF;
}

void
vga_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	MWBOOL	srcvga, dstvga;

	/* decide which blit algorithm to use*/
	srcvga = srcpsd->flags & PSF_SCREEN;
	dstvga = dstpsd->flags & PSF_SCREEN;

	if(srcvga) {
		if(dstvga)
			vga_to_vga_blit(dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, op);
		else
			vga_to_mempl4_blit(dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, op);
	} else {
		if(dstvga)
			mempl4_to_vga_blit(dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, op);
		else
			mempl4_to_mempl4_blit(dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, op);
	}
}

static SUBDRIVER memplan4_none = {
	mempl4_drawpixel,
	mempl4_readpixel,
	mempl4_drawhorzline,
	mempl4_drawvertline,
	gen_fillrect,
	vga_blit,
	NULL,       /* FrameBlit*/
	NULL,       /* FrameStretchBlit*/
	0, //linear4_convblit_copy_mask_mono_byte_msb,
	NULL,       /* BlitCopyMaskMonoByteLSB*/
	NULL,       /* BlitCopyMaskMonoWordMSB*/
	NULL,       /* BlitBlendMaskAlphaByte*/
	NULL,       /* BlitCopyRGBA8888*/
	NULL,       /* BlitSrcOverRGBA8888*/
	NULL        /* BlitCopyRGB888*/
};

PSUBDRIVER memplan4[4] = {
	&memplan4_none,
#if MW_FEATURE_PORTRAIT
	&fbportrait_left, &fbportrait_right, &fbportrait_down
#endif
};
