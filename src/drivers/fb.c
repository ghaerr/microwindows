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
#include "genfont.h"
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
	extern PSUBDRIVER fblinear32bgra[4];
	extern PSUBDRIVER fblinear32rgba[4];

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
		case 24:
			pdriver = fblinear24;
			break;
		case 32:
			if (psd->data_format == MWIF_RGBA8888)	/* RGBA pixmaps*/
				pdriver = fblinear32rgba;
			else
				pdriver = fblinear32bgra;
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

/* set standard data_format from bpp and pixtype*/
int
set_data_format(PSD psd)
{
	int data_format = 0;

	switch(psd->pixtype) {
	case MWPF_TRUECOLOR8888:
		data_format = MWIF_BGRA8888;
		break;
	case MWPF_TRUECOLORABGR:
		data_format = MWIF_RGBA8888;
		break;
	case MWPF_TRUECOLOR888:
		data_format = MWIF_BGR888;
		break;
	case MWPF_TRUECOLOR565:
		data_format = MWIF_RGB565;
		break;
	case MWPF_TRUECOLOR555:
		data_format = MWIF_RGB555;
		break;
	case MWPF_TRUECOLOR332:
		data_format = MWIF_RGB332;
		break;
	case MWPF_TRUECOLOR233:
		data_format = MWIF_BGR233;
		break;
	case MWPF_PALETTE:
		switch (psd->bpp) {
		case 8:
			data_format = MWIF_PAL8;
			break;
		case 4:
			data_format = MWIF_PAL4;
			break;
		case 2:
			data_format = MWIF_PAL2;
			break;
		case 1:
			data_format = MWIF_PAL1;
			break;
		}
		break;
	}

	return data_format;
}

/* general routine to return screen info, ok for all fb devices*/
void
gen_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->data_format = psd->data_format;
	psi->ncolors = psd->ncolors;
	psi->fonts = NUMBER_FONTS;
	psi->portrait = psd->portrait;
	psi->fbdriver = TRUE;	/* running fb driver, can direct map*/
	psi->pixtype = psd->pixtype;

	switch (psd->pixtype) {
	case MWPF_TRUECOLOR8888:
		psi->amask 	= 0xff000000;
		psi->rmask 	= 0x00ff0000;
		psi->gmask 	= 0x0000ff00;
		psi->bmask	= 0x000000ff;
		break;
	case MWPF_TRUECOLORABGR:
		psi->amask 	= 0xff000000;
		psi->rmask	= 0x000000ff;
		psi->gmask 	= 0x0000ff00;
		psi->bmask 	= 0x00ff0000;
	case MWPF_TRUECOLOR888:
		psi->amask 	= 0x00000000;
		psi->rmask 	= 0x00ff0000;
		psi->gmask 	= 0x0000ff00;
		psi->bmask	= 0x000000ff;
		break;
	case MWPF_TRUECOLOR565:
		psi->amask  = 0x0000;
		psi->rmask 	= 0xf800;
		psi->gmask 	= 0x07e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR555:
		psi->amask  = 0x8000;
		psi->rmask 	= 0x7c00;
		psi->gmask 	= 0x03e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR332:
		psi->amask 	= 0x00;
		psi->rmask 	= 0xe0;
		psi->gmask 	= 0x1c;
		psi->bmask	= 0x03;
		break;
	case MWPF_PALETTE:
	default:
		psi->amask 	= 0x00;
		psi->rmask 	= 0xff;
		psi->gmask 	= 0xff;
		psi->bmask	= 0xff;
		break;
	}

	//eCos
    //psi->ydpcm = 42; 		/* 320 / (3 * 2.54)*/
    //psi->xdpcm = 38; 		/* 240 / (2.5 * 2.54)*/
	//psp
    //psi->ydpcm = 120;
    //psi->xdpcm = 120;
	if(psd->yvirtres > 480) {	//FIXME update
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(psd->yvirtres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
        } else if(psd->yvirtres <= 240) {
		/* half VGA 640x240 */
		psi->xdpcm = 14;        /* assumes screen width of 24 cm*/ 
		psi->ydpcm =  5;
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}
