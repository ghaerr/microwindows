/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Based on code by Jakob Eriksson.
 *
 * Hercules Graphics Screen Driver, PC bios version
 * 	This driver uses int10 bios to to get the address of the 
 * 	ROM character font which is used for the character bitmaps.  
 * 	All other access to the hardware is controlled through this driver.
 *
 * 	All text/font drawing code is based on the above routines and
 * 	the included entry points for getting the ROM bitmap data.  Compiled
 * 	in fonts aren't supported for size reasons.  scr_bogl supports them.
 *
 *	The environment variable CHARHEIGHT if set will set the assumed rom
 *	font character height, which defaults to 14.
 */
#if ELKS
#include <linuxmt/ntty.h>
#endif
#include <stdio.h>
#include "device.h"
#include "vgaplan4.h"
#include "romfont.h"

/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENADDR(offset) 	MK_FP(0xb000, (offset))

#define SCREEN_ON 		8
#define BLINKER_ON 		0x20

/* HERC driver entry points*/
static PSD  HERC_open(PSD psd);
static void HERC_close(PSD psd);
static void HERC_getscreeninfo(PSD psd,PMWSCREENINFO psi);;
static void HERC_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void HERC_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL HERC_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void HERC_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void HERC_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void HERC_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
		MWPIXELVAL c);
static void HERC_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
		PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
static PSD  HERC_allocatememgc(PSD psd);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	HERC_open,
	HERC_close,
	HERC_getscreeninfo,
	HERC_setpalette,
	HERC_drawpixel,
	HERC_readpixel,
	HERC_drawhline,
	HERC_drawvline,
	HERC_fillrect,
	pcrom_fonts,
	HERC_blit,
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	HERC_allocatememgc,
	NULL,			/* MapMemGC*/
	NULL			/* FreeMemGC*/
};

static PSD
HERC_open(PSD psd)
{
#if HAVEIOPERM
	/* get permission to write to the hercules ports*/
	if(ioperm(0x3b4, 0x0d, 1))
		return NULL;
#endif
#if ELKS
	/* disallow console switching while in graphics mode*/
	if(ioctl(0, DCGET_GRAPH) != 0)
		return NULL;
#endif

	/* enter graphics mode*/
	outportb(0x3bf, 1+2);
	outportb(0x3b8, 0);
	outport(0x3b4, 0x3500);
	outport(0x3b4, 0x2d01);
	outport(0x3b4, 0x2e02); /* Linesync at 46th character */
	outport(0x3b4, 0x0703); /* linesync width 7 chrclock ticks */
	outport(0x3b4, 0x5b04); /* height 92 chars (368 lines) */
	outport(0x3b4, 0x0205); /* height adjust */
	outport(0x3b4, 0x5706); /* lines / picture (348 lines)  */
	outport(0x3b4, 0x5707); /* picturesync: after 87 character lines */
	outport(0x3b4, 0x0309); /* character height: 4 lines / char */
	outportb(0x3b8, 2+8); 	/* Allow graphics mode and video on */

	/* init driver variables*/
	psd->xres = psd->xvirtres = 720;
	psd->yres = psd->yvirtres = 350;
	psd->planes = 1;
	psd->bpp = 1;
	psd->ncolors = 2;
	psd->pixtype = MWPF_PALETTE;
	psd->flags = PSF_SCREEN;
	psd->addr = SCREENADDR(0);
	psd->linelen = 90;

	/* init pc rom font routines*/
	pcrom_init(psd);

	return psd;
}

static void
HERC_close(PSD psd)
{
	int			i;
	volatile FARADDR	dst;
	static char 		herc_txt_tbl[12]= {
		0x61,0x50,0x52,0x0f,0x19,6,0x19,0x19,2,0x0d,0x0b,0x0c
	};

#if ELKS
	/* allow console switching again*/
	ioctl(0, DCREL_GRAPH);
#endif

	/* switch back to text mode*/
	outportb(0x3bf, 0);
	outportb(0x3b8, 0);

	for(i = 0; i < 12; ++i) {
		outportb(0x3b4, i);
		outportb(0x3b5, herc_txt_tbl[i]);
	}

	/* blank screen*/
	dst = SCREENADDR(0);
	for(i = 0; i <= 0x3fff; ++i) {
		PUTBYTE_FP(dst++, 0x00);
		PUTBYTE_FP(dst++, 0x07);
	}
	outportb(0x3b8, BLINKER_ON | SCREEN_ON);
}

static void
HERC_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;
	psi->xdpcm = 30;		/* assumes screen width of 24 cm*/
	psi->ydpcm = 19;		/* assumes screen height of 18 cm*/
}

static void
HERC_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	/* no palette available*/
}

static void
HERC_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	unsigned int 		offset;
	unsigned char 		mask = 128;
	volatile FARADDR	dst;

	offset = 0x2000 * (y&3);
	offset += 90*(y/4) + x/8;
	dst = SCREENADDR(offset);
	mask >>= x & 7;
	if(c)
		ORBYTE_FP(dst, mask);
	else ANDBYTE_FP(dst, ~mask);
}

static MWPIXELVAL
HERC_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	unsigned int 		offset;
	unsigned char 		mask = 128;
	volatile FARADDR	dst;

	offset = 0x2000 * (y&3);
	offset += 90*(y/4) + x/8;
	dst = SCREENADDR(offset);
	mask >>= x & 7;
	return GETBYTE_FP(dst) & mask? 1: 0;
}

static void
HERC_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	/*Optimized HERC_drawhline() by thomas_d_stewart@hotmail.com*/
	unsigned int rowoffset, x1yoffset, x2yoffset;
	unsigned int startbyte, endbyte;
	volatile FARADDR dst;

	/*offset of the row */
	rowoffset = 8192 * (y % 4) + (y / 4) * 90;
	
	/*offset of first byte in line */
	x1yoffset = rowoffset + (x1 / 8);
	
	/*ofset of the last byte in line */
	x2yoffset = rowoffset + (x2 / 8);


	/*shift "11111111" > buy (x1%8) to fill with 0's*/
	startbyte = 0xff >> (x1 % 8);
	/*same but in < dir*
	endbyte = 0xff << (x2 % 8);

	/* convert x1yoffset to a screen address */
	dst = SCREENADDR(x1yoffset);

	if(c)
		ORBYTE_FP(dst, startbyte);    /* output byte to mem */
	else ANDBYTE_FP(dst, ~startbyte);

	x1yoffset++; /* increment so we are writing to the next byte */
	while(x1yoffset < x2yoffset) {
	       	dst = SCREENADDR(x1yoffset); /*convert x1yoffset to a scr address */
		if(c)
			ORBYTE_FP(dst, 0xff); /*ouput bytes */
		else ANDBYTE_FP(dst, ~0xff);
		x1yoffset++;
		}

	dst = SCREENADDR(x2yoffset); /* convert x2yoffset to a screen address */
	if(c)
		ORBYTE_FP(dst, endbyte); /* output byte to mem */
	else ANDBYTE_FP(dst, ~endbyte);



	/*NON Optimized version left in case my one goes wroung */
	/*while(x1 <= x2)
	  HERC_drawpixel(x1++, y, c);*/
}

static void
HERC_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	/* fixme write optimized vline*/
	/*
	 * Driver doesn't support vline yet
	 */
	while(y1 <= y2)
		HERC_drawpixel(x, y1++, c);
}

static void
HERC_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	while(y1 <= y2)
		HERC_drawhline(x1, x2, y1++, c);
}

static void
HERC_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op)
{
	/* FIXME*/
}

/* allocate a memory screen device*/
static PSD 
HERC_allocatememgc(PSD psd)
{
	/* if driver doesn't have blit, fail*/
	return NULL;
}
