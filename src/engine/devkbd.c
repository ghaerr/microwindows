/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Device-independent keyboard routines
 */
#include "device.h"

/**
 * Open the keyboard.
 *
 * @return Fd (>0) on success, -1 on error, -2 if no keyboard.
 */
int
GdOpenKeyboard(void)
{
	int	fd;

	if ((fd = kbddev.Open(&kbddev)) == -1)
		return -1;

	/* possible -2 return means no kbd*/
	return fd;
}

/**
 *
 * Close the keyboard.
 */
void
GdCloseKeyboard(void)
{
	kbddev.Close();
}

/**
 * Return the possible modifiers (shift, control, etc) for the keyboard.
 *
 * modifiers: Recieves bitmask of supported modifiers.
 * curmodifiers: Recieves bitmask of current modifiers.
 */
void
GdGetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	kbddev.GetModifierInfo(modifiers, curmodifiers);
}

/**
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, 1 if keypress, 2 if keyrelease.
 * This is a non-blocking call.  Returns -2 if ESC pressed.
 *
 * @param buf Recieves the key.
 * @param modifiers Recieves the state of the modifiers (shift etc).
 * @param scancode Recieves the scancode of the key.
 * @return -1 on error, 0 if no data is ready,
 * 1 if keypress, 2 if keyrelease.
 */
int
GdReadKeyboard(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	return kbddev.Read(buf, modifiers, scancode);
}
