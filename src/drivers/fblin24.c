/*
 * Copyright (c) 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * 24bpp Linear Video Driver for Microwindows (BGR byte order)
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "convblit.h"
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
#if DEBUG
	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);
	addr += (x + y * psd->linelen) * 3;
	DRAWON;
	if(gr_mode == MWROP_COPY) {
		*addr++ = b;
		*addr++ = g;
		*addr = r;
	} else {
		applyOp(gr_mode, b, addr, ADDR8); ++addr;
		applyOp(gr_mode, g, addr, ADDR8); ++addr;
		applyOp(gr_mode, r, addr, ADDR8);
	}

	if (psd->Update)
		psd->Update(psd, x, y, 1, 1);
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear24_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;
#if DEBUG
	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	addr += (x + y * psd->linelen) * 3;
	return RGB2PIXEL888(addr[2], addr[1], addr[0]);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear24_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	MWUCHAR	r, g, b;
	MWCOORD X1 = x1;
#if DEBUG
	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
#endif
	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);
	addr += (x1 + y * psd->linelen) * 3;
	DRAWON;
	if(gr_mode == MWROP_COPY) {
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

	if (psd->Update)
		psd->Update(psd, X1, y, x2-X1+1, 1);
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear24_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen * 3;
	MWUCHAR	r, g, b;
	MWCOORD Y1 = y1;
#if DEBUG
	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
#endif
	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);
	addr += (x + y1 * psd->linelen) * 3;
	DRAWON;
	if(gr_mode == MWROP_COPY) {
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

	if (psd->Update)
		psd->Update(psd, x, Y1, 1, y2-Y1+1);
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear24_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	ADDR8	dst = dstpsd->addr;
	ADDR8	src = srcpsd->addr;
	int	i;
	int	dlinelen = dstpsd->linelen * 3;
	int	slinelen = srcpsd->linelen * 3;
	int	dlinelen_minus_w = (dstpsd->linelen - w) * 3;
	int	slinelen_minus_w = (srcpsd->linelen - w) * 3;
	int H = h;
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
	DRAWON;
	dst += (dstx + dsty * dstpsd->linelen) * 3;
	src += (srcx + srcy * srcpsd->linelen) * 3;

	if (op == MWROP_BLENDCONSTANT) {
	uint32_t alpha = 150;

	while(--h >= 0) {
		for(i=0; i<w; ++i) {
			if (alpha != 0) {
				register uint32_t pd = *dst;
				*dst++ = muldiv255(alpha, *src++ - pd) + pd;
				pd = *dst;
				*dst++ = muldiv255(alpha, *src++ - pd) + pd;
				pd = *dst;
				*dst++ = muldiv255(alpha, *src++ - pd) + pd;
			} else {
				dst += 3;
				src += 3;
			}
		}
		dst += dlinelen_minus_w;
		src += slinelen_minus_w;
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
			/* a _fast_ memcpy is a _must_ in this routine*/
			memmove(dst, src, w*3);
			dst += dlinelen;
			src += slinelen;
		}
	} else {
		while (--h >= 0) {
			applyOp4(w*3, op, src, dst, ADDR8);
			dst += dlinelen_minus_w;
			src += slinelen_minus_w;
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

/* Blit a 24-bit image.
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
linear24_stretchblitex(PSD dstpsd,
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

	/*DPRINTF("Nano-X: linear24_stretchflipblit( dest=(%d,%d) %dx%d )\n",
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

	/* DPRINTF("linear24_stretchblitex: X: One step=%d, err-=%d; normal step=%d, err+=%d\n"
		"Y: One step=%d, err-=%d; normal step=%d, err+=%d\n",
	   src_x_step_one, x_denominator, src_x_step_normal, x_error_step_normal,
	   src_y_step_one, y_denominator, src_y_step_normal, y_error_step_normal);
	 */

	/* Pointer to the first source pixel */
	next_src_ptr = ((unsigned char *) srcpsd->addr) + 3 * (src_y_start * srcpsd->linelen + src_x_start);

	/* Cache the width of a scanline in dest */
	dest_y_step = dstpsd->linelen;

	/* Pointer to the first dest pixel */
	next_dest_ptr = ((unsigned char *) dstpsd->addr) + 3 * (dest_y_start * dest_y_step + dest_x_start);

	/* Convert from pixels to bytes (only for this 24bpp mode) */
	src_x_step_normal *= 3;
	src_x_step_one *= 3;
	src_y_step_normal *= 3;
	src_y_step_one *= 3;

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
				*dest_ptr++ = src_ptr[0];
				*dest_ptr++ = src_ptr[1];
				*dest_ptr++ = src_ptr[2];

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
			x_count = width * 3;
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
				applyOp(op, src_ptr[0], dest_ptr, ADDR8);
				dest_ptr++;
				applyOp(op, src_ptr[1], dest_ptr, ADDR8);
				dest_ptr++;
				applyOp(op, src_ptr[2], dest_ptr, ADDR8);
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

	if (dstpsd->Update)
		dstpsd->Update(dstpsd, dest_x_start, dest_y_start, width, height);
}

SUBDRIVER fblinear24 = {
	linear24_init,
	linear24_drawpixel,
	linear24_readpixel,
	linear24_drawhorzline,
	linear24_drawvertline,
	gen_fillrect,
	linear24_blit,
	0, 					/* drawarea*/
	linear24_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgr,		/* ft2 non-alias*/
	convblit_copy_mask_mono_byte_lsb_bgr,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_bgr,		/* core/pcf non-alias*/
	convblit_blend_mask_alpha_byte_bgr,			/* ft2/t1 antialias*/
	convblit_srcover_rgba8888_bgr888,			/* RGBA images w/alpha*/
	convblit_copy_rgb888_bgr888					/* RGB images no alpha*/
};

SUBDRIVER fblinear24_left = {
	NULL,
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	fbportrait_left_blit,
	0, 					/* drawarea*/
	fbportrait_left_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgr_left,
	convblit_copy_mask_mono_byte_lsb_bgr_left,
	convblit_copy_mask_mono_word_msb_bgr_left,
	convblit_blend_mask_alpha_byte_bgr_left,
	convblit_srcover_rgba8888_bgr888_left,
	convblit_copy_rgb888_bgr888_left
};

SUBDRIVER fblinear24_right = {
	NULL,
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	fbportrait_right_blit,
	0, 					/* drawarea*/
	fbportrait_right_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgr_right,
	convblit_copy_mask_mono_byte_lsb_bgr_right,
	convblit_copy_mask_mono_word_msb_bgr_right,
	convblit_blend_mask_alpha_byte_bgr_right,
	convblit_srcover_rgba8888_bgr888_right,
	convblit_copy_rgb888_bgr888_right
};

SUBDRIVER fblinear24_down = {
	NULL,
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	fbportrait_down_blit,
	0, 					/* drawarea*/
	fbportrait_down_stretchblitex,
	convblit_copy_mask_mono_byte_msb_bgr_down,
	convblit_copy_mask_mono_byte_lsb_bgr_down,
	convblit_copy_mask_mono_word_msb_bgr_down,
	convblit_blend_mask_alpha_byte_bgr_down,
	convblit_srcover_rgba8888_bgr888_down,
	convblit_copy_rgb888_bgr888_down
};
