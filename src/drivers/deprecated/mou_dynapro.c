/* Dynapro SC3 touchscreen driver 
   Written by Jordan Crouse, September 5, 2001

   Copyright 2001, Century Embedded Technologies 
*/

/* TODO:
   Add support for rotated displays 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include "device.h"

/* The current and saved termios */
static struct termios ios_saved, ios_current;

/* The file descriptor for the mouse device */
static int mouFd = 0;

/* The calibration data */
/* We plug in some generic default data here, so if */
/* all else fails, the pointer should still work    */

unsigned short calData[4] = { 73, 956, 883, 167 };

/* Flags to indicate if we should invert the values on the X and Y axis */
/* This does not mean rotation, just a backwards TS                     */

static int invX = 0, invY = 0;

extern SCREENDEVICE scrdev;

static int initSerial(char *dev) {
  
  /* Open up the serial port */
  int fd = open(dev, O_NONBLOCK);
  
  if (fd <= 0) {
    EPRINTF("Error opening %s\n", dev);
    return(-1);
  }
  
  /* Save the previous IO settings */

  tcgetattr(fd, &ios_saved);
  ios_current = ios_saved;

  cfmakeraw(&ios_current);

  /* Set the baud rate */

  cfsetispeed(&ios_current, B2400);  
  cfsetospeed(&ios_current, B2400);
  
  /* Set the data bits and remove the parity */

  ios_current.c_cflag &= ~(CSIZE | PARENB);
  ios_current.c_cflag |= CS8;

  ios_current.c_cc[VMIN] = 3;
  ios_current.c_cc[VTIME] = 1;

  tcsetattr(fd, TCSANOW, &ios_current);
  tcflush(fd, TCIOFLUSH);
  
  return(fd);
}

static void closeSerial(int fd) {

  /* Restore the saved settings */

  tcsetattr(fd, TCSANOW, &ios_saved);
  tcflush(fd, TCIOFLUSH);

  close(fd);
}

/* Read a byte from the fd */

static int readSerial(int fd) {

  unsigned char f;
  int val = read(fd, &f, sizeof(f));
  
  if (val <= 0) return(val);
  
  return((int) f);
}

/* Get some data points from the device */

static int getInput(int fd, int *data) {

  int count = 0;
  int state = 0;

  /* Read the data coming in off the line */

  while(1) {
    int c;

    c = readSerial(fd);

    if (c < 0) {
      if (errno == EAGAIN || errno == EINTR) return(0);
      else return(-1);
    }
    
    if (count++ > 150) return(0);  
    

    switch(state) {

    case 0:
      if (c & 0x80) {
	data[0] = (unsigned char) c;
	state = 1;
      }

      /* This is a useless warning */

#ifdef NOTUSED
      else 
	fprintf(stderr, "Non start byte recieved (%2.2x)\n", c);
#endif

      break;

    case 1:
      if (!(c & 0x80)) {
	data[1] = (unsigned char) c;
	state = 2;
      }
      else {
#ifdef NOTUSED
	fprintf(stderr, "Got a start byte in the middle of the packet\n");
#endif

	data[0] = (unsigned char) c;

	state = 0;
      }

      break;

    case 2:

      if (!(c & 0x80)) {
	data[2] = (unsigned char) c;
	return(1);
      }
      else {
#ifdef NOTUSED
	fprintf(stderr, "Got a start byte in the middle of the packet\n");
#endif
	data[0] = (unsigned char) c;

	state = 0;
      }

      break;
    }
  }

  return(1);
}

/* Read calibration from a file */

static int readCalibration(char *filename) {

  char buffer[128];

  int calfd = open(filename, O_RDONLY);

  /* If the file doesn't exist, then just use the default values */
  if (calfd < 0)  return(0);
  
  /* The data will be formated as 4 values:
     xmin xmax ymin ymax
  */

  if (read(calfd, buffer, sizeof(buffer)) <= 0) {
    close(calfd);
    return(0);
  }
  
  sscanf(buffer, "%d %d %d %d\n", &calData[0], &calData[1], &calData[2], &calData[3]);
  
  close(calfd);

  /* Check the calibration values, and reverse them if we have to */

  if (calData[1] < calData[0]) {
    int  tmp = calData[1];

    calData[1] = calData[0];
    calData[0] = tmp;
    
    invX = 1;
  }

  if (calData[3] < calData[2]) {
    int tmp = calData[3];
    calData[3] = calData[2];
    calData[2] = tmp;

    invY = 1;
  }
  
  return(0);
}

static void doScale(int x, int y, int *dx, int *dy) {
  
  /* Scale the data appropriately to the screen */
  
  *dx = ((x - calData[0]) * scrdev.xres) / (calData[1] - calData[0]);
  *dy = ((y - calData[2]) * scrdev.yres) / (calData[3] - calData[2]);
}

static int MOU_Open(MOUSEDEVICE *pmd) {

  int calfd;

  mouFd = initSerial("/dev/ttyS1");

  if (mouFd == -1) {
    EPRINTF("Unable to open /dev/ttyS1 for reading\n");
    return(-1);
  }

  if (readCalibration("/etc/caldata") == -1) {
    closeSerial(mouFd);
    EPRINTF("Unable to get the calibration data\n");
    return(-1);
  }

  /* success */
  
  GdHideCursor(&scrdev);
  return(mouFd);
}

static int MOU_GetButtonInfo(void) {
  return(MWBUTTON_L);
}

static void MOU_GetDefaultAccel(int *pscale, int *pthresh) {
  *pscale = 3;
  *pthresh = 5;
}

static void MOU_Close(void) {

  if (mouFd >= 0)
    closeSerial(mouFd);
  
  mouFd = -1;
}

static int MOU_Read(MWCOORD *px, MWCOORD *py, MWCOORD *pz, int *pb) {

  int i;

  int totalx = 0, totaly = 0;
  int data[4];

  /* For now, only grab one reading, this touchscreen is pretty */
  /* good, so jitter isn't a problem                            */

  for(i = 0; i < 1; i++) {
    int x, y, sx, sy;
    int val;

    val = getInput(mouFd, data);
    
    if (val == -1) return(0);
    else if (val == 0) break;

    x = data[1] | ((data[0] & 0x38) << 4);
    y = data[2] | ((data[0] & 0x07) << 7);

    /* Send the data in for calibration */
    doScale(x, y, &sx, &sy);

    /* Inver the axii if needed */

    if (invX) 
      sx = scrdev.xres - sx;
    if (invY)
      sy = scrdev.yres - sy;

    totalx += sx;
    totaly += sy;
  }

  if (i == 0) return(0);

  *px = totalx / (i);
  *py = totaly / (i);

  /* Record the last state of the mouse */
 
  *pb = ( (data[0] & 0x40) ? MWBUTTON_L : 0 );
  *pz = 0;

  if (!*pb) return(3); else return(2);
}

MOUSEDEVICE mousedev = {
  MOU_Open,
  MOU_Close,
  MOU_GetButtonInfo,
  MOU_GetDefaultAccel,
  MOU_Read,
  NULL
};

