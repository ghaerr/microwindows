/*
 * Georg Potthast 2015
 *
 * Allegro Keyboard Driver
 * 
 * if TRANSLATE_ESCAPE_SEQUENCES is set in device.h, then we
 * hard-decode function keys for Linux console and KDE konsole.
 not tested with allegro yet.
 */
#include <string.h>
#include <stdio.h>
#include "device.h"
/* Windows GUI applications start with a WinMain() entry point, rather than the standard main() 
entry point. Allegro is configured to build GUI applications by default and to do some magic in 
order to make a regular main() work with them, but you have to help it out a bit by writing 
END_OF_MAIN() right after your main() function. If you don't want to do that, you can just 
include winalleg.h and write a WinMain() function. Note that this magic may bring about conflicts 
with a few programs using direct calls to Win32 API functions; for these programs, the regular 
WinMain() is required and the magic must be disabled by defining the preprocessor symbol 
ALLEGRO_NO_MAGIC_MAIN before including Allegro headers.

If you want to build a console application using Allegro, you have to define the preprocessor 
symbol ALLEGRO_USE_CONSOLE before including Allegro headers; it will instruct the library to use 
console features and also to disable the special processing of the main() function described above. 
*/
#define ALLEGRO_USE_CONSOLE
#include <allegro.h>

#define CTRL(x)	  ((x) & 0x1f)

extern int escape_quits;

static int  Alleg_Open(KBDDEVICE *pkd);
static void Alleg_Close(void);
static void Alleg_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  Alleg_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  Alleg_Poll(void);

KBDDEVICE kbddev = {
	Alleg_Open,
	Alleg_Close,
	Alleg_GetModifierInfo,
	Alleg_Read,
	Alleg_Poll
};

int closedownflag;

/*
 * Open the keyboard.
 * This is real simple, we just use a special file handle
 * that allows non-blocking I/O, and put the terminal into
 * character mode.
 */
static int
Alleg_Open(KBDDEVICE *pkd)
{
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
Alleg_Close(void)
{
}

static int
Alleg_Poll(void)
{
    if (closedownflag) return 1;
    
	if ((!keypressed())) 
	  return 0;
	else
	  return 1;
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
Alleg_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
	   *curmodifiers = 0;
	   if (key_shifts & KB_SHIFT_FLAG) *curmodifiers |= MWKMOD_SHIFT;
	   if (key_shifts & KB_CTRL_FLAG) *curmodifiers |= MWKMOD_CTRL;
	   if (key_shifts & KB_ALT_FLAG) *curmodifiers |= MWKMOD_ALT;   
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the modifier keys (ALT, SHIFT, etc).  Returns -1 on error, 0 if no data
 * is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
Alleg_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{

	uint16_t cc;			/* characters read */
	MWKEY mwkey;
	MWSCANCODE scanvalue;

    if (closedownflag) return -2; /* special case ESC - terminate application*/

	if (keypressed()) {
	cc = readkey();
	mwkey = cc & 0xFF;
	scanvalue = (MWSCANCODE)cc>>8; 
    } else {
	*kbuf = 0;		
	*scancode = 0;
    return 0; //no key pressed
    }

   *modifiers = 0;
   if (key_shifts & KB_SHIFT_FLAG) *modifiers |= MWKMOD_SHIFT;
   if (key_shifts & KB_CTRL_FLAG) *modifiers |= MWKMOD_CTRL;
   if (key_shifts & KB_ALT_FLAG) *modifiers |= MWKMOD_ALT;   

	*kbuf = mwkey;		
	*scancode = scanvalue;
    if(*kbuf == 0x1b)	return -2; /* special case ESC - terminate application*/
	return 1;		/* keypress*/
}


