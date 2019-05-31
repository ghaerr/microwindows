/*
 * Copyright (c) 2010, 2019 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Framebuffer Emulator screen driver
 * Set SCREEN=FBE in config.
 * Also useful as template when developing new screen drivers
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

/*
 * Define TESTDRIVER=1 to allocate a memory buffer instead of failing if
 * the FBE shared framebuffer isn't found. This is useful when developing
 * new screen drivers or just wanting to compile up a screen driver that
 * isn't dependent on any libraries on a new platform.
 */
#define TESTDRIVER	0

#if !TESTDRIVER
#include <fcntl.h>
#include <sys/mman.h>
#endif

#if !defined(SCREEN_DEPTH) && (MWPIXEL_FORMAT == MWPF_PALETTE)
/* SCREEN_DEPTH is used only for palette modes*/
#error SCREEN_DEPTH not defined - must be set for palette modes
#endif

static int fb = -1;						/* Framebuffer file handle*/

static PSD  fbe_open(PSD psd);
static void fbe_close(PSD psd);
static void fbe_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static int fbe_preselect(PSD psd);
static void fbe_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	fbe_open,
	fbe_close,
	fbe_setpalette,
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
#if TESTDRIVER
	fbe_update,
	fbe_preselect
#endif
};

/* open framebuffer mmap'd by FBE*/
static PSD
fbe_open(PSD psd)
{
	char *env;

	int flags = PSF_SCREEN;		/* init psd, don't allocate framebuffer*/

	if (!gen_initpsd(psd, MWPIXEL_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, flags))
		return NULL;

#if TESTDRIVER
	/* set to copy aggregate screen update region in Update()*/
	psd->flags |= PSF_DELAYUPDATE;

	/* set if screen driver subsystem requires polling and select()*/
	psd->flags |= PSF_CANTBLOCK;

	/*
	 * Allocate framebuffer
	 * psd->size is calculated by subdriver init
	 */
	if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;
	psd->flags |= PSF_ADDRMALLOC;
#else
	/* open framebuffer file for mmap*/
	if((env = getenv("FRAMEBUFFER")) == NULL)
		env = MW_PATH_FBE_FRAMEBUFFER;
	fb = open(env, O_RDWR);
	if (fb >= 0)
	{
		/* mmap framebuffer into this address space*/
		int extra = getpagesize() - 1;
		psd->size = (psd->size + extra) & ~extra;	/* extend to page boundary*/
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
		if (psd->addr == NULL || psd->addr == (unsigned char *)-1)
		{
			EPRINTF("Error mmaping shared framebuffer %s: %m\n", env);
			close(fb);
			return NULL;
		}
	}
	else {
		EPRINTF("Error opening %s\n", env);
		return NULL;
	}
#endif

	return psd;	/* success*/
}

/* close framebuffer*/
static void
fbe_close(PSD psd)
{
#if !TESTDRIVER
	if (fb >= 0)
		close(fb);
	fb = -1;
#endif
	if ((psd->flags & PSF_ADDRMALLOC))
		free (psd->addr);
}

/* setup palette*/
static void
fbe_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

#if TESTDRIVER
/*
 * The following routines are not required for FBE framebuffer, but
 * may be useful when developing a new screen driver.
 * SAMPLE UNWORKING CODE, requires dstpixels and dstpitch initialization below.
 */

/* bounding rectangle for aggregrate screen update*/
static MWCOORD upminX, upminY, upmaxX, upmaxY;

/* update graphics lib from framebuffer*/
static void
fbe_draw(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	unsigned char *dstpixels = NULL; /* set to destination pixels in graphics lib*/
	unsigned int   dstpitch = 0;	 /* set to width in bytes of destination pixel row*/

	/* assumes destination pixels in same format * as MWPIXEL_FORMAT set in config!*/
	if (dstpixels)
		copy_framebuffer(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1,
			dstpixels, dstpitch);
}

/* called before select(), returns # pending events*/
static int
fbe_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX >= 0 || upmaxY >= 0)) {
		fbe_draw(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);

		/* reset update region*/
		upminX = upminY = MAX_MWCOORD;
		upmaxX = upmaxY = MIN_MWCOORD;
	}

	/* return nonzero if subsystem events available and driver uses PSF_CANTBLOCK*/
	return 0;
}

/* called from framebuffer drivers with bounding rect of updated framebuffer region*/
static void
fbe_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = MWMIN(x, upminX);
			upminY = MWMIN(y, upminY);
			upmaxX = MWMAX(upmaxX, x+width-1);
			upmaxY = MWMAX(upmaxY, y+height-1);
	} else
		fbe_draw(psd, x, y, width, height);
}
#endif /* TESTDRIVER*/
