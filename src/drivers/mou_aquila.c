/*
 * Copyright (c) 2019 Mohamed Anwar
 *
 * Aquila /dev/mouse Mouse Driver
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#define MOUSE_DEV_FILE    "/dev/mouse"

static int  mouse_open(MOUSEDEVICE *pmd);
static void mouse_close(void);
static int  mouse_getbuttoninfo(void);
static void	mouse_getdefaultaccel(int *pscale, int *pthresh);
static int  mouse_read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

static int mouse_fd;

MOUSEDEVICE mousedev = {
    mouse_open,
    mouse_close,
    mouse_getbuttoninfo,
    mouse_getdefaultaccel,
    mouse_read,
    NULL,
    MOUSE_NORMAL    /* flags*/
};

/////////////////////////////////////////////

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int mouse_open(MOUSEDEVICE *pmd)
{
    mouse_fd = open(MOUSE_DEV_FILE, O_NONBLOCK);

    if (mouse_fd < 0) {
        return -1;
    }

    return mouse_fd;
}

/////////////////////////////////////////////

/*
 * Close the mouse device.
 */
static void mouse_close(void)
{
    if (mouse_fd > 0)
        close(mouse_fd);

    mouse_fd = -1;
}

/////////////////////////////////////////////

/*
 * Get mouse buttons supported
 */

static int mouse_getbuttoninfo(void)
{
    return MWBUTTON_L | MWBUTTON_R;
}

/////////////////////////////////////////////

/*
 * Get default mouse acceleration settings
 */
static void mouse_getdefaultaccel(int *pscale, int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/////////////////////////////////////////////


/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */

/*
	O_NONBLOCK reads of /dev/mouse  routinely returns -1 when no
        new data is available. When new data is available, we read three
	bytes: 
		data[0] is mouse packet
		data[1] is delta x
		data[2] is delta y
*/

struct mouse_packet {
    uint8_t left    : 1;
    uint8_t right   : 1;
    uint8_t mid     : 1;
    uint8_t _1      : 1;
    uint8_t x_sign  : 1;
    uint8_t y_sign  : 1;
    uint8_t x_over  : 1;
    uint8_t y_over  : 1;
} __packed;

static int mouse_read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
    unsigned char packet[3];

    if (read(mouse_fd, packet, 3) != 3)
        return 0;

    struct mouse_packet *data;
    data = (struct mouse_packet *) &packet[0];

    int x = ((data->x_sign ? -256 : 0) + packet[1]);
    int y = ((data->y_sign ? -256 : 0) + packet[2]);

    int button = 0;
    button |= data->left?  MWBUTTON_L : 0;
    button |= data->right? MWBUTTON_R : 0;

	*bp = button;

	*dx = x;	// integer from signed char
	*dy = -y;       
	*dz = 0;

	return 1;
}

/////////////////////////////////////////////
