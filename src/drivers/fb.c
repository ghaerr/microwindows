/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows framebuffer selection routines
 * Including this file will drag in all fblinX framebuffer subdrivers
 * When a new framebuffer subdriver is written, it should be referenced
 * here in select_fb_driver.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "genmem.h"
#include "fb.h"

/* select a framebuffer subdriver based on planes and bpp*/
/* modify this procedure to add a new framebuffer subdriver*/
PSUBDRIVER 
select_fb_subdriver(PSD psd)
{
	PSUBDRIVER  driver = NULL;
	extern SUBDRIVER fblinear1;
	extern SUBDRIVER fblinear2;
	extern SUBDRIVER fblinear4;
	extern SUBDRIVER fblinear8;
	extern SUBDRIVER fblinear16;
	extern SUBDRIVER fblinear24;
	extern SUBDRIVER fblinear32;
#if FBVGA
	extern SUBDRIVER vgaplan4;
	extern SUBDRIVER memplan4;

	/* FB_TYPE_VGA_PLANES*/
	if(psd->planes == 4 && psd->bpp == 4) {
		if(psd->flags & PSF_MEMORY)
			driver = &memplan4;
		else driver = &vgaplan4;
	}
#endif

	/* FB_TYPE_PACKED_PIXELS*/
	/* device and memory drivers are the same for packed pixels*/
	if(psd->planes == 1) {
		switch(psd->bpp) {
		case 1:
			driver = &fblinear1;
			break;
		case 2:
			driver = &fblinear2;
			break;
		case 4:
			driver = &fblinear4;
			break;
		case 8:
			driver = &fblinear8;
			break;
		case 16:
			driver = &fblinear16;
			break;
		case 24:
			driver = &fblinear24;
			break;
		case 32:
			driver = &fblinear32;
			break;
		}
	}

	/* return driver selected*/
	return driver;
}

/* 
 * Initialize memory device with passed parms,
 * select suitable framebuffer subdriver,
 * and set subdriver in memory device.
 */
MWBOOL
fb_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
	int size,void *addr)
{
	PSUBDRIVER subdriver;

	/* initialize mem screen driver*/
	initmemgc(mempsd, w, h, planes, bpp, linelen, size, addr);

/* FIXME kluge for current portrait mode subdriver in scr_fbportrait.c*/
//if(mempsd->portrait != MWPORTRAIT_NONE) return 1;

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(mempsd);
	if(!subdriver)
		return 0;

	/* set and initialize subdriver into mem screen driver*/
	if(!set_subdriver(mempsd, subdriver, TRUE))
		return 0;

	return 1;
}
