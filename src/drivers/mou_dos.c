/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * DOS Mouse Driver, uses int 33h
 * Added fix for DJGPP 32 bit compilation
 */
#include <dos.h>

#ifndef __PACIFIC__
#include <bios.h>
#endif

#include "device.h"

#define	SCALE	1	/* default scaling factor for acceleration WAS 3*/
#define	THRESH	10	/* default threshhold for acceleration WAS 0*/

static int  	MOU_Open(MOUSEDEVICE *pmd);
static void 	MOU_Close(void);
static int  	MOU_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	MOU_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz,int *bp);
static int	MOU_Poll(void);

MOUSEDEVICE mousedev = {
	MOU_Open,
	MOU_Close,
	MOU_GetButtonInfo,
	MOU_GetDefaultAccel,
	MOU_Read,
	MOU_Poll
};

static int mouse_fd;

/*
 * Open up the mouse device.
 */
static int
MOU_Open(MOUSEDEVICE *pmd)
{
	union REGS 	regset;

	/* init mouse*/
	regset.x.ax = 0;
	int86(0x33, &regset, &regset);

	/* set mickey-to-pixel ratio*/
	regset.x.ax = 0x0f;
	regset.x.cx = 16;	/* # mickeys per 8 pixels x direction (default 8)*/
	regset.x.dx = 32;	/* # mickeys per 8 pixels y direction (default 16)*/
	int86(0x33, &regset, &regset);

	/* read motion counters to reset*/
	regset.x.ax = 0x0b;
	int86(0x33, &regset, &regset);

	return 1;
}

/*
 * Close the mouse device.
 */
static void
MOU_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
MOU_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
MOU_GetDefaultAccel(int *pscale,int *pthresh)
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
MOU_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	union REGS 	regset;
	int		buttons;

	/* read motion counters*/
	regset.x.ax = 0x0b;
	int86(0x33, &regset, &regset);

	*dx = (short)regset.x.cx;
	*dy = (short)regset.x.dx;
	*dz = 0;

	/* read button status*/
	regset.x.ax = 3;
	int86(0x33, &regset, &regset);

	buttons = 0;
	if(regset.x.bx & 01)
		buttons |= MWBUTTON_L;
	if(regset.x.bx & 02)
		buttons |= MWBUTTON_R;
	if(regset.x.bx & 04)
		buttons |= MWBUTTON_M;
	*bp = buttons;

	return 1;
}

static int
MOU_Poll(void)
{
	return 1;
}
