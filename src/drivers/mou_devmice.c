/*
 * Copyright (c) 1999, 2019 Greg Haerr <greg@censoft.com>
 *
 * Kurt Nalty 2019 
 *	5/20/19 added scrollwheel support <greg@censoft.com>
 *
 * /dev/input/mice Mouse Driver
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#define MICE_DEV_FILE    "/dev/input/mice"

static int  	Mice_Open(MOUSEDEVICE *pmd);
static void 	Mice_Close(void);
static int  	Mice_GetButtonInfo(void);
static void	Mice_GetDefaultAccel(int *pscale,int *pthresh);
static int  	Mice_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

static int      mouse_fd = -1;

/* sequence to mouse device to send ImPS/2 events*/
static const unsigned char imps2[] = { 0xf3, 200, 0xf3, 100, 0xf3, 80 };

/* Interface to Mouse Device Driver*/
// (from device.h)

//typedef struct _mousedevice {
//        int     (*Open)(struct _mousedevice *);
//        void    (*Close)(void);
//        int     (*GetButtonInfo)(void);
//        void    (*GetDefaultAccel)(int *pscale,int *pthresh);
//        int     (*Read)(MWCOORD *dx,MWCOORD *dy,MWCOORD *dz,int *bp);
//        int     (*Poll)(void);       /* not required if have select() */
//        int     flags;               /* raw, normal, transform flags  */
//} MOUSEDEVICE;

// #define MOUSE_NORMAL            0x0000  /* mouse in normal mode*/
// #define MOUSE_RAW               0x0001  /* mouse in raw mode*/
// #define MOUSE_TRANSFORM         0x0002  /* perform transform*/


/////////////////////////////////////////////

MOUSEDEVICE mousedev = {
	Mice_Open,
	Mice_Close,
	Mice_GetButtonInfo,
	Mice_GetDefaultAccel,
	Mice_Read,
	NULL,
	MOUSE_NORMAL    /* flags*/
};

/////////////////////////////////////////////

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
Mice_Open(MOUSEDEVICE *pmd)
{
	unsigned char buf[4];

	mouse_fd = open(MICE_DEV_FILE, O_RDWR | O_NONBLOCK);

    /* switch the mouse to ImPS/2 protocol*/
	if (write(mouse_fd, imps2, sizeof(imps2)) != sizeof(imps2))
		EPRINTF("Can't switch to ImPS/2 protocol\n");
	if (read(mouse_fd, buf, 4) != 1 || buf[0] != 0xF4)
		EPRINTF("Failed to switch to ImPS/2 protocol.\n");

	if (mouse_fd < 0)
		return MOUSE_FAIL;

	return mouse_fd;
}

/////////////////////////////////////////////

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

/////////////////////////////////////////////

/*
 * Get mouse buttons supported
 */
static int
Mice_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

/////////////////////////////////////////////

/*
 * Get default mouse acceleration settings
 */
static void
Mice_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/////////////////////////////////////////////


/* IntelliMouse PS/2 protocol uses four byte reports
 * (PS/2 protocol omits last byte):
 *      Bit   7     6     5     4     3     2     1     0
 * --------+-----+-----+-----+-----+-----+-----+-----+-----
 *  Byte 0 |  0     0   Neg-Y Neg-X   1    Mid  Right Left
 *  Byte 1 |  X     X     X     X     X     X     X     X
 *  Byte 2 |  Y     Y     Y     Y     Y     Y     Y     Y
 *  Byte 3 |  W     W     W     W     W     W     W     W
 *
 * XXXXXXXX, YYYYYYYY, and WWWWWWWW are 8-bit two's complement values
 * indicating changes in x-coordinate, y-coordinate, and scroll wheel.
 * That is, 0 = no change, 1..127 = positive change +1 to +127,
 * and 129..255 = negative change -127 to -1.
 *
 * Left, Right, and Mid are the three button states, 1 if being depressed.
 * Neg-X and Neg-Y are set if XXXXXXXX and YYYYYYYY are negative, respectively.
 */
static int
Mice_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	unsigned char data[4];
	int Bytes_Read;
	int left, middle, right, button = 0;
	signed char x, y, w;

// Read Mouse. Ask for four bytes, expect three or four.

	Bytes_Read = read(mouse_fd, data, sizeof(data));
	if (Bytes_Read != 3 && Bytes_Read != 4)
		return MOUSE_NODATA;

	left = data[0] & 0x1;
	right = data[0] & 0x2;
	middle = data[0] & 0x4;

// remap these bits to Nano-X expected format

	if (left)	button |= MWBUTTON_L;
	if (middle)	button |= MWBUTTON_M;
	if (right)	button |= MWBUTTON_R;


// mwtypes.h:268:typedef int    MWCOORD;        // device coordinates

	x =   (signed char) data[1];
	y = - (signed char) data[2];  // y axis flipped between conventions
	if (Bytes_Read == 4) {
        w = (signed char) data[3];
		if (w > 0)
			button |= MWBUTTON_SCROLLUP;
		if (w < 0)
			button |= MWBUTTON_SCROLLDN;
	}

	*dx = x;	// integer from signed char
	*dy = y;       
	*dz = 0;
	*bp = button;

	return MOUSE_RELPOS;
}

/////////////////////////////////////////////
