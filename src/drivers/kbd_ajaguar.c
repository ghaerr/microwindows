/*
 * Copyright (c) 2019 Jean-Paul Mari <djipi.mari@gmail.com>
 *
 * Atari Jaguar Keyboard Driver
 */
#include <stdio.h>
#include "device.h"

static int  kbdAJAGUAR_Open(KBDDEVICE *pkd);
static void kbdAJAGUAR_Close(void);
static void kbdAJAGUAR_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  kbdAJAGUAR_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  kbdAJAGUAR_Poll(void);

KBDDEVICE kbddev __attribute__ ((section (".mwjagdata"))) = {
	kbdAJAGUAR_Open,
	kbdAJAGUAR_Close,
	kbdAJAGUAR_GetModifierInfo,
	kbdAJAGUAR_Read,
	kbdAJAGUAR_Poll
};

/*
 * Poll for keyboard events
 */
static int kbdAJAGUAR_Poll(void)
{
	return 0;
}

/*
 * Open the keyboard.
 */
static int kbdAJAGUAR_Open(KBDDEVICE *pkd)
{
	return DRIVER_OKNULLDEV;	/* ok, no kbd*/
}

/*
 * Close the keyboard.
 */
static void kbdAJAGUAR_Close(void)
{
}

/*
 * Return the possible modifiers for the keyboard.
 */
static void kbdAJAGUAR_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
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
static int kbdAJAGUAR_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	return KBD_NODATA;
}
