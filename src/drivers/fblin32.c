/*
 * Copyright (c) 1999, 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * 32bpp Linear Video Driver for Microwindows (BGRA or RGBA byte order)
 * Writes memory image: |B|G|R|A| LE 0xARGB BE 0xBGRA MWPF_TRUECOLOR8888
 * Writes memory image: |R|G|B|A| LE 0xABGR BE 0xRGBA MWPF_TRUECOLORABGR
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "convblit.h"
#include "fb.h"

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear32_init(PSD psd)
{
	if (!psd->size) {
		psd->size = psd->yres * psd->linelen;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32*/
		psd->linelen /= 4;
	}
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear32_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	DRAWON;
	if (gr_mode == MWROP_COPY)
		((ADDR32)psd->addr)[x + y * psd->linelen] = c;
	else
		applyOp(gr_mode, c, &((ADDR32)psd->addr)[x + y * psd->linelen], ADDR32);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y, 1, 1);
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear32_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return ((ADDR32)psd->addr)[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear32_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register ADDR32	addr = ((ADDR32)psd->addr) + x1 + y * psd->linelen;
	MWCOORD X1 = x1;
#if DEBUG
	assert (psd->addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY) {
		while(x1++ <= x2)		/* memsetl(addr, c, x2-x1+1)*/
			*addr++ = c;
	} else {
		while(x1++ <= x2) {
			applyOp(gr_mode, c, addr, ADDR32);
			++addr;
		}
	}
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, X1, y, x2-X1+1, 1);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear32_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	linelen = psd->linelen;
	MWCOORD Y1 = y1;
	register ADDR32	addr = ((ADDR32)psd->addr) + x + y1 * linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY) {
		while(y1++ <= y2) {
			*addr = c;
			addr += linelen;
		}
	} else {
		while(y1++ <= y2) {
			applyOp(gr_mode, c, addr, ADDR32);
			addr += linelen;
		}
	}

	if (psd->Update)
		psd->Update(psd, x, Y1, 1, y2-Y1+1);
	DRAWOFF;
}

/* srccopy bitblt*/
static void
linear32_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	ADDR8	dst8, src8;
	ADDR32	dst = dstpsd->addr;
	ADDR32	src = srcpsd->addr;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	dlinelen_minus_w4;
	int	slinelen_minus_w4;
	int	H = h;
#if DEBUG
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
#endif
	dst += dstx + dsty * dlinelen;
	src += srcx + srcy * slinelen;

	DRAWON;

	if (op == MWROP_BLENDCONSTANT) {
	uint32_t alpha = 150;

	dlinelen_minus_w4 = (dlinelen - w) * 4;
	slinelen_minus_w4 = (slinelen - w) * 4;
	src8 = (ADDR8)src;
	dst8 = (ADDR8)dst;

	while(--h >= 0) {
		for(i=0; i<w; ++i) {
			if (alpha != 0) {
#if 0
 				// d = muldiv255(a, d - s) + s;
				uint32_t ssa = 255 - alpha;
				uint32_t ps = *src8++;
				*dst8 = muldiv255(ssa, *dst8 - ps) + ps;
				++dst8;
				ps = *src8++;
				*dst8 = muldiv255(ssa, *dst8 - ps) + ps;
				++dst8;
				ps = *src8++;
				*dst8 = muldiv255(ssa, *dst8 - ps) + ps;
				++dst8;

				//d = muldiv255(d, 255 - a) + a;
				*dst8 = muldiv255(*dst8, 255 - alpha) + alpha;
				++dst8;
				++src8;
#endif
#if 1
 				// d = muldiv255(a, s - d) + d
				uint32_t pd = *dst8;
				*dst8++ = muldiv255(alpha, *src8++ - pd) + pd;
				pd = *dst8;
				*dst8++ = muldiv255(alpha, *src8++ - pd) + pd;
				pd = *dst8;
				*dst8++ = muldiv255(alpha, *src8++ - pd) + pd;

				// d += muldiv255(a, 255 - d)
				*dst8 += muldiv255(alpha, 255 - *dst);
				++dst8;
				++src8;
#endif
			} else {
				// src alpha 0, leave dst alpha as is
				dst8 += 4;
				src8 += 4;
			}
		}
		dst8 += dlinelen_minus_w4;
		src8 += slinelen_minus_w4;
	}

	} else if (op == MWROP_COPY) {
		/* copy from bottom up if dst in src rectangle*/
		/* memmove is used to handle x case*/
		if (srcy < dsty) {
			src += (h-1) * slinelen;
			dst += (h-1) * dlinelen;
			slinelen *= -1;
			dlinelen *= -1;
		}
		while(--h >= 0) {
			/* a _fast_ memmove is a _must_ in this routine*/
			memmove(dst, src, w<<2);
			dst += dlinelen;
			src += slinelen;
		}
	} else if (op == MWROP_SRC_OVER) {
		src8 = (ADDR8)src;
		dst8 = (ADDR8)dst;
		while (h--) {
			for (i = w; --i >= 0;) {
				register uint32_t as;

				if ((as = src8[3]) == 255) {	//FIXME constant ok only with BGRA and RGBA formats
					dst8[0] = src8[0];
					dst8[1] = src8[1];
					dst8[2] = src8[2];
					dst8[3] = src8[3];
					src8 += 4;
					dst8 += 4;
				} else if (as != 0) {
 					// d = muldiv255(a, s - d) + d
					register uint32_t pd = *dst8;
					*dst8++ = muldiv255(as, *src8++ - pd) + pd;
					pd = *dst8;
					*dst8++ = muldiv255(as, *src8++ - pd) + pd;
					pd = *dst8;
					*dst8++ = muldiv255(as, *src8++ - pd) + pd;

					//d = muldiv255(d, 255 - a) + a
					*dst8 = muldiv255(*dst8, 255 - as) + as;	// FIXME see above
					++dst8;
					++src8;
				} else {
					// src alpha 0, leave dst alpha as is
					src8 += 4;
					dst8 += 4;
				}
			}
			dst8 += (dlinelen - w) * 4;
			src8 += (slinelen - w) * 4;
		}
	} else {
		while (--h >= 0) {
			applyOp4(w, op, src, dst, ADDR32);					// FIXME see above
			dst += dlinelen - w;
			src += slinelen - w;
		}
	}

	if (dstpsd->Update)
		dstpsd->Update(dstpsd, dstx, dsty, w, H);
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
linear32_stretchblitex(PSD dstpsd,
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

	/* DPRINTF("linear32_stretchblitex: X: One step=%d, err-=%d; normal step=%d, err+=%d\n"
		"Y: One step=%d, err-=%d; normal step=%d, err+=%d\n",
	   src_x_step_one, x_denominator, src_x_step_normal, x_error_step_normal,
	   src_y_step_one, y_denominator, src_y_step_normal, y_error_step_normal);
	 */

	/* Pointer to the first source pixel */
	next_src_ptr = ((uint32_t *) srcpsd->addr) + src_y_start * srcpsd->linelen + src_x_start;

	/* Cache the width of a scanline in dest */
	dest_y_step = dstpsd->linelen;

	/* Pointer to the first dest pixel */
	next_dest_ptr = ((uint32_t *) dstpsd->addr) + (dest_y_start * dest_y_step) + dest_x_start;

	/*
	 * Note: The MWROP_SRC case below is a simple expansion of the
	 * default case.  It can be removed without significant speed
	 * penalty if you need to reduce code size.
	 *
	 * The MWROP_CLEAR case could be removed.  But it is a large
	 * speed increase for a small quantity of code.
	 */
	DRAWON;
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
	DRAWOFF;

	if (dstpsd->Update)
		dstpsd->Update(dstpsd, dest_x_start, dest_y_start, width, height);
}

static SUBDRIVER fblinear32_none = {
	linear32_init,
	linear32_drawpixel,
	linear32_readpixel,
	linear32_drawhorzline,
	linear32_drawvertline,
	gen_fillrect,
	linear32_blit,
	linear32_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgra,		/* ft2 non-alias*/
	convblit_copy_mask_mono_byte_lsb_bgra,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_bgra,		/* core/pcf non-alias*/
	convblit_blend_mask_alpha_byte_bgra,		/* ft2/t1 antialias*/
	convblit_srcover_rgba8888_bgra8888,			/* RGBA images w/alpha*/
	convblit_copy_rgb888_bgra8888				/* RGB images no alpha*/
};

static SUBDRIVER fblinear32_left = {
	NULL,
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	fbportrait_left_blit,
	fbportrait_left_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgra_left,
	convblit_copy_mask_mono_byte_lsb_bgra_left,
	convblit_copy_mask_mono_word_msb_bgra_left,
	convblit_blend_mask_alpha_byte_bgra_left,
	convblit_srcover_rgba8888_bgra8888_left,
	convblit_copy_rgb888_bgra8888_left
};

static SUBDRIVER fblinear32_right = {
	NULL,
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	fbportrait_right_blit,
	fbportrait_right_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgra_right,
	convblit_copy_mask_mono_byte_lsb_bgra_right,
	convblit_copy_mask_mono_word_msb_bgra_right,
	convblit_blend_mask_alpha_byte_bgra_right,
	convblit_srcover_rgba8888_bgra8888_right,
	convblit_copy_rgb888_bgra8888_right
};

static SUBDRIVER fblinear32_down = {
	NULL,
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	fbportrait_down_blit,
	fbportrait_down_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgra_down,
	convblit_copy_mask_mono_byte_lsb_bgra_down,
	convblit_copy_mask_mono_word_msb_bgra_down,
	convblit_blend_mask_alpha_byte_bgra_down,
	convblit_srcover_rgba8888_bgra8888_down,
	convblit_copy_rgb888_bgra8888_down
};

PSUBDRIVER fblinear32[4] = {
	&fblinear32_none, &fblinear32_left, &fblinear32_right, &fblinear32_down
};
