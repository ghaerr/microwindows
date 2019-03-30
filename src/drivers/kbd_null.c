/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Null Keyboard Driver
 */
#include <stdio.h>
#include "device.h"

static int  kbdNUL_Open(KBDDEVICE *pkd);
static void kbdNUL_Close(void);
static void kbdNUL_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  kbdNUL_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  kbdNUL_Poll(void);

KBDDEVICE kbddev = {
	kbdNUL_Open,
	kbdNUL_Close,
	kbdNUL_GetModifierInfo,
	kbdNUL_Read,
	kbdNUL_Poll
};

/*
 * Poll for keyboard events
 */
static int
kbdNUL_Poll(void)
{
	return 0;
}

/*
 * Open the keyboard.
 */
static int
kbdNUL_Open(KBDDEVICE *pkd)
{
	return DRIVER_OKNULLDEV;	/* ok, no kbd*/
}

/*
 * Close the keyboard.
 */
static void
kbdNUL_Close(void)
{
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
kbdNUL_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
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
kbdNUL_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	return KBD_NODATA;
}
