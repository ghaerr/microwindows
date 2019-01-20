/*
 * Copyright (c) 2010, 2019 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Framebuffer Emulator screen driver
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

/* path to framebuffer emulator's framebuffer memory file for mmap()*/
#define PATH_FRAMEBUFFER "/tmp/fb0"

#if !defined(SCREEN_DEPTH) && (MWPIXEL_FORMAT == MWPF_PALETTE)
/* SCREEN_DEPTH is used only for palette modes*/
#error SCREEN_DEPTH not defined - must be set for palette modes
#endif

static int fb = -1;						/* Framebuffer file handle*/

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);

void fb_graphicsflush(PSD psd);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	fb_open,
	fb_close,
	NULL,				/* SetPalette*/
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	NULL,				/* Update*/
	NULL				/* PreSelect*/
};

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
	PSUBDRIVER subdriver;
	char *env;

	psd->pixtype = MWPIXEL_FORMAT;				/* SCREEN_PIXTYPE in config*/
	psd->xres = psd->xvirtres = SCREEN_WIDTH;	/* SCREEN_WIDTH in config*/
	psd->yres = psd->yvirtres = SCREEN_HEIGHT;	/* SCREEN_HEIGHT in config*/

	/* use pixel format to set bpp*/
	psd->pixtype = MWPIXEL_FORMAT;
	switch (psd->pixtype) {
	case MWPF_TRUECOLORARGB:
	case MWPF_TRUECOLORABGR:
	default:
		psd->bpp = 32;
		break;

	case MWPF_TRUECOLORRGB:
		psd->bpp = 24;
		break;

	case MWPF_TRUECOLOR565:
	case MWPF_TRUECOLOR555:
		psd->bpp = 16;
		break;

	case MWPF_TRUECOLOR332:
		psd->bpp = 8;
		break;

#if MWPIXEL_FORMAT == MWPF_PALETTE
	case MWPF_PALETTE:
		psd->bpp = SCREEN_DEPTH;				/* SCREEN_DEPTH in config*/
		break;
#endif
	}
	psd->planes = 1;

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* Calculate the correct size and pitch from xres, yres and bpp*/
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp, &psd->size, &psd->pitch);

	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);
	psd->flags = PSF_SCREEN;
	psd->portrait = MWPORTRAIT_NONE;

	/* select an fb subdriver matching our planes and bpp for backing store*/
	subdriver = select_fb_subdriver(psd);
	psd->orgsubdriver = subdriver;
	if (!subdriver)
		return NULL;

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

	/* open framebuffer file for mmap*/
	if((env = getenv("FRAMEBUFFER")) == NULL)
		env = PATH_FRAMEBUFFER;
	fb = open(env, O_RDWR);
	if (fb >= 0) {
		/* mmap framebuffer into this address space*/
		psd->size = (psd->size + getpagesize() - 1) / getpagesize() * getpagesize();
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
		if (psd->addr == NULL || psd->addr == (unsigned char *)-1) {
			EPRINTF("Error mmaping shared framebuffer %s: %m\n", env);
			close(fb);
			return NULL;
		}
	}
	else {
		printf("Error opening %s\n", env);
		return NULL;
	}
#if 0
	else {
		/* allocate framebuffer*/
		if ((psd->addr = malloc(psd->size)) == NULL)
			return NULL;
		psd->flags |= PSF_ADDRMALLOC;
	}
#endif

	return psd;	/* success*/
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
	if (fb >= 0)
		close(fb);
	fb = -1;

	if ((psd->flags & PSF_ADDRMALLOC) && psd->addr)
		free (psd->addr);
	psd->addr = NULL;
}
