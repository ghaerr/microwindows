/*
 * Copyright (c) 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * Left portrait mode subdriver for Microwindows
 *
 * Left rotation:
 * X -> Y
 * Y -> maxx - X - w
 * W -> H
 * H -> W
 */
#include <string.h>
#include <stdlib.h>
#include "uni_std.h"
#include "device.h"
#include "fb.h"

void
fbportrait_left_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	psd->orgsubdriver->DrawPixel(psd, y, psd->xvirtres-x-1, c);
}

MWPIXELVAL
fbportrait_left_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return psd->orgsubdriver->ReadPixel(psd, y, psd->xvirtres-x-1);
}

void
fbportrait_left_drawhorzline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	/*x2 = psd->xvirtres-x2-1;
	while (x2 <= (psd->xvirtres-x1-1))
		fbportrait_left_drawpixel(psd, y, x2++, c);*/

	psd->orgsubdriver->DrawVertLine(psd, y, psd->xvirtres-x2-1, psd->xvirtres-x1-1, c);
}

void
fbportrait_left_drawvertline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	/*while (y1 <= y2)
		fbportrait_left_drawpixel(psd, y1++, psd->xvirtres-x-1, c);*/

	psd->orgsubdriver->DrawHorzLine(psd, y1, y2, psd->xvirtres-x-1, c);
}

void
fbportrait_left_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	/* temporarily stop updates for speed*/
	void (*Update)(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height) = psd->Update;
	MWCOORD X2;
	MWCOORD W = y2-y1+1;
	MWCOORD H = x2-x1+1;
	psd->Update = NULL;

	x1 = psd->xvirtres-x1-1;
	X2 = x2 = psd->xvirtres-x2-1;
	while(x2 <= x1)
		psd->orgsubdriver->DrawHorzLine(psd, y1, y2, x2++, c);

	/* now redraw once if external update required*/
	if (Update) {
		Update(psd, y1, X2, W, H);
		psd->Update = Update;
	}
}

void
fbportrait_left_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	PSD srcpsd, MWCOORD srcx,MWCOORD srcy,int op)
{
	dstpsd->orgsubdriver->BlitFallback(dstpsd, desty, dstpsd->xvirtres-destx-w,
		h, w, srcpsd, srcy, srcpsd->xvirtres-srcx-w, op);
}

void
fbportrait_left_convblit_blend_mask_alpha_byte(PSD dstpsd, PMWBLITPARMS gc)
{
	ADDR8 alpha_in, alpha_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_h;
	MWBLITPARMS	l_gc;

	if (!dstpsd->orgsubdriver->BlitBlendMaskAlphaByte)
		return;

	/* create new gc with rotated coords*/
	l_gc.width = gc->height;
	l_gc.height = gc->width;
	l_gc.dstx = gc->dsty;
	l_gc.dsty = dstpsd->xvirtres - gc->dstx - gc->width;
	l_gc.srcx = 0;	//FIXME rotate may fail with srcx/srcy not zero
	l_gc.srcy = 0;
	l_gc.src_pitch = l_gc.width;	/* 1 byte alpha channel*/

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.fg_pixelval = gc->fg_pixelval;
	l_gc.bg_pixelval = gc->bg_pixelval;
	l_gc.usebg = gc->usebg;
	l_gc.dst_pitch = gc->dst_pitch;
	l_gc.data_out = gc->data_out;

	if (!(l_gc.data = ALLOCA(l_gc.width * l_gc.height)))
		return;

	alpha_in = ((ADDR8)gc->data) + gc->src_pitch * gc->srcy + gc->srcx;
	in_w = gc->width;
	in_h = gc->height;

	alpha_out = l_gc.data;
	//out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_left_8bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_x;
			out_x = in_y;

			//alpha_out[(out_y * out_w) + out_x] = alpha_in[(in_y * in_w) + in_x];
			alpha_out[(out_y * l_gc.src_pitch) + out_x] = alpha_in[(in_y * gc->src_pitch) + in_x];
		}
	}

	dstpsd->orgsubdriver->BlitBlendMaskAlphaByte(dstpsd, &l_gc);

	FREEA(l_gc.data);
}

void
fbportrait_left_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_h;
	MWBLITPARMS	l_gc;

	if (!psd->orgsubdriver->BlitCopyMaskMonoByteMSB)
		return;

	/* create new gc with rotated coords*/
	l_gc.width = gc->height;
	l_gc.height = gc->width;
	l_gc.dstx = gc->dsty;
	l_gc.dsty = psd->xvirtres - gc->dstx - gc->width;
	l_gc.srcx = 0;	//FIXME rotate may fail with srcx/srcy not zero
	l_gc.srcy = 0;
	l_gc.src_pitch = (l_gc.width + 7) / 8;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.fg_pixelval = gc->fg_pixelval;
	l_gc.bg_pixelval = gc->bg_pixelval;
	l_gc.usebg = gc->usebg;
	l_gc.dst_pitch = gc->dst_pitch;
	l_gc.data_out = gc->data_out;

	if (!(l_gc.data = ALLOCA(l_gc.height * l_gc.src_pitch)))
		return;
	memset(l_gc.data, 0, l_gc.height * l_gc.src_pitch);

	pixel_in = ((ADDR8)gc->data) + gc->src_pitch * gc->srcy + gc->srcx;
	in_w = gc->width;
	in_h = gc->height;

	pixel_out = l_gc.data;
	//out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_left_1bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_x;
			out_x = in_y;

			//pixel_out[(out_y * out_w) + out_x] = pixel_in[(in_y * in_w) + in_x];
			if (pixel_in[in_y*gc->src_pitch + (in_x >> 3)] & (0x80 >> (in_x&7)))
				pixel_out[out_y*l_gc.src_pitch + (out_x >> 3)] |= (0x80 >> (out_x&7));
		}
	}

	psd->orgsubdriver->BlitCopyMaskMonoByteMSB(psd, &l_gc);

	FREEA(l_gc.data);
}

void
fbportrait_left_convblit_copy_mask_mono_byte_lsb(PSD psd, PMWBLITPARMS gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_h;
	MWBLITPARMS	l_gc;

	if (!psd->orgsubdriver->BlitCopyMaskMonoByteLSB)
		return;

	/* create new gc with rotated coords*/
	l_gc.width = gc->height;
	l_gc.height = gc->width;
	l_gc.dstx = gc->dsty;
	l_gc.dsty = psd->xvirtres - gc->dstx - gc->width;
	l_gc.srcx = 0;	//FIXME rotate may fail with srcx/srcy not zero
	l_gc.srcy = 0;
	l_gc.src_pitch = (l_gc.width + 7) / 8;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.fg_pixelval = gc->fg_pixelval;
	l_gc.bg_pixelval = gc->bg_pixelval;
	l_gc.usebg = gc->usebg;
	l_gc.dst_pitch = gc->dst_pitch;

	if (!(l_gc.data = ALLOCA(l_gc.height * l_gc.src_pitch)))
		return;
	memset(l_gc.data, 0, l_gc.height * l_gc.src_pitch);

	pixel_in = ((ADDR8)gc->data) + gc->src_pitch * gc->srcy + gc->srcx;
	in_w = gc->width;
	in_h = gc->height;

	pixel_out = l_gc.data;
	//out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_left_1bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_x;
			out_x = in_y;

			//pixel_out[(out_y * out_w) + out_x] = pixel_in[(in_y * in_w) + in_x];
			if (pixel_in[in_y*gc->src_pitch + (in_x >> 3)] & (0x01 << (in_x&7)))
				pixel_out[out_y*l_gc.src_pitch + (out_x >> 3)] |= (0x01 << (out_x&7));
		}
	}

	psd->orgsubdriver->BlitCopyMaskMonoByteLSB(psd, &l_gc);

	FREEA(l_gc.data);
}

SUBDRIVER fbportrait_left = {
	fbportrait_left_drawpixel,
	fbportrait_left_readpixel,
	fbportrait_left_drawhorzline,
	fbportrait_left_drawvertline,
	fbportrait_left_fillrect,
	fbportrait_left_blit,
	NULL,		/* FrameBlit*/
	NULL,		/* FrameStretchBlit*/
	fbportrait_left_convblit_copy_mask_mono_byte_msb,	/* FT2 non-alias*/
	fbportrait_left_convblit_copy_mask_mono_byte_lsb,	/* T1LIB non-alias*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	fbportrait_left_convblit_blend_mask_alpha_byte,		/* FT2/T1 anti-alias*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL		/* BlitCopyRGB888*/
};
