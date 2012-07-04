/*
 * Copyright (c) 1999, 2004, 2010 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver for RTEMS (uses Microframebuffer api)
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
#define _GNU_SOURCE 1
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"
#include <rtems/fb.h>

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette);

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

/* static variables*/
static int fb;			/* Framebuffer file handle. */
static int status;		/* 0=never inited, 1=once inited, 2=inited. */
static short saved_red[16];	/* original hw palette*/
static short saved_green[16];
static short saved_blue[16];

/* local functions*/
static void	set_directcolor_palette(PSD psd);
//void ioctl_getpalette(int start, int len, short *red, short *green, short *blue);
//void ioctl_setpalette(int start, int len, short *red, short *green, short *blue);

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
	char *	env;
	int	type, visual;
	int	tty;
	PSUBDRIVER subdriver;
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;

	assert(status < 2);

	/* locate and open framebuffer, get info*/
	if(!(env = getenv("FRAMEBUFFER")))
		env = "/dev/fb0";
	fb = open( env, O_RDWR);
	if(fb < 0) {
		EPRINTF("Error opening %s: %m\n", env);
		return NULL;
	}

	if (ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix) || ioctl(fb, FBIOGET_VSCREENINFO, &fb_var )) {
		EPRINTF("Error getting screen info\n" );
		return NULL;
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
			psd->pixtype = MWPF_TRUECOLOR565;
			break;
		case 24:
			psd->pixtype = MWPF_TRUECOLOR888;
			break;
		case 32:
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
			psd->pixtype = MWPF_TRUECOLOR8888;
#else
			psd->pixtype = MWPF_TRUECOLORABGR;
#endif
			break;
		default:
			EPRINTF("Unsupported %d color (%d bpp) truecolor framebuffer\n", psd->ncolors, psd->bpp);
			goto fail;
		}
	} else psd->pixtype = MWPF_PALETTE;

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver) {
		EPRINTF("No driver for screen type %d visual %d bpp %d\n", type, visual, psd->bpp);
		goto fail;
	}

	psd->size = (psd->size + getpagesize () - 1) / getpagesize () * getpagesize ();
	psd->addr = fb_fix.smem_start;		/* maps FB memory to user space */

	/*if( ufb_mmap_to_user_space( fb, &psd->addr, (void *)fb_info.smem_start, fb_info.smem_len)) {
		EPRINTF("Error mapping FB memory to user space\n" );
		goto fail;
	}*/

	/*exec.func_no = FB_FUNC_ENTER_GRAPHICS;
    exec.param = 0;
	if( ioctl(fb, FB_EXEC_FUNCTION , ( void *)&exec)) {
		EPRINTF("Error entering graphics\n");
		return NULL;
	}*/

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

	/* save original palette*/
	ioctl_getpalette(0, 16, saved_red, saved_green, saved_blue);

	/* setup direct color palette if required (ATI cards)*/
	if(visual == FB_VISUAL_DIRECTCOLOR)
		set_directcolor_palette(psd);

	status = 2;
	return psd;	/* success*/

fail:
	close( fb );
	return NULL;
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
	/* if not opened, return*/
	if(status != 2)
		return;
	status = 1;

  	/* reset hw palette*/
	ioctl_setpalette(0, 16, saved_red, saved_green, saved_blue);

	/* unmaps memory from user's space */
	/* this function previously returned 0
	I will see later what I can do about it
	ufb_unmmap_from_user_space( fb, psd->addr );*/

	/* restore TEXT mode */
	/*exec.func_no = FB_FUNC_EXIT_GRAPHICS;
	ioctl( fb, FB_EXEC_FUNCTION, &exec);*/

	/* close tty and framebuffer*/
	close( fb );
}

/* setup directcolor palette - required for ATI cards*/
static void
set_directcolor_palette(PSD psd)
{
	int i;
	short r[256], g[256], b[256];

	/* 16bpp uses 32 palette entries*/
	if(psd->bpp == 16) {
		/* FIXME: this still doesn't work*/
		for(i=0; i<32; ++i) {
#if 0
			r[i] = g[i] = b[i] = ((i<<11)|(i<<6)|i)<<8;
			r[i] = g[i] = b[i] = ((i<<5)|i)<<10;
			r[i] = g[i] = b[i] = i<<11;
#endif
			r[i] = g[i] = b[i] = (i<<11) | (i<<3);
		}
		ioctl_setpalette(0, 32, r, g, b);
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
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl( fb, FBIOGETCMAP, &cmap );
}

/* set framebuffer palette*/
void
ioctl_setpalette(int start, int len, short *red, short *green, short *blue)
{
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl( fb, FBIOPUTCMAP, &cmap );
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
