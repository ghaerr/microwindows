/*
 * Microwindows Screen Driver for Fiwix kernel framebuffer
 *
 * Copyright (c) 2024 Greg Haerr <greg@censoft.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fiwix/kd.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

#define HAVE_TEXTMODE	0

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette);

static int fb = -1;				/* framebuffer file handle*/

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	fb_open,
	fb_close,
	fb_setpalette,
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	NULL,				/* Update*/
	NULL				/* PreSelect*/
};

#define IO_FB_XRES	2
#define IO_FB_YRES	3

/* open framebuffer driver*/
static PSD
fb_open(PSD psd)
{
	char *path;
	PSUBDRIVER subdriver;

	path = getenv("FRAMEBUFFER");
	if (!path)
		path = MW_PATH_FRAMEBUFFER;

	fb = open(path, O_RDWR);
	if (fb < 0) {
		EPRINTF("Error %d opening %s. Check kernel config\n", errno, path);
		return NULL;
	}

	/* setup screen device */
	psd->portrait = MWPORTRAIT_NONE;
	psd->xres = psd->xvirtres = ioctl(fb, IO_FB_XRES, 0);
	psd->yres = psd->yvirtres = ioctl(fb, IO_FB_YRES, 0);
	psd->bpp = 32;
	psd->pitch = psd->xres * (psd->bpp >> 3);
	psd->size = psd->yres * psd->pitch;
	psd->planes = 1;
	psd->ncolors = 1 << 24;
	psd->flags = PSF_SCREEN;
	psd->pixtype = MWPF_TRUECOLORARGB;

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	EPRINTF("%dx%dx%dbpp pitch %d colors %d pixtype %d\n", psd->xres, psd->yres,
		psd->bpp, psd->pitch, psd->ncolors, psd->pixtype);

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver) {
		EPRINTF("No driver for screen bpp %d\n", psd->bpp);
		goto fail;
	}

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

	/* mmap framebuffer into this address space*/
	psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
	if (psd->addr == NULL || (unsigned int)psd->addr >= -4095) {
		EPRINTF("Error mapping %s: %d\n", MW_PATH_FRAMEBUFFER, psd->addr);
		goto fail;
	}

#if HAVE_TEXTMODE
	/* open tty, enter graphics mode*/
	int tty = open ("/dev/tty0", O_RDWR);
	if (tty < 0) {
		EPRINTF("Error can't open /dev/tty0: %d\n", errno);
		goto fail;
	}
	/* FIXME: KDSETMODE doesn't remove cursor! */
	if (ioctl (tty, KDSETMODE, KD_GRAPHICS) == -1) {
		EPRINTF("Error setting graphics mode: %d\n", errno);
		close(tty);
		goto fail;
	}
	close(tty);
#endif

	return psd;	/* success*/

fail:
	close(fb);
	fb = -1;
	return NULL;
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
	if (fb < 0)
		return;
  
#if HAVE_TEXTMODE
	/* enter text mode*/
	int tty = open ("/dev/tty0", O_RDWR);
	ioctl(tty, KDSETMODE, KD_TEXT);
	close(tty);
#endif

	/* unmap framebuffer */
	munmap(psd->addr, psd->size);
	close(fb);
	fb = -1;
}

static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette)
{
}
