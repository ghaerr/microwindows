/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * SDL2 Keyboard Driver
 * based on original SDL port by Georg Potthast
 */
#include <string.h>
#include <stdio.h>
#include "device.h"
#include <SDL2/SDL.h>

static int  kbdsdl_Open(KBDDEVICE *pkd);
static void kbdsdl_Close(void);
static void kbdsdl_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  kbdsdl_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  kbdsdl_Poll(void);

KBDDEVICE kbddev = {
	kbdsdl_Open,
	kbdsdl_Close,
	kbdsdl_GetModifierInfo,
	kbdsdl_Read,
	kbdsdl_Poll	
};

int sdl_pollevents(void);

static MWKEYMOD save_modifiers;
 
/*
 * Open the keyboard.
 */
static int
kbdsdl_Open(KBDDEVICE *pkd)
{

	return DRIVER_OKNOTFILEDESC;		/* ok, not file descriptor and not null kbd driver*/
}

/*
 * Close the keyboard.
 */
static void
kbdsdl_Close(void)
{
}

/*
 * Keyboard poll entry point
 */
static int
kbdsdl_Poll(void)
{
	return (sdl_pollevents() >= 2);	/* 2=keyboard, 3=quit*/
}

/*
 * Return the possible modifiers for the keyboard.
 */
static void
kbdsdl_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
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

static int keytrans[] = {
	SDLK_F1,			MWKEY_F1,
	SDLK_F2,			MWKEY_F2,
	SDLK_F3,			MWKEY_F3,
	SDLK_F4,			MWKEY_F4,
	SDLK_F5,			MWKEY_F5,
	SDLK_F6,			MWKEY_F6,
	SDLK_F7,			MWKEY_F7,
	SDLK_F8,			MWKEY_F8,
	SDLK_F9,			MWKEY_F9,
	SDLK_F10,			MWKEY_F10,
	SDLK_F11,			MWKEY_F11,
	SDLK_F12,			MWKEY_F12,
	SDLK_F15,			MWKEY_QUIT,
	SDLK_PRINTSCREEN,	MWKEY_PRINT,
	SDLK_CAPSLOCK,		MWKEY_CAPSLOCK,
	SDLK_SCROLLLOCK,	MWKEY_SCROLLOCK,
	SDLK_PAUSE,			MWKEY_PAUSE,
	SDLK_INSERT,		MWKEY_INSERT,
	SDLK_HOME,			MWKEY_HOME,
	SDLK_PAGEUP,		MWKEY_PAGEUP,
	SDLK_DELETE,		MWKEY_DELETE,
	SDLK_END,			MWKEY_END,
	SDLK_PAGEDOWN,		MWKEY_PAGEDOWN,
	SDLK_RIGHT,			MWKEY_RIGHT,
	SDLK_LEFT,			MWKEY_LEFT,
	SDLK_DOWN,			MWKEY_DOWN,
	SDLK_UP,			MWKEY_UP,
	SDLK_KP_DIVIDE,		MWKEY_KP_DIVIDE,
	SDLK_KP_MULTIPLY,	MWKEY_KP_MULTIPLY,
	SDLK_KP_MINUS,		MWKEY_KP_MINUS,
	SDLK_KP_PLUS,		MWKEY_KP_PLUS,
	SDLK_KP_ENTER,		MWKEY_KP_ENTER,
	SDLK_KP_1,			MWKEY_KP1,
	SDLK_KP_2,			MWKEY_KP2,
	SDLK_KP_3,			MWKEY_KP3,
	SDLK_KP_4,			MWKEY_KP4,
	SDLK_KP_5,			MWKEY_KP5,
	SDLK_KP_6,			MWKEY_KP6,
	SDLK_KP_7,			MWKEY_KP7,
	SDLK_KP_8,			MWKEY_KP8,
	SDLK_KP_9,			MWKEY_KP9,
	SDLK_KP_0,			MWKEY_KP0,
	SDLK_KP_PERIOD,		MWKEY_KP_PERIOD,
	SDLK_KP_EQUALS,		MWKEY_KP_EQUALS,
	SDLK_MENU,			MWKEY_MENU,
	SDLK_SELECT,		MWKEY_SELECTDOWN,
	SDLK_SYSREQ,		MWKEY_SYSREQ,
	SDLK_CANCEL,		MWKEY_CANCEL,
	SDLK_LCTRL,			MWKEY_LCTRL,
	SDLK_LSHIFT,		MWKEY_LSHIFT,
	SDLK_LALT,			MWKEY_LALT,
	SDLK_LGUI,			MWKEY_ALTGR,
	SDLK_RCTRL,			MWKEY_RCTRL,
	SDLK_RSHIFT,		MWKEY_RSHIFT,
	SDLK_RALT,			MWKEY_RALT,
	SDLK_RGUI,			MWKEY_ALTGR,
	SDLK_APP1,			MWKEY_APP1,
	SDLK_APP2,			MWKEY_APP2,
	SDLK_CANCEL,		MWKEY_CANCEL,
	0
};

static int
fnkey_convert(int key)
{
	int *kp = keytrans;

	while (*kp)
	{
		if (key == *kp)
			return *(kp + 1);
		kp += 2;
	}
	return key;
}

/*
 * This reads a keystroke event, and the current state of the modifier keys (ALT, SHIFT, etc). 
 * Returns KBD_NODATA, KBD_QUIT, KBD_KEYPRESS or KBD_KEYRELEASE
 * This is a non-blocking call.
 */
static int
kbdsdl_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	int mwkey, m;
	SDL_Scancode sc;
	SDL_Keymod mod;
	SDL_Event event;

	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_TEXTINPUT)) { 
		switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			sc = event.key.keysym.scancode;
			mwkey = event.key.keysym.sym;
			mod = SDL_GetModState();
//printf("key %x,%x %x = %x\n", mwkey, sc, mod, SDL_GetKeyFromScancode(sc));
			m = 0;
			if (mwkey < 256 && (mod & (KMOD_SHIFT|KMOD_CAPS))) {
				m |= MWKMOD_SHIFT;
				mwkey = shift_convert(mwkey);
			}
			if (mwkey < 256 && (mod & KMOD_CTRL)) {
				m |= MWKMOD_CTRL;
				mwkey &= 0x1f;			/* convert to control char*/
			}
			if (mod & KMOD_ALT)
				m |= MWKMOD_ALT;
			save_modifiers = m;	/* save for GetModifierInfo*/
			*modifiers = m;

			if (mwkey >= 128) {			/* convert function key from SDL To MW*/
				mwkey = fnkey_convert(mwkey);
				if (mwkey == 0)
					return KBD_NODATA;
			}

			*kbuf = mwkey;		
			*scancode = sc;
			return (event.type == SDL_KEYDOWN)? KBD_KEYPRESS: KBD_KEYRELEASE;

		case SDL_TEXTINPUT:
			mwkey = event.text.text[0];
			return KBD_NODATA;			/* ignore for now*/
		}
	}
	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_QUIT, SDL_QUIT)) {
		return KBD_QUIT;				/* terminate application*/
	}

	return KBD_NODATA;
}
