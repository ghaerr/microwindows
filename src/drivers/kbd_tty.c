/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * /dev/tty TTY Keyboard Driver
 */
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include "device.h"

#if ELKS
#define	KEYBOARD	"/dev/tty1"	/* keyboard associated with screen*/
#else
#define	KEYBOARD	"/dev/tty"	/* keyboard associated with screen*/
#endif

extern int escape_quits;

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
#if _MINIX
	TTY_Poll
#else
	NULL
#endif
};

static	int		fd;		/* file descriptor for keyboard */
static	struct termios	old;		/* original terminal modes */

/*
 * Open the keyboard.
 * This is real simple, we just use a special file handle
 * that allows non-blocking I/O, and put the terminal into
 * character mode.
 */
static int
TTY_Open(KBDDEVICE *pkd)
{
  char *env;

	int		i;
	int		ledstate = 0;
	struct termios	new;

	/* Open "CONSOLE" or /dev/tty device*/
	if(!(env = getenv("CONSOLE")))
		fd = open(KEYBOARD, O_NONBLOCK);
	else
		fd = open(env, O_NONBLOCK);
	if (fd < 0)
		return -1;

	if (tcgetattr(fd, &old) < 0)
		goto err;

	new = old;
	/* If you uncomment ISIG and BRKINT below, then ^C will be ignored.*/
	new.c_lflag &= ~(ECHO | ICANON | IEXTEN /*| ISIG*/);
	new.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON /*| BRKINT*/);
	new.c_cflag &= ~(CSIZE | PARENB);
	new.c_cflag |= CS8;
	new.c_cc[VMIN] = 0;
	new.c_cc[VTIME] = 0;

	if(tcsetattr(fd, TCSAFLUSH, &new) < 0)
		goto err;
	return fd;

err:
	close(fd);
	fd = 0;
	return -1;
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
TTY_Close(void)
{
	tcsetattr(fd, TCSANOW, &old);
	close(fd);
	fd = 0;
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
TTY_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
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
TTY_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	int	cc;			/* characters read */
	MWKEY	mwkey;
	unsigned char buf[1];

	cc = read(fd, buf, 1);
	if (cc > 0) {
		mwkey = buf[0];
		if (mwkey == 27 && escape_quits)	/* ESC -> quit*/
			mwkey = MWKEY_QUIT;
		else if (mwkey == ('P'&0x1f))		/* ^P -> print*/
			mwkey = MWKEY_PRINT;

		*kbuf = mwkey;		/* no translation*/
		*modifiers = 0;		/* no modifiers*/
		*scancode = 0;		/* no scancode*/
		return 1;		/* keypress*/
	}
	if ((cc < 0) && (errno != EINTR) && (errno != EAGAIN)) {
		return -1;
	}
	return 0;
}

static int
TTY_Poll(void)
{
	return 1;	/* used by _MINIX only*/
}
