/*
 * tslib touchscreen driver
 *
 * Copyright (c) 2009, TimeTerminal AB
 *
 * Based on mou_touchscreen.c:
 * Copyright (c) 2003, Century Software, Inc.
 * Written by Jordan Crouse <jordan@cosmicpenguin.net>
 * 
 * Download tslib from: http://sourceforge.net/projects/tslib.berlios/
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "device.h"

#include <tslib.h>

static int pd_fd = -1;
static struct tsdev *ts = NULL;

extern SCREENDEVICE scrdev;

static int PD_Open(MOUSEDEVICE *pmd)
{
	char *tsdevice = NULL;

	if ((tsdevice = getenv("TSLIB_TSDEVICE")) != NULL) {
		ts = ts_open(tsdevice, 1);
	} else {
		ts = ts_open("/dev/input/event0", 1);
	}

	if (!ts) {
		EPRINTF("Error opening touchscreen device [%s]: %s\n",
			tsdevice, strerror(errno));
		return DRIVER_FAIL;
	}

	if (ts_config(ts)) {
		EPRINTF("Error configuring touchscreen device: %s\n",
			strerror(errno));
		ts_close(ts);
		return DRIVER_FAIL;
	}

	GdHideCursor(&scrdev);
	return ts_fd(ts);
}

static void PD_Close(void)
{
	/* Close the touch panel device. */

	if(pd_fd < 0) return;

	ts_close(ts);
	pd_fd = -1;
}

static int PD_GetButtonInfo(void)
{
	/* get "mouse" buttons supported */
	return MWBUTTON_L;
}

static void PD_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = 3;
	*pthresh = 5;
}

static int PD_Read(MWCOORD *px, MWCOORD *py, MWCOORD *pz, int *pb)
{
	struct ts_sample samp;
	int ret;

	ret = ts_read(ts, &samp, 1);

	if (ret <= 0) {
		if (errno == EINTR || errno == EAGAIN)
			return MOUSE_NODATA;
		EPRINTF("Error reading from touchscreen: %s\n", strerror(errno));
		return MOUSE_FAIL;
	}

	*px = samp.x;
	*py = samp.y;
	*pb = (samp.pressure) ? MWBUTTON_L : 0;
	*pz = samp.pressure;

	if(!*pb)
		return MOUSE_NOMOVE;	/* report position but don't move mouse cursor*/
	return MOUSE_ABSPOS;
}

MOUSEDEVICE mousedev = {
	PD_Open,
	PD_Close,
	PD_GetButtonInfo,
	PD_GetDefaultAccel,
	PD_Read,
	NULL,
	MOUSE_RAW   /* Input filter flags */
};
