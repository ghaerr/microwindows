/*
 * Allegro/Android Keyboard Driver
 *
 * Written Georg Potthast 2015
 * Updated to new driver format Greg Haerr 2019
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
#include <allegro5/allegro.h>

static int  allegro_Open(KBDDEVICE *pkd);
static void allegro_Close(void);
static void allegro_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  allegro_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  allegro_Poll(void);

KBDDEVICE kbddev = {
	allegro_Open,
	allegro_Close,
	allegro_GetModifierInfo,
	allegro_Read,
	allegro_Poll
};

extern ALLEGRO_EVENT_QUEUE *allegro_kbdqueue;
extern ALLEGRO_EVENT_QUEUE *allegro_scrqueue;
static MWKEYMOD save_modifiers;

/*
 * Open the keyboard.
 */
static int
allegro_Open(KBDDEVICE *pkd)
{
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the keyboard.
 */
static void
allegro_Close(void)
{
}

/*
 * Keyboard poll entry point.
 */
static int
allegro_Poll(void)
{
	ALLEGRO_EVENT event;

	if (al_peek_next_event(allegro_scrqueue, &event))
		return 1;

	if (!al_is_keyboard_installed())
		return 0;

	if (al_peek_next_event(allegro_kbdqueue, &event))
		return 1;

	return 0;
}

/*
 * Return the possible modifiers for the keyboard.
 */
static void
allegro_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
	   *curmodifiers = save_modifiers;
}

/*
 * Reads a keystroke and the current state of the modifier keys (ALT, SHIFT, etc).
 * This is a non-blocking call.
 */
static int
allegro_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	ALLEGRO_EVENT event;
	ALLEGRO_KEYBOARD_STATE kbdstate;
	static int mwkey,scanvalue;

	if (al_get_next_event(allegro_scrqueue, &event))
		if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
			return KBD_QUIT;	/* terminate application*/

	if (al_get_next_event(allegro_kbdqueue, &event))
	{
		/* there are further key events */
		if (event.type == ALLEGRO_EVENT_KEY_CHAR)
		{
			mwkey = event.keyboard.unichar & 0xff;
			scanvalue = event.keyboard.keycode;
			al_get_keyboard_state(&kbdstate);
		}
		else
		{
			if (event.type == ALLEGRO_EVENT_KEY_UP)
			{
				*kbuf = mwkey;
				*scancode = scanvalue;
				*modifiers = 0;
				return KBD_KEYRELEASE; /* keyrelease received*/
			}
			else return KBD_NODATA;
		}

		*modifiers = 0;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_SHIFT)) *modifiers |= MWKMOD_SHIFT;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_CTRL))  *modifiers |= MWKMOD_CTRL;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_ALT))   *modifiers |= MWKMOD_ALT;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_ALTGR)) *modifiers |= MWKMOD_ALT;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_LWIN))  *modifiers |= MWKMOD_META;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_RWIN))  *modifiers |= MWKMOD_META;
		save_modifiers = *modifiers;

		*kbuf = mwkey;
		*scancode = scanvalue;
		return KBD_KEYPRESS;		/* keypress received*/
	}
	return KBD_NODATA;
}
