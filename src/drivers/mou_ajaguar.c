/*
 * Copyright (c) 2019 Jean-Paul Mari <djipi.mari@gmail.com>
 *
 * Atari Jaguar Mouse Driver
 */
#include <stdio.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	AJAGUAR_Open(MOUSEDEVICE *pmd);
static void 	AJAGUAR_Close(void);
static int  	AJAGUAR_GetButtonInfo(void);
static void	AJAGUAR_GetDefaultAccel(int *pscale,int *pthresh);
static int  	AJAGUAR_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	AJAGUAR_Poll(void);

MOUSEDEVICE mousedev __attribute__ ((section (".mwjagdata"))) = {
	AJAGUAR_Open,
	AJAGUAR_Close,
	AJAGUAR_GetButtonInfo,
	AJAGUAR_GetDefaultAccel,
	AJAGUAR_Read,
	AJAGUAR_Poll
};

/*
 * Poll for events
 */

static int AJAGUAR_Poll(void)
{
  return 0;
}

/*
 * Open up the mouse device.
 */
static int AJAGUAR_Open(MOUSEDEVICE *pmd)
{
	return DRIVER_OKNULLDEV;	/* ok, no mouse*/
}

/*
 * Close the mouse device.
 */
static void AJAGUAR_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int AJAGUAR_GetButtonInfo(void)
{
	return 0;
}

/*
 * Get default mouse acceleration settings
 */
static void AJAGUAR_GetDefaultAccel(int *pscale,int *pthresh)
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
static int AJAGUAR_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	return MOUSE_NODATA;
}
