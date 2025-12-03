/*
 * EGA/VGA 16 color 4 planes Screen Driver, direct hardware or bios
 * 	If HWINIT 1 is #defined, the file vgainit.c is
 * 	used to provide non-bios direct hw initialization of the VGA
 * 	chipset.  Otherwise, this driver uses int10 bios to 
 * 	set/reset graphics/text modes.
 *
 * 	If ROMFONT is #defined, the file romfont.c is used to
 * 	draw all fonts, afte the bios is called to
 * 	get the address of the ROM character font which
 * 	is used for the character bitmaps.  Otherwise, the
 * 	file genfont.c is used to draw linked in fonts, without
 * 	consulting the bios.
 *
 * 	All other access to the hardware is controlled through this driver.
 *
 * 	This driver links with another file, vgaplan4_vga.c,
 * 	the portable VGA 4 planes 16 color driver. This file itself
 * 	doesn't know about any planar or packed arrangement, relying
 * 	on routines in vgaplan4_vga.c for drawing.
 *
 * 	All text/font drawing code is based on the above routines and
 * 	the included entry points for getting the ROM bitmap data.
 *
 * 	If the environment variable EGAMODE is set, the driver implements
 *	the EGA 640x350 (mode 10h) resolution, otherwise 640x480 (mode 12h)
 *	graphics mode is set.
 *
 *	The environment variable CHARHEIGHT if set will set the assumed rom
 *	font character height, which defaults to 14.
 *
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * 6 Feb 2025 Greg Haerr Revived from the dead for ELKS!
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#if ELKS
#include <linuxmt/ntty.h>	/* DCGET_GRAPH ioctl */
#define HWINIT		0	/* =1 for non-bios direct hardware init*/
#define ROMFONT		0	/* =1 uses PC rom fonts */
#elif _MINIX
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ibm/int86.h>
#define HWINIT		0
#define ROMFONT		0
#elif RTEMS
#define HWINIT		1
#define ROMFONT		0
#else
#define HWINIT		0
#define ROMFONT		0
#endif

#include "device.h"
#include "vgaplan4.h"
#include "genmem.h"
#include "fb.h"
#if ROMFONT
#include "romfont.h"
#else
#include "genfont.h"
#endif

/* VGA driver entry points*/
static PSD  VGA_open(PSD psd);
static void VGA_close(PSD psd);
static void VGA_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void VGA_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);

SCREENDEVICE	scrdev = {
    0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
#if ROMFONT
	pcrom_fonts,
#else
	gen_fonts,
#endif
	VGA_open,
	VGA_close,
	VGA_setpalette,
	VGA_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	0,                      /* gen_setportrait*/
	NULL,			/* PreSelect*/
};

/* operating mode*/
static MWBOOL VGAMODE = TRUE;	/* ega or vga screen rows*/

/* int10 functions*/
#define FNGR640x480	0x0012	/* function for graphics mode 640x480x16*/
#define FNGR640x350	0x0010	/* function for graphics mode 640x350x16*/
#define FNTEXT		0x0003	/* function for 80x25 text mode*/

static PSD
VGA_open(PSD psd)
{
	extern PSUBDRIVER vgaplan4[4];

	/* setup operating mode from environment variable*/
	if(getenv("EGAMODE"))
		VGAMODE = FALSE;
	else VGAMODE = TRUE;

#if ELKS
	/* disallow console switching while in graphics mode*/
	//ioctl(2, DCGET_GRAPH);
#endif

#if HWINIT
	ega_hwinit();		/* enter graphics mode*/
#else
	/* init bios graphics mode*/
	int10(VGAMODE? FNGR640x480: FNGR640x350, 0);
#endif

	/* init driver variables depending on ega/vga mode*/
	psd->xres = psd->xvirtres = 640;
	psd->yres = psd->yvirtres = VGAMODE? 480: 350;
	psd->planes = 4;
	psd->bpp = 4;
	psd->ncolors = 16;
	psd->pixtype = MWPF_PALETTE;
	psd->flags = PSF_SCREEN;
	set_subdriver(psd, vgaplan4[0]);
	vga_init(psd);		/* init planes driver (sets psd->pitch)*/

#if ROMFONT
	pcrom_init(psd);	/* init pc rom font routines*/
#endif
#if 0
	ROM_CHAR_HEIGHT = VGAMODE? 16: 14;
#endif
	return psd;
}

static void
VGA_close(PSD psd)
{
#if ELKS
	//ioctl(2, DCREL_GRAPH); /* allow console switching again*/
#endif
#if HWINIT
	ega_hwterm();
#else
	int10(FNTEXT, 0);       /* init bios 80x25 text mode*/
#endif
}

static void
VGA_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->data_format = psd->data_format;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;
	if(VGAMODE) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}

static void
VGA_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

#if _MINIX
FARADDR int10(int mode, int z)
{
   int fd;
   struct mio_int86 mint86;
   static int once = 0;

   if (!once) {
        int s;
        struct mio_ldt86 mldt86;
        struct segdesc_s {              /* segment descriptor for protected mode */
	        u16_t limit_low;
	        u16_t base_low;
	        u8_t base_middle;
	        u8_t access;                  /* |P|DL|1|X|E|R|A| */
	        u8_t granularity;             /* |G|X|0|A|LIMT| */
	        u8_t base_high;
        } *dp;
        unsigned long vidmem = 0xA0000;

	fd = open("/dev/mem", O_RDONLY);
	dp = (struct segdesc_s *) mldt86.entry;
	mldt86.idx = 1;
	s = ioctl(fd, MIOCGLDT86, &mldt86);
	dp->limit_low    = 0xFFFF;
	dp->base_low     = (vidmem >>  0) & 0xFFFF;
	dp->base_middle  = (vidmem >> 16) & 0xFF;
	dp->base_high    = (vidmem >> 24) & 0xFF;
	dp->granularity |= 0x80;
	mldt86.idx = 2;
	s = ioctl(fd, MIOCSLDT86, &mldt86);
	close(fd);
        once = 1;
   }
   fd = open("/dev/mem", O_RDONLY);
   memset(&mint86, 0, sizeof(mint86));
   ioctl(fd, MIOCINT86, &mint86);
   mint86.reg86.b.intno = 0x10;
   mint86.reg86.b.al = mode & 0xFF;
   ioctl(fd, MIOCINT86, &mint86);
   close(fd);
}
#endif
