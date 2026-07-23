#include <fcntl.h>
#include <unistd.h>
#include <nuttx/config.h>
#include <nuttx/input/touchscreen.h>
#include "device.h"

#ifndef NUTTX_TOUCHSCREEN_PATH
#  define NUTTX_TOUCHSCREEN_PATH "/dev/input0"
#endif

#define SCALE   3
#define THRESH  5

static int fd = -1;

static int nuttxmouse_Open(MOUSEDEVICE *pmd);
static void nuttxmouse_Close(void);
static int nuttxmouse_GetButtonInfo(void);
static void nuttxmouse_GetDefaultAccel(int *pscale, int *pthresh);
static int nuttxmouse_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

MOUSEDEVICE mousedev =
{
  nuttxmouse_Open,
  nuttxmouse_Close,
  nuttxmouse_GetButtonInfo,
  nuttxmouse_GetDefaultAccel,
  nuttxmouse_Read,
  NULL,
  MOUSE_NORMAL
};

static int nuttxmouse_Open(MOUSEDEVICE *pmd)
{
  useconds_t delay = 1000;

  while (delay <= 1024000)
    {
      fd = open(NUTTX_TOUCHSCREEN_PATH, O_RDONLY | O_NONBLOCK);
      if (fd >= 0)
        {
          return DRIVER_OKFILEDESC(fd);
        }

      usleep(delay);
      delay *= 2;
    }

  return MOUSE_FAIL;
}

static void nuttxmouse_Close(void)
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
}

static int nuttxmouse_GetButtonInfo(void)
{
  return MWBUTTON_L;
}

static void nuttxmouse_GetDefaultAccel(int *pscale, int *pthresh)
{
  *pscale = SCALE;
  *pthresh = THRESH;
}

static int nuttxmouse_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
  struct touch_sample_s sample;
  int n = read(fd, &sample, sizeof(sample));
  if (n <= 0)
    return MOUSE_NODATA;

  *dz = 0;

  uint8_t flags = sample.point[0].flags;
  *bp = (flags & (TOUCH_DOWN | TOUCH_MOVE)) ? MWBUTTON_L : 0;

  /* Single-touch: only sample.point[0] is used.
   * For multi-touch support, iterate npoints and report each contact. */

  if (flags & TOUCH_POS_VALID)
    {
      *dx = sample.point[0].x;
      *dy = sample.point[0].y;
      return MOUSE_ABSPOS;
    }

  return MOUSE_NOMOVE;
}
