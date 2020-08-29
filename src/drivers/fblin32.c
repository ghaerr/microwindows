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
#include <stdlib.h>
#include "device.h"
#include "convblit.h"
#include "fb.h"
#include "genmem.h"

/* Set pixel at x, y, to pixelval c*/
static void
linear32_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x << 2);
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	DRAWON;
	if (gr_mode == MWROP_COPY)
		*((ADDR32)addr) = c;
	else
		APPLYOP(gr_mode, 1, (uint32_t), c, *(ADDR32), addr, 0, 0);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y, 1, 1);
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear32_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x << 2);
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return *((ADDR32)addr);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear32_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x1 << 2);
	int width = x2-x1+1;
#if DEBUG
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
	{
		int w = width;
		while (--w >= 0)
		{
			*((ADDR32)addr) = c;
			addr += 4;
		}
	}
	else
		APPLYOP(gr_mode, width, (uint32_t), c, *(ADDR32), addr, 0, 4);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x1, y, width, 1);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear32_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	pitch = psd->pitch;
	register unsigned char *addr = psd->addr + y1 * pitch + (x << 2);
	int height = y2-y1+1;
#if DEBUG
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
			*((ADDR32)addr) = c;
			addr += pitch;
		}
	}
	else
		APPLYOP(gr_mode, height, (uint32_t), c, *(ADDR32), addr, 0, pitch);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y1, 1, height);
}

/* BGRA subdriver*/
static SUBDRIVER fblinear32bgra_none = {
	linear32_drawpixel,
	linear32_readpixel,
	linear32_drawhorzline,
	linear32_drawvertline,
	gen_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_bgra,		/* ft2 non-alias*/
	convblit_copy_mask_mono_byte_lsb_bgra,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_bgra,		/* core/pcf non-alias*/
	convblit_blend_mask_alpha_byte_bgra,		/* ft2/t1 antialias*/
	convblit_copy_rgba8888_bgra8888,			/* RGBA image copy (GdArea MWPF_RGB)*/
	convblit_srcover_rgba8888_bgra8888,			/* RGBA images w/alpha*/
	convblit_copy_rgb888_bgra8888,				/* RGB images no alpha*/
	frameblit_stretch_rgba8888_bgra8888			/* RGBA stretchblit*/
};

#if MW_FEATURE_PORTRAIT
static SUBDRIVER fblinear32bgra_left = {
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_bgra,
	convblit_copy_mask_mono_byte_lsb_bgra,
	convblit_copy_mask_mono_word_msb_bgra,
	convblit_blend_mask_alpha_byte_bgra,
	convblit_copy_rgba8888_bgra8888,
	convblit_srcover_rgba8888_bgra8888,
	convblit_copy_rgb888_bgra8888,
	frameblit_stretch_rgba8888_bgra8888
};

static SUBDRIVER fblinear32bgra_right = {
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_bgra,
	convblit_copy_mask_mono_byte_lsb_bgra,
	convblit_copy_mask_mono_word_msb_bgra,
	convblit_blend_mask_alpha_byte_bgra,
	convblit_copy_rgba8888_bgra8888,
	convblit_srcover_rgba8888_bgra8888,
	convblit_copy_rgb888_bgra8888,
	frameblit_stretch_rgba8888_bgra8888
};

static SUBDRIVER fblinear32bgra_down = {
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_bgra,
	convblit_copy_mask_mono_byte_lsb_bgra,
	convblit_copy_mask_mono_word_msb_bgra,
	convblit_blend_mask_alpha_byte_bgra,
	convblit_copy_rgba8888_bgra8888,
	convblit_srcover_rgba8888_bgra8888,
	convblit_copy_rgb888_bgra8888,
	frameblit_stretch_rgba8888_bgra8888
};
#endif

PSUBDRIVER fblinear32bgra[4] = {
	&fblinear32bgra_none
#if MW_FEATURE_PORTRAIT
	, &fblinear32bgra_left, &fblinear32bgra_right, &fblinear32bgra_down
#endif
};

/* RGBA subdriver*/
static SUBDRIVER fblinear32rgba_none = {
	linear32_drawpixel,
	linear32_readpixel,
	linear32_drawhorzline,
	linear32_drawvertline,
	gen_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_rgba,		/* ft2 non-alias*/
	convblit_copy_mask_mono_byte_lsb_rgba,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_rgba,		/* core/pcf non-alias*/
	convblit_blend_mask_alpha_byte_rgba,		/* ft2/t1 antialias*/
	convblit_copy_rgba8888_rgba8888,			/* RGBA image copy (GdArea MWPF_RGB)*/
	convblit_srcover_rgba8888_rgba8888,			/* RGBA images w/alpha*/
	convblit_copy_rgb888_rgba8888,				/* RGB images no alpha*/
	frameblit_stretch_xxxa8888					/* RGBA -> RGBA stretchblit*/
};

#if MW_FEATURE_PORTRAIT
static SUBDRIVER fblinear32rgba_left = {
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_rgba,
	convblit_copy_mask_mono_byte_lsb_rgba,
	convblit_copy_mask_mono_word_msb_rgba,
	convblit_blend_mask_alpha_byte_rgba,
	convblit_copy_rgba8888_rgba8888,
	convblit_srcover_rgba8888_rgba8888,
	convblit_copy_rgb888_rgba8888,
	frameblit_stretch_xxxa8888
};

static SUBDRIVER fblinear32rgba_right = {
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_rgba,
	convblit_copy_mask_mono_byte_lsb_rgba,
	convblit_copy_mask_mono_word_msb_rgba,
	convblit_blend_mask_alpha_byte_rgba,
	convblit_copy_rgba8888_rgba8888,
	convblit_srcover_rgba8888_rgba8888,
	convblit_copy_rgb888_rgba8888,
	frameblit_stretch_xxxa8888
};

static SUBDRIVER fblinear32rgba_down = {
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	frameblit_stretch_xxxa8888,
	convblit_copy_mask_mono_byte_msb_rgba,
	convblit_copy_mask_mono_byte_lsb_rgba,
	convblit_copy_mask_mono_word_msb_rgba,
	convblit_blend_mask_alpha_byte_rgba,
	convblit_copy_rgba8888_rgba8888,
	convblit_srcover_rgba8888_rgba8888,
	convblit_copy_rgb888_rgba8888,
	frameblit_stretch_xxxa8888
};
#endif

PSUBDRIVER fblinear32rgba[4] = {
	&fblinear32rgba_none
#if MW_FEATURE_PORTRAIT
	, &fblinear32rgba_left, &fblinear32rgba_right, &fblinear32rgba_down
#endif
};
