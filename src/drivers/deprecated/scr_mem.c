/*
 * Copyright (c) 2010 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Memory Screen Driver
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

/* define to allow mmap'd framebuffer for use with fb emulator*/
//#define PATH_FRAMEBUFFER "/tmp/fb0"	/* mmap'd framebuffer file if present*/

static int fb = -1;						/* Framebuffer file handle*/
MWCLIPREGION *fb_updateregion = NULL;	/* global update region for application handling*/

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);

void fb_graphicsflush(PSD psd);

SCREENDEVICE scrdev = {
	0, SCREEN_WIDTH, SCREEN_HEIGHT, 0,0,0,0, NULL, 0, NULL, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0,0,0,
	gen_fonts,
	fb_open,
	fb_close,
	NULL,				/* SetPalette*/
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	fb_update,
	NULL				/* PreSelect*/
};

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
	PSUBDRIVER subdriver;
	char *env;

	/* set statically in struct definition, may be overridden before calling fb_open*/
	//psd->xres = psd->xvirtres = SCREEN_WIDTH;
	//psd->yres = psd->yvirtres = SCREEN_HEIGHT;

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
		psd->bpp = SCREEN_DEPTH;
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

#ifdef PATH_FRAMEBUFFER
	/* try opening framebuffer file for mmap*/
	if((env = getenv("FRAMEBUFFER")) == NULL)
		env = PATH_FRAMEBUFFER;
	fb = open(env, O_RDWR);
#endif
	if (fb >= 0) {
		/* mmap framebuffer into this address space*/
		psd->size = (psd->size + getpagesize() - 1) / getpagesize() * getpagesize();
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
		if (psd->addr == NULL || psd->addr == (unsigned char *)-1) {
			EPRINTF("Error mmaping shared framebuffer %s: %m\n", env);
			close(fb);
			return NULL;
		}
	} else {
		/* allocate framebuffer*/
		if ((psd->addr = malloc(psd->size)) == NULL)
			return NULL;
		psd->flags |= PSF_ADDRMALLOC;
	}

	/* allocate update region*/
	fb_updateregion = GdAllocRegion();

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

 	if (fb_updateregion)
  		GdDestroyRegion(fb_updateregion);
	fb_updateregion = NULL;
}

/* update framebuffer*/
static void
fb_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	MWRECT rc;

	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

	/* add update rect to update region*/
	rc.left = x;
	rc.right = x + width;
	rc.top = y;
	rc.bottom = y + height;
	GdUnionRectWithRegion(&rc, fb_updateregion);
}

void
fb_graphicsflush(PSD psd)
{
	MWRECT *prc = fb_updateregion->rects;
	int count = fb_updateregion->numRects;
	int x, y, w, h;
	int n = 1;

	/* use single bounding box extent*/
	x = fb_updateregion->extents.left;
	y = fb_updateregion->extents.top;
	w = fb_updateregion->extents.right - x;
	h = fb_updateregion->extents.bottom - y;
	DPRINTF("Extents: %d (%d,%d %d,%d)\n", count, x, y, w, h);

	/* or loop through individual list*/
	while (--count >= 0) {
		int rx1, ry1, rx2, ry2;

		rx1 = prc->left;
		ry1 = prc->top;
		rx2 = prc->right;
		ry2 = prc->bottom;
		DPRINTF(" %d: %d,%d %d,%d\n", n++, rx1, ry1, rx2-rx1, ry2-ry1);
		++prc;
	}

	/* empty update region*/
	GdSetRectRegion(fb_updateregion, 0, 0, 0, 0);
}
