/*
 * Microwindows touch screen driver for NEC Harrier Demo Board.
 *
 * Copyright (c) 2000 Century Software Embedded Technologies
 *
 * Requires /dev/tpanel kernel device driver for the VRC4173 chip 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/tpanel.h>
#include "device.h"
#include "mou_tp.h"

/* Very basic handling for the touch panel */
/* Mostly borrowed from mou_ipaq.c which I believe was mostly */
/* borrowed from mou_tp.c */

/* Define this if you want to use the filter instead of the average method */
/* #define USE_FILTER */

/* Defines used throughout */
#define TP_STATUS_HARDDATALOST 0x1000
#define TP_STATUS_SOFTDATALOST 0x2000
#define TP_STATUS_PENCONTACT   0x4000
#define TP_STATUS_DATAVALID    0x8000

/* Fix these when we know the right size */

#define TP_MIN_X_SIZE          291
#define TP_MIN_Y_SIZE          355

#define TP_MAX_X_SIZE          3839
#define TP_MAX_Y_SIZE          3711

#define DATA_STATUS  0
#define DATA_YPLUS   1
#define DATA_YMINUS  2
#define DATA_XPLUS   3
#define DATA_XMINUS  4
#define DATA_Z       5


#ifdef USE_FILTER
#define MOU_SAMPLE_RATE   1
#else
#define MOU_SAMPLE_RATE   10
#endif

#define MOU_READ_INTERVAL 5000

  /* Data format (from kernel driver): */
  /* unsigned short status */
  /* unsigned short x+ (raw) */
  /* unsigned short x- (raw) */
  /* unsigned short y+ (raw) */
  /* unsigned short y- (raw) */
  /* unsigned short z (presssure, raw) */

static int pd_fd = -1;
int enable_pointing_coordinate_transform = 1;

static TRANSFORMATION_COEFFICIENTS tc;

#ifndef TEST
extern SCREENDEVICE scrdev;
#endif

#ifdef TEST
#undef EPRINTF
#undef DPRINTF

#define EPRINTF printf
#define DPRINTF printf
#endif
 
int GetPointerCalibrationData(void)
{
	/*
	 * Read the calibration data from the calibration file.
	 * Calibration file format is seven coefficients separated by spaces.
	 */

	/* Get pointer calibration data from this file */
	const char cal_filename[] = "/etc/pointercal";

	int items;

	FILE* f = fopen(cal_filename, "r");
	if ( f == NULL )
	{
		EPRINTF("Error %d opening pointer calibration file %s.\n",
			errno, cal_filename);
		EPRINTF("Please type \"/usr/bin/tpcal > %s\" to calibrate\n",
			cal_filename);
		return -1;
	}

	items = fscanf(f, "%d %d %d %d %d %d %d",
		&tc.a, &tc.b, &tc.c, &tc.d, &tc.e, &tc.f, &tc.s);
	if ( items != 7 )
	{
		EPRINTF("Improperly formatted pointer calibration file %s.\n",
			cal_filename);
		return -1;
	}

#if TEST
		EPRINTF("a=%d b=%d c=%d d=%d e=%d f=%d s=%d\n",
			tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
#endif

	return 0;
}

inline MWPOINT DeviceToScreen(MWPOINT p)
{
	/*
	 * Transform device coordinates to screen coordinates.
	 * Take a point p in device coordinates and return the corresponding
	 * point in screen coodinates.
	 * This can scale, translate, rotate and/or skew, based on the
	 * coefficients calculated above based on the list of screen
	 * vs. device coordinates.
	 */

	static MWPOINT prev;
	/* set slop at 3/4 pixel */
	const short slop = TRANSFORMATION_UNITS_PER_PIXEL * 3 / 4;
	MWPOINT new, out;

	/* transform */
	new.x = (tc.a * p.x + tc.b * p.y + tc.c) / tc.s;
	new.y = (tc.d * p.x + tc.e * p.y + tc.f) / tc.s;

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
  struct scanparam s;
  int settle_upper_limit;
  int result;
  
  /* Open the device */
  pd_fd = open("/dev/tpanel", O_NONBLOCK);
  
  if (pd_fd < 0)
    {
      EPRINTF("Error %d opening touch panel\n", errno);
      return -1;
    }

  s.interval = MOU_READ_INTERVAL;

  /*
   * Upper limit on settle time is approximately (scan_interval / 5) - 60
   * (5 conversions and some fixed overhead)
   * The opmtimal value is the lowest that doesn't cause significant
   * distortion.
   */

  settle_upper_limit = (s.interval / 5) - 60;
  s.settletime = settle_upper_limit * 50 / 100;
  result = ioctl(pd_fd, TPSETSCANPARM, &s);

  if ( result < 0 )
    EPRINTF("Error %d, result %d setting scan parameters.\n",
	    result, errno);
  
  if (enable_pointing_coordinate_transform)
    { 
      if (GetPointerCalibrationData() < 0)
	{
	  close(pd_fd);
	  return -1;
	}
    }
  
  /* We choose not to hide the cursor for now, others may want to */
  
#ifdef NOTUSED
#ifndef TEST
    /* Hide the cursor */
  GdHideCursor(&scrdev);
#endif
#endif
  
  return(pd_fd);
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

#define MAX_DEVICE_READS 10
static int read_tp(unsigned short *x, unsigned short *y, 
		   unsigned short *z, int *b, unsigned short *status, unsigned char block)
{
  unsigned char read_count = 0;
  unsigned short tempx, tempy;
  int bytes_read;
  unsigned short data[6];
  
  /* Uh, oh -- The driver is slow and fat, so we get lots of EAGAINS between   */
  /* reads.  Thats never good.  So we loop here for some count before we bail  */

  while(read_count < MAX_DEVICE_READS)
    {
      bytes_read = read(pd_fd, data, sizeof(data));
  
      if (bytes_read != sizeof(data))
	{
	  if (errno != EAGAIN)
	    {
	      EPRINTF("Error reading touch panel.  errno = %d\n", errno);
	      return(errno);
	    }

	  if (block)
	    {
	      if (read_count++ == MAX_DEVICE_READS)
		return(EAGAIN);
	      else
		usleep(MOU_READ_INTERVAL / MAX_DEVICE_READS);
	    }
	  else
	    return(EAGAIN);
	}
      else
	break;
    }

  tempx = data[DATA_XPLUS];
  tempy = data[DATA_YPLUS];

  /* Sanity check */
  /* This is based on measured values.  Occassionally, we get a bad read from the board */
  /* This is to ensure that really wacked out reads don't get through.                  */

  if ((data[DATA_STATUS] & TP_STATUS_DATAVALID) == TP_STATUS_DATAVALID)
    {
       if (enable_pointing_coordinate_transform)
	 {
	   if (tempx < TP_MIN_X_SIZE || tempx > TP_MAX_X_SIZE)
	     {
#ifdef TEST
	       EPRINTF("Got an out of range X value.  X=%d,Y=%d,B=%d\n",
		       tempx, tempy, 
		       ((data[DATA_STATUS] & TP_STATUS_PENCONTACT) ? MWBUTTON_L : 0));
#endif
	       return(EAGAIN);
	     }
	   
	   if (tempy < TP_MIN_Y_SIZE || tempy > TP_MAX_Y_SIZE)
	     {
#ifdef TEST
	       EPRINTF("Got an out of range Y value.  X=%d,Y=%d,B=%d\n",
		       tempx, tempy, 
		       ((data[DATA_STATUS] & TP_STATUS_PENCONTACT) ? MWBUTTON_L : 0));
#endif
	       return(EAGAIN);
	     }
	 }

       *x = tempx;
       *y = tempy;
       *z = data[DATA_Z];
    }
  else
    { 
      *x = 0;
      *y = 0;
      *z = 0;
    }

  *b = ((data[DATA_STATUS] & TP_STATUS_PENCONTACT) ? MWBUTTON_L : 0);  
  *status = data[DATA_STATUS];

  return(0);
}


static int PD_Read(MWCOORD *px, MWCOORD *py, MWCOORD *pz, int *pb)
{
#ifdef USE_FILTER
  /* Filter stuff borrowed from mou_tp.c */
  const int iir_shift_bits = 3;
  const int iir_sample_depth = (1 << iir_shift_bits);
  
  static int iir_accum_x = 0;
  static int iir_accum_y = 0;
  static int iir_accum_z = 0;
  static int iir_count = 0;
#else
  double cx, cy, cz;
#endif

  /* Other local variables */
  MWPOINT transformed;
  int err = 0;
  unsigned short samples = 0;
  unsigned short xpos = 0;
  unsigned short ypos = 0;
  unsigned short zpos = 0;
  unsigned short status = 0;

  *pb = 0;
  *px = 0;
  *py = 0;
  *pz = 0;

#ifndef USE_FILTER
  cx = 0;
  cy = 0;
  cz = 0;
#endif

  if ((err = read_tp(&xpos, &ypos, &zpos, pb, &status, 0)))
    {
      if (err == EAGAIN)
	return(0);
      else
	return(1);
    }

  /* Check the status of the button */

  if ( (status & TP_STATUS_DATAVALID) != TP_STATUS_DATAVALID)
    {
      if (*pb)
	return(0);
      else
	goto button_up;
    }

  while((status & TP_STATUS_DATAVALID) == TP_STATUS_DATAVALID)
    {
      int tempb = 0;

      err = read_tp(&xpos, &ypos, &zpos, &tempb, &status, 1);
 
      if (err == EAGAIN)
	{
	  if (!samples)
	    continue; /* We need at least one reading! */
	  else
	    break; /* The device continues to not respond.  Bail */
	}
      else if (err)
	return(-1);

      /* If the data is invalid and the button is down, then bail */
      /* Otherwise, record the button data */
     
      if ( (status & TP_STATUS_DATAVALID) != TP_STATUS_DATAVALID)
	{
	  if (tempb)
	    return(0); /* Button is down, but data is invalid */      
	  else 
	    {
	      *pb = tempb; /* Record button up */
	      goto button_up;
	    }
	}

#ifdef USE_FILTER

      /* Run the newly aquired data through a filter */
      /* is filter ready? */
      if ( iir_count == iir_sample_depth )
	{
	  /* make room for new sample */
	  iir_accum_x -= iir_accum_x >> iir_shift_bits;
	  iir_accum_y -= iir_accum_y >> iir_shift_bits;
	  iir_accum_z -= iir_accum_z >> iir_shift_bits;
	  
	  /* feed new sample to filter */
	  iir_accum_x += xpos;
	  iir_accum_y += ypos;
	  iir_accum_z += zpos;
	}
      else
	{
	  iir_accum_x += xpos;
	  iir_accum_y += ypos;
	  iir_accum_z += zpos;
	  iir_count += 1;
	}

#else
      cx += xpos;
      cy += ypos;
      cz += zpos;
#endif

      samples++;
      
      /* Enough samples?? */
      if (samples >= MOU_SAMPLE_RATE)
	break;
    }

  if (!samples)
    return(0);

#ifdef USE_FILTER  
  /* We're not done gathering samples yet */
  if (iir_count < iir_sample_depth)
    return(0);

  if (enable_pointing_coordinate_transform)
    {	  
      /* transform x,y to screen coords */
      transformed.x = iir_accum_x;
      transformed.y = iir_accum_y;
      transformed = DeviceToScreen(transformed);
      
      *px = transformed.x >> 2;
      *py = transformed.y >> 2;
    }
  else
    {
      *px = (MWCOORD) abs(iir_accum_x);
      *py = (MWCOORD) abs(iir_accum_y);
    }
#else
  
  if (enable_pointing_coordinate_transform)
    {
      transformed.x = (cx / samples);
      transformed.y = (cy / samples);
      
      transformed = DeviceToScreen(transformed);
      
      *px = (MWCOORD) transformed.x >> 2;
      *py = (MWCOORD) transformed.y >> 2;
    }
  else
    {
      *px = (MWCOORD) abs(cx / samples);
      *py = (MWCOORD) abs(cy / samples);
    }
#endif
  
 button_up:
  if (! *pb)
    {
#ifdef USE_FILTER 
     /* reset the filter */
      iir_count = 0;
      iir_accum_x = 0;
      iir_accum_y = 0;
      iir_accum_z = 0;
#endif
      return(3);
    }
  else
    return(2); /* XYZ and button data */

}

#ifndef TEST
MOUSEDEVICE mousedev = {
	PD_Open,
	PD_Close,
	PD_GetButtonInfo,
	PD_GetDefaultAccel,
	PD_Read,
	NULL
};
#endif

#ifdef TEST
int main(int argc, char ** v)
{
	int x, y, z;

	int	b;
	int result;
	
	DPRINTF("Opening touch panel...\n");

	if((result=PD_Open(0)) < 0)
	  {
	    
	    DPRINTF("Error %d, result %d opening touch-panel\n", errno, result);
	    exit(0);
	  }

	DPRINTF("Reading touch panel...\n");

	while(1) 
	  {
	    result = PD_Read(&x, &y, &z, &b);
	    
	    if( result > 0) 
	      {
		DPRINTF("(%d,%d,%d) b = %d\n",x, y, z, b);		
	      }
	  }
}
#endif

