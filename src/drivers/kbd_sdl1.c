/*
 * Copyright (c) 1999, 2003 Greg Haerr <greg@censoft.com>
 * Georg Potthast 2016
 *
 * SDL1 Keyboard Driver
 * 
 * if TRANSLATE_ESCAPE_SEQUENCES is set in device.h, then we
 * hard-decode function keys for Linux console and KDE konsole.
 not tested with allegro yet.
 */
#include <string.h>
#include <stdio.h>
#include "device.h"

#include <SDL/SDL.h>

#define CTRL(x)	  ((x) & 0x1f)

extern int escape_quits;
SDL_Surface *screen;
SDL_Event event;
SDLMod mod;

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

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_EVENTTHREAD);
  screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
  atexit(SDL_Quit);
  SDL_EnableUNICODE( 1 );
     
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
  if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, (SDL_EVENTMASK (SDL_KEYDOWN) | SDL_EVENTMASK (SDL_KEYUP)))) return 1; //read event in read function

  if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_QUITMASK)) {
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

  static int mwkey,scanvalue;

  if (closedown == 1) {SDL_Quit(); return -2;} /* special case ESC - terminate application*/
  //read key event
  if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, (SDL_EVENTMASK (SDL_KEYDOWN) | SDL_EVENTMASK (SDL_KEYUP)))) { 
    if (event.type == SDL_KEYDOWN){
      scanvalue = event.key.keysym.scancode;
      /* If the Unicode value is less than 0x80 then the    */
      /* unicode value can be used to get a printable       */
      /* representation of the key, using (char)unicode.    */
      if( event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0 ){
	    mwkey = (char)event.key.keysym.unicode;
      } else{
	    mwkey = (char)event.key.keysym.unicode;
      }
      if (mwkey == SDLK_ESCAPE) {SDL_Quit(); return -2;} /* special case ESC - terminate application*/
      //continue after else section if now
  } else if (event.type == SDL_KEYUP) {
            *kbuf = mwkey;		
            *scancode = scanvalue;
            return 2; //key released    
  }
    
   *modifiers = 0;
#if 0
   if (scanvalue == 303 || scanvalue == 304)  *modifiers |= MWKMOD_SHIFT;
   if (scanvalue == 305 || scanvalue == 306)  *modifiers |= MWKMOD_CTRL;
   if (scanvalue == 308)  *modifiers |= MWKMOD_ALT;
   if (scanvalue == 313)  *modifiers |= MWKMOD_ALTGR;
#endif

   mod = event.key.keysym.mod;
   if( mod & KMOD_SHIFT) *modifiers |= MWKMOD_SHIFT;
   if( mod & KMOD_CTRL ) *modifiers |= MWKMOD_CTRL;
   if( mod & KMOD_LALT ) *modifiers |= MWKMOD_ALT;
   if( mod & KMOD_RALT ) *modifiers |= MWKMOD_ALTGR;
   save_modifiers=*modifiers;
   
    *kbuf = mwkey;		
    *scancode = scanvalue;

    return 1;		/* keypress received*/
    
  } 
  return 0; //if no event received

} //end of sdl_read


