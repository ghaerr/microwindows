/*
 * Copyright (c) 1999, 2000, 2001 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver for Linux kernel framebuffers
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 * 
 * Note: modify select_fb_driver() to add new framebuffer subdrivers
 */
#define _GNU_SOURCE 1
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef ARCH_LINUX_POWERPPC
#ifdef __GLIBC__
#include <sys/io.h>
#else
#include <asm/io.h>
#endif
#endif
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

/* for Osprey and Embedded Planet boards, set HAVETEXTMODE=0*/
#define HAVETEXTMODE	1	/* =0 for graphics only systems*/
#define EMBEDDEDPLANET	0	/* =1 for kluge embeddedplanet ppc framebuffer*/

#ifndef FB_TYPE_VGA_PLANES
#define FB_TYPE_VGA_PLANES 4
#endif

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_setportrait(PSD psd, int portraitmode);
static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette);
static void gen_getscreeninfo(PSD psd,PMWSCREENINFO psi);


SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	fb_open,
	fb_close,
	gen_getscreeninfo,
	fb_setpalette,
	NULL,			/* DrawPixel subdriver*/
	NULL,			/* ReadPixel subdriver*/
	NULL,			/* DrawHorzLine subdriver*/
	NULL,			/* DrawVertLine subdriver*/
	NULL,			/* FillRect subdriver*/
	gen_fonts,
	NULL,			/* Blit subdriver*/
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	fb_mapmemgc,
	gen_freememgc,
	NULL,			/* StretchBlit subdriver*/
	fb_setportrait		/* SetPortrait*/
};

/* static variables*/
static int fb;			/* Framebuffer file handle. */
static int status;		/* 0=never inited, 1=once inited, 2=inited. */
static short saved_red[16];	/* original hw palette*/
static short saved_green[16];
static short saved_blue[16];

extern SUBDRIVER fbportrait_left, fbportrait_right, fbportrait_down;
static PSUBDRIVER pdrivers[4] = { /* portrait mode subdrivers*/
	NULL, &fbportrait_left, &fbportrait_right, &fbportrait_down
};

/* local functions*/
static void	set_directcolor_palette(PSD psd);

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
	char *	env;
	int	type, visual;
	PSUBDRIVER subdriver;
#if EMBEDDEDPLANET
	env = "/dev/lcd";
	fb = open(env, O_RDWR);
	if(fb < 0) {
		EPRINTF("Error opening %s: %m", env);
		return NULL;
	}
	/* framebuffer attributes are fixed*/
	type = FB_TYPE_PACKED_PIXELS;
	visual = FB_VISUAL_PSEUDOCOLOR;
	psd->xres = psd->xvirtres = 640;
	psd->yres = psd->yvirtres = 480;
	psd->planes = 1;
	psd->bpp = 8;
	/* set linelen to byte length, possibly converted later*/
	psd->linelen = psd->xvirtres;
	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);
	psd->size = 0;		/* force subdriver init of size*/

        if (ioctl(fb, 1, 0) < 0) {
		EPRINTF("Error: can't enable LCD");
		goto fail;
	}
#else
	int	tty;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;

	assert(status < 2);

	/* locate and open framebuffer, get info*/
	if(!(env = getenv("FRAMEBUFFER")))
		env = "/dev/fb0";
	fb = open(env, O_RDWR);
	if(fb < 0) {
		EPRINTF("Error opening %s: %m. Check kernel config\n", env);
		return NULL;
	}
	if(ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix) == -1 ||
		ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) == -1) {
			EPRINTF("Error reading screen info: %m\n");
			goto fail;
	}
	/* setup screen device from framebuffer info*/
	type = fb_fix.type;
	visual = fb_fix.visual;

	psd->portrait = MWPORTRAIT_NONE;
	psd->xres = psd->xvirtres = fb_var.xres;
	psd->yres = psd->yvirtres = fb_var.yres;

	/* set planes from fb type*/
	if (type == FB_TYPE_VGA_PLANES)
		psd->planes = 4;
	else if (type == FB_TYPE_PACKED_PIXELS)
		psd->planes = 1;
	else psd->planes = 0;	/* force error later*/

	psd->bpp = fb_var.bits_per_pixel;
	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);

	/* set linelen to byte length, possibly converted later*/
	psd->linelen = fb_fix.line_length;
	psd->size = 0;		/* force subdriver init of size*/
#endif /* !EMBEDDEDPLANET*/

	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
	if (psd->bpp == 16)
		psd->flags |= PSF_HAVEOP_COPY;

	/* set pixel format*/
#ifndef TPHELIO /* temp kluge: VTech Helio kernel needs changing*/
	if(visual == FB_VISUAL_TRUECOLOR || visual == FB_VISUAL_DIRECTCOLOR) {
		switch(psd->bpp) {
		case 8:
			psd->pixtype = MWPF_TRUECOLOR332;
			break;
		case 16:
			if (fb_var.green.length == 5)
				psd->pixtype = MWPF_TRUECOLOR555;
			else
				psd->pixtype = MWPF_TRUECOLOR565;
			break;
		case 24:
			psd->pixtype = MWPF_TRUECOLOR888;
			break;
		case 32:
			psd->pixtype = MWPF_TRUECOLOR0888;
			break;
		default:
			EPRINTF(
			"Unsupported %d color (%d bpp) truecolor framebuffer\n",
				psd->ncolors, psd->bpp);
			goto fail;
		}
	} else 
#endif
		psd->pixtype = MWPF_PALETTE;

	/*DPRINTF("%dx%dx%d linelen %d type %d visual %d bpp %d\n", psd->xres,
	 	psd->yres, psd->ncolors, psd->linelen, type, visual,
		psd->bpp);*/

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver) {
		EPRINTF("No driver for screen type %d visual %d bpp %d\n",
			type, visual, psd->bpp);
		goto fail;
	}

	/*
	 * set and initialize subdriver into screen driver
	 * psd->size is calculated by subdriver init
	 */
	if(!set_subdriver(psd, subdriver, TRUE)) {
		EPRINTF("Driver initialize failed type %d visual %d bpp %d\n",
			type, visual, psd->bpp);
		goto fail;
	}

	/* remember original subdriver for portrait mode switching*/
	pdrivers[0] = psd->orgsubdriver = subdriver;

#if HAVETEXTMODE
	/* open tty, enter graphics mode*/
	tty = open ("/dev/tty0", O_RDWR);
	if(tty < 0) {
		EPRINTF("Error can't open /dev/tty0: %m\n");
		goto fail;
	}
	if(ioctl (tty, KDSETMODE, KD_GRAPHICS) == -1) {
		EPRINTF("Error setting graphics mode: %m\n");
		close(tty);
		goto fail;
	}
	close(tty);
#endif
	/* mmap framebuffer into this address space*/
	psd->size = (psd->size + getpagesize () - 1)
			/ getpagesize () * getpagesize ();
	psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);
	if(psd->addr == NULL || psd->addr == (unsigned char *)-1) {
		EPRINTF("Error mmaping %s: %m\n", env);
		goto fail;
	}

	/* save original palette*/
	ioctl_getpalette(0, 16, saved_red, saved_green, saved_blue);

	/* setup direct color palette if required (ATI cards)*/
	if(visual == FB_VISUAL_DIRECTCOLOR)
		set_directcolor_palette(psd);

	status = 2;
	return psd;	/* success*/

fail:
	close(fb);
	return NULL;
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
	int	tty;

	/* if not opened, return*/
	if(status != 2)
		return;
	status = 1;

  	/* reset hw palette*/
	ioctl_setpalette(0, 16, saved_red, saved_green, saved_blue);
  
	/* unmap framebuffer*/
	munmap(psd->addr, psd->size);
  
#if HAVETEXTMODE
	/* enter text mode*/
	tty = open ("/dev/tty0", O_RDWR);
	ioctl(tty, KDSETMODE, KD_TEXT);
	close(tty);
#endif
	/* close framebuffer*/
	close(fb);
}

static void
fb_setportrait(PSD psd, int portraitmode)
{
	psd->portrait = portraitmode;

	/* swap x and y in left or right portrait modes*/
	if (portraitmode & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT)) {
		/* swap x, y*/
		psd->xvirtres = psd->yres;
		psd->yvirtres = psd->xres;
	} else {
		/* normal x, y*/
		psd->xvirtres = psd->xres;
		psd->yvirtres = psd->yres;
	}
	/* assign portrait subdriver which calls original subdriver*/
	if (portraitmode == MWPORTRAIT_DOWN)
		portraitmode = 3;	/* change bitpos to index*/
	set_subdriver(psd, pdrivers[portraitmode], FALSE);
}

/* setup directcolor palette - required for ATI cards*/
static void
set_directcolor_palette(PSD psd)
{
	int i;
	short r[256];

	/* 16bpp uses 32 palette entries*/
	if(psd->bpp == 16) {
		for(i=0; i<32; ++i)
			r[i] = i<<11;
		ioctl_setpalette(0, 32, r, r, r);
	} else {
		/* 32bpp uses 256 entries*/
		for(i=0; i<256; ++i)
			r[i] = i<<8;
		ioctl_setpalette(0, 256, r, r, r);
	}
}

static int fade = 100;

/* convert Microwindows palette to framebuffer format and set it*/
static void
fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette)
{
	int 	i;
	unsigned short 	red[count];
	unsigned short 	green[count];
	unsigned short 	blue[count];

	/* convert palette to framebuffer format*/
	for(i=0; i < count; i++) {
		MWPALENTRY *p = &palette[i];

		/* grayscale computation:
		 * red[i] = green[i] = blue[i] =
		 *	(p->r * 77 + p->g * 151 + p->b * 28);
		 */
		red[i] = (p->r * fade / 100) << 8;
		green[i] = (p->g * fade / 100) << 8;
		blue[i] = (p->b * fade / 100) << 8;
	}
	ioctl_setpalette(first, count, red, green, blue);
}

/* get framebuffer palette*/
void
ioctl_getpalette(int start, int len, short *red, short *green, short *blue)
{
#if EMBEDDEDPLANET
	int 		i;
	unsigned short 	colors[256];

	ioctl(fb, 4, colors);
	for (i = start; ((i - start) < len) && (i < 256); i++) {
		red[i - start] = (colors[i] & 0xf00) << 4;
		green[i - start] = (colors[i] & 0x0f0) << 8;
		blue[i - start] = (colors[i] & 0x00f) << 12;
	}
#else
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl(fb, FBIOGETCMAP, &cmap);
#endif
}

/* set framebuffer palette*/
void
ioctl_setpalette(int start, int len, short *red, short *green, short *blue)
{
#if EMBEDDEDPLANET
	int 		i;
	unsigned short 	colors[256];

	ioctl(fb, 4, colors);
	for (i = start; ((i - start) < len) && (i < 256); i++) {
		colors[i] = ((red[i - start] & 0xf000) >> 4)
			| ((green[i - start] & 0xf000) >> 8)
			| ((blue[i - start] & 0xf000) >> 12);
	}
	ioctl(fb, 3, colors);
#else
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl(fb, FBIOPUTCMAP, &cmap);
#endif
}

/* experimental palette animation*/
void
setfadelevel(PSD psd, int f)
{
	int 		i;
	unsigned short 	r[256], g[256], b[256];
	extern MWPALENTRY gr_palette[256];

	if(psd->pixtype != MWPF_PALETTE)
		return;

	fade = f;
	if(fade > 100)
		fade = 100;
	for(i=0; i<256; ++i) {
		r[i] = (gr_palette[i].r * fade / 100) << 8;
		g[i] = (gr_palette[i].g * fade / 100) << 8;
		b[i] = (gr_palette[i].b * fade / 100) << 8;
	}
	ioctl_setpalette(0, 256, r, g, b);
}

static void
gen_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->fonts = NUMBER_FONTS;
	psi->portrait = psd->portrait;
	psi->fbdriver = TRUE;	/* running fb driver, can direct map*/

	psi->pixtype = psd->pixtype;
	switch (psd->pixtype) {
	case MWPF_TRUECOLOR0888:
	case MWPF_TRUECOLOR888:
		psi->rmask 	= 0xff0000;
		psi->gmask 	= 0x00ff00;
		psi->bmask	= 0x0000ff;
		break;
	case MWPF_TRUECOLOR565:
		psi->rmask 	= 0xf800;
		psi->gmask 	= 0x07e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR555:
		psi->rmask 	= 0x7c00;
		psi->gmask 	= 0x03e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR332:
		psi->rmask 	= 0xe0;
		psi->gmask 	= 0x1c;
		psi->bmask	= 0x03;
		break;
	case MWPF_PALETTE:
	default:
		psi->rmask 	= 0xff;
		psi->gmask 	= 0xff;
		psi->bmask	= 0xff;
		break;
	}

	if(psd->yvirtres > 480) {
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
