#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <nuttx/config.h>
#include <nuttx/streams.h>
#include <nuttx/input/kbd_codec.h>
#include "device.h"
#include "kbd_nuttx_common.h"

#ifndef NUTTX_KBD_RAW_PATH
#  define NUTTX_KBD_RAW_PATH "/dev/kbda"
#endif

#define RAW_BUF_SIZE 64

static int kbd_fd = -1;
static MWKEYMOD modifiers = 0;

static bool g_pending_release = false;
static uint8_t g_last_raw_key = 0;

static uint8_t raw_buf[RAW_BUF_SIZE];
static int raw_buf_len = 0;
static int raw_buf_pos = 0;
static struct kbd_getstate_s g_kbd_state;

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

static MWKEY raw_byte_to_mwkey(uint8_t ch)
{
  switch (ch)
    {
    case 0x7f:
      return MWKEY_BACKSPACE;
    case 0x0a:
      return MWKEY_ENTER;
    default:
      if (ch < 128)
        {
          return (MWKEY)ch;
        }
      return MWKEY_UNKNOWN;
    }
}

static void raw_reset(void)
{
  raw_buf_len = 0;
  raw_buf_pos = 0;
  memset(&g_kbd_state, 0, sizeof(g_kbd_state));
}

static int nuttxkbd_Open(KBDDEVICE *pkd)
{
  useconds_t delay = 1000;

  if (kbd_fd >= 0)
    {
      close(kbd_fd);
      kbd_fd = -1;
    }

  modifiers = 0;
  g_pending_release = false;
  raw_reset();

  while (delay <= 1024000)
    {
      kbd_fd = open(NUTTX_KBD_RAW_PATH, O_RDONLY | O_NONBLOCK);
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
  g_pending_release = false;
  raw_reset();
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
  uint8_t ch;
  int n;
  int ret;
  off_t nget_before;
  struct lib_meminstream_s memstream;

  if (g_pending_release)
    {
      g_pending_release = false;
      *kbuf = (MWKEY)g_last_raw_key;
      *mods = modifiers;
      *scancode = 0;
      return KBD_KEYRELEASE;
    }

  if (raw_buf_pos >= raw_buf_len)
    {
      n = read(kbd_fd, raw_buf, RAW_BUF_SIZE);
      if (n <= 0)
        {
          return KBD_NODATA;
        }

      raw_buf_pos = 0;
      raw_buf_len = n;
    }

  lib_meminstream(&memstream,
                  (FAR const char *)(raw_buf + raw_buf_pos),
                  raw_buf_len - raw_buf_pos);
  nget_before = memstream.common.nget;

  ret = kbd_decode((FAR struct lib_instream_s *)&memstream,
                   &g_kbd_state, &ch);
  raw_buf_pos += (int)(memstream.common.nget - nget_before);

  switch (ret)
    {
    case KBD_ERROR:
      raw_buf_pos = raw_buf_len;
      return KBD_NODATA;

    case KBD_PRESS:
      {
        MWKEY key = raw_byte_to_mwkey(ch);
        if (key == MWKEY_UNKNOWN)
          {
            return KBD_NODATA;
          }

        g_pending_release = true;
        g_last_raw_key = (uint8_t)key;

        *kbuf = key;
        *mods = modifiers;
        *scancode = 0;
        return KBD_KEYPRESS;
      }

    case KBD_RELEASE:
      return KBD_NODATA;

    case KBD_SPECPRESS:
      {
        MWKEY key = translate_keycode(ch);
        update_modifiers(&modifiers, key, 1);

        *kbuf = key;
        *mods = modifiers;
        *scancode = 0;
        return KBD_KEYPRESS;
      }

    case KBD_SPECREL:
      {
        MWKEY key = translate_keycode(ch);
        update_modifiers(&modifiers, key, 0);

        *kbuf = key;
        *mods = modifiers;
        *scancode = 0;
        return KBD_KEYRELEASE;
      }

    default:
      return KBD_NODATA;
    }
}
