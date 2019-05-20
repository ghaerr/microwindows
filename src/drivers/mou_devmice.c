/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Kurt Nalty 2019 
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
static int  	Mice_Poll(void);

static int      mouse_fd = -1;

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
//	Mice_Poll
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
        mouse_fd = open(MICE_DEV_FILE, O_RDONLY | O_NONBLOCK);

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
        return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
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


/*
	O_UNBLOCK reads of /dev/input/mice  routinely returns -1 when no
        new data is available. When new data is available, we read three
	bytes: 
		data[0] is mouse button status,
			bit 2 middle, bit 1 right, bit 0 left
		data[1] is delta x (signed byte)
		data[2] is delta y (signed byte)

*/

static int
Mice_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	unsigned char data[4];
	int Bytes_Read;
        int left, middle, right, button = 0;
        signed char x, y, z;

// Read Mouse. Ask for four bytes, expect three.

        Bytes_Read = read(mouse_fd, data, sizeof(data));
	if (Bytes_Read != 3)
		return MOUSE_NODATA;

        left = data[0] & 0x1;
        right = data[0] & 0x2;
        middle = data[0] & 0x4;

// remap these bits to Nano-X expected format

	if (left)	button |= MWBUTTON_L;
	if (middle)	button |= MWBUTTON_M;
	if (right)	button |= MWBUTTON_R;

	*bp = button;

// mwtypes.h:268:typedef int    MWCOORD;        // device coordinates

        x =   (signed char) data[1];
        y = - (signed char) data[2];  // y axis flipped between conventions
        z =   0;
	*dx = x;	// integer from signed char
	*dy = y;       
	*dz = z;

	return MOUSE_RELPOS;
}

/////////////////////////////////////////////

/*
 * Poll for events
 */

static int
Mice_Poll(void)
{
  return 0;
}

/////////////////////////////////////////////
