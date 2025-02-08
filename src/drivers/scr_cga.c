/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * CGA Screen Driver
 * This driver is created and modifed, based on EGA/VGA driver.
 * T. Yamada 2024
 *
 * 	For CGA, 2 color, 640x200 resolution
 * 	This driver uses bios for graphical setting.
 *
 * 	This file itself doesn't know about any planar or packed arrangement, relying soley
 * 	on routines in vgaplan4_cga.c for drawing.
 */

#include <linuxmt/ntty.h>
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "vgaplan4.h"
#include "genmem.h"
#if ROMFONT
#include "romfont.h"
#else
#include "genfont.h"
#endif

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

/* CGA driver entry points*/
static PSD  CGA_open(PSD psd);
static void CGA_close(PSD psd);
static void CGA_getscreeninfo(PSD psd, PMWSCREENINFO psi);;
static void CGA_setpalette(PSD psd,int first,int count, MWPALENTRY *pal);

SCREENDEVICE	scrdev = {
    0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
#if ROMFONT
        pcrom_fonts,
#else
        gen_fonts,
#endif
        CGA_open,
        CGA_close,
        CGA_setpalette,
        CGA_getscreeninfo,
        gen_allocatememgc,
        gen_mapmemgc,
        gen_freememgc,
        0,                      /* gen_setportrait*/
        NULL,                   /* PreSelect*/
};

/* operating mode*/
static MWBOOL MONOMODE = FALSE;	/* monochrome mode*/

/* int10 functions*/
#define FNGR640x200	0x0006	/* function for graphics mode 640x200x2*/
#define FNTEXT		0x0003	/* function for 80x25 text mode*/

static PSD
CGA_open(PSD psd)
{
	extern SUBDRIVER vgaplan4_none;

	/* setup operating mode from environment variable*/
	if(getenv("MONOMODE"))
		MONOMODE = TRUE;
	else MONOMODE = FALSE;

	int10(FNGR640x200, 0);

	/* init driver variables depending on cga mode*/
	psd->xres = psd->xvirtres = 640;
	psd->yres = psd->yvirtres = 200;
	psd->planes = 1;
	psd->pixtype = MWPF_PALETTE;
	psd->flags = PSF_SCREEN;
	if (MONOMODE) {
		psd->bpp = 1;
		psd->ncolors = 2;
	} else {
		psd->bpp = 4;           /* color applications */
		psd->ncolors = 16;
	}
    set_subdriver(psd, &vgaplan4_none);

	cga_init(psd);              /* init planes driver (sets psd->linelen)*/

#if ROMFONT
	pcrom_init(psd);            /* init pc rom font routines*/
#endif
	return psd;
}

static void
CGA_close(PSD psd)
{
	int10(FNTEXT, 0);           /* init bios 80x25 text mode*/
}

static void
CGA_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
    psd->data_format = psd->data_format;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	/* 640x200 */
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 11;	/* assumes screen height of 18 cm*/
}

static void
CGA_setpalette(PSD psd,int first,int count, MWPALENTRY *pal)
{
	/* not yet implemented, std 16 color palette assumed*/
}
