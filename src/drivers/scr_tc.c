/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Copyright (c) 2000 Victor Larionov, Victor Rogachev <rogach@sut.ru>
 * Modified by mlkao
 *
 * Screen Driver using BGI for DOS TURBOC
 *
 * This driver requires the following BGI entry points:
 * 	initgraph, closegraph, 
 * 	putpixel, getpixel
 * 	setcolor, line, setfillstyle, bar
 *
 * All graphics drawing primitives are based on top of these functions.
 *
 * This file also contains the generalized low-level font/text
 * drawing routines, which will be split out into another file.
 * Both fixed and proportional fonts are supported.
 */

#include <stdio.h>
#include <graphics.h>
#include "device.h"
#include "genfont.h"

/* specific bgi driver entry points*/
static PSD  BGI_open(PSD psd);
static void BGI_close(PSD psd);
static void BGI_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void BGI_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void BGI_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL BGI_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void BGI_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void BGI_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void BGI_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,MWPIXELVAL c);
static void BGI_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
		PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
static PSD  BGI_allocatememgc(PSD psd);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	BGI_open,
	BGI_close,
	BGI_getscreeninfo,
	BGI_setpalette,
	BGI_drawpixel,
	BGI_readpixel,
	BGI_drawhline,
	BGI_drawvline,
	BGI_fillrect,
	gen_fonts,
	BGI_blit,
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	BGI_allocatememgc,
	NULL,			/* MapMemGC*/
	NULL			/* FreeMemGC*/
};

/* add by mlkao */
extern int gr_mode;	/* temp kluge*/
static struct linesettingstype lineinfo;
static struct palettetype bgi_pal;

static PSD
BGI_open(PSD psd)
{
	int		gd=VGA;
	int		gm=VGAHI;

	registerbgidriver(EGAVGA_driver);
	initgraph(&gd,&gm,"");

	getlinesettings(&lineinfo);

	psd->xres = psd->xvirtres = getmaxx()+1;
	psd->yres = psd->yvirtres = getmaxy()+1;
	psd->linelen = lineinfo.thickness;
	psd->planes = 1;
	psd->bpp = 4;		/* FIXME?? */
	psd->ncolors = getmaxcolor() + 1;
	psd->flags = PSF_SCREEN;
	psd->addr = 0;		/* FIXME */

	/* note: must change psd->pixtype here for truecolor systems*/
	psd->pixtype = MWPF_PALETTE;
	return psd;
}

static void
BGI_close(PSD psd)
{
	closegraph();
}

static void
BGI_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	if(scrdev.yvirtres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(scrdev.yvirtres > 350) {
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
BGI_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	/* std 16 color palette assumed*/
}

static void
BGI_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	switch(gr_mode) {
		case MWMODE_COPY: break;
		case MWMODE_XOR: c ^= getpixel(x, y); break;
		case MWMODE_OR:  c |= getpixel(x, y); break;
		case MWMODE_AND: c &= getpixel(x, y); break;
	}
	putpixel(x, y, c);
}

static MWPIXELVAL
BGI_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return getpixel(x, y);
}

static void
BGI_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	MWCOORD x;

	if (x1 > x2) {
		x  = x1;
		x1 = x2;
		x2 = x;
	}

	setcolor(c);
	switch(gr_mode) {
		case MWMODE_COPY:
			setwritemode(COPY_PUT);
			break;
		case MWMODE_XOR:
			setwritemode(XOR_PUT);
			break;
		case MWMODE_OR: 
			for(x = x1; x <= x2; x++)
				putpixel(x, y, c | getpixel(x, y));
			return;
		case MWMODE_AND:
			for(x = x1; x <= x2; x++)
				putpixel(x, y, c & getpixel(x, y));
			return;
	}
	line(x1, y, x2, y);
}

static void
BGI_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	MWCOORD y;

	if (y1 > y2) {
		y  = y1;
		y1 = y2;
		y2 = y;
	}

	setcolor(c);
	switch(gr_mode) {
		case MWMODE_COPY:
			setwritemode(COPY_PUT);
			break;
		case MWMODE_XOR:
			setwritemode(XOR_PUT); 
			break;
		case MWMODE_OR: 
			for(y = y1; y <= y2; y++)
				putpixel(x, y, c | getpixel(x, y));
				return;
		case MWMODE_AND:
			for(y = y1; y <= y2; y++)
				putpixel(x, y, c & getpixel(x, y));
				return;
	}
	line(x, y1, x, y2);
}

static void
BGI_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
	MWCOORD x, y;

	if (x1 > x2) {
		x  = x1;
		x1 = x2;
		x2 = x;
	}
	if (y1 > y2) {
		y  = y1;
		y1 = y2;
		y2 = y;
	}

	switch(gr_mode) {
		case MWMODE_COPY:
			setfillstyle(1,c);
			bar(x1, y1, x2, y2);
			break;

		case MWMODE_XOR:
			for(x = x1; x <= x2; x++)
				for(y = y1; y <= y2; y++)
					putpixel(x, y, c ^ getpixel(x, y));
			break;

		case MWMODE_OR:
			for(x = x1; x <= x2; x++)
				for(y = y1; y <= y2; y++)
					putpixel(x, y, c | getpixel(x, y));
			break;

		case MWMODE_AND:
			for(x = x1; x <= x2; x++)
				for(y = y1; y <= y2; y++)
					putpixel(x, y, c & getpixel(x, y));
			break;

		default:
			break;
	}
}

static void
BGI_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
		PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op)
{
	/* FIXME*/
}

/* allocate a memory screen device*/
static PSD 
BGI_allocatememgc(PSD psd)
{
	/* if driver doesn't have blit, fail*/
	return NULL;
}
