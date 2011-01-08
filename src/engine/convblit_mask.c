#include <stdio.h>
#include <assert.h>
/*
 * Device-independent low level blit routines - 1bpp bitmap and 8bpp alpha mask input,
 *		24/32bpp 8888 or 16bpp 565/555 output
 *
 * Copyright (c) 2010 Greg Haerr <greg@censoft.com>
 *
 * This file will need to be modified when adding a new hardware framebuffer
 * image format.  Currently, 32bpp BGRA and 24bpp BGR are defined.
 *
 * These routines do no range checking, clipping, or cursor
 * overwriting checks, but instead draw directly to the
 * data_out memory buffer specified in the passed BLITPARMS struct.
 */
#include "device.h"
#include "convblit.h"
#include "../drivers/fb.h"		// DRAWON macro

/* for convenience in specifying inline parms*/
#define R		0		/* RGBA parms*/
#define G		1
#define B		2
#define A		3

#define NONE	MWPORTRAIT_NONE
#define LEFT	MWPORTRAIT_LEFT
#define RIGHT	MWPORTRAIT_RIGHT
#define DOWN	MWPORTRAIT_DOWN

/*
 * Conversion blit to draw 1bpp mono msb/lsb first byte/word bitmap into 8888 image.
 * Data can be word/byte array, padded to word/byte boundary.
 *
 * This function is defined as a macro first, since other #defines are used
 * to parameterize the src width, and then an inline function is generated,
 * which is then used with additional parameters to build the fast final function.
 */
#define CONVBLIT_COPY_MASK_MONO(funcname)	\
static inline void ALWAYS_INLINE funcname(PSD psd, PMWBLITPARMS gc,\
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)\
{\
	unsigned char *src, *dst;\
	int minx, maxx;\
	int dsz, dst_pitch;\
	int height, newx, newy;\
	int src_pitch = gc->src_pitch;\
	int usebg = gc->usebg;\
	uint32_t fg = gc->fg_colorval;				/* color values in MWCOLORVAL format*/\
	unsigned char fg_r = REDVALUE(fg);\
	unsigned char fg_g = GREENVALUE(fg);\
	unsigned char fg_b = BLUEVALUE(fg);\
	unsigned char fg_a = ALPHAVALUE(fg);\
	uint32_t bg = gc->bg_colorval;\
	unsigned char bg_r = REDVALUE(bg);\
	unsigned char bg_g = GREENVALUE(bg);\
	unsigned char bg_b = BLUEVALUE(bg);\
	unsigned char bg_a = ALPHAVALUE(bg);\
\
	/* compiler can optimize out switch statement and most else to constants*/\
	switch (PORTRAIT) {\
	case NONE:\
	default: \
		dsz = DSZ;					/* dst: next pixel over*/\
		dst_pitch = gc->dst_pitch;	/* dst: next line down*/\
		break;\
\
	case LEFT:\
		/* change dst top left to lower left for left portrait*/\
		/* rotate left: X -> Y, Y -> maxx - X*/\
		newx = gc->dsty;\
		gc->dsty = psd->xvirtres - gc->dstx - 1;\
		gc->dstx = newx;\
\
		dsz = -gc->dst_pitch;		/* dst: next row up*/\
		dst_pitch = DSZ;			/* dst: next pixel right*/\
		break;\
\
	case RIGHT:\
		/* change dst top left to upper right for right portrait*/\
 		/* Rotate right: X -> maxy - y - h, Y -> X, W -> H, H -> W*/\
		newy = gc->dstx;\
		gc->dstx = psd->yvirtres - gc->dsty - 1;\
		gc->dsty = newy;\
\
		dsz = gc->dst_pitch;		/* dst: next pixel down*/\
		dst_pitch = -DSZ;			/* dst: next pixel left*/\
		break;\
\
	case DOWN:\
		/* change dst top left to lower right for down portrait*/\
 		/* Rotate down: X -> maxx - x - w, Y -> maxy - y - h*/\
		gc->dstx = psd->xvirtres - gc->dstx - 1;\
		gc->dsty = psd->yvirtres - gc->dsty - 1;\
\
		dsz = -DSZ;					/* dst: next pixel left*/\
		dst_pitch = -gc->dst_pitch;	/* dst: next pixel up*/\
		break;\
	}\
\
	minx = gc->srcx;\
	maxx = minx + gc->width;\
\
	if (sizeof(SRC_TYPE) == 2)\
		src = ((unsigned char *)gc->data) + gc->srcy *     src_pitch + ((minx >> 4) << 1);\
	else\
		src = ((unsigned char *)gc->data) + gc->srcy *     src_pitch + (minx >> 3);\
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + gc->dstx * DSZ;\
\
	/*\
	 * Create new 8888/888/16bpp image from mono bitmap using current fg (and bg if usebg) color\
	 */\
	DRAWON;\
	height = gc->height;\
	while (--height >= 0)\
	{\
		register unsigned char *d = dst;\
		register SRC_TYPE *s = (SRC_TYPE *)src;\
		SRC_TYPE bitvalue = 0;				/* init to avoid compiler warning*/\
		int x;\
\
		if ( (minx & SRC_TYPE_MASK) != 0)	/* init srcx != 0 case*/\
			bitvalue = *s++;\
\
		for (x = minx; x < maxx; x++)\
		{\
			if ( (x & SRC_TYPE_MASK) == 0)\
				bitvalue = *s++;\
\
			if (bitvalue & BITNUM(x & SRC_TYPE_MASK))\
			{\
				if (DA >= 0)\
					d[DA] = fg_a;\
\
				if (DSZ == 2)\
					((unsigned short *)d)[0] = RGB2PIXEL(fg_r, fg_g, fg_b);\
				else\
				{\
					d[DR] = fg_r;\
					d[DG] = fg_g;\
					d[DB] = fg_b;\
				}\
			}\
			else if (usebg)\
			{\
				if (DA >= 0)\
					d[DA] = bg_a;\
\
				if (DSZ == 2)\
					((unsigned short *)d)[0] = RGB2PIXEL(bg_r, bg_g, bg_b);\
				else\
				{\
					d[DR] = bg_r;\
					d[DG] = bg_g;\
					d[DB] = bg_b;\
				}\
			}\
			d += dsz;\
		}\
		src += src_pitch;		/* src: next line down*/\
		dst += dst_pitch;\
	}\
	DRAWOFF;\
\
	/* update screen bits if driver requires it*/\
	if (!psd->Update)\
		return;\
\
	switch (PORTRAIT) {		/* switch will be optimized out*/\
	case NONE:\
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);\
		break;\
\
	case LEFT:\
		/* adjust x,y,w,h to physical top left and w/h*/\
		psd->Update(psd, gc->dstx, gc->dsty - gc->width + 1, gc->height, gc->width);\
		break;\
\
	case RIGHT:\
		/* adjust x,y,w,h to physical top left and w/h*/\
		psd->Update(psd, gc->dstx - gc->height + 1, gc->dsty, gc->height, gc->width);\
		break;\
\
	case DOWN:\
		/* adjust x,y,w,h to physical top left and w/h*/\
		psd->Update(psd, gc->dstx - gc->width + 1, gc->dsty - gc->height + 1, gc->width, gc->height);\
		break;\
	}\
}

/*
 * Routine to draw mono 1bpp LSBFirst bitmap into 8888 image.
 * Bitmap is byte array.
 *
 * Used to draw T1LIB non-antialiased glyphs.
 */
#define SRC_TYPE		unsigned char					/* byte*/
#define BITNUM(n) 		(0x01 << (n))					/* lsb*/
#define SRC_TYPE_MASK	(8*sizeof(SRC_TYPE) - 1)		/* x address boundary mask*/
CONVBLIT_COPY_MASK_MONO(convblit_copy_mask_mono_byte_lsb)
#undef SRC_TYPE
#undef SRC_TYPE_MASK
#undef BITNUM

/* 32bpp RGBA*/
void convblit_copy_mask_mono_byte_lsb_rgba(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 4, R,G,B,A, psd->portrait);
}

/* 32bpp BGRA*/
void convblit_copy_mask_mono_byte_lsb_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 4, B,G,R,A, psd->portrait);
}

/* 24bpp BGR*/
void convblit_copy_mask_mono_byte_lsb_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 3, B,G,R,-1, psd->portrait);
}

/* 16bpp 565/555*/
void convblit_copy_mask_mono_byte_lsb_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 2, 0,0,0,-1, psd->portrait);
}

/*
 * Routine to draw mono 1bpp MSBFirst bitmap into 8888 image.
 * Bitmap is word array, padded to word boundary.
 *
 * Used to draw all core builtin MWCFONT bitmap glyphs, PCF and FNT.
 */
#define SRC_TYPE		unsigned short					/* word*/
#define BITNUM(n) 		(0x8000 >> (n))					/* msb*/
#define SRC_TYPE_MASK	(8*sizeof(SRC_TYPE) - 1)		/* x address boundary mask*/
CONVBLIT_COPY_MASK_MONO(convblit_copy_mask_mono_word_msb)
#undef SRC_TYPE
#undef SRC_TYPE_MASK
#undef BITNUM

/* 32bpp RGBA*/
void convblit_copy_mask_mono_word_msb_rgba(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 4, R,G,B,A, psd->portrait);
}

/* 32bpp BGRA*/
void convblit_copy_mask_mono_word_msb_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 4, B,G,R,A, psd->portrait);
}


/* 24bpp BGR*/
void convblit_copy_mask_mono_word_msb_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 3, B,G,R,-1, psd->portrait);
}

/* 16bpp 565/555*/
void convblit_copy_mask_mono_word_msb_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 2, 0,0,0,-1, psd->portrait);
}

/*
 * Routine to draw mono 1bpp MSBFirst bitmap into 8888 image.
 * Bitmap is byte array.
 *
 * Used to draw FT2 non-antialiased glyphs.
 */
#define SRC_TYPE		unsigned char					/* byte*/
#define BITNUM(n) 		(0x80 >> (n))					/* msb*/
#define SRC_TYPE_MASK	(8*sizeof(SRC_TYPE) - 1)		/* x address boundary mask*/
CONVBLIT_COPY_MASK_MONO(convblit_copy_mask_mono_byte_msb)
#undef SRC_TYPE
#undef SRC_TYPE_MASK
#undef BITNUM

/* 32bpp RGBA*/
void convblit_copy_mask_mono_byte_msb_rgba(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 4, R,G,B,A, psd->portrait);
}

/* 32bpp BGRA*/
void convblit_copy_mask_mono_byte_msb_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 4, B,G,R,A, psd->portrait);
}

/* 24bpp BGR*/
void convblit_copy_mask_mono_byte_msb_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 3, B,G,R,-1, psd->portrait);
}

/* 16bpp 565/555*/
void convblit_copy_mask_mono_byte_msb_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 2, 0,0,0,-1, psd->portrait);
}

/*
 * Conversion blit to blend 8bpp alpha with fg/bg onto 24bpp 888 or 32bpp 8888 image
 */
static inline void ALWAYS_INLINE convblit_blend_mask_alpha_byte(PSD psd, PMWBLITPARMS gc,
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)
{
	unsigned char *src, *dst;
	int dsz, dst_pitch;
	int height, newx, newy;
	int src_pitch = gc->src_pitch;
	int usebg = gc->usebg;
	uint32_t fg = gc->fg_colorval;				/* color values in MWCOLORVAL format*/
	unsigned char fg_r = REDVALUE(fg);
	unsigned char fg_g = GREENVALUE(fg);
	unsigned char fg_b = BLUEVALUE(fg);
	unsigned char fg_a = ALPHAVALUE(fg);
	uint32_t bg = gc->bg_colorval;
	unsigned char bg_r = REDVALUE(bg);
	unsigned char bg_g = GREENVALUE(bg);
	unsigned char bg_b = BLUEVALUE(bg);
	unsigned char bg_a = ALPHAVALUE(bg);

	/* compiler will optimize out switch statement and most else to constants*/
	switch (PORTRAIT) {
	case NONE:
	default:
		dsz = DSZ;					/* dst: next pixel over*/
		dst_pitch = gc->dst_pitch;	/* dst: next line down*/
		break;

	case LEFT:
		/* change dst top left to lower left for left portrait*/
		/* rotate left: X -> Y, Y -> maxx - X*/
		newx = gc->dsty;
		gc->dsty = psd->xvirtres - gc->dstx - 1;
		gc->dstx = newx;

		dsz = -gc->dst_pitch;		/* dst: next row up*/
		dst_pitch = DSZ;			/* dst: next pixel right*/
		break;

	case RIGHT:
		/* change dst top left to upper right for right portrait*/
 		/* Rotate right: X -> maxy - y - h, Y -> X, W -> H, H -> W*/
		newy = gc->dstx;
		gc->dstx = psd->yvirtres - gc->dsty - 1;
		gc->dsty = newy;

		dsz = gc->dst_pitch;		/* dst: next pixel down*/
		dst_pitch = -DSZ;			/* dst: next pixel left*/
		break;

	case DOWN:
		/* change dst top left to lower right for down portrait*/
 		/* Rotate down: X -> maxx - x - w, Y -> maxy - y - h*/
		gc->dstx = psd->xvirtres - gc->dstx - 1;
		gc->dsty = psd->yvirtres - gc->dsty - 1;

		dsz = -DSZ;					/* dst: next pixel left*/
		dst_pitch = -gc->dst_pitch;	/* dst: next pixel up*/
		break;
	}

	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + gc->srcx;
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + gc->dstx * DSZ;

	/*
	 * Blend fg/bg with alpha byte array onto destination image
	 */
	DRAWON;
	height = gc->height;
	while (--height >= 0)
	{
		register unsigned char *d = dst;
		register unsigned char *s = src;
		unsigned int alpha;
		int w = gc->width;

		while (--w >= 0)
		{
			/* inline implementation will optimize out all but usebg and alpha compares in inner loop*/
			if ((alpha = *s++) == 0)					/* draw background only if specified*/
			{
				if (usebg)
				{
					if (DSZ == 2)
						((unsigned short *)d)[0] = RGB2PIXEL(bg_r, bg_g, bg_b);
					else
					{
						d[DR] = bg_r;
						d[DG] = bg_g;
						d[DB] = bg_b;
						if (DA >= 0)
							d[DA] = bg_a;
					}
				} 
			}
			else if (alpha == 255)					/* copy source*/
			{
				if (DSZ == 2)
					((unsigned short *)d)[0] = RGB2PIXEL(fg_r, fg_g, fg_b);
				else
				{
					d[DR] = fg_r;
					d[DG] = fg_g;
					d[DB] = fg_b;
					if (DA >= 0)
						d[DA] = fg_a;
				}
			}
			else									/* blend source w/dest or passed bg*/
			{
				if (usebg)
				{
					if (DSZ == 2) {
						unsigned short sr = RED2PIXEL(fg_r);
						unsigned short sg = GREEN2PIXEL(fg_g);
						unsigned short sb = BLUE2PIXEL(fg_b);
						alpha = 255 - alpha + 1; /* flip alpha then add 1 (see muldiv255)*/

						/* d = muldiv255(255-a, d - s) + s*/
						((unsigned short *)d)[0] = muldiv255_16bpp(gc->bg_pixelval, sr, sg, sb, alpha);
					}
					else
					{
 						/* d = muldiv255(a, s - d) + d*/
						d[DR] = muldiv255(alpha, fg_r - bg_r) + bg_r;
						d[DG] = muldiv255(alpha, fg_g - bg_g) + bg_g;
						d[DB] = muldiv255(alpha, fg_b - bg_b) + bg_b;

 						/*d = muldiv255(a, 255 - d) + d*/
						if (DA >= 0)
							d[DA] = muldiv255(alpha, 255 - bg_a) + bg_a;
					}
				}
				else
				{
					if (DSZ == 2) {
						unsigned short sr = RED2PIXEL(fg_r);
						unsigned short sg = GREEN2PIXEL(fg_g);
						unsigned short sb = BLUE2PIXEL(fg_b);
						alpha = 255 - alpha + 1; /* flip alpha then add 1 (see muldiv255)*/

						/* d = muldiv255(255-a, d - s) + s*/
						((unsigned short *)d)[0] =
							muldiv255_16bpp(((unsigned short *)d)[0], sr, sg, sb, alpha);
					}
					else
					{
 						/*d += muldiv255(a, 255 - d)*/
						if (DA >= 0)
							d[DA] += muldiv255(alpha, 255 - d[DA]);

 						/* d += muldiv255(a, s - d)*/
						d[DR] += muldiv255(alpha, fg_r - d[DR]);
						d[DG] += muldiv255(alpha, fg_g - d[DG]);
						d[DB] += muldiv255(alpha, fg_b - d[DB]);
					}
				}
			}
			d += dsz;
		}
		src += src_pitch;				/* src: next line down*/
		dst += dst_pitch;
	}
	DRAWOFF;

	/* update screen bits if driver requires it*/
	if (!psd->Update)
		return;

	switch (PORTRAIT) {		/* switch will be optimized out*/
	case NONE:
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
		break;

	case LEFT:
		/* adjust x,y,w,h to physical top left and w/h*/
		psd->Update(psd, gc->dstx, gc->dsty - gc->width + 1, gc->height, gc->width);
		break;

	case RIGHT:
		/* adjust x,y,w,h to physical top left and w/h*/
		psd->Update(psd, gc->dstx - gc->height + 1, gc->dsty, gc->height, gc->width);
		break;

	case DOWN:
		/* adjust x,y,w,h to physical top left and w/h*/
		psd->Update(psd, gc->dstx - gc->width + 1, gc->dsty - gc->height + 1, gc->width, gc->height);
		break;
	}
}

/*
 * Routine to blend 8bpp alpha byte array with fg/bg to 8888 image.
 *
 * Used to draw FT2 and T1LIB antialiased glyphs.
 */

/* 32bpp RGBA*/
void convblit_blend_mask_alpha_byte_rgba(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 4, R,G,B,A, psd->portrait);
}

/* 32bpp BGRA*/
void convblit_blend_mask_alpha_byte_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 4, B,G,R,A, psd->portrait);
}

/* 24bpp BGR*/
void convblit_blend_mask_alpha_byte_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 3, B,G,R,-1, psd->portrait);
}

/* 16bpp 565/555*/
void convblit_blend_mask_alpha_byte_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 2, 0,0,0,-1, psd->portrait);
}
