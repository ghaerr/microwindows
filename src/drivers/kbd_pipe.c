/*
 * Copyright (C) 2000 by VTech Informations LTD.
 *
 * This source code is released under MPL
 * 
 * Vladimir Cotfas <vladimircotfas@vtech.ca> Aug 31, 2000
 *
 * Named pipe Keyboard Driver for Microwindows
 * See src/demos/nxkbd/srvconn.c for example client-side driver
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "device.h"

static char kbdName[] = "/tmp/.nano-X-softkbd";
#define KBD_NAMED_PIPE	kbdName	

#define _SOFT_DEBUG	0

static int kbd_fd = -1; /* keyboar driver FIFO */

static int  soft_Open(KBDDEVICE* pkd);
static void soft_Close();
static void soft_GetModifierInfo(MWKEYMOD* modifiers, MWKEYMOD* curmodifiers);
static int  soft_Read(MWKEY* buf, MWKEYMOD* modifiers, MWSCANCODE* scancode);

KBDDEVICE kbddev = {
    soft_Open,
    soft_Close,
    soft_GetModifierInfo,
    soft_Read,
    NULL
};

/*
 * Open the keyboard.
 */
static int
soft_Open(KBDDEVICE* pkd)
{
        struct stat s;

        /* Check if the file already exists: */
        if (!stat(KBD_NAMED_PIPE, &s)) {
                if (unlink(KBD_NAMED_PIPE) < 0)
                        return -1;
        }
	if (mkfifo(KBD_NAMED_PIPE, 0600) < 0) {
		EPRINTF("mkfifo() error %d ('%s')\n", \
			errno, sys_errlist[errno]);
		return -1;
	}

        /* Open the named pipe */
        if ((kbd_fd = open(KBD_NAMED_PIPE, O_RDONLY | O_NONBLOCK)) < 0) {
		EPRINTF("open() error %d ('%s')\n", \
			errno, sys_errlist[errno]);
                return -1;
	}
	
    	return kbd_fd;
}

/*
 * Close the keyboard.
 */
static void
soft_Close()
{
#if _SOFT_DEBUG
    EPRINTF("kbd_soft.c: soft_Close(): closing named pipe %d\n", kbd_fd);
#endif
    if (kbd_fd >= 0)
	    close(kbd_fd);
    kbd_fd = -1;

    unlink(KBD_NAMED_PIPE);
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
soft_GetModifierInfo(MWKEYMOD* modifiers, MWKEYMOD* curmodifiers)
{
#if _SOFT_DEBUG
    EPRINTF("kbd_soft.c: soft_GetModifierInfo(): being asked about modifiers\n");
#endif
    if (modifiers)
        *modifiers = 0;			/* no modifiers available */
    if (curmodifiers)
        *curmodifiers = 0;		/* no modifiers available */
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */

static int
soft_Read(MWKEY* buf, MWKEYMOD* modifiers, MWSCANCODE* scancode)
{
    int cc;
    *modifiers = 0;         /* no modifiers yet */
    *scancode = 0;          /* no scancode yet */

    cc = read(kbd_fd, buf, 1); /* this is NON BLOCKING read */

    if (cc > 0) {
	if(*buf == 0x1b)
	      return -2;      /* special case ESC*/
#if _SOFT_DEBUG
    	EPRINTF("kbd_soft.c: soft_Read(): read '%c', cc = %d\n", 
	        buf[0], cc); 
	fflush(NULL);
#endif
	return 1;
    }

    return 0;
}
