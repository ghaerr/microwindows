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
#include <stdlib.h>

/* We want to do string copying fast, so inline assembly if possible */
#ifndef __OPTIMIZE__
#define __OPTIMIZE__
#endif
#include <stdlib.h>
#include "device.h"
#include "convblit.h"
#include "fb.h"
#include "genmem.h"

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
static int init_alpha_lookup(void);

/* Set pixel at x, y, to pixelval c*/
static void
linear8_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + x;
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
		*addr = c;
	else
		APPLYOP(gr_mode, 1, (unsigned char), c, *(ADDR8), addr, 0, 0);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y, 1, 1);
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear8_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + x;
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return *addr;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear8_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + x1;
	int width = x2-x1+1;
#if DEBUG
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	if(gr_mode == MWROP_COPY)
	{
		//memset(addr, c, width);
		int w = width;
		while(--w >= 0)
			*addr++ = c;
	}
	else
		APPLYOP(gr_mode, width, (unsigned char), c, *(ADDR8), addr, 0, 1);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x1, y, width, 1);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear8_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	pitch = psd->pitch;
	register unsigned char *addr = psd->addr + y1 * pitch + x;
	int height = y2-y1+1;
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
	{
		int h = height;
		while (--h >= 0)
		{
			*addr = c;
			addr += pitch;
		}
	}
	else
		APPLYOP(gr_mode, height, (unsigned char), c, *(ADDR8), addr, 0, pitch);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y1, 1, height);
}

#if MW_FEATURE_PALETTE
/* FIXME create lookup table whenever palette changed*/
static int
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
#endif /* MW_FEATURE_PALETTE*/

/*
 * Routine to draw mono 1bpp MSBFirst bitmap to 8bpp
 * Bitmap is byte array.
 *
 * Used to draw T1LIB non-antialiased glyphs.
 */
static void
linear8_convblit_copy_mask_mono_byte_lsb(PSD psd, PMWBLITPARMS gc)
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

	src = ((ADDR8) gc->data) + gc->src_pitch * gc->srcy + first_byte;
	dst = ((ADDR8) gc->data_out) + gc->dst_pitch * gc->dsty + gc->dstx;
	fg = gc->fg_pixelval;
	bg = gc->bg_pixelval;

	advance_src = gc->src_pitch - last_byte + first_byte - 1;
	advance_dst = gc->dst_pitch - gc->width;

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

	if (psd->Update)
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
	DRAWOFF;

#undef MWI_IS_BIT_BEFORE_OR_EQUAL
#undef MWI_ADVANCE_BIT
#undef MWI_BIT_NO
#undef MWI_FIRST_BIT
#undef MWI_LAST_BIT
}

/*
 * Routine to draw mono 1bpp MSBFirst bitmap to 8bpp
 * Bitmap is byte array.
 *
 * Used to draw FT2 non-antialiased glyphs.
 */
static void
linear8_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc)
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

	src = ((ADDR8) gc->data) + gc->src_pitch * gc->srcy + first_byte;
	dst = ((ADDR8) gc->data_out) + gc->dst_pitch * gc->dsty + gc->dstx;
	fg = gc->fg_pixelval;
	bg = gc->bg_pixelval;

	advance_src = gc->src_pitch - last_byte + first_byte - 1;
	advance_dst = gc->dst_pitch - gc->width;

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

	if (psd->Update)
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
	DRAWOFF;

#undef MWI_IS_BIT_BEFORE_OR_EQUAL
#undef MWI_ADVANCE_BIT
#undef MWI_BIT_NO
#undef MWI_FIRST_BIT
#undef MWI_LAST_BIT
}

/*
 * Routine to blend 8bpp alpha byte array with fg/bg to 8bpp
 *
 * Used to draw FT2 and T1LIB antialiased glyphs.
 */
static void
linear8_convblit_blend_mask_alpha_byte(PSD psd, PMWBLITPARMS gc)
{
#if MW_FEATURE_PALETTE
	ADDR8 dst, alpha;
	int x, y;
	unsigned int as;
	int src_row_step, dst_row_step;

	/* init alpha lookup tables*/
	if(!rgb_to_palindex) {
		if (!init_alpha_lookup())
			return;
	}

	alpha = ((ADDR8) gc->data) + gc->src_pitch * gc->srcy + gc->srcx;
	dst = ((ADDR8) gc->data_out) + gc->dst_pitch * gc->dsty + gc->dstx;

	src_row_step = gc->src_pitch - gc->width;
	dst_row_step = gc->dst_pitch - gc->width;

	DRAWON;
	for (y = 0; y < gc->height; y++) {
		for (x = 0; x < gc->width; x++) {
			if ((as = *alpha++) == 255)
				*dst++ = gc->fg_pixelval;
			else if (as != 0) {
				/* Create 5 bit alpha value index for 256 color indexing*/

				/* Get source RGB555 value for source alpha value*/
				unsigned short s = alpha_to_rgb[((as >> 3) << 8) + gc->fg_pixelval];

				/* Get destination RGB555 value for dest alpha value*/
				unsigned short d = alpha_to_rgb[(((as >> 3) ^ 31) << 8) + *dst];

				/* Add RGB values together and get closest palette index to it*/
				*dst++ = rgb_to_palindex[s + d];
			} else if(gc->usebg)		/* alpha 0 - draw bkgnd*/
				*dst++ = gc->bg_pixelval;
			else
				++dst;
		}
		alpha += src_row_step;
		dst += dst_row_step;
	}

	if (psd->Update)
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
	DRAWOFF;
#endif /* MW_FEATURE_PALETTE*/
}

static SUBDRIVER fblinear8_none = {
	linear8_drawpixel,
	linear8_readpixel,
	linear8_drawhorzline,
	linear8_drawvertline,
	gen_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_8bpp,
	frameblit_stretch_8bpp,
	linear8_convblit_copy_mask_mono_byte_msb,	/* FT2 non-alias*/
	linear8_convblit_copy_mask_mono_byte_lsb,	/* T1LIB non-alias*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/	/* core, PCF, FNT will use GdBitmap fallback*/
	linear8_convblit_blend_mask_alpha_byte,		/* FT2/T1 anti-alias*/
	NULL,		/* BlitCopyRGBA8888*/			/* images will use GdDrawAreaByPoint fallback*/
	NULL,		/* BlitSrcOverRGBA8888*/		/* images will use GdDrawImageByPoint fallback*/
	NULL,		/* BlitCopyRGB888*/				/* images will use GdDrawImageByPoint fallback*/
	NULL		/* BlitStretchRGBA8888*/
};

#if MW_FEATURE_PORTRAIT
SUBDRIVER fblinear8_left = {
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_8bpp,
	frameblit_stretch_8bpp,
	fbportrait_left_convblit_copy_mask_mono_byte_msb,	/* FT2 non-alias*/
	fbportrait_left_convblit_copy_mask_mono_byte_lsb,	/* T1LIB non-alias*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	fbportrait_left_convblit_blend_mask_alpha_byte,		/* FT2/T1 anti-alias*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL,		/* BlitCopyRGB888*/
	NULL		/* BlitStretchRGBA8888*/
};

SUBDRIVER fblinear8_right = {
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_8bpp,
	frameblit_stretch_8bpp,
	fbportrait_right_convblit_copy_mask_mono_byte_msb,	/* FT2 non-alias*/
	fbportrait_right_convblit_copy_mask_mono_byte_lsb,	/* T1LIB non-alias*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	fbportrait_right_convblit_blend_mask_alpha_byte,	/* FT2/T1 anti-alias*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL,		/* BlitCopyRGB888*/
	NULL		/* BlitStretchRGBA8888*/
};

SUBDRIVER fblinear8_down = {
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_8bpp,
	frameblit_stretch_8bpp,
	fbportrait_down_convblit_copy_mask_mono_byte_msb,	/* FT2 non-alias*/
	fbportrait_down_convblit_copy_mask_mono_byte_lsb,	/* T1LIB non-alias*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	fbportrait_down_convblit_blend_mask_alpha_byte,		/* FT2/T1 anti-alias*/
	NULL,		/* BlitCopyRGBA8888*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL,		/* BlitCopyRGB888*/
	NULL		/* BlitStretchRGBA8888*/
};
#endif

PSUBDRIVER fblinear8[4] = {
	&fblinear8_none
#if MW_FEATURE_PORTRAIT
	, &fblinear8_left, &fblinear8_right, &fblinear8_down
#endif
};
