/*
 * Copyright (c) 2000, 2001 Greg Haerr <greg@censoft.com>
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

	return mempsd;
}

/* initialize memory device with passed parms*/
void
initmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
	int size,void *addr)
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
	mempsd->linelen = linelen;
	mempsd->size = size;
	mempsd->addr = addr;
}

void
gen_freememgc(PSD mempsd)
{
	assert(mempsd->flags & PSF_MEMORY);

	/* note: mempsd->addr must be freed elsewhere*/

	free(mempsd);
}

void
gen_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
	while(y1 <= y2)
		psd->DrawHorzLine(psd, x1, x2, y1++, c);
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
	psd->Blit 		= subdriver->Blit;
	psd->DrawArea 		= subdriver->DrawArea;
	psd->StretchBlit 	= subdriver->StretchBlit;

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
	subdriver->FillRect	 	= psd->FillRect;
	subdriver->Blit 		= psd->Blit;
	subdriver->DrawArea 		= psd->DrawArea;
	subdriver->StretchBlit 		= psd->StretchBlit;
}
