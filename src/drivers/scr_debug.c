/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Experimental debug screen driver for Microwindows
 *
 * 21-Feb-2000 ajr@ecs.soton.ac.uk
 * Stripped down the VGA driver to make a debug driver so that I can debug the
 * rest of the code.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"

/* DBG driver entry points*/
static PSD  DBG_open(PSD psd);
static void DBG_close(PSD psd);
static void DBG_getscreeninfo(PSD psd,PMWSCREENINFO psi);;
static void DBG_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void DBG_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL DBG_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void DBG_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void DBG_drawvline(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2,MWPIXELVAL c);
static void DBG_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
		MWPIXELVAL c);
void DBG_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
		PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	DBG_open,
	DBG_close,
	DBG_getscreeninfo,
	DBG_setpalette,
	DBG_drawpixel,
	DBG_readpixel,
	DBG_drawhline,
	DBG_drawvline,
	DBG_fillrect,
	gen_fonts,
	DBG_blit,
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	NULL,			/* MapMemGC*/
	NULL			/* FreeMemGC*/
};

#define printd(_a)

static PSD
DBG_open(PSD psd)
{
	/* init driver variables depending on ega/vga mode*/
	psd->xres = psd->xvirtres = 640;
	psd->yres = psd->yvirtres = 480;
	psd->planes = 4;
	psd->bpp = 4;
	psd->ncolors = 16;
	psd->pixtype = MWPF_PALETTE;
	psd->flags = PSF_SCREEN;

	return psd;
}

static void
DBG_close(PSD psd)
{
	printd("DBG_close()\n");
}

static void
DBG_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = 1;

	/* DBG 640x480*/
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	printd("DBG_getscreeninfo()\n");
}

static void
DBG_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	printd("DBG_setpalette()\n");
}

static void
DBG_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	printd("DBG_drawpixel()\n");
}

static MWPIXELVAL
DBG_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	printd("DBG_readpixel()\n");
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
DBG_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	printd("DBG_drawhline()\n");
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
DBG_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	printd("DBG_drawvline()\n");
}

static void
DBG_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
	printd("DBG_fillrect()\n");
}

void DBG_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	printd("DBG_blit()\n");
}
