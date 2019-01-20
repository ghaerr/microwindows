/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com> 
 *
 *
 *  SDL1 driver by Georg Potthast 2016
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

#include <SDL/SDL.h>

//#define logfile

/* specific grxlib driver entry points*/
static PSD  sdl_open(PSD psd);
static void sdl_close(PSD psd);
static void sdl_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void sdl_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void sdl_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

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
	sdl_open,
	sdl_close,
	sdl_setpalette,       
	sdl_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,  /*gen_setportrait,        */
	sdl_update,		/* Update*/
	NULL				/* PreSelect*/
};


#ifdef logfile
FILE *dateihandle, *fopen();
#endif

extern SDL_Surface *screen;
int unlock_flag;
Uint32 colour;

/*
**	Open graphics
*/
static PSD
sdl_open(PSD psd)
{
	PSUBDRIVER subdriver;

#ifdef logfile
dateihandle = fopen("test.log","w");
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
sdl_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);
	//GrSetMode(GR_default_text);
	//set_gfx_mode(GFX_TEXT,640,480,0,0);
	SDL_Quit();
}

/*
**	Get Screen Info
*/
static void
sdl_getscreeninfo(PSD psd,PMWSCREENINFO psi)
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
sdl_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
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
sdl_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
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
#if 0
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
#endif
#if 1
else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	

if (unlock_flag==0){
  if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
}
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);

		for (y = 0; y < height; y++) {
			for (x = 0; x < width+2; x++) { 
			         //if ((addr+y+x)>(psd->addr+psd->size)) return; //do not read outside the screen buffer memory area or crash
				 unsigned long redcolor   =  ((((unsigned short *)addr)[x]) & 0xF800) >>11;
				 unsigned long greencolor =  ((((unsigned short *)addr)[x]) & 0x07E0) >>5;
				 unsigned long bluecolor  =  ((((unsigned short *)addr)[x]) & 0x001F);
				 redcolor   = redcolor * 255 /31;
				 greencolor = greencolor * 255 /63;
				 bluecolor  = bluecolor * 255 /31;
				 //al_draw_pixel(destx+x,desty+y,al_map_rgb(redcolor,greencolor,bluecolor));
				colour=SDL_MapRGBA(screen->format, redcolor,greencolor,bluecolor,0xFF);
				*((Uint32*)screen->pixels + ((desty * psd->pitch + (destx <<1)) + y*psd->pitch + (x))/2) = colour; //divide by 2 since cast to Uint32*			
				 
			}
			addr += psd->pitch;
		}
	}

else if (psd->pixtype == MWPF_TRUECOLOR888)
	{

if (unlock_flag==0){
  if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
}

		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		unsigned int extra = psd->pitch - width * 3;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*3; x = x+3) { 			
				 //al_draw_pixel(destx+(x/3),desty+y,al_map_rgb((unsigned char)addr[x+2],(unsigned char)addr[x+1],(unsigned char)addr[x]));				
				colour=SDL_MapRGBA(screen->format, (unsigned char)addr[x+2],(unsigned char)addr[x+1],(unsigned char)addr[x],0xFF);
				*((Uint32*)screen->pixels + ((desty * psd->pitch + (destx *3)) + y*psd->pitch + (x))/3) = colour; //divide by 3 since cast to Uint32*			
			}
			addr += psd->pitch;
		}
	}
#endif
else if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)) & (psd->bpp != 8))
	{

#if 0
static int counter;
counter++; //let flicker
fprintf(stdout,"writing pixel width:%d,%d\n",width,counter); fflush(stdout);
if (counter >10) counter=0;
#endif



if (unlock_flag==0){
  if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
}
	    unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x = x+4) { 			
				if ((addr+y+x*4)>(psd->addr+psd->size)) return; //do not read outside the screen buffer memory area or crash
				colour=SDL_MapRGBA(screen->format, (unsigned char)addr[x+2],(unsigned char)addr[x+1],(unsigned char)addr[x], (unsigned char)addr[x+3]);
				*((Uint32*)screen->pixels + ((desty * psd->pitch + (destx << 2)) + y*psd->pitch + (x))/4) = colour; //divide by 4 since cast to Uint32*			
				//al_draw_pixel(destx+(x/4),desty+y,al_map_rgb((unsigned char)addr[x+2],(unsigned char)addr[x+1],(unsigned char)addr[x]));				
			}
			addr += psd->pitch;  
		}
		unlock_flag=1;
	}

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
