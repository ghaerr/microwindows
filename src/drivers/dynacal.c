/* Calibration program for the Dynapro S3 controller */
/* Written by Jordan Crouse, September 5, 2001       */

/* Copyright, 2001, Century Embedded Technologies    */


#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/termios.h>
#include <nano-X.h>

struct {
  int scrX;
  int scrY;
  int rawX;
  int rawY;
} dataPoints[4];

/* The current and saved termios */
static struct termios ios_saved;
static struct termios ios_current;

#define BGCOLOR GR_RGB(0,0,0)
#define FGCOLOR GR_RGB(255,255,255)

char *instructions[4] = {
  "Sony CIS Calibration",
  " ",
  "Please press on each target",
  "as they appear"
};

int calInitSerial(char *dev) {

  /* Open up the serial port */
  int fd = open(dev, O_NONBLOCK);

  if (fd <= 0) {
    perror("open serial");
    return(-1);
  }

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

void calCloseSerial(int fd) {
  tcsetattr(fd, TCSANOW, &ios_saved);
  tcflush(fd, TCIOFLUSH);

  close(fd);
}

int calReadSerial(int fd) {

  unsigned char f;
  int val = read(fd, &f, sizeof(f));
  
  if (val <= 0) return(val);
  
  return((int) f);
}

/* Fd is the port to watch, data is the data that we are getting */

int calGetInput(int fd, int *data) {

  int count = 0;
  int state = 0;

  /* Read the data coming in off the line */

  while(1) {
    
    int c = calReadSerial(fd);

    if (c < 0 && errno != EAGAIN) return(-1);
    
    if (count++ > 500) return(0);
    if (c <= 0) continue;

    switch(state) {

    case 0:
      if (c & 0x80) {
	data[0] = (unsigned char) c;
	state = 1;
      }
      else 
	fprintf(stderr, "Non start byte recieved (%2.2x)\n", c);
      
      break;

    case 1:
      if (!(c & 0x80)) {
	data[1] = (unsigned char) c;
	state = 2;
      }
      else {
	fprintf(stderr, "Got a start byte in the middle of the packet\n");
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
	fprintf(stderr, "Got a start byte in the middle of the packet\n");
	data[0] = (unsigned char) c;

	state = 0;
      }

      break;
    }
  }

  return(1);
}


void drawText(GR_WINDOW_ID id, char **text, int count) {

  int tw, th, tb;
  int xpos, ypos;
  int i;
  
  GR_GC_ID gc = GrNewGC();
  GR_FONT_ID font = GrCreateFont(GR_FONT_GUI_VAR, 12, 0);
  GR_WINDOW_INFO info;

  GrGetWindowInfo(id, &info);

  GrSetGCFont(gc, font);
  GrSetGCForeground(gc, FGCOLOR);
  GrSetGCBackground(gc, BGCOLOR);

  /* Get the first line of text from the array, and check the size */
  GrGetGCTextSize(gc, text[0], -1, GR_TFTOP, &tw, &th, &tb);

  ypos = (info.height - ((count * th)+ 3)) / 2;
  
  /* Draw each line of the instructions */

  for(i = 0; i < count; i++) {
    GrGetGCTextSize(gc, text[i], -1, GR_TFTOP, &tw, &th, &tb);    
    xpos = (info.width - tw) / 2;
    GrText(id, gc, xpos, ypos, text[i], -1, GR_TFTOP);
    
    ypos += th + 3;
  }

  GrDestroyGC(gc);
  GrDestroyFont(font);
}

int doPoints(GR_WINDOW_ID id, int fd) {

  int data[4];
  int err = 0, i;
  fd_set fdset;

  GR_GC_ID gc = GrNewGC();
  
  for(i = 0; i < 4; i++) {

    int totalx = 0, totaly = 0;
    int p;
    
    /* Clear the previous point */

    if (i - 1 >= 0) {
      GrSetGCForeground(gc, BGCOLOR);
      GrFillRect(id, gc, dataPoints[i - 1].scrX - 10, 
		 dataPoints[i - 1].scrY - 10, 20, 20);
    }

    /* Now draw the new point */
    GrSetGCForeground(gc, GR_RGB(255,0,0));

    GrFillRect(id, gc, dataPoints[i].scrX - 10, dataPoints[i].scrY - 1, 
	       20, 2);
    
    GrFillRect(id, gc, dataPoints[i].scrX - 1, dataPoints[i].scrY - 10, 
	       2, 20);
    
    GrFlush();
    
    /* Wait until we get a button click */
    
    FD_SET(fd, &fdset);
    
    if (select(fd + 1, &fdset, 0, 0, 0) != -1) {
      int val;
      int index = 0;

      while(1) {
	val = calGetInput(fd, data);
	if (val < 0) break;
	if (val == 0) continue;

	totaly += (data[2] | ((data[0] & 0x07) << 7));
	totalx += (data[1] | ((data[0] & 0x38) << 4));
	
	index++;
	if (!(data[0] & 0x40)) break;
      }

      if (index > 0) {
	dataPoints[i].rawX = totalx / index;
	dataPoints[i].rawY = totaly / index;
      }
    }
  }

  GrDestroyGC(gc);
  return(err);
}
 

int main(int argc, char **argv) {

  int serialFd;
  int outFd;

  GR_SCREEN_INFO info;
  GR_WINDOW_ID calWindow;
  GR_EVENT event;

  /* Open up the graphics */

  if (GrOpen() == -1) {
    fprintf(stderr, "Error!  Unable to open the graphics engine\n");
    return(-1);
  }

  /* Now open the serial port */

  serialFd = calInitSerial("/dev/ttyS1");
  if (serialFd == -1) {
    fprintf(stderr, "Error!  Unable to open the touchscreen device\n");
    return(-1);
  }

  GrGetScreenInfo(&info);

  /* Decide which points we are going to touch */

  dataPoints[0].scrX = 10; 
  dataPoints[0].scrY = 10;

  dataPoints[1].scrX = 10; 
  dataPoints[1].scrY = info.rows - 10;

  dataPoints[2].scrX = info.cols - 10; 
  dataPoints[2].scrY = info.rows - 10;

  dataPoints[3].scrX = info.cols - 10; 
  dataPoints[3].scrY = 10;

  /* Now, create a window that spans the entire size of the screen */
  calWindow = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, info.cols, info.rows, 0, BGCOLOR, FGCOLOR);
  GrSelectEvents(calWindow, GR_EVENT_MASK_EXPOSURE);
  GrMapWindow(calWindow);
  /* Wait for exposure */
  while(GrPeekEvent(&event) != GR_EVENT_TYPE_EXPOSURE);
  
  /* Ok, now that we have been exposed, draw the instructions */
  drawText(calWindow, instructions, 4);

  if (!doPoints(calWindow, serialFd)) {

    double scrXDelta, rawXDelta;
    double scrYDelta, rawYDelta;

    double deltaX, deltaY;

    scrXDelta = (double) dataPoints[2].scrX - dataPoints[0].scrX;
    rawXDelta = (double) dataPoints[2].rawX - dataPoints[0].rawX;
    
    scrYDelta = (double) dataPoints[1].scrY - dataPoints[0].scrY;
    rawYDelta = (double) dataPoints[1].rawY - dataPoints[0].rawY;

    /* We can now extrapolate and discover the extreme edges of the screen */

    /* First, the low values */
    
    deltaX = abs( (rawXDelta / scrXDelta) * ((double) dataPoints[0].scrX));
    deltaY = abs( (rawYDelta / scrYDelta) * ((double) dataPoints[0].scrY));

    /*
    deltaX = abs((double) dataPoints[0].scrX * rawXDelta) / scrXDelta);
    deltaY = abs((double) (dataPoints[0].scrY * rawYDelta) / scrYDelta);
    */

    /* Print out the raw values, accounting for possible inversion */

    if (dataPoints[0].rawX > dataPoints[2].rawX) {
      printf("%d ", (int) (dataPoints[0].rawX + deltaX));
      printf("%d ", (int) (dataPoints[2].rawX - deltaX));
    }
    else {
      printf("%d ", (int) (dataPoints[0].rawX - deltaX));
      printf("%d ", (int) (dataPoints[2].rawX + deltaX));
    }
 
    if (dataPoints[0].rawY >dataPoints[1].rawY) {
      printf("%d ", (int) (dataPoints[0].rawY + deltaY));
      printf("%d\n", (int) (dataPoints[1].rawY - deltaY));
    }
    else {
      printf("%d ", (int) (dataPoints[0].rawY - deltaY));
      printf("%d\n", (int) (dataPoints[1].rawY + deltaY));
    }
  }
  else {
    fprintf(stderr, "Error - Unable to read the touchscreen\n");
  }

  /* Close everything down */
  calCloseSerial(serialFd);
  GrClose();

  /* Byebye! */
  return(0);
}
    
    
    
    
      
