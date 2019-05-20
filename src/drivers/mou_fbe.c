/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Framebuffer Emulator mouse driver
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  Mice_Open(MOUSEDEVICE *pmd);
static void Mice_Close(void);
static int  Mice_GetButtonInfo(void);
static void	Mice_GetDefaultAccel(int *pscale,int *pthresh);
static int  Mice_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

static int      mouse_fd = -1;

MOUSEDEVICE mousedev = {
	Mice_Open,
	Mice_Close,
	Mice_GetButtonInfo,
	Mice_GetDefaultAccel,
	Mice_Read,
	NULL,
	MOUSE_NORMAL    /* flags*/
};

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
Mice_Open(MOUSEDEVICE *pmd)
{
	mouse_fd = open(MW_PATH_FBE_MOUSE, O_RDONLY | O_NONBLOCK);

	if (mouse_fd < 0)
		return MOUSE_FAIL;

	return DRIVER_OKFILEDESC(mouse_fd);
}

/*
 * Close the mouse device.
 */
static void
Mice_Close(void)
{
	if (mouse_fd >= 0)
		close(mouse_fd);
	mouse_fd = -1;
}

/*
 * Get mouse buttons supported
 */
static int
Mice_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

/*
 * Get default mouse acceleration settings
 */
static void
Mice_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/*
	FBE read mouse protocol - 6 bytes
		buf[0] is 0xF4,
		buf[1] is mouse button status using MWBUTTON_*
		buf[2-3] is absolute X position (little endian short)
		buf[4-5] is absolute Y position (little endian short)
*/
static int
Mice_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	unsigned char buf[6];
	int n, buttons, x, y;

	n = read(mouse_fd, buf, sizeof(buf));
	if (n != 6 || buf[0] != 0xF4)
		return MOUSE_NODATA;

	buttons = buf[1];
	x = *(short *)&buf[2];
	y = *(short *)&buf[4];

	*bp = buttons;
	*dx = x;
	*dy = y;       
	*dz = 0;

	return MOUSE_ABSPOS;
}
