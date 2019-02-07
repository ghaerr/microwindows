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

static int keytrans[] = {
	ALLEGRO_KEY_PAD_0,		MWKEY_KP0,
	ALLEGRO_KEY_PAD_1,		MWKEY_KP1,
	ALLEGRO_KEY_PAD_2,		MWKEY_KP2,
	ALLEGRO_KEY_PAD_3,		MWKEY_KP3,
	ALLEGRO_KEY_PAD_4,		MWKEY_KP4,
	ALLEGRO_KEY_PAD_5,		MWKEY_KP5,
	ALLEGRO_KEY_PAD_6,		MWKEY_KP6,
	ALLEGRO_KEY_PAD_7,		MWKEY_KP7,
	ALLEGRO_KEY_PAD_8,		MWKEY_KP8,
	ALLEGRO_KEY_PAD_9,		MWKEY_KP9,
	ALLEGRO_KEY_F1,			MWKEY_F1,
	ALLEGRO_KEY_F2,			MWKEY_F2,
	ALLEGRO_KEY_F3,			MWKEY_F3,
	ALLEGRO_KEY_F4,			MWKEY_F4,
	ALLEGRO_KEY_F5,			MWKEY_F5,
	ALLEGRO_KEY_F6,			MWKEY_F6,
	ALLEGRO_KEY_F7,			MWKEY_F7,
	ALLEGRO_KEY_F8,			MWKEY_F8,
	ALLEGRO_KEY_F9,			MWKEY_F9,
	ALLEGRO_KEY_F10,		MWKEY_F10,
	ALLEGRO_KEY_F11,		MWKEY_F11,
	ALLEGRO_KEY_F12,		MWKEY_F12,
	ALLEGRO_KEY_INSERT,		MWKEY_INSERT,
	ALLEGRO_KEY_DELETE,		MWKEY_DELETE,
	ALLEGRO_KEY_HOME,		MWKEY_HOME,
	ALLEGRO_KEY_END,		MWKEY_END,
	ALLEGRO_KEY_PGUP,		MWKEY_PAGEUP,
	ALLEGRO_KEY_PGDN,		MWKEY_PAGEDOWN,
	ALLEGRO_KEY_LEFT,		MWKEY_LEFT,
	ALLEGRO_KEY_RIGHT,		MWKEY_RIGHT,
	ALLEGRO_KEY_UP,			MWKEY_UP,
	ALLEGRO_KEY_DOWN,		MWKEY_DOWN,
	ALLEGRO_KEY_PAD_SLASH,	MWKEY_KP_DIVIDE,
	ALLEGRO_KEY_PAD_ASTERISK,MWKEY_KP_MULTIPLY,
	ALLEGRO_KEY_PAD_MINUS,	MWKEY_KP_MINUS,
	ALLEGRO_KEY_PAD_PLUS,	MWKEY_KP_PLUS,
	ALLEGRO_KEY_PAD_DELETE,	MWKEY_DELETE,
	ALLEGRO_KEY_PAD_ENTER,	MWKEY_KP_ENTER,
	ALLEGRO_KEY_PRINTSCREEN,MWKEY_PRINT,
	ALLEGRO_KEY_PAUSE,		MWKEY_QUIT,
	ALLEGRO_KEY_PAD_EQUALS,	MWKEY_KP_EQUALS,
	ALLEGRO_KEY_SELECT,		MWKEY_SELECTDOWN,
	ALLEGRO_KEY_LSHIFT,		MWKEY_LSHIFT,
	ALLEGRO_KEY_RSHIFT,		MWKEY_RSHIFT,
	ALLEGRO_KEY_LCTRL,		MWKEY_LCTRL,
	ALLEGRO_KEY_RCTRL,		MWKEY_RCTRL,
	ALLEGRO_KEY_ALT,		MWKEY_LALT,
	ALLEGRO_KEY_ALTGR,		MWKEY_ALTGR,
	ALLEGRO_KEY_LWIN,		MWKEY_LMETA,
	ALLEGRO_KEY_RWIN,		MWKEY_RMETA,
	ALLEGRO_KEY_MENU,		MWKEY_MENU,
	ALLEGRO_KEY_SCROLLLOCK,	MWKEY_SCROLLOCK,
	ALLEGRO_KEY_NUMLOCK,	MWKEY_NUMLOCK,
	ALLEGRO_KEY_CAPSLOCK,	MWKEY_CAPSLOCK,
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
	return 0;
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
	int k;
	static int mwkey,scanvalue;

	if (al_get_next_event(allegro_scrqueue, &event))
		if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
			return KBD_QUIT;	/* terminate application*/

	if (al_get_next_event(allegro_kbdqueue, &event))
	{
//printf("keytype %d unicode %d, scan %d\n", event.type, event.keyboard.unichar, event.keyboard.keycode);
		switch (event.type) {
		case ALLEGRO_EVENT_KEY_DOWN:
			/* handle shift/ctrl/alt modifiers specially*/
			if (event.keyboard.keycode < ALLEGRO_KEY_MODIFIERS)
				return KBD_NODATA;			/* other keys handled in KEY_CHAR*/
			mwkey = fnkey_convert(event.keyboard.keycode);
			break;

		case ALLEGRO_EVENT_KEY_UP:
			/* check if function or modifier key*/
			k = fnkey_convert(event.keyboard.keycode);
			if (k)							/* yes, process it*/
				mwkey = k;
			/* else use last mwkey and scanvalue, as keyboard.unichar not reported*/
			break;

		case ALLEGRO_EVENT_KEY_CHAR:
			/* if not unicode key, check for fnkey processing*/
			if (event.keyboard.unichar == 0)
				mwkey = fnkey_convert(event.keyboard.keycode);
			else
				mwkey = event.keyboard.unichar & 0xff;	/* handle non-function key*/
			scanvalue = event.keyboard.keycode;
		}
		if (!mwkey)
			return KBD_NODATA;

		*modifiers = 0;
		al_get_keyboard_state(&kbdstate);
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_SHIFT)) *modifiers |= MWKMOD_SHIFT;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_CTRL))  *modifiers |= MWKMOD_CTRL;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_ALT))   *modifiers |= MWKMOD_ALT;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_ALTGR)) *modifiers |= MWKMOD_ALT;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_LWIN))  *modifiers |= MWKMOD_META;
		if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_RWIN))  *modifiers |= MWKMOD_META;
		save_modifiers = *modifiers;

		*kbuf = mwkey;
		*scancode = scanvalue;
		return (event.type == ALLEGRO_EVENT_KEY_UP)?  KBD_KEYRELEASE: KBD_KEYPRESS;
	}
	return KBD_NODATA;
}
/* vim: set ts=4 */
