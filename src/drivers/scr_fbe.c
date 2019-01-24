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

/* path to framebuffer emulator's framebuffer memory file for mmap()*/
#define PATH_FRAMEBUFFER "/tmp/fb0"

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

/* init framebuffer*/
static PSD
fbe_open(PSD psd)
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

#if TESTDRIVER

	/* allocate framebuffer*/
	if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;
	psd->flags |= PSF_ADDRMALLOC;
	psd->flags |= PSF_DELAYUPDATE;		/* set to copy aggregate screen update region in Update()*/
	//psd->flags |= PSF_CANTBLOCK;		/* set if screen driver subsystem requires polling and select()*/

#else

	/* open framebuffer file for mmap*/
	if((env = getenv("FRAMEBUFFER")) == NULL)
		env = PATH_FRAMEBUFFER;
	fb = open(env, O_RDWR);
	if (fb >= 0)
	{
		/* mmap framebuffer into this address space*/
		psd->size = (psd->size + getpagesize() - 1) / getpagesize() * getpagesize();
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
		if (psd->addr == NULL || psd->addr == (unsigned char *)-1)
		{
			EPRINTF("Error mmaping shared framebuffer %s: %m\n", env);
			close(fb);
			return NULL;
		}
	}
	else {
		printf("Error opening %s\n", env);
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

#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define max(a,b)            (((a) > (b)) ? (a) : (b))

static MWCOORD upminX, upminY, upmaxX, upmaxY;		/* bounding rectangle for aggregrate screen update*/
static unsigned char *dstpixels = NULL;		/* NEW DRIVER ONLY - set to destination pixels and pitch*/
static unsigned int   dstpitch;				/* width in bytes of destination pixel row*/

/* drivers/copyframebuffer.c - assumes destination pixels in same format as MWPIXEL_FORMAT set in config!*/

extern void copy_framebuffer(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	unsigned char *dstpixels, unsigned int dstpitch);

/* called before select(), returns # pending events*/
static int
fbe_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX || upmaxY)) {
		if (dstpixels)
			copy_framebuffer(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1, dstpixels, dstpitch);

		/* reset update region*/
		upminX = upminY = ~(1 << ((sizeof(int)*8)-1));	// largest positive int
		upmaxX = upmaxY = 0;
	}

	/* return nonzero if subsystem events available and driver uses PSF_CANTBLOCK*/
	return 0;
}

static void
fbe_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = min(x, upminX);
			upminY = min(y, upminY);
			upmaxX = max(upmaxX, x+width-1);
			upmaxY = max(upmaxY, y+height-1);
	} else {
		if (dstpixels)
			copy_framebuffer(psd, x, y, width, height, dstpixels, dstpitch);
	}
}
#endif /* TESTDRIVER*/
