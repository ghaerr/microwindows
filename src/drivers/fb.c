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
	PSUBDRIVER  driver = NULL;
	extern SUBDRIVER fblinear1;
	extern SUBDRIVER fblinear2;
	extern SUBDRIVER fblinear4;
	extern SUBDRIVER fblinear8;
	extern SUBDRIVER fblinear16;
	extern SUBDRIVER fblinear18;
	extern SUBDRIVER fblinear24;
	extern SUBDRIVER fblinear32;
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
			break;
		case 18: // addon VB May 2007 : 18bpp fb linear driver
			driver = &fblinear18;
			break ;
		case 24:
			driver = &fblinear24;
			break;
		case 32:
			if (psd->pixtype == MWPF_TRUECOLOR8888 || psd->pixtype == MWPF_TRUECOLORABGR) {
				driver = &fblinear32alpha;
			} else {
				driver = &fblinear32;
			}
			break;
		}
	}

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
	extern SUBDRIVER fbportrait_left;
	extern SUBDRIVER fbportrait_right;
	extern SUBDRIVER fbportrait_down;

	switch (psd->portrait) {
	case MWPORTRAIT_NONE:
		subdriver = psd->orgsubdriver;
		break;
	case MWPORTRAIT_LEFT:
		subdriver = &fbportrait_left;
		break;
	case MWPORTRAIT_RIGHT:
		subdriver = &fbportrait_right;
		break;
	case MWPORTRAIT_DOWN:
		subdriver = &fbportrait_down;
		break;
	}
	set_subdriver(psd, subdriver, FALSE);
}
