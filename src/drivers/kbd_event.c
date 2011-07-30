/*
 * Copyright (c) 2008 Davide Rizzo <davide@elpa.it>
 *
 * Keyboard Driver, standard input events version
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include "device.h"
#include "keymap_standard.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int fd = -1;
static MWKEYMOD curmodif = 0, allmodif = 0;

/*
 * Open the keyboard.
 */
static int KBD_Open(KBDDEVICE *pkd)
{
	int i, r;
	char fname[64];
	for (i = 0; i < 32; i++)
	{
		sprintf(fname, "/sys/class/input/event%d/device/capabilities/ev", i);
		fd = open(fname, O_RDONLY);
		if (fd < 0)
			continue;
		r = read(fd, fname, sizeof(fname));
		close(fd);
		if (r <= 0)
			continue;
    fname[r - 1] = '\0';
		if ((strtoul(fname, NULL, 16) & (1 << EV_MSC)) == 0)
			continue;
    sprintf(fname, "/dev/input/event%d", i);
		fd = open(fname, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
			continue;
		curmodif = 0;
		/* TODO: Assign allmodif */
		allmodif = MWKMOD_LSHIFT | MWKMOD_RSHIFT| MWKMOD_LCTRL |
                        MWKMOD_RCTRL | MWKMOD_LALT | MWKMOD_RALT |
			MWKMOD_LMETA | MWKMOD_RMETA | MWKMOD_NUM |
                        MWKMOD_CAPS | MWKMOD_ALTGR | MWKMOD_SCR;
    return fd;
	}
	EPRINTF("Error %d opening keyboard input device\n", errno);
	return errno;
}

/*
 * Close the keyboard.
 */
static void KBD_Close(void)
{
 	if(fd >= 0)
		close(fd);
	fd = -1;
}

/*
 * Return the possible modifiers for the keyboard.
 */
static void KBD_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	curmodif &= allmodif;
	if (modifiers)
		*modifiers = allmodif;
	if (curmodifiers)
		*curmodifiers = curmodif;
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */
static int KBD_Read(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *pscancode)
{
	struct input_event event;
	int bytes_read;
	/* read a data point */
	while ((bytes_read = read(fd, &event, sizeof(event))) == sizeof(event))
	{
		if (event.type == EV_KEY)
		{
			if (event.value)
			{
				switch (event.code)
				{
				case KEY_LEFTSHIFT:
					curmodif |= MWKMOD_LSHIFT;
					break;
				case KEY_RIGHTSHIFT:
					curmodif |= MWKMOD_RSHIFT;
					break;
				case KEY_LEFTCTRL:
					curmodif |= MWKMOD_LCTRL;
					break;
				case KEY_RIGHTCTRL:
					curmodif |= MWKMOD_RCTRL;
					break;
				case KEY_LEFTALT:
					curmodif |= MWKMOD_LALT;
					break;
				case KEY_RIGHTALT:
					curmodif |= MWKMOD_RALT;
					break;
				case KEY_LEFTMETA:
					curmodif |= MWKMOD_LMETA;
					break;
				case KEY_RIGHTMETA:
					curmodif |= MWKMOD_RMETA;
					break;
				case KEY_NUMLOCK:
					curmodif |= MWKMOD_NUM;
					break;
				case KEY_CAPSLOCK:
					curmodif |= MWKMOD_CAPS;
					break;
				case KEY_SCROLLLOCK:
					curmodif |= MWKMOD_SCR;
					break;
				}
			}
			else
			{
				switch (event.code)
				{
				case KEY_LEFTSHIFT:
					curmodif &= ~MWKMOD_LSHIFT;
					break;
				case KEY_RIGHTSHIFT:
					curmodif &= ~MWKMOD_RSHIFT;
					break;
				case KEY_LEFTCTRL:
					curmodif &= ~MWKMOD_LCTRL;
					break;
				case KEY_RIGHTCTRL:
					curmodif &= ~MWKMOD_RCTRL;
					break;
				case KEY_LEFTALT:
					curmodif &= ~MWKMOD_LALT;
					break;
				case KEY_RIGHTALT:
					curmodif &= ~MWKMOD_RALT;
					break;
				case KEY_LEFTMETA:
					curmodif &= ~MWKMOD_LMETA;
					break;
				case KEY_RIGHTMETA:
					curmodif &= ~MWKMOD_RMETA;
					break;
				case KEY_NUMLOCK:
					curmodif &= ~MWKMOD_NUM;
					break;
				case KEY_CAPSLOCK:
					curmodif &= ~MWKMOD_CAPS;
					break;
				case KEY_SCROLLLOCK:
					curmodif &= ~MWKMOD_SCR;
					break;
				}
			}
			if (*modifiers)
				*modifiers = curmodif;
			if (event.code < ARRAY_SIZE(keymap))
			{
				*buf = keymap[event.code];
				*pscancode = event.code;
				if (*buf == MWKEY_ESCAPE)
					return -2;
				return event.value ? 1 : 2;
			}
		}
	}
	if(bytes_read == -1)
	{
		if (errno == EINTR || errno == EAGAIN) return 0;
		EPRINTF("Error %d reading from keyboard\n", errno);
		return -1;
	}
	if(bytes_read != 0)
	{
		EPRINTF("Wrong number of bytes %d read from keyboard "
		"(expected %d)\n", bytes_read, sizeof(event));
		return -1;
	}
	return 0;
}

KBDDEVICE kbddev = {
	KBD_Open,
	KBD_Close,
	KBD_GetModifierInfo,
	KBD_Read,
	NULL
};

