/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Null Keyboard Driver
 */
#include <stdio.h>
#include "device.h"

static int  NUL_Open(KBDDEVICE *pkd);
static void NUL_Close(void);
static void NUL_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  NUL_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  NUL_Poll(void);

KBDDEVICE kbddev = {
	NUL_Open,
	NUL_Close,
	NUL_GetModifierInfo,
	NUL_Read,
	NUL_Poll
};

/*
 * Poll for keyboard events
 */
static int
NUL_Poll(void)
{
	return 0;
}

/*
 * Open the keyboard.
 */
static int
NUL_Open(KBDDEVICE *pkd)
{
	return -2;	/* no kbd*/
}

/*
 * Close the keyboard.
 */
static void
NUL_Close(void)
{
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
NUL_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = 0;		/* no modifiers available */
	if (curmodifiers)
		*curmodifiers = 0;
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the modifier keys (ALT, SHIFT, etc).  Returns -1 on error, 0 if no data
 * is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
NUL_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	return 0;
}
