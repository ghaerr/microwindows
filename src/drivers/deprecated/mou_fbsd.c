/* #include <stdio.h> */
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <machine/mouse.h>
#include <machine/console.h>

/* #include <vgl.h> */

#include "device.h"

#define SCALE 3
#define THRESH 5


static int FBSD_Open(MOUSEDEVICE *pmd);
static void FBSD_Close(void);
static int FBSD_GetButtonInfo(void);
static void FBSD_GetDefaultAccel(int *pscale, int *pthresh);
static int FBSD_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz,int *bp);
MOUSEDEVICE mousedev = {
    FBSD_Open,
    FBSD_Close,
    FBSD_GetButtonInfo,
    FBSD_GetDefaultAccel,
    FBSD_Read,
    NULL
};

static int mouse_fd=0;

static int FBSD_Open(MOUSEDEVICE *pmd)
{

    mousemode_t theMouseMode;

    mouse_fd=open("/dev/sysmouse",O_RDONLY);
    if(mouse_fd < 0)
    {
	return(-1);
    }
    ioctl(mouse_fd, MOUSE_GETMODE, &theMouseMode);
    theMouseMode.level=1;
    ioctl(mouse_fd, MOUSE_SETMODE, &theMouseMode);
    return mouse_fd;
    
}

static void FBSD_Close(void)
{
    if (mouse_fd > 0)
    {
	close(mouse_fd);
    }
    mouse_fd=0;

}

static int FBSD_GetButtonInfo(void)
{
    return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

static void FBSD_GetDefaultAccel(int *pscale, int *pthresh)
{
    *pscale = SCALE;
    *pthresh = THRESH;
}

extern void FBSD_handle_event(void);

static int FBSD_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz,
		     int *bp)

{
    mousestatus_t theStatus;
    int butStat=0;
    int retVal=0;

    FBSD_handle_event();
    
    ioctl(mouse_fd,MOUSE_GETSTATUS, &theStatus);

    if(theStatus.flags | MOUSE_POSCHANGED)
    {
	*dx=theStatus.dx;
	*dy=theStatus.dy;
	*dz=theStatus.dz;
	retVal|=1;
    }

    if(theStatus.button & 0x1)
    {
	butStat|=MWBUTTON_L;
	retVal|=1;
    }
    if(theStatus.button & 0x2)
    {
	butStat|=MWBUTTON_M;
	retVal|=1;
    }
    if(theStatus.button & 0x4)
    {
	butStat|=MWBUTTON_R;
	retVal|=1;
    }
	
    *bp=butStat;
    return retVal;
}
