#include <stdio.h>
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

/* for convenience in specifying inline parms*/
#define R		0		/* RGBA parms*/
#define G		1
#define B		2
#define A		3

#define NONE	0		/* portrait parm*/
#define LEFT	1
#define RIGHT	2
#define DOWN	3

/*
 * Conversion blit to draw 1bpp mono msb/lsb first byte/word bitmap into 8888 image.
 * Data can be word/byte array, padded to word/byte boundary.
 *
 * This function is defined as a macro first, since other #defines are used
 * to parameterize the src width, and then an inline function is generated,
 * which is then used with additional parameters to build the fast final function.
 */
#define CONVBLIT_COPY_MASK_MONO(funcname)	\
static inline void funcname(PSD psd, PMWBLITPARMS gc,\
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
	/* compiler will optimize out switch statement and most else to constants*/\
	switch (PORTRAIT) {\
	case NONE:\
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
	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + (gc->srcx >> 3);\
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + gc->dstx * DSZ;\
\
	/*\
	 * Create new 8888 image from mono bitmap using current fg (and bg if usebg) color\
	 */\
	height = gc->height;\
	while (--height >= 0)\
	{\
		register unsigned char *d = dst;\
		register SRC_TYPE *s = (SRC_TYPE *)src;\
		SRC_TYPE bitvalue;\
		int x;\
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
					((unsigned short *)d)[0] = RGB2PIXEL565(fg_r, fg_g, fg_b);\
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
					((unsigned short *)d)[0] = RGB2PIXEL565(bg_r, bg_g, bg_b);\
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
printf("convblit_mask_mono\n");\
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

/* 32bpp BGRA*/
void convblit_copy_mask_mono_byte_lsb_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 4, B,G,R,A, NONE);
}

void convblit_copy_mask_mono_byte_lsb_bgra_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 4, B,G,R,A, LEFT);
}

void convblit_copy_mask_mono_byte_lsb_bgra_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 4, B,G,R,A, RIGHT);
}

void convblit_copy_mask_mono_byte_lsb_bgra_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 4, B,G,R,A, DOWN);
}

/* 24bpp BGR*/
void convblit_copy_mask_mono_byte_lsb_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 3, B,G,R,-1, NONE);
}

void convblit_copy_mask_mono_byte_lsb_bgr_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 3, B,G,R,-1, LEFT);
}

void convblit_copy_mask_mono_byte_lsb_bgr_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 3, B,G,R,-1, RIGHT);
}

void convblit_copy_mask_mono_byte_lsb_bgr_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 3, B,G,R,-1, DOWN);
}

/* 16bpp 565/555*/
void convblit_copy_mask_mono_byte_lsb_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 2, 0,0,0,-1, NONE);
}

void convblit_copy_mask_mono_byte_lsb_16bpp_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 2, 0,0,0,-1, LEFT);
}

void convblit_copy_mask_mono_byte_lsb_16bpp_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 2, 0,0,0,-1, RIGHT);
}

void convblit_copy_mask_mono_byte_lsb_16bpp_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_lsb(psd, gc, 2, 0,0,0,-1, DOWN);
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

/* 32bpp BGRA*/
void convblit_copy_mask_mono_word_msb_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 4, B,G,R,A, NONE);
}

void convblit_copy_mask_mono_word_msb_bgra_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 4, B,G,R,A, LEFT);
}

void convblit_copy_mask_mono_word_msb_bgra_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 4, B,G,R,A, RIGHT);
}

void convblit_copy_mask_mono_word_msb_bgra_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 4, B,G,R,A, DOWN);
}

/* 24bpp BGR*/
void convblit_copy_mask_mono_word_msb_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 3, B,G,R,-1, NONE);
}

void convblit_copy_mask_mono_word_msb_bgr_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 3, B,G,R,-1, LEFT);
}

void convblit_copy_mask_mono_word_msb_bgr_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 3, B,G,R,-1, RIGHT);
}

void convblit_copy_mask_mono_word_msb_bgr_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 3, B,G,R,-1, DOWN);
}

/* 16bpp 565/555*/
void convblit_copy_mask_mono_word_msb_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 2, 0,0,0,-1, NONE);
}

void convblit_copy_mask_mono_word_msb_16bpp_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 2, 0,0,0,-1, LEFT);
}

void convblit_copy_mask_mono_word_msb_16bpp_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 2, 0,0,0,-1, RIGHT);
}

void convblit_copy_mask_mono_word_msb_16bpp_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_word_msb(psd, gc, 2, 0,0,0,-1, DOWN);
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

/* 32bpp BGRA*/
void convblit_copy_mask_mono_byte_msb_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 4, B,G,R,A, NONE);
}

void convblit_copy_mask_mono_byte_msb_bgra_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 4, B,G,R,A, LEFT);
}

void convblit_copy_mask_mono_byte_msb_bgra_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 4, B,G,R,A, RIGHT);
}

void convblit_copy_mask_mono_byte_msb_bgra_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 4, B,G,R,A, DOWN);
}

/* 24bpp BGR*/
void convblit_copy_mask_mono_byte_msb_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 3, B,G,R,-1, NONE);
}

void convblit_copy_mask_mono_byte_msb_bgr_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 3, B,G,R,-1, LEFT);
}

void convblit_copy_mask_mono_byte_msb_bgr_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 3, B,G,R,-1, RIGHT);
}

void convblit_copy_mask_mono_byte_msb_bgr_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 3, B,G,R,-1, DOWN);
}

/* 16bpp 565/555*/
void convblit_copy_mask_mono_byte_msb_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 2, 0,0,0,-1, NONE);
}

void convblit_copy_mask_mono_byte_msb_16bpp_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 2, 0,0,0,-1, LEFT);
}

void convblit_copy_mask_mono_byte_msb_16bpp_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 2, 0,0,0,-1, RIGHT);
}

void convblit_copy_mask_mono_byte_msb_16bpp_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_copy_mask_mono_byte_msb(psd, gc, 2, 0,0,0,-1, DOWN);
}

/*
 * Conversion blit to blend 8bpp alpha with fg/bg onto 24bpp 888 or 32bpp 8888 image
 */
static inline void convblit_blend_mask_alpha_byte(PSD psd, PMWBLITPARMS gc,
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
						((unsigned short *)d)[0] = RGB2PIXEL565(bg_r, bg_g, bg_g);
					else
					{
						if (DA >= 0)
							d[DA] = bg_a;
						d[DR] = bg_r;
						d[DG] = bg_g;
						d[DB] = bg_b;
					}
				} 
			}
			else if (alpha == 255)					/* copy source*/
			{
				if (DSZ == 2)
					((unsigned short *)d)[0] = RGB2PIXEL565(fg_r, fg_g, fg_g);
				else
				{
					if (DA >= 0)
						d[DA] = fg_a;
					d[DR] = fg_r;
					d[DG] = fg_g;
					d[DB] = fg_b;
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
 						/*d = muldiv255(a, 255 - d) + d*/
						if (DA >= 0)
							d[DA] = muldiv255(alpha, 255 - bg_a) + bg_a;

 						/* d = muldiv255(a, s - d) + d*/
						d[DR] = muldiv255(alpha, fg_r - bg_r) + bg_r;
						d[DG] = muldiv255(alpha, fg_g - bg_g) + bg_g;
						d[DB] = muldiv255(alpha, fg_b - bg_b) + bg_b;
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
						((unsigned short *)d)[0] = muldiv255_16bpp(((unsigned short *)d)[0], sr, sg, sb, alpha);
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
printf("convblit_mask_alpha\n");
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
/* 32bpp BGRA*/
void convblit_blend_mask_alpha_byte_bgra(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 4, B,G,R,A, NONE);
}

void convblit_blend_mask_alpha_byte_bgra_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 4, B,G,R,A, LEFT);
}

void convblit_blend_mask_alpha_byte_bgra_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 4, B,G,R,A, RIGHT);
}

void convblit_blend_mask_alpha_byte_bgra_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 4, B,G,R,A, DOWN);
}

/* 24bpp BGR*/
void convblit_blend_mask_alpha_byte_bgr(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 3, B,G,R,-1, NONE);
}

void convblit_blend_mask_alpha_byte_bgr_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 3, B,G,R,-1, LEFT);
}

void convblit_blend_mask_alpha_byte_bgr_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 3, B,G,R,-1, RIGHT);
}

void convblit_blend_mask_alpha_byte_bgr_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 3, B,G,R,-1, DOWN);
}

/* 16bpp 565/555*/
void convblit_blend_mask_alpha_byte_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 2, 0,0,0,-1, NONE);
}

void convblit_blend_mask_alpha_byte_16bpp_left(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 2, 0,0,0,-1, LEFT);
}

void convblit_blend_mask_alpha_byte_16bpp_right(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 2, 0,0,0,-1, RIGHT);
}

void convblit_blend_mask_alpha_byte_16bpp_down(PSD psd, PMWBLITPARMS gc)
{
	convblit_blend_mask_alpha_byte(psd, gc, 2, 0,0,0,-1, DOWN);
}

#if LATER
/*
 * Conversion blit to draw 1bpp mono msb/lsb first byte bitmap into 8888 image.
 * Data is byte array, padded to byte boundary.
 */
#define CONVBLIT_COPY_MASK_MONO_8888_LARGE(funcname,A,R,G,B)	\
void funcname(PSD psd, PMWBLITPARMS gc)\
{\
/*\
 * The difference between the MSB_FIRST and LSB_FIRST variants of\
 * this function is simply the definition of these three #defines.\
 *\
 * BEFORE_OR_EQUAL(A,B) returns true if bit A is before\
 *     (i.e. to the left of) bit B.\
 * NEXT_BIT(X) advances X on to the next bit to the right,\
 *     and stores the result back in X.\
 * BITNUM(N), where 0<=n<=7, gives the Nth bit, where 0 is the\
 *     leftmost bit and 7 is the rightmost bit.  This is a constant\
 *     iff N is a constant.\
 */\
	unsigned char prefix_first_bit;\
	unsigned char postfix_first_bit = FIRST_BIT;\
	unsigned char postfix_last_bit;\
	unsigned char bitmap_byte;\
	unsigned char mask;\
	int first_byte, last_byte, size_main, t, y;\
	unsigned char *src, *dst;\
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
	/* The bit in the first byte, which corresponds to the leftmost pixel. */\
	prefix_first_bit = BITNUM(gc->srcx & 7);\
\
	/* The bit in the last byte, which corresponds to the rightmost pixel. */\
	postfix_last_bit = BITNUM((gc->srcx + gc->width - 1) & 7);\
\
	/* The index into each scanline of the first byte to use. */\
	first_byte = gc->srcx >> 3;\
\
	/* The index into each scanline of the last byte to use. */\
	last_byte = (gc->srcx + gc->width - 1) >> 3;\
\
	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + first_byte;\
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + (gc->dstx << 2);\
\
	if (first_byte != last_byte) {\
		/* total bytes to use, less two special cased bytes (first and last)*/\
		size_main = last_byte - first_byte + 1 - 2;\
\
		if (prefix_first_bit == FIRST_BIT) {	/* No need to special case. */\
			prefix_first_bit = 0;\
			size_main++;\
		}\
		if (postfix_last_bit == LAST_BIT) {		/* No need to special case. */\
			postfix_last_bit = 0;\
			size_main++;\
		}\
	} else if (prefix_first_bit == FIRST_BIT && postfix_last_bit == LAST_BIT) {\
		prefix_first_bit = 0;\
		postfix_last_bit = 0;\
		size_main = 1;							/* Exactly one byte wide. */\
	} else {\
		postfix_first_bit = prefix_first_bit;	/* Do everything in 'postfix' loop. */\
		prefix_first_bit = 0;\
		size_main = 0;			/* Very narrow pixmap, fits in single first byte. */\
	}\
\
	for (y = 0; y < gc->height; y++) {\
		register unsigned char *d = dst;\
		register unsigned char *s = src;\
\
		/* Do pixels of partial first byte */\
		if (prefix_first_bit) {\
			bitmap_byte = *s++;\
			for (mask = prefix_first_bit; mask; NEXT_BIT(mask)) {\
				if (mask & bitmap_byte)\
				{\
					d[A] = fg_a;\
					d[R] = fg_r;\
					d[G] = fg_g;\
					d[B] = fg_b;\
				}\
				else if (usebg)\
				{\
					d[A] = bg_a;\
					d[R] = bg_r;\
					d[G] = bg_g;\
					d[B] = bg_b;\
				}\
				d += 4;\
			}\
		}\
\
		/* Do all pixels of main part one byte at a time */\
		for (t = size_main; t != 0; t--) {\
			bitmap_byte = *s++;\
\
			if (BITNUM(0) & bitmap_byte) {\
				d[A] = fg_a;\
				d[R] = fg_r;\
				d[G] = fg_g;\
				d[B] = fg_b;\
			} else if (usebg) {\
				d[A] = bg_a;\
				d[R] = bg_r;\
				d[G] = bg_g;\
				d[B] = bg_b;\
			}\
			if (BITNUM(1) & bitmap_byte) {\
				d[A+4] = fg_a;\
				d[R+4] = fg_r;\
				d[G+4] = fg_g;\
				d[B+4] = fg_b;\
			} else if (usebg) {\
				d[A+4] = bg_a;\
				d[R+4] = bg_r;\
				d[G+4] = bg_g;\
				d[B+4] = bg_b;\
			}\
			if (BITNUM(2) & bitmap_byte) {\
				d[A+8] = fg_a;\
				d[R+8] = fg_r;\
				d[G+8] = fg_g;\
				d[B+8] = fg_b;\
			} else if (usebg) {\
				d[A+8] = bg_a;\
				d[R+8] = bg_r;\
				d[G+8] = bg_g;\
				d[B+8] = bg_b;\
			}\
			if (BITNUM(3) & bitmap_byte) {\
				d[A+12] = fg_a;\
				d[R+12] = fg_r;\
				d[G+12] = fg_g;\
				d[B+12] = fg_b;\
			} else if (usebg) {\
				d[A+12] = bg_a;\
				d[R+12] = bg_r;\
				d[G+12] = bg_g;\
				d[B+12] = bg_b;\
			}\
			if (BITNUM(4) & bitmap_byte) {\
				d[A+16] = fg_a;\
				d[R+16] = fg_r;\
				d[G+16] = fg_g;\
				d[B+16] = fg_b;\
			} else if (usebg) {\
				d[A+16] = bg_a;\
				d[R+16] = bg_r;\
				d[G+16] = bg_g;\
				d[B+16] = bg_b;\
			}\
			if (BITNUM(5) & bitmap_byte) {\
				d[A+20] = fg_a;\
				d[R+20] = fg_r;\
				d[G+20] = fg_g;\
				d[B+20] = fg_b;\
			} else if (usebg) {\
				d[A+20] = bg_a;\
				d[R+20] = bg_r;\
				d[G+20] = bg_g;\
				d[B+20] = bg_b;\
			}\
			if (BITNUM(6) & bitmap_byte) {\
				d[A+24] = fg_a;\
				d[R+24] = fg_r;\
				d[G+24] = fg_g;\
				d[B+24] = fg_b;\
			} else if (usebg) {\
				d[A+24] = bg_a;\
				d[R+24] = bg_r;\
				d[G+24] = bg_g;\
				d[B+24] = bg_b;\
			}\
			if (BITNUM(7) & bitmap_byte) {\
				d[A+28] = fg_a;\
				d[R+28] = fg_r;\
				d[G+28] = fg_g;\
				d[B+28] = fg_b;\
			} else if (usebg) {\
				d[A+28] = bg_a;\
				d[R+28] = bg_r;\
				d[G+28] = bg_g;\
				d[B+28] = bg_b;\
			}\
			d += 32;\
		}\
\
		/* Do last few bits of line */\
		if (postfix_last_bit) {\
			bitmap_byte = *s++;\
			for (mask = postfix_first_bit; BEFORE_OR_EQUAL(mask, postfix_last_bit); NEXT_BIT(mask)) {\
				if (mask & bitmap_byte)\
				{\
					d[A] = fg_a;\
					d[R] = fg_r;\
					d[G] = fg_g;\
					d[B] = fg_b;\
				}\
				else if (usebg)\
				{\
					d[A] = bg_a;\
					d[R] = bg_r;\
					d[G] = bg_g;\
					d[B] = bg_b;\
				}\
				d += 4;\
			}\
		}\
\
		src += gc->src_pitch;\
		dst += gc->dst_pitch;\
	}\
\
	/* update screen bits if driver requires it*/\
	if (psd->Update)\
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);\
}


/*
 * Fast, larger routine to draw mono 1bpp MSBFirst bitmap into ARGB image.
 * Bitmap is byte array.
 * This routine is best used for large bitmaps, as setup times are longer
 * than the simpler CONVBLIT_COPY_MASK_MONO routine.
 *
 * Used to draw FT2 non-antialiased glyphs.
 */
#define BEFORE_OR_EQUAL(a,b) ((a) >= (b))			/* msb*/
#define NEXT_BIT(target) ((target) >>= 1)			/* msb*/
#define BITNUM(n) (0x80 >> (n))						/* byte msb*/
#define FIRST_BIT BITNUM(0)
#define LAST_BIT  BITNUM(7)
CONVBLIT_COPY_MASK_MONO_8888_LARGE(convblit_copy_mask_mono_byte_msb_bgra_large,3,2,1,0)
#undef BEFORE_OR_EQUAL
#undef NEXT_BIT
#undef BITNUM
#undef FIRST_BIT
#undef LAST_BIT

/*
 * Fast, larger routine to draw mono 1bpp LSBFirst bitmap into ARGB image.
 * Bitmap is byte array.
 * This routine is best used for large bitmaps, as setup times are longer
 * than the simpler CONVBLIT_COPY_MASK_MONO_8888 routine.
 *
 * Used to draw T1LIB non-antialiased glyphs.
 */
#define BEFORE_OR_EQUAL(a,b) ((a) <= (b))			/* lsb*/
#define NEXT_BIT(target) ((target) <<= 1)			/* lsb*/
#define BITNUM(n) (0x01 << (n))						/* byte lsb*/
#define FIRST_BIT BITNUM(0)
#define LAST_BIT  BITNUM(7)
CONVBLIT_COPY_MASK_MONO_8888_LARGE(convblit_copy_mask_mono_byte_lsb_bgra_large,3,2,1,0)
#undef BEFORE_OR_EQUAL
#undef NEXT_BIT
#undef BITNUM
#undef FIRST_BIT
#undef LAST_BIT

#endif /* LATER*/
