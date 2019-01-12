/* Keymapings for the Zaurus handheld computer */

/* Missing kernel keys:
   bar (32)
   +adiaeresis (51)
   +udiaeresis (52)
   +odiaeresis (53)
   +Adiaeresis (54)
   +Udiaeresis (55)
   +Odiaeresis (56)
   +ssharp (57)
*/

/* Kernel map -> our map:
   F2 = MWKEY_MENU
   Control (88) -> MWKEY_APP1
   Alt (89) -> MWKEY_APP2
   None (90) -> MWKEY_APP4
   Space (91) -> MWKEY_ACCEPT
*/
   
static MWKEY keymap[128] = {
  0x0, 'a', 'b', 'c', 'd', 'e', 'f', 'g',                                               /* 00 */
  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',                                               /* 08 */
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',                                               /* 10 */ 
  'x', 'y', 'z', MWKEY_LSHIFT, MWKEY_ENTER, MWKEY_MENU, 0x0, MWKEY_BACKSPACE,            /* 18 */
  0x0, 0x0, MWKEY_ESCAPE, MWKEY_LEFT, MWKEY_UP, MWKEY_DOWN, MWKEY_RIGHT, MWKEY_ENTER,   /* 20 */
  0x0, '1', '2', '3', '4', '5','6', '7',                                                /* 28 */
  '8', '9', '0', 0x0, 0x0, 0x0, 0x0, 0x0,                                                /* 30 */
  0x0, 0x0, '-', '+', MWKEY_CAPSLOCK, '@', '?', ',',                                    /* 38 */     
  '.', MWKEY_TAB, MWKEY_F5, MWKEY_F6, MWKEY_F7, '/', '\'', ';',                         /* 40 */
  '\"', ':', '#', '$', '%', '_', '&', '*',                                              /* 48 */
  '(', MWKEY_BACKSPACE, MWKEY_F10, '=', ')', '~', '<', '>',                             /* 50 */
  MWKEY_APP1, MWKEY_APP2, MWKEY_APP4, MWKEY_ACCEPT, ' ',  0x0, '!',  0x0,               /* 58 */
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, MWKEY_RSHIFT,                                       /* 60 */
  MWKEY_LCTRL, MWKEY_RCTRL, MWKEY_LALT, MWKEY_RALT, MWKEY_ALTGR, 0x0, 0x0, 0x0,         /* 68 */
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,                                               /* 70 */
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0                                                /* 78 */
};

