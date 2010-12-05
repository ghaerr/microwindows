/*
 * Copyright (c) 1999, 2000, 2001, 2007 Greg Haerr <greg@censoft.com>
 *
 * 16bpp Linear Video Driver for Microwindows (RGB565 or RGB555)
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
#include <string.h>

#include "device.h"
#include "convblit.h"
#include "fb.h"

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear16_init(PSD psd)
{
	if (!psd->size) {
		psd->size = psd->yres * psd->linelen;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32*/
		psd->linelen /= 2;
	}
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear16_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register ADDR16	addr = ((ADDR16)psd->addr) + x + y * psd->linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
		*addr = c;
	else
		APPLYOP(gr_mode, 1, (MWPIXELVAL), c, *(ADDR16), addr, 0, 0);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y, 1, 1);
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear16_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return ((ADDR16)psd->addr)[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear16_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register ADDR16	addr = ((ADDR16)psd->addr) + x1 + y * psd->linelen;
	int width = x2-x1+1;
#if DEBUG
	assert (psd->addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
#endif

	DRAWON;
	if(gr_mode == MWROP_COPY)
	{
		int w = width;
		while(--w >= 0)
			*addr++ = c;
	}
	else
		APPLYOP(gr_mode, width, (MWPIXELVAL), c, *(ADDR32), addr, 0, 1);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x1, y, width, 1);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear16_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	linelen = psd->linelen;
	register ADDR16	addr = ((ADDR16)psd->addr) + x + y1 * linelen;
	int height = y2-y1+1;
#if DEBUG
	assert (addr != 0);
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
			addr += linelen;
		}
	}
	else
		APPLYOP(gr_mode, height, (MWPIXELVAL), c, *(ADDR16), addr, 0, linelen);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y1, 1, height);
}

static SUBDRIVER fblinear16_none = {
	linear16_init,
	linear16_drawpixel,
	linear16_readpixel,
	linear16_drawhorzline,
	linear16_drawvertline,
	gen_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_16bpp,
	frameblit_stretch_16bpp,
	convblit_copy_mask_mono_byte_msb_16bpp,		/* ft2 non-alias*/
	convblit_copy_mask_mono_byte_lsb_16bpp,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_16bpp,		/* core/pcf non-alias*/
	convblit_blend_mask_alpha_byte_16bpp,		/* ft2/t1 antialias*/
	convblit_copy_rgba8888_16bpp,				/* RGBA image copy (GdArea MWPF_RGB)*/
	convblit_srcover_rgba8888_16bpp,			/* RGBA images w/alpha*/
	convblit_copy_rgb888_16bpp					/* RGB images no alpha*/
};

static SUBDRIVER fblinear16_left = {
	NULL,
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_16bpp,
	frameblit_stretch_16bpp,
	convblit_copy_mask_mono_byte_msb_16bpp,
	convblit_copy_mask_mono_byte_lsb_16bpp,
	convblit_copy_mask_mono_word_msb_16bpp,
	convblit_blend_mask_alpha_byte_16bpp,
	convblit_copy_rgba8888_16bpp,
	convblit_srcover_rgba8888_16bpp,
	convblit_copy_rgb888_16bpp
};

static SUBDRIVER fblinear16_right = {
	NULL,
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_16bpp,
	frameblit_stretch_16bpp,
	convblit_copy_mask_mono_byte_msb_16bpp,
	convblit_copy_mask_mono_byte_lsb_16bpp,
	convblit_copy_mask_mono_word_msb_16bpp,
	convblit_blend_mask_alpha_byte_16bpp,
	convblit_copy_rgba8888_16bpp,
	convblit_srcover_rgba8888_16bpp,
	convblit_copy_rgb888_16bpp
};

static SUBDRIVER fblinear16_down = {
	NULL,
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_16bpp,
	frameblit_stretch_16bpp,
	convblit_copy_mask_mono_byte_msb_16bpp,
	convblit_copy_mask_mono_byte_lsb_16bpp,
	convblit_copy_mask_mono_word_msb_16bpp,
	convblit_blend_mask_alpha_byte_16bpp,
	convblit_copy_rgba8888_16bpp,
	convblit_srcover_rgba8888_16bpp,
	convblit_copy_rgb888_16bpp
};

PSUBDRIVER fblinear16[4] = {
	&fblinear16_none, &fblinear16_left, &fblinear16_right, &fblinear16_down
};
