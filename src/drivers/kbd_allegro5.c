/*
 * Allegro/Android Keyboard Driver
 *
 * Georg Potthast 2015
 */
#include <string.h>
#include <stdio.h>
#include "device.h"

/* Windows GUI applications start with a WinMain() entry point, rather than the standard main() 
entry point. Allegro is configured to build GUI applications by default and to do some magic in 
order to make a regular main() work with them, but you have to help it out a bit by writing 
END_OF_MAIN() right after your main() function. If you don't want to do that, you can just 
include winalleg.h and write a WinMain() function. Note that this magic may bring about conflicts 
with a few programs using direct calls to Win32 API functions; for these programs, the regular 
WinMain() is required and the magic must be disabled by defining the preprocessor symbol 
ALLEGRO_NO_MAGIC_MAIN before including Allegro headers.

If you want to build a console application using Allegro, you have to define the preprocessor 
symbol ALLEGRO_USE_CONSOLE before including Allegro headers; it will instruct the library to use 
console features and also to disable the special processing of the main() function described above. 
*/

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro.h>

#if ANDROID
#include <allegro5/allegro_android.h>
#endif

static int  allegro_Open(KBDDEVICE *pkd);
static void allegro_Close(void);
static void allegro_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int  allegro_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int  allegro_Poll(void);

KBDDEVICE kbddev = {
	allegro_Open,
	allegro_Close,
	allegro_GetModifierInfo,
	allegro_Read,
	allegro_Poll	
};

ALLEGRO_DISPLAY *display;
ALLEGRO_EVENT_QUEUE *a_event_queue_k;
ALLEGRO_EVENT_QUEUE *a_event_queue_m;
ALLEGRO_EVENT_QUEUE *a_event_queue_d;
ALLEGRO_EVENT a_event;
ALLEGRO_KEYBOARD_STATE kbdstate;
static int closedown;
static MWKEYMOD save_modifiers;

/*
 * keyboard driver is called first, so initialize allegro here 
 */
int
init_allegro(void)
{
    if(!al_init())
    {
        EPRINTF("Error!, Allegro has failed to initialize.\n");
		return -1;
    }

#if ANDROID
    //al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
    al_set_new_display_flags(ALLEGRO_WINDOWED);
    //al_set_new_display_option(ALLEGRO_COLOR_SIZE,32,ALLEGRO_SUGGEST);
    //al_set_new_display_option(ALLEGRO_CAN_DRAW_INTO_BITMAP,1,ALLEGRO_REQUIRE);
#endif  
    //al_set_new_display_option(ALLEGRO_COLOR_SIZE,32,ALLEGRO_SUGGEST);
    display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);

    if(display == NULL)
    {
        EPRINTF("Error!, Failed to create the display.");
		return -1;
    }    
    al_set_system_mouse_cursor(display,ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);

    if(!al_install_keyboard())
    {
        EPRINTF("Error!, Failed to install keyboard.\n");
        return -1;
    }

    if(!al_install_mouse())
    {
        EPRINTF("Error!, Failed to install mouse.");
        return -1;
    }    

    a_event_queue_k = al_create_event_queue();
    if(a_event_queue_k == NULL)
    {
       EPRINTF("Error!, Failed to create the keyboard event queue.");
       return -1;
    }    
    al_register_event_source(a_event_queue_k, al_get_keyboard_event_source());

    a_event_queue_m = al_create_event_queue();
    if(a_event_queue_m == NULL)
    {
       EPRINTF("Error!, Failed to create the mouse event queue.");
       return -1;
    }    
#if !ANDROID
    al_register_event_source(a_event_queue_m, al_get_mouse_event_source());
#endif
    a_event_queue_d = al_create_event_queue();
    if(a_event_queue_d == NULL)
    {
       EPRINTF("Error!, Failed to create the display event queue.");
       return -1;
    }    
    al_register_event_source(a_event_queue_d, al_get_display_event_source(display));

#if ANDROID
    if(!al_install_touch_input())
    {
      EPRINTF("Error!, Failed to install touch_input.");	
    } else {
      al_set_mouse_emulation_mode(ALLEGRO_MOUSE_EMULATION_5_0_x);
      al_register_event_source(a_event_queue_m,
	  	al_get_touch_input_mouse_emulation_event_source());
    }
#endif 

#if ANDROID
	//allow to use al_open to read (only) files from the apk store, e.g. fonts
    al_android_set_apk_file_interface();
#endif

    return 1; //ok    
}
 
/*
 * Open the keyboard.
 */
static int
allegro_Open(KBDDEVICE *pkd)
{

  if (init_allegro() != 1)
  	return DRIVER_OKNULLDEV; //no kbd

  return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the keyboard.
 */
static void
allegro_Close(void)
{
    al_destroy_display(display);
    al_destroy_event_queue(a_event_queue_k);
    al_destroy_event_queue(a_event_queue_m);
    al_destroy_event_queue(a_event_queue_d);
}

/*
 * Poll entry point
 */
static int
allegro_Poll(void)
{
  if (!al_is_keyboard_installed())
  	return 0;

  if (al_peek_next_event(a_event_queue_k, &a_event))
  	return 1; 			//read event in read function
  
  if (al_get_next_event(a_event_queue_d, &a_event)) //remove any event from display queue
  {
    if (a_event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
	{
         closedown = 1;
         return 1; 		// received the quit key
    }
  }
  return 0;
}

/*
 * Return the possible modifiers for the keyboard.
 */
static void
allegro_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
	   *curmodifiers = save_modifiers;
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the modifier keys (ALT, SHIFT, etc).  Returns -1 on error, 0 if no data
 * is ready, 1 on a keypress, and 2 on keyrelease.
 * This is a non-blocking call.
 */
static int
allegro_Read(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
  static int mwkey,scanvalue;

  if (closedown)
  	return KBD_QUIT; /* terminate application*/

  if (!al_is_keyboard_installed())
  	return KBD_NODATA;

  if (al_get_next_event(a_event_queue_k, &a_event))
  {
     /* there are further key events */
	 if (a_event.type == ALLEGRO_EVENT_KEY_CHAR)
	 {
	  mwkey = a_event.keyboard.unichar & 0xff;
	  scanvalue = a_event.keyboard.keycode; 
	  al_get_keyboard_state(&kbdstate);
     } else {
         if (a_event.type == ALLEGRO_EVENT_KEY_UP) {
            *kbuf = mwkey;		
            *scancode = scanvalue;
   			*modifiers = 0;
            return KBD_KEYRELEASE; //key released    
	    } else {
	        return KBD_NODATA;
        }
	}

   //if (al_key_down(&kbdstate, ALLEGRO_KEY_ESCAPE))
   	//return KBD_QUIT; /* special case ESC - terminate application*/

   *modifiers = 0;
   if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_SHIFT)) *modifiers |= MWKMOD_SHIFT;
   if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_CTRL))  *modifiers |= MWKMOD_CTRL;
   if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_ALT))   *modifiers |= MWKMOD_ALT;
   if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_ALTGR)) *modifiers |= MWKMOD_ALT;
   if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_LWIN))  *modifiers |= MWKMOD_META;
   if (al_key_down(&kbdstate, ALLEGRO_KEYMOD_RWIN))  *modifiers |= MWKMOD_META;
   save_modifiers = *modifiers;
   
    *kbuf = mwkey;		
    *scancode = scanvalue;
    return KBD_KEYPRESS;		/* keypress received*/
  } 
  return KBD_NODATA;
}
