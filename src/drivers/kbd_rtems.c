/*
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000 - Rosimildo da Silva
//           (c) 2004 - Andrey Astafiev             
//  
// MODULE DESCRIPTION: 
// This module implements the PC keyboard driver for systems that implements 
// the Micro Input Device interface. This driver is not specific in any way
// to RTEMS. It could be used with any sustem that implements such interface.
//
// The skeleton of this driver was based on standard Microwindows drivers
// and input_rtems.c file wrtten by Rosimildo da Silva.
//
// MODIFICATION/HISTORY:
//
// Revision 1.1.1.1  2001/06/21 06:32:41  greg
// Microwindows pre8 with patches
//
// Revision 1.1.1.1  2001/06/05 03:44:01  root
// First import of 5/5/2001 Microwindows to CVS
//
//
/////////////////////////////////////////////////////////////////////////////
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <rtems/mw_uid.h>
#include "device.h"

extern int close (int fd); /* RTEMS does not include close() in stdio.h */

#define    SCALE        3    /* default scaling factor for acceleration */
#define    THRESH       5    /* default threshhold for acceleration */

/* prototypes of the Kbd driver */
int     MWKbd_Open (KBDDEVICE *pkd);
void    MWKbd_Close (void);
void	MWKbd_GetModifierInfo (MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
int	MWKbd_Read (MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);

KBDDEVICE kbddev = {
        MWKbd_Open,
	MWKbd_Close,
	MWKbd_GetModifierInfo,
	MWKbd_Read,
	NULL
};

struct MW_UID_MESSAGE m_kbd = { 0 };
static int kbd_fd   = -1;
static const char *Q_NAME        = "MWQ";
#define            Q_MAX_MSGS      20


/*
 * Open the keyboard.
 */
int
MWKbd_Open (KBDDEVICE *pkd)
{
        int rc;
	m_kbd.type = MV_UID_INVALID;
	kbd_fd = fileno (stdin);
	rc = uid_open_queue (Q_NAME, O_CREAT | O_RDWR, Q_MAX_MSGS);
	uid_register_device (kbd_fd, Q_NAME);
	return 1;
}

/*
 * Close the keyboard.
 */
void
MWKbd_Close (void)
{
        uid_unregister_device (kbd_fd);
	uid_close_queue ();
	close (kbd_fd);
}

/*
 * Return the possible modifiers for the keyboard.
 */
void
MWKbd_GetModifierInfo (MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
        *modifiers = 0;      /* no modifiers available */
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */
int
MWKbd_Read (MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
        /* check if new KBD event has been posted */
        if( m_kbd.type != MV_UID_INVALID )
        {
	        *buf = m_kbd.m.kbd.code;
//	          *modifiers = m_kbd.m.kbd.modifiers;
		*modifiers = 0;

		/* consume event */
		m_kbd.type = MV_UID_INVALID;
		return 1;
	}
	return 0;
}
