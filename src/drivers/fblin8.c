/*
 * Copyright (c) 1999, 2000, 2001, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * 8bpp Linear Video Driver for Microwindows
 * 	2000/01/26 added alpha blending with lookup tables (64k total)
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>

/* We want to do string copying fast, so inline assembly if possible */
#ifndef __OPTIMIZE__
#define __OPTIMIZE__
#endif
#include <string.h>

#include "device.h"
#include "fb.h"

/*
 * Alpha lookup tables for 256 color palette systems
 * A 5 bit alpha value is used to keep tables smaller.
 *
 * Two tables are created.  The first, alpha_to_rgb contains 15 bit RGB 
 * values for each alpha value for each color: 32*256 short words.
 * RGB values can then be blended.  The second, rgb_to_palindex contains
 * the closest color (palette index) for each of the 5-bit
 * R, G, and B values: 32*32*32 bytes.
 */
static unsigned short *alpha_to_rgb = NULL;
static unsigned char  *rgb_to_palindex = NULL;
int init_alpha_lookup(void);

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear8_init(PSD psd)
{
	if (!psd->size)
		psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear8_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
		((ADDR8)psd->addr)[x + y * psd->linelen] = c;
	else
		applyOp(gr_mode, c, &((ADDR8)psd->addr)[ x + y * psd->linelen], ADDR8);
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear8_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return ((ADDR8)psd->addr)[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear8_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register ADDR8 addr = ((ADDR8)psd->addr) + x1 + y * psd->linelen;
#if DEBUG
	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
		memset(addr, c, x2 - x1 + 1);
	else {
		while(x1++ <= x2) {
			applyOp(gr_mode, c, addr, ADDR8);
			++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear8_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	linelen = psd->linelen;
	register ADDR8 addr = ((ADDR8)psd->addr) + x + y1 * linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY) {
		while(y1++ <= y2) {
			*addr = c;
			addr += linelen;
		}
	} else {
		while(y1++ <= y2) {
			applyOp(gr_mode, c, addr, ADDR8);
			addr += linelen;
		}
	}
	DRAWOFF;
}

/* srccopy bitblt*/
static void
linear8_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	ADDR8	dst;
	ADDR8	src;
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

	dst = ((ADDR8)dstpsd->addr) + dstx + dsty * dlinelen;
	src = ((ADDR8)srcpsd->addr) + srcx + srcy * slinelen;

	DRAWON;

	if (op == MWROP_BLENDCONSTANT) {
	unsigned int srcalpha = 150;
	unsigned int dstalpha;
	int i;

	/* create alpha lookup tables*/
	if(!rgb_to_palindex) {
		if (!init_alpha_lookup())
			goto copy;
	}

	/* 
	 * Create 5 bit alpha value index for 256 color indexing.
	 * Destination alpha is (1 - source) alpha
	 */
	dstalpha = ((srcalpha >> 3) ^ 31) << 8;
	srcalpha =  (srcalpha >> 3) << 8;

	while(--h >= 0) {
	    for(i=0; i<w; ++i) {
			/* Get source RGB555 value for source alpha value*/
			unsigned short s = alpha_to_rgb[srcalpha + *src++];

			/* Get destination RGB555 value for dest alpha value*/
			unsigned short d = alpha_to_rgb[dstalpha + *dst];

			/* Add RGB values together and get closest palette index to it*/
			*dst++ = rgb_to_palindex[s + d];
	    }
	    dst += dlinelen - w;
	    src += slinelen - w;
	}
	} else if (op == MWROP_COPY) {
copy:
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
			memmove(dst, src, w);
			dst += dlinelen;
			src += slinelen;
		}
	} else {
		while (--h >= 0) {
			applyOp4(w, op, src, dst, ADDR8);
			dst += dlinelen - w;
			src += slinelen - w;
		}
	}
	DRAWOFF;
}

#if 0000 /* DEPRECATED*/
/* srccopy stretchblt*/
static void
linear8_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, int op)
{
	ADDR8	dst;
	ADDR8	src;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	i, ymax;
	int	row_pos, row_inc;
	int	col_pos, col_inc;
	unsigned char pixel = 0;

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

		dst = ((ADDR8)dstpsd->addr) + dstx + dsty*dlinelen;
		src = ((ADDR8)srcpsd->addr) + srcx + (srcy-1)*slinelen;

		/* copy a row of pixels*/
		col_pos = 0x10000;
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
#endif /* DEPRECATED*/

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

/* Blit a 8-bit image.
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
linear8_stretchblitex(PSD dstpsd,
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
			 int op)
{
	/* Pointer to the current pixel in the source image */
	unsigned char *RESTRICT src_ptr;

	/* Pointer to x=xs1 on the next line in the source image */
	unsigned char *RESTRICT next_src_ptr;

	/* Pointer to the current pixel in the dest image */
	unsigned char *RESTRICT dest_ptr;

	/* Pointer to x=xd1 on the next line in the dest image */
	unsigned char *next_dest_ptr;

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

	/* DPRINTF("Nano-X: linear8_stretchflipblit( dest=(%d,%d) %dx%d )\n",
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
	x_error_step_normal = ABS(x_step_fraction) - ABS(src_x_step_normal) * x_denominator;

	src_y_step_normal = y_step_fraction / y_denominator;
	src_y_step_one = SIGN(y_step_fraction) * srcpsd->linelen;
	y_error_step_normal = ABS(y_step_fraction) - ABS(src_y_step_normal) * y_denominator;
	src_y_step_normal *= srcpsd->linelen;

	/* DPRINTF("linear8_stretchblitex: X: One step=%d, err-=%d; normal step=%d, err+=%d\n"
		"Y: One step=%d, err-=%d; normal step=%d, err+=%d\n",
	   src_x_step_one, x_denominator, src_x_step_normal, x_error_step_normal,
	   src_y_step_one, y_denominator, src_y_step_normal, y_error_step_normal);
	 */

	/* Pointer to the first source pixel */
	next_src_ptr = ((unsigned char *) srcpsd->addr) + src_y_start * srcpsd->linelen + src_x_start;

	/* Cache the width of a scanline in dest */
	dest_y_step = dstpsd->linelen;

	/* Pointer to the first dest pixel */
	next_dest_ptr = ((unsigned char *) dstpsd->addr) + (dest_y_start * dest_y_step) + dest_x_start;

	/*
	 * Note: The MWROP_SRC case below is a simple expansion of the
	 * default case.  It can be removed without significant speed
	 * penalty if you need to reduce code size.
	 *
	 * The MWROP_CLEAR case could be removed.  But it is a large
	 * speed increase for a small quantity of code.
	 */
	switch (op) {
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
				applyOp(op, *src_ptr, dest_ptr, ADDR8);
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

/* FIXME create lookup table whenever palette changed*/
int
init_alpha_lookup(void)
{
	int	i, a;
	int	r, g, b;
	extern MWPALENTRY gr_palette[256];

	if(!alpha_to_rgb)
		alpha_to_rgb = (unsigned short *)malloc(sizeof(unsigned short)*32*256);
	if(!rgb_to_palindex)
		rgb_to_palindex = (unsigned char *)malloc(sizeof(unsigned char)*32*32*32);
	if(!rgb_to_palindex || !alpha_to_rgb)
		return 0;

	/*
	 * Precompute alpha to rgb lookup by premultiplying
	 * each palette rgb value by each possible alpha
	 * and storing it as RGB555.
	 */
	for(i=0; i<256; ++i) {
		MWPALENTRY *p = &gr_palette[i];
		for(a=0; a<32; ++a) {
			alpha_to_rgb[(a<<8)+i] =
				(((p->r * a / 31)>>3) << 10) |
				(((p->g * a / 31)>>3) << 5) |
				 ((p->b * a / 31)>>3);
		}
	}

	/*
	 * Precompute RGB555 to palette index table by
	 * finding the nearest palette index for all RGB555 colors.
	 */
	for(r=0; r<32; ++r) {
		for(g=0; g<32; ++g)
			for(b=0; b<32; ++b)
				rgb_to_palindex[ (r<<10)|(g<<5)|b] =
					GdFindNearestColor(gr_palette, 256, MWRGB(r<<3, g<<3, b<<3));
	}
	return 1;
}

#if MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST
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
 *       usebg                 If set, bg_color is used.  If zero,
 *                                then '0' bits are transparent.
 */
static void
linear8_drawarea_bitmap_bytes_lsb_first(PSD psd, driver_gc_t * gc)
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
#define MWI_IS_BIT_BEFORE_OR_EQUAL(a,b) ((a) <= (b))
#define MWI_ADVANCE_BIT(target) ((target) <<= 1)
#define MWI_BIT_NO(n) (0x01 << (n))

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
	uint32_t fg, bg;
	int first_byte, last_byte;
	int size_main;
	int t, y;
	unsigned int advance_src, advance_dst;
	ADDR8 src;
	ADDR8 dst;

	/* The bit in the first byte, which corresponds to the leftmost pixel. */
	prefix_first_bit = MWI_BIT_NO(gc->srcx & 7);

	/* The bit in the last byte, which corresponds to the rightmost pixel. */
	postfix_last_bit = MWI_BIT_NO((gc->srcx + gc->width - 1) & 7);

	/* The index into each scanline of the first byte to use. */
	first_byte = gc->srcx >> 3;

	/* The index into each scanline of the last byte to use. */
	last_byte = (gc->srcx + gc->width - 1) >> 3;

	src = ((ADDR8) gc->data) + gc->src_linelen * gc->srcy + first_byte;
	dst = ((ADDR8) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
	fg = gc->fg_color;
	bg = gc->bg_color;

	advance_src = gc->src_linelen - last_byte + first_byte - 1;
	advance_dst = psd->linelen - gc->width;

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
	} else if ((prefix_first_bit == MWI_FIRST_BIT) && (postfix_last_bit == MWI_LAST_BIT)) {
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

	if (gc->usebg) {
		for (y = 0; y < gc->height; y++) {

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask; MWI_ADVANCE_BIT(mask)) {
					*dst++ = (mask & bitmap_byte) ? fg : bg;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				*dst++ = (MWI_BIT_NO(0) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(1) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(2) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(3) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(4) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(5) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(6) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(7) & bitmap_byte) ? fg : bg;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask, postfix_last_bit); MWI_ADVANCE_BIT(mask)) {
						*dst++ = (mask & bitmap_byte) ? fg : bg;
				}
			}

			src += advance_src;
			dst += advance_dst;
		}
	} else {
		for (y = 0; y < gc->height; y++) {

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask; MWI_ADVANCE_BIT(mask)) {
					if (mask & bitmap_byte)
						*dst = fg;
					dst++;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				if (MWI_BIT_NO(0) & bitmap_byte) dst[0] = fg;
				if (MWI_BIT_NO(1) & bitmap_byte) dst[1] = fg;
				if (MWI_BIT_NO(2) & bitmap_byte) dst[2] = fg;
				if (MWI_BIT_NO(3) & bitmap_byte) dst[3] = fg;
				if (MWI_BIT_NO(4) & bitmap_byte) dst[4] = fg;
				if (MWI_BIT_NO(5) & bitmap_byte) dst[5] = fg;
				if (MWI_BIT_NO(6) & bitmap_byte) dst[6] = fg;
				if (MWI_BIT_NO(7) & bitmap_byte) dst[7] = fg;

				dst += 8;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask, postfix_last_bit); MWI_ADVANCE_BIT(mask)) {
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
 *       data                   Pixmap data
 *       fg_color                 Color of a '1' bit
 *       bg_color                 Color of a '0' bit
 *       usebg                 If set, bg_color is used.  If zero,
 *                                then '0' bits are transparent.
 */
static void
linear8_drawarea_bitmap_bytes_msb_first(PSD psd, driver_gc_t * gc)
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
	unsigned char fg, bg;
	int first_byte, last_byte;
	int size_main;
	int t, y;
	unsigned int advance_src, advance_dst;
	ADDR8 src;
	ADDR8 dst;

	/* The bit in the first byte, which corresponds to the leftmost pixel. */
	prefix_first_bit = MWI_BIT_NO(gc->srcx & 7);

	/* The bit in the last byte, which corresponds to the rightmost pixel. */
	postfix_last_bit = MWI_BIT_NO((gc->srcx + gc->width - 1) & 7);

	/* The index into each scanline of the first byte to use. */
	first_byte = gc->srcx >> 3;

	/* The index into each scanline of the last byte to use. */
	last_byte = (gc->srcx + gc->width - 1) >> 3;

	src = ((ADDR8) gc->data) + gc->src_linelen * gc->srcy + first_byte;
	dst = ((ADDR8) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
	fg = gc->fg_color;
	bg = gc->bg_color;

	advance_src = gc->src_linelen - last_byte + first_byte - 1;
	advance_dst = psd->linelen - gc->width;

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
	} else if ((prefix_first_bit == MWI_FIRST_BIT) && (postfix_last_bit == MWI_LAST_BIT)) {
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

	if (gc->usebg) {
		for (y = 0; y < gc->height; y++) {

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask; MWI_ADVANCE_BIT(mask)) {
					*dst++ = (mask & bitmap_byte) ? fg : bg;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				*dst++ = (MWI_BIT_NO(0) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(1) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(2) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(3) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(4) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(5) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(6) & bitmap_byte) ? fg : bg;
				*dst++ = (MWI_BIT_NO(7) & bitmap_byte) ? fg : bg;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask, postfix_last_bit); MWI_ADVANCE_BIT(mask)) {
						*dst++ = (mask & bitmap_byte) ? fg : bg;
				}
			}

			src += advance_src;
			dst += advance_dst;
		}
	} else {
		for (y = 0; y < gc->height; y++) {

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask; MWI_ADVANCE_BIT(mask)) {
					if (mask & bitmap_byte)
						*dst = fg;
					dst++;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				if (MWI_BIT_NO(0) & bitmap_byte) dst[0] = fg;
				if (MWI_BIT_NO(1) & bitmap_byte) dst[1] = fg;
				if (MWI_BIT_NO(2) & bitmap_byte) dst[2] = fg;
				if (MWI_BIT_NO(3) & bitmap_byte) dst[3] = fg;
				if (MWI_BIT_NO(4) & bitmap_byte) dst[4] = fg;
				if (MWI_BIT_NO(5) & bitmap_byte) dst[5] = fg;
				if (MWI_BIT_NO(6) & bitmap_byte) dst[6] = fg;
				if (MWI_BIT_NO(7) & bitmap_byte) dst[7] = fg;

				dst += 8;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask, postfix_last_bit); MWI_ADVANCE_BIT(mask)) {
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

#if MW_FEATURE_PSDOP_ALPHACOL
static void
linear8_drawarea_alphacol(PSD psd, driver_gc_t * gc)
{
	ADDR8 dst, alpha;
	int x, y;
	unsigned int as;
	int src_row_step, dst_row_step;

	/* init alpha lookup tables*/
	if(!rgb_to_palindex) {
		if (!init_alpha_lookup())
			return;
	}

	alpha = ((ADDR8) gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
	dst = ((ADDR8) psd->addr) + psd->linelen * gc->dsty + gc->dstx;

	src_row_step = gc->src_linelen - gc->width;
	dst_row_step = psd->linelen - gc->width;

	DRAWON;
	for (y = 0; y < gc->height; y++) {
		for (x = 0; x < gc->width; x++) {
			if ((as = *alpha++) == 255)
				*dst++ = gc->fg_color;
			else if (as != 0) {
				/* Create 5 bit alpha value index for 256 color indexing*/

				/* Get source RGB555 value for source alpha value*/
				unsigned short s = alpha_to_rgb[((as >> 3) << 8) + gc->fg_color];

				/* Get destination RGB555 value for dest alpha value*/
				unsigned short d = alpha_to_rgb[(((as >> 3) ^ 31) << 8) + *dst];

				/* Add RGB values together and get closest palette index to it*/
				*dst++ = rgb_to_palindex[s + d];
			} else if(gc->usebg)		/* alpha 0 - draw bkgnd*/
				*dst++ = gc->bg_color;
			else
				++dst;
		}
		alpha += src_row_step;
		dst += dst_row_step;
	}
	DRAWOFF;
}
#endif /* MW_FEATURE_PSDOP_ALPHACOL */

static void
linear8_drawarea(PSD psd, driver_gc_t * gc)
{
	assert(psd->addr != 0);
	/*assert(gc->width <= gc->srcw); */
	assert(gc->dstx >= 0 && gc->dstx + gc->width <= psd->xres);
	/*assert(gc->dsty >= 0 && gc->dsty+gc->height <= psd->yres); */
	/*assert(gc->srcx >= 0 && gc->srcx+gc->width <= gc->srcw); */
	assert(gc->srcy >= 0);
	/*DPRINTF("linear8_drawarea op=%d dstx=%d dsty=%d\n", op, gc->dstx, gc->dsty);*/

	switch (gc->op) {
#if MW_FEATURE_PSDOP_ALPHACOL
	case PSDOP_ALPHACOL:
		linear8_drawarea_alphacol(psd, gc);
		break;
#endif

#if MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST
	case PSDOP_BITMAP_BYTES_LSB_FIRST:
		linear8_drawarea_bitmap_bytes_lsb_first(psd, gc);
		break;
#endif

#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
	case PSDOP_BITMAP_BYTES_MSB_FIRST:
		linear8_drawarea_bitmap_bytes_msb_first(psd, gc);
		break;
#endif

	}
}

SUBDRIVER fblinear8 = {
	linear8_init,
	linear8_drawpixel,
	linear8_readpixel,
	linear8_drawhorzline,
	linear8_drawvertline,
	gen_fillrect,
	linear8_blit,
	linear8_drawarea,
	linear8_stretchblitex
};
