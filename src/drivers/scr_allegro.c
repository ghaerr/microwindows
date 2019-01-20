/*
 * Screen Driver using Allegro 4  Library
 *
 * adapted by Georg Potthast 2015
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
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#define ALLEGRO_USE_CONSOLE
#include <allegro.h> //allegro 4

/* specific allegro lib driver entry points*/
static PSD  allegro_open(PSD psd);
static void allegro_close(PSD psd);
static void allegro_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void allegro_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

#ifndef SCREEN_WIDTH
//#define SCREEN_WIDTH 640 
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
//#define SCREEN_HEIGHT 480 
#define SCREEN_HEIGHT 768
#endif

#ifndef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE MWPF_TRUECOLOR8888
//#define MWPF_TRUECOLOR565 /* pixel is packed 16 bits 5/6/5 RGB truecolor*/
#endif

SUBDRIVER subdriver;

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	allegro_open,
	allegro_close,
	allegro_setpalette,       
	allegro_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,  /*gen_setportrait,        */
	allegro_update,		/* Update*/
	NULL				/* PreSelect*/
};

extern int closedownflag;

PSD psdglobal;

void close_button_proc(void){    
    closedownflag=1;
    return;
}

/*
**	Open graphics
*/
static PSD
allegro_open(PSD psd)
{
	PSUBDRIVER subdriver;

	int avwidth,avheight,avbpp;
	
	avwidth = SCREEN_WIDTH;
	avheight = SCREEN_HEIGHT;
	if(SCREEN_PIXTYPE == MWPF_TRUECOLOR8888) {
		avbpp=32;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR888) {
		avbpp=24;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR565)  {
		avbpp=16;
	} else {
		avbpp=8; //palette
	}
	
	psdglobal=psd;
	
/****************************************/
    allegro_init();
    install_keyboard();
    install_mouse();
    set_close_button_callback(close_button_proc);
	set_color_depth(avbpp);
	
#if __MINGW32__ 
    set_gfx_mode(GFX_GDI,1024,768,0,0); //GDI for Windows
#else
	set_gfx_mode(GFX_XWINDOWS,640,480,0,0);
	//set_gfx_mode(GFX_AUTODETECT,1024,768,0,0);
	//set_gfx_mode(GFX_SAFE,640,480,0,0);
#endif
/****************************************/

	psd->xres = psd->xvirtres = 1024; //avwidth; //GrScreenX();
	psd->yres = psd->yvirtres = 768; //avheight; //GrScreenY();
	psd->planes = 1;
	psd->bpp = avbpp; //md_info->bpp;
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
allegro_close(PSD psd)
{
	/* free buffer memory */
	free(psd->addr);
}

/*
**	Get Screen Info
*/
static void
allegro_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);

	psi->fbdriver = FALSE;	/* not running fb driver, no direct map */

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
allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	int i;
	for(i=first; i < (first+count); i++) {
		//will set to all black screen if no valid palette data passed
		//GrSetColor(i, pal->r, pal->g, pal->b);
	}
}

/*
**	Blit
*/
static void
allegro_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = 0; //psd->xres;
	if (!height)
		height = 0; //psd->yres;

/*
typedef unsigned char *		ADDR8;
typedef unsigned short *	ADDR16;
typedef uint32_t *			ADDR32;
*/

	MWCOORD x,y;
	MWPIXELVAL c;

	static int testval=0;
	testval++;
if (psd->pixtype == MWPF_TRUECOLOR332)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				c = addr[x];
				//GrPlot(destx+x, desty+y, c); 
				_putpixel(screen,destx+x,desty+y,(int)c);
			}
			addr += psd->pitch;
		}
	}
else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*2; x++) {
                uint16_t blue16  =  ((uint16_t *)addr)[x] & 0x001F;
                uint16_t green16 = (((uint16_t *)addr)[x] >>5) & 0x001F;
                uint16_t red16   = (((uint16_t *)addr)[x] >>11) & 0x001F;

				_putpixel16(screen,destx+x,desty+y,makecol16(red16*255,green16*255,blue16*255));
			}
			addr += psd->pitch;
		}
	}
else if (psd->pixtype == MWPF_TRUECOLOR888)
	{
	unsigned char *addr = psd->addr + desty * psd->pitch + (destx *3);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*3; x = x+3) { 			
				if ((addr+y+x*3)>(psd->addr+psd->size)) return; //do not read outside the screen buffer memory area or crash			
                _putpixel24(screen,destx+(x/3),desty+y,makecol24((int)(unsigned char)addr[x+2],(int)(unsigned char)addr[x+1],(int)(unsigned char)addr[x])); 
	    	}
	    	addr += psd->pitch;
		}

	}
else if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)) & (psd->bpp != 8))
	{
	unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x = x+4) { 			
				if ((addr+y+x*4)>(psd->addr+psd->size)) return; //do not read outside the screen buffer memory area or crash
                //_putpixel32(screen,destx+(x/4),desty+y,makecol32((unsigned char *)addr[x+2],(unsigned char *)addr[x+1],(unsigned char *)addr[x])); 
                _putpixel32(screen,destx+(x/4),desty+y,makecol32((int)(unsigned char)addr[x+2],(int)(unsigned char)addr[x+1],(int)(unsigned char)addr[x])); 
	    	}
	    	addr += psd->pitch;
		}
	}
else /* MWPF_PALETTE*/
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				putpixel(screen,destx+x,desty+y,(int)(unsigned char)addr[x]); //untested
			}
			addr += psd->pitch;
		}
	}
}
