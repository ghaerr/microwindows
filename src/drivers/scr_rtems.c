/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver for RTEMS (uses Microframebuffer api)
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 * 
 * Note: modify select_fb_driver() to add new framebuffer subdrivers
 */
#define _GNU_SOURCE 1
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
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
#include <rtems/mw_fb.h>

#ifndef FB_TYPE_VGA_PLANES
#define FB_TYPE_VGA_PLANES 4
#endif

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
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
	gen_freememgc
};

/* static variables*/
static int fb;			/* Framebuffer file handle. */
static int status;		/* 0=never inited, 1=once inited, 2=inited. */
static short saved_red[16];	/* original hw palette*/
static short saved_green[16];
static short saved_blue[16];

/* local functions*/
static void	set_directcolor_palette(PSD psd);
#if 0
static void 	ioctl_getpalette(int start, int len, short *red, short *green,
			short *blue);
static void	ioctl_setpalette(int start, int len, short *red, short *green,
			short *blue);
#endif

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
	char *	env;
	int	type, visual;
	int	tty;
	PSUBDRIVER subdriver;
	struct fb_screeninfo fb_info;

	assert(status < 2);
  
	/* locate and open framebuffer, get info*/
	if(!(env = getenv("FRAMEBUFFER")))
		env = "/dev/fb0";
	fb = open( env, O_RDWR);
	if(fb < 0) {
		EPRINTF("Error opening %s: %m\n", env);
      return NULL;
   }
   
   if( ufb_get_screen_info( fb, &fb_info ) )
   {
      EPRINTF("Error getting screen info\n" );
      return NULL;
   }
	/* setup screen device from framebuffer info*/
	type = fb_info.type;
	visual = fb_info.visual;

	psd->xres = psd->xvirtres = fb_info.xres;
	psd->yres = psd->yvirtres = fb_info.yres;

	/* set planes from fb type*/
	if (type == FB_TYPE_VGA_PLANES)
		psd->planes = 4;
	else if (type == FB_TYPE_PACKED_PIXELS)
		psd->planes = 1;
	else psd->planes = 0;	/* force error later*/

	psd->bpp = fb_info.bits_per_pixel;
	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);

	/* set linelen to byte length, possibly converted later*/
	psd->linelen = fb_info.line_length;
	psd->size = 0;		/* force subdriver init of size*/

#if HAVEBLIT
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
#else
	psd->flags = PSF_SCREEN;
#endif
	if (psd->bpp == 16)
		psd->flags |= PSF_HAVEOP_COPY;

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
			psd->pixtype = MWPF_TRUECOLOR0888;
			break;
		default:
			EPRINTF(
			"Unsupported %d color (%d bpp) truecolor framebuffer\n",
				psd->ncolors, psd->bpp);
			goto fail;
		}
	} else psd->pixtype = MWPF_PALETTE;

	psd->size = fb_info.smem_len;
   /* maps FB memory to user space */
   if( ufb_mmap_to_user_space( fb, &psd->addr, 
                              ( void *)fb_info.smem_start, fb_info.smem_len ) )
   {
      EPRINTF("Error mapping FB memory to user space\n" );
      goto fail;
   }

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

   if( ufb_enter_graphics( fb, 0 ) )
   {
      EPRINTF("Error entering graphics\n");
      return NULL;
   }

	/*
	 * set and initialize subdriver into screen driver
	 * psd->size is calculated by subdriver init
	 */
	if(!set_subdriver(psd, subdriver, TRUE )) 
	{
		EPRINTF("Driver initialize failed type %d visual %d bpp %d\n",
			type, visual, psd->bpp);
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
   ufb_unmmap_from_user_space( fb, psd->addr );

   /* restore TEXT mode */
   ufb_exit_graphics( fb );
  
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
static void
ioctl_getpalette(int start, int len, short *red, short *green, short *blue)
{
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

   ufb_get_palette( fb, &cmap );
}

/* set framebuffer palette*/
static void
ioctl_setpalette(int start, int len, short *red, short *green, short *blue)
{
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

   ufb_set_palette( fb, &cmap );
}

static void
gen_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	if(psd->yvirtres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(psd->yvirtres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
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
