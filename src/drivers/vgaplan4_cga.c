/*
 * 16 color 4 planes CGA Planar Video Driver for Microwindows
 *
 * Based on BOGL - Ben's Own Graphics Library.
 *   Written by Ben Pfaff <pfaffben@debian.org>.
 *	 BOGL is licensed under the terms of the GNU General Public License
 *
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * In this driver, psd->pitch is line byte length, not line pixel length
 *
 * Modified for CGA
 * T. Yamada 2024
 */

/*#define NDEBUG*/
#include <assert.h>
#include "device.h"
#include "vgaplan4.h"
#include "fb.h"
#include "genmem.h"

/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENBASE0 	MK_FP(0xb800, 0)
#define SCREENBASE1 	MK_FP(0xba00, 0)
#define BYTESPERLINE	80

static FARADDR screenbase_table[2] = {
	SCREENBASE0, SCREENBASE1
};

/* precalculated mask bits*/
static unsigned char mask[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/* Init CGA controller, calc pitch */
int
cga_init(PSD psd)
{
	psd->pitch = BYTESPERLINE;
	psd->addr = 0;		/* addr, size unused in driver */
	psd->size = 0;
	return 1;
}

/* draw a pixel at x,y of color c*/
void
cga_drawpixel(PSD psd, MWCOORD x,  MWCOORD y, MWPIXELVAL c)
{
	FARADDR dst;
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	/*if (y < psd->yres) {*/
		dst = screenbase_table[y&1] + x / 8 + y / 2 * BYTESPERLINE;
		if(gr_mode == MWROP_XOR) {
			if (c) PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x&7]));
		} else {
			if (c) ORBYTE_FP (dst,mask[x&7]);
			else  ANDBYTE_FP (dst,~mask[x&7]);
		}
	/*}*/
	DRAWOFF;
}

/* Return pixel value at x,y*/
MWPIXELVAL
cga_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	FARADDR		src;
	MWPIXELVAL	c = 0;

	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	DRAWON;
	src = screenbase_table[y&1] + x / 8 + y / 2 * BYTESPERLINE;
	if(GETBYTE_FP(src) & mask[x&7])
		c = 1;
	DRAWOFF;
	return c;
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
cga_drawhorzline(PSD psd,  MWCOORD x1,  MWCOORD x2,  MWCOORD y,
	MWPIXELVAL c)
{
	FARADDR dst, last;
	MWCOORD x1_ini = x1;

	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	/*if (y < psd->yres) {*/
		/* OR/AND mode is not supported for CGA for now */
		if(gr_mode == MWROP_COPY) {
			x1 = x1_ini;
			dst = screenbase_table[y&1] + x1 / 8 + y / 2 * BYTESPERLINE;
			if (x1 / 8 == x2 / 8) {
				while (x1 <= x2) {
					if (c) ORBYTE_FP (dst,mask[x1&7]);
					else  ANDBYTE_FP (dst,~mask[x1&7]);
					x1++;
				}
			} else {

				while (x1 % 8) {
					if (c) ORBYTE_FP (dst,mask[x1&7]);
					else  ANDBYTE_FP (dst,~mask[x1&7]);
					x1++;
				}
				if (x1_ini % 8)
					dst++;

				last = screenbase_table[y&1] + x2 / 8 + y / 2 * BYTESPERLINE;
				while (dst < last) {
					if (c) PUTBYTE_FP (dst++, 255);
					else   PUTBYTE_FP (dst++, 0);
				}

				x1 = ((x2 >> 3) << 3);
				while (x1 <= x2) {
					if (c) ORBYTE_FP (dst,mask[x1&7]);
					else  ANDBYTE_FP (dst,~mask[x1&7]);
					x1++;
				}
			}
		} else if(gr_mode == MWROP_XOR) {
			x1 = x1_ini;
			dst = screenbase_table[y&1] + x1 / 8 + y / 2 * BYTESPERLINE;
			if (x1 / 8 == x2 / 8) {
				while(x1 <= x2) {
					if (c) PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x1&7]));
					x1++;
				}
			} else {
				while (x1 % 8) {
					if (c) PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x1&7]));
					x1++;
				}
				if (x1_ini % 8)
				    dst++;

				last = screenbase_table[y&1] + x2 / 8 + y / 2 * BYTESPERLINE;
				while (dst < last) {
					if (c) PUTBYTE_FP(dst,~GETBYTE_FP(dst));
					dst++;
				}

				x1 = ((x2 >> 3) << 3);
				while (x1 <= x2) {
					if (c) PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x1&7]));
					x1++;
				}
			}
		} else {
			/* slower method, draw pixel by pixel*/
			while(x1 <= x2) {
				if (c) ORBYTE_FP (screenbase_table[y&1] + x1 / 8 + y / 2 * BYTESPERLINE,mask[x1&7]);
				else  ANDBYTE_FP (screenbase_table[y&1] + x1 / 8 + y / 2 * BYTESPERLINE,~mask[x1&7]);
				x1++;
			}
		}
	/*}*/
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
cga_drawvertline(PSD psd, MWCOORD x,  MWCOORD y1,  MWCOORD y2, MWPIXELVAL c)
{
	FARADDR dst;
	MWCOORD y = y1;

	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	if(gr_mode == MWROP_XOR) {
		while (y <= y2 /*&& y < psd->yres*/) {
			dst = screenbase_table[y&1] + x / 8 + y / 2 * BYTESPERLINE;
			if (c) PUTBYTE_FP(dst,(GETBYTE_FP(dst) ^ mask[x&7]));
			y++;
		}
	} else {
		while (y <= y2 /*&& y < psd->yres*/) {
			dst = screenbase_table[y&1] + x / 8 + y / 2 * BYTESPERLINE;
			if (c) ORBYTE_FP (dst,mask[x&7]);
			else  ANDBYTE_FP (dst,~mask[x&7]);
			y++;
		}
	}
	DRAWOFF;
}

static SUBDRIVER cgaplan4_none = {
    cga_drawpixel,
    cga_readpixel,
    cga_drawhorzline,
    cga_drawvertline,
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

PSUBDRIVER cgaplan4[4] = {
    &cgaplan4_none,
#if MW_FEATURE_PORTRAIT
    &fbportrait_left, &fbportrait_right, &fbportrait_down
#endif
};
