/*
 * Author KennyD 
*    for use with nano-X on the "nintendo dual screen" 
 *   Large portions are based on devkitPro :: keyboard.h &is Copyright (C) 2007 Jason Rogers (Dovoto)
 *	
 */

#include <nds.h>
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

extern int nds_keypress;

/*
 * Open the keyboard.
 */
static int
KBD_Open(KBDDEVICE *pkd)
{
	keyboardDemoInit();

	keyboardShow();
	return 1;
}

/*
 * Close the keyboard.
 */
static void
KBD_Close(void)
{
	keyboardHide();
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
 * 
 */
static int
KBD_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	*modifiers = 0;

	if(nds_keypress > 0) {
		*buf = nds_keypress;
		nds_keypress = -nds_keypress;
	}
	else
		return KBD_NODATA;
	
	return KBD_KEYPRESS;
}

static int
KBD_Poll(void)
{
	nds_keypress = keyboardUpdate();
	if(nds_keypress > 0)
		return 1;
	return 0;
}
