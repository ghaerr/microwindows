/* 
 * Linux VR touchscreen driver 
 * This is a specific touchscreen driver for the VR based devices
 *
 * Copyright (C) 1999 Bradley D. LaRonde <brad@ltc.com>
 * Portions Copyright (c) 2001 Kevin Oh <webmaster@prg-lib.net>
 * Portions Copyright (c) 1999, 2000, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include "device.h"

/* The following devices are supported by this driver:
 * TOUCHSCREEN_HELIO - The Helio
 * TOUCHSCREEN_EPLANET - Embedded Planet
 */

#ifdef TOUCHSCREEN_HELIO
#define TS_DEVICE "Helio"
#define TS_DEVICE_FILE "/dev/tpanel"
#endif

#ifdef TOUCHSCREEN_EPLANET
#define TS_DEVICE "Embedded Planet"
#define TS_DEVICE_FILE "/dev/tpanel"
#include <linux/tpanel.h>
#endif

#ifndef TS_DEVICE
#error "You didn't define a device for the VR-Linux touchscreen driver!"
#endif

static int PD_Open(MOUSEDEVICE *pmd)
{	
#ifdef TOUCHSCREEN_EPLANET
	struct scanparam s;
	int settle_upper_limit;
	int result;
#endif
	
	if((pd_fd = open(TS_DEVICE_FILE, O_NONBLOCK)) < 0) {
		EPRINTF("[%s} Error %d opening touchscreen device [%s]\n", 
			TS_DEVICE, errno, TS_DEVICE_FILE);
		return -1;
	}

#ifdef TOUCHSCREEN_EPLANET
	/* set interval to 5000us (200Hz) */
	s.interval = 5000;
	/*
	 * Upper limit on settle time is approximately (scan_interval / 5) - 60
	 * (5 conversions and some fixed overhead)
	 * The opmtimal value is the lowest that doesn't cause significant
	 * distortion.
	 * 50% of upper limit works well on my Clio.  25% gets into distortion.
	 */
	settle_upper_limit = (s.interval / 5) - 60;
	s.settletime = settle_upper_limit * 50 / 100;
	result = ioctl(pd_fd, TPSETSCANPARM, &s);
	if ( result < 0 )
		EPRINTF("Error %d, result %d setting scan parameters.\n",
			result, errno);
#endif

	GdHideCursor(&scrdev);  
	return pd_fd;
}

static void PD_Close(void)
{
 	/* Close the touch panel device. */
	if (pd_fd > 0)
		close(pd_fd);
	pd_fd = 0;
}

static int PD_GetButtonInfo(void)
{
 	/* get "mouse" buttons supported */
	return MWBUTTON_L;
}

static void PD_GetDefaultAccel(int *pscale,int *pthresh)
{
	/*
	 * Get default mouse acceleration settings
	 * This doesn't make sense for a touch panel.
	 * Just return something inconspicuous for now.
	 */
	*pscale = 3;
	*pthresh = 5;
}

static int PD_Read(MWCOORD *px, MWCOORD *py, MWCOORD *pz, int *pb) {

#ifdef TOUCHSCREEN_HELIO
	short data[3];
#else
	short data[6];
#endif
	int bytes_read;

	bytes_read = read(pd_fd, &event, sizeof(event));
	
	if(bytes_read != sizeof(event)) {
		if(bytes_read == -1) {
			if(errno == EINTR || errno == EAGAIN) return 0;
			EPRINTF("[%s] Error %d reading from touch panel\n", TS_DEVICE, errno);
			return -1;
		}
		
		EPRINTF("[%s] Wrong number of bytes %d read from touch panel "
			"(expected %d)\n", TS_DEVICE, bytes_read, sizeof(event));
		return 0;
	}
	
#ifndef TOUCHSCREEN_HELIO
	
	if (data[0] & 0x2000) return 0;  /* Lost the data */
	
	if ((data[0] & 0x8000) == 0) {
		if ((data[0] & 0x4000) == 0) {
			*pb = 0;
			return 3;
		}

		return 0;  /* No data yet */
	}

	*px = data[2] - data[1];
	*py = data[4] - data[3];
	*pz = data[5];
	
	*pb = MWBUTTON_L;
#else
        *px = data[1];
        *py = data[2];
	*pz = data[0] ? 2000 : 0;
#endif

	return 2;
}

MOUSEDEVICE mousedev = {
	PD_Open,
	PD_Close,
	PD_GetButtonInfo,
	PD_GetDefaultAccel,
	PD_Read,
	NULL
	MOUSE_TRANSFORM /* input filter flags */
};
