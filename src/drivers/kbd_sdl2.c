/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * SDL2 Keyboard Driver
 * based on original SDL port by Georg Potthast
 */
#include <string.h>
#include <stdio.h>
#include "device.h"

#if EMSCRIPTEN
#include <emscripten.h>
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

static int  sdl_Open(KBDDEVICE *pkd);
static void sdl_Close(void);
static void sdl_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  sdl_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  sdl_Poll(void);

KBDDEVICE kbddev = {
	sdl_Open,
	sdl_Close,
	sdl_GetModifierInfo,
	sdl_Read,
	sdl_Poll	
};

int sdl_pollevents(void);

static MWKEYMOD save_modifiers;
 
/*
 * Open the keyboard.
 */
static int
sdl_Open(KBDDEVICE *pkd)
{

	return -3;		/* ok, not file descriptor and not null kbd driver*/
}

/*
 * Close the keyboard.
 */
static void
sdl_Close(void)
{
}

/*
 * Keyboard poll entry point
 */
static int
sdl_Poll(void)
{
	return (sdl_pollevents() >= 2);	/* 2=keyboard, 3=quit*/
}

/*
 * Return the possible modifiers for the keyboard.
 */
static void
sdl_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
	   *curmodifiers = save_modifiers;
}

/* convert key code to shift-key code*/
static int
shift_convert(int key)
{
	if (key >= 'a' && key < 127)
		return (key ^ 0x20);			/* upper case*/
	switch (key) {
	case '`':
		return '~';
	case '1':
		return '!';
	case '2':
		return '@';
	case '3':
		return '#';
	case '4':
		return '$';
	case '5':
		return '%';
	case '6':
		return '^';
	case '7':
		return '&';
	case '8':
		return '*';
	case '9':
		return '(';
	case '0':
		return ')';
	case '-':
		return '_';
	case '=':
		return '+';
	case '[':
		return '{';
	case ']':
		return '}';
	case '\\':
		return '|';
	case ';':
		return ':';
	case '\'':
		return '"';
	case ',':
		return '<';
	case '.':
		return '>';
	case '/':
		return '?';
	}
	return key;
}

/*
 * This reads a keystroke event, and the current state of the modifier keys (ALT, SHIFT, etc). 
 * Returns -1 on error, 0 if no data is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
sdl_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	int mwkey, m;
	SDL_Scancode sc;
	SDL_Keymod mod;
	SDL_Event event;

	if (sdl_pollevents() < 2)		/* 2=keyboard, 3=quit*/
		return 0;

	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_TEXTINPUT)) { 
		switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			sc = event.key.keysym.scancode;
			mwkey = event.key.keysym.sym;
			mod = SDL_GetModState();
//printf("key %x,%x %x = %x\n", mwkey, sc, mod, SDL_GetKeyFromScancode(sc));
			m = 0;
			if (mod & (KMOD_SHIFT|KMOD_CAPS)) {
				m |= MWKMOD_SHIFT;
				mwkey = shift_convert(mwkey);
			}
			if (mod & KMOD_CTRL) {
				m |= MWKMOD_CTRL;
				mwkey &= 0x1f;			/* convert to control char*/
			}
			if (mod & KMOD_ALT)
				m |= MWKMOD_ALT;
			save_modifiers = m;	/* save for GetModifierInfo*/
			*modifiers = m;

			if (mwkey > 127)			/* return only ascii for now*/
				mwkey = 0;
			*kbuf = mwkey;		
			*scancode = sc;
			return (event.type == SDL_KEYDOWN)? 1: 2;

		case SDL_TEXTINPUT:
			mwkey = event.text.text[0];
			return 0;	/* ignore for now*/
		}
	}
	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_QUIT, SDL_QUIT)) {
		//printf("SDL: Quit\n");
		return -2;	/* terminate application*/
	}

	return 0;		/* no event*/
}
