/*
 * Generic touchscreen driver 
 *
 * Copyright (c) 2003, Century Software, Inc.
 * Written by Jordan Crouse <jordan@cosmicpenguin.net>
 */

/* The following devices are supported by this driver:
   TOUCHSCREEN_ZAURUS - Sharp Zaurus S5500 
   TOUCHSCREEN_IPAQ - Compaq Ipaq 3x00
   TOUCHSCREEN_TUXSCREEN - Shannon IS2630
   TOUCHSCREEN_ADS - Applied Data Systems Graphics Client+ devices
   TOUCHSCREEN_ADS7846 - TI ADS6847 (PSI OMAP Innovator)
*/

/* To add a new device, add a new item to the config file
   (MOUSEZZZ=Y for example), and hack drivers/Makefile
   to add the define (-DTOUCHSCREEN_ZZZ for example).  Finally,
   add a new header file to drivers (touchscreen_zzz.h for example)
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "device.h"

#ifdef TOUCHSCREEN_ZAURUS
#include "touchscreen_zaurus.h"
#endif

#ifdef TOUCHSCREEN_IPAQ
#include "touchscreen_ipaq.h"
#endif

#ifdef TOUCHSCREEN_ADS
#include "touchscreen_ads.h"
#endif

#ifdef TOUCHSCREEN_ADS7846
#include "touchscreen_ads7846.h"
#endif

/* The tuxscreen just uses the generic ucb1x00 driver */
#ifdef TOUCHSCREEN_TUXSCREEN
#include "touchscreen_ucb1x00.h"
#endif

#ifndef TS_DEVICE
#error "You didn't define a device for the generic touchscreen driver!"
#endif

static int pd_fd = -1;
extern SCREENDEVICE scrdev;

static int PD_Open(MOUSEDEVICE *pmd)
{	       
	if((pd_fd = open(TS_DEVICE_FILE, O_NONBLOCK)) < 0) {
		EPRINTF("Error %d opening %s touchscreen device [%s]\n", 
			errno, TS_DEVICE, TS_DEVICE_FILE);
		return -1;
	}

	GdHideCursor(&scrdev);  
	return pd_fd;
}

static void PD_Close(void)
{
	/* Close the touch panel device. */
 
	if(pd_fd < 0) return;
 
	close(pd_fd);
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

static int PD_Read(MWCOORD *px, MWCOORD *py, MWCOORD *pz, int *pb, int mode)
{
	struct ts_event event;
	int bytes_read;
  
	/* read a data point */
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

	*px = event.x;
	*py = event.y;

#if defined(TOUCHSCREEN_IPAQ) || defined(TOUCHSCREEN_ADS7846)
	*pb = (event.pressure) ? MWBUTTON_L : 0;
#else
	*pb = (event.pressure > 50) ? MWBUTTON_L : 0;
#endif

	*pz = event.pressure;
  
	if(!*pb)
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
	MOUSE_TRANSFORM   /* Input filter flags */
};
