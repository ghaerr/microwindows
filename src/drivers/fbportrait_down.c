/*
 * Copyright (c) 2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * Portrait mode subdriver for Microwindows
 */
#include "device.h"
#include "fb.h"

static void
fbportrait_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	x = psd->xvirtres-x-1;
	psd->orgsubdriver->DrawPixel(psd, x, psd->yvirtres-y-1, c);
}

static MWPIXELVAL
fbportrait_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	x = psd->xvirtres-x-1;
	return psd->orgsubdriver->ReadPixel(psd, x, psd->yvirtres-y-1);
}

static void
fbportrait_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	x1 = psd->xvirtres-x1-1;
	x2 = psd->xvirtres-x2-1;
	psd->orgsubdriver->DrawHorzLine(psd, x2, x1, psd->yvirtres-y-1, c);
}

static void
fbportrait_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	x = psd->xvirtres-x-1;
	psd->orgsubdriver->DrawVertLine(psd, x, psd->yvirtres-y2-1, psd->yvirtres-y1-1, c);
}

static void
fbportrait_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	//y2 = psd->yvirtres-y2-1;
	//y1 = psd->yvirtres-y1-1;
	//x1 = psd->xvirtres-x1-1;
	//x2 = psd->xvirtres-x2-1;
	while (y1 <= y2)
		psd->DrawHorzLine(psd, x1, x2, y1++, c);
}

static void
fbportrait_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
    dstpsd->orgsubdriver->Blit(dstpsd, dstpsd->xvirtres-destx-w, dstpsd->yvirtres-desty-h,   
 		w, h, srcpsd, srcpsd->xvirtres-srcx-w, srcpsd->yvirtres-srcy-h, op);  

}

static void
fbportrait_stretchblit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, long op)
{
	//dstpsd->orgsubdriver->StretchBlit(dstpsd, dstpsd->yvirtres-desty-dsth, destx,
		//dsth, dstw, srcpsd, srcpsd->yvirtres-srcy-srch, srcx,
		//srch, srcw, op);
}

#if MW_FEATURE_PSDOP_ALPHACOL
static void
fbportrait_drawarea_alphacol(PSD dstpsd, driver_gc_t * gc)
{
	ADDR8 alpha_in, alpha_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	driver_gc_t	l_gc;

	l_gc = *gc;
	l_gc.dstx = dstpsd->xvirtres - gc->dstx - gc->dstw;
	l_gc.dsty = dstpsd->yvirtres - gc->dsty - gc->dsth;
	//l_gc.dstw = gc->dsth;
	//l_gc.dsth = gc->dstw;

	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_linelen = l_gc.dstw;
	if (!(l_gc.misc = ALLOCA(l_gc.dstw * l_gc.dsth)))
		return;

	alpha_in = ((ADDR8)gc->misc) + gc->src_linelen * gc->srcy + gc->srcx;
	in_w = gc->dstw;
	in_h = gc->dsth;

	alpha_out = l_gc.misc;
	out_w = l_gc.dstw;
	out_h = l_gc.dsth;

	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_y;
			out_x = (out_w - 1) - in_x;

			alpha_out[(out_y * out_w) + out_x] = alpha_in[(in_y * in_w) + in_x];
		}
	}

	dstpsd->orgsubdriver->DrawArea(dstpsd, &l_gc, PSDOP_ALPHACOL);

	FREEA(l_gc.misc);
}
#endif

static void
fbportrait_drawarea(PSD dstpsd, driver_gc_t * gc, int op)
{
	if (!dstpsd->orgsubdriver->DrawArea)
		return;

	switch(op) {
#if MW_FEATURE_PSDOP_ALPHACOL
	case PSDOP_ALPHACOL:
		fbportrait_drawarea_alphacol(dstpsd, gc);
		break;
#endif
	}
}

SUBDRIVER fbportrait_down = {
	NULL,
	fbportrait_drawpixel,
	fbportrait_readpixel,
	fbportrait_drawhorzline,
	fbportrait_drawvertline,
	fbportrait_fillrect,
	fbportrait_blit,
	fbportrait_drawarea,
	fbportrait_stretchblit
};
