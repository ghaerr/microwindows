/*
 * Copyright (c) 1999, 2003 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * /dev/tty TTY Keyboard Driver
 * 
 * if TRANSLATE_ESCAPE_SEQUENCES is set in device.h, then we
 * hard-decode function keys for Linux console and KDE konsole.
 */
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include "device.h"

/*#define DEBUG_ESCAPE_SEQUENCES*/

#if ELKS
#define	KEYBOARD	"/dev/tty1"	/* keyboard associated with screen*/
#else
#define	KEYBOARD	"/dev/tty"	/* keyboard associated with screen*/
#endif

#define CTRL(x)	  ((x) & 0x1f)

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
	char *		kbd;
	struct termios	new;

	/* Open "CONSOLE" or /dev/tty device*/
	if (!(kbd = getenv("CONSOLE")))
		kbd = KEYBOARD;
	fd = open(kbd, O_NONBLOCK);
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
#if TRANSLATE_ESCAPE_SEQUENCES
	static unsigned char buf[5];
	static int buflen = 0;
#else
	unsigned char buf[1];
	const int buflen = 0;
#endif
	int	cc;			/* characters read */
	MWKEY mwkey;

	cc = read(fd, buf + buflen, 1);
	if ((cc < 0) && (errno != EINTR) && (errno != EAGAIN)) {
		return -1;
	}
	if (cc <= 0) {
		return 0;
	}
	
	mwkey = buf[0];
#if TRANSLATE_ESCAPE_SEQUENCES
	if (mwkey == 27) {
		mwkey = 0;
		buflen += cc;
		if (buflen < 3) {
			/* Need more characters - escape sequences are 3+ chars */
			do {
				cc = read(fd, buf + buflen, 3 - buflen);
			} while ((cc < 0) && ((errno == EINTR) || (errno == EAGAIN)));
			if (cc < 0) {
				return -1;
			}
			buflen += cc;
			if (buflen < 3) {
				/* Partial escape sequence - will be completed on next read() */
				return 0;
			}
		}

		switch (buf[1]) {
		case 'O': /* Letter O */
			switch (buf[2]) {
			case 'P': mwkey = MWKEY_F1; break;
			case 'Q': mwkey = MWKEY_F2; break;
			case 'R': mwkey = MWKEY_F3; break;
			case 'S': mwkey = MWKEY_F4; break;
			}
			break;
		case '1':
			switch (buf[2]) {
			case '5': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F5;
				  break;
			case '7': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F6;
				  break;
			case '8': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F7;
				  break;
			case '9': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F8;
				  break;
			}
			break;
		case '2':
			switch (buf[2]) {
			case '0': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F9;  
				  break;
			case '1': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F10; 
				  break;
			case '3': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F11; 
				  break;
			case '4': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_F12; 
				  break;
			}
			break;
		case '[':
			switch (buf[2]) {
			case '1':
				if (buflen < 4)
					return 0;
				switch (buf[3]) {
				case '~': mwkey = MWKEY_HOME; 
					  break;
				case '5': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F5; 
					  break;
				case '7': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F6; 
					  break;
				case '8': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F7; 
					  break;
				case '9': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F8; 
					  break;
				}
				break;
			case '2':
				if (buflen < 4)
					return 0;
				switch (buf[3]) {
				case '~': mwkey = MWKEY_INSERT; break;
				case '0': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F9;  
					  break;
				case '1': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F10; 
					  break;
				case '3': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F11; 
					  break;
				case '4': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F12; 
					  break;
				}
				break;
			case '3': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_DELETE;   
				  break;
			case '4': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_END;      
				  break;
			case '5': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_PAGEUP;   
				  break;
			case '6': if (buflen < 4) return 0;
				  if (buf[3] == '~') mwkey = MWKEY_PAGEDOWN; 
				  break;
			case 'A': mwkey = MWKEY_UP;    break;
			case 'B': mwkey = MWKEY_DOWN;  break;
			case 'C': mwkey = MWKEY_RIGHT; break;
			case 'D': mwkey = MWKEY_LEFT;  break;
			case 'F': mwkey = MWKEY_END;   break;
			case 'H': mwkey = MWKEY_HOME;  break;
			case '[':
				if (buflen < 4)
					return 0;
				switch (buf[3]) {
				case '7': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F6; 
					  break;
				case '8': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F7; 
					  break;
				case '9': if (buflen < 5) return 0;
					  if (buf[4] == '~') mwkey = MWKEY_F8; 
					  break;
				case 'A': mwkey = MWKEY_F1; break;
				case 'B': mwkey = MWKEY_F2; break;
				case 'C': mwkey = MWKEY_F3; break;
				case 'D': mwkey = MWKEY_F4; break;
				case 'E': mwkey = MWKEY_F5; break;
				}
				break;
			}
			break;
		case 27:
			if (buf[2] == 27) mwkey = MWKEY_ESCAPE;
			break;
		}
		if (mwkey == 0) {
			if (buflen >= 5) {
				EPRINTF("WARNING: Unknown escape sequence ESC '%c' '%c' '%c' '%c'.\n"
					"(If you're trying to type a real escape, you have to press it 3 times.)\n",
					buf[1], buf[2], buf[3], buf[4]);
			} else if (buflen == 4) {
				EPRINTF("WARNING: Unknown escape sequence ESC '%c' '%c' '%c'.\n"
					"(If you're trying to type a real escape, you have to press it 3 times.)\n",
					buf[1], buf[2], buf[3]);
			} else {
				EPRINTF("WARNING: Unknown escape sequence ESC '%c' '%c'.\n"
					"(If you're trying to type a real escape, you have to press it 3 times.)\n",
					buf[1], buf[2]);
			}
			buflen = 0;
			return 0;
		}
#ifdef DEBUG_ESCAPE_SEQUENCES
		if (buflen >= 5) {
			EPRINTF("Got chars: ESC '%c' '%c' '%c' '%c'.\n",
				buf[1], buf[2], buf[3], buf[4]);
		} else if (buflen == 4) {
			EPRINTF("Got chars: ESC '%c' '%c' '%c'.\n",
				buf[1], buf[2], buf[3]);
		} else {
			EPRINTF("Got chars: ESC '%c' '%c'.\n",
				buf[1], buf[2]);
		}
#endif
		/* Parsed key - don't need the buffer any more. */
		buflen = 0;
	} else 
#endif /* TRANSLATE_ESCAPE_SEQUENCES */

	if (mwkey == CTRL('P'))			/* ^P -> print*/
		mwkey = MWKEY_PRINT;
	/*else if (mwkey == CTRL('S') || mwkey == CTRL('Q'))
		mwkey = MWKEY_SCROLLOCK;*/

	if ((mwkey == MWKEY_ESCAPE) && escape_quits)
		mwkey = MWKEY_QUIT;

#ifdef DEBUG_ESCAPE_SEQUENCES
	if (buf[0] != 27) {
		EPRINTF("Got char: '%c'.\n", buf[0]);
	}
	EPRINTF("Got key: %d\n", mwkey);
#endif

	*kbuf = mwkey;		/* no translation*/
	*modifiers = 0;		/* no modifiers*/
	*scancode = 0;		/* no scancode*/
	return 1;		/* keypress*/
}

static int
TTY_Poll(void)
{
	return 1;	/* used by _MINIX only*/
}
