/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
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
	PSUBDRIVER driver = NULL;
	PSUBDRIVER left = &fbportrait_left;
	PSUBDRIVER right = &fbportrait_right;
	PSUBDRIVER down = &fbportrait_down;
	extern SUBDRIVER fblinear1;
	extern SUBDRIVER fblinear2;
	extern SUBDRIVER fblinear4;
	extern SUBDRIVER fblinear8;
	extern SUBDRIVER fblinear16;
	extern SUBDRIVER fblinear16_left;
	extern SUBDRIVER fblinear16_right;
	extern SUBDRIVER fblinear16_down;
	extern SUBDRIVER fblinear24;
	extern SUBDRIVER fblinear24_left;
	extern SUBDRIVER fblinear24_right;
	extern SUBDRIVER fblinear24_down;
	extern SUBDRIVER fblinear32;
	extern SUBDRIVER fblinear32_left;
	extern SUBDRIVER fblinear32_right;
	extern SUBDRIVER fblinear32_down;
	extern SUBDRIVER fblinear32alpha;
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
			left = &fblinear16_left;
			right = &fblinear16_right;
			down = &fblinear16_down;
			break;
		case 18: // addon VB May 2007 : 18bpp fb linear driver FIXME
		case 24:
			driver = &fblinear24;
			left = &fblinear24_left;
			right = &fblinear24_right;
			down = &fblinear24_down;
printf("selecting 24bpp subdriver\n");
			break;
		case 32:
			if (psd->pixtype == MWPF_TRUECOLOR8888 || psd->pixtype == MWPF_TRUECOLORABGR) {
				driver = &fblinear32alpha;
			} else {
				driver = &fblinear32;
				left = &fblinear32_left;
				right = &fblinear32_right;
				down = &fblinear32_down;
printf("selecting 32bpp subdriver\n");
			}
			break;
		}
	}

	//FIXME KLUGE!!
	psd->left_subdriver = left;
	psd->right_subdriver = right;
	psd->down_subdriver = down;

	/* return driver selected*/
	return driver;
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
