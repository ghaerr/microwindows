/*
 * Copyright (c) 1999, 2000, 2001 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * 16bpp Linear Video Driver for Microwindows
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>
#include <stdlib.h>

/* We want to do string copying fast, so inline assembly if possible */
#ifndef __OPTIMIZE__
#define __OPTIMIZE__
#endif
#include <string.h>

#include "device.h"
#include "fb.h"

#define USE_16BIT_ACCESS 0	/* =1 to force 16 bit display access*/

#if USE_16BIT_ACCESS
#define memcpy(d,s,nbytes)	memcpy16(d,s,(nbytes)>>1)
#define memmove(d,s,nbytes)	memcpy16(d,s,(nbytes)>>1)
static void
memcpy16(unsigned short *dst, unsigned short *src, int nwords)
{
	while (--nwords >= 0)
		*dst++ = *src++;
}
#endif

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear16_init(PSD psd)
{
	if (!psd->size) {
		psd->size = psd->yres * psd->linelen;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32*/
		psd->linelen /= 2;
	}
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear16_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	if(gr_mode == MWMODE_COPY)
		addr[x + y * psd->linelen] = c;
	else
		applyOp(gr_mode, c, &addr[x + y * psd->linelen], ADDR16);
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear16_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return addr[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear16_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += x1 + y * psd->linelen;
	if(gr_mode == MWMODE_COPY) {
		/* FIXME: memsetw(dst, c, x2-x1+1)*/
		while(x1++ <= x2)
			*addr++ = c;
	} else {
		while (x1++ <= x2) {
			applyOp(gr_mode, c, addr, ADDR16);
			++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear16_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR16	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	addr += x + y1 * linelen;
	if(gr_mode == MWMODE_COPY) {
		while(y1++ <= y2) {
			*addr = c;
			addr += linelen;
		}
	} else {
		while (y1++ <= y2) {
			applyOp(gr_mode, c, addr, ADDR16);
			addr += linelen;
		}
	}
	DRAWOFF;
}

/* srccopy bitblt*/
static void
linear16_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR16	dst = dstpsd->addr;
	ADDR16	src = srcpsd->addr;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
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
	dst += dstx + dsty * dlinelen;
	src += srcx + srcy * slinelen;

#if ALPHABLEND
	if((op & MWROP_EXTENSION) != MWROP_BLENDCONSTANT)
		goto stdblit;
	alpha = op & 0xff;

	if (dstpsd->pixtype == MWPF_TRUECOLOR565) {
		while(--h >= 0) {
			for(i=0; i<w; ++i) {
				unsigned int s = *src++;
				unsigned int d = *dst;
				unsigned int t = d & 0xf800;
				unsigned int m1, m2, m3;
				m1 = (((((s & 0xf800) - t)*alpha)>>8) & 0xf800) + t;
				t = d & 0x07e0;
				m2 = (((((s & 0x07e0) - t)*alpha)>>8) & 0x07e0) + t;
				t = d & 0x001f;
				m3 = (((((s & 0x001f) - t)*alpha)>>8) & 0x001f) + t;
				*dst++ = m1 | m2 | m3;
			}
			dst += dlinelen - w;
			src += slinelen - w;
		}
	} else {
		/* 5/5/5 format*/
		while(--h >= 0) {
			for(i=0; i<w; ++i) {
				unsigned int s = *src++;
				unsigned int d = *dst;
				unsigned int t = d & 0x7c00;
				unsigned int m1, m2, m3;
				m1 = (((((s & 0x7c00) - t)*alpha)>>8) & 0x7c00) + t;
				t = d & 0x03e0;
				m2 = (((((s & 0x03e0) - t)*alpha)>>8) & 0x03e0) + t;
				t = d & 0x001f;
				m3 = (((((s & 0x001f) - t)*alpha)>>8) & 0x001f) + t;
				*dst++ = m1 | m2 | m3;
			}
			dst += dlinelen - w;
			src += slinelen - w;
		}
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
		while (--h >= 0) {
			/* a _fast_ memcpy is a _must_ in this routine*/
			memmove(dst, src, w<<1);
			dst += dlinelen;
			src += slinelen;
		}
	} else {
		while (--h >= 0) {
			for (i=0; i<w; i++) {
				applyOp(MWROP_TO_MODE(op), *src, dst, ADDR16);
				++src;
				++dst;
			}
			dst += dlinelen - w;
			src += slinelen - w;
		}
	}
	DRAWOFF;
}

/* VERY experimental globals for debugging stretchblit off-by-some bug*/
extern int g_row_inc, g_col_inc;

/* srccopy stretchblt*/
static void
linear16_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, long op)
{
	ADDR16	dst;
	ADDR16	src;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	i, ymax;
	int	row_pos, row_inc;
	int	col_pos, col_inc;
	unsigned short pixel = 0;

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
if (g_row_inc) row_inc = g_row_inc; else
	row_inc = (srch << 16) / dsth;

	/* stretch blit using integer ratio between src/dst height/width*/
	for (ymax = dsty+dsth; dsty<ymax; ++dsty) {

		/* find source y position*/
		while (row_pos >= 0x10000L) {
			++srcy;
			row_pos -= 0x10000L;
		}

		dst = (ADDR16)dstpsd->addr + dstx + dsty*dlinelen;
		src = (ADDR16)srcpsd->addr + srcx + (srcy-1)*slinelen;

		/* copy a row of pixels*/
		col_pos = 0x10000;
if (g_col_inc) col_inc = g_col_inc; else
		col_inc = (srcw << 16) / dstw;
		for (i=0; i<dstw; ++i) {
			/* get source x pixel*/
			while (col_pos >= 0x10000L) {
				pixel = *src++;
				col_pos -= 0x10000L;
			}
			*dst++ = pixel;
			col_pos += col_inc;
		}

		row_pos += row_inc;
	}
	DRAWOFF;
}

/*
 * This stretchblit code was originally written for the TriMedia
 * VLIW CPU.  Therefore it uses RESTRICT pointers, and the special
 * one-assembler-opcode pseudo-functions SIGN and ABS.
 *
 * (The 'restrict' extension is in C99, so for a C99 compiler you
 * could "#define RESTRICT restrict" or put
 * "CFLAGS += -DRESTRICT=restrict" in the makefile).
 *
 * Compatibility definitions:
 */
#ifndef RESTRICT
#define RESTRICT
#endif
#ifndef SIGN
#define SIGN(x) (((x) > 0) ? 1 : (((x) == 0) ? 0 : -1))
#endif
#ifndef ABS
#define ABS(x) (((x) >= 0) ? (x) : -(x))
#endif

/* Blit a 16-bit image.
 * Can stretch the image by any X and/or Y scale factor.
 * Can flip the image in the X and/or Y axis.
 *
 * This is the faster version with no per-pixel multiply and a single
 * decision tree for the inner loop, by Jon.  Based on Alex's original
 * all-integer version.
 *
 * Paramaters:
 * srf              - Dest surface
 * dest_x_start
 * dest_y_start    - Top left corner of dest rectangle
 * width, height   - Size in dest co-ordinates.
 * x_denominator   - Denominator for source X value fractions.  Note that
 *                   this must be even, and all the numerators must also be
 *                   even, so we can easily divide by 2.
 * y_denominator   - Denominator for source Y value fractions.  Note that
 *                   this must be even, and all the numerators must also be
 *                   even, so we can easily divide by 2.
 * src_x_fraction  -
 * src_y_fraction  - Point in source that corresponds to the top left corner
 *                   of the pixel (dest_x_start, dest_y_start).  This is a
 *                   fraction - to get a float, divide by y_denominator.
 * x_step_fraction - X step in src for an "x++" step in dest.  May be negative
 *                   (for a flip).  Expressed as a fraction - divide it by
 *                   x_denominator for a float.
 * y_step_fraction - Y step in src for a  "y++" step in dest.  May be negative
 *                   (for a flip).  Expressed as a fraction - divide it by
 *                   y_denominator for a float.
 * image           - Source image.
 * op              - Raster operation, currently ignored.
 */
static void
linear16_stretchblitex(PSD dstpsd,
			 PSD srcpsd,
			 MWCOORD dest_x_start,
			 MWCOORD dest_y_start,
			 MWCOORD width,
			 MWCOORD height,
			 int x_denominator,
			 int y_denominator,
			 int src_x_fraction,
			 int src_y_fraction,
			 int x_step_fraction,
			 int y_step_fraction,
			 long op)
{
	/* Pointer to the current pixel in the source image */
	unsigned short *RESTRICT src_ptr;

	/* Pointer to x=xs1 on the next line in the source image */
	unsigned short *RESTRICT next_src_ptr;

	/* Pointer to the current pixel in the dest image */
	unsigned short *RESTRICT dest_ptr;

	/* Pointer to x=xd1 on the next line in the dest image */
	unsigned short *next_dest_ptr;

	/* Keep track of error in the source co-ordinates */
	int x_error;
	int y_error;

	/* 1-unit steps "forward" through the source image, as steps in the image
	 * byte array.
	 */
	int src_x_step_one;
	int src_y_step_one;

	/* normal steps "forward" through the source image, as steps in the image
	 * byte array.
	 */
	int src_x_step_normal;
	int src_y_step_normal;

	/* 1-unit steps "forward" through the source image, as steps in the image
	 * byte array.
	 */
	int x_error_step_normal;
	int y_error_step_normal;

	/* Countdown to the end of the destination image */
	int x_count;
	int y_count;

	/* Start position in source, in whole pixels */
	int src_x_start;
	int src_y_start;

	/* Error values for start X position in source */
	int x_error_start;

	/* 1-unit step down dest, in bytes. */
	int dest_y_step;

	/*DPRINTF("Nano-X: linear32_stretchflipblit( dest=(%d,%d) %dx%d )\n",
	       dest_x_start, dest_y_start, width, height);*/

	/* We add half a dest pixel here so we're sampling from the middle of
	 * the dest pixel, not the top left corner.
	 */
	src_x_fraction += (x_step_fraction >> 1);
	src_y_fraction += (y_step_fraction >> 1);

	/* Seperate the whole part from the fractions.
	 *
	 * Also, We need to do lots of comparisons to see if error values are
	 * >= x_denominator.  So subtract an extra x_denominator for speed - then
	 * we can just check if it's >= 0.
	 */
	src_x_start = src_x_fraction / x_denominator;
	src_y_start = src_y_fraction / y_denominator;
	x_error_start = src_x_fraction - (src_x_start + 1) * x_denominator;
	y_error = src_y_fraction - (src_y_start + 1) * y_denominator;

	/* precalculate various deltas */

	src_x_step_normal = x_step_fraction / x_denominator;
	src_x_step_one = SIGN(x_step_fraction);
	x_error_step_normal =
		ABS(x_step_fraction) - ABS(src_x_step_normal) * x_denominator;

	src_y_step_normal = y_step_fraction / y_denominator;
	src_y_step_one = SIGN(y_step_fraction) * srcpsd->linelen;
	y_error_step_normal =
		ABS(y_step_fraction) - ABS(src_y_step_normal) * y_denominator;
	src_y_step_normal *= srcpsd->linelen;

	/* DPRINTF("ov_stretch_image8: X: One step=%d, err-=%d; normal step=%d, err+=%d\n                   Y: One step=%d, err-=%d; normal step=%d, err+=%d\n",
	   src_x_step_one, x_denominator, src_x_step_normal, x_error_step_normal,
	   src_y_step_one, y_denominator, src_y_step_normal, y_error_step_normal);
	 */

	/* Pointer to the first source pixel */
	next_src_ptr =
		((unsigned short *) srcpsd->addr) +
		src_y_start * srcpsd->linelen + src_x_start;

	/* Cache the width of a scanline in dest */
	dest_y_step = dstpsd->linelen;

	/* Pointer to the first dest pixel */
	next_dest_ptr =
		((unsigned short *) dstpsd->addr) +
		(dest_y_start * dest_y_step) + dest_x_start;

	/*
	 * Note: The MWROP_SRC case below is a simple expansion of the
	 * default case.  It can be removed without significant speed
	 * penalty if you need to reduce code size.
	 *
	 * The MWROP_CLEAR case could be removed.  But it is a large
	 * speed increase for a small quantity of code.
	 */
	switch (op & MWROP_EXTENSION) {
	case MWROP_SRC:
		/* Benchmarking shows that this while loop is faster than the equivalent
		 * for loop: for(y_count=0; y_count<height; y_count++) { ... }
		 */
		y_count = height;
		while (y_count-- > 0) {
			src_ptr = next_src_ptr;
			dest_ptr = next_dest_ptr;

			x_error = x_error_start;

			x_count = width;
			while (x_count-- > 0) {
				*dest_ptr++ = *src_ptr;

				src_ptr += src_x_step_normal;
				x_error += x_error_step_normal;

				if (x_error >= 0) {
					src_ptr += src_x_step_one;
					x_error -= x_denominator;
				}
			}

			next_dest_ptr += dest_y_step;

			next_src_ptr += src_y_step_normal;
			y_error += y_error_step_normal;

			if (y_error >= 0) {
				next_src_ptr += src_y_step_one;
				y_error -= y_denominator;
			}
		}
		break;

	case MWROP_CLEAR:
		y_count = height;
		while (y_count-- > 0) {
			dest_ptr = next_dest_ptr;
			x_count = width;
			while (x_count-- > 0) {
				*dest_ptr++ = 0;
			}
			next_dest_ptr += dest_y_step;
		}
		break;

	default:
		y_count = height;
		while (y_count-- > 0) {
			src_ptr = next_src_ptr;
			dest_ptr = next_dest_ptr;

			x_error = x_error_start;

			x_count = width;
			while (x_count-- > 0) {
				applyOp(MWROP_TO_MODE(op), *src_ptr, dest_ptr, ADDR16);
				dest_ptr++;

				src_ptr += src_x_step_normal;
				x_error += x_error_step_normal;

				if (x_error >= 0) {
					src_ptr += src_x_step_one;
					x_error -= x_denominator;
				}
			}

			next_dest_ptr += dest_y_step;

			next_src_ptr += src_y_step_normal;
			y_error += y_error_step_normal;

			if (y_error >= 0) {
				next_src_ptr += src_y_step_one;
				y_error -= y_denominator;
			}
		}
		break;

	}
}

#if MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST
/* FIXME is the following "optimization" worthwhile?  My guess is not,
 * but I'll leave it in for now.
 */
static void init_wordmask_lookup(unsigned short **byte2wordmask)
{
	unsigned short *maskp, *b2wm;
	int t, x, u;

	b2wm = *byte2wordmask = malloc(256*8*2);
	if ( b2wm == 0 )
		exit(17);
	for ( t=0; t < 256; t++ ) {
		maskp = b2wm + 8 * t;
		x = t;
		for ( u=1; u < 256; u <<= 1 )
			if ( x & u )
				*maskp++ = 0xffff;
			else
				*maskp++ = 0x0000;
	}
}

/* psd->DrawArea operation PSDOP_BITMAP_BYTES_LSB_FIRST which
 * takes a pixmap, each line is byte aligned, and copies it
 * to the screen using fg_color and bg_color to replace a 1
 * and 0 in the pixmap.  This pixmap is ordered the wrong
 * way around; it has the leftmost pixel (on the screen) in
 * LSB (Bit 0) of the bytes.
 *
 * The reason why this non-intuitive bit ordering is used is
 * to match the bit ordering used in the T1lib font rendering
 * library.
 *
 * Variables used in the gc:
 *       dstx, dsty, dsth, dstw   Destination rectangle
 *       srcx, srcy               Source rectangle
 *       src_linelen              Linesize in bytes of source
 *       pixels                   Pixmap data
 *       fg_color                 Color of a '1' bit
 *       bg_color                 Color of a '0' bit
 *       gr_usebg                 If set, bg_color is used.  If zero,
 *                                then '0' bits are transparent.
 */
static void
linear16_drawarea_bitmap_bytes_lsb_first(PSD psd, driver_gc_t * gc)
{
	int first_byte, last_byte;
	int hard_prefix, hard_postfix;
	unsigned short prefixbits, postfixbits, *maskp;
	unsigned short xor_color, m;
	unsigned short prefix_mask = 0, prefix_last = 0;
	unsigned short postfix_mask = 0, postfix_last = 0;
	unsigned short fg_color = gc->fg_color;
	unsigned short bg_color = gc->bg_color;
	int size_main, t, y;
	unsigned int advance_src, advance_dst;
	unsigned char bitmap_byte;
	ADDR8 src;
	ADDR16 dst;

	static unsigned short *byte2wordmask = 0;

	prefixbits = gc->srcx & 7;
	postfixbits = (gc->srcx + gc->dstw - 1) & 7;
	first_byte = gc->srcx >> 3;
	last_byte = (gc->srcx + gc->dstw - 1) >> 3;

	src = ((ADDR8) gc->pixels) + gc->src_linelen * gc->srcy + first_byte;
	dst = ((ADDR16) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
	xor_color = fg_color ^ bg_color;

	size_main = last_byte - first_byte + 1;
	hard_prefix = 0;
	hard_postfix = 0;

	if (first_byte != last_byte) {
		if (prefixbits != 0) {
			/* Needs to do a few odd bits first */
			hard_prefix = 1;
			size_main--;
			prefix_mask = 1 << prefixbits;
			prefix_last = 0x100;
		}
		if ( postfixbits != 7 ) {
			/* Last byte in source contains a few odd bits */
			hard_postfix = 1;
			postfix_mask = 1;
			postfix_last = 2 << postfixbits;
			size_main--;
		}
	} else {
		/* Very narrow pixmap, fits in single first byte */
		hard_prefix = 1;
		hard_postfix = 0;
		size_main = 0;
		prefix_mask = 1 << prefixbits;
		prefix_last = 1 << (prefixbits + gc->dstw);
	}

	advance_src = gc->src_linelen - last_byte + first_byte - 1;
	advance_dst = psd->linelen - gc->dstw;

	if ( byte2wordmask == 0 )
		init_wordmask_lookup(&byte2wordmask);

	DRAWON;
	if (gc->gr_usebg) {
		for (y = 0; y < gc->dsth; y++) {

			/* Do pixels of partial first byte */
			if (hard_prefix) {
				for (m = prefix_mask; m < prefix_last;
				     m <<= 1) {
					*dst++ = (m & *src) ? fg_color :
						bg_color;
				}
				src++;
			}

			/* Do all pixels of main part one byte at a time */
			for (t = 0; t < size_main; t++) {
				maskp = byte2wordmask + 8 * (*src++);

				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
				*dst++ = gc->
					bg_color ^ (*maskp++ & xor_color);
			}

			/* Do last few bits of line */
			if (hard_postfix) {
				for (m = postfix_mask; m < postfix_last;
				     m <<= 1) {
					*dst++ = (m & *src) ? fg_color :
						bg_color;
				}
				src++;
			}

			src += advance_src;
			dst += advance_dst;
		}
	} else {
		for (y = 0; y < gc->dsth; y++) {

			/* Do pixels of partial first byte */
			if (hard_prefix) {
				bitmap_byte = *src++;
				for (m = prefix_mask; m < prefix_last;
				     m <<= 1) {
					if (m & bitmap_byte)
						*dst = fg_color;
					dst++;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = 0; t < size_main; t++) {
				bitmap_byte = *src++;

				if (0x01 & bitmap_byte)
					dst[0] = fg_color;
				if (0x02 & bitmap_byte)
					dst[1] = fg_color;
				if (0x04 & bitmap_byte)
					dst[2] = fg_color;
				if (0x08 & bitmap_byte)
					dst[3] = fg_color;
				if (0x10 & bitmap_byte)
					dst[4] = fg_color;
				if (0x20 & bitmap_byte)
					dst[5] = fg_color;
				if (0x40 & bitmap_byte)
					dst[6] = fg_color;
				if (0x80 & bitmap_byte)
					dst[7] = fg_color;

				dst += 8;
			}

			/* Do last few bits of line */
			if (hard_postfix) {
				bitmap_byte = *src++;
				for (m = postfix_mask; m < postfix_last;
				     m <<= 1) {
					if (m & bitmap_byte)
						*dst = fg_color;
					dst++;
				}
			}

			src += advance_src;
			dst += advance_dst;
		}
	}
	DRAWOFF;
}
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST */


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
 *                                then '0' bits are transparent.
 */
static void
linear16_drawarea_bitmap_bytes_msb_first(PSD psd, driver_gc_t * gc)
{
/*
 * The difference between the MSB_FIRST and LSB_FIRST variants of
 * this function is simply the definition of these three #defines.
 *
 * MWI_IS_BIT_BEFORE_OR_EQUAL(A,B) returns true if bit A is before
 *     (i.e. to the left of) bit B.
 * MWI_ADVANCE_BIT(X) advances X on to the next bit to the right,
 *     and stores the result back in X.
 * MWI_BIT_NO(N), where 0<=n<=7, gives the Nth bit, where 0 is the
 *     leftmost bit and 7 is the rightmost bit.  This is a constant
 *     iff N is a constant.
 */
#define MWI_IS_BIT_BEFORE_OR_EQUAL(a,b) ((a) >= (b))
#define MWI_ADVANCE_BIT(target) ((target) >>= 1)
#define MWI_BIT_NO(n) (0x80 >> (n))

/*
 * Two convenience defines - these are the same for MSB_FIRST and
 * LSB_FIRST.
 */
#define MWI_FIRST_BIT MWI_BIT_NO(0)
#define MWI_LAST_BIT  MWI_BIT_NO(7)

	unsigned char prefix_first_bit;
	unsigned char postfix_first_bit = MWI_FIRST_BIT;
	unsigned char postfix_last_bit;
	unsigned char bitmap_byte;
	unsigned char mask;
	unsigned short fg, bg;
	int first_byte, last_byte;
	int size_main;
	int t, y;
	unsigned int advance_src, advance_dst;
	ADDR8 src;
	ADDR16 dst;

	/* The bit in the first byte, which corresponds to the leftmost pixel. */
	prefix_first_bit = MWI_BIT_NO(gc->srcx & 7);

	/* The bit in the last byte, which corresponds to the rightmost pixel. */
	postfix_last_bit = MWI_BIT_NO((gc->srcx + gc->dstw - 1) & 7);

	/* The index into each scanline of the first byte to use. */
	first_byte = gc->srcx >> 3;

	/* The index into each scanline of the last byte to use. */
	last_byte = (gc->srcx + gc->dstw - 1) >> 3;

	src = ((ADDR8) gc->pixels) + gc->src_linelen * gc->srcy + first_byte;
	dst = ((ADDR16) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
	fg = gc->fg_color;
	bg = gc->bg_color;

	advance_src = gc->src_linelen - last_byte + first_byte - 1;
	advance_dst = psd->linelen - gc->dstw;

	if (first_byte != last_byte) {
		/* The total number of bytes to use, less the two special-cased
		 * bytes (first and last).
		 */
		size_main = last_byte - first_byte + 1 - 2;

		if (prefix_first_bit == MWI_FIRST_BIT) {
			/* No need to special case. */
			prefix_first_bit = 0;
			size_main++;
		}
		if (postfix_last_bit == MWI_LAST_BIT) {
			/* No need to special case. */
			postfix_last_bit = 0;
			size_main++;
		}
	} else if ((prefix_first_bit == MWI_FIRST_BIT)
		   && (postfix_last_bit == MWI_LAST_BIT)) {
		/* Exactly one byte wide. */
		prefix_first_bit = 0;
		postfix_last_bit = 0;
		size_main = 1;
	} else {
		/* Very narrow pixmap, fits in single first byte. */
		/* Do everything in 'postfix' loop. */
		postfix_first_bit = prefix_first_bit;
		prefix_first_bit = 0;
		size_main = 0;
	}

	DRAWON;

	if (gc->gr_usebg) {
		for (y = 0; y < gc->dsth; y++) {

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask;
				     MWI_ADVANCE_BIT(mask)) {
					*dst++ = (mask & bitmap_byte) ? fg :
						bg;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				*dst++ = (MWI_BIT_NO(0) & bitmap_byte) ? fg :
					bg;
				*dst++ = (MWI_BIT_NO(1) & bitmap_byte) ? fg :
					bg;
				*dst++ = (MWI_BIT_NO(2) & bitmap_byte) ? fg :
					bg;
				*dst++ = (MWI_BIT_NO(3) & bitmap_byte) ? fg :
					bg;
				*dst++ = (MWI_BIT_NO(4) & bitmap_byte) ? fg :
					bg;
				*dst++ = (MWI_BIT_NO(5) & bitmap_byte) ? fg :
					bg;
				*dst++ = (MWI_BIT_NO(6) & bitmap_byte) ? fg :
					bg;
				*dst++ = (MWI_BIT_NO(7) & bitmap_byte) ? fg :
					bg;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask,
								postfix_last_bit);
				     MWI_ADVANCE_BIT(mask)) {
					*dst++ = (mask & bitmap_byte) ? fg :
						bg;
				}
			}

			src += advance_src;
			dst += advance_dst;
		}
	} else {
		for (y = 0; y < gc->dsth; y++) {

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask;
				     MWI_ADVANCE_BIT(mask)) {
					if (mask & bitmap_byte)
						*dst = fg;
					dst++;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				if (MWI_BIT_NO(0) & bitmap_byte)
					dst[0] = fg;
				if (MWI_BIT_NO(1) & bitmap_byte)
					dst[1] = fg;
				if (MWI_BIT_NO(2) & bitmap_byte)
					dst[2] = fg;
				if (MWI_BIT_NO(3) & bitmap_byte)
					dst[3] = fg;
				if (MWI_BIT_NO(4) & bitmap_byte)
					dst[4] = fg;
				if (MWI_BIT_NO(5) & bitmap_byte)
					dst[5] = fg;
				if (MWI_BIT_NO(6) & bitmap_byte)
					dst[6] = fg;
				if (MWI_BIT_NO(7) & bitmap_byte)
					dst[7] = fg;

				dst += 8;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask,
								postfix_last_bit);
				     MWI_ADVANCE_BIT(mask)) {
					if (mask & bitmap_byte)
						*dst = fg;
					dst++;
				}
			}

			src += advance_src;
			dst += advance_dst;
		}
	}

	DRAWOFF;

#undef MWI_IS_BIT_BEFORE_OR_EQUAL
#undef MWI_ADVANCE_BIT
#undef MWI_BIT_NO
#undef MWI_FIRST_BIT
#undef MWI_LAST_BIT
}
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST */


#if MW_FEATURE_PSDOP_ALPHAMAP

/* FIXME should make linear16_drawarea_alphamap() work in 5/5/5 mode
 * (currently it assumes 5/6/5 mode).
 */
#if MWPIXEL_FORMAT == MWPF_TRUECOLOR555
#error PSDOP_ALPHAMAP does not work in 5/5/5 mode!!
#endif

static unsigned short *low2scale = 0, *high2scale = 0;

static void
init_alpha_lookup(void)
{
        unsigned short a, x;
        unsigned short r, g, b;
        unsigned short idx;

        low2scale = malloc(32 * 256 * sizeof(low2scale[0]));
        high2scale = malloc(32 * 256 * sizeof(high2scale[0]));

	if (high2scale == 0 || low2scale == 0)
		exit(17);

        for ( a=0; a < 32; a++ )
                for ( x=0; x < 256; x++ ) {
                        idx = (a << 8) | x;
                        /* High byte */
                        r = (x >> 3) * a / 31;
                        g = ((x << 3) & 0x38) * a / 31;
                        high2scale[idx] = (r << 11) | (g << 5);
                        /* Low byte */
                        b = (x & 0x1f) * a / 31;
                        g = ((x >> 5) & 0x7) * a / 31;
                        low2scale[idx] = (g << 5) | b;
                }
}

static void
linear16_drawarea_alphamap(PSD psd, driver_gc_t * gc)
{
	ADDR16 src, dst;
	ADDR8 alpha;
	unsigned short ps, pd;
	unsigned as, ad;
	int x, y;
	int src_row_step, dst_row_step;

	if ( low2scale == 0 )
		init_alpha_lookup();

	src = ((ADDR16) gc->pixels) + gc->srcx + gc->src_linelen * gc->srcy;
	alpha = ((ADDR8) gc->misc) + gc->src_linelen * gc->srcy + gc->srcx;
	dst = ((ADDR16) psd->addr) + psd->linelen * gc->dsty + gc->dstx;

	src_row_step = gc->src_linelen - gc->dstw;
	dst_row_step = psd->linelen - gc->dstw;

	DRAWON;
	for (y = 0; y < gc->dsth; y++) {
		for (x = 0; x < gc->dstw; x++) {
			as = (((unsigned short) ((*alpha++) >> 3)) << 8);
			ad = (31 << 8) - as;
			ps = *src++;
			pd = *dst++;
			*dst++ = ((as == 0) ? pd :
				  ((ad == 0) ? ps :
				   low2scale[as | (ps >> 8)] +
				   high2scale[as | (ps & 0xFF)] +
				   low2scale[ad | (pd >> 8)] +
				   high2scale[ad | (pd & 0xFF)]));
		}
		alpha += src_row_step;
		src += src_row_step;
		dst += dst_row_step;
	}
	DRAWOFF;
}
#endif /* MW_FEATURE_PSDOP_ALPHAMAP */


#if MW_FEATURE_PSDOP_ALPHACOL
static void
linear16_drawarea_alphacol(PSD psd, driver_gc_t * gc)
{
	ADDR16 dst;
	ADDR8 alpha;
	unsigned ps, pd;
	int as;
	long psr, psg, psb;
	int x, y;
	int src_row_step, dst_row_step;

	alpha = ((ADDR8) gc->misc) + gc->src_linelen * gc->srcy + gc->srcx;
	dst = ((ADDR16) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
	ps = gc->fg_color;

	src_row_step = gc->src_linelen - gc->dstw;
	dst_row_step = psd->linelen - gc->dstw;

#define COLOR_MASK_R_565 0xF800U
#define COLOR_MASK_G_565 0x07E0U
#define COLOR_MASK_B_565 0x001FU

#define COLOR_MASK_R_555 0x7C00U
#define COLOR_MASK_G_555 0x03E0U
#define COLOR_MASK_B_555 0x001FU

	DRAWON;
	if (psd->pixtype == MWPF_TRUECOLOR565) {
		psr = (long) (ps & COLOR_MASK_R_565);
		psg = (long) (ps & COLOR_MASK_G_565);
		psb = (long) (ps & COLOR_MASK_B_565);

		for (y = 0; y < gc->dsth; y++) {
			for (x = 0; x < gc->dstw; x++) {
				as = *alpha++;
				if (as == 255) {
					*dst++ = ps;
				} else if (as != 0) {
					/*
					 * Scale alpha value from 255ths to 256ths
					 * (In other words, if as >= 128, add 1 to it)
					 *
					 * Also flip the direction of alpha, so it's
					 * backwards from it's usual meaning.
					 * This is because the equation below is most
					 * easily written with source and dest interchanged
					 * (since we can split ps into it's components
					 * before we enter the loop)
					 */
					as = 256 - (as + (as >> 7));
					pd = *dst;

					*dst++ = ((unsigned)
						  (((((long)
						      (pd & COLOR_MASK_R_565)
						      - psr) * as) >> 8) +
						   psr) & COLOR_MASK_R_565)
						|
						((unsigned)
						 (((((long)
						     (pd & COLOR_MASK_G_565) -
						     psg) * as) >> 8) +
						  psg) & COLOR_MASK_G_565)
						|
						((unsigned)
						 (((((long)
						     (pd & COLOR_MASK_B_565) -
						     psb) * as) >> 8) +
						  psb) & COLOR_MASK_B_565);
				} else {
					dst++;
				}
			}
			alpha += src_row_step;
			dst += dst_row_step;
		}
	} else {
		psr = (long) (ps & COLOR_MASK_R_555);
		psg = (long) (ps & COLOR_MASK_G_555);
		psb = (long) (ps & COLOR_MASK_B_555);

		for (y = 0; y < gc->dsth; y++) {
			for (x = 0; x < gc->dstw; x++) {
				as = *alpha++;
				if (as == 255) {
					*dst++ = ps;
				} else if (as != 0) {
					/*
					 * Scale alpha value from 255ths to 256ths
					 * (In other words, if as >= 128, add 1 to it)
					 *
					 * Also flip the direction of alpha, so it's
					 * backwards from it's usual meaning.
					 * This is because the equation below is most
					 * easily written with source and dest interchanged
					 * (since we can split ps into it's components
					 * before we enter the loop)
					 */
					as = 256 - (as + (as >> 7));
					pd = *dst;

					*dst++ = ((unsigned)
						  (((((long)
						      (pd & COLOR_MASK_R_555)
						      - psr) * as) >> 8) +
						   psr) & COLOR_MASK_R_555)
						|
						((unsigned)
						 (((((long)
						     (pd & COLOR_MASK_G_555) -
						     psg) * as) >> 8) +
						  psg) & COLOR_MASK_G_555)
						|
						((unsigned)
						 (((((long)
						     (pd & COLOR_MASK_B_555) -
						     psb) * as) >> 8) +
						  psb) & COLOR_MASK_B_555);
				} else {
					dst++;
				}
			}
			alpha += src_row_step;
			dst += dst_row_step;
		}
	}
	DRAWOFF;
}
#endif /* MW_FEATURE_PSDOP_ALPHACOL */

#if MW_FEATURE_PSDOP_COPY
static void
linear16_drawarea_copyall(PSD psd, driver_gc_t * gc)
{
	ADDR16 src16, dst;
	int linesize, x, y;
	unsigned short pcol;

	linesize = 2 * gc->dstw;
	src16 = ((ADDR16) gc->pixels) + gc->srcx + gc->src_linelen * gc->srcy;
	dst = ((ADDR16) psd->addr) + gc->dstx + psd->linelen * gc->dsty;

	DRAWON;
	for (y = 1; y < gc->dsth; y++) {
		memcpy(dst, src16, linesize);
		src16 += gc->src_linelen;
		dst += psd->linelen;
	}
	memcpy(dst, src16, linesize);	/* To be seriously ANSI */
	DRAWOFF;
}

static void
linear16_drawarea_copytrans(PSD psd, driver_gc_t * gc)
{
	ADDR16	src16, dst, rsrc, rdst;
	int linesize, x, y;
	unsigned short pcol;

	src16 = ((ADDR16) gc->pixels) + gc->srcx + gc->src_linelen * gc->srcy;
	dst = ((ADDR16) psd->addr) + gc->dstx + psd->linelen * gc->dsty;

	DRAWON;
	for (y = 0; y < gc->dsth; y++) {
		rdst = dst;
		rsrc = src16;
		for (x = 0; x < gc->dstw; x++) {
			pcol = *rsrc++;
			if (pcol == gc->bg_color)
				rdst++;
			else
				*rdst++ = pcol;
		}
		dst += psd->linelen;
		src16 += gc->src_linelen;
	}
	DRAWOFF;
}
#endif

static void
linear16_drawarea(PSD psd, driver_gc_t * gc, int op)
{
	assert(psd->addr != 0);
	/*assert(gc->dstw <= gc->srcw);*/
	assert(gc->dstx >= 0 && gc->dstx+gc->dstw <= psd->xres);
	/*assert(gc->dsty >= 0 && gc->dsty+gc->dsth <= psd->yres);*/
	/*assert(gc->srcx >= 0 && gc->srcx+gc->dstw <= gc->srcw);*/
	assert(gc->srcy >= 0);
	/*DPRINTF("linear16_drawarea op=%d dstx=%d dsty=%d\n", op, gc->dstx, gc->dsty);*/

	switch (op) {

#if MW_FEATURE_PSDOP_COPY
	case PSDOP_COPY:
		if (gc->gr_usebg) {
			linear16_drawarea_copyall(psd, gc);
		} else {
			linear16_drawarea_copytrans(psd, gc);
		}
		break;

	case PSDOP_COPYALL:
		linear16_drawarea_copyall(psd, gc);
		break;

	case PSDOP_COPYTRANS:
		linear16_drawarea_copytrans(psd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_COPY */

#if MW_FEATURE_PSDOP_ALPHAMAP
	case PSDOP_ALPHAMAP:
		linear16_drawarea_alphamap(psd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_ALPHAMAP */

#if MW_FEATURE_PSDOP_ALPHACOL
	case PSDOP_ALPHACOL:
		linear16_drawarea_alphacol(psd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_ALPHACOL */

#if MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST
	case PSDOP_BITMAP_BYTES_LSB_FIRST:
		linear16_drawarea_bitmap_bytes_lsb_first(psd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST */

#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
	case PSDOP_BITMAP_BYTES_MSB_FIRST:
		linear16_drawarea_bitmap_bytes_msb_first(psd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST */

	}
}

SUBDRIVER fblinear16 = {
	linear16_init,
	linear16_drawpixel,
	linear16_readpixel,
	linear16_drawhorzline,
	linear16_drawvertline,
	gen_fillrect,
	linear16_blit,
	linear16_drawarea,
	linear16_stretchblit,
	linear16_stretchblitex,
};
