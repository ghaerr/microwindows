/*
 * Copyright (c) 1999, 2003 Greg Haerr <greg@censoft.com>
 * Georg Potthast 2016
 *
 * SDL2 Keyboard Driver
 * 
 * if TRANSLATE_ESCAPE_SEQUENCES is set in device.h, then we
 * hard-decode function keys for Linux console and KDE konsole.
 not tested with allegro yet.
 */
#include <string.h>
#include <stdio.h>
#include "device.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#define CTRL(x)	  ((x) & 0x1f)

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 768
#endif

extern int escape_quits;
SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;
SDL_Event event;
Uint16 mod;

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

static int closedown;
static MWKEYMOD save_modifiers;
 
/*
 * Open the keyboard.
 * This is real simple, we just use a special file handle
 * that allows non-blocking I/O, and put the terminal into
 * character mode.
 */
/*
 * keyboard driver is called first, so initialize sdl here 
 */
static int
sdl_Open(KBDDEVICE *pkd)
{

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS); 

  sdlWindow = SDL_CreateWindow("Microwindows",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
	    0);

  atexit(SDL_Quit);
  SDL_StartTextInput();

  screen = SDL_GetWindowSurface(sdlWindow);

  return 1; //ok    
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
sdl_Close(void)
{
  SDL_Quit();
}

/*
**
*/
static int
sdl_Poll(void)
{
  SDL_PumpEvents();
  if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) return 1; //read event in read function

  if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_QUIT, SDL_QUIT)) {
    closedown=1;
    return 1; //i.e. received the "closedown" key
  }

  return 0;   //no event that we are interested in
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
sdl_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
	   *curmodifiers = save_modifiers;
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the modifier keys (ALT, SHIFT, etc).  Returns -1 on error, 0 if no data
 * is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
sdl_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{

  static int mwkey;
  static SDL_Scancode sdlScancode;
  char text[120];

  if (closedown == 1) {SDL_Quit(); return -2;} /* special case ESC - terminate application*/
  //read key event
  if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_TEXTINPUT )) { 
    if (event.type == SDL_KEYDOWN){
      sdlScancode = event.key.keysym.scancode;
      mwkey = (char) SDL_GetKeyFromScancode(sdlScancode);
      if (mwkey == SDLK_ESCAPE) {SDL_Quit(); return -2;} /* special case ESC - terminate application*/
      //continue after else section if now - i.e. receive Textinput event
  } else if (event.type == SDL_TEXTINPUT) {
      mwkey = event.text.text[ 0 ];
      //printf("Textinput: %c\n",mwkey);
  } else if (event.type == SDL_KEYUP) {
            *kbuf = mwkey;		
            *scancode = sdlScancode;
            return 2; //key released    
  }
    
   *modifiers = 0;
   mod = SDL_GetModState();
   if( mod & KMOD_SHIFT) *modifiers |= MWKMOD_SHIFT;
   if( mod & KMOD_CTRL ) *modifiers |= MWKMOD_CTRL;
   if( mod & KMOD_LALT ) *modifiers |= MWKMOD_ALT;
   if( mod & KMOD_RALT ) *modifiers |= MWKMOD_ALTGR;
   save_modifiers=*modifiers;
   
    *kbuf = mwkey;		
    *scancode = sdlScancode;

    return 1;		/* keypress received*/
    
  } 
  return 0; //if no event received

} //end of sdl_read


