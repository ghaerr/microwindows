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

/*
 * Routine to draw mono 1bpp MSBFirst bitmap into 8888 image.
 * Bitmap is word array, padded to word boundary.
 *
 * Used to draw all core builtin MWCFONT bitmap glyphs, PCF and FNT.
 */
#define SRC_TYPE		unsigned short					/* word*/
//#define BITNUM(n) 		(0x8000 >> (n))					/* msb*/
#define SRC_TYPE_MASK	(8*sizeof(SRC_TYPE) - 1)		/* x address boundary mask*/

/*
 * Conversion blit to draw 1bpp mono msb/lsb first byte/word bitmap into 8888 image.
 * Data can be word/byte array, padded to word/byte boundary.
 *
 * This function is defined as a macro first, since other #defines are used
 * to parameterize the src width, and then an inline function is generated,
 * which is then used with additional parameters to build the fast final function.
 */
void convblit_copy_mask_mono_word_msb(PSD psd, PMWBLITPARMS gc,
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)
{
	unsigned char *src, *dst;
	int minx, maxx;
	int dsz, dst_pitch;
	int height, newx, newy;
	int src_pitch = gc->src_pitch;
	int usebg = gc->usebg;
	uint32_t fg = gc->fg_colorval;				/* color values in MWCOLORVAL format*/
	unsigned char fg_r = fg & 0xff;			// REDVALUE
	unsigned char fg_g = (fg >> 8) & 0xff;	// GREENVALUE
	unsigned char fg_b = (fg >> 16) & 0xff;	// BLUEVALUE
	unsigned char fg_a = (fg >> 24) & 0xff;	// ALPHAVALUE
	uint32_t bg = gc->bg_colorval;
	unsigned char bg_r = bg & 0xff;			// REDVALUE
	unsigned char bg_g = (bg >> 8) & 0xff;	// GREENVALUE
	unsigned char bg_b = (bg >> 16) & 0xff;	// BLUEVALUE
	unsigned char bg_a = (bg >> 24) & 0xff;	// ALPHAVALUE

	dsz = DSZ;					/* dst: next pixel over*/
	dst_pitch = gc->dst_pitch;	/* dst: next line down*/

	minx = gc->srcx;
	maxx = minx + gc->width;

	if (sizeof(SRC_TYPE) == 2)
		src = ((unsigned char *)gc->data) + gc->srcy *     src_pitch + ((minx >> 4) << 1);
	else
		src = ((unsigned char *)gc->data) + gc->srcy *     src_pitch + (minx >> 3);
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + gc->dstx * DSZ;

	/*
	 * Create new 8888/888/16bpp image from mono bitmap using current fg (and bg if usebg) color
	 */
	DRAWON;
	height = gc->height;
	while (--height >= 0)
	{
		register unsigned char *d = dst;
		register SRC_TYPE *s = (SRC_TYPE *)src;
		SRC_TYPE bitvalue = 0;				/* init to avoid compiler warning*/
		int x;

		if ( (minx & SRC_TYPE_MASK) != 0)	/* init srcx != 0 case*/
			bitvalue = *s++;

		for (x = minx; x < maxx; x++)
		{
			if ( (x & SRC_TYPE_MASK) == 0)
				bitvalue = *s++;

			if (bitvalue & (0x8000 >> (x & SRC_TYPE_MASK)))
			{
				if (DA >= 0)
					d[DA] = fg_a;

				{
					d[DR] = fg_r;
					d[DG] = fg_g;
					d[DB] = fg_b;
				}
			}
			else if (usebg)
			{
				if (DA >= 0)
					d[DA] = bg_a;

				{
					d[DR] = bg_r;
					d[DG] = bg_g;
					d[DB] = bg_b;
				}
			}
			d += dsz;
		}
		src += src_pitch;		/* src: next line down*/
		dst += dst_pitch;
	}
	DRAWOFF;

	/* update screen bits if driver requires it*/
	if (psd->Update)
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
}

/* 32bpp RGBA*/
void convblit_copy_mask_mono_word_msb_rgba(PSD psd, PMWBLITPARMS gc)
{
	//convblit_copy_mask_mono_word_msb(psd, gc, 4, R,G,B,A, psd->portrait);
	  convblit_copy_mask_mono_word_msb(psd, gc, 4, 0,1,2,3, psd->portrait);
}
