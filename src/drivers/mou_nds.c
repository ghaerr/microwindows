/**
 * Microwindows Nintendo DS Mouse Driver
 * Copyright (c) 2011 Derek Carter
 * Portions Copyright (c) 1999, 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 */
#include <nds.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#define KEY_ALL_DIR   ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT )
#define KEY_ALL_ABXY  ( KEY_A | KEY_B | KEY_X | KEY_Y )
#define KEY_ALL_LR    ( KEY_L | KEY_R )
#define KEY_ALL_FRONT ( KEY_ALL_ABXY | KEY_TOUCH | KEY_ALL_DIR )
#define KEY_ALL       ( KEY_ALL_FRONT | KEY_ALL_LR )

/* local routines*/
static int  	NDS_Open(MOUSEDEVICE *pmd);
static void 	NDS_Close(void);
static int  	NDS_GetButtonInfo(void);
static void	NDS_GetDefaultAccel(int *pscale,int *pthresh);
static int  	NDS_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bptr);
static int	NDS_Poll(void);

touchPosition cur_pos;
touchPosition new_pos;
KEYPAD_BITS cur_keypad;
KEYPAD_BITS new_keypad;

MOUSEDEVICE mousedev = {
	NDS_Open,
	NDS_Close,
	NDS_GetButtonInfo,
	NDS_GetDefaultAccel,
	NDS_Read,
	NDS_Poll,
    MOUSE_NORMAL    /* flags*/
};

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
NDS_Open(MOUSEDEVICE *pmd)
{
  touchRead( &cur_pos );
  cur_keypad = keysCurrent();
  return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the mouse device.
 */
static void
NDS_Close(void)
{

}

/*
 * Get mouse buttons supported
 */
static int
NDS_GetButtonInfo(void)
{
  // Supports left and right buttons
  return MWBUTTON_L | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
NDS_GetDefaultAccel(int *pscale,int *pthresh)
{
  *pscale = SCALE;
  *pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 * status 0 = error
 * status 1 = relative
 * status 2 = exact
 * status 3 = don't move mouse
 */
static int
NDS_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
  int delta_keypad = (new_keypad ^ cur_keypad );

  *bp = 0;
  *dx = 0;
  *dy = 0;
  *dz = 0;

  // If touch is being pressed right now
  if ( new_keypad & KEY_TOUCH )
    {
      touchRead( &new_pos );
      // On touching the mouse reset current to new
      if (!(delta_keypad & KEY_TOUCH))
	{
	  scanKeys();
	  if (keysHeld() & KEY_TOUCH)
	    {
	      *dx = new_pos.px - cur_pos.px;
	      *dy = new_pos.py - cur_pos.py;
	    }
	}
      cur_pos = new_pos;
    }

  if (new_keypad & KEY_Y)
    {
      *bp = MWBUTTON_L;
    }
  if (new_keypad & KEY_A)
    {
      *bp = MWBUTTON_R;
    }
  
  cur_keypad = new_keypad;
  return MOUSE_RELPOS;
}

/*
 * Poll for events
 * 1 = activity
 * 0 = nothing
 */

static int
NDS_Poll(void)
{
  scanKeys();
  new_keypad = keysCurrent();

  // Check for touch changes and movement
  if ( ( ( new_keypad ^ cur_keypad ) & KEY_ALL_FRONT ) ||
       ( new_keypad & KEY_ALL_FRONT ) )
  {
    return 1;
  }
  
  return 0;
}
