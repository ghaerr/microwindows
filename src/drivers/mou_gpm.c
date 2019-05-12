/* 
 * Copyright (c) 1999, 2002 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1999 Alex Holden
 * Copyright (c) 1991 David I. Bell
 *
 * GPM Mouse Driver
 *
 * Rewritten to understand the Logitech Mouseman protocol which GPM
 * produces on /dev/gpmdata when in repeater mode. Remember to start
 * GPM with the -R flag or it won't work. (gpm -R -t ps2)
 *
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "device.h"

#define	SCALE		2	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#define GPM_DEV_FILE	"/dev/gpmdata"
#define GPM_WHEEL_FILE	"/dev/gpmwheel"

static int  	GPM_Open(MOUSEDEVICE *pmd);
static void 	GPM_Close(void);
static int  	GPM_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	GPM_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

MOUSEDEVICE mousedev = {
	GPM_Open,
	GPM_Close,
	GPM_GetButtonInfo,
	MOU_GetDefaultAccel,
	GPM_Read,
	NULL,
	MOUSE_NORMAL	/* flags*/
};

static int mouse_fd;
static int wheel_fd;

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
GPM_Open(MOUSEDEVICE *pmd)
{
	mouse_fd = open(GPM_DEV_FILE, O_NONBLOCK);

	if (mouse_fd < 0)
		return DRIVER_FAIL;

	wheel_fd = open(GPM_WHEEL_FILE, O_NONBLOCK);

	return mouse_fd;
}

/*
 * Close the mouse device.
 */
static void
GPM_Close(void)
{
	if (mouse_fd > 0)
		close(mouse_fd);
	mouse_fd = -1;
	if (wheel_fd > 0)
		close(wheel_fd);
	wheel_fd = -1;
}

/*
 * Get mouse buttons supported
 */
static int
GPM_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
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
GPM_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	static unsigned char buf[5];
	static int nbytes;
	int n;
	char wheelbyte = 0;
	static char lastwheelbyte;

    if (wheel_fd > 0) {
	    while((n = read(wheel_fd, &buf[nbytes], 1 - nbytes))) {
		    if(n < 0) break;
		    nbytes += n;
         }
	     if (nbytes>0) wheelbyte = (signed char)(buf[0]);

	     //simulate release button by returning *bp=0
	     if (wheelbyte==lastwheelbyte) {
	 	    wheelbyte = 0;
	 	    buf[0]=buf[1]=buf[2]=buf[3]=buf[4]=0; //to be save	
	     }
	     lastwheelbyte = wheelbyte;
    }

	nbytes = 0; //clear again

	while((n = read(mouse_fd, &buf[nbytes], 5 - nbytes))) {
		if(n < 0) {
			if ((errno == EINTR) || (errno == EAGAIN))
				return MOUSE_NODATA;
			else return MOUSE_FAIL;
		}

		nbytes += n;

		if(nbytes == 5) {
			// just for wheelbyte - 4=up, 5=down
			if (wheelbyte>0){
				if (wheelbyte==4) *bp = MWBUTTON_SCROLLUP;
				if (wheelbyte==5) *bp = MWBUTTON_SCROLLDN;
			} else {
				// button data matches defines, no conversion
				*bp = (~buf[0]) & 0x07;
			}
			if (*bp>256) *bp = 0; //remove invalid values

			//buttons done, handle mouse movement now
			*dx = (signed char)(buf[1]) + (signed char)(buf[3]);
			*dy = -((signed char)(buf[2]) + (signed char)(buf[4]));
			*dz = 0;
			nbytes = 0;
			return MOUSE_RELPOS;
		}

	}
	return MOUSE_NODATA;
}
