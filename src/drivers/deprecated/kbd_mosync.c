/*
 * Microwindows keyboard driver for MoSync
 *
 * Copyright (c) 2010 DI (FH) Ludwig Ertl / CSP GmbH
 *
 */
#include <ma.h>
#include "device.h"

extern MAEvent g_event;

static MWKEY MapKey (int key);
static int  MOS_Open(KBDDEVICE *pkd);
static void MOS_Close(void);
static void MOS_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  MOS_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);

KBDDEVICE kbddev = {
	MOS_Open,
	MOS_Close,
	MOS_GetModifierInfo,
	MOS_Read,
	NULL
};

static int
MOS_Open(KBDDEVICE * pkd)
{
	return 0;
}

static void
MOS_Close(void)
{
}

static void
MOS_GetModifierInfo(MWKEYMOD * modifiers, MWKEYMOD * curmodifiers)
{
	if (modifiers)
		*modifiers = 0;	/* no modifiers available */
	if (curmodifiers)
		*curmodifiers = 0;
}

// 0	-	No key pressed
// 1	-	Key down
// 2	-	Key released
static int
MOS_Read(MWKEY * kbuf, MWKEYMOD * modifiers, MWSCANCODE * scancode)
{
	*modifiers = 0;

	switch (g_event.type)
	{
	case EVENT_TYPE_KEY_PRESSED:
		*kbuf = MapKey(g_event.key);
		*scancode = g_event.nativeKey;
		return 1;
	case EVENT_TYPE_KEY_RELEASED:
		*kbuf = MapKey(g_event.key);
		*scancode = g_event.nativeKey;
		return 2;
	}

	return 0;
}

#define MAP_KEY(key) case MAK_##key: return MWKEY_##key;

static MWKEY MapKey (int key)
{
	switch (key)
	{
	MAP_KEY(LEFT);
	MAP_KEY(RIGHT);
	MAP_KEY(UP);
	MAP_KEY(DOWN);
	MAP_KEY(INSERT);
	MAP_KEY(DELETE);
	MAP_KEY(HOME);
	MAP_KEY(END);
	MAP_KEY(PAGEUP);
	MAP_KEY(PAGEDOWN);
	MAP_KEY(KP0);
	MAP_KEY(KP1);
	MAP_KEY(KP2);
	MAP_KEY(KP3);
	MAP_KEY(KP4);
	MAP_KEY(KP5);
	MAP_KEY(KP6);
	MAP_KEY(KP7);
	MAP_KEY(KP8);
	MAP_KEY(KP9);
	MAP_KEY(KP_PERIOD);
	MAP_KEY(KP_DIVIDE);
	MAP_KEY(KP_MULTIPLY);
	MAP_KEY(KP_MINUS);
	MAP_KEY(KP_PLUS);
	MAP_KEY(KP_ENTER);
	MAP_KEY(KP_EQUALS);
	MAP_KEY(LSHIFT);
	MAP_KEY(RSHIFT);
	MAP_KEY(LCTRL);
	MAP_KEY(RCTRL);
	MAP_KEY(LALT);
	MAP_KEY(RALT);
	MAP_KEY(MENU);
	case MAK_CLEAR: return MWKEY_BACKSPACE;
	case MAK_FIRE: return MWKEY_ACCEPT;
	case MAK_SOFTLEFT: return MWKEY_LMETA;
	case MAK_SOFTRIGHT: return MWKEY_RMETA;
	default: return key;
	}
	return MWKEY_UNKNOWN;
}

