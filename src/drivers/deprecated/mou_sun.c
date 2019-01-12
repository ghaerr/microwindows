/*
 * Copyright (c) 2002 Greg Haerr <greg@censoft.com>
 *
 * Sun Mouse Driver
 *
 * Rewritten to understand the Sun mouse protocol
 *
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#define SUN_DEV_FILE	"/dev/sunmouse"

static int  	SUN_Open(MOUSEDEVICE *pmd);
static void 	SUN_Close(void);
static int  	SUN_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	SUN_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

MOUSEDEVICE mousedev = {
	SUN_Open,
	SUN_Close,
	SUN_GetButtonInfo,
	MOU_GetDefaultAccel,
	SUN_Read,
	NULL
};

static int mouse_fd;

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
SUN_Open(MOUSEDEVICE *pmd)
{
        struct termios termios;

	mouse_fd = open(SUN_DEV_FILE, O_NONBLOCK);
	if (mouse_fd < 0)
		return -1;

	tcgetattr(mouse_fd, &termios);

	if(cfgetispeed(&termios) != B1200)
		cfsetispeed(&termios, B1200);

	termios.c_iflag = IGNBRK | IGNPAR;
	termios.c_oflag = 0;
	termios.c_lflag = 0;
	termios.c_line = 0;
	termios.c_cc[VTIME] = 0;
	termios.c_cc[VMIN] = 1;
	termios.c_cflag = CS8 | CSTOPB | CREAD | CLOCAL | HUPCL | B1200;

	tcsetattr(mouse_fd, TCSAFLUSH, &termios);

	return mouse_fd;
}

/*
 * Close the mouse device.
 */
static void
SUN_Close(void)
{
	if (mouse_fd > 0)
		close(mouse_fd);
	mouse_fd = -1;
}

/*
 * Get mouse buttons supported
 */
static int
SUN_GetButtonInfo(void)
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
SUN_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	static unsigned char buf[3];
	static int nbytes;
	int n;

	while((n = read(mouse_fd, &buf[0], 1))) {
		if(n < 0) {
			if ((errno == EINTR) || (errno == EAGAIN))
				return 0;
			else return -1;
		}
                break;
        }
        if((buf[0]&0xf8) != 0x80)
		return 0;
	nbytes = 1;
	while((n = read(mouse_fd, &buf[nbytes], 3 - nbytes))) {
		if(n < 0) {
			if ((errno == EINTR) || (errno == EAGAIN))
				return 0;
			else return -1;
		}

		nbytes += n;

		if(nbytes == 3) {
			/* button data matches defines, no conversion*/
			*bp =  (~buf[0]) & 0x07;
			*dx =  (signed char)(buf[1]);
			*dy = -(signed char)(buf[2]);
			*dz = 0;
			nbytes = 0;
			return 1;
		}
	}
	return 0;
}
