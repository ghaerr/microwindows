/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
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
	psd->orgsubdriver->DrawPixel(psd, psd->yvirtres-y-1, x, c);
}

static MWPIXELVAL
fbportrait_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return psd->orgsubdriver->ReadPixel(psd, psd->yvirtres-y-1, x);
}

static void
fbportrait_drawhorzline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	psd->orgsubdriver->DrawVertLine(psd, psd->yvirtres-y-1, x1,
		x2, c);

	/*
	 * Uncomment the following if driver doesn't support hline
	x2 = x2;
	while(x2 <= (x1))
		fb_drawpixel(psd, psd->yvirtres-y-1, x2++, c);
	 */
}

static void
fbportrait_drawvertline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	psd->orgsubdriver->DrawHorzLine(psd, psd->yvirtres-y2-1, psd->yvirtres-y1-1,
		x, c);

	/*
	 * Uncomment the following if driver doesn't support vline
	while(y1 <= y2)
		fb_drawpixel(psd, psd->yvirtres-1-(y1++), x, c);
	 */
}

static void
fbportrait_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
	while(x2 <= x1)
		psd->orgsubdriver->DrawHorzLine(psd, psd->yvirtres-y2-1,
			psd->yvirtres-y1-1, x2++, c);
}

static void
fbportrait_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	PSD srcpsd, MWCOORD srcx,MWCOORD srcy,long op)
{
	dstpsd->orgsubdriver->Blit(dstpsd, dstpsd->yvirtres-desty-h, destx,
		h, w, srcpsd, srcpsd->yvirtres-srcy-h, srcx, op);
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

SUBDRIVER fbportrait_right = {
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
