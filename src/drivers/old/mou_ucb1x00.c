/*
 * Touch screen driver for ucb1100, ucb1200 and ucb1300 based systems.
 *
 * Copyright (c) 2000 Century Software Embedded Technologies
 * Copyright (c) 2002 Alex Holden <alex@alexholden.net>
 *
 * Requires /dev/ucb1x00-ts (c 10 14).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "device.h"

/* file descriptor for touch panel */
static int pd_fd = -1;

/* Set this for the appropriate filename */
#define DEVICE "/dev/ucb1x00-ts"

/* This is used to set the default calibration settings,
   for ease of use.  Most often this will be off, so people 
   should calculate their own */
static MWTRANSFORM default_transform = {
	-46141, 0, 44157853, 0, -35869, 33645300, 65536
};

extern SCREENDEVICE scrdev;

struct ts_event {
	unsigned short	pressure;
	unsigned short	x;
	unsigned short	y;
	unsigned short	pad;
	struct timeval	stamp;
};

static int
PD_Open(MOUSEDEVICE * pmd)
{
	/*
	 * open up the touch-panel device.
	 * Return the fd if successful, or negative if unsuccessful.
	 */
	if ((pd_fd = open(DEVICE, O_NONBLOCK)) < 0) {
		EPRINTF("Error %d opening touch panel\n", errno);
		return -1;
	}

	/* This is the default transform for this device */
	/* We set it here, so that the system will come up normally */
	GdSetTransform(&default_transform);

	/* This should normally be disabled, but we leave it on for debugging */
	/*GdHideCursor(&scrdev);*/

	return pd_fd;
}

static void
PD_Close(void)
{
	/* Close the touch panel device. */
	if (pd_fd < 0)
		return;

	close(pd_fd);
	pd_fd = -1;
}

static int
PD_GetButtonInfo(void)
{
	/* get "mouse" buttons supported */
	return MWBUTTON_L;
}

static void
PD_GetDefaultAccel(int *pscale, int *pthresh)
{
	*pscale = 3;
	*pthresh = 5;
}

static int
PD_Read(MWCOORD * px, MWCOORD * py, MWCOORD * pz, int *pb, int mode)
{
	/* read a data point */
	struct ts_event event;
	int bytes_read;

	bytes_read = read(pd_fd, &event, sizeof(event));

	if (bytes_read != sizeof(event)) {
		if (bytes_read == -1) {
			if (errno == EINTR || errno == EAGAIN)
				return 0;
			EPRINTF("Error %d reading from touch panel\n", errno);
			return -1;
		}

		EPRINTF("Wrong number of bytes %d read from touch panel "
			"(expected %d)\n", bytes_read, sizeof(event));
		return 0;
	}

	*px = event.x;
	*py = event.y;
	*pz = event.pressure;
	*pb = (event.pressure > 50) ? MWBUTTON_L : 0;

	if (!*pb)
		return 3;
	return 2;
}

MOUSEDEVICE mousedev = {
	PD_Open,
	PD_Close,
	PD_GetButtonInfo,
	PD_GetDefaultAccel,
	PD_Read,
	NULL,
	MOUSE_TRANSFORM		/* flags*/
};
