/*
 * Microwindows IR remote control keyboard driver
 * Written by Koninklijke Philips Electronics N.V.
 *
 * Based on the Microwindows /dev/tty console scancode
 * keyboard driver for Linux
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * 
 * Portions contributed by Koninklijke Philips Electronics N.V.
 * These portions are Copyright 2002-2003 Koninklijke Philips Electronics
 * N.V.  All Rights Reserved.  These portions are licensed under the
 * terms of the Mozilla Public License, version 1.1, or, at your
 * option, the GNU General Public License version 2.0.  Please see
 * the file "ChangeLog" for documentation regarding these
 * contributions.
 *
 *
 * This driver uses the LIRC protocol, which is a de-facto standard
 * for IR remote controls on Linux.
 *
 * There are several parts to this driver:
 * - Low-level I/O and parsing functions, in mwlirc.c and mwlirc.h
 * - Generic functions, at the top of this file.
 * - Several drivers, one for each supported remote control, near the
 *   bottom of this file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "mwtypes.h"
#include "device.h"
#include "mwlirc.h"

static int LIRC_Open(KBDDEVICE * pkd);
static void LIRC_Close(void);
static void LIRC_GetModifierInfo(MWKEYMOD * modifiers,
				 MWKEYMOD * curmodifiers);
static int LIRC_Read(MWKEY * kbuf, MWKEYMOD * modifiers,
		     MWSCANCODE * scancode);

static void LIRC_UpdateKeyState(MWBOOL isDown, MWKEY mwkey);

static int remoteKeyboardHandler(mwlirc_keystroke * event, MWKEY * kbuf,
				 MWSCANCODE * pscancode);
static int remoteControlHandler(mwlirc_keystroke * event, MWKEY * kbuf,
				MWSCANCODE * pscancode);


KBDDEVICE kbddev = {
	LIRC_Open,
	LIRC_Close,
	LIRC_GetModifierInfo,
	LIRC_Read,
	NULL
};


/* If a driver wants to press & release a button at once, it should set
 * these to the release event, and then just return the press event.
 */
static MWKEY LIRC_pending_keyrelease_ch = 0;
static MWSCANCODE LIRC_pending_keyrelease_scan = 0;

/* The global keymod state */
static MWKEYMOD LIRC_keymod = 0;


typedef int (*remoteHandler_t) (mwlirc_keystroke * event, MWKEY * kbuf,
				MWSCANCODE * pscancode);

typedef struct remoteDriver_t_
{
	const char *name;
	remoteHandler_t handler;
}
remoteDriver_t;

/*
 * List of remote control names.
 */
static remoteDriver_t remoteDrivers[] = {
	{"remote", remoteControlHandler},
	{"keyboard", remoteKeyboardHandler},
	{NULL, NULL}
};

static int LIRC_fd = -1;


/*
 * Open the remote control.
 */
static int
LIRC_Open(KBDDEVICE * pkd)
{
	int fd = mwlirc_init(MWLIRC_NONBLOCK);

	LIRC_pending_keyrelease_ch = 0;
	LIRC_pending_keyrelease_scan = 0;
	LIRC_keymod = 0;

	LIRC_fd = fd;
	return fd;
}


/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
LIRC_Close(void)
{
	LIRC_fd = -1;
	mwlirc_close();
}


/* Update the internal keyboard state */
static void
LIRC_UpdateKeyState(MWBOOL isDown, MWKEY mwkey)
{
	MWKEYMOD changebit = 0;

	switch (mwkey) {
	case MWKEY_NUMLOCK:
		if (!isDown)
			LIRC_keymod ^= MWKMOD_NUM;
		return;
	case MWKEY_CAPSLOCK:
		if (!isDown)
			LIRC_keymod ^= MWKMOD_CAPS;
		return;
	case MWKEY_LCTRL:
		changebit = MWKMOD_LCTRL;
		break;
	case MWKEY_RCTRL:
		changebit = MWKMOD_RCTRL;
		break;
	case MWKEY_LSHIFT:
		changebit = MWKMOD_LSHIFT;
		break;
	case MWKEY_RSHIFT:
		changebit = MWKMOD_RSHIFT;
		break;
	case MWKEY_LALT:
		changebit = MWKMOD_LALT;
		break;
	case MWKEY_RALT:
		changebit = MWKMOD_RALT;
		break;
	case MWKEY_LMETA:
		changebit = MWKMOD_LMETA;
		break;
	case MWKEY_RMETA:
		changebit = MWKMOD_RMETA;
		break;
	case MWKEY_ALTGR:
		changebit = MWKMOD_ALTGR;
		break;
	default:
		return;
	}

	if (isDown)
		LIRC_keymod |= changebit;
	else
		LIRC_keymod &= ~changebit;
}


/*
 * Return the possible modifiers and current modifiers for the keyboard.
 */
static void
LIRC_GetModifierInfo(MWKEYMOD * modifiers, MWKEYMOD * curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_NUM | MWKMOD_CAPS
			| MWKMOD_LCTRL
			| MWKMOD_LSHIFT | MWKMOD_RSHIFT
			| MWKMOD_LALT | MWKMOD_RALT
			| MWKMOD_LMETA | MWKMOD_RMETA;
	if (curmodifiers)
		*curmodifiers = LIRC_keymod;
}


/*
 * This reads one keystroke from the keyboard, and the current state of
 * the modifier keys (ALT, SHIFT, etc).  Returns -1 on error, 0 if no data
 * is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
LIRC_Read(MWKEY * kbuf, MWKEYMOD * modifiers, MWSCANCODE * pscancode)
{
	mwlirc_keystroke event[1];
	int err;
	const remoteDriver_t *driver;
	int result = 0;

	if (LIRC_pending_keyrelease_ch != 0) {
		*kbuf = LIRC_pending_keyrelease_ch;
		*modifiers = LIRC_keymod;
		*pscancode = LIRC_pending_keyrelease_scan;
		LIRC_pending_keyrelease_ch = 0;
		LIRC_pending_keyrelease_scan = 0;
		return 2;
	}

	do {
		/*printf("Calling mwlirc_read_keystroke()\n");*/
		err = mwlirc_read_keystroke_norepeat(event);
		/*printf("Called  mwlirc_read_keystroke() - %d\n", err);*/
		if (err != 0) {
			return (err == MWLIRC_ERROR_AGAIN) ? 0 : -1;
		}

		driver = remoteDrivers;
		while ((driver->name)
		       && (0 != strcmp(driver->name, event->rc))) {
			driver++;
		}
		if (driver->name) {
			/*printf("LIRC_Read() - processing remote '%s', key '%s'\n", event->rc, event->name);*/
			result = driver->handler(event, kbuf, pscancode);
		} else {
			printf("LIRC_Read() - Unrecognized remote control '%s'. (Key '%s')\n", event->rc, event->name);
		}
	}
	while (result == 0);

	*modifiers = LIRC_keymod;
	return (result == 3 ? 0 : result);
}

/* ************************************************************************ */
/* * End of generic IR code                                               * */
/* ************************************************************************ */



/* ************************************************************************ */
/* * Start of IR keyboard driver                                          * */
/* ************************************************************************ */
/*
 * To use this driver, you must follow the following conventions
 * when allocating key names in LIRC.
 *
 * Key names always end in one of these symbols:
 *     '\' - Keypressed.
 *     '^' - Keyreleased.
 * The rest of the symbol must be one of the strings defined in keyboardKeys,
 * or one of these specials:
 *     "fn"  - "Fn" key
 *
 * The name of this remote is "keyboard"
 */

typedef struct keyboardKey_t_
{
	MWBOOL isDown;
	const char *name;
	const MWKEY normal;
	const MWKEY shift;
	const MWKEY num;
	const MWKEY fn;
}
keyboardKey_t;

static keyboardKey_t keyboardKeys[] = {

// Row 1
	{0, "ESCAPE", MWKEY_ESCAPE, MWKEY_ESCAPE, MWKEY_ESCAPE, 0},
	{0, "F1", MWKEY_F1, MWKEY_F1, MWKEY_F1, MWKEY_F11},
	{0, "F2", MWKEY_F2, MWKEY_F2, MWKEY_F2, MWKEY_F12},
	{0, "F3", MWKEY_F3, MWKEY_F3, MWKEY_F3, 0},
	{0, "F4", MWKEY_F4, MWKEY_F4, MWKEY_F4, 0},
	{0, "F5", MWKEY_F5, MWKEY_F5, MWKEY_F5, 0},
	{0, "F6", MWKEY_F6, MWKEY_F6, MWKEY_F6, 0},
	{0, "F7", MWKEY_F7, MWKEY_F7, MWKEY_F7, 0},
	{0, "F8", MWKEY_F8, MWKEY_F8, MWKEY_F8, 0},
	{0, "F9", MWKEY_F9, MWKEY_F9, MWKEY_F9, 0},
	{0, "F10", MWKEY_F10, MWKEY_F10, MWKEY_F10, 0},
	{0, "NUMLOCK", MWKEY_NUMLOCK, MWKEY_NUMLOCK, MWKEY_NUMLOCK,
	 MWKEY_NUMLOCK},
	{0, "PRINT", MWKEY_PRINT, MWKEY_PRINT, MWKEY_PRINT, 0},
	{0, "SCROLLOCK", MWKEY_SCROLLOCK, MWKEY_SCROLLOCK, MWKEY_SCROLLOCK,
	 0},
	{0, "PAUSE", MWKEY_PAUSE, MWKEY_PAUSE, MWKEY_PAUSE, 0},
	{0, "INSERT", MWKEY_INSERT, MWKEY_INSERT, MWKEY_INSERT, 0},
/*	{ 0, "PAUSE",     MWKEY_PAUSE,     MWKEY_PAUSE,     MWKEY_PAUSE,     0 }, */

// Row 2
	{0, "1", '1', '!', 0, 0},
	{0, "2", '2', '@', 0, 0},
	{0, "3", '3', '#', 0, 0},
	{0, "4", '4', '$', 0, 0},
	{0, "5", '5', '%', 0, 0},
	{0, "6", '6', '^', 0, 0},
	{0, "7", '7', '&', '7', '7'},
	{0, "8", '8', '*', '8', '8'},
	{0, "9", '9', '(', '9', '9'},
	{0, "0", '0', ')', '*', '*'},
	{0, "-", '-', '_', 0, 0},
	{0, "=", '=', '+', 0, 0},
	{0, "BACKSPACE", MWKEY_BACKSPACE, MWKEY_BACKSPACE, MWKEY_BACKSPACE,
	 0},

// Row 3
	{0, "TAB", MWKEY_TAB, MWKEY_TAB, MWKEY_TAB, 0},
	{0, "q", 'q', 'Q', 0, 0},
	{0, "w", 'w', 'W', 0, 0},
	{0, "e", 'e', 'E', 0, 0},
	{0, "r", 'r', 'R', 0, 0},
	{0, "t", 't', 'T', 0, 0},
	{0, "y", 'y', 'Y', 0, 0},
	{0, "u", 'u', 'U', '4', '4'},
	{0, "i", 'i', 'I', '5', '5'},
	{0, "o", 'o', 'O', '6', '6'},
	{0, "p", 'p', 'P', '-', '-'},
	{0, "[", '[', '{', 0, 0},
	{0, "]", ']', '}', 0, 0},
	{0, "\\", '\\', '\\', 0, 0},

// Row 4
	{0, "CAPSLOCK", MWKEY_CAPSLOCK, MWKEY_CAPSLOCK, MWKEY_CAPSLOCK,
	 MWKEY_CAPSLOCK},
	{0, "a", 'a', 'A', 0, 0},
	{0, "s", 's', 'S', 0, 0},
	{0, "d", 'd', 'D', 0, 0},
	{0, "f", 'f', 'F', 0, 0},
	{0, "g", 'g', 'G', 0, 0},
	{0, "h", 'h', 'H', 0, 0},
	{0, "j", 'j', 'J', '1', '1'},
	{0, "k", 'k', 'K', '2', '2'},
	{0, "l", 'l', 'L', '3', '3'},
	{0, ";", ';', ':', '+', '+'},
	{0, "'", '\'', '"', 0, 0},
	{0, "ENTER", MWKEY_ENTER, MWKEY_ENTER, MWKEY_ENTER, 0},

// Row 5
	{0, "LSHIFT", MWKEY_LSHIFT, MWKEY_LSHIFT, MWKEY_LSHIFT, MWKEY_LSHIFT},
	{0, "z", 'z', 'Z', 0, 0},
	{0, "x", 'x', 'X', 0, 0},
	{0, "c", 'c', 'C', 0, 0},
	{0, "v", 'v', 'V', 0, 0},
	{0, "b", 'b', 'B', 0, 0},
	{0, "n", 'n', 'N', 0, 0},
	{0, "m", 'm', 'M', '0', '0'},
	{0, ",", ',', '<', 0, 0},
	{0, ".", '.', '>', 0, 0},
	{0, "RSHIFT", MWKEY_RSHIFT, MWKEY_RSHIFT, MWKEY_RSHIFT, MWKEY_RSHIFT},
	{0, "UP", MWKEY_UP, MWKEY_UP, MWKEY_UP, MWKEY_PAGEUP},
	{0, "/", '/', '?', 0, 0},

// Row 6
	{0, "LCTRL", MWKEY_LCTRL, MWKEY_LCTRL, MWKEY_LCTRL, MWKEY_LCTRL},
	{0, "LMETA", MWKEY_LMETA, MWKEY_LMETA, MWKEY_LMETA, MWKEY_LMETA},
	{0, "LALT", MWKEY_LALT, MWKEY_LALT, MWKEY_LALT, MWKEY_LALT},
	{0, "SPACE", ' ', ' ', ' ', 0},
	{0, "`", '`', '~', 0, 0},
	{0, "RALT", MWKEY_RALT, MWKEY_RALT, MWKEY_RALT, MWKEY_RALT},
	{0, "RMETA", MWKEY_RMETA, MWKEY_RMETA, MWKEY_RMETA, MWKEY_RMETA},
	{0, "LEFT", MWKEY_LEFT, MWKEY_LEFT, MWKEY_LEFT, MWKEY_HOME},
	{0, "DOWN", MWKEY_DOWN, MWKEY_DOWN, MWKEY_DOWN, MWKEY_PAGEDOWN},
	{0, "RIGHT", MWKEY_RIGHT, MWKEY_RIGHT, MWKEY_RIGHT, MWKEY_END},

// End of table marker
	{0, NULL, 0, 0, 0, 0}
};

static int LIRC_fn = 0;

static int
remoteKeyboardHandler(mwlirc_keystroke * event, MWKEY * kbuf,
		      MWSCANCODE * pscancode)
{
	MWKEY ch;
	MWSCANCODE scan;
	keyboardKey_t *k;
	MWBOOL isDown;

	unsigned len = strlen(event->name);
	if (len < 2) {
		printf("LIRC_Read() - Invalid key '%s' - too short.  (Must be at least 2 characters!)\n", event->name);
		return 0;
	}
	len--;
	switch (event->name[len]) {
	case '\\':
		isDown = 1;
		break;
	case '^':
		isDown = 0;
		break;
	default:
		printf("LIRC_Read() - Invalid key '%s' - no up/down (^ or \\) indicator\n", event->name);
		return 0;
	}
	event->name[len] = '\0';

	if (0 == strcmp(event->name, "fn")) {
		LIRC_fn = isDown;
		return 0;	/* Silent key */
	}

	k = keyboardKeys;

	while (0 != strcmp(k->name, event->name)) {
		k++;
		if (NULL == k->name) {
			printf("LIRC_Read() - Unrecognized key '%s'\n",
			       event->name);
			return 0;	/* Unrecognized - treat as 'No data' */
		}
	}

	if (isDown == k->isDown)
		return 0;	/* No change */

	k->isDown = isDown;

	scan = k->normal;
	ch = ((LIRC_fn) ? k->fn :
	      ((LIRC_keymod & MWKMOD_NUM) ? k->num :
	       ((LIRC_keymod & MWKMOD_SHIFT) ? k->shift : (k->normal))));

	if (ch == 0)
		return 0;	/* Ignore key */

	if (LIRC_keymod & MWKMOD_CAPS) {
		/* If it's a letter, flip the case. */
		if ((ch >= 'a') && (ch <= 'z')) {
			ch -= 'a' - 'A';
		} else if ((ch >= 'A') && (ch <= 'Z')) {
			ch += 'a' - 'A';
		}
	}

	LIRC_UpdateKeyState(isDown, ch);

	/*printf("LIRC_Read() - Got key '%s', char '%c', code 0x%x\n", k->name, ch, scan);*/
	*kbuf = ch;
	*pscancode = scan;

	return (isDown ? 1 /* press */ : 2 /* release */ );
}

/* ************************************************************************ */
/* * End of IR keyboard driver                                            * */
/* ************************************************************************ */



/* ************************************************************************ */
/* * Start of IR remote driver                                            * */
/* ************************************************************************ */
/*
 * To use this driver, you must follow the following conventions
 * when allocating key names in LIRC.
 *
 * Key names can be:
 * - A single ASCII character
 * - One of the strings defined in remoteKeys.
 *
 * The name of this remote is "remote"
 */

typedef struct remoteKey_t_
{
	int code;
	const char *name;
}
remoteKey_t;

static const remoteKey_t remoteKeys[] = {
	{MWKEY_ACCEPT,              "ACCEPT"},
	{MWKEY_BACKSPACE,           "BACKSPACE"},
	{MWKEY_BALANCE_LEFT,        "BALANCE_LEFT"},
	{MWKEY_BALANCE_RIGHT,       "BALANCE_RIGHT"},
	{MWKEY_BASS_BOOST_DOWN,     "BASS_BOOST_DOWN"},
	{MWKEY_BASS_BOOST_UP,       "BASS_BOOST_UP"},
	{MWKEY_CANCEL,              "CANCEL"},
	{MWKEY_CHANNEL_DOWN,        "CHANNEL_DOWN"},
	{MWKEY_CHANNEL_UP,          "CHANNEL_UP"},
	{MWKEY_CLEAR_FAVORITE_0,    "CLEAR_FAVORITE_0"},
	{MWKEY_CLEAR_FAVORITE_1,    "CLEAR_FAVORITE_1"},
	{MWKEY_CLEAR_FAVORITE_2,    "CLEAR_FAVORITE_2"},
	{MWKEY_CLEAR_FAVORITE_3,    "CLEAR_FAVORITE_3"},
	{MWKEY_COLORED_KEY_0,       "COLORED_KEY_0"},
	{MWKEY_COLORED_KEY_1,       "COLORED_KEY_1"},
	{MWKEY_COLORED_KEY_2,       "COLORED_KEY_2"},
	{MWKEY_COLORED_KEY_3,       "COLORED_KEY_3"},
	{MWKEY_COLORED_KEY_4,       "COLORED_KEY_4"},
	{MWKEY_COLORED_KEY_5,       "COLORED_KEY_5"},
	{MWKEY_DELETE,              "DELETE"},
	{MWKEY_DOWN,                "DOWN"},
	{MWKEY_DIMMER,              "DIMMER"},
	{MWKEY_DISPLAY_SWAP,        "DISPLAY_SWAP"},
	{MWKEY_EJECT_TOGGLE,        "EJECT_TOGGLE"},
	{MWKEY_END,                 "END"},
	{MWKEY_ENTER,               "ENTER"},
	{MWKEY_ESCAPE,              "ESCAPE"},
	{MWKEY_F1,                  "F1"},
	{MWKEY_F2,                  "F2"},
	{MWKEY_F3,                  "F3"},
	{MWKEY_F4,                  "F4"},
	{MWKEY_F5,                  "F5"},
	{MWKEY_F6,                  "F6"},
	{MWKEY_F7,                  "F7"},
	{MWKEY_F8,                  "F8"},
	{MWKEY_F9,                  "F9"},
	{MWKEY_F10,                 "F10"},
	{MWKEY_F11,                 "F11"},
	{MWKEY_F12,                 "F12"},
	{MWKEY_FADER_FRONT,         "FADER_FRONT"},
	{MWKEY_FADER_REAR,          "FADER_REAR"},
	{MWKEY_FAST_FWD,            "FAST_FWD"},
	{MWKEY_GO_TO_END,           "GO_TO_END"},
	{MWKEY_GO_TO_START,         "GO_TO_START"},
	{MWKEY_GUIDE,               "GUIDE"},
	{MWKEY_HOME,                "HOME"},
	{MWKEY_INFO,                "INFO"},
//	{MWKEY_INSERT,              "INSERT"},
	{MWKEY_LEFT,                "LEFT"},
	{MWKEY_MUTE,                "MUTE"},
	{MWKEY_PAGEDOWN,            "PAGEDOWN"},
	{MWKEY_PAGEUP,              "PAGEUP"},
	{MWKEY_PAUSE,               "PAUSE"},
	{MWKEY_PINP_TOGGLE,         "PINP_TOGGLE"},
	{MWKEY_PLAY,                "PLAY"},
	{MWKEY_PLAY_SPEED_DOWN,     "PLAY_SPEED_DOWN"},
	{MWKEY_PLAY_SPEED_RESET,    "PLAY_SPEED_RESET"},
	{MWKEY_PLAY_SPEED_UP,       "PLAY_SPEED_UP"},
	{MWKEY_POWER,               "POWER"},
	{MWKEY_PRINT,               "PRINT"},
	{MWKEY_RANDOM_TOGGLE,       "RANDOM_TOGGLE"},
	{MWKEY_RECALL_FAVORITE_0,   "RECALL_FAVORITE_0"},
	{MWKEY_RECALL_FAVORITE_1,   "RECALL_FAVORITE_1"},
	{MWKEY_RECALL_FAVORITE_2,   "RECALL_FAVORITE_2"},
	{MWKEY_RECALL_FAVORITE_3,   "RECALL_FAVORITE_3"},
	{MWKEY_RECORD,              "RECORD"},
	{MWKEY_RECORD_SPEED_NEXT,   "RECORD_SPEED_NEXT"},
	{MWKEY_REWIND,              "REWIND"},
	{MWKEY_RIGHT,               "RIGHT"},
	{MWKEY_SCAN_CHANNELS_TOGGLE,"SCAN_CHANNELS_TOGGLE"},
	{MWKEY_SCREEN_MODE_NEXT,    "SCREEN_MODE_NEXT"},
//	{ MWKEY_SCROLLOCK,          "SCROLLOCK"},
	{' ',                       "SPACE"},
	{MWKEY_SPLIT_SCREEN_TOGGLE, "SPLIT_SCREEN_TOGGLE"},
	{MWKEY_STOP,                "STOP"},
	{MWKEY_STORE_FAVORITE_0,    "STORE_FAVORITE_0"},
	{MWKEY_STORE_FAVORITE_1,    "STORE_FAVORITE_1"},
	{MWKEY_STORE_FAVORITE_2,    "STORE_FAVORITE_2"},
	{MWKEY_STORE_FAVORITE_3,    "STORE_FAVORITE_3"},
	{MWKEY_SUBTITLE,            "SUBTITLE"},
	{MWKEY_SURROUND_MODE_NEXT,  "SURROUND_MODE_NEXT"},
	{MWKEY_TAB,                 "TAB"},
	{MWKEY_TELETEXT,            "TELETEXT"},
	{MWKEY_TRACK_NEXT,          "TRACK_NEXT"},
	{MWKEY_TRACK_PREV,          "TRACK_PREV"},
	{MWKEY_UP,                  "UP"},
	{MWKEY_VIDEO_MODE_NEXT,     "VIDEO_MODE_NEXT"},
	{MWKEY_VOLUME_DOWN,         "VOLUME_DOWN"},
	{MWKEY_VOLUME_UP,           "VOLUME_UP"},
	{MWKEY_WINK,                "WINK"},
	{0, NULL}
};

static int
remoteControlHandler(mwlirc_keystroke * event, MWKEY * kbuf,
		     MWSCANCODE * pscancode)
{
	MWKEY ch;
	unsigned len = strlen(event->name);

	if (len == 1) {
		/* Should be a digit. */
		ch = event->name[0];
		/*printf("LIRC_Read() - Got key '%c', char '%c', code 0x%x\n", ch, ch, ch);*/
	} else {
		/* FIXME: This is a linear search, should use a binary search instead (faster). */
		const remoteKey_t *k = remoteKeys;
		while (0 != strcmp(k->name, event->name)) {
			k++;
			if (NULL == k->name) {
				printf("LIRC_Read() - Unrecognized key '%s'\n", event->name);
				return 0;	/* Unrecognized - treat as 'No data' */
			}
		}
		ch = k->code;
		/*printf("LIRC_Read() - Got key '%s', no char, code 0x%x\n", k->name, ch);*/
	}

	*kbuf = ch;
	*pscancode = ch;

	/* send keypress, queue keyrelease. */
	LIRC_pending_keyrelease_ch = ch;
	LIRC_pending_keyrelease_scan = ch;
	return 1;
}

/* ************************************************************************ */
/* * End of IR remote driver                                              * */
/* ************************************************************************ */

