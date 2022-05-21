/* Portions Copyright 2003, Jordan Crouse (jordan@cosmicpenguin.net) */

#include "nxlib.h"
#include <string.h>
#include "X11/keysym.h"
#include "X11/Xutil.h"
#include "keysymstr.h"

#include <stdlib.h>
#include "uni_std.h"
#include <fcntl.h>
#if !__MINGW32__
#include <sys/ioctl.h>
#endif

#if linux
#include <linux/keyboard.h>
#include <linux/kd.h>
#else
#include "linux/keyboard.h"
#include "linux/kd.h"
#endif

#define KEYBOARD "/dev/tty0"		/* device to get keymappings from*/

#define OLDWAY		0				/* new way required for FLTK on X11, old was linux only*/

/* kernel unicode tables per shiftstate and scancode*/
#define NUM_VGAKEYMAPS	(1<<KG_CAPSSHIFT)	/* kernel key maps*/
static unsigned short	os_keymap[NUM_VGAKEYMAPS][NR_KEYS];
static MWKEYMOD modstate;
static int map_loaded = 0;

/* Standard keymapings for kernel values */
/* (from microwin/src/drivers/keymap_standard.h)*/
/* this table should be retrieved through Microwindows*/
static MWKEY mwscan_to_mwkey[128] = {
MWKEY_UNKNOWN, MWKEY_ESCAPE, '1', '2', '3',				/* 0*/
'4', '5', '6', '7', '8',						/* 5*/
'9', '0', '-', '=', MWKEY_BACKSPACE,					/* 10*/
MWKEY_TAB, 'q', 'w', 'e', 'r',						/* 15*/
't', 'y', 'u', 'i', 'o',						/* 20*/
'p', '[', ']', MWKEY_ENTER, MWKEY_LCTRL,				/* 25*/
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

/* load Linux keyboard mappings, used as first try for scancode conversion*/
static void
LoadKernelKeymaps(void)
{
#if OLDWAY
	int 		map, i;
	struct kbentry 	entry;
	char *		kbd;
	int 		fd;
	int		ledstate = 0;

	/* Open "CONSOLE" or /dev/tty device*/
	if(!(kbd = getenv("CONSOLE")))
		kbd = KEYBOARD;
	fd = open(kbd, O_NONBLOCK);
	if (fd < 0)
		return;

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
				    (KTYP(entry.kb_value) == KT_LETTER))
						os_keymap[map][i] = entry.kb_value;
			}
		}
	}

	/* preset CAPSLOCK and NUMLOCK from startup LED state*/
	if (ioctl(fd, KDGETLED, &ledstate) == 0) {
		if (ledstate & LED_CAP) {
			modstate |= MWKMOD_CAPS;
		}
		if (ledstate & LED_NUM) {
			modstate |= MWKMOD_NUM;
		}
	}

	close(fd);
#endif /* OLDWAY*/
	map_loaded = 1;
}

/* translate a scancode and modifier state to an MWKEY*/
static MWKEY
TranslateScancode(int scancode)
{
	unsigned short	mwkey = 0;
	int		map = 0;

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
			
			switch (mwscan_to_mwkey[scancode]) {
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
				mwkey = mwscan_to_mwkey[scancode] - MWKEY_KP0 + '0';
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
		mwkey = mwscan_to_mwkey[scancode];

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
	case MWKEY_NUMLOCK:
		modstate ^= MWKMOD_NUM;	
		break;
	case MWKEY_CAPSLOCK:
		modstate ^= MWKMOD_CAPS;	
		break;
		}

	/* DPRINTF("TranslateScancode %02x to mwkey %d\n", scancode, mwkey); */
	return mwkey;
}

static struct {
	GR_KEY nxKey;
	KeySym xKey;
} mwkey_to_xkey[] = {
	{MWKEY_RIGHT, XK_Right},
	{MWKEY_LEFT, XK_Left},
	{MWKEY_UP, XK_Up},
	{MWKEY_DOWN, XK_Down},
	{MWKEY_PAGEDOWN, XK_Page_Down},
	{MWKEY_PAGEUP, XK_Page_Up},
	{MWKEY_INSERT, XK_Insert},
	{MWKEY_DELETE, XK_Delete},
	{MWKEY_HOME, XK_Home},
	{MWKEY_END, XK_End},
	{MWKEY_TAB, XK_Tab},
	{MWKEY_BACKSPACE, XK_BackSpace},
	{MWKEY_ENTER, XK_Return},
	{MWKEY_ESCAPE, XK_Escape},
	{MWKEY_LCTRL, XK_Control_L},
	{MWKEY_RCTRL, XK_Control_R},
	{MWKEY_LSHIFT, XK_Shift_L},
	{MWKEY_RSHIFT, XK_Shift_R},
	{MWKEY_LALT, XK_Alt_L},
	{MWKEY_LALT, XK_Mode_switch},		/* MACOSX left or right alt/opt*/
	{MWKEY_RALT, XK_Alt_R},
	{MWKEY_LMETA, XK_Meta_L},
	{MWKEY_RMETA, XK_Meta_R},
	{MWKEY_PAUSE, XK_Pause},
	{MWKEY_PRINT, XK_Print},
	{MWKEY_SYSREQ, XK_Sys_Req},
	{MWKEY_BREAK, XK_Break},
	{MWKEY_NUMLOCK, XK_Num_Lock},
	{MWKEY_CAPSLOCK, XK_Caps_Lock},
	{MWKEY_SCROLLOCK, XK_Scroll_Lock},
	{MWKEY_F1, XK_F1},
	{MWKEY_F2, XK_F2},
	{MWKEY_F3, XK_F3},
	{MWKEY_F4, XK_F4},
	{MWKEY_F5, XK_F5},
	{MWKEY_F6, XK_F6},
	{MWKEY_F7, XK_F7},
	{MWKEY_F8, XK_F8},
	{MWKEY_F9, XK_F9},
	{MWKEY_F10, XK_F10},
	{MWKEY_F11, XK_F11},
	{MWKEY_F12, XK_F12},
	{MWKEY_KP1, XK_KP_End},
	{MWKEY_KP2, XK_KP_Down},
	{MWKEY_KP3, XK_KP_Page_Down},
	{MWKEY_KP4, XK_KP_Left},
	{MWKEY_KP5, XK_KP_Home},
	{MWKEY_KP6, XK_KP_Right},
	{MWKEY_KP7, XK_KP_Home},
	{MWKEY_KP8, XK_KP_Up},
	{MWKEY_KP9, XK_KP_Page_Up},
	{MWKEY_KP0, XK_KP_Insert},
	{MWKEY_KP_PERIOD, XK_KP_Delete},
	{MWKEY_KP_ENTER, XK_KP_Enter},
	{MWKEY_KP_DIVIDE, XK_KP_Divide},
	{MWKEY_KP_MULTIPLY, XK_KP_Multiply},
	{MWKEY_KP_MINUS, XK_KP_Subtract},
	{MWKEY_KP_PLUS, XK_KP_Add},
	{MWKEY_MENU, XK_Menu},
	{0xffff, 0xffff}
};


/* load Linux kernel keymap, ignores parameter*/
int 
XRefreshKeyboardMapping(XMappingEvent* event)
{ 
	LoadKernelKeymaps();
	return 0;
}

/* translate keycode to KeySym, no control/shift processing*/
/* no international keyboard support */
#if NeedWidePrototypes
KeySym XKeycodeToKeysym(Display *dpy, unsigned int kc, int index)
#else
KeySym XKeycodeToKeysym(Display *dpy, KeyCode kc, int index)
#endif
{
	//DPRINTF("XKeycodeToKeysym called\n");
	int	i;
	MWKEY	mwkey;

	if (kc > 127)
		return NoSymbol;

	/* first convert scancode to mwkey*/
	mwkey = TranslateScancode(kc);
	
	/* then possibly convert mwkey to X KeySym*/
	for (i=0; mwkey_to_xkey[i].nxKey != 0xffff; i++) {
		if (mwkey == mwkey_to_xkey[i].nxKey)
			return mwkey_to_xkey[i].xKey;
	}

	/* assume X KeySym is same as MWKEY value*/
	return mwkey;
}
/* translate keyvalue to KeySym, no control/shift processing*/

KeySym
XMWKeyToKeysym(Display *dpy, unsigned int kv, int index)
{
	int	i;
	MWKEY	mwkey;

	//if (kv > 127)
	//	return NoSymbol;

	//DPRINTF("XMWKeyToKeysym called - %X\n",(unsigned int)kv);

	mwkey = kv;

	/* then possibly convert mwkey to X KeySym*/
	for (i=0; mwkey_to_xkey[i].nxKey != 0xffff; i++) {
		if (mwkey == mwkey_to_xkey[i].nxKey)
			return mwkey_to_xkey[i].xKey;
	}

	/* assume X KeySym is same as MWKEY value*/
	return mwkey;
}

/* translate event->keycode/event->y_root into KeySym, no control/shift processing*/
KeySym
XLookupKeysym(XKeyEvent *event, int index)
{
#if OLDWAY
	return XKeycodeToKeysym(event->display, event->keycode, index);
#else
	/* event->y_root set in NextEvent.c*/
	return XMWKeyToKeysym(event->display, (unsigned int) event->y_root, index);
#endif
}

/* translate event->keycode into *keysym, control/shift processing*/
int
XLookupString(XKeyEvent *event, char *buffer, int nbytes, KeySym *keysym,
	XComposeStatus *status)
{
	KeySym k;

	modstate &= 0xffff ^ MWKMOD_SHIFT;
	modstate &= 0xffff ^ MWKMOD_CTRL;

	/* translate Control/Shift*/
	if ((event->state & ControlMask) /* && k < 256*/) {
		modstate |= MWKMOD_CTRL;
		//k &= 0x1f;
	} else if (event->state & ShiftMask)
		modstate |= MWKMOD_SHIFT;
	else if( event->state & Mod1Mask)
		modstate |= MWKMOD_ALTGR;

	k = XLookupKeysym(event, 0);

	//DPRINTF("XLookupString called - %X\n",(unsigned int)k);

#if OLDWAY
	if(!map_loaded) {
		/* translate Control/Shift*/
		if ((event->state & ControlMask) && k < 256)
			k &= 0x1f;
		else if (event->state & ShiftMask) {
			if (k >= 'a' && k <= 'z')
				k = k-'a'+'A';
			else switch (k) {
			case '`': k = '~'; break;
			case '1': k = '!'; break;
			case '2': k = '@'; break;
			case '3': k = '#'; break;
			case '4': k = '$'; break;
			case '5': k = '%'; break;
			case '6': k = '^'; break;
			case '7': k = '&'; break;
			case '8': k = '*'; break;
			case '9': k = '('; break;
			case '0': k = ')'; break;
			case '-': k = '_'; break;
			case '=': k = '+'; break;
			case '\\': k = '|'; break;
			case '[': k = '{'; break;
			case ']': k = '}'; break;
			case ';': k = ':'; break;
			case '\'': k = '"'; break;
			case ',': k = '<'; break;
			case '.': k = '>'; break;
			case '/': k = '?'; break;
			}
		}
	}
#endif /* OLDWAY*/

	if (keysym)
		*keysym = k;
	buffer[0] = (char)k;

	if (k & 0xFF00) {	/* non-ASCII key, return 0 for cursor keys in FLTK */
		switch (k) {
		case XK_BackSpace:
		case XK_Tab:
		case XK_Linefeed:
		case XK_Return:
		case XK_Escape:
		case XK_Delete:
			break;
		default:
			return 0;
		}
	}
	return 1;
}

/* Freeking ugly! */
KeySym
XStringToKeysym(_Xconst char *string)
{
	int i;

	for (i=0; i < NX_KEYSYMSTR_COUNT; i++)
		if (strcmp(nxKeyStrings[i].str, string) == 0)
			return nxKeyStrings[i].keysym;

	return NoSymbol;
}

char *XKeysymToString(KeySym ks)
{
	int i;

	DPRINTF("XKeysymToString called [%x]\n", (int)ks);
	for (i=0; i < NX_KEYSYMSTR_COUNT; i++)
		if (nxKeyStrings[i].keysym == ks) return nxKeyStrings[i].str;

	return NULL;
}

/* translate KeySym to KeyCode*/
KeyCode
XKeysymToKeycode(Display *dpy, KeySym ks)
{
	int i;

	for (i=0; i<128; ++i)
		if (mwscan_to_mwkey[i] == ks)
			return i;
	return NoSymbol;
}

/* Translate the keysym to upper case and lower case */

void
XConvertCase(KeySym in, KeySym *upper, KeySym *lower)
{
	if (in & MWKEY_NONASCII_MASK) 
		*upper = *lower = in;
	else {
		*upper = (in >= 'a' && in <= 'z')? in-'a'+'A': in;
		*lower = (in >= 'A' && in <= 'A')? in-'A'+'a': in;
	}
}
  
#if 0000
/*
 * Microwindows ttyscan.c compatible scancode conversion
 * table.  Note this is NOT the same as the Linux kernel
 * table due to the HACK XXX in ttyscan.c after getting
 * the kernel scancode.  FIXME
 */
#define UNKNOWN	0
static MWKEY mwscan_to_mwkey[128] = {
	UNKNOWN,	/*  0*/
	UNKNOWN,	/*  1*/
	UNKNOWN,	/*  2*/
	UNKNOWN,	/*  3*/
	UNKNOWN,	/*  4*/
	UNKNOWN,	/*  5*/
	UNKNOWN,	/*  6*/
	UNKNOWN,	/*  7*/
	UNKNOWN,	/*  8*/
	MWKEY_ESCAPE,	/*  9*/
	'1',		/* 10*/
	'2',		/* 11*/
	'3',		/* 12*/
	'4',		/* 13*/
	'5',		/* 14*/
	'6',		/* 15*/
	'7',		/* 16*/
	'8',		/* 17*/
	'9',		/* 18*/
	'0',		/* 19*/
	'-',		/* 20*/
	UNKNOWN,	/* 21*/
	MWKEY_BACKSPACE,/* 22*/
	MWKEY_TAB,	/* 23*/
	'q',		/* 24*/
	'w',		/* 25*/
	'e',		/* 26*/
	'r',		/* 27*/
	't',		/* 28*/
	'y',		/* 29*/
	'u',		/* 30*/
	'i',		/* 31*/
	'o',		/* 32*/
	'p',		/* 33*/
	'[',		/* 34*/
	']',		/* 35*/
	MWKEY_ENTER,	/* 36*/
	MWKEY_LCTRL,	/* 37*/
	'a',		/* 38*/
	's',		/* 39*/
	'd',		/* 40*/
	'f',		/* 41*/
	'g',		/* 42*/
	'h',		/* 43*/
	'j',		/* 44*/
	'k',		/* 45*/
	'l',		/* 46*/
	';',		/* 47*/
	'\'',		/* 48*/
	'`',		/* 49*/
	MWKEY_LSHIFT,	/* 50*/
	'\\',		/* 51*/
	'z',		/* 52*/
	'x',		/* 53*/
	'c',		/* 54*/
	'v',		/* 55*/
	'b',		/* 56*/
	'n',		/* 57*/
	'm',		/* 58*/
	',',		/* 59*/
	'.',		/* 60*/
	'/',		/* 61*/
	MWKEY_RSHIFT,	/* 62*/
	MWKEY_KP_MULTIPLY,/* 63*/
	MWKEY_LALT,	/* 64*/
	' ',		/* 65*/
	UNKNOWN,	/* 66*/
	MWKEY_F1,	/* 67*/
	MWKEY_F2,	/* 68*/
	MWKEY_F3,	/* 69*/
	MWKEY_F4,	/* 70*/
	MWKEY_F5,	/* 71*/
	MWKEY_F6,	/* 72*/
	MWKEY_F7,	/* 73*/
	MWKEY_F8,	/* 74*/
	MWKEY_F9,	/* 75*/
	MWKEY_F10,	/* 76*/
	UNKNOWN,	/* 77*/
	UNKNOWN,	/* 78*/
	MWKEY_KP7,	/* 79*/
	MWKEY_KP8,	/* 80*/
	MWKEY_KP9,	/* 81*/
	MWKEY_KP_MINUS,	/* 82*/
	MWKEY_KP4,	/* 83*/
	MWKEY_KP5,	/* 84*/
	MWKEY_KP6,	/* 85*/
	MWKEY_KP_PLUS,	/* 86*/
	MWKEY_KP1,	/* 87*/
	MWKEY_KP2,	/* 88*/
	MWKEY_KP3,	/* 89*/
	MWKEY_KP0,	/* 90*/
	MWKEY_KP_PERIOD,/* 91*/
	UNKNOWN,	/* 92*/
	UNKNOWN,	/* 93*/
	UNKNOWN,	/* 94*/
	MWKEY_F11,	/* 95*/
	MWKEY_F12,	/* 96*/
	MWKEY_HOME,	/* 97*/
	MWKEY_UP,	/* 98*/
	MWKEY_PAGEUP,	/* 99*/
	MWKEY_LEFT,	/*100*/
	UNKNOWN,	/*101*/
	MWKEY_RIGHT,	/*102*/
	MWKEY_END,	/*103*/
	MWKEY_DOWN,	/*104*/
	MWKEY_PAGEDOWN,	/*105*/
	MWKEY_INSERT,	/*106*/
	MWKEY_DELETE,	/*107*/
	MWKEY_KP_ENTER,	/*108*/
	MWKEY_RCTRL,	/*109*/
	UNKNOWN,	/*110*/
	MWKEY_PRINT,	/*111*/
	MWKEY_KP_DIVIDE, /*112*/
	MWKEY_RALT,	/*113*/
	UNKNOWN,	/*114*/
	UNKNOWN,	/*115*/
	UNKNOWN,	/*116*/
	UNKNOWN,	/*117*/
	UNKNOWN,	/*118*/
	UNKNOWN,	/*119*/
	UNKNOWN,	/*120*/
	UNKNOWN,	/*121*/
	UNKNOWN,	/*122*/
	UNKNOWN,	/*123*/
	UNKNOWN,	/*124*/
	UNKNOWN,	/*125*/
	UNKNOWN,	/*126*/
	UNKNOWN		/*127*/
};
#endif
