/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * SDL2 Mouse Driver
 * based on original SDL port by Georg Potthast
 */
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include <SDL2/SDL.h>

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	mousdl_Open(MOUSEDEVICE *pmd);
static void 	mousdl_Close(void);
static int  	mousdl_GetButtonInfo(void);
static void		mousdl_GetDefaultAccel(int *pscale,int *pthresh);
static int  	mousdl_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	mousdl_Poll(void);

MOUSEDEVICE mousedev = {
	mousdl_Open,
	mousdl_Close,
	mousdl_GetButtonInfo,
	mousdl_GetDefaultAccel,
	mousdl_Read,
	mousdl_Poll,
};

int sdl_pollevents(void);
extern float sdlZoom;

/*
 * Open the mouse
 */
static int
mousdl_Open(MOUSEDEVICE *pmd)
{
	return DRIVER_OKNOTFILEDESC;		/* ok, not file descriptor and not null mouse driver*/
}

/*
 * Close the mouse
 */
static void mousdl_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
mousdl_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

/*
 * Get default mouse acceleration settings
 */
static void
mousdl_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/*
 * Mouse poll entry point
 */
static int
mousdl_Poll(void)
{
	return (sdl_pollevents() == 1);	/* 1=mouse*/
}

/*
 * Read mouse event.
 * Returns MOUSE_NODATA or MOUSE_ABSPOS
 * This is a non-blocking call.
 */

static int
mousdl_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	int xm, ym;
	int buttons = 0;
	SDL_Event event;
	static int lastx, lasty, lastdn;
	extern SCREENDEVICE scrdev;

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
		*dx = (int)xm / sdlZoom;
		*dy = (int)ym / sdlZoom;
		*dz = 0;
		*bp = buttons;

		return MOUSE_ABSPOS;
	}

	/* handle touchpad events FIXME need right mouse button support*/
	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FINGERDOWN, SDL_FINGERMOTION)) {
		if (event.type == SDL_FINGERDOWN) {
			*dx = lastx = (int)(event.tfinger.x * scrdev.xres / sdlZoom);
			*dy = lasty = (int)(event.tfinger.y * scrdev.yres / sdlZoom);
    			lastdn = MWBUTTON_L;
//printf("mousedn %d,%d\n", lastx, lasty);
			*dz = 0;
			*bp = lastdn;
			return MOUSE_NODATA;
		}

		if (event.type == SDL_FINGERUP) {
			*dx = lastx = (int)(event.tfinger.x * scrdev.xres / sdlZoom);
			*dy = lasty = (int)(event.tfinger.y * scrdev.yres / sdlZoom);
			lastdn = 0;
//printf("mouseup %d,%d\n", lastx, lasty);
			*dz = 0;
			*bp = lastdn;
			return MOUSE_NODATA;		/* absolute position*/
		}

		if (event.type == SDL_FINGERMOTION) {
			if (lastdn == 0)
				return MOUSE_NODATA;	/* no motion without finger down*/
			*dx = lastx = (int)(event.tfinger.x * scrdev.xres / sdlZoom);
			*dy = lasty = (int)(event.tfinger.y * scrdev.yres / sdlZoom);
//printf("mousemv %d,%d\n", lastx, lasty);
			*dz = 0;
			*bp = lastdn;

			return MOUSE_NODATA;
		}
	}
	return MOUSE_NODATA;
}
