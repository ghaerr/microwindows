/*
 * Copyright (C) 2000 Andrew K. Milton -- akm@theinternet.com.au
 *
 * Basically I stole stuff from all the other drivers, a bit here
 * a bit there. Don't look in here for a "good" example of a driver.
 *
 * FreeBSD VGL screen driver for Microwindows
 */

#include <stdio.h>
#include <machine/console.h>
#include <vgl.h>
#include <signal.h>
#include <osreldate.h>

/*#include "def.h" */
/*#include "hardware.h" */
/*##include "title_gz.h" */

#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 800
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 600
#endif

#ifndef SCREEN_DEPTH
#define SCREEN_DEPTH 8
#endif


#ifndef MWPIXEL_FORMAT
#define MWPIXEL_FORMAT MWPF_PALETTE
#endif

#if ALPHABLEND
/*
 * Alpha lookup tables for 256 color palette systems
 * A 5 bit alpha value is used to keep tables smaller.
 *
 * Two tables are created.  The first, alpha_to_rgb contains 15 bit RGB 
 * values for each alpha value for each color: 32*256 short words.
 * RGB values can then be blended.  The second, rgb_to_palindex contains
 * the closest color (palette index) for each of the 5-bit
 * R, G, and B values: 32*32*32 bytes.
 */
static unsigned short *alpha_to_rgb = NULL;
static unsigned char  *rgb_to_palindex = NULL;
static void init_alpha_lookup(void);
#endif

static SCREENDEVICE savebits;	/* permanent offscreen drawing buffer*/

static PSD  FBSD_open(PSD psd);
static void FBSD_close(PSD psd);
static void FBSD_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void FBSD_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void FBSD_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL FBSD_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void FBSD_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void FBSD_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void FBSD_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,MWPIXELVAL c);

static void FBSD_preselect(PSD psd);
static void FBSD_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,
		      MWCOORD w,MWCOORD h,
		      PSD srcpsd,MWCOORD srcx,MWCOORD srcy, long op);

static void FBSD_blit2(PSD dstpsd,MWCOORD destx,MWCOORD desty,
		      MWCOORD w,MWCOORD h,
		      PSD srcpsd,MWCOORD srcx,MWCOORD srcy, long op);


SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	FBSD_open,
	FBSD_close,
	FBSD_getscreeninfo,
	FBSD_setpalette,
	FBSD_drawpixel,
	FBSD_readpixel,
	FBSD_drawhline,
	FBSD_drawvline,
	FBSD_fillrect,
	gen_fonts,
	FBSD_blit2,
	FBSD_preselect,
	NULL,			/* DrawArea*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	fb_mapmemgc,
	gen_freememgc
};

void FBSD_handle_event(void)
{
    VGLCheckSwitch();
}

static void FBSD_close(PSD psd)
{
    VGLEnd();
}

static PSD FBSD_open(PSD psd)
{
    PSUBDRIVER subdriver;
    int size, linelen;

    if (geteuid() != 0) 
    {
	fprintf(stderr, "The current graphics console architecture ");
	fprintf(stderr, "only permits super-user to access it, ");
	fprintf(stderr, "therefore you either have to obtain such ");
	fprintf(stderr, "permissions or ask your sysadmin to put ");
	fprintf(stderr, "set-user-id on");
	exit(1);
    }



    if (VGLInit(SW_VESA_CG800x600) != 0) 
    {
	fprintf(stderr, "WARNING! Could not initialise VESA mode. ");
	fprintf(stderr, "Trying to fallback to the VGA 640x480 mode\n");
	perror("microwin");

	if (VGLInit(SW_CG640x480) != 0)
	{
	    fprintf(stderr, "WARNING! Could not initialise VGA mode ");
	    fprintf(stderr, "either. Please check your kernel.\n");
	    perror("microwin");
	    return NULL;

	}
    }

    psd -> xres    = psd->xvirtres = VGLDisplay->Xsize;
    psd -> yres    = psd->yvirtres = VGLDisplay->Ysize;
    psd -> linelen = VGLDisplay->Xsize;
    psd -> planes  = 1;
    psd -> pixtype = MWPIXEL_FORMAT;
    psd -> bpp = 8;

/*     switch(psd->pixtype) { */
/*     case MWPF_PALETTE: */
/* 	    psd->bpp = SCREEN_DEPTH; */
/* 	    break; */
/*     case MWPF_TRUECOLOR0888: */
/*     default: */
/* 	    psd->bpp = 32; */
/* 	    break; */
/*     case MWPF_TRUECOLOR888: */
/* 	    psd->bpp = 24; */
/* 	    break; */
/*     case MWPF_TRUECOLOR565: */
/* 	    psd->bpp = 16; */
/* 	    break; */
/*     case MWPF_TRUECOLOR332: */
/* 	    psd->bpp = 8; */
/* 	    break; */
/*     } */


    /*    psd->ncolors = psd->bpp >= 24? (1 << 24): (1 << psd->bpp); */
    psd->ncolors = 256;
    psd->size = 0;
    psd->addr = NULL;
    psd->flags = PSF_SCREEN|PSF_HAVEBLIT;

    savebits=*psd;
    savebits.flags=PSF_MEMORY | PSF_HAVEBLIT;
    /* select a fb subdriver matching our planes and bpp */
    subdriver = select_fb_subdriver(&savebits);
    if (!subdriver) 
    {
	fprintf(stderr,"Subdriver allocation failed!\n");
	return NULL;
    }
    /* calc size and linelen of savebits alloc*/
    GdCalcMemGCAlloc(&savebits, savebits.xvirtres, savebits.yvirtres, 
		     0, 0, &size, &linelen);

    savebits.linelen = linelen;
    savebits.size = size;
    if ((savebits.addr = malloc(size)) == NULL)
    {
	fprintf(stderr,"Malloc for %d Failed!\n",size);
	return NULL;
    }

    set_subdriver(&savebits, subdriver, TRUE);
    return psd;
}

static void FBSD_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
    psi->rows = psd->yvirtres;
    psi->cols = psd->xvirtres;
    psi->planes = psd->planes;
    psi->bpp = psd->bpp;
    psi->ncolors = psd->ncolors;
    psi->pixtype = psd->pixtype;
    psi->fonts = NUMBER_FONTS;
    if(psd->yvirtres > 480) 
    {
	/* SVGA 800x600*/
	psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
    } 
    else if(psd->yvirtres > 350) 
    {
	/* VGA 640x480*/
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
    } 
    else 
    {
	/* EGA 640x350*/
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
    }
}

static void FBSD_setpalette(PSD psd, int first, int count, 
			    MWPALENTRY *pal)
{
    while(first < 256 && count-- > 0) 
    {
	VGLSetPaletteIndex(first++, pal->r>>2, pal->g>>2, pal->b>>2);
	++pal;
    }

}

static void FBSD_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
    VGLSetXY(VGLDisplay, x, y, (unsigned char)c);
    savebits.DrawPixel(&savebits, x, y, c);
}

static MWPIXELVAL FBSD_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
    return savebits.ReadPixel(&savebits,x,y);
/*    return(VGLGetXY(VGLDisplay, x, y)); */
}

static void FBSD_drawhline(PSD psd, MWCOORD x1, MWCOORD x2, 
			   MWCOORD y, MWPIXELVAL c)
{
    VGLLine(VGLDisplay, x1, y, x2, y, c);
    savebits.DrawHorzLine(&savebits,x1,x2,y,c);
}

static void FBSD_drawvline(PSD psd, MWCOORD x, MWCOORD y1, 
			   MWCOORD y2, MWPIXELVAL c)
{
    VGLLine(VGLDisplay, x, y1, x, y2, (unsigned char)c);
    savebits.DrawVertLine(&savebits,x, y1, y2, c);
}

static void FBSD_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
    VGLFilledBox(VGLDisplay,x1, y1, x2, y2, (unsigned char)c);
    savebits.FillRect(&savebits,x1, y1, x2, y2, c);
}

static void FBSD_preselect(PSD psd)
{
    VGLCheckSwitch();
}

/*
 * Really Really stupid blit 
 */
static void FBSD_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,
		      MWCOORD w,MWCOORD h,
		      PSD srcpsd,MWCOORD srcx,MWCOORD srcy, long op)
{

    int x, y;

    if (dstpsd == srcpsd)
    {
	if(dstpsd->flags & PSF_SCREEN)
	{
 	    VGLBitmapCopy(VGLDisplay, srcx, srcy, VGLDisplay, 
 			  destx, desty,  w, h); 
 	    savebits.Blit(&savebits, destx, desty, w, h,
 			  &savebits, srcx, srcy, op); 
	}
	else
	{
	    /* memory to memory blit, use offscreen blitter*/
	    dstpsd->Blit(dstpsd, destx, desty, w, h, srcpsd, srcx, srcy, op);
	}
    }
    else if (dstpsd->flags & PSF_SCREEN)
    {
	VGLBitmap *bitmap;
	
	bitmap=VGLBitmapCreate(MEMBUF , w, h, NULL);
	VGLBitmapAllocateBits(bitmap);
	for (y = 0; y < h; y++) 
	{
	    for (x = 0; x < w; x++) 
	    {
		MWPIXELVAL c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
		VGLSetXY(bitmap, x, y, c);
		/* update screen savebits*/
		savebits.DrawPixel(&savebits, destx+x, desty+y, c);
	    }
	}
	VGLBitmapCopy(bitmap, srcx, srcy, VGLDisplay, 
		      destx, desty,  w, h); 
	VGLBitmapDestroy(bitmap);
    }
    else if (srcpsd->flags & PSF_SCREEN)
    {
	for (y = 0; y < h; y++) 
	{
	    for (x = 0; x < w; x++) 
	    {	
		MWPIXELVAL c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
		dstpsd->DrawPixel(dstpsd, destx+x, desty+y, c);
	    }
	}
    }
}

/* srccopy bitblt*/
static void
FBSD_blit2(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	   PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
#if ALPHABLEND

    unsigned int srcalpha, dstalpha;

    if((op & MWROP_EXTENSION) != MWROP_BLENDCONSTANT)
	goto stdblit;
    srcalpha = op & 0xff;

    /* FIXME create lookup table after palette is stabilized...*/
    if(!rgb_to_palindex || !alpha_to_rgb) 
    {
	init_alpha_lookup();
	if(!rgb_to_palindex || !alpha_to_rgb)
	    goto stdblit;
    }

    /* Create 5 bit alpha value index for 256 color indexing*/

    /* destination alpha is (1 - source) alpha*/
    dstalpha = ((srcalpha>>3) ^ 31) << 8;
    srcalpha = (srcalpha>>3) << 8;

    while(--h >= 0) 
    {
	int	i;
	for(i=0; i<w; ++i) 
	{
	    int c;
	    /* Get source RGB555 value for source alpha value*/
	    unsigned short s = alpha_to_rgb[srcalpha + 
					   srcpsd->ReadPixel(
					       srcpsd,srcx+i,srcy+h)];

	    /* Get destination RGB555 value for dest alpha value*/
	    unsigned short d = alpha_to_rgb[dstalpha +
					   dstpsd->ReadPixel(
					       srcpsd,dstx+i,dsty+h)];

	    /* Add RGB values together and get closest palette index to it*/
	    VGLSetXY(VGLDisplay, dstx+i, dsty+h,
		     c=rgb_to_palindex[s + d]);
	    savebits.DrawPixel(&savebits, dstx+i, dsty+h, c);
	}
    }
    return;
 stdblit:
#endif
    FBSD_blit(dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, op);
}


#if ALPHABLEND
static void init_alpha_lookup(void)
{
    int	i, a;
    int	r, g, b;
    extern MWPALENTRY gr_palette[256];

    if(!alpha_to_rgb)
	alpha_to_rgb = (unsigned short *)malloc(
	    sizeof(unsigned short)*32*256);
    if(!rgb_to_palindex)
	rgb_to_palindex = (unsigned char *)malloc(
	    sizeof(unsigned char)*32*32*32);
    if(!rgb_to_palindex || !alpha_to_rgb)
	return;

    /*
     * Precompute alpha to rgb lookup by premultiplying
     * each palette rgb value by each possible alpha
     * and storing it as RGB555.
     */
    for(i=0; i<256; ++i) {
	MWPALENTRY *p = &gr_palette[i];
	for(a=0; a<32; ++a) {
	    alpha_to_rgb[(a<<8)+i] =
		(((p->r * a / 31)>>3) << 10) |
		(((p->g * a / 31)>>3) << 5) |
		((p->b * a / 31)>>3);
	}
    }

    /*
     * Precompute RGB555 to palette index table by
     * finding the nearest palette index for all RGB555 colors.
     */
    for(r=0; r<32; ++r) {
	for(g=0; g<32; ++g)
	    for(b=0; b<32; ++b)
		rgb_to_palindex[ (r<<10)|(g<<5)|b] =
		    GdFindNearestColor(gr_palette, 256,
				       MWRGB(r<<3, g<<3, b<<3));
    }
}
#endif
