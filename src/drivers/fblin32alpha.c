/*
 * Copyright (c) 1999, 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * 32bpp (With Alpha) Linear Video Driver for Microwindows
 *
 * Written by Koninklijke Philips Electronics N.V.
 * Based on the existing 32bpp (no alpha) driver:
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 *
 * Portions contributed by Koninklijke Philips Electronics N.V.
 * These portions are Copyright 2002, 2003 Koninklijke Philips Electronics
 * N.V.  All Rights Reserved.  These portions are licensed under the
 * terms of the Mozilla Public License, version 1.1, or, at your
 * option, the GNU General Public License version 2.0.  Please see
 * the file "ChangeLog" for documentation regarding these
 * contributions.
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "swap.h"
#include "device.h"
#include "fb.h"

/* It's a lot easier to treat the buffer as an array of bytes when
 * alpha blending, and an array of uint32 otherwise.  However,
 * without some care, that would not be portable due to endian
 * issues.
 *
 * These macros help solve this problem.  They define which bytes
 * in memory correspond to which field.
 */
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
/* little endian ABGR*/
#define OFFSET_A 3
#define OFFSET_B 2
#define OFFSET_G 1
#define OFFSET_R 0
#define COLOR2PIXEL COLOR2PIXELABGR
#else

/* ARGB*/
#define COLOR2PIXEL COLOR2PIXEL8888

#if MW_CPU_BIG_ENDIAN
#define OFFSET_A 0
#define OFFSET_R 1
#define OFFSET_G 2
#define OFFSET_B 3
#else
//FIXME check, this may not an error for alpha channel passed in blit
#define OFFSET_A 3
#define OFFSET_R 2
#define OFFSET_G 1
#define OFFSET_B 0
#endif

#endif

static int
linear32a_init(PSD psd)
{
#ifndef NDEBUG
	/*
	 * Check the endian mode of the CPU matches what was specified
	 * in the config file at compile time.
	 *
	 * (Also validates that the various #defines are internally
	 *  consistent).
	 */
	uint32_t endian_check = COLOR2PIXEL(MWARGB(1, 2, 3, 4));
	assert(((char *) (&endian_check))[OFFSET_A] == 1);
	assert(((char *) (&endian_check))[OFFSET_R] == 2);
	assert(((char *) (&endian_check))[OFFSET_G] == 3);
	assert(((char *) (&endian_check))[OFFSET_B] == 4);
#endif

	if (!psd->size) {
		psd->size = psd->yres * psd->linelen;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32 */
		psd->linelen /= 4;
	}
	return 1;
}

/* Calc linelen and mmap size, return 0 on fail*/
/* Set pixel at x, y, to pixelval c*/
static void
linear32a_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register ADDR32 addr = ((ADDR32)psd->addr) + x + y * psd->linelen;
	uint32_t psr, psg, psb, as, pd;
#if DEBUG
	assert(addr != 0);
	assert(x >= 0 && x < psd->xres);
	assert(y >= 0 && y < psd->yres);
#endif

	DRAWON;
	if (gr_mode == MWROP_COPY)
		*addr = c;
	else if (gr_mode == MWROP_SRC_OVER) {
			if ((as = (c >> 24)) == 255)
				*addr = c;
			else if (as != 0) {
				psr = c & 0x00FF0000UL;
				psg = c & 0x0000FF00UL;
				psb = c & 0x000000FFUL;
				/*
				 * Flip the direction of alpha, so it's
				 * backwards from it's usual meaning.
				 * This is because the equation below is most
				 * easily written with source and dest interchanged
				 * (since we can split ps into it's components
				 * before we enter the loop)
				 *
				 * Alpha is then adjusted +1 for 92% accurate blend
				 * with one multiply and shift.
				 */
				as = 255 - as + 1;
				pd = *addr;
				*addr = 
				      ((((((pd & 0x00FF0000UL) - psr) * as) >> 8) + psr) & 0x00FF0000UL)
					| ((((((pd & 0x0000FF00UL) - psg) * as) >> 8) + psg) & 0x0000FF00UL)
					| ((((((pd & 0x000000FFUL) - psb) * as) >> 8) + psb) & 0x000000FFUL)
					| ((((256-as) << 24) + ((pd & 0xFF000000UL) >> 8) * as) & 0xFF000000UL);
			}
	} else if (gr_mode <= MWROP_SIMPLE_MAX)
		applyOp(gr_mode, c, addr, ADDR32);
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear32a_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
#if DEBUG
	assert(psd->addr != 0);
	assert(x >= 0 && x < psd->xres);
	assert(y >= 0 && y < psd->yres);
#endif
	return ((ADDR32)psd->addr)[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear32a_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register ADDR32 addr = ((ADDR32)psd->addr) + x1 + y * psd->linelen;
	uint32_t psr, psg, psb, as, pd;
	uint32_t cache_input, cache_output;

#if DEBUG
	assert(psd->addr != 0);
	assert(x1 >= 0 && x1 < psd->xres);
	assert(x2 >= 0 && x2 < psd->xres);
	assert(x2 >= x1);
	assert(y >= 0 && y < psd->yres);
#endif

	DRAWON;
	if (gr_mode == MWROP_COPY) {
		/* FIXME: memsetl(dst, c, x2-x1+1) */
		while (x1++ <= x2)
			*addr++ = c;
	} else if (gr_mode == MWROP_SRC_OVER) {
			if ((as = (c >> 24)) == 255) {
				while (x1++ <= x2)
					*addr++ = c;
			} else if (as != 0) {
				psr = c & 0x00FF0000UL;
				psg = c & 0x0000FF00UL;
				psb = c & 0x000000FFUL;
				/*
				 * Flip the direction of alpha, so it's
				 * backwards from it's usual meaning.
				 * This is because the equation below is most
				 * easily written with source and dest interchanged
				 * (since we can split ps into it's components
				 * before we enter the loop)
				 *
				 * Alpha is then adjusted +1 for 92% accurate blend
				 * with one multiply and shift.
				 */
				as = 255 - as + 1;
				pd = *addr;
				cache_input = pd;
				*addr++ = cache_output = 
				      ((((((pd & 0x00FF0000UL) - psr) * as) >> 8) + psr) & 0x00FF0000UL)
					| ((((((pd & 0x0000FF00UL) - psg) * as) >> 8) + psg) & 0x0000FF00UL)
					| ((((((pd & 0x000000FFUL) - psb) * as) >> 8) + psb) & 0x000000FFUL)
					| ((((256-as) << 24) + ((pd & 0xFF000000UL) >> 8) * as) & 0xFF000000UL);

				while (++x1 <= x2) {
					pd = *addr;
					if (cache_input != pd) {
						cache_input = pd;
						cache_output =
				      ((((((pd & 0x00FF0000UL) - psr) * as) >> 8) + psr) & 0x00FF0000UL)
					| ((((((pd & 0x0000FF00UL) - psg) * as) >> 8) + psg) & 0x0000FF00UL)
					| ((((((pd & 0x000000FFUL) - psb) * as) >> 8) + psb) & 0x000000FFUL)
					| ((((256-as) << 24) + ((pd & 0xFF000000UL) >> 8) * as) & 0xFF000000UL);
					}
					*addr++ = cache_output;
				}
			}
	} else if (gr_mode <= MWROP_SIMPLE_MAX) {
		while (x1++ <= x2) {
			applyOp(gr_mode, c, addr, ADDR32);
			++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear32a_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int linelen = psd->linelen;
	register ADDR32 addr = ((ADDR32)psd->addr) + x + y1 * linelen;
	uint32_t psr, psg, psb, as, pd;
	uint32_t cache_input, cache_output;
#if DEBUG
	assert(psd->addr != 0);
	assert(x >= 0 && x < psd->xres);
	assert(y1 >= 0 && y1 < psd->yres);
	assert(y2 >= 0 && y2 < psd->yres);
	assert(y2 >= y1);
#endif

	DRAWON;
	if (gr_mode == MWROP_COPY) {
		while (y1++ <= y2) {
			*addr = c;
			addr += linelen;
		}
	} else if (gr_mode == MWROP_SRC_OVER) {
			if ((as = (c >> 24)) == 255) {
				while (y1++ <= y2) {
					*addr = c;
					addr += linelen;
				}
			} else if (as != 0) {
				psr = c & 0x00FF0000UL;
				psg = c & 0x0000FF00UL;
				psb = c & 0x000000FFUL;
				/*
				 * Flip the direction of alpha, so it's
				 * backwards from it's usual meaning.
				 * This is because the equation below is most
				 * easily written with source and dest interchanged
				 * (since we can split ps into it's components
				 * before we enter the loop)
				 *
				 * Alpha is then adjusted +1 for 92% accurate blend
				 * with one multiply and shift.
				 */
				as = 255 - as + 1;
				pd = *addr;
				cache_input = pd;
				*addr = cache_output =
				      ((((((pd & 0x00FF0000UL) - psr) * as) >> 8) + psr) & 0x00FF0000UL)
					| ((((((pd & 0x0000FF00UL) - psg) * as) >> 8) + psg) & 0x0000FF00UL)
					| ((((((pd & 0x000000FFUL) - psb) * as) >> 8) + psb) & 0x000000FFUL)
					| ((((256-as) << 24) + ((pd & 0xFF000000UL) >> 8) * as) & 0xFF000000UL);
				addr += linelen;

				while (++y1 <= y2) {
					pd = *addr;
					if (cache_input != pd) {
						cache_input = pd;
						*addr = cache_output =
				      ((((((pd & 0x00FF0000UL) - psr) * as) >> 8) + psr) & 0x00FF0000UL)
					| ((((((pd & 0x0000FF00UL) - psg) * as) >> 8) + psg) & 0x0000FF00UL)
					| ((((((pd & 0x000000FFUL) - psb) * as) >> 8) + psb) & 0x000000FFUL)
					| ((((256-as) << 24) + ((pd & 0xFF000000UL) >> 8) * as) & 0xFF000000UL);
					}
					addr += linelen;
				}
			}
	} else if (gr_mode <= MWROP_SIMPLE_MAX) {
		while (y1++ <= y2) {
			applyOp(gr_mode, c, addr, ADDR32);
			addr += linelen;
		}
	}
	DRAWOFF;
}

/* srccopy bitblt*/
static void
linear32a_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	       PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	ADDR32 dst = dstpsd->addr;
	ADDR32 src = srcpsd->addr;
	ADDR8 dst8, src8;
	int i;
	int dlinelen = dstpsd->linelen;
	int slinelen = srcpsd->linelen;
	int dlinelen_minus_w4;
	int slinelen_minus_w4;

	assert(dst != 0);
	assert(dstx >= 0 && dstx < dstpsd->xres);
	assert(dsty >= 0 && dsty < dstpsd->yres);
	assert(w > 0);
	assert(h > 0);
	assert(src != 0);
	assert(srcx >= 0 && srcx < srcpsd->xres);
	assert(srcy >= 0 && srcy < srcpsd->yres);
	assert(dstx + w <= dstpsd->xres);
	assert(dsty + h <= dstpsd->yres);
	assert(srcx + w <= srcpsd->xres);
	assert(srcy + h <= srcpsd->yres);

	DRAWON;
	dst += dstx + dsty * dlinelen;
	src += srcx + srcy * slinelen;

	if (op == MWROP_BLENDCONSTANT) {
	uint32_t alpha = 150;

	dlinelen_minus_w4 = (dlinelen - w) * 4;
	slinelen_minus_w4 = (slinelen - w) * 4;
	src8 = (ADDR8) src;
	dst8 = (ADDR8) dst;

	while (--h >= 0) {
		for (i = 0; i < w; ++i) {
			if (alpha == 255) {
				dst8[0] = src8[0];
				dst8[1] = src8[1];
				dst8[2] = src8[2];
				dst8[3] = src8[3];
			} else if (alpha != 0) {
 				// d += muldiv255(a, s - d)
				dst8[OFFSET_B] += muldiv255(alpha, src8[OFFSET_B] - dst8[OFFSET_B]);
				dst8[OFFSET_G] += muldiv255(alpha, src8[OFFSET_G] - dst8[OFFSET_G]);
				dst8[OFFSET_R] += muldiv255(alpha, src8[OFFSET_R] - dst8[OFFSET_R]);

				//d = muldiv255(d, 255 - a) + a
				dst8[OFFSET_A] = muldiv255(dst8[OFFSET_A], 255 - alpha) + alpha;
			}
			dst8 += 4;
			src8 += 4;
		}
		dst8 += dlinelen_minus_w4;
		src8 += slinelen_minus_w4;
	}
	} else if (op == MWROP_COPY) {
		/* copy from bottom up if dst in src rectangle */
		/* memmove is used to handle x case */
		if (srcy < dsty) {
			src += (h - 1) * slinelen;
			dst += (h - 1) * dlinelen;
			slinelen *= -1;
			dlinelen *= -1;
		}
		while (--h >= 0) {
			/* a _fast_ memmove is a _must_ in this routine */
			memmove(dst, src, w << 2);
			dst += dlinelen;
			src += slinelen;
		}
	} else if (op == MWROP_SRC_OVER) {
		src8 = (ADDR8) src;
		dst8 = (ADDR8) dst;
		while (h--) {
			for (i = w; --i >= 0;) {
				register uint32_t as;

				if ((as = src8[OFFSET_A]) == 255) {	//FIXME should this be constant offset?
					dst8[0] = src8[0];
					dst8[1] = src8[1];
					dst8[2] = src8[2];
					dst8[3] = src8[3];
				} else if (as != 0) {
 					// d += muldiv255(a, s - d)
					dst8[OFFSET_B] += muldiv255(as, src8[OFFSET_B] - dst8[OFFSET_B]);
					dst8[OFFSET_G] += muldiv255(as, src8[OFFSET_G] - dst8[OFFSET_G]);
					dst8[OFFSET_R] += muldiv255(as, src8[OFFSET_R] - dst8[OFFSET_R]);

					//d = muldiv255(d, 255 - a) + a;
					dst8[OFFSET_A] = muldiv255(dst8[OFFSET_A], 255 - as) + as;
				}
				// src alpha 0, leave dst alpha as is
				src8 += 4;
				dst8 += 4;
			}
			dst8 += (dlinelen - w) * 4;
			src8 += (slinelen - w) * 4;
		}
	} else if (op <= MWROP_SIMPLE_MAX) {
		while (--h >= 0) {
			applyOp4(w, op, src, dst, ADDR32);
			dst += dlinelen - w;
			src += slinelen - w;
		}
	}
	DRAWOFF;
}

#if 0000  /* DEPRECATED*/
/* srccopy stretchblt*/
static void
linear32a_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
		      MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy,
		      MWCOORD srcw, MWCOORD srch, int op)
{
	ADDR32 dst;
	ADDR32 src;
	int dlinelen = dstpsd->linelen;
	int slinelen = srcpsd->linelen;
	int i, ymax;
	int row_pos, row_inc;
	int col_pos, col_inc;
	uint32_t pixel = 0;

	assert(dstpsd->addr != 0);
	assert(dstx >= 0 && dstx < dstpsd->xres);
	assert(dsty >= 0 && dsty < dstpsd->yres);
	assert(dstw > 0);
	assert(dsth > 0);
	assert(srcpsd->addr != 0);
	assert(srcx >= 0 && srcx < srcpsd->xres);
	assert(srcy >= 0 && srcy < srcpsd->yres);
	assert(srcw > 0);
	assert(srch > 0);
	assert(dstx + dstw <= dstpsd->xres);
	assert(dsty + dsth <= dstpsd->yres);
	assert(srcx + srcw <= srcpsd->xres);
	assert(srcy + srch <= srcpsd->yres);

	DRAWON;
	row_pos = 0x10000;
	row_inc = (srch << 16) / dsth;

	/* stretch blit using integer ratio between src/dst height/width */
	for (ymax = dsty + dsth; dsty < ymax; ++dsty) {

		/* find source y position */
		while (row_pos >= 0x10000L) {
			++srcy;
			row_pos -= 0x10000L;
		}

		dst = (ADDR32) dstpsd->addr + dstx + dsty * dlinelen;
		src = (ADDR32) srcpsd->addr + srcx + (srcy - 1) * slinelen;

		/* copy a row of pixels */
		col_pos = 0x10000;
		col_inc = (srcw << 16) / dstw;
		for (i = 0; i < dstw; ++i) {
			/* get source x pixel */
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

/* Blit a 32-bit image.
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
linear32a_stretchblitex(PSD dstpsd,
			PSD srcpsd,
			MWCOORD dest_x_start,
			MWCOORD dest_y_start,
			MWCOORD width,
			MWCOORD height,
			int x_denominator,
			int y_denominator,
			int src_x_fraction,
			int src_y_fraction,
			int x_step_fraction, int y_step_fraction, int op)
{
	/* Pointer to the current pixel in the source image */
	uint32_t *RESTRICT src_ptr;

	/* Pointer to x=xs1 on the next line in the source image */
	uint32_t *RESTRICT next_src_ptr;

	/* Pointer to the current pixel in the dest image */
	uint32_t *RESTRICT dest_ptr;

	/* Pointer to x=xd1 on the next line in the dest image */
	uint32_t *next_dest_ptr;

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
	   dest_x_start, dest_y_start, width, height); */

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

	/* DPRINTF("linear32alpha_stretchblitex: X: One step=%d, err-=%d; normal step=%d, err+=%d\n"
		"Y: One step=%d, err-=%d; normal step=%d, err+=%d\n",
	   src_x_step_one, x_denominator, src_x_step_normal, x_error_step_normal,
	   src_y_step_one, y_denominator, src_y_step_normal, y_error_step_normal);
	 */

	/* Pointer to the first source pixel */
	next_src_ptr = ((uint32_t *) srcpsd->addr) +
		src_y_start * srcpsd->linelen + src_x_start;

	/* Cache the width of a scanline in dest */
	dest_y_step = dstpsd->linelen;

	/* Pointer to the first dest pixel */
	next_dest_ptr = ((uint32_t *) dstpsd->addr) + (dest_y_start * dest_y_step) + dest_x_start;

	/*
	 * Note: The MWROP_SRC and MWROP_XOR_FGBG cases below are simple
	 * expansions of the default case.  They can be removed without
	 * significant speed penalty if you need to reduce code size.
	 *
	 * The SRC_OVER case cannot be removed (since applyOp doesn't
	 * handle it correctly).
	 *
	 * The MWROP_CLEAR case could be removed.  But it is a large
	 * speed increase for a small quantity of code.
	 *
	 * FIXME Porter-Duff rules other than SRC_OVER not handled!!
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

	case MWROP_SRC_OVER:
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
				uint32_t orig;
				uint32_t c = *src_ptr;
				int a = (c >> 24) & 0xFF;

				if (a == 255) {
					*dest_ptr = c;
				} else if (a != 0) {
					int s, d;
					uint32_t result;
					
					orig = *dest_ptr;
					
					//FIXME rewrite with muldiv
					s = (int)(c    >> 16) & 0xFF;
					d = (int)(orig >> 16) & 0xFF;
					result = ((uint32_t)((((s - d) * a) >> 8) + d) << 16);

					s = (int)(c    >> 8) & 0xFF;
					d = (int)(orig >> 8) & 0xFF;
					result |= ((uint32_t)((((s - d) * a) >> 8) + d) << 8);

					s = (int)c    & 0xFF;
					d = (int)orig & 0xFF;
					result |= ((uint32_t)(((s - d) * a) >> 8) + d);

					d = (int)(orig >> 24) & 0xFF;
					result |= ((uint32_t)(a + ((d * (256 - a)) >> 8)) << 24);
					*dest_ptr = result;
				}

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

	case MWROP_XOR_FGBG:
		y_count = height;
		while (y_count-- > 0) {
			src_ptr = next_src_ptr;
			dest_ptr = next_dest_ptr;

			x_error = x_error_start;

			x_count = width;
			while (x_count-- > 0) {
				*dest_ptr++ ^= *src_ptr ^ gr_background;

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
				applyOp(op, *src_ptr, dest_ptr, ADDR32);
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
linear32a_drawarea_bitmap_bytes_lsb_first(PSD psd, driver_gc_t * gc)
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
	ADDR32 dst;

	/* The bit in the first byte, which corresponds to the leftmost pixel. */
	prefix_first_bit = MWI_BIT_NO(gc->srcx & 7);

	/* The bit in the last byte, which corresponds to the rightmost pixel. */
	postfix_last_bit = MWI_BIT_NO((gc->srcx + gc->width - 1) & 7);

	/* The index into each scanline of the first byte to use. */
	first_byte = gc->srcx >> 3;

	/* The index into each scanline of the last byte to use. */
	last_byte = (gc->srcx + gc->width - 1) >> 3;

	src = ((ADDR8) gc->data) + gc->src_linelen * gc->srcy + first_byte;
	dst = ((ADDR32) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
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
linear32a_drawarea_bitmap_bytes_msb_first(PSD psd, driver_gc_t * gc)
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
	uint32_t fg, bg;
	int first_byte, last_byte;
	int size_main;
	int t, y;
	unsigned int advance_src, advance_dst;
	ADDR8 src;
	ADDR32 dst;

	/* The bit in the first byte, which corresponds to the leftmost pixel. */
	prefix_first_bit = MWI_BIT_NO(gc->srcx & 7);

	/* The bit in the last byte, which corresponds to the rightmost pixel. */
	postfix_last_bit = MWI_BIT_NO((gc->srcx + gc->width - 1) & 7);

	/* The index into each scanline of the first byte to use. */
	first_byte = gc->srcx >> 3;

	/* The index into each scanline of the last byte to use. */
	last_byte = (gc->srcx + gc->width - 1) >> 3;

	src = ((ADDR8) gc->data) + gc->src_linelen * gc->srcy + first_byte;
	dst = ((ADDR32) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
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
				for (mask = prefix_first_bit; mask;
				     MWI_ADVANCE_BIT(mask)) {
					*dst++ = (mask & bitmap_byte) ? fg :
						bg;
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
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask, postfix_last_bit);
				     MWI_ADVANCE_BIT(mask)) {
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


static void
linear32a_drawarea_alphacol(PSD psd, driver_gc_t * gc)
{
	ADDR32 dst;
	ADDR8 alpha;
	uint32_t psr, psg, psb, as, ps, pd;
	int x, y;
	int src_row_step, dst_row_step;

	alpha = ((ADDR8) gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
	dst = ((ADDR32) psd->addr) + psd->linelen * gc->dsty + gc->dstx;
	ps = gc->fg_color;
	psr = ps & 0x00FF0000UL;
	psg = ps & 0x0000FF00UL;
	psb = ps & 0x000000FFUL;

	src_row_step = gc->src_linelen - gc->width;
	dst_row_step = psd->linelen - gc->width;

	DRAWON;
	for (y = 0; y < gc->height; y++) {
		for (x = 0; x < gc->width; x++) {
			if ((as = *alpha++) == 255)
				*dst++ = ps;
			else if (as != 0) {
				/*
				 * Flip the direction of alpha, so it's
				 * backwards from it's usual meaning.
				 * This is because the equation below is most
				 * easily written with source and dest interchanged
				 * (since we can split ps into it's components
				 * before we enter the loop)
				 *
				 * Alpha is then adjusted +1 for 92% accurate blend
				 * with one multiply and shift.
				 */
				as = 255 - as + 1;
				pd = gc->usebg? gc->bg_color: *dst;
				*dst++ =
					  ((((((pd & 0x00FF0000UL) - psr) * as) >> 8) + psr) & 0x00FF0000UL)
					| ((((((pd & 0x0000FF00UL) - psg) * as) >> 8) + psg) & 0x0000FF00UL)
					| ((((((pd & 0x000000FFUL) - psb) * as) >> 8) + psb) & 0x000000FFUL)
					| ((((256-as) << 24) + ((pd & 0xFF000000UL) >> 8) * as) & 0xFF000000UL);
			} else if(gc->usebg)		/* alpha is 0 - draw bkgnd*/
				*dst++ = gc->bg_color;
			else
				++dst;
		}
		alpha += src_row_step;
		dst += dst_row_step;
	}
	DRAWOFF;
}

static void
linear32a_drawarea(PSD psd, driver_gc_t * gc)
{
	assert(psd->addr != 0);
	/*assert(gc->width <= gc->srcw); */
	assert(gc->dstx >= 0 && gc->dstx + gc->width <= psd->xres);
	/*assert(gc->dsty >= 0 && gc->dsty+gc->height <= psd->yres); */
	/*assert(gc->srcx >= 0 && gc->srcx+gc->width <= gc->srcw); */
	assert(gc->srcy >= 0);
	/*DPRINTF("linear32a_drawarea op=%d dstx=%d dsty=%d\n", op, gc->dstx, gc->dsty);*/

	switch (gc->op) {
	case PSDOP_ALPHACOL:
		linear32a_drawarea_alphacol(psd, gc);
		break;

	case PSDOP_BITMAP_BYTES_LSB_FIRST:
		linear32a_drawarea_bitmap_bytes_lsb_first(psd, gc);
		break;

	case PSDOP_BITMAP_BYTES_MSB_FIRST:
		linear32a_drawarea_bitmap_bytes_msb_first(psd, gc);
		break;
	}
}

SUBDRIVER fblinear32alpha = {
	linear32a_init,
	linear32a_drawpixel,
	linear32a_readpixel,
	linear32a_drawhorzline,
	linear32a_drawvertline,
	gen_fillrect,
	linear32a_blit,
	linear32a_drawarea,
	linear32a_stretchblitex
};
