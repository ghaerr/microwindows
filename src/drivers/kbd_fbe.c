/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Framebuffer Emulator keyboard driver
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "device.h"

static int  TTY_Open(KBDDEVICE *pkd);
static void TTY_Close(void);
static void TTY_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  TTY_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  TTY_Poll(void);

KBDDEVICE kbddev = {
	TTY_Open,
	TTY_Close,
	TTY_GetModifierInfo,
	TTY_Read,
	NULL
};

static int keyboard_fd = -1;

/*
 * Open the keyboard.
 */
static int
TTY_Open(KBDDEVICE *pkd)
{
	keyboard_fd = open(MW_PATH_FBE_KEYBOARD, O_RDONLY | O_NONBLOCK);

	if (keyboard_fd < 0)
		return DRIVER_FAIL;

	return DRIVER_OKFILEDESC(keyboard_fd);
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
TTY_Close(void)
{
	if (keyboard_fd >= 0)
		close(keyboard_fd);
	keyboard_fd = -1;
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
TTY_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
		*curmodifiers = 0;
}

/*
	FBE read keyboard protocol - 8 bytes:
		buf[0] is 0xF5,
		buf[1] is KBD_PRESS or KBD_RELEASE
		buf[2-3] is MWKEY (little endian short)
		buf[4-5] is MWKEYMOD (little endian short)
		buf[6-7] is MWKEYMOD (little endian short)
*/
static int
TTY_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	MWKEY mwkey;
	MWKEYMOD mwkeymod;
	MWSCANCODE mwscan;
	int n;
	unsigned char buf[8];

	n = read(keyboard_fd, buf, sizeof(buf));
	if (n != 8 || buf[0] != 0xF5)
		return KBD_NODATA;

	mwkey = 	*(short *)&buf[2];
	mwkeymod = 	*(short *)&buf[4];
	mwscan = 	*(short *)&buf[6];

	*kbuf = mwkey;
	*modifiers = mwkeymod;
	*scancode = mwscan;
	return buf[1];		/* KBD_KEYPRESS or KBD_KEYRELEASE*/
}
