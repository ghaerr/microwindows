/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Microwindows /dev/tty console scancode keyboard driver for Linux
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/keyboard.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include "device.h"
#include "fb.h"

#define KEYBOARD	"/dev/tty"	/* console kbd to open*/

static int  TTY_Open(KBDDEVICE *pkd);
static void TTY_Close(void);
static void TTY_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  TTY_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);

KBDDEVICE kbddev = {
	TTY_Open,
	TTY_Close,
	TTY_GetModifierInfo,
	TTY_Read,
	NULL
};

#define RELEASED	0
#define PRESSED		1

static	int		fd;		/* file descriptor for keyboard */
static	struct termios	old;		/* original terminal modes */
static  int 		old_kbd_mode;
static unsigned char 	key_state[MWKEY_LAST];	/* FIXME - make sparse array */
static MWKEYMOD 	key_modstate;

/* kernel unicode tables per shiftstate and scancode*/
#define NUM_VGAKEYMAPS	(1<<KG_CAPSSHIFT)	/* kernel key maps*/
static unsigned short	os_keymap[NUM_VGAKEYMAPS][NR_KEYS];

/* PC scancode -> Microwindows key value mapping for non-Linux kernel values*/
static MWKEY		keymap[128] = {
MWKEY_UNKNOWN, MWKEY_ESCAPE, '1', '2', '3',				/* 0*/
'4', '5', '6', '7', '8',						/* 5*/
'9', '0', '-', '=', MWKEY_BACKSPACE,					/* 10*/
MWKEY_TAB, 'q', 'w', 'e', 'r',						/* 15*/
't', 'y', 'u', 'i', 'o',						/* 20*/
'o', '[', ']', MWKEY_ENTER, MWKEY_LCTRL,				/* 25*/
'a', 's', 'd', 'f', 'g',						/* 30*/
'h', 'j', 'k', 'l', ';',						/* 35*/
'\'', '`', MWKEY_LSHIFT, '\\', 'z',					/* 40*/
'x', 'c', 'v', 'b', 'n',						/* 45*/
'm', ',', '.', '/', MWKEY_RSHIFT,					/* 50*/
MWKEY_KP_MULTIPLY, MWKEY_LALT, ' ', MWKEY_CAPSLOCK, MWKEY_F1, 		/* 55*/
MWKEY_F2, MWKEY_F3, MWKEY_F4, MWKEY_F5, MWKEY_F6, 			/* 60*/
MWKEY_F7, MWKEY_F8, MWKEY_F9, MWKEY_F10, MWKEY_NUMLOCK, 		/* 65*/
MWKEY_SCROLLOCK, MWKEY_KP7, MWKEY_KP8, MWKEY_KP9, MWKEY_KP_MINUS,	/* 70*/
MWKEY_KP4, MWKEY_KP5, MWKEY_KP6, MWKEY_KP_PLUS, MWKEY_KP1, 		/* 75*/
MWKEY_KP2, MWKEY_KP3, MWKEY_KP0, MWKEY_KP_PERIOD, MWKEY_UNKNOWN, 	/* 80*/
MWKEY_UNKNOWN, MWKEY_UNKNOWN, MWKEY_F11, MWKEY_F12, MWKEY_UNKNOWN,	/* 85*/
MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,	/* 90*/
MWKEY_UNKNOWN, MWKEY_KP_ENTER, MWKEY_RCTRL, MWKEY_KP_DIVIDE,MWKEY_PRINT,/* 95*/
MWKEY_RALT, MWKEY_BREAK, MWKEY_HOME, MWKEY_UP, MWKEY_PAGEUP,		/* 100*/
MWKEY_LEFT, MWKEY_RIGHT, MWKEY_END, MWKEY_DOWN, MWKEY_PAGEDOWN,		/* 105*/
MWKEY_INSERT, MWKEY_DELETE, MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,	/* 110*/
MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_PAUSE,	/* 115*/
MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,MWKEY_UNKNOWN,	/* 120*/
MWKEY_LMETA, MWKEY_RMETA, MWKEY_MENU					/* 125*/
};

static MWBOOL	UpdateKeyState(int pressed, MWKEY mwkey);
static void	UpdateLEDState(MWKEYMOD modstate);
static MWKEY	TranslateScancode(int scancode, MWKEYMOD modstate);
static void	LoadKernelKeymaps(int fd);
static MWBOOL	switch_vt(unsigned short which);

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

	/* Save previous settings*/
	if (ioctl(fd, KDGKBMODE, &old_kbd_mode) < 0) {
		perror("KDGKMODE");
		goto err;
	}
	if (tcgetattr(fd, &old) < 0)
		goto err;

	/* Set medium-raw keyboard mode */
	new = old;
	/* ISIG and BRKINT must be set otherwise '2' is ^C (scancode 3)!!*/
	new.c_lflag &= ~(ICANON | ECHO | ISIG);
	new.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON 
			| BRKINT);
	new.c_cc[VMIN] = 0;
	new.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &new) < 0) {
		TTY_Close();
		return -1;
	}
	if (ioctl(fd, KDSKBMODE, K_MEDIUMRAW) < 0) {
		TTY_Close();
		return -1;
	}

	/* Load OS keymappings*/
	LoadKernelKeymaps(fd);

	/* Initialize keyboard state*/
	key_modstate = MWKMOD_NONE;
	for (i=0; i<MWKEY_LAST; ++i)
		key_state[i] = RELEASED;
	
	/* preset CAPSLOCK and NUMLOCK from startup LED state*/
	if (ioctl(fd, KDGETLED, &ledstate) == 0) {
		if (ledstate & LED_CAP) {
			key_modstate |= MWKMOD_CAPS;
			key_state[MWKEY_CAPSLOCK] = PRESSED;
		}
		if (ledstate & LED_NUM) {
			key_modstate |= MWKMOD_NUM;
			key_state[MWKEY_NUMLOCK] = PRESSED;
		}
	}
	UpdateLEDState(key_modstate);

	return fd;

err:
	close(fd);
	fd = -1;
	return -1;
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
TTY_Close(void)
{
	int	ledstate = 0x80000000L;

	if (fd >= 0) {
		/* revert LEDs to follow key modifiers*/
		if (ioctl(fd, KDSETLED, ledstate) < 0)
			perror("KDSETLED");

		/* reset terminal mode*/
		if (ioctl(fd, KDSKBMODE, old_kbd_mode) < 0)
			perror("KDSKBMODE");
		tcsetattr(fd, TCSAFLUSH, &old);

		close(fd);
	}
	fd = -1;
}

/*
 * Return the possible modifiers and current modifiers for the keyboard.
 */
static  void
TTY_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_CTRL | MWKMOD_SHIFT | MWKMOD_ALT |
			MWKMOD_META | MWKMOD_ALTGR | MWKMOD_CAPS | MWKMOD_NUM;
	if (curmodifiers)
		*curmodifiers = key_modstate;
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the modifier keys (ALT, SHIFT, etc).  Returns -1 on error, 0 if no data
 * is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
TTY_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *pscancode)
{
	int	cc;			/* characters read */
	int 	pressed;
	int 	scancode;
	MWKEY	mwkey;
	unsigned char buf[128];

	cc = read(fd, buf, 1);
	if (cc > 0) {
		pressed = (*buf & 0x80) ? RELEASED: PRESSED;
		scancode = *buf & 0x7f;
		mwkey = keymap[scancode];

		/**if(pressed) {
			printf("scan %02x really: %08x\n", *buf&0x7F, *buf);
			printf("mwkey: %02x (%c)\n", mwkey, mwkey);
		}**/

		/* Handle Alt-FN for vt switch */
		switch (mwkey) {
		case MWKEY_F1:
		case MWKEY_F2:
		case MWKEY_F3:
		case MWKEY_F4:
		case MWKEY_F5:
		case MWKEY_F6:
		case MWKEY_F7:
		case MWKEY_F8:
		case MWKEY_F9:
		case MWKEY_F10:
		case MWKEY_F11:
		case MWKEY_F12:
			if (key_modstate & MWKMOD_ALT) {
				if (switch_vt(mwkey-MWKEY_F1+1)) {
					mwkey = MWKEY_REDRAW;
				}
			}
			break;
			/* Fall through to normal processing */
		default:
			/* update internal key states*/
			if (!UpdateKeyState(pressed, mwkey))
				return 0;

			/* mwkey is 0 if only a modifier is hit */
			if(mwkey != MWKEY_LCTRL && 
			   mwkey != MWKEY_RCTRL &&
			   mwkey != MWKEY_LALT &&
			   mwkey != MWKEY_RALT &&
			   mwkey != MWKEY_RSHIFT &&
			   mwkey != MWKEY_LSHIFT) {
				/* translate scancode to key value*/
				mwkey = TranslateScancode(scancode, key_modstate);
			} else {
				//printf("Modifier only\n");
				//mwkey = 0;
			}
			
			/* XXX Hack to get scancodes to come out the same as 
			   everything else */
			switch(scancode) {
				case 0x1:           /* esc 		*/

				case 0x29:          /* `		*/
				case 0x2  ... 0xe:  /* 1 - BackSpace 	*/

				case 0xf  ... 0x1b: /* TAB - ] 		*/
				case 0x2b:          /* \		*/

				case 0x3a:	    /* Caps-Lock	*/
				case 0x1e ... 0x28: /* a - '		*/
				case 0x1c:          /* Enter		*/

				case 0x2a:          /* LShift		*/
				case 0x2c ... 0x35: /* z - /		*/
				case 0x36:          /* RShift		*/

				case 0x1d:          /* LCtrl		*/
				//case 0x7d:          /* LWin		*/
				case 0x38:          /* LAlt		*/
				case 0x39:          /* Space		*/
				//case 0x64:          /* RAlt		*/
				//case 0x7e:          /* RWin		*/
				//case 0x7f:          /* Win-PopupMenu	*/
				//case 0x61:          /* RCtrl		*/

				//case 0x63:          /* SysReq		*/
				//case 0x46:          /* Scroll Lock	*/
				//case 0x77:          /* Pause/Break	*/
					scancode += 8;
					break;

				case 0x6e:            /* Insert		*/
					scancode -= 0x4;
					break;
				case 0x66:            /* Home		*/
				case 0x68:            /* Page-Up	*/
					scancode -= 0x5;
					break;

				case 0x6f:            /* Delete		*/
				case 0x6b:            /* End		*/
				case 0x6d:            /* Page-Down	*/
					scancode -= 0x4;
					break;
				
				case 0x67:            /* Up arrow	*/
				case 0x69:            /* Left arrow	*/
					scancode -= 0x5;
					break;
				
				case 0x6a:            /* Right arrow	*/
				case 0x6c:            /* Down arrow	*/
					scancode -= 0x4;
					break;

				default: 
					break;
			}
			break;
		}	
		*kbuf = mwkey;
		*modifiers = key_modstate;
		*pscancode = scancode;

		/**if(pressed) {
			printf("Returning: mwkey: 0x%04x, mods: 0x%x,
				sc:0x%04x\n\n", *kbuf, *modifiers, *pscancode);
		}**/
		return pressed ? 1 : 2;
	}

	if ((cc < 0) && (errno != EINTR) && (errno != EAGAIN))
		return -1;
	return 0;
}

/* Update the internal keyboard state, return TRUE if changed*/
static MWBOOL
UpdateKeyState(int pressed, MWKEY mwkey)
{
	MWKEYMOD modstate = key_modstate;

	//printf("UpdateKeyState %02x %02x\n", pressed, mwkey);
	if (pressed == PRESSED) {
		switch (mwkey) {
		case MWKEY_NUMLOCK:
		case MWKEY_CAPSLOCK:
			/* change state on release because of auto-repeat*/
			return FALSE;
		case MWKEY_LCTRL:
			modstate |= MWKMOD_LCTRL;
			break;
		case MWKEY_RCTRL:
			modstate |= MWKMOD_RCTRL;
			break;
		case MWKEY_LSHIFT:
			modstate |= MWKMOD_LSHIFT;
			break;
		case MWKEY_RSHIFT:
			modstate |= MWKMOD_RSHIFT;
			break;
		case MWKEY_LALT:
			modstate |= MWKMOD_LALT;
			break;
		case MWKEY_RALT:
			modstate |= MWKMOD_RALT;
			break;
		case MWKEY_LMETA:
			modstate |= MWKMOD_LMETA;
			break;
		case MWKEY_RMETA:
			modstate |= MWKMOD_RMETA;
			break;
		case MWKEY_ALTGR:
			modstate |= MWKMOD_ALTGR;
			break;
		default:
			break;
		}
	} else {
		switch (mwkey) {
		case MWKEY_NUMLOCK:
			key_modstate ^= MWKMOD_NUM;
			key_state[MWKEY_NUMLOCK] ^= PRESSED;
			UpdateLEDState(key_modstate);
			return TRUE;
		case MWKEY_CAPSLOCK:
			key_modstate ^= MWKMOD_CAPS;
			key_state[MWKEY_CAPSLOCK] ^= PRESSED;
			UpdateLEDState(key_modstate);
			return TRUE;
		case MWKEY_LCTRL:
			modstate &= ~MWKMOD_LCTRL;
			break;
		case MWKEY_RCTRL:
			modstate &= ~MWKMOD_RCTRL;
			break;
		case MWKEY_LSHIFT:
			modstate &= ~MWKMOD_LSHIFT;
			break;
		case MWKEY_RSHIFT:
			modstate &= ~MWKMOD_RSHIFT;
			break;
		case MWKEY_LALT:
			modstate &= ~MWKMOD_LALT;
			break;
		case MWKEY_RALT:
			modstate &= ~MWKMOD_RALT;
			break;
		case MWKEY_LMETA:
			modstate &= ~MWKMOD_LMETA;
			break;
		case MWKEY_RMETA:
			modstate &= ~MWKMOD_RMETA;
			break;
		case MWKEY_ALTGR:
			modstate &= ~MWKMOD_ALTGR;
			break;
		default:
			break;
		}
	}

#if 0
	/* Drop events that don't change state */
	if (key_state[mwkey] == pressed)
		return FALSE;
#endif
	/* Update internal keyboard state */
	key_state[mwkey] = (unsigned char)pressed;
	key_modstate = modstate;
	return TRUE;
}

static void
UpdateLEDState(MWKEYMOD modstate)
{
	int	ledstate = 0;

	if (modstate & MWKMOD_CAPS)
		ledstate |= LED_CAP;
	if (modstate & MWKMOD_NUM)
		ledstate |= LED_NUM;
	ioctl(fd, KDSETLED, ledstate);
}

/* translate a scancode and modifier state to an MWKEY*/
static MWKEY
TranslateScancode(int scancode, MWKEYMOD modstate)
{
	unsigned short	mwkey = 0;
	int		map = 0;

	//printf("Translate: 0x%04x\n", scancode);

	/* determine appropriate kernel table*/
	if (modstate & MWKMOD_SHIFT)
		map |= (1<<KG_SHIFT);
	if (modstate & MWKMOD_CTRL)
		map |= (1<<KG_CTRL);
	if (modstate & MWKMOD_ALT)
		map |= (1<<KG_ALT);
	if (modstate & MWKMOD_ALTGR)
		map |= (1<<KG_ALTGR);
	if (KTYP(os_keymap[map][scancode]) == KT_LETTER) {
		if (modstate & MWKMOD_CAPS)
			map |= (1<<KG_SHIFT);
	}
	if (KTYP(os_keymap[map][scancode]) == KT_PAD) {
		if (modstate & MWKMOD_NUM) {
			switch (keymap[scancode]) {
			case MWKEY_KP0:
			case MWKEY_KP1:
			case MWKEY_KP2:
			case MWKEY_KP3:
			case MWKEY_KP4:
			case MWKEY_KP5:
			case MWKEY_KP6:
			case MWKEY_KP7:
			case MWKEY_KP8:
			case MWKEY_KP9:
				mwkey = keymap[scancode] - MWKEY_KP0 + '0';
				break;
			case MWKEY_KP_PERIOD:
				mwkey = '.';
				break;
			case MWKEY_KP_DIVIDE:
				mwkey = '/';
				break;
			case MWKEY_KP_MULTIPLY:
				mwkey = '*';
				break;
			case MWKEY_KP_MINUS:
				mwkey = '-';
				break;
			case MWKEY_KP_PLUS:
				mwkey = '+';
				break;
			case MWKEY_KP_ENTER:
				mwkey = MWKEY_ENTER;
				break;
			case MWKEY_KP_EQUALS:
				mwkey = '-';
				break;
			}
		}
	} else
		mwkey = KVAL(os_keymap[map][scancode]);
	
	if (!mwkey)
		mwkey = keymap[scancode];

	/* perform additional translations*/
	switch (mwkey) {
	case 127:
		mwkey = MWKEY_BACKSPACE;
		break;
	case MWKEY_BREAK:
	case MWKEY_PAUSE:
		mwkey = MWKEY_QUIT;
		break;
	case 0x1c:	/* kernel maps print key to ctrl-\ */
	case MWKEY_SYSREQ:
		mwkey = MWKEY_PRINT;
		break;
	}

	/* printf("TranslateScancode %02x to mwkey %d\n", scancode, mwkey); */
	return mwkey;
}

/* load Linux keyboard mappings, used as first try for scancode conversion*/
static void
LoadKernelKeymaps(int fd)
{
	int 		map, i;
	struct kbentry 	entry;

	/* Load all the keysym mappings */
	for (map=0; map<NUM_VGAKEYMAPS; ++map) {
		memset(os_keymap[map], 0, NR_KEYS*sizeof(unsigned short));
		for (i=0; i<NR_KEYS; ++i) {
			entry.kb_table = map;
			entry.kb_index = i;
			if (ioctl(fd, KDGKBENT, &entry) == 0) {
				/* change K_ENTER to \r*/
				if (entry.kb_value == K_ENTER)
					entry.kb_value = K(KT_ASCII,13);

				if ((KTYP(entry.kb_value) == KT_LATIN) ||
				    (KTYP(entry.kb_value) == KT_ASCII) ||
				    (KTYP(entry.kb_value) == KT_PAD) ||
				    (KTYP(entry.kb_value) == KT_LETTER)
				    )
					os_keymap[map][i] = entry.kb_value;
			}
		}
	}

}
/* Handle switching to another VC, returns when our VC is back */
static MWBOOL
switch_vt(unsigned short which)
{
	struct vt_stat vtstate;
	unsigned short current;
	static unsigned short r[16], g[16], b[16];

	/* Figure out whether or not we're switching to a new console */
	if ((ioctl(fd, VT_GETSTATE, &vtstate) < 0) ||
	    (which == vtstate.v_active)) {
		return FALSE;
	}
	current = vtstate.v_active;

	/* save palette, goto text mode*/
	ioctl_getpalette(0, 16, r, g, b);
	ioctl(fd, KDSETMODE, KD_TEXT);

	/* New console, switch to it */
	if (ioctl(fd, VT_ACTIVATE, which) == 0) {
		/* Wait for our console to be activated again */
		ioctl(fd, VT_WAITACTIVE, which);
		while (ioctl(fd, VT_WAITACTIVE, current) < 0) {
			if ((errno != EINTR) && (errno != EAGAIN)) {
				/* Unknown VT error, cancel*/
				break;
			}
			usleep(100000);
		}
	}

	/* Restore graphics mode and the contents of the screen */
	ioctl(fd, KDSETMODE, KD_GRAPHICS);
	ioctl_setpalette(0, 16, r, g, b);
	return TRUE;
}
