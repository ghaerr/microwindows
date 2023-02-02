/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * SDL2 Screen Driver
 * based on original SDL port by Georg Potthast
 */
#include <stdio.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"
#include <SDL2/SDL.h>

static PSD  sdl_open(PSD psd);
static void sdl_close(PSD psd);
static void sdl_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void sdl_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h);
static int  sdl_preselect(PSD psd);

static int sdl_setup(PSD psd);
int sdl_pollevents(void);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	sdl_open,
	sdl_close,
	sdl_setpalette,       
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	sdl_update,
	sdl_preselect
};

static MWCOORD upminX, upminY, upmaxX, upmaxY;	/* sdl_preselect and sdl_update*/

static SDL_Window *sdlWindow;
static SDL_Renderer *sdlRenderer;
static SDL_Texture *sdlTexture;
#ifndef SDL_ZOOM
#define SDL_ZOOM	1.0			/* normally set in config file*/
#endif
float sdlZoom = SDL_ZOOM;

/*
 * init sdl subsystem, return < 0 on error
 */
static int
sdl_setup(PSD psd)
{
	int	pixelformat;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
		EPRINTF("Can't initialize SDL\n");
		return -1;
	}

	sdlWindow = SDL_CreateWindow("Microwindows SDL",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			psd->xres*sdlZoom, psd->yres*sdlZoom,
#ifdef __EMSCRIPTEN__
			SDL_WINDOW_ALLOW_HIGHDPI);
#else
			SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
#endif
	if (!sdlWindow) {
		EPRINTF("SDL: Can't create window\n");
		return -1;
	}
#if 0
	SDL_Surface *screen = SDL_GetWindowSurface(sdlWindow);
	EPRINTF("SDL pixel format %0x, type %0x\n", screen->format->format,
		SDL_PIXELTYPE(screen->format->format));
#else
	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, 0);
	if (!sdlRenderer) {
		EPRINTF("SDL: Can't create renderer\n");
		return -1;
	}
	/* 
	 * The config file SCREEN_PIXTYPE is used to set the SDL texture pixel format
	 * to match the Microwindows framebuffer format, which eliminates pixel conversions.
	 */
	switch (psd->pixtype) {
	case MWPF_TRUECOLORARGB:
		pixelformat = SDL_PIXELFORMAT_ARGB8888;
		break;
	case MWPF_TRUECOLORABGR:
		pixelformat = SDL_PIXELFORMAT_ABGR8888;
		break;
	case MWPF_TRUECOLOR888:
		pixelformat = SDL_PIXELFORMAT_RGB24;
		break;
	case MWPF_TRUECOLOR565:
		pixelformat = SDL_PIXELFORMAT_RGB565;
		break;
	case MWPF_TRUECOLOR555:
		pixelformat = SDL_PIXELFORMAT_RGB555;
		break;
	case MWPF_TRUECOLOR332:
		pixelformat = SDL_PIXELFORMAT_RGB332;
		break;
	case MWPF_PALETTE:
		pixelformat = SDL_PIXELFORMAT_INDEX8;
		break;
	default:
		EPRINTF("SDL: Unsupported pixel format %d\n", psd->pixtype);
		return -1;
	}
	sdlTexture = SDL_CreateTexture(sdlRenderer, pixelformat, SDL_TEXTUREACCESS_STREAMING,
							psd->xres, psd->yres);
	if (!sdlTexture) {
		EPRINTF("SDL: Can't create texture\n");
		return -1;
	}
#endif

	/* setup zoom*/
	SDL_RenderSetLogicalSize(sdlRenderer, psd->xres, psd->yres);
	SDL_RenderSetScale(sdlRenderer, sdlZoom, sdlZoom);

	//SDL_StartTextInput();
  	SDL_ShowCursor(SDL_DISABLE);	/* hide SDL cursor*/

	SDL_PumpEvents();			/* SDL bug: must call before output or black window overwrite*/

	return 0;
}

/* return nonzero if event available*/
int
sdl_pollevents(void)
{
	SDL_Event event;

  	if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
		if (event.type >= SDL_MOUSEMOTION && event.type <= SDL_MOUSEWHEEL)
			return 1;
		if (event.type >= SDL_FINGERDOWN && event.type <= SDL_FINGERMOTION)
			return 1;
		if (event.type >= SDL_KEYDOWN && event.type <= SDL_TEXTINPUT)
			return 2;
		if (event.type == SDL_QUIT)
			return 3;

		/* dump event*/
  		SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
		//DPRINTF("SDL: dumping event %x\n", event.type);
	}
	SDL_PumpEvents();

	return 0;
}

/*
 * Open graphics
 */
static PSD
sdl_open(PSD psd)
{
	/* init psd and allocate framebuffer*/
	int flags = PSF_SCREEN | PSF_ADDRMALLOC | PSF_DELAYUPDATE | PSF_CANTBLOCK;

	if (!gen_initpsd(psd, MWPIXEL_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, flags))
		return NULL;

	/* initialize SDL subsystem*/
	if (sdl_setup(psd) < 0)
		return NULL;	/* error*/

	return psd;
}

/*
 * Close graphics
 */
static void
sdl_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);

	SDL_Quit();
}

/*
 * Set Palette
 */
static void
sdl_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

/* update SDL from Microwindows framebuffer*/
static void
sdl_draw(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
//printf("update %d,%d %d,%d\n", x, y, width, height);
#if 0
	/* tell SDL we're going to write to window surface*/
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	/* copy from Microwindows framebuffer to SDL*/
	copy_framebuffer(psd, x, y, width, height, screen->pixels, screen->pitch);

	/* flush buffer*/
	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
	SDL_UpdateWindowSurface(sdlWindow);
#else
	/* set region to update*/
	SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = width;
	r.h = height;

	unsigned char *pixels = psd->addr + y * psd->pitch + x * (psd->bpp >> 3);
	SDL_UpdateTexture(sdlTexture, &r, pixels, psd->pitch);

	/* copy texture to display*/
	SDL_SetRenderDrawColor(sdlRenderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
#endif
}

/* called before select(), returns # pending events*/
static int
sdl_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region to SDL server*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX >= 0 || upmaxY >= 0)) {
		sdl_draw(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);

		/* reset update region*/
		upminX = upminY = MAX_MWCOORD;
		upmaxX = upmaxY = MIN_MWCOORD;
	}

	/* return nonzero if SDL event available*/
	return sdl_pollevents();
}

static void
sdl_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = MWMIN(x, upminX);
			upminY = MWMIN(y, upminY);
			upmaxX = MWMAX(upmaxX, x+width-1);
			upmaxY = MWMAX(upmaxY, y+height-1);
	} else
		sdl_draw(psd, x, y, width, height);
}
