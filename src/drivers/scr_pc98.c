/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * PC-98 16 color 4 planes Screen Driver
 * This driver is created and modifed, based on EGA/VGA driver.
 * T. Yamada 2022
 *
 * 	For PC-98, 640x400 resolution
 * 	This driver uses LIO for graphical setting.
 *
 * 	This file itself doesn't know about any planar or packed arrangement, relying soley
 * 	on routines in vgaplan4_pc98.c for drawing.
 */

#include <linuxmt/ntty.h>
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "vgaplan4.h"
#include "genmem.h"
#include "fb.h"
#if ROMFONT
#include "romfont.h"
#else
#include "genfont.h"
#endif

#define LIOSEG      0xF990  /* ROM Segment for LIO */
#define LIOINT      0xA0    /* Starting LIO interrupt number */
#define LIO_M_SIZE  5200

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

/* PC98 driver entry points*/
static PSD  PC98_open(PSD psd);
static void PC98_close(PSD psd);
static void PC98_getscreeninfo(PSD psd, PMWSCREENINFO psi);;
static void PC98_setpalette(PSD psd,int first,int count, MWPALENTRY *pal);

SCREENDEVICE	scrdev = {
    0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
#if ROMFONT
        pcrom_fonts,
#else
        gen_fonts,
#endif
        PC98_open,
        PC98_close,
        PC98_setpalette,
        PC98_getscreeninfo,
        gen_allocatememgc,
        gen_mapmemgc,
        gen_freememgc,
        0,                      /* gen_setportrait*/
        NULL,                   /* PreSelect*/
};

void int_A0(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "int $0xA0;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A1(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA1;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A2(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA2;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A3(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA3;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

static PSD
PC98_open(PSD psd)
{
	unsigned char *lio_malloc;
	unsigned char __far *lio_m;
	unsigned int lio_m_seg;

	unsigned long __far *intvec;
	unsigned int __far *lioaddr;
	unsigned int i;

	unsigned int __far *tvram;

	extern SUBDRIVER vgaplan4_none;

	// Clear 80*25 words text vram starting from segment 0xA000
	tvram = (unsigned int __far *) _MK_FP(0xA000,0);
	for (i = 0; i < 2000; i++) {
		*tvram = 0;
		tvram++;
	}

	/* init driver variables for PC-98*/
	psd->xres = psd->xvirtres = 640;
	psd->yres = psd->yvirtres = 400;
	psd->planes = 4;
	psd->bpp = 4;
	psd->ncolors = 16;
	psd->pixtype = MWPF_PALETTE;
	psd->flags = PSF_SCREEN;
    set_subdriver(psd, &vgaplan4_none);

	pc98_init(psd);             /* init planes driver (sets psd->addr and psd->linelen)*/

	// Clear graphic vram
	for (i = 0; i < psd->yres; i++)
		pc98_drawhorzline(psd, 0, psd->xres, i, 0);

    /* interrupt vector for INT 0xA0 */
	intvec = (unsigned long __far *) _MK_FP(0, LIOINT<<2);
    /* Starting Rom Address for INT 0xA0 handler */
	lioaddr = (unsigned int __far *) _MK_FP(LIOSEG, 6);

	// Set interrupt vector 0xA0 - 0xAF
	for (i = 0; i < 16; i++) {
		*intvec = (unsigned long) _MK_FP(LIOSEG, *lioaddr);
		intvec++;
		lioaddr += 2;
	}

	// Allocate memory for LIO
	lio_malloc = malloc(LIO_M_SIZE);
	lio_m = (unsigned char __far *) lio_malloc;

	lio_m_seg = (unsigned int) ((((unsigned long) lio_m) >> 16) + ((((unsigned long) lio_m) & 0xFFFF) >> 4) + 1);
	lio_m = (unsigned char __far *) (((unsigned long) lio_m_seg) << 16);

	int_A0(lio_m_seg); // Init

	lio_m[0] = 0x03; // Color 640x400
	lio_m[1] = 0x00;
	lio_m[2] = 0x00;
	lio_m[3] = 0x01;
	int_A1(lio_m_seg);

	lio_m[0] = 0x00;
	lio_m[1] = 0x00;
	lio_m[2] = 0x00;
	lio_m[3] = 0x00;
	lio_m[4] = 0x7F; // 639
	lio_m[5] = 0x02;
	lio_m[6] = 0x8F; // 399
	lio_m[7] = 0x01;
	lio_m[8] = 0xFF;
	lio_m[9] = 0xFF;
	int_A2(lio_m_seg);

	lio_m[0] = 0xFF;
	lio_m[1] = 0xFF;
	lio_m[2] = 0xFF;
	lio_m[3] = 0x02; // 16 Color mode
	int_A3(lio_m_seg);

	free(lio_malloc);

#if ROMFONT
	pcrom_init(psd);            /* init pc rom font routines*/
#endif

	outb(0x0B,0x68); // Dot Access for font
	return psd;
}

static void
PC98_close(PSD psd)
{
	outb(0x0C,0xA2);   // GDC Stop
	outb(0x0A,0x68);   // Code Access for font
}

static void
PC98_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
    psi->rows = psd->yvirtres;
    psi->cols = psd->xvirtres;
    psi->planes = psd->planes;
    psi->bpp = psd->bpp;
    psi->data_format = psd->data_format;
    psi->ncolors = psd->ncolors;
    psi->pixtype = psd->pixtype;
    psi->fonts = NUMBER_FONTS;

	/* 640x400 */
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 27;	/* assumes screen height of 15 cm*/
}

static void
PC98_setpalette(PSD psd,int first,int count, MWPALENTRY *pal)
{
	/* not yet implemented, std 16 color palette assumed*/
}
