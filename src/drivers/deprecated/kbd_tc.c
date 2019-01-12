/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Copyright (c) 2000 Victor Larionov, Victor Rogachev <rogach@sut.ru>
 *
 * Keyboard Driver, TURBO C version
 */

#include "device.h"
#include <bios.h>

static int  KBD_Open(KBDDEVICE *pkd);
static void KBD_Close(void);
static void KBD_GetModifierInfo(int *modifiers);
static int  KBD_Read(MWUCHAR *buf, int *modifiers);
static int  KBD_Poll(void);

KBDDEVICE kbddev = {
	KBD_Open,
	KBD_Close,
	KBD_GetModifierInfo,
	KBD_Read,
	KBD_Poll
};

/*
 * Open the keyboard.
 */
static int
KBD_Open(KBDDEVICE *pkd)
{
	return 1;
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
KBD_GetModifierInfo(int *modifiers)
{
	*modifiers = bioskey(2);		/* no modifiers available */
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */
static int
KBD_Read(MWUCHAR *buf, int *modifiers)
{
	/* wait until a char is ready*/
	if(!bioskey(1))
		return 0;

	/* read keyboard shift status*/
	*modifiers = bioskey(2);

	/* read keyboard character*/
	*buf = bioskey(0);

	if(*buf == 0x1b)			/* special case ESC*/
		return -2;
	return 1;
}

static int
KBD_Poll(void)
{
	if (bioskey(1)!=0)
	  return 1;
	else
	  return 0;
}
