/*
 * Microwindows touch screen driver for Psion 5
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#define _LINUX_TYPES_H
#include <linux/fb.h>
#include </linux/include/asm-arm/arch/touch_psion.h>

#include "device.h"

/* file descriptor for touch panel */
static int pd_fd;

/* Hack extern to used when hiding the mouse cursor
 * There needs to be a better way to do this
*/
extern SCREENDEVICE scrdev;

static int PD_Open(MOUSEDEVICE *pmd)
{
 	/*
	 * open up the touch-panel device.
	 * Return the fd if successful, or negative if unsuccessful.
	 */

	pd_fd = open("/dev/touch_psion", O_NONBLOCK);
	if (pd_fd < 0) {
		fprintf(stderr, "Error %d opening touch panel\n", errno);
		return -1;
	}

	GdHideCursor(&scrdev);
	return pd_fd;
}

static void PD_Close(void)
{
 	/* Close the touch panel device. */
	fprintf(stderr,"PD_Close called\n");
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

static int PD_Read(MWCOORD *px, MWCOORD *py, MWCOORD *pz, int *pb)
{
	/* read a data point */
        struct touch_psion_event data;
	int bytes_read;

	bytes_read = read(pd_fd, &data, sizeof(data));

	if (bytes_read != sizeof(data)) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;
		return -1;
	}

	*px = (MWCOORD)data.x_c;
	*py = (MWCOORD)data.y_c;

	*pb = (data.down>0?MWBUTTON_L:0);
	*pz = 0;

	if((*px == -1 || *py == -1) && *pb >= 0)
		return 3;			/* only have button data */
	if((*px == -1 || *py == -1) && *pb < 0)
		return 0;			/* don't have any data   */
	return 2;				/* have full set of data */
}

MOUSEDEVICE mousedev = {
	PD_Open,
	PD_Close,
	PD_GetButtonInfo,
	PD_GetDefaultAccel,
	PD_Read,
	NULL
};

#ifdef TEST
int main(int argc, char ** v)
{
	MWCOORD 	x, y, z;
	MWBUTTON	b;
	int result;
	int mouse = -1;
	printf("Opening touch panel...\n");

	if((result=PD_Open(0)) < 0)
		printf("Error %d, result %d opening touch-panel\n", errno, result);

	/* This stuff below can be used to set some of the parameters of the
	 * driver from the command line before going in to the test loop.
	 * Could this have been done better? Yup.
	 */

	if(argc > 1) {
		ioctl(result,1,atoi(v[1]));
		fprintf(stderr,"Setting pressure to %d\n",atoi(v[1]));
	} 
	if(argc > 2) {
		ioctl(result,2,atoi(v[2]));
		fprintf(stderr,"Setting updelay to %d\n",atoi(v[2]));
	}
	if(argc > 3) {
		ioctl(result,3,atoi(v[3]));
		fprintf(stderr,"Setting raw x to %d\n",atoi(v[3]));
	}
	if(argc > 4) {
		ioctl(result,4,atoi(v[4]));
		fprintf(stderr,"Setting raw y to %d\n",atoi(v[4]));
	} 
	if(argc > 5) {
		ioctl(result,5,atoi(v[5]));
		fprintf(stderr,"Setting res x to %d\n",atoi(v[5]));
	}
	if(argc > 6) {
		ioctl(result,6,atoi(v[6]));
		fprintf(stderr,"Setting res y to %d\n",atoi(v[6]));
	}
	if(argc > 7) {
		ioctl(result,7,atoi(v[7]));
		fprintf(stderr,"Setting fudge x to %d\n",atoi(v[7]));
	}
	if(argc > 8) {
		ioctl(result,8,atoi(v[8]));
		fprintf(stderr,"Setting fudge y to %d\n",atoi(v[8]));
	}
	if(argc > 9) {
		ioctl(result,9,atoi(v[9]));
		fprintf(stderr,"Setting average sample to %d\n",atoi(v[9]));
	} 
	if(argc > 10) {
		ioctl(result,10,atoi(v[10]));
		fprintf(stderr,"Setting raw min x to %d\n",atoi(v[10]));
	}
	if(argc > 11) {
		ioctl(result,11,atoi(v[11]));
		fprintf(stderr,"Setting raw min y to %d\n",atoi(v[11]));
	} 

	printf("Reading touch panel...\n");

	while(1) {
		result = PD_Read(&x, &y, &z, &b);
		if( result > 0) {
			if(mouse != b) {
				mouse = b;
				if(mouse) 
					printf("Pen Down\n");
				else
					printf("Pen Up\n");
			}

			printf("%d,%d,%d,%d,%d\n", result, x, y, z, b);

		}
	}
}
#endif
