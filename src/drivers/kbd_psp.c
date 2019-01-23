/*
 * Crappy PSP keyboard driver by Jim Paris.
 * Piggybacks on the mouse driver and just reads whatever the last button
 * state that the mouse driver read.
 */

#include <psputils.h>
#include <pspkernel.h>
#include <pspdebug.h>

#include "device.h"

static int  KBD_Open(KBDDEVICE *pkd);
static void KBD_Close(void);
static  void
KBD_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int
KBD_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int	KBD_Poll(void);

KBDDEVICE kbddev = {
	KBD_Open,
	KBD_Close,
	KBD_GetModifierInfo,
	KBD_Read,
	KBD_Poll
};

extern int psp_keypress;

/*
 * Open the keyboard.
 */
static int
KBD_Open(KBDDEVICE *pkd)
{
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the keyboard.
 */
static void
KBD_Close(void)
{
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
KBD_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if(modifiers)
		*modifiers = 0;			/* no modifiers available */
	if(curmodifiers)
		*curmodifiers = 0;			/* no modifiers available */
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */
static int
KBD_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	*modifiers = 0;
	
	if(psp_keypress > 0) {
		*buf = psp_keypress;
		psp_keypress = -psp_keypress;
	}
	else
		return 0;
	
	return 1;
}

static int
KBD_Poll(void)
{
	if(psp_keypress > 0)
		return 1;
	return 0;
}
