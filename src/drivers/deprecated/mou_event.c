/*
 * Generic event touchscreen driver
 *
 * Copyright (c) 2008, ELPA sas
 * Written by Davide Rizzo <davide@elpa.it>
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include "device.h"

#define TS_DEVICE "/dev/input/event0"

extern SCREENDEVICE scrdev;
static int fd = -1;

static int PD_Open(MOUSEDEVICE *pmd)
{
	int i, r;
	char fname[64];
	for(i = 0; i < 32; i++)
	{
		sprintf(fname, "/sys/class/input/event%d/device/capabilities/ev", i);
		fd = open(fname, O_RDONLY);
		if(fd < 0)
			continue;
		r = read(fd, fname, sizeof(fname));
		close(fd);
		if(r <= 0)
			continue;
		fname[r - 1] = '\0';
		if((strtoul(fname, NULL, 16) & (1 << EV_ABS)) == 0)
			continue;
		sprintf(fname, "/dev/input/event%d", i);
		fd = open(fname, O_RDONLY | O_NONBLOCK);
		if(fd < 0)
			continue;
		GdHideCursor(&scrdev);
		return fd;
	}
	EPRINTF("Error %d opening mouse input device\n", errno);
	return errno;
}

static void PD_Close(void)
{
 	if(fd < 0)
		return;
	close(fd);
	fd = -1;
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
	struct input_event event;
	int bytes_read;
	static int x, y, z;
	/* read a data point */
	while((bytes_read = read(fd, &event, sizeof(event))) == sizeof(event))
	{
		switch(event.type)
		{
		case EV_ABS:
			switch(event.code)
			{
			case ABS_X:
				x = event.value;
				break;
			case ABS_Y:
				y = event.value;
				break;
			}
			break;
		case EV_KEY:
			if(event.code == BTN_TOUCH)
				z = event.value;
			break;
		case EV_SYN:
			*px = x;
			*py = y;
			*pb = z ? MWBUTTON_L : 0;
			*pz = z;
			if(!*pb)
				return 3;
			return 2;
			break;
		}
	}
	if(bytes_read == -1)
	{
		if(errno == EINTR || errno == EAGAIN) return 0;
		EPRINTF("[%s] Error %d reading from touch panel\n", TS_DEVICE, errno);
		return -1;
	}
	if(bytes_read != 0)
	{
		EPRINTF("[%s] Wrong number of bytes %d read from touch panel "
		"(expected %d)\n", TS_DEVICE, bytes_read, sizeof(event));
		return -1;
	}
	return 0;
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
