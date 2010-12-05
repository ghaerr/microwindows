/*
 * Copyright (c) 1999, 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
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
	register ADDR32	addr = ((ADDR32)psd->addr) + x + y * psd->linelen;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	DRAWON;
	if (gr_mode == MWROP_COPY)
		*addr = c;
	else
		APPLYOP(gr_mode, 1, (MWPIXELVAL), c, *(ADDR32), addr, 0, 0);
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
	int width = x2-x1+1;
#if DEBUG
	assert (psd->addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
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
linear32_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	linelen = psd->linelen;
	register ADDR32	addr = ((ADDR32)psd->addr) + x + y1 * linelen;
	int height = y2-y1+1;
#if DEBUG
	assert (psd->addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
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
		APPLYOP(gr_mode, height, (MWPIXELVAL), c, *(ADDR32), addr, 0, linelen);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y1, 1, height);
}

static SUBDRIVER fblinear32_none = {
	linear32_init,
	linear32_drawpixel,
	linear32_readpixel,
	linear32_drawhorzline,
	linear32_drawvertline,
	gen_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
	convblit_copy_mask_mono_byte_msb_rgba,
	convblit_copy_mask_mono_byte_lsb_rgba,
	convblit_copy_mask_mono_word_msb_rgba,
	convblit_blend_mask_alpha_byte_rgba,
	convblit_copy_rgba8888_rgba8888,
	convblit_srcover_rgba8888_rgba8888,
	convblit_copy_rgb888_rgba8888
#else
	convblit_copy_mask_mono_byte_msb_bgra,		/* ft2 non-alias*/
	convblit_copy_mask_mono_byte_lsb_bgra,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_bgra,		/* core/pcf non-alias*/
	convblit_blend_mask_alpha_byte_bgra,		/* ft2/t1 antialias*/
	convblit_copy_rgba8888_bgra8888,			/* RGBA image copy (GdArea MWPF_RGB)*/
	convblit_srcover_rgba8888_bgra8888,			/* RGBA images w/alpha*/
	convblit_copy_rgb888_bgra8888				/* RGB images no alpha*/
#endif
};

static SUBDRIVER fblinear32_left = {
	NULL,
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
	convblit_copy_mask_mono_byte_msb_rgba,
	convblit_copy_mask_mono_byte_lsb_rgba,
	convblit_copy_mask_mono_word_msb_rgba,
	convblit_blend_mask_alpha_byte_rgba,
	convblit_copy_rgba8888_rgba8888,
	convblit_srcover_rgba8888_rgba8888,
	convblit_copy_rgb888_rgba8888
#else
	convblit_copy_mask_mono_byte_msb_bgra,
	convblit_copy_mask_mono_byte_lsb_bgra,
	convblit_copy_mask_mono_word_msb_bgra,
	convblit_blend_mask_alpha_byte_bgra,
	convblit_copy_rgba8888_bgra8888,
	convblit_srcover_rgba8888_bgra8888,
	convblit_copy_rgb888_bgra8888
#endif
};

static SUBDRIVER fblinear32_right = {
	NULL,
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
	convblit_copy_mask_mono_byte_msb_rgba,
	convblit_copy_mask_mono_byte_lsb_rgba,
	convblit_copy_mask_mono_word_msb_rgba,
	convblit_blend_mask_alpha_byte_rgba,
	convblit_copy_rgba8888_rgba8888,
	convblit_srcover_rgba8888_rgba8888,
	convblit_copy_rgb888_rgba8888
#else
	convblit_copy_mask_mono_byte_msb_bgra,
	convblit_copy_mask_mono_byte_lsb_bgra,
	convblit_copy_mask_mono_word_msb_bgra,
	convblit_blend_mask_alpha_byte_bgra,
	convblit_copy_rgba8888_bgra8888,
	convblit_srcover_rgba8888_bgra8888,
	convblit_copy_rgb888_bgra8888
#endif
};

static SUBDRIVER fblinear32_down = {
	NULL,
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
	convblit_copy_mask_mono_byte_msb_rgba,
	convblit_copy_mask_mono_byte_lsb_rgba,
	convblit_copy_mask_mono_word_msb_rgba,
	convblit_blend_mask_alpha_byte_rgba,
	convblit_copy_rgba8888_rgba8888,
	convblit_srcover_rgba8888_rgba8888,
	convblit_copy_rgb888_rgba8888
#else
	convblit_copy_mask_mono_byte_msb_bgra,
	convblit_copy_mask_mono_byte_lsb_bgra,
	convblit_copy_mask_mono_word_msb_bgra,
	convblit_blend_mask_alpha_byte_bgra,
	convblit_copy_rgba8888_bgra8888,
	convblit_srcover_rgba8888_bgra8888,
	convblit_copy_rgb888_bgra8888
#endif
};

PSUBDRIVER fblinear32[4] = {
	&fblinear32_none, &fblinear32_left, &fblinear32_right, &fblinear32_down
};
