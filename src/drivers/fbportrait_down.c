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
	//y2 = psd->yvirtres-y2-1;
	//y1 = psd->yvirtres-y1-1;
	//x1 = psd->xvirtres-x1-1;
	//x2 = psd->xvirtres-x2-1;
	while (y1 <= y2)
		psd->DrawHorzLine(psd, x1, x2, y1++, c);
}

void
fbportrait_down_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
    dstpsd->orgsubdriver->Blit(dstpsd, dstpsd->xvirtres-destx-w, dstpsd->yvirtres-desty-h,   
 		w, h, srcpsd, srcpsd->xvirtres-srcx-w, srcpsd->yvirtres-srcy-h, op);  

}

void
fbportrait_down_stretchblitex(PSD dstpsd, PSD srcpsd, MWCOORD dest_x_start, int dest_y_start,
	MWCOORD width, int height, int x_denominator, int y_denominator,
	int src_x_fraction, int src_y_fraction,
	int x_step_fraction, int y_step_fraction, int op)
{ 
	// X -> xmax - X - w
	// Y -> ymax - Y - h
	dstpsd->orgsubdriver->StretchBlitEx(dstpsd, srcpsd,
		dstpsd->xvirtres - dest_x_start - width, dstpsd->yvirtres - dest_y_start - height,
		width, height, x_denominator, y_denominator,
		srcpsd->xvirtres - src_x_fraction - width, srcpsd->yvirtres - src_y_fraction - height,
		x_step_fraction, y_step_fraction,
		op);
}

static void
fbportrait_down_drawarea_alphacol(PSD dstpsd, driver_gc_t * gc)
{
	ADDR8 alpha_in, alpha_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	driver_gc_t	l_gc;

	/* create new gc with flipped coords*/
	l_gc.dstx = dstpsd->xvirtres - gc->dstx - gc->width;
	l_gc.dsty = dstpsd->yvirtres - gc->dsty - gc->height;
	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_linelen = gc->width;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.width = gc->width;
	l_gc.height = gc->height;
	l_gc.fg_color = gc->fg_color;
	l_gc.bg_color = gc->bg_color;
	l_gc.usebg = gc->usebg;
	l_gc.dst_linelen = gc->dst_linelen;

	if (!(l_gc.data = ALLOCA(l_gc.width * l_gc.height)))
		return;

	alpha_in = ((ADDR8)gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
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
			alpha_out[(out_y * l_gc.src_linelen) + out_x] = alpha_in[(in_y * gc->src_linelen) + in_x];
		}
	}

	dstpsd->orgsubdriver->DrawArea(dstpsd, &l_gc);

	FREEA(l_gc.data);
}

static void
fbportrait_down_drawarea_bitmap_bytes_msb_first(PSD psd, driver_gc_t * gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	driver_gc_t	l_gc;

	/* create new gc with flipped coords*/
	l_gc.dstx = psd->xvirtres - gc->dstx - gc->width;
	l_gc.dsty = psd->yvirtres - gc->dsty - gc->height;
	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_linelen = gc->width;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.width = gc->width;
	l_gc.height = gc->height;
	l_gc.fg_color = gc->fg_color;
	l_gc.bg_color = gc->bg_color;
	l_gc.usebg = gc->usebg;
	l_gc.dst_linelen = gc->dst_linelen;

	if (!(l_gc.data = ALLOCA(l_gc.height * l_gc.src_linelen)))
		return;
	memset(l_gc.data, 0, l_gc.height * l_gc.src_linelen);

	pixel_in = ((ADDR8)gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
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
			if (pixel_in[in_y*gc->src_linelen + (in_x >> 3)] & (0x80 >> (in_x&7)))
				pixel_out[out_y*l_gc.src_linelen + (out_x >> 3)] |= (0x80 >> (out_x&7));
		}
	}

	psd->orgsubdriver->DrawArea(psd, &l_gc);

	FREEA(l_gc.data);
}

static void
fbportrait_down_drawarea_bitmap_bytes_lsb_first(PSD psd, driver_gc_t * gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	driver_gc_t	l_gc;

	/* create new gc with flipped coords*/
	l_gc.dstx = psd->xvirtres - gc->dstx - gc->width;
	l_gc.dsty = psd->yvirtres - gc->dsty - gc->height;
	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_linelen = gc->width;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.width = gc->width;
	l_gc.height = gc->height;
	l_gc.fg_color = gc->fg_color;
	l_gc.bg_color = gc->bg_color;
	l_gc.usebg = gc->usebg;
	l_gc.dst_linelen = gc->dst_linelen;

	if (!(l_gc.data = ALLOCA(l_gc.height * l_gc.src_linelen)))
		return;
	memset(l_gc.data, 0, l_gc.height * l_gc.src_linelen);

	pixel_in = ((ADDR8)gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
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
			if (pixel_in[in_y*gc->src_linelen + (in_x >> 3)] & (0x01 << (in_x&7)))
				pixel_out[out_y*l_gc.src_linelen + (out_x >> 3)] |= (0x01 << (out_x&7));
		}
	}

	psd->orgsubdriver->DrawArea(psd, &l_gc);

	FREEA(l_gc.data);
}

void
fbportrait_down_drawarea(PSD dstpsd, driver_gc_t * gc)
{
	if (!dstpsd->orgsubdriver->DrawArea)
		return;

	switch(gc->op) {
	case PSDOP_ALPHACOL:
		fbportrait_down_drawarea_alphacol(dstpsd, gc);
		break;

	case PSDOP_BITMAP_BYTES_MSB_FIRST:
		fbportrait_down_drawarea_bitmap_bytes_msb_first(dstpsd, gc);
		break;

	case PSDOP_BITMAP_BYTES_LSB_FIRST:
		fbportrait_down_drawarea_bitmap_bytes_lsb_first(dstpsd, gc);
		break;
	}
}

SUBDRIVER fbportrait_down = {
	NULL,
	fbportrait_down_drawpixel,
	fbportrait_down_readpixel,
	fbportrait_down_drawhorzline,
	fbportrait_down_drawvertline,
	fbportrait_down_fillrect,
	fbportrait_down_blit,
	fbportrait_down_drawarea,
	fbportrait_down_stretchblitex
};
