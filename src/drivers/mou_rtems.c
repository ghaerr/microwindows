/*
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 - Rosimildo da Silva
//           (c) 2004 - Andrey Astafiev
//
// MODULE DESCRIPTION: 
// This module implements the mouse driver for systems that implements 
// the Micro Input Device interface. This driver is not specific in any way
// to RTEMS. It could be used with any sustem that implements such interface.
//
// The skeleton of this driver was based on standard Microwindows drivers
// and file input_rtems.c written by Rosimildo da Silva.
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

/* prototypes of the mouse driver */
static int      MWMou_Open (MOUSEDEVICE *pmd);
static void     MWMou_Close (void);
static int      MWMou_GetButtonInfo (void);
static void     MWMou_GetDefaultAccel (int *pscale,int *pthresh);
static int      MWMou_Read (MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

MOUSEDEVICE mousedev = 
{
        MWMou_Open,
	MWMou_Close,
	MWMou_GetButtonInfo,
	MWMou_GetDefaultAccel,
	MWMou_Read,
	NULL,
	MOUSE_NORMAL
};


static int mou_fd = -1;
struct MW_UID_MESSAGE m_mou = { 0 };
static const char *Q_NAME        = "MWQ";
#define            Q_MAX_MSGS      20
#define            MOUSE_DEVICE    "/dev/mouse"


/*
 * Open up the mouse device.
 */
static int
MWMou_Open (MOUSEDEVICE *pmd)
{
        int rc;
	m_mou.type = MV_UID_INVALID;
	rc = uid_open_queue (Q_NAME, O_CREAT | O_RDWR, Q_MAX_MSGS);
	mou_fd = open (MOUSE_DEVICE, O_NONBLOCK);
	uid_register_device (mou_fd, Q_NAME);
	return 2;
}

/*
 * Close the mouse device.
 */
static void
MWMou_Close (void)
{
        uid_unregister_device (mou_fd);
	uid_close_queue ();
	close (mou_fd);
}

/*
 * Get mouse buttons supported
 */
static int
MWMou_GetButtonInfo (void)
{
        return 0;
}

/*
 * Get default mouse acceleration settings
 */
static void
MWMou_GetDefaultAccel (int *pscale,int *pthresh)
{
        *pscale = SCALE;
	*pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
static int
MWMou_Read (MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
        /* check if a new mouse event has been posted */
        if (m_mou.type != MV_UID_INVALID)
	{
                /* check which return to send up ... */
        	int rc = (m_mou.type == MV_UID_REL_POS) ? 1 : 2;

		*bp = m_mou.m.pos.btns;
		*dx = m_mou.m.pos.x;
		*dy = m_mou.m.pos.y;
		*dz = m_mou.m.pos.z;
		/* consume event */
		m_mou.type = MV_UID_INVALID;
		return rc;
	}
	return 0;
}
