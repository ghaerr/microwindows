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

#ifdef MW_FEATURE_TWO_KEYBOARDS
/**
 * Open the second keyboard.
 *
 * @return Fd (>0) on success, -1 on error, -2 if no keyboard.
 */
int
GdOpenKeyboard2(void)
{
	return kbddev2.Open(&kbddev2);
}
#endif

/**
 * Close the keyboard.
 */
void
GdCloseKeyboard(void)
{
	kbddev.Close();
#ifdef MW_FEATURE_TWO_KEYBOARDS
	kbddev2.Close();
#endif
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
#ifdef MW_FEATURE_TWO_KEYBOARDS
	MWKEYMOD modifiers2;
	MWKEYMOD curmodifiers2;
#endif
	kbddev.GetModifierInfo(modifiers, curmodifiers);
#ifdef MW_FEATURE_TWO_KEYBOARDS
	kbddev2.GetModifierInfo(&modifiers2, &curmodifiers2);
	if (modifiers)
		*modifiers |= modifiers2;
	if (curmodifiers)
		*curmodifiers |= curmodifiers2;
#endif
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
	int result = kbddev.Read(buf, modifiers, scancode);
#ifdef MW_FEATURE_TWO_KEYBOARDS
	if (result == 0)
		result = kbddev2.Read(buf, modifiers, scancode);
#endif
	return result;
}
