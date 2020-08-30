/*
 * Copyright (c) 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * 24bpp Linear Video Driver for Microwindows (BGR byte order)
 * Writes memory image: |B|G|R| MWPF_TRUECOLOR888
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
linear24_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + x * 3;
	MWUCHAR r = PIXEL888RED(c);
	MWUCHAR g = PIXEL888GREEN(c);
	MWUCHAR b = PIXEL888BLUE(c);
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
	{
		addr[0] = b;
		addr[1] = g;
		addr[2] = r;
	}
	else
	{
		APPLYOP(gr_mode, 1, (MWUCHAR), b, *(ADDR8), addr, 0, 1);
		APPLYOP(gr_mode, 1, (MWUCHAR), g, *(ADDR8), addr, 0, 1);
		APPLYOP(gr_mode, 1, (MWUCHAR), r, *(ADDR8), addr, 0, 1);
	}
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y, 1, 1);
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear24_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + x * 3;
#if DEBUG
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
#endif
	return RGB2PIXEL888(addr[2], addr[1], addr[0]);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear24_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + x1 * 3;
	MWUCHAR r = PIXEL888RED(c);
	MWUCHAR g = PIXEL888GREEN(c);
	MWUCHAR b = PIXEL888BLUE(c);
	int w = x2-x1+1;
#if DEBUG
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
#endif
	DRAWON;
	if(gr_mode == MWROP_COPY)
	{
		while(--w >= 0)
		{
			*addr++ = b;
			*addr++ = g;
			*addr++ = r;
		}
	}
	else
	{
		while(--w >= 0)
		{
			APPLYOP(gr_mode, 1, (MWUCHAR), b, *(ADDR8), addr, 0, 1);
			APPLYOP(gr_mode, 1, (MWUCHAR), g, *(ADDR8), addr, 0, 1);
			APPLYOP(gr_mode, 1, (MWUCHAR), r, *(ADDR8), addr, 0, 1);
		}
	}
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x1, y, x2-x1+1, 1);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear24_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	pitch = psd->pitch;
	register unsigned char *addr = psd->addr + y1 * pitch + x * 3;
	MWUCHAR r = PIXEL888RED(c);
	MWUCHAR g = PIXEL888GREEN(c);
	MWUCHAR b = PIXEL888BLUE(c);
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
		while (--height >= 0)
		{
			addr[0] = b;
			addr[1] = g;
			addr[2] = r;
			addr += pitch;
		}
	}
	else
	{
		while (--height >= 0)
		{
			APPLYOP(gr_mode, 1, (MWUCHAR), b, *(ADDR8), addr, 0, 1);
			APPLYOP(gr_mode, 1, (MWUCHAR), g, *(ADDR8), addr, 0, 1);
			APPLYOP(gr_mode, 1, (MWUCHAR), r, *(ADDR8), addr, 0, 1);
			addr += pitch - 3;
		}
	}
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y1, 1, y2-y1+1);
}

static SUBDRIVER fblinear24_none = {
	linear24_drawpixel,
	linear24_readpixel,
	linear24_drawhorzline,
	linear24_drawvertline,
	gen_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_24bpp,
	frameblit_stretch_24bpp,
	convblit_copy_mask_mono_byte_msb_bgr,		/* ft2 non-alias*/
	convblit_copy_mask_mono_byte_lsb_bgr,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_bgr,		/* core/pcf non-alias*/
	convblit_blend_mask_alpha_byte_bgr,			/* ft2/t1 antialias*/
	convblit_copy_rgba8888_bgr888,				/* RGBA image copy (GdArea MWPF_RGB)*/
	convblit_srcover_rgba8888_bgr888,			/* RGBA images w/alpha*/
	convblit_copy_rgb888_bgr888, 				/* RGB images no alpha*/
	frameblit_stretch_rgba8888_bgr888			/* RGBA stretchblit*/
};

#if MW_FEATURE_PORTRAIT	
static SUBDRIVER fblinear24_left = {
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_24bpp,
	frameblit_stretch_24bpp,
	convblit_copy_mask_mono_byte_msb_bgr,
	convblit_copy_mask_mono_byte_lsb_bgr,
	convblit_copy_mask_mono_word_msb_bgr,
	convblit_blend_mask_alpha_byte_bgr,
	convblit_copy_rgba8888_bgr888,
	convblit_srcover_rgba8888_bgr888,
	convblit_copy_rgb888_bgr888,
	frameblit_stretch_rgba8888_bgr888
};

static SUBDRIVER fblinear24_right = {
	fbportrait_right_drawpixel,
	fbportrait_right_readpixel,
	fbportrait_right_drawhorzline,
	fbportrait_right_drawvertline,
	fbportrait_right_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_24bpp,
	frameblit_stretch_24bpp,
	convblit_copy_mask_mono_byte_msb_bgr,
	convblit_copy_mask_mono_byte_lsb_bgr,
	convblit_copy_mask_mono_word_msb_bgr,
	convblit_blend_mask_alpha_byte_bgr,
	convblit_copy_rgba8888_bgr888,
	convblit_srcover_rgba8888_bgr888,
	convblit_copy_rgb888_bgr888,
	frameblit_stretch_rgba8888_bgr888
};

static SUBDRIVER fblinear24_down = {
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_24bpp,
	frameblit_stretch_24bpp,
	convblit_copy_mask_mono_byte_msb_bgr,
	convblit_copy_mask_mono_byte_lsb_bgr,
	convblit_copy_mask_mono_word_msb_bgr,
	convblit_blend_mask_alpha_byte_bgr,
	convblit_copy_rgba8888_bgr888,
	convblit_srcover_rgba8888_bgr888,
	convblit_copy_rgb888_bgr888,
	frameblit_stretch_rgba8888_bgr888
};
#endif

PSUBDRIVER fblinear24[4] = {
	&fblinear24_none
#if MW_FEATURE_PORTRAIT
	, &fblinear24_left, &fblinear24_right, &fblinear24_down
#endif
};
