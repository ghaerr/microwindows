/*
 * Microwindows touch screen driver for G.Mate YOPY
 *
 * Copyright (c) 2000 Century Software Embedded Technologies
 *
 * Requires /dev/yopy-ts kernel mouse driver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include "device.h"

/* file descriptor for touch panel */
static int pd_fd = -1;

/* Hack extern to used when hiding the mouse cursor
 * There needs to be a better way to do this
*/
extern SCREENDEVICE scrdev;

static int cal[7];

typedef struct {
        int x, y;
} XYPOINT;

#define TRANSFORMATION_UNITS_PER_PIXEL 4
static int GetPointerCalibrationData(void)
{
  /*
   * Read the calibration data from the calibration file.
   * Calibration file format is seven coefficients separated by spaces.
   */
  
  /* Get pointer calibration data from this file */
  const char cal_filename[] = "/etc/ts.conf";

  int items;

  FILE* f = fopen(cal_filename, "r");
  if ( f == NULL )
    {
      EPRINTF("Error %d opening pointer calibration file %s.\n",
	      errno, cal_filename);
      return -1;
    }

  items = fscanf(f, "%d %d %d %d %d %d %d",
		 &cal[0], &cal[1], &cal[2], &cal[3], &cal[4], &cal[5], &cal[6]);
  if ( items != 7 )
    {
      EPRINTF("Improperly formatted pointer calibration file %s.\n",
	      cal_filename);
      return -1;
    }

#ifdef TEST
  EPRINTF("a=%d b=%d c=%d d=%d e=%d f=%d s=%d\n",
	  cal[0], cal[1], cal[2], cal[3], cal[4], cal[5], cal[6]);
#endif

  return 0;
}

static XYPOINT DeviceToScreen(XYPOINT p)
{
  /*
   * Transform device coordinates to screen coordinates.
   * Take a point p in device coordinates and return the corresponding
   * point in screen coodinates.
   * This can scale, translate, rotate and/or skew, based on the coefficients
   * calculated above based on the list of screen vs. device coordinates.
   */
  
  static XYPOINT prev;
  /* set slop at 3/4 pixel */
  const short slop = TRANSFORMATION_UNITS_PER_PIXEL * 3 / 4;
  XYPOINT new, out;
  
  /* transform */
  new.x = (cal[0] * p.x + cal[1] * p.y + cal[2]) / cal[6];
  new.y = (cal[3] * p.x + cal[4] * p.y + cal[5]) / cal[6];
  
  /* hysteresis (thanks to John Siau) */
  if ( abs(new.x - prev.x) >= slop )
    out.x = (new.x | 0x3) ^ 0x3;
  else
    out.x = prev.x;
  
  if ( abs(new.y - prev.y) >= slop )
    out.y = (new.y | 0x3) ^ 0x3;
  else
    out.y = prev.y;
  
  prev = out;
  
  return out;
}

static int PD_Open(MOUSEDEVICE *pmd)
{
  /*
   * open up the touch-panel device.
   * Return the fd if successful, or negative if unsuccessful.
   */
  
  pd_fd = open("/dev/yopy-ts", O_NONBLOCK);
  if (pd_fd < 0) {
    EPRINTF("Error %d opening touch panel\n", errno);
    return -1;
  }
  
  GetPointerCalibrationData();
  GdHideCursor(&scrdev);
  
  return pd_fd;
}

static void PD_Close(void)
{
  /* Close the touch panel device. */
  if (pd_fd >= 0)
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

  unsigned long data;
  int bytes_read;

  XYPOINT transformed;

  bytes_read = read(pd_fd, &data, sizeof(data));

  if (bytes_read != sizeof(data)) {
    if (errno == EINTR || errno == EAGAIN) {
      return 0;
    }
    return 0;
  }
	
  transformed.x = (data & 0x3ff);
  transformed.y = (data >> 10) & 0x3ff;
	
  transformed = DeviceToScreen(transformed);
	
  transformed.x >>= 2;
  transformed.y >>= 2;

  *px = transformed.x;
  *py = transformed.y;

  *pb = (((data >> 31) & 0x1) ? MWBUTTON_L : 0);

  *pz = 0;

  if(! *pb )
    return 3;			/* only have button data */
  else 
    return 2;			/* have full set of data */
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
  MWCOORD x, y, z;
  int	b;
  int result;
  int mouse = -1;
  DPRINTF("Opening touch panel...\n");
  
  if((result=PD_Open(0)) < 0)
    DPRINTF("Error %d, result %d opening touch-panel\n", errno, result);
  
  while(1) {
    result = PD_Read(&x, &y, &z, &b);
    if( result > 0) {
      if(mouse != b) {
	mouse = b;
	if(mouse) 
	  DPRINTF("Pen Down\n");
	else
	  DPRINTF("Pen Up\n");
      }
      DPRINTF("%d,%d,%d,%d,%d\n", result, x, y, z, b);
    }
  }
}
#endif
