/*
 * Copyright (c) 1999, 2000, 2003, 2005, 2006, 2010 Greg Haerr <greg@censoft.com>
 *
 * 4bpp Packed Linear Video Driver for Microwindows (high nibble first)
 * Vr41xx Palm PC
 *
 * If INVERT4BPP is defined, then the values are inverted before drawing.
 * VTech Helio
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"

#include "fb.h"
DEFINE_applyOpR

#if INVERT4BPP
#define INVERT(c)	((c) = (~(c) & 0x0f))
#else
#define INVERT(c)
#endif

/* Fast set pixel at addr, x, to pixelval c*/
#define linear4_drawpixelfast(psd, addr, x, c) \
	*addr = (*addr & notmask[(x)&1]) | ((c) << ((1-((x)&1))<<2));

static const unsigned char notmask[2] = { 0x0f, 0xf0};

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
	ADDR8 addr = ((ADDR8)psd->addr) + (x>>1) + y * psd->linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	INVERT(c);
	if(gr_mode == MWROP_COPY) {
		*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
	} else {
		*addr = (*addr & notmask[x&1]) | 
			((applyOpR(gr_mode, c, *addr >> ((1-(x&1))<<2)) & 0x0f) << ((1-(x&1))<<2));
	}
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear4_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	MWPIXELVAL	c;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	c = (((ADDR8)psd->addr)[(x>>1) + y * psd->linelen] >> ((1-(x&1))<<2) ) & 0x0f;
	INVERT(c);
	return c;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear4_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register ADDR8 addr = ((ADDR8)psd->addr) + (x1>>1) + y * psd->linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	INVERT(c);
	if(gr_mode == MWROP_COPY) {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))<<2));
			if((++x1 & 1) == 0)
				++addr;
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&1]) | 
				((applyOpR(gr_mode, c, *addr >> ((1-(x1&1))<<2)) & 0x0f) << ((1-(x1&1))<<2));
			if((++x1 & 1) == 0)
				++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear4_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	linelen = psd->linelen;
	ADDR8 addr = ((ADDR8)psd->addr) + (x>>1) + y1 * linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	INVERT(c);
	if(gr_mode == MWROP_COPY) {
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
			addr += linelen;
		}
	} else {
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&1]) | 
			    ((applyOpR(gr_mode, c, *addr >> ((1-(x&1))<<2)) & 0x0f) << ((1-(x&1))<<2));
			addr += linelen;
		}
	}
	DRAWOFF;
}

/* srccopy bitblt*/
static void
linear4_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
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

	if (op > MWROP_SIMPLE_MAX)
		op = MWROP_COPY;
	dst = ((ADDR8)dstpsd->addr) + (dstx>>1) + dsty * dlinelen;
	src = ((ADDR8)srcpsd->addr) + (srcx>>1) + srcy * slinelen;

	DRAWON;
	while(--h >= 0) {
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		ADDR8	d = dst;
		ADDR8	s = src;
		unsigned char c = *s;

		INVERT(c);
		for (i=0; i<w; ++i) {
			if (op == MWROP_COPY) {
				*d = (*d & notmask[dx&1]) | ((c >> ((1-(sx&1))<<2) & 0x0f) << ((1-(dx&1))<<2));
			} else {
				*d = (*d & notmask[dx&1]) |
					((applyOpR(op,
						c >> ((1-(sx&1))<<2), *d >> ((1-(dx&1))<<2)) & 0x0f) << ((1-(dx&1))<<2)
					);
			}

			if((++dx & 1) == 0)
				++d;
			if((++sx & 1) == 0) {
				++s;
				c = *s;
				INVERT(c);
			}
		}
		dst += dlinelen;
		src += slinelen;
	}
	DRAWOFF;
}

/*
 * Routine to draw mono 1bpp MSBFirst bitmap to MS nibble 4bpp
 * Bitmap is byte array.
 *
 * Used to draw FT2 non-antialiased glyphs.
 */
static void
linear4_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc)
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
	dst = ((ADDR8) psd->addr) + (psd->linelen * gc->dsty + gc->dstx) / 2;	/* 4bpp = 2 ppb */

	fg = gc->fg_pixelval;
	bg = gc->bg_pixelval;

	advance_src = gc->src_pitch - last_byte + first_byte - 1;
	advance_dst = psd->linelen - (gc->width / 2);				/* 4bpp = 2 ppb */

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
			int x = 0;
			int X = gc->dstx;
			ADDR8 addr = ((ADDR8)psd->addr) + (X>>1) + (gc->dsty+y) * psd->linelen;

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask; MWI_ADVANCE_BIT(mask)) {
					linear4_drawpixelfast(psd,addr,  X, (mask & bitmap_byte)? fg: bg);
					++x; if ((++X & 1) == 0) ++addr;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(0) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(1) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(2) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(3) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(4) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(5) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(6) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
				linear4_drawpixelfast(psd, addr, X, (MWI_BIT_NO(7) & bitmap_byte)? fg: bg);
				++x; if ((++X & 1) == 0) ++addr;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask, postfix_last_bit); MWI_ADVANCE_BIT(mask)) {
					linear4_drawpixelfast(psd, addr, X, (mask & bitmap_byte)? fg: bg);
					++x; if ((++X & 1) == 0) ++addr;
				}
			}
			src += advance_src;
			dst += advance_dst;
		}
	} else {	/* don't use background */
		for (y = 0; y < gc->height; y++) {
			int x = 0;
			int X = gc->dstx;
			ADDR8 addr = ((ADDR8)psd->addr) + (X>>1) + (gc->dsty+y) * psd->linelen;

			/* Do pixels of partial first byte */
			if (prefix_first_bit) {
				bitmap_byte = *src++;
				for (mask = prefix_first_bit; mask; MWI_ADVANCE_BIT(mask)) {
					if (mask & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
					++x; if ((++X & 1) == 0) ++addr;
					++dst;
				}
			}

			/* Do all pixels of main part one byte at a time */
			for (t = size_main; t != 0; t--) {
				bitmap_byte = *src++;

				if (MWI_BIT_NO(0) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;
				if (MWI_BIT_NO(1) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;
				if (MWI_BIT_NO(2) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;
				if (MWI_BIT_NO(3) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;
				if (MWI_BIT_NO(4) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;
				if (MWI_BIT_NO(5) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;
				if (MWI_BIT_NO(6) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;
				if (MWI_BIT_NO(7) & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
				++x; if ((++X & 1) == 0) ++addr;

				dst += 8;
			}

			/* Do last few bits of line */
			if (postfix_last_bit) {
				bitmap_byte = *src++;
				for (mask = postfix_first_bit;
				     MWI_IS_BIT_BEFORE_OR_EQUAL(mask, postfix_last_bit); MWI_ADVANCE_BIT(mask)) {
					if (mask & bitmap_byte)
						linear4_drawpixelfast(psd, addr, X, fg);
					++x; if ((++X & 1) == 0) ++addr;
					++dst;
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

static SUBDRIVER fblinear4_none = {
	linear4_init,
	linear4_drawpixel,
	linear4_readpixel,
	linear4_drawhorzline,
	linear4_drawvertline,
	gen_fillrect,
	linear4_blit,
	NULL,		/* DrawArea*/
	NULL,		/* StretchBlitEx*/
	linear4_convblit_copy_mask_mono_byte_msb,
	NULL,		/* BlitCopyMaskMonoByteLSB*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	NULL,		/* BlitBlendMaskAlphaByte*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL		/* BlitCopyRGB888*/
};

PSUBDRIVER fblinear4[4] = {
	&fblinear4_none, &fbportrait_left, &fbportrait_right, &fbportrait_down
};
