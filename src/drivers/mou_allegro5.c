/*
 * Allegro/Android Mouse Driver
 *
 * written by Georg Potthast 2012
 */
#include <stdio.h>
#include <stdlib.h>
#include "device.h"

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro.h>

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	mallegro_Open(MOUSEDEVICE *pmd);
static void 	mallegro_Close(void);
static int  	mallegro_GetButtonInfo(void);
static void		mallegro_GetDefaultAccel(int *pscale,int *pthresh);
static int  	mallegro_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int  	mallegro_Poll(void);

MOUSEDEVICE mousedev = {
	mallegro_Open,
	mallegro_Close,
	mallegro_GetButtonInfo,
	mallegro_GetDefaultAccel,
	mallegro_Read,
	mallegro_Poll,
};

extern ALLEGRO_DISPLAY *display;
extern ALLEGRO_BITMAP *display_bitmap;
extern ALLEGRO_EVENT_QUEUE *a_event_queue_m;
extern ALLEGRO_EVENT a_event;
ALLEGRO_MOUSE_STATE mstate;
extern ALLEGRO_BITMAP *scrmem;
extern int zoomfactor;

/*
 * Mouse Poll
 */
static int
mallegro_Poll(void)
{
#if ANDROID
	if(al_is_bitmap_locked(scrmem))
	{
    	al_unlock_bitmap(scrmem);
    	al_set_target_bitmap(al_get_backbuffer(display));
    	//al_draw_bitmap(scrmem, 0, 0, 0);
    	al_draw_scaled_rotated_bitmap(scrmem, 0, 0, 0, 0, zoomfactor, zoomfactor, 0, 0);
    	//al_draw_scaled_rotated_bitmap(scrmem, 0, al_get_bitmap_height(al_get_backbuffer(display)), 0, 0, 2, 2, ALLEGRO_PI/2, 0);
    	//al_draw_scaled_rotated_bitmap(scrmem, 0, 500, 0, 0, 3, 3, ALLEGRO_PI/2, 0); //would have to recalculate mouse
    	al_flip_display();
	}  
#else
    if(al_is_bitmap_locked(display_bitmap)) {
      al_unlock_bitmap(display_bitmap);       
      al_flip_display(); 
    }
#endif

  if (!al_is_mouse_installed())
  	return 0;
  
  if (al_peek_next_event(a_event_queue_m, &a_event))
  	return 1; 			//read event in read function

  return 0;
}

/*
 * Open up the mouse device.
 */
static int
mallegro_Open(MOUSEDEVICE *pmd)
{
  al_hide_mouse_cursor(display); //here, or two mouse cursors
      
  return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the mouse device.
 */
static void
mallegro_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static int
mallegro_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP | MWBUTTON_SCROLLDN;
}

/*
 * Get default mouse acceleration settings
 */
static void mallegro_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/*
 * Read Mouse, non-blocking.
 */
static int
mallegro_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
	int buttons = 0;
	int mickeyz = 0;
	static int mz; 
  
  if (!al_is_mouse_installed())
  	return MOUSE_NODATA;

    al_get_next_event(a_event_queue_m, &a_event); //remove from queue

	switch(a_event.type) {
    case ALLEGRO_EVENT_MOUSE_AXES:
    case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
    case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        break; 

    case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:        
    case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
    default:
        return MOUSE_NODATA;
	}

	al_get_mouse_state_axis(&mstate, 2); // 2= read z-axis vertical wheel	
	//calculate wheel button (up/down)
	if(mstate.z != mz)
	    mickeyz = mstate.z - mz;
	else
	    mickeyz = 0;
	mz = mstate.z;
	
	al_get_mouse_state(&mstate); //call above returns no button press
	//microwindows expects the mouse position at the unzoomed position - so divide
	*dx=mstate.x/zoomfactor; 
	*dy=mstate.y/zoomfactor;
    *dz = 0; //unused
	*bp = 0;

	if (mstate.buttons & 1)		/* Primary (e.g. left) mouse button is held. */
    	buttons |= MWBUTTON_L;
	if (mstate.buttons & 2)		/* Secondary (e.g. right) mouse button is held. */
    	buttons |= MWBUTTON_R;
	if (mstate.buttons & 4)		/* Tertiary (e.g. middle) mouse button is held. */
    	buttons |= MWBUTTON_M;
	if (mickeyz > 0)
    	buttons |= MWBUTTON_SCROLLUP;  
	if (mickeyz < 0)
    	buttons |= MWBUTTON_SCROLLDN;

	*bp = buttons;

	return MOUSE_ABSPOS; 		// return absolute mouse position
}
