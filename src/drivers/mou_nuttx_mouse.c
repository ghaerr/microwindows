#include <fcntl.h>
#include <unistd.h>
#include <nuttx/config.h>
#include <nuttx/input/mouse.h>
#include "device.h"

#ifndef NUTTX_MOUSE_PATH
#  define NUTTX_MOUSE_PATH "/dev/mouse0"
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
      fd = open(NUTTX_MOUSE_PATH, O_RDONLY | O_NONBLOCK);
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
  return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R |
         MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

static void nuttxmouse_GetDefaultAccel(int *pscale, int *pthresh)
{
  *pscale = SCALE;
  *pthresh = THRESH;
}

static int nuttxmouse_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
  struct mouse_report_s report;
  int n = read(fd, &report, sizeof(report));
  if (n <= 0)
    return MOUSE_NODATA;

  *dx = report.x;
  *dy = report.y;
  *dz = report.wheel;

  *bp = 0;
  if (report.buttons & MOUSE_BUTTON_1)
    *bp |= MWBUTTON_L;
  if (report.buttons & MOUSE_BUTTON_2)
    *bp |= MWBUTTON_R;
  if (report.buttons & MOUSE_BUTTON_3)
    *bp |= MWBUTTON_M;

  if (report.wheel > 0)
    *bp |= MWBUTTON_SCROLLUP;
  if (report.wheel < 0)
    *bp |= MWBUTTON_SCROLLDN;

  return MOUSE_ABSPOS;
}
