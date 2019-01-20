/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * SDL2 Mouse Driver
 * based on original SDL port by Georg Potthast
 */
#include <stdio.h>
#include <stdlib.h>
#include "device.h"

#if EMSCRIPTEN
#include <emscripten.h>
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	sdl_Open(MOUSEDEVICE *pmd);
static void 	sdl_Close(void);
static int  	sdl_GetButtonInfo(void);
static void	sdl_GetDefaultAccel(int *pscale,int *pthresh);
static int  	sdl_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	sdl_Poll(void);

MOUSEDEVICE mousedev = {
	sdl_Open,
	sdl_Close,
	sdl_GetButtonInfo,
	sdl_GetDefaultAccel,
	sdl_Read,
	sdl_Poll,
};

int sdl_pollevents(void);

/*
 * Open the mouse
 */
static int
sdl_Open(MOUSEDEVICE *pmd)
{
	return -3;		/* ok, not file descriptor and not null mouse driver*/
}

/*
 * Close the mouse
 */
static void sdl_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
sdl_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

/*
 * Get default mouse acceleration settings
 */
static void
sdl_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/*
 * Mouse poll entry point
 */
static int
sdl_Poll(void)
{
	return (sdl_pollevents() == 1);	/* 1=mouse*/
}

/*
 * Read mouse event.
 * Returns -1 on error, 0 for no mouse event, and 1 on mouse event.
 * This is a non-blocking call.
 */

static int
sdl_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	int xm, ym;
	int buttons = 0;
	SDL_Event event;
	static int lastx, lasty, lastdn;
	extern SCREENDEVICE scrdev;

	if (sdl_pollevents() != 1)		/* 1=mouse*/
		return 0;

	/* handle mouse events*/
	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL)) {
		int state = SDL_GetMouseState(&xm, &ym);
//printf("mouseev %x, %d,%d\n", state, xm, ym);
		if (state & SDL_BUTTON(SDL_BUTTON_LEFT))
    		buttons |= MWBUTTON_L;
		if (state & (SDL_BUTTON(SDL_BUTTON_RIGHT) | SDL_BUTTON(SDL_BUTTON_X1)))
			buttons |= MWBUTTON_R;
		if (state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
			buttons |= MWBUTTON_M;

		if (event.type == SDL_MOUSEWHEEL) {
			if (event.wheel.y < 0)
				buttons |= MWBUTTON_SCROLLDN; /* wheel down*/
			else
				buttons |= MWBUTTON_SCROLLUP; /* wheel up*/
		}
		*dx = xm;
		*dy = ym;
		*dz = 0;
		*bp = buttons;

		return 2;	/* absolute position**/
	}

	/* handle touchpad events FIXME need right mouse button support*/
	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FINGERDOWN, SDL_FINGERMOTION)) {
		if (event.type == SDL_FINGERDOWN) {
			*dx = lastx = (int)(event.tfinger.x * scrdev.xres);
			*dy = lasty = (int)(event.tfinger.y * scrdev.yres);
    			lastdn = MWBUTTON_L;
//printf("mousedn %d,%d\n", lastx, lasty);
			*dz = 0;
			*bp = lastdn;
			return 0;
		}

		if (event.type == SDL_FINGERUP) {
			*dx = lastx = (int)(event.tfinger.x * scrdev.xres);
			*dy = lasty = (int)(event.tfinger.y * scrdev.yres);
			lastdn = 0;
//printf("mouseup %d,%d\n", lastx, lasty);
			*dz = 0;
			*bp = lastdn;
			return 0;	/* absolute position*/
		}

		if (event.type == SDL_FINGERMOTION) {
			if (lastdn == 0)
				return 0;	/* no motion without finger down*/
			*dx = lastx = (int)(event.tfinger.x * scrdev.xres);
			*dy = lasty = (int)(event.tfinger.y * scrdev.yres);
//printf("mousemv %d,%d\n", lastx, lasty);
			*dz = 0;
			*bp = lastdn;

			return 0;	/* absolute position**/
		}
	}
	return 0;		/* no event*/
}
