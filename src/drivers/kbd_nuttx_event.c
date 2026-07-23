#include <fcntl.h>
#include <unistd.h>
#include <nuttx/config.h>
#include <nuttx/input/keyboard.h>
#include "device.h"
#include "kbd_nuttx_common.h"

#ifndef NUTTX_KBD_EVENT_PATH
#  define NUTTX_KBD_EVENT_PATH "/dev/kbd"
#endif

static int kbd_fd = -1;
static MWKEYMOD modifiers = 0;

static int nuttxkbd_Open(KBDDEVICE *pkd);
static void nuttxkbd_Close(void);
static void nuttxkbd_GetModifierInfo(MWKEYMOD *mods, MWKEYMOD *curmods);
static int nuttxkbd_Read(MWKEY *kbuf, MWKEYMOD *mods, MWSCANCODE *scancode);

KBDDEVICE kbddev =
{
  nuttxkbd_Open,
  nuttxkbd_Close,
  nuttxkbd_GetModifierInfo,
  nuttxkbd_Read,
  NULL
};

static int nuttxkbd_Open(KBDDEVICE *pkd)
{
  useconds_t delay = 1000;

  if (kbd_fd >= 0)
    {
      close(kbd_fd);
      kbd_fd = -1;
    }

  modifiers = 0;

  while (delay <= 1024000)
    {
      kbd_fd = open(NUTTX_KBD_EVENT_PATH, O_RDONLY | O_NONBLOCK);
      if (kbd_fd >= 0)
        {
          return DRIVER_OKFILEDESC(kbd_fd);
        }
      usleep(delay);
      delay *= 2;
    }

  return DRIVER_FAIL;
}

static void nuttxkbd_Close(void)
{
  if (kbd_fd >= 0)
    {
      close(kbd_fd);
      kbd_fd = -1;
    }
  modifiers = 0;
}

static void nuttxkbd_GetModifierInfo(MWKEYMOD *mods, MWKEYMOD *curmods)
{
  if (mods)
    {
      *mods = MWKMOD_SHIFT |
              MWKMOD_CTRL | MWKMOD_ALT | MWKMOD_META |
              MWKMOD_NUM | MWKMOD_CAPS;
    }
  if (curmods)
    {
      *curmods = modifiers;
    }
}

static int nuttxkbd_Read(MWKEY *kbuf, MWKEYMOD *mods, MWSCANCODE *scancode)
{
  struct keyboard_event_s event;
  int n;

  n = read(kbd_fd, &event, sizeof(event));
  if (n < (int)sizeof(event))
    {
      return KBD_NODATA;
    }

  MWKEY key = translate_keycode(event.code);
  int press = (event.type == KEYBOARD_PRESS);

  update_modifiers(&modifiers, key, press);

  *kbuf = key;
  *mods = modifiers;
  *scancode = 0;

  return press ? KBD_KEYPRESS : KBD_KEYRELEASE;
}
