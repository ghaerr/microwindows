/*
 * 16 color 4 planes PC-98 Planar Video Driver for Microwindows
 *
 * Based on BOGL - Ben's Own Graphics Library.
 *   Written by Ben Pfaff <pfaffben@debian.org>.
 *	 BOGL is licensed under the terms of the GNU General Public License
 *
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * In this driver, psd->pitch is line byte length, not line pixel length
 *
 * Modified for PC-98
 * T. Yamada 2022
 */

/*#define NDEBUG*/
#include <assert.h>
#include "device.h"
#include "vgaplan4.h"
#include "fb.h"
#include "genmem.h"

/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENBASE0 	MK_FP(0xa800, 0)
#define SCREENBASE1 	MK_FP(0xb000, 0)
#define SCREENBASE2 	MK_FP(0xb800, 0)
#define SCREENBASE3 	MK_FP(0xe000, 0)
#define BYTESPERLINE		80

static FARADDR screenbase_table[4] = {
	SCREENBASE0, SCREENBASE2, SCREENBASE1, SCREENBASE3
};

/* precalculated mask bits*/
static unsigned char mask[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/* Init VGA controller, calc pitch and mmap size, return 0 on fail*/
int
pc98_init(PSD psd)
{
	psd->pitch = BYTESPERLINE;
	psd->addr = 0;		/* addr, size unused in driver */
	psd->size = 0;

	return 1;
}

/* draw a pixel at x,y of color c*/
void
pc98_drawpixel(PSD psd, MWCOORD x,  MWCOORD y, MWPIXELVAL c)
{
	FARADDR dst;
	int		plane;

	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	if(gr_mode == MWROP_XOR) {
		for(plane=0; plane<4; ++plane) {
			dst = screenbase_table[plane] + x / 8 + y * BYTESPERLINE;
			if  (c & (1 << plane)) {
				PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x&7]));
			}
		}
	} else {
		for(plane=0; plane<4; ++plane) {
			dst = screenbase_table[plane] + x / 8 + y * BYTESPERLINE;
			if  (c & (1 << plane)) {
				ORBYTE_FP (dst,mask[x&7]);
			}
			else {
				ANDBYTE_FP (dst,~mask[x&7]);
			}
		}
	}
	DRAWOFF;
}

/* Return 4-bit pixel value at x,y*/
MWPIXELVAL
pc98_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	FARADDR		src;
	int		    plane;
	MWPIXELVAL	c = 0;

	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	DRAWON;
	for(plane=0; plane<4; ++plane) {
		src = screenbase_table[plane] + x / 8 + y * BYTESPERLINE;
		if(GETBYTE_FP(src) & mask[x&7])
			c |= 1 << plane;
	}
	DRAWOFF;
	return c;
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
pc98_drawhorzline(PSD psd,  MWCOORD x1,  MWCOORD x2,  MWCOORD y, MWPIXELVAL c)
{
	FARADDR dst, last;
	int		plane;
	MWCOORD x1_ini = x1;

	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	/* OR/AND mode is not supported for PC-98 for now */
	if(gr_mode == MWROP_COPY) {
		for(plane=0; plane<4; ++plane) {
			x1 = x1_ini;
			dst = screenbase_table[plane] + x1 / 8 + y * BYTESPERLINE;
			if (x1 / 8 == x2 / 8) {
				while(x1 <= x2) {
					if  (c & (1 << plane)) {
						ORBYTE_FP (dst,mask[x1&7]);
					}
					else {
						ANDBYTE_FP (dst,~mask[x1&7]);
					}
					x1++;
				}
			} else {

				while (x1 % 8) {
					if  (c & (1 << plane)) {
						ORBYTE_FP (dst,mask[x1&7]);
					}
					else {
						ANDBYTE_FP (dst,~mask[x1&7]);
					}
					x1++;
				}
				if (x1_ini % 8)
					dst++;

				last = screenbase_table[plane] + x2 / 8 + y * BYTESPERLINE;
				while (dst < last) {
					if  (c & (1 << plane)) {
						PUTBYTE_FP(dst, 255);
					}
					else {
						PUTBYTE_FP(dst, 0);
					}
					dst++;
				}

				x1 = ((x2 >> 3) << 3);
				while (x1 <= x2) {
					if  (c & (1 << plane)) {
						ORBYTE_FP (dst,mask[x1&7]);
					}
					else {
						ANDBYTE_FP (dst,~mask[x1&7]);
					}
					x1++;
				}
			}
		}
	} else if(gr_mode == MWROP_XOR) {
		for(plane=0; plane<4; ++plane) {
			x1 = x1_ini;
			dst = screenbase_table[plane] + x1 / 8 + y * BYTESPERLINE;
			if (x1 / 8 == x2 / 8) {
				while(x1 <= x2) {
					if  (c & (1 << plane)) {
						PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x1&7]));
					}
					x1++;
				}
			} else {

				while (x1 % 8) {
					if  (c & (1 << plane)) {
						PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x1&7]));
					}
					x1++;
				}
				if (x1_ini % 8)
					dst++;

				last = screenbase_table[plane] + x2 / 8 + y * BYTESPERLINE;
				while (dst < last) {
					if  (c & (1 << plane)) {
						PUTBYTE_FP(dst,~GETBYTE_FP(dst));
						dst++;
					}
				}

				x1 = ((x2 >> 3) << 3);
				while (x1 <= x2) {
					if  (c & (1 << plane)) {
						PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x1&7]));
					}
					x1++;
				}
			}
		}
	} else {
		/* slower method, draw pixel by pixel*/
		while(x1 <= x2) {
			for(plane=0; plane<4; ++plane) {
				if  (c & (1 << plane)) {
					ORBYTE_FP (screenbase_table[plane] + x1 / 8 + y * BYTESPERLINE,mask[x1&7]);
				}
				else {
					ANDBYTE_FP (screenbase_table[plane] + x1 / 8 + y * BYTESPERLINE,~mask[x1&7]);
				}
			}
			x1++;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
pc98_drawvertline(PSD psd, MWCOORD x,  MWCOORD y1,  MWCOORD y2, MWPIXELVAL c)
{
	FARADDR dst, last;
	int		plane;

	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	if(gr_mode == MWROP_XOR) {
		for(plane=0; plane<4; ++plane) {
			dst = screenbase_table[plane] + x / 8 + y1 * BYTESPERLINE;
			last = screenbase_table[plane] + x / 8 + y2 * BYTESPERLINE;
			while (dst <= last) {
				if  (c & (1 << plane)) {
					PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x&7]));
				}
				dst += BYTESPERLINE;
			}
		}
	} else {
		for(plane=0; plane<4; ++plane) {
			dst = screenbase_table[plane] + x / 8 + y1 * BYTESPERLINE;
			last = screenbase_table[plane] + x / 8 + y2 * BYTESPERLINE;
			while (dst <= last) {
				if  (c & (1 << plane)) {
					ORBYTE_FP (dst,mask[x&7]);
				}
				else {
					ANDBYTE_FP (dst,~mask[x&7]);
				}
				dst += BYTESPERLINE;
			}
		}
	}
	DRAWOFF;
}

static SUBDRIVER pc98plan4_none = {
        pc98_drawpixel,
        pc98_readpixel,
        pc98_drawhorzline,
        pc98_drawvertline,
        gen_fillrect,
        vga_blit,
        NULL,       /* FrameBlit*/
        NULL,       /* FrameStretchBlit*/
        0, //linear4_convblit_copy_mask_mono_byte_msb,
        NULL,       /* BlitCopyMaskMonoByteLSB*/
        NULL,       /* BlitCopyMaskMonoWordMSB*/
        NULL,       /* BlitBlendMaskAlphaByte*/
        NULL,       /* BlitCopyRGBA8888*/
        NULL,       /* BlitSrcOverRGBA8888*/
        NULL        /* BlitCopyRGB888*/

};

PSUBDRIVER pc98plan4[4] = {
        &pc98plan4_none,
#if MW_FEATURE_PORTRAIT
        &fbportrait_left, &fbportrait_right, &fbportrait_down
#endif
};
