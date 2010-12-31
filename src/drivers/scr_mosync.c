/*
 * Copyright (c) 2010 Greg Haerr <greg@censoft.com>
 * moSync framebuffer adaptions (c) 2010 Ludwig Ertl / CSP GmbH
 *
 * Microwindows Screen Driver for moSync framebuffer
 */
#include <ma.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	fb_open,
	fb_close,
	NULL,				/* SetPalette not supported by moSync framebuffer */
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait
	NULL,				/* Update*/
	NULL				/* PreSelect*/
};

/* static variables*/
static int status;		/* 0=never inited, 1=once inited, 2=inited. */
static MAFrameBufferInfo fbinfo;

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
	PSUBDRIVER subdriver;
	int iRet;

	assert(status < 2);

	/* setup screen device from framebuffer info*/
	maFrameBufferGetInfo(&fbinfo);

	psd->portrait = MWPORTRAIT_NONE;
	psd->xres = psd->xvirtres = fbinfo.width;
	psd->yres = psd->yvirtres = fbinfo.height;
	psd->planes = 1;
	psd->bpp = fbinfo.bitsPerPixel;
	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);
	psd->pitch = fbinfo.pitch;
	psd->size = fbinfo.sizeInBytes;
    psd->flags = PSF_SCREEN | PSF_ADDRMALLOC;

	/* set pixel format*/
	switch(psd->bpp) {
	case 8:
		psd->pixtype = MWPF_TRUECOLOR332;
		break;
	case 16:
		if (fbinfo.greenBits == 5)
			psd->pixtype = MWPF_TRUECOLOR555;
		else
			psd->pixtype = MWPF_TRUECOLOR565;
		break;
	case 24:
		psd->pixtype = MWPF_TRUECOLOR888;
		break;
	case 32:
		psd->pixtype = MWPF_TRUECOLOR8888;
		break;
	default:
		EPRINTF("Unsupported %ld color (%d bpp) truecolor framebuffer\n", psd->ncolors, psd->bpp);
		return NULL;
	}

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver) {
		EPRINTF("No driver for screen bpp %d\n", psd->bpp);
		return NULL;
	}

	/* set subdriver into screen driver */
	set_subdriver(psd, subdriver);

	/* allocate framebuffer (uses lots of memory!) */
	if (!(psd->addr = calloc (1, psd->size))) {
		EPRINTF("Out of memory allocating Framebuffer of %d bytes\n", psd->size);
		return NULL;
	}

	if ((iRet = maFrameBufferInit(psd->addr))<=0) {
		EPRINTF("Error %d initializing framebuffer\n", iRet);
		return NULL;
	}

	status = 2;
	return psd;	/* success*/
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
	/* if not opened, return*/
	if(status != 2)
		return;
	status = 1;

	/* unmap framebuffer*/
	maFrameBufferClose();
	free (psd->addr);
}
