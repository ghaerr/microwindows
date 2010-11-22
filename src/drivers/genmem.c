/*
 * Copyright (c) 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Screen Driver Utilities
 * 
 * Microwindows memory device routines
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"

/* allocate a memory screen device*/
PSD 
gen_allocatememgc(PSD psd)
{
	PSD	mempsd;

	/* if driver doesn't have blit, fail*/
	if((psd->flags & PSF_HAVEBLIT) == 0)
		return NULL;

	mempsd = malloc(sizeof(SCREENDEVICE));
	if (!mempsd)
		return NULL;

	/* copy passed device get initial values*/
	*mempsd = *psd;

	/* initialize*/
	mempsd->flags |= PSF_MEMORY;
	mempsd->flags &= ~PSF_SCREEN;
	mempsd->addr = NULL;
	mempsd->Update = NULL;			/* no external updates required for mem device*/

	return mempsd;
}

/* initialize memory device with passed parms*/
void
gen_initmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
	int data_format,int pitch,int size,void *addr)
{
	assert(mempsd->flags & PSF_MEMORY);

	/* create mem psd w/h aligned with hw screen w/h*/
	if (mempsd->portrait & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT)) {
		mempsd->yres = w;
		mempsd->xres = h;
	} else {
		mempsd->xres = w;
		mempsd->yres = h;		
	}
	mempsd->xvirtres = w;
	mempsd->yvirtres = h;
	mempsd->planes = planes;
	mempsd->bpp = bpp;
	mempsd->data_format = data_format;
	mempsd->linelen = linelen;
	mempsd->pitch = pitch;
	mempsd->size = size;
	mempsd->addr = addr;
}

/* 
 * Initialize memory device with passed parms,
 * select suitable framebuffer subdriver,
 * and set subdriver in memory device.
 *
 * Pixmaps are always drawn using linear fb drivers,
 * and drawn using portrait mode subdrivers if in portrait mode,
 * then blitted using swapped x,y coords for speed with
 * no rotation required.
 */
MWBOOL
gen_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
	int data_format,int pitch,int size,void *addr)
{
	PSUBDRIVER subdriver;

	/* initialize mem screen driver*/
	gen_initmemgc(mempsd, w, h, planes, bpp, data_format, linelen, pitch, size, addr);

	/* select and init hw compatible framebuffer subdriver for pixmap drawing*/
	subdriver = select_fb_subdriver(mempsd);
	if(!subdriver || !subdriver->Init(mempsd))
		return 0;

	/* pixmap portrait subdriver will callback fb drivers, not screen drivers*/
	//mempsd->orgsubdriver = subdriver;

	/* assign portrait subdriver or regular fb driver for pixmap drawing*/
	set_portrait_subdriver(mempsd);

	return 1;
}

void
gen_freememgc(PSD mempsd)
{
	assert(mempsd->flags & PSF_MEMORY);

	/* note: mempsd->addr must be freed elsewhere*/

	free(mempsd);
}

void
gen_setportrait(PSD psd, int portraitmode)
{
	psd->portrait = portraitmode;

	/* swap x and y in left or right portrait modes*/
	if (portraitmode & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT)) {
		/* swap x, y*/
		psd->xvirtres = psd->yres;
		psd->yvirtres = psd->xres;
	} else {
		/* normal x, y*/
		psd->xvirtres = psd->xres;
		psd->yvirtres = psd->yres;
	}

	/* assign portrait subdriver or original driver*/
	set_portrait_subdriver(psd);
}

/*
 * Set portrait subdriver or original subdriver according
 * to current portrait mode.
 */
void
set_portrait_subdriver(PSD psd)
{
	PSUBDRIVER subdriver;

	switch (psd->portrait) {
	case MWPORTRAIT_NONE:
		subdriver = psd->orgsubdriver;
		break;
	case MWPORTRAIT_LEFT:
		subdriver = psd->left_subdriver;
		break;
	case MWPORTRAIT_RIGHT:
		subdriver = psd->right_subdriver;
		break;
	case MWPORTRAIT_DOWN:
		subdriver = psd->down_subdriver;
		break;
	}
	set_subdriver(psd, subdriver, FALSE);
}

void
gen_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
	/* temporarily stop updates for speed*/
	void (*Update)(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height) = psd->Update;
	int X1 = x1;
	int Y1 = y1;
	psd->Update = NULL;

	if (psd->portrait & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT))
		while(x1 <= x2)
			psd->DrawVertLine(psd, x1++, y1, y2, c);
	else
		while(y1 <= y2)
			psd->DrawHorzLine(psd, x1, x2, y1++, c);

	/* now redraw once if external update required*/
	if (Update) {
		Update(psd, X1, Y1, x2-X1+1, y2-Y1+1);
		psd->Update = Update;
	}
}

/*
 * Set subdriver entry points in screen device
 * Initialize subdriver if init flag is TRUE
 * Return 0 on fail
 */
MWBOOL
set_subdriver(PSD psd, PSUBDRIVER subdriver, MWBOOL init)
{
	/* set subdriver entry points in screen driver*/
	psd->DrawPixel 		= subdriver->DrawPixel;
	psd->ReadPixel 		= subdriver->ReadPixel;
	psd->DrawHorzLine 	= subdriver->DrawHorzLine;
	psd->DrawVertLine 	= subdriver->DrawVertLine;
	psd->FillRect	 	= subdriver->FillRect;
	psd->Blit 			= subdriver->Blit;
	psd->DrawArea 		= subdriver->DrawArea;
	psd->StretchBlitEx	= subdriver->StretchBlitEx;
	psd->BlitCopyMaskMonoByteMSB = subdriver->BlitCopyMaskMonoByteMSB;
	psd->BlitCopyMaskMonoByteLSB = subdriver->BlitCopyMaskMonoByteLSB;
	psd->BlitCopyMaskMonoWordMSB = subdriver->BlitCopyMaskMonoWordMSB;
	psd->BlitBlendMaskAlphaByte  = subdriver->BlitBlendMaskAlphaByte;
	psd->BlitSrcOverRGBA8888     = subdriver->BlitSrcOverRGBA8888;
	psd->BlitCopyRGB888          = subdriver->BlitCopyRGB888;

	/* call driver init procedure to calc map size and linelen*/
	if (init && !subdriver->Init(psd))
		return 0;
	return 1;
}

/* fill in a subdriver struct from passed screen device*/
void
get_subdriver(PSD psd, PSUBDRIVER subdriver)
{
	/* set subdriver entry points in screen driver*/
	subdriver->DrawPixel 		= psd->DrawPixel;
	subdriver->ReadPixel 		= psd->ReadPixel;
	subdriver->DrawHorzLine 	= psd->DrawHorzLine;
	subdriver->DrawVertLine 	= psd->DrawVertLine;
	subdriver->FillRect	 		= psd->FillRect;
	subdriver->Blit 			= psd->Blit;
	subdriver->DrawArea 		= psd->DrawArea;
	subdriver->StretchBlitEx	= psd->StretchBlitEx;
	subdriver->BlitCopyMaskMonoByteMSB = psd->BlitCopyMaskMonoByteMSB;
	subdriver->BlitCopyMaskMonoByteLSB = psd->BlitCopyMaskMonoByteLSB;
	subdriver->BlitCopyMaskMonoWordMSB = psd->BlitCopyMaskMonoWordMSB;
	subdriver->BlitBlendMaskAlphaByte  = psd->BlitBlendMaskAlphaByte;
	subdriver->BlitSrcOverRGBA8888     = psd->BlitSrcOverRGBA8888;
	subdriver->BlitCopyRGB888          = psd->BlitCopyRGB888;
}
