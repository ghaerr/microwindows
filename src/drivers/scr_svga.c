/*
 * Copyright (c) 1999, 2000, 2001 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver using SVGA Library
 *
 * This driver requires the following SVGA entry points:
 * 	vga_init, vga_setmode,
 * 	vga_drawpixel, vga_getpixel,
 * 	vga_setegacolor, vga_drawline,
 *	vga_getscansegment, vga_drawscansegment
 *
 * All graphics drawing primitives are based on top of these functions.
 */
/*#define NDEBUG*/
#include <assert.h>
#include <stdio.h>
#include <vga.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"

#define MAXLINELEN	800	/* max line byte/pixel length*/

/* specific driver entry points*/
static PSD  SVGA_open(PSD psd);
static void SVGA_close(PSD psd);
static void SVGA_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void SVGA_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void SVGA_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL SVGA_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void SVGA_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void SVGA_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void SVGA_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2,
		MWCOORD y2, MWPIXELVAL c);
static void SVGA_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
		MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	SVGA_open,
	SVGA_close,
	SVGA_getscreeninfo,
	SVGA_setpalette,
	SVGA_drawpixel,
	SVGA_readpixel,
	SVGA_drawhline,
	SVGA_drawvline,
	SVGA_fillrect,
	gen_fonts,
	SVGA_blit,
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	NULL,			/* MapMemGC*/
	NULL,			/* FreeMemGC*/
	NULL,			/* StretchBlit subdriver*/
	NULL			/* SetPortrait*/
};

extern int gr_mode;	/* temp kluge*/

static PSD
SVGA_open(PSD psd)
{
	int		mode;
	vga_modeinfo *	modeinfo;

	vga_init();

	mode = G800x600x256;
	if(!vga_hasmode(mode))
		mode = G640x480x256;
	if(!vga_hasmode(mode))
		mode = G640x480x16;
	if(vga_setmode(mode) == -1)
		return NULL;
	modeinfo = vga_getmodeinfo(mode);

	psd->xres = psd->xvirtres = modeinfo->width;
	psd->yres = psd->yvirtres = modeinfo->height;
	psd->linelen = modeinfo->linewidth;
	psd->ncolors = modeinfo->colors;
	if(psd->ncolors == 256) {
		psd->planes = 1;
		psd->bpp = 8;
	} else {
		psd->planes = 4;
		psd->bpp = 4;
	}
	/* note: must change psd->pixtype here for truecolor systems*/
	psd->pixtype = MWPF_PALETTE;
	psd->flags = PSF_SCREEN;
	psd->size = 0;
	psd->addr = NULL;

	/*DPRINTF("mode: %dx%dx%d bpp %d\n", psd->xres, psd->yres,
	 	psd->ncolors, psd->bpp);*/

	return psd;
}

static void
SVGA_close(PSD psd)
{
	vga_setmode(TEXT);
}

static void
SVGA_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->fonts = NUMBER_FONTS;
	psi->portrait = MWPF_PORTRAIT_NONE;
	psi->fbdriver = FALSE;	/* not running fb driver, no direct map*/
	psi->pixtype = psd->pixtype;
	psi->rmask 	= 0xff;
	psi->gmask 	= 0xff;
	psi->bmask	= 0xff;

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

static void
SVGA_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	while(first < 256 && count-- > 0) {
		vga_setpalette(first++, pal->r>>2, pal->g>>2, pal->b>>2);
		++pal;
	}
}

static void
SVGA_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	unsigned char gline, line = c;

	if(gr_mode == MWMODE_COPY) {
		/* vga_drawpixel apparently doesn't work with 256 colors...
		 * vga_setegacolor(c);
		 * vga_drawpixel(x, y);
		 */
		vga_drawscansegment(&line, x, y, 1);
		return;
	}
	/*
	 * This fishery is required because vgalib doesn't support
	 * xor drawing mode without acceleration.
	 */
	vga_getscansegment(&gline, x, y, 1);
	line ^= gline;
	vga_drawscansegment(&line, x, y, 1);
}

static MWPIXELVAL
SVGA_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return vga_getpixel(x, y);
}

static void
SVGA_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	int 	i, width;
	unsigned char getline[MAXLINELEN];
	static int lastcolor = -1;
	static int lastwidth = -1;
	static unsigned char line[MAXLINELEN];

	/*
	 * All this fishery is required for two reasons:
	 * one, vga_drawline is way too slow to be called to fill
	 * rectangles, so vga_drawscansegment is used instead.  In
	 * addition, vgalib doesn't support xor drawing mode
	 * without acceleration!!, so we've got to do it ourselves
	 * with vga_getscansegment...
	 */
	width = x2-x1+1;

	/* this is faster than calling vga_drawline !!!*/
	if(width != lastwidth || c != lastcolor) {
		lastwidth = width;
		lastcolor = c;
		for(i=0; i<width; ++i)
			line[i] = c;
	}
	if(gr_mode == MWMODE_XOR) {
		vga_getscansegment(getline, x1, y, width);
		for(i=0; i<width; ++i)
			line[i] ^= getline[i];
		lastwidth = -1;
	}
	vga_drawscansegment(line, x1, y, width);

	/*
	 * Non-fishery version is *slow* and doesn't support XOR.
	vga_setegacolor(c);
	vga_drawline(x1, y, x2, y2);
	 */
}

static void
SVGA_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	if(gr_mode == MWMODE_COPY) {
		vga_setegacolor(c);
		vga_drawline(x, y1, x, y2);
	}

	/* slower version required for XOR drawing support*/
	while(y1 <= y2)
		SVGA_drawpixel(psd, x, y1++, c);
}

static void
SVGA_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	while(y1 <= y2)
		SVGA_drawhline(psd, x1, x2, y1++, c);
}

/* only screen-to-screen blit implemented, op ignored*/
/* FIXME*/
static void
SVGA_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	unsigned char line[MAXLINELEN];

	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	if(!(srcpsd->flags & PSF_SCREEN) || !(dstpsd->flags & PSF_SCREEN))
		return;

	while(--h >= 0) {
		vga_getscansegment(line, srcx, srcy, w);
		vga_drawscansegment(line, dstx, dsty, w);
		++dsty;
		++srcy;
	}
}

static int fade = 100;
/* experimental palette animation*/
void
setfadelevel(PSD psd, int f)
{
	int 		i;
	extern MWPALENTRY gr_palette[256];
	MWPALENTRY local_palette[256];

	if(psd->pixtype != MWPF_PALETTE)
		return;

	fade = f;
	if(fade > 100)
		fade = 100;
	for(i=0; i<256; ++i) {

		local_palette[i].r = (gr_palette[i].r * fade / 100);
		local_palette[i].g = (gr_palette[i].g * fade / 100);
		local_palette[i].b = (gr_palette[i].b * fade / 100);
	}
   SVGA_setpalette( psd, 0,256,local_palette );
}
