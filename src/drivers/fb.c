/*
 * Copyright (c) 2000, 2010 Greg Haerr <greg@censoft.com>
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
	PSUBDRIVER *pdriver = NULL;
	extern PSUBDRIVER fblinear1[4];
	extern PSUBDRIVER fblinear2[4];
	extern PSUBDRIVER fblinear4[4];
	extern PSUBDRIVER fblinear8[4];
	extern PSUBDRIVER fblinear16[4];
	extern PSUBDRIVER fblinear24[4];
	extern PSUBDRIVER fblinear32[4];
	extern PSUBDRIVER fblinear32alpha[4];

	/* FB_TYPE_PACKED_PIXELS*/
	/* device and memory drivers are the same for packed pixels*/
	if(psd->planes == 1) {
		switch(psd->bpp) {
		case 1:
			pdriver = fblinear1;
			break;
		case 2:
			pdriver = fblinear2;
			break;
		case 4:
			pdriver = fblinear4;
			break;
		case 8:
			pdriver = fblinear8;
			break;
		case 16:
			pdriver = fblinear16;
			break;
		case 18: // addon VB May 2007 : 18bpp fb linear driver FIXME
		case 24:
			pdriver = fblinear24;
printf("selecting 24bpp subdriver\n");
			break;
		case 32:
			if (psd->pixtype == MWPF_TRUECOLOR8888 || psd->pixtype == MWPF_TRUECOLORABGR) {
				pdriver = fblinear32alpha;
			} else {
				pdriver = fblinear32;
printf("selecting 32bpp subdriver\n");
			}
			break;
		}
	}

	if (!pdriver)
		return NULL;

	/* remember normal and portrait subdrivers*/
	psd->orgsubdriver = pdriver[0];
	psd->left_subdriver = pdriver[1];
	psd->right_subdriver = pdriver[2];
	psd->down_subdriver = pdriver[3];

	/* return driver selected*/
	return pdriver[0];
}
