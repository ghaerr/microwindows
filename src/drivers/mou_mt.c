/*
 *  MicroTouch touch panel driver for MicroTouch capacitive and resistive
 *  controllers at RS232 port.
 *
 *  We expect the Microtouch Controller configured to 9600 Baud, 8 data bits,
 *  1 stop bit, no parity
 *
 *  Written by Holger Waechtler <hwaechtler@users.sourceforge.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "device.h"

static int  	MT_Open(MOUSEDEVICE *pmd);
static void 	MT_Close(void);
static int  	MT_GetButtonInfo(void);
static void	MT_GetDefaultAccel(int *pscale,int *pthresh);
static int  	MT_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

MOUSEDEVICE mousedev = {
	MT_Open,
	MT_Close,
	MT_GetButtonInfo,
	MT_GetDefaultAccel,
	MT_Read,
	NULL
};

static int mt_fd;


#define CMD(fd,x) \
   do { \
      write (fd, "\001" x "\r", strlen(x) + 2); \
      tcdrain(fd); \
   } while (0)



/*
 * Open up the mouse device.
 */
static int
MT_Open(MOUSEDEVICE *pmd)
{
   char *devname = "/dev/ttyS0";
   struct termios termios;

   if ((mt_fd = open (devname, O_RDWR | O_NONBLOCK)) < 0) {
      fprintf (stderr, "error opening '%s'\n", devname);
      exit (-1);
   }

   tcgetattr (mt_fd, &termios);

   cfsetispeed (&termios, B9600);
   cfsetospeed (&termios, B9600);

   cfmakeraw (&termios);
   termios.c_cflag &= ~CBAUD;
   termios.c_cflag = B9600;
   termios.c_cflag |= CS8 | CREAD;
   termios.c_oflag = 0;
   termios.c_oflag &= ~(OCRNL | ONLRET);

   tcsetattr (mt_fd, TCSAFLUSH, &termios);

   CMD(mt_fd,"R");
   CMD(mt_fd,"AD");
   CMD(mt_fd,"PN812");
   CMD(mt_fd,"FT");
   CMD(mt_fd,"MS");
   CMD(mt_fd,"PL");

  /*
   *  This is a sort of hack, but our embedded PPC was to slow to read all
   *  responses from the Microtouch controller. To get out of this we simply
   *  discard them ...
   */
   usleep (250000);
   tcflush (mt_fd, TCIOFLUSH);

   return 0;
}

/*
 * Close the mouse device.
 */
static void
MT_Close(void)
{
   if (mt_fd > 0)
      close (mt_fd);

   mt_fd = 0;
}

/*
 * Get mouse buttons supported
 */
static int
MT_GetButtonInfo(void)
{
   return MWBUTTON_L;
}


/*
 * doesn't makes sense for a touch panel ...
 */
static void
MT_GetDefaultAccel(int *pscale,int *pthresh)
{
   *pscale = 3;
   *pthresh = 5;
}



char buf [5];
int bytes_in_buf = 0;


/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
static int
MT_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
   if (bytes_in_buf == 5) {
      bytes_in_buf = 0;

      *dx = ((buf[1] & 0x7f) | ((buf[2] & 0x7f) << 7));
      *dy = ((buf[3] & 0x7f) | ((buf[4] & 0x7f) << 7));
      *dx = (*dx * scrdev.xvirtres) >> 14;
      *dy = scrdev.yvirtres - ((*dy * scrdev.yvirtres) >> 14);
      *dz = 1;

      if (!(buf[0] & (1 << 6)))
         *bp = 0;
      else
         *bp = MWBUTTON_L;

      return 2;
   }

   if (read (mt_fd, buf + bytes_in_buf, 1) == 1)
      bytes_in_buf++;

   if ((buf[bytes_in_buf-1] & 0x80) != 0) {
      buf [0] = buf [bytes_in_buf-1];
      bytes_in_buf = 1;
   }       

   return 0;
}

