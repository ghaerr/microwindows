/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com> 
 *
 *
 *  SDL2 driver by Georg Potthast 2016
 *
 */
#include <stdio.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#if EMSCRIPTEN
#include <emscripten.h>
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

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
#define SCREEN_PIXTYPE MWPF_TRUECOLORARGB
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

extern SDL_Window *sdlWindow;
extern SDL_Renderer *sdlRenderer;
extern SDL_Texture *sdlTexture;
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

	if(SCREEN_PIXTYPE == MWPF_TRUECOLORARGB) {
		psd->bpp = 32;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR888) {
		psd->bpp = 24;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR565)  {
		psd->bpp = 16;
	} else {
		psd->bpp = 8;
	}

	psd->xres = psd->xvirtres = SCREEN_WIDTH;
	psd->yres = psd->yvirtres = SCREEN_HEIGHT;
	psd->planes = 1;
	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_ADDRMALLOC;
	/* Calculate the correct size and linelen here */
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp, &psd->size, &psd->pitch);

    if(psd->bpp == 32) {
		psd->pixtype = MWPF_TRUECOLORARGB;	
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

static void
sdl_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	MWCOORD x,y;

//printf("sdl_update %d,%d %d,%d\n", destx, desty, width, height);
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

#if 0
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
else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	
if (unlock_flag==0){ if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen); }

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
if (unlock_flag==0){ if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen); }

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
else if ((MWPIXEL_FORMAT == MWPF_TRUECOLORARGB) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR))
#endif
	{
if (unlock_flag==0) { if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen); }

		// our framebuffer is MWPF_TRUECOLORARGB which is BGRA byte order, and so are SDL screenbits
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			unsigned char *framebuffer = screen->pixels + (desty+y) * screen->pitch + (destx << 2);
			for (x = 0; x < width; x++) { 			
				unsigned long pixel = ((uint32_t *)addr)[x];
				*(uint32_t *)framebuffer = pixel;
				framebuffer += 4;
			}
			addr += psd->pitch;  
		}
		unlock_flag=1;

	}
#if 0
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
#endif  
}

#if 0
static void
update_from_savebits(PSD psd, unsigned int destx, unsigned int desty, int w, int h)
{
	XImage *img;
	unsigned int x, y;
	char *data;

	/* allocate buffer */
	if (x11_depth >= 24)
		data = malloc(w * 4 * h);
	else if (x11_depth > 8)	/* 15, 16 */
		data = malloc(w * 2 * h);
	else			/* 1,2,4,8 */
		data = malloc((w * x11_depth + 7) / 8 * h);

	/* copy from offscreen to screen */
	img = XCreateImage(x11_dpy, x11_vis, x11_depth, ZPixmap, 0, data, w, h, 8, 0);

	/* Use optimized loops for most common framebuffer modes */

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = addr[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = ((ADDR16)addr)[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#elif MWPIXEL_FORMAT == MWPF_TRUECOLOR888
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		unsigned int extra = psd->pitch - w * 3;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = RGB2PIXEL888(addr[2], addr[1], addr[0]);
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
				addr += 3;
			}
			addr += extra;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLORARGB) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = ((ADDR32)addr)[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#else /* MWPF_PALETTE*/
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = addr[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#endif

	XPutImage(x11_dpy, x11_win, x11_gc, img, 0, 0, destx, desty, w, h);
	XDestroyImage(img);
}

/* called before select(), returns # pending events*/
static int
sdl2_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region to X11 server*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX || upmaxY)) {
		update_from_savebits(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);
		upminX = upminY = ~(1 << ((sizeof(int)*8)-1));	// largest positive int
		upmaxX = upmaxY = 0;
	}

	XFlush(x11_dpy);
	return XPending(x11_dpy);
}

static void
sdl2_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = min(x, upminX);
			upminY = min(y, upminY);
			upmaxX = max(upmaxX, x+width-1);
			upmaxY = max(upmaxY, y+height-1);
	} else
		update_from_savebits(psd, x, y, width, height);
}
#endif
