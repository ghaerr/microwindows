/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com> 
 *
 * Copyright (c) 1999 Victor Rogachev <rogach@sut.ru>
 *
 * Screen Driver using DJGPP & GRX  Library
 *
 *  For only GRX lib 
 *
 * This driver requires the following GRX entry points:
 * 	GrSetMode, GrSetColor, GrPlot, GrPixel,
 * 	GrHLine, GrVLine, GrFilledBox
 *
 * All graphics drawing primitives are based on top of these functions.
 */

#include <stdio.h>
#include "device.h"
#include "genfont.h"

#include <grx20.h>

/* specific grxlib driver entry points*/
static PSD  DJGR_open(PSD psd);
static void DJGR_close(PSD psd);
static void DJGR_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void DJGR_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void DJGR_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL DJGR_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void DJGR_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void DJGR_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void DJGR_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,MWPIXELVAL c);
static void DJGR_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,
		MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
static PSD  DJGR_allocatememgc(PSD psd);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	DJGR_open,
	DJGR_close,
	DJGR_getscreeninfo,
	DJGR_setpalette,
	DJGR_drawpixel,
	DJGR_readpixel,
	DJGR_drawhline,
	DJGR_drawvline,
	DJGR_fillrect,
	gen_fonts,
	DJGR_blit,
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	DJGR_allocatememgc,
	NULL,			/* MapMemGC*/
	NULL			/* FreeMemGC*/
};

extern int gr_mode;	/* temp kluge*/


/*
**	Open graphics
*/
static PSD
DJGR_open(PSD psd)
{
	int		x;
	int		y;
	int		c;
        GrVideoMode	*md_info;

	x = 640;
	y = 480;
	c = 256;

        GrSetMode(GR_width_height_color_graphics,x,y,c);

        md_info = (GrVideoMode *) GrCurrentVideoMode();

	psd->xres = psd->xvirtres = GrScreenX();
	psd->yres = psd->yvirtres = GrScreenY();
	psd->linelen = md_info->lineoffset;
	psd->planes = 1;
	psd->bpp = md_info->bpp;
	psd->ncolors = GrNumColors();
	psd->flags = PSF_SCREEN;
	psd->addr = 0;		/* FIXME */

	/* note: must change psd->pixtype here for truecolor systems*/
	psd->pixtype = MWPF_PALETTE;

	return psd;
}

/*
**	Close graphics
*/
static void
DJGR_close(PSD psd)
{
	GrSetMode(GR_default_text);
}

/*
**	Get Screen Info
*/
static void
DJGR_getscreeninfo(PSD psd,PMWSCREENINFO psi)
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

/*
**	Set Palette
*/
static void
DJGR_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	while(first < 256 && count-- > 0) {
		GrSetColor(first++, pal->r, pal->g, pal->b);
		++pal;
	}
}

/*
**	Draw Pixel
*/
static void
DJGR_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	GrPlot(x, y, c);
}

/*
**	Read Pixel
*/
static MWPIXELVAL
DJGR_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return GrPixel(x, y);
}

/*
**	Draw Horizontal Line
*/
static void
DJGR_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	GrHLine(x1, x2, y, c);
}

/*
**	Draw Vertical Line
*/
static void
DJGR_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	GrVLine(x, y1, y2, c);
}

/*
**	Filled Box
*/
static void
DJGR_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	GrFilledBox(x1, y1, x2, y2, c);
}

/*
**	Blit
*/
static void
DJGR_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
		PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op)
{
	/* FIXME*/
}

/* allocate a memory screen device*/
static PSD 
DJGR_allocatememgc(PSD psd)
{
	/* if driver doesn't have blit, fail*/
	return NULL;
}
