/*
 * Copyright (c) 2001 Greg Haerr <greg@censoft.com>
 *
 * Portrait mode subdriver for Microwindows
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "fb.h"

static void
fbportrait_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	//x = psd->xvirtres-x-1;
	psd->orgsubdriver->DrawPixel(psd, x, psd->yvirtres-y-1, c);
}

static MWPIXELVAL
fbportrait_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	//x = psd->xvirtres-x-1;
	return psd->orgsubdriver->ReadPixel(psd, x, psd->yvirtres-y-1);
}

static void
fbportrait_drawhorzline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	//x1 = psd->xvirtres-x1-1;
	//x2 = psd->xvirtres-x2-1;
	psd->orgsubdriver->DrawHorzLine(psd, x1, x2, psd->yvirtres-y-1, c);
}

static void
fbportrait_drawvertline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	//x = psd->xvirtres-x-1;
	psd->orgsubdriver->DrawVertLine(psd, x, psd->yvirtres-y2-1,
		psd->yvirtres-y1-1, c);
}

static void
fbportrait_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
	y2 = psd->yvirtres-y2-1;
	y1 = psd->yvirtres-y1-1;
	//x1 = psd->xvirtres-x2-1;
	//x2 = psd->xvirtres-x1-1;
	while(y2 <= y1)
		psd->DrawHorzLine(psd, x1, x2, y2++, c);
}

static void
fbportrait_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	PSD srcpsd, MWCOORD srcx,MWCOORD srcy,long op)
{
	//dstpsd->orgsubdriver->Blit(dstpsd, desty, dstpsd->xvirtres-destx-w,
		//h, w, srcpsd, srcy, srcpsd->xvirtres-srcx-w, op);
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

SUBDRIVER fbportrait_down = {
	NULL,
	fbportrait_drawpixel,
	fbportrait_readpixel,
	fbportrait_drawhorzline,
	fbportrait_drawvertline,
	gen_fillrect,
	fbportrait_blit,
	NULL,
	fbportrait_stretchblit
};
