/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2007, 2010, 2019 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 Koninklijke Philips Electronics
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 *
 * Microwindows Screen Driver for Linux kernel framebuffers or framebuffer emulator
 *
 * To use with bin/fbe, setenv FRAMEBUFFER=/tmp/fb0 or use scr_fbe.c driver (SCREEN=FBE in config)
 * 
 * Note: modify select_fb_driver() to add new framebuffer subdrivers
 */
#define _GNU_SOURCE 1
#include <fcntl.h>
#include <limits.h>
#if LINUX
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#ifndef FB_TYPE_VGA_PLANES
#define FB_TYPE_VGA_PLANES 4
#endif

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette);
static PSD open_linuxfb(PSD psd);
static void	set_directcolor_palette(PSD psd);

/* static variables*/
static int fb = -1;				/* framebuffer file handle*/
#if LINUX
static short saved_red[16];		/* original hw palette*/
static short saved_green[16];
static short saved_blue[16];
static struct fb_fix_screeninfo  fb_fix;
static struct fb_var_screeninfo fb_var;
#endif

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

/* open framebuffer driver*/
static PSD
fb_open(PSD psd)
{
	char *	env;
	int		fbe;

	/* special case framebuffer emulator override*/
	env = getenv("FRAMEBUFFER");
	fbe = env && !strcmp(env, MW_PATH_FBE_FRAMEBUFFER);

	/*
	 * Locate and open framebuffer:
	 * If not FBE, try /dev/fb0 or environment override.
	 * Otherwise try FBE /tmp/fb0.
	 */
	if (!fbe)
		fb = open(env? env: MW_PATH_FRAMEBUFFER, O_RDWR);
	if (fb < 0) {	/* fb is < 0 at fb_open() entry or if open() fails above in !fbe case*/
		/* try framebuffer emulator*/
		fb = open(MW_PATH_FBE_FRAMEBUFFER, O_RDWR);
		if (fb >= 0) {
			int flags = PSF_SCREEN;		/* init psd, don't allocate framebuffer*/
			int extra = getpagesize() - 1;

			/* init framebuffer emulator to config-set values*/
			if (!gen_initpsd(psd, MWPIXEL_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, flags))
				goto fail;

			/* mmap framebuffer into this address space*/
			psd->size = (psd->size + extra) & ~extra;	/* extend to page boundary*/
			psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
			if (psd->addr == (unsigned char *)-1) {
				EPRINTF("Error mmaping shared framebuffer %s: %m\n", MW_PATH_FBE_FRAMEBUFFER);
fail:
				close(fb);
				fb = -1;
				return NULL;
			}
			return psd;		/* FBE success*/
		}
	}
	if(fb < 0) {
		EPRINTF("Error opening %s: %m. Check kernel config\n", env? env: MW_PATH_FRAMEBUFFER);
		return NULL;
	}

	/* continue with Linux framebuffer open*/
	return open_linuxfb(psd);
}

/* open linux framebuffer*/
static PSD
open_linuxfb(PSD psd)
{
#if LINUX
	int	type, visual;
	int extra = getpagesize() - 1;
	PSUBDRIVER subdriver;

	/* get dynamic framebuffer info*/
	if (ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix) == -1 ||
		ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) == -1) {
			EPRINTF("Error reading screen info: %m\n");
			goto fail;
	}

	/* setup screen device from framebuffer info*/
	type = fb_fix.type;
	visual = fb_fix.visual;

	psd->portrait = MWPORTRAIT_NONE;
#if LINUX_SPARC
	psd->xres = psd->xvirtres = fb_var.xres_virtual;
	psd->yres = psd->yvirtres = fb_var.yres_virtual;
#else
	psd->xres = psd->xvirtres = fb_var.xres;
	psd->yres = psd->yvirtres = fb_var.yres;
#endif

	/* set planes from fb type*/
	if (type == FB_TYPE_VGA_PLANES)
		psd->planes = 4;
	else if (type == FB_TYPE_PACKED_PIXELS)
		psd->planes = 1;
	else psd->planes = 0;	/* force error later*/

	psd->bpp = fb_var.bits_per_pixel;
	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);
	if (psd->bpp == 15)		/* allow 15bpp for static fb emulator init only*/
		psd->bpp = 16;

	psd->pitch = fb_fix.line_length;
	psd->size = psd->yres * psd->pitch;
    psd->flags = PSF_SCREEN;

	/* set pixel format*/
	if(visual == FB_VISUAL_TRUECOLOR || visual == FB_VISUAL_DIRECTCOLOR) {
		switch(psd->bpp) {
		case 8:
			psd->pixtype = MWPF_TRUECOLOR332;
			break;
		case 16:
			if (fb_var.green.length == 5)
				psd->pixtype = MWPF_TRUECOLOR555;	// FIXME must also set MWPF_PIXELFORMAT in config
			else
				psd->pixtype = MWPF_TRUECOLOR565;
			break;
		case 18:
		case 24:
			psd->pixtype = MWPF_TRUECOLORRGB;
			break;
		case 32:
			psd->pixtype = MWPF_TRUECOLORARGB;
			break;
		default:
			EPRINTF("Unsupported %d color (%d bpp) truecolor framebuffer\n", psd->ncolors, psd->bpp);
			goto fail;
		}
	} else 
		psd->pixtype = MWPF_PALETTE;

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	EPRINTF("%dx%dx%dbpp pitch %d type %d visual %d colors %d pixtype %d\n", psd->xres, psd->yres,
		(psd->pixtype == MWPF_TRUECOLOR555)? 15: psd->bpp, psd->pitch, type, visual,
		psd->ncolors, psd->pixtype);

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver) {
		EPRINTF("No driver for screen type %d visual %d bpp %d\n", type, visual, psd->bpp);
		goto fail;
	}

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

#if HAVE_TEXTMODE
	{
	/* open tty, enter graphics mode*/
	int tty = open ("/dev/tty0", O_RDWR);
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
	}
#endif

	/* mmap framebuffer into this address space*/
	psd->size = (psd->size + extra) & ~extra;		/* extend to page boundary*/

#if LINUX_SPARC
#define CG3_MMAP_OFFSET 0x4000000
#define CG6_RAM    		0x70016000
#define TCX_RAM8BIT		0x00000000
#define TCX_RAM24BIT	0x01000000
	switch (fb_fix.accel) {
	case FB_ACCEL_SUN_CGTHREE:
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,MAP_SHARED,fb,CG3_MMAP_OFFSET);
		break;
	case FB_ACCEL_SUN_CGSIX:
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,MAP_SHARED,fb,CG6_RAM);
		break;
	case FB_ACCEL_SUN_TCX:
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,MAP_SHARED,fb,TCX_RAM24BIT);
 		break;
	default:
		EPRINTF("Don't know how to mmap %s with accel %d\n", MW_PATH_FRAMEBUFFER, fb_fix.accel);
		goto fail;
	}
#elif LINUX_BLACKFIN
	psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_FILE,fb,0);
#elif UCLINUX
	psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,0,fb,0);
#else
	psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);
#endif
	if(psd->addr == NULL || psd->addr == (unsigned char *)-1) {
		EPRINTF("Error mmaping %s: %m\n", MW_PATH_FRAMEBUFFER);
		goto fail;
	}

	/* save original palette*/
	ioctl_getpalette(0, 16, saved_red, saved_green, saved_blue);

	/* setup direct color palette if required (ATI cards)*/
	if(visual == FB_VISUAL_DIRECTCOLOR)
		set_directcolor_palette(psd);

	return psd;	/* success*/

fail:
	close(fb);
	fb = -1;
#endif /* LINUX*/
	return NULL;
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
	/* if not opened, return*/
	if (fb < 0)
		return;
#if LINUX
  	/* reset hw palette*/
	ioctl_setpalette(0, 16, saved_red, saved_green, saved_blue);
  
	/* unmap framebuffer*/
	munmap(psd->addr, psd->size);
  
#if HAVE_TEXTMODE
	{
	/* enter text mode*/
	int tty = open ("/dev/tty0", O_RDWR);
	ioctl(tty, KDSETMODE, KD_TEXT);
	close(tty);
	}
#endif
#endif /* LINUX*/

	/* close framebuffer*/
	close(fb);
	fb = -1;
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

/* convert Microwindows palette to framebuffer format and set it*/
static void
fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette)
{
	int 	i;
	short 	red[256];
	short 	green[256];
	short 	blue[256];
	static const int fade = 100;

	if (count > 256)
		count = 256;

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
#ifdef FBIOGETCMAP
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = (unsigned short *)red;
	cmap.green = (unsigned short *)green;
	cmap.blue = (unsigned short *)blue;
	cmap.transp = NULL;
	ioctl(fb, FBIOGETCMAP, &cmap);
#endif
}

/* set framebuffer palette*/
void
ioctl_setpalette(int start, int len, short *red, short *green, short *blue)
{
#ifdef FBIOPUTCMAP
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = (unsigned short *)red;
	cmap.green = (unsigned short *)green;
	cmap.blue = (unsigned short *)blue;
	cmap.transp = NULL;

	ioctl(fb, FBIOPUTCMAP, &cmap);
#endif
}
