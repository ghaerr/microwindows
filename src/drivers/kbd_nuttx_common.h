#ifndef KBD_NUTTX_COMMON_H
#define KBD_NUTTX_COMMON_H

#include <nuttx/input/kbd_codec.h>
#include "mwtypes.h"

static MWKEY translate_keycode(uint32_t code)
{
  switch (code)
    {
    case KEYCODE_BACKDEL:
      return MWKEY_BACKSPACE;
    case KEYCODE_FWDDEL:
      return MWKEY_DELETE;
    case KEYCODE_HOME:
      return MWKEY_HOME;
    case KEYCODE_END:
      return MWKEY_END;
    case KEYCODE_LEFT:
      return MWKEY_LEFT;
    case KEYCODE_RIGHT:
      return MWKEY_RIGHT;
    case KEYCODE_UP:
      return MWKEY_UP;
    case KEYCODE_DOWN:
      return MWKEY_DOWN;
    case KEYCODE_PAGEUP:
      return MWKEY_PAGEUP;
    case KEYCODE_PAGEDOWN:
      return MWKEY_PAGEDOWN;
    case KEYCODE_INSERT:
      return MWKEY_INSERT;
    case KEYCODE_ENTER:
      return MWKEY_ENTER;
    case KEYCODE_F1:
      return MWKEY_F1;
    case KEYCODE_F2:
      return MWKEY_F2;
    case KEYCODE_F3:
      return MWKEY_F3;
    case KEYCODE_F4:
      return MWKEY_F4;
    case KEYCODE_F5:
      return MWKEY_F5;
    case KEYCODE_F6:
      return MWKEY_F6;
    case KEYCODE_F7:
      return MWKEY_F7;
    case KEYCODE_F8:
      return MWKEY_F8;
    case KEYCODE_F9:
      return MWKEY_F9;
    case KEYCODE_F10:
      return MWKEY_F10;
    case KEYCODE_F11:
      return MWKEY_F11;
    case KEYCODE_F12:
      return MWKEY_F12;
    case KEYCODE_CAPSLOCK:
      return MWKEY_CAPSLOCK;
    case KEYCODE_NUMLOCK:
      return MWKEY_NUMLOCK;
    case KEYCODE_SCROLLLOCK:
      return MWKEY_SCROLLOCK;
    case KEYCODE_PAUSE:
      return MWKEY_PAUSE;
    case KEYCODE_PRTSCRN:
      return MWKEY_PRINT;
    case KEYCODE_MENU:
      return MWKEY_MENU;

    default:
      if (code < 128)
        {
          return (MWKEY)code;
        }
      return MWKEY_UNKNOWN;
    }
}

static void update_modifiers(MWKEYMOD *modifiers, MWKEY key, int press)
{
  MWKEYMOD mask = 0;

  switch (key)
    {
    case MWKEY_LSHIFT:
      mask = MWKMOD_LSHIFT;
      break;
    case MWKEY_RSHIFT:
      mask = MWKMOD_RSHIFT;
      break;
    case MWKEY_LCTRL:
      mask = MWKMOD_LCTRL;
      break;
    case MWKEY_RCTRL:
      mask = MWKMOD_RCTRL;
      break;
    case MWKEY_LALT:
      mask = MWKMOD_LALT;
      break;
    case MWKEY_RALT:
      mask = MWKMOD_RALT;
      break;
    case MWKEY_LMETA:
      mask = MWKMOD_LMETA;
      break;
    case MWKEY_RMETA:
      mask = MWKMOD_RMETA;
      break;
    case MWKEY_NUMLOCK:
      mask = MWKMOD_NUM;
      break;
    case MWKEY_CAPSLOCK:
      mask = MWKMOD_CAPS;
      break;
    }

  if (mask)
    {
      if (press)
        {
          *modifiers |= mask;
        }
      else
        {
          *modifiers &= ~mask;
        }
    }
}

#endif
