/*
 * Microwindows keyboard driver for Compaq iPAQ
 *
 * Copyright (c) 2000, 2003 Century Software Embedded Technologies
 * Written by Jordan Crouse
 */
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "device.h"

#define IPAQ_SCANCODE_RECORD    129
#define IPAQ_SCANCODE_CALENDAR  130
#define IPAQ_SCANCODE_CONTACTS 131
#define IPAQ_SCANCODE_Q        132
#define IPAQ_SCANCODE_START    133
#define IPAQ_SCANCODE_UP       134 /* keycode up */
#define IPAQ_SCANCODE_RIGHT    135 /* keycode right */
#define IPAQ_SCANCODE_LEFT     136 /* keycode left */
#define IPAQ_SCANCODE_DOWN     137 /* keycode down */
#define IPAQ_SCANCODE_ACTION   138
#define IPAQ_SCANCODE_SUSPEND  139

#ifdef __ECOS
#define KEYBOARD "/dev/kbd"
#else
#define KEYBOARD "/dev/h3600_key"
#endif

static int  IPAQ_Open(KBDDEVICE *pkd);
static void IPAQ_Close(void);
static void IPAQ_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  IPAQ_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);

KBDDEVICE kbddev = {
	IPAQ_Open,
	IPAQ_Close,
	IPAQ_GetModifierInfo,
	IPAQ_Read,
	NULL
};

static int fd;

/* From the kernel - this maps a single index into */
/* the correct scancode */
static unsigned char scancodes[] = {
        0, 			/* unused */
        IPAQ_SCANCODE_RECORD, 	/* 1 -> record button */
        IPAQ_SCANCODE_CALENDAR, /* 2 -> calendar */
        IPAQ_SCANCODE_CONTACTS, /* 3 -> contact */
        IPAQ_SCANCODE_Q, 	/* 4 -> Q button */
        IPAQ_SCANCODE_START, 	/* 5 -> start menu */
        IPAQ_SCANCODE_UP, 	/* 6 -> up */
        IPAQ_SCANCODE_RIGHT, 	/* 7 -> right */
        IPAQ_SCANCODE_LEFT, 	/* 8 -> left */
        IPAQ_SCANCODE_DOWN, 	/* 9 -> down */
	IPAQ_SCANCODE_ACTION, 	/* 10 */
	IPAQ_SCANCODE_SUSPEND
};

static int
IPAQ_Open(KBDDEVICE * pkd)
{
	/* Open the keyboard and get it ready for use */
	fd = open(KEYBOARD, O_RDONLY | O_NONBLOCK);

	if (fd < 0) {
		DPRINTF("%s - Can't open keyboard!\n", __FUNCTION__);
		return -1;
	}

	return fd;
}

static void
IPAQ_Close(void)
{
	close(fd);
	fd = -1;
}

static void
IPAQ_GetModifierInfo(MWKEYMOD * modifiers, MWKEYMOD * curmodifiers)
{
	if (modifiers)
		*modifiers = 0;	/* no modifiers available */
	if (curmodifiers)
		*curmodifiers = 0;
}


static int
IPAQ_Read(MWKEY * kbuf, MWKEYMOD * modifiers, MWSCANCODE * scancode)
{
	int keydown = 0;
	int cc = 0;
	char buf[1];

	cc = read(fd, &buf, 1);

	if (cc < 0) {
		if ((errno != EINTR) && (errno != EAGAIN)
		    && (errno != EINVAL)) {
			perror("IPAQ KEY");
			return (-1);
		} else {
			return (0);
		}
	}
	if (cc == 0)
		return (0);

	/* If the code is less than 127, then we know that */
	/* we have a key down.  Figure out what we've got */

	*modifiers = 0;

	if (*buf < 127) {
		keydown = 1;	/* Key pressed but not released */

		if (*buf > 9)
			return (0);

		*scancode = scancodes[(int) *buf];
	} else {
		keydown = 2;	/* key released */
		*scancode = *buf;
	}

	switch (*scancode) {
	case IPAQ_SCANCODE_RECORD:
		*kbuf = MWKEY_RECORD;
		break;

	case IPAQ_SCANCODE_CALENDAR:
		*kbuf = MWKEY_APP1;
		break;

	case IPAQ_SCANCODE_CONTACTS:
		*kbuf = MWKEY_APP2;
		break;

	case IPAQ_SCANCODE_Q:
		*kbuf = MWKEY_MENU;
		break;

	case IPAQ_SCANCODE_START:
/*      	*kbuf = MWKEY_LAST;*/
		*kbuf = MWKEY_CANCEL;
		break;

	case IPAQ_SCANCODE_UP:
		*kbuf = MWKEY_UP;
		break;

	case IPAQ_SCANCODE_DOWN:
		*kbuf = MWKEY_DOWN;
		break;

	case IPAQ_SCANCODE_LEFT:
		*kbuf = MWKEY_LEFT;
		break;

	case IPAQ_SCANCODE_RIGHT:
		*kbuf = MWKEY_RIGHT;
		break;

	case IPAQ_SCANCODE_ACTION:
		*kbuf = MWKEY_ENTER;
		break;

	case IPAQ_SCANCODE_SUSPEND:
		*kbuf = MWKEY_SUSPEND;
		break;

	default:
		DPRINTF("Ipaq - Unknown scancode %d\n", *scancode);
		return 0;
	}

	return keydown;
}
