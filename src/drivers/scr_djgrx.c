/*
 * Screen Driver using DJGPP & GRX  Library
 *
 *  For only GRX lib 
 *
 *  adapted to version 0.93 by Georg Potthast
 *
 */
//#define MWPF_RGB	   0	/* pseudo, convert from packed 32 bit RGB*/
//#define MWPF_PIXELVAL	   1	/* pseudo, no convert from packed PIXELVAL*/
//#define MWPF_PALETTE	   2	/* pixel is packed 8 bits 1, 4 or 8 pal index*/
//#define MWPF_TRUECOLOR888  4	/* pixel is packed 24 bits R/G/B RGB truecolor*/
//#define MWPF_TRUECOLOR565  5	/* pixel is packed 16 bits 5/6/5 RGB truecolor*/
//#define MWPF_TRUECOLOR555  6	/* pixel is packed 16 bits 5/5/5 RGB truecolor*/
//#define MWPF_TRUECOLOR332  7	/* pixel is packed  8 bits 3/3/2 RGB truecolor*/
//#define MWPF_TRUECOLOR8888 8	/* pixel is packed 32 bits A/R/G/B ARGB truecolor with alpha */
//#define MWPF_TRUECOLOR0888 8	/* deprecated*/
//#define MWPF_TRUECOLOR233  9	/* pixel is packed  8 bits 2/3/3 BGR truecolor*/
//#define MWPF_HWPIXELVAL   10	/* pseudo, no convert, pixels are in hw format*/
//#define MWPF_TRUECOLORABGR 11	/* pixel is packed

#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#include <grx20.h>

/* specific grxlib driver entry points*/
static PSD  DJGR_open(PSD psd);
static void DJGR_close(PSD psd);
static void DJGR_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void DJGR_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void DJGR_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 800
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 600
#endif

#ifndef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE MWPF_TRUECOLOR565
#endif

SUBDRIVER subdriver;

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	DJGR_open,
	DJGR_close,
	DJGR_setpalette,       
	DJGR_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,  /*gen_setportrait,        */
	DJGR_update,		/* Update*/
	NULL				/* PreSelect*/
};

/*
**	Open graphics
*/
static PSD
DJGR_open(PSD psd)
{
	PSUBDRIVER subdriver;

	GrVideoMode	*md_info;
	
	int vwidth,vheight,vbpp;
	
	vwidth = SCREEN_WIDTH;
	vheight = SCREEN_HEIGHT;
	if(SCREEN_PIXTYPE == MWPF_TRUECOLOR8888) {
		vbpp=32;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR888) {
		vbpp=24;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR565)  {
		vbpp=16;
	} else {
		vbpp=8; //palette
	}

    GrSetMode(GR_width_height_bpp_graphics,vwidth,vheight,vbpp);

    md_info = (GrVideoMode *) GrCurrentVideoMode();

	psd->xres = psd->xvirtres = GrScreenX();
	psd->yres = psd->yvirtres = GrScreenY();
	psd->planes = 1;
	psd->bpp = md_info->bpp;
	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_ADDRMALLOC;
	/* Calculate the correct size and linelen here */
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp,
		&psd->size, &psd->pitch);

    if(psd->bpp == 32) {
		psd->pixtype = MWPF_TRUECOLOR8888;	
	} else if(psd->bpp == 16) {
		psd->pixtype = MWPF_TRUECOLOR565; 
	} else if(psd->bpp == 24)  {
		psd->pixtype = MWPF_TRUECOLOR888;
	} else {
		psd->pixtype = MWPF_PALETTE;
	}
		  
  psd->portrait = MWPORTRAIT_NONE;
  psd->data_format = set_data_format(psd);

  /*
   * set and initialize subdriver into screen driver
   * psd->size is calculated by subdriver init
   */
  subdriver = select_fb_subdriver(psd);
  
  psd->orgsubdriver = subdriver;

  set_subdriver(psd, subdriver);

  if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;

  return psd;

}

/*
**	Close graphics
*/
static void
DJGR_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);
	GrSetMode(GR_default_text);
}

/*
**	Get Screen Info
*/
static void
DJGR_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);

	if(scrdev.yvirtres > 600) {
		/* SVGA 1024x768*/
		psi->xdpcm = 42;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 42;	/* assumes screen height of 18 cm*/        
	} else if(scrdev.yvirtres > 480) {
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
	int i;
	for(i=first; i < (first+count); i++) {
		//will set to all black screen if no valid palette data passed
		GrSetColor(i, pal->r, pal->g, pal->b);
	}
}

/*
**	Blit
*/
static void
DJGR_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

/*
typedef unsigned char *		ADDR8;
typedef unsigned short *	ADDR16;
typedef uint32_t *			ADDR32;
*/

	MWCOORD x,y;
	MWPIXELVAL c;

/* got to read from psd->addr and write with GrPlot()*/

	//if (!((width == 1) || (height == 1))) return;
	//printf("U: %d %d %d %d ",destx,desty,width,height);

if (psd->pixtype == MWPF_TRUECOLOR332)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				c = addr[x];
				GrPlot(destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		}
	}
else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*2; x++) {
				MWPIXELVAL c = ((unsigned short *)addr)[x]; 
				GrPlot(destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		}
	}
else if (psd->pixtype == MWPF_TRUECOLOR888)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		unsigned int extra = psd->pitch - width * 3;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*3; x++) {
				MWPIXELVAL c = RGB2PIXEL888(addr[2], addr[1], addr[0]);
				GrPlot(destx+x, desty+y, c);
				addr += 3;
			}
			addr += extra;
		}
	}
else if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)) & (psd->bpp != 8))
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x++) {				
				MWPIXELVAL c = ((unsigned short *)addr)[x]; //MWPIXELVAL=uint32_t				
				GrPlot(destx+x, desty+y, c);
	    	}
			addr += psd->pitch;
		}
	}
else /* MWPF_PALETTE*/
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				MWPIXELVAL c = addr[x];
				GrPlot(destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		}
	}

}
