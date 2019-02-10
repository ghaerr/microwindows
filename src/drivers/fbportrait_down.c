/*
 * Copyright (c) 2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * Portrait mode subdriver for Microwindows
 *
 * Down rotation:
 * X -> xmax - X - w
 * Y -> ymax - Y - h
 * W -> W
 * H -> H
 */
#include <string.h>
#include <stdlib.h>
#include "uni_std.h"
#include "device.h"
#include "fb.h"

void
fbportrait_down_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	x = psd->xvirtres-x-1;
	psd->orgsubdriver->DrawPixel(psd, x, psd->yvirtres-y-1, c);
}

MWPIXELVAL
fbportrait_down_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	x = psd->xvirtres-x-1;
	return psd->orgsubdriver->ReadPixel(psd, x, psd->yvirtres-y-1);
}

void
fbportrait_down_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	x1 = psd->xvirtres-x1-1;
	x2 = psd->xvirtres-x2-1;
	psd->orgsubdriver->DrawHorzLine(psd, x2, x1, psd->yvirtres-y-1, c);
}

void
fbportrait_down_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	x = psd->xvirtres-x-1;
	psd->orgsubdriver->DrawVertLine(psd, x, psd->yvirtres-y2-1, psd->yvirtres-y1-1, c);
}

void
fbportrait_down_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	/* temporarily stop updates for speed*/
	void (*Update)(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height) = psd->Update;
	MWCOORD Y1 = y1;
	psd->Update = NULL;

	//y2 = psd->yvirtres-y2-1;
	//y1 = psd->yvirtres-y1-1;
	//x1 = psd->xvirtres-x1-1;
	//x2 = psd->xvirtres-x2-1;
	while (y1 <= y2)
		psd->DrawHorzLine(psd, x1, x2, y1++, c);

	/* now redraw once if external update required*/
	if (Update) {
		MWCOORD W = x2-x1+1;
		MWCOORD H = y2-Y1+1;
		y2 = psd->yvirtres-y2-1;
		x2 = psd->xvirtres-x2-1;
		Update(psd, x2, y2, W, H);
		psd->Update = Update;
	}
}

void
fbportrait_down_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
    dstpsd->orgsubdriver->BlitFallback(dstpsd, dstpsd->xvirtres-destx-w, dstpsd->yvirtres-desty-h,   
 		w, h, srcpsd, srcpsd->xvirtres-srcx-w, srcpsd->yvirtres-srcy-h, op);  

}

void
fbportrait_down_convblit_blend_mask_alpha_byte(PSD dstpsd, PMWBLITPARMS gc)
{
	ADDR8 alpha_in, alpha_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	MWBLITPARMS	l_gc;

	if (!dstpsd->orgsubdriver->BlitBlendMaskAlphaByte)
		return;

	/* create new gc with flipped coords*/
	l_gc.dstx = dstpsd->xvirtres - gc->dstx - gc->width;
	l_gc.dsty = dstpsd->yvirtres - gc->dsty - gc->height;
	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_pitch = gc->width;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.width = gc->width;
	l_gc.height = gc->height;
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
	out_w = l_gc.width;
	out_h = l_gc.height;

	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_y;
			out_x = (out_w - 1) - in_x;

			//alpha_out[(out_y * out_w) + out_x] = alpha_in[(in_y * in_w) + in_x];
			alpha_out[(out_y * l_gc.src_pitch) + out_x] = alpha_in[(in_y * gc->src_pitch) + in_x];
		}
	}

	dstpsd->orgsubdriver->BlitBlendMaskAlphaByte(dstpsd, &l_gc);

	FREEA(l_gc.data);
}

void
fbportrait_down_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	MWBLITPARMS	l_gc;

	if (!psd->orgsubdriver->BlitCopyMaskMonoByteMSB)
		return;

	/* create new gc with flipped coords*/
	l_gc.dstx = psd->xvirtres - gc->dstx - gc->width;
	l_gc.dsty = psd->yvirtres - gc->dsty - gc->height;
	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_pitch = gc->width;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.width = gc->width;
	l_gc.height = gc->height;
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
	out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_down_1bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_y;
			out_x = (out_w - 1) - in_x;

			//pixel_out[(out_y * out_w) + out_x] = pixel_in[(in_y * in_w) + in_x];
			if (pixel_in[in_y*gc->src_pitch + (in_x >> 3)] & (0x80 >> (in_x&7)))
				pixel_out[out_y*l_gc.src_pitch + (out_x >> 3)] |= (0x80 >> (out_x&7));
		}
	}

	psd->orgsubdriver->BlitCopyMaskMonoByteMSB(psd, &l_gc);

	FREEA(l_gc.data);
}

void
fbportrait_down_convblit_copy_mask_mono_byte_lsb(PSD psd, PMWBLITPARMS gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	MWBLITPARMS	l_gc;

	if (!psd->orgsubdriver->BlitCopyMaskMonoByteLSB)
		return;

	/* create new gc with flipped coords*/
	l_gc.dstx = psd->xvirtres - gc->dstx - gc->width;
	l_gc.dsty = psd->yvirtres - gc->dsty - gc->height;
	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_pitch = gc->width;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.width = gc->width;
	l_gc.height = gc->height;
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
	out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_down_1bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_y;
			out_x = (out_w - 1) - in_x;

			//pixel_out[(out_y * out_w) + out_x] = pixel_in[(in_y * in_w) + in_x];
			if (pixel_in[in_y*gc->src_pitch + (in_x >> 3)] & (0x01 << (in_x&7)))
				pixel_out[out_y*l_gc.src_pitch + (out_x >> 3)] |= (0x01 << (out_x&7));
		}
	}

	psd->orgsubdriver->BlitCopyMaskMonoByteLSB(psd, &l_gc);

	FREEA(l_gc.data);
}

SUBDRIVER fbportrait_down = {
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	fbportrait_down_blit,
	NULL,		/* FrameBlit*/
	NULL,		/* FrameStretchBlit*/
	fbportrait_down_convblit_copy_mask_mono_byte_msb,	/* FT2 non-alias*/
	fbportrait_down_convblit_copy_mask_mono_byte_lsb,	/* T1LIB non-alias*/
	NULL,		/* BlitCopyMaskMonoWordMSB*/
	fbportrait_down_convblit_blend_mask_alpha_byte,		/* FT2/T1 anti-alias*/
	NULL,		/* BlitCopyRGBA8888*/
	NULL,		/* BlitSrcOverRGBA8888*/
	NULL		/* BlitCopyRGB888*/
};
