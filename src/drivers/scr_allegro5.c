/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com> 
 *
 * Copyright (c) 1999 Victor Rogachev <rogach@sut.ru>
 *
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
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro.h>

#define logfile

/* specific grxlib driver entry points*/
static PSD  allegro_open(PSD psd);
static void allegro_close(PSD psd);
static void allegro_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void allegro_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
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

extern ALLEGRO_DISPLAY *display;
ALLEGRO_BITMAP *display_bitmap, *scrmem;
ALLEGRO_LOCKED_REGION *locked_region;
int lock_flags = ALLEGRO_LOCK_READWRITE;
int zoomfactor=1;

#ifdef logfile
FILE *dateihandle, *fopen();
#endif

/*
**	Open graphics
*/
static PSD
allegro_open(PSD psd)
{
	PSUBDRIVER subdriver;

#ifdef logfile
dateihandle = fopen("test.log","w");
#endif

#if _ANDROID_
scrmem = al_create_bitmap(al_get_bitmap_width(al_get_backbuffer(display)),al_get_bitmap_height(al_get_backbuffer(display)));
al_set_target_bitmap(scrmem);
al_clear_to_color(al_map_rgb_f(0x0, 0, 0)); //black background
//calc zoom factor being a multiple of 0.2 (f.e. 6) for nice looking zoom - assume letter orientation of Android device
zoomfactor=al_get_bitmap_width(al_get_backbuffer(display))/SCREEN_WIDTH; //2575/640 =4
#endif

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

	psd->xres = psd->xvirtres = avwidth; //GrScreenX();
	psd->yres = psd->yvirtres = avheight; //GrScreenY();
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
	/* free framebuffer memory */
	free(psd->addr);
	//GrSetMode(GR_default_text);
	//set_gfx_mode(GFX_TEXT,640,480,0,0);
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

/* got to read from psd->addr and write with GrPlot()*/

	static int testval=0;
	testval++;
if (psd->pixtype == MWPF_TRUECOLOR332)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				c = addr[x];
				//GrPlot(destx+x, desty+y, c); 
				////_putpixel(screen,destx+x,desty+y,(int)c);
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
				//GrPlot(destx+x, desty+y, c); 
				////_putpixel16(screen,destx+x,desty+y,(int)c);
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
				//GrPlot(destx+x, desty+y, c);
				////_putpixel24(screen,destx+x,desty+y,(int)c);
				addr += 3;
			}
			addr += extra;
		}
	}
else if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)) & (psd->bpp != 8))
	{
/*
psd->addr = fixed pointer to start of entire internal microwindows pixel buffer
destx = logical pixel start horizontal axsis, desty = logical pixel start vertical axsis
width = logical line width, height = number of lines
addr = physical start of pixel block to render passed by microwindows
addr += psd->pitch = physical start of next line of pixel block to render
width*4 (=width <<2) = physical line length of pixel block to render
*/  
#if _ANDROID_
if(!al_is_bitmap_locked(scrmem)) al_lock_bitmap(scrmem, ALLEGRO_PIXEL_FORMAT_ANY, 0);
al_set_target_bitmap(scrmem);
  
	    unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x = x+4) { 			
				if ((addr+y+x*4)>(psd->addr+psd->size)) return; //do not read outside the screen buffer memory area or crash
				 //the android display is created as default by Allegro as 565 (16 bit) therefore al_draw_pixel to do conversion from 32bit
				 al_draw_pixel(destx+(x/4),desty+y,al_map_rgb((unsigned char)addr[x+2],(unsigned char)addr[x+1],(unsigned char)addr[x]));				
			}
			addr += psd->pitch;  
		}
	}
#else
display_bitmap = al_get_backbuffer(display);
al_set_target_bitmap(display_bitmap);
al_set_target_backbuffer(display);

if(!al_is_bitmap_locked(display_bitmap))locked_region = al_lock_bitmap(display_bitmap, ALLEGRO_PIXEL_FORMAT_RGBA_8888, lock_flags);
//if(!al_is_bitmap_locked(display_bitmap))locked_region = al_lock_bitmap(display_bitmap, ALLEGRO_PIXEL_FORMAT_ANY, lock_flags);

	    unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x = x+4) { 			
				if ((addr+y+x*4)>(psd->addr+psd->size)) return; //do not read outside the screen buffer memory area or crash
				al_put_pixel(destx+(x/4),desty+y,al_map_rgb((unsigned char)addr[x+2],(unsigned char)addr[x+1],(unsigned char)addr[x]));
			}
			addr += psd->pitch;  
		}
	}
#endif

else /* MWPF_PALETTE*/
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				MWPIXELVAL c = addr[x];
				//GrPlot(destx+x, desty+y, c); 
				//putpixel(screen,destx+x,desty+y,(int)c);
			}
			addr += psd->pitch;
		}
	}
}
