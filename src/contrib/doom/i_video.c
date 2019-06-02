// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for Microwindows
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <alloca.h>
#include "m_swap.h"
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "doomdef.h"
#define MWINCLUDECOLORS
#include "nano-X.h"

static GR_WINDOW_ID	win;
static GR_GC_ID		gc;
static GR_PALETTE	pal;

// Fake mouse handling.
boolean		grabMouse;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int	multiply=1;


//
//  Translates the key 
//
int
xlatekey(int key)
{
    int rc;

    switch(key) {
      case MWKEY_LEFT:	rc = KEY_LEFTARROW;	break;
      case MWKEY_RIGHT:	rc = KEY_RIGHTARROW;	break;
      case MWKEY_DOWN:	rc = KEY_DOWNARROW;	break;
      case MWKEY_UP:	rc = KEY_UPARROW;	break;
      case MWKEY_ESCAPE:rc = KEY_ESCAPE;	break;
      case MWKEY_ENTER:	rc = KEY_ENTER;		break;
      case MWKEY_TAB:	rc = KEY_TAB;		break;
      case MWKEY_F1:	rc = KEY_F1;		break;
      case MWKEY_F2:	rc = KEY_F2;		break;
      case MWKEY_F3:	rc = KEY_F3;		break;
      case MWKEY_F4:	rc = KEY_F4;		break;
      case MWKEY_F5:	rc = KEY_F5;		break;
      case MWKEY_F6:	rc = KEY_F6;		break;
      case MWKEY_F7:	rc = KEY_F7;		break;
      case MWKEY_F8:	rc = KEY_F8;		break;
      case MWKEY_F9:	rc = KEY_F9;		break;
      case MWKEY_F10:	rc = KEY_F10;		break;
      case MWKEY_F11:	rc = KEY_F11;		break;
      case MWKEY_F12:	rc = KEY_F12;		break;
	
      case MWKEY_BACKSPACE:
      case MWKEY_DELETE:rc = KEY_BACKSPACE;	break;

      case MWKEY_PAUSE:	rc = KEY_PAUSE;		break;

      case MWKEY_KP_EQUALS:
      case '=':		rc = KEY_EQUALS;	break;

      case MWKEY_KP_MINUS:
      case '-':		rc = KEY_MINUS;		break;

      case MWKEY_LSHIFT:
      case MWKEY_RSHIFT:rc = KEY_RSHIFT;	break;

      case MWKEY_LCTRL:
      case MWKEY_RCTRL:	rc = KEY_RCTRL;		break;
	
      case MWKEY_LALT:
      case MWKEY_LMETA:
      case MWKEY_RALT:
      case MWKEY_RMETA:	rc = KEY_RALT;		break;
	
      default:		rc = key;		break;
    }
    return rc;
}

void I_ShutdownGraphics(void)
{
  GrClose();
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
}

/* This processes Nano-X events */
void I_GetEvent(GR_EVENT *Event)
{
    int 		ch;
    unsigned int	buttonstate;
    event_t		event;
    int			xpos, ypos, xrel, yrel;
    static int		lastx = 0;
    static int		lasty = 0;

    switch (Event->type)
    {
        case GR_EVENT_TYPE_KEY_DOWN:
	event.type = ev_keydown;
	ch = ((GR_EVENT_KEYSTROKE *)Event)->ch;
	event.data1 = xlatekey(ch);
	D_PostEvent(&event);
        break;

      case GR_EVENT_TYPE_KEY_UP:
	event.type = ev_keyup;
	ch = ((GR_EVENT_KEYSTROKE *)Event)->ch;
	event.data1 = xlatekey(ch);
	D_PostEvent(&event);
	break;

      case GR_EVENT_TYPE_BUTTON_DOWN:
      case GR_EVENT_TYPE_BUTTON_UP:
	buttonstate = ((GR_EVENT_MOUSE *)Event)->buttons;
	event.type = ev_mouse;
	event.data1 = 0
	    | (buttonstate & GR_BUTTON_L ? 1 : 0)
	    | (buttonstate & GR_BUTTON_M ? 2 : 0)
	    | (buttonstate & GR_BUTTON_R ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	break;

      case GR_EVENT_TYPE_MOUSE_MOTION:
	    //if (grabMouse)
		//WarpMouse(screen->w/2, screen->h/2);

	    buttonstate = ((GR_EVENT_MOUSE *)Event)->buttons;
	    xpos = ((GR_EVENT_MOUSE *)Event)->x;
	    ypos = ((GR_EVENT_MOUSE *)Event)->y;
	    xrel = xpos - lastx;
	    yrel = ypos - lasty;
	    lastx = xpos;
	    lasty = ypos;

	    event.type = ev_mouse;
	    event.data1 = 0
	        | (buttonstate & GR_BUTTON_L ? 1 : 0)
	        | (buttonstate & GR_BUTTON_M ? 2 : 0)
	        | (buttonstate & GR_BUTTON_R ? 4 : 0);
	    event.data2 = xrel << 2;
	    event.data3 = -yrel << 2;
	    D_PostEvent(&event);
	break;

      case GR_EVENT_TYPE_CLOSE_REQ:
	I_Quit();
	break;
    }

}

//
// I_StartTic
//
void I_StartTic (void)
{
    GR_EVENT Event;

    do {
    	GrCheckNextEvent(&Event);
	I_GetEvent(&Event);
    } while (Event.type != GR_EVENT_TYPE_NONE);
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{

    static int	lasttic;
    int		tics;
    int		i;

    // draws little dots on the bottom of the screen
    if (devparm) {
	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;
	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	for ( ; i<20*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

	// allocate temp buffer for scaling
	int outpitch = SCREENWIDTH * multiply;
	int outsize = SCREENHEIGHT * multiply * outpitch;
	unsigned char *output = alloca(outsize * sizeof(unsigned char *));

    // scales the screen size before blitting it
    if (multiply == 1)
    {
	unsigned char *olineptr;
	unsigned char *ilineptr;
	int y;

	ilineptr = (unsigned char *) screens[0];
	olineptr = (unsigned char *) output;

	y = SCREENHEIGHT;
	while (y--)
	{
	    memcpy(olineptr, ilineptr, outpitch);
	    ilineptr += SCREENWIDTH;
	    olineptr += outpitch;
	}
    }
    else if (multiply == 2)
    {
	unsigned int *olineptrs[2];
	unsigned int *ilineptr;
	int x, y, i;
	unsigned int twoopixels;
	unsigned int twomoreopixels;
	unsigned int fouripixels;

	ilineptr = (unsigned int *) (screens[0]);
	for (i=0 ; i<2 ; i++) {
	    olineptrs[i] = (unsigned int *)&((unsigned char *)output)[i*outpitch];
        }

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
	    do
	    {
		fouripixels = *ilineptr++;
		twoopixels =	(fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xffff00)
		    |	((fouripixels>>16) & 0xff);
		twomoreopixels =	((fouripixels<<16) & 0xff000000)
		    |	((fouripixels<<8) & 0xffff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
#else
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
#endif
	    } while (x-=4);
	    olineptrs[0] += outpitch/4;
	    olineptrs[1] += outpitch/4;
	}

    }
    else if (multiply == 3)
    {
	unsigned int *olineptrs[3];
	unsigned int *ilineptr;
	int x, y, i;
	unsigned int fouropixels[3];
	unsigned int fouripixels;

	ilineptr = (unsigned int *) (screens[0]);
	for (i=0 ; i<3 ; i++) {
	    olineptrs[i] = 
		(unsigned int *)&((unsigned char *)output)[i*outpitch];
        }

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
	    do
	    {
		fouripixels = *ilineptr++;
		fouropixels[0] = (fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xff0000)
		    |	((fouripixels>>16) & 0xffff);
		fouropixels[1] = ((fouripixels<<8) & 0xff000000)
		    |	(fouripixels & 0xffff00)
		    |	((fouripixels>>8) & 0xff);
		fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
		    |	((fouripixels<<8) & 0xff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
#else
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
#endif
	    } while (x-=4);
	    olineptrs[0] += 2*outpitch/4;
	    olineptrs[1] += 2*outpitch/4;
	    olineptrs[2] += 2*outpitch/4;
	}

    }

#if MWPIXEL_FORMAT == MWPF_TRUECOLORARGB
	/* convert palette screen to 32-bit ARGB and display*/
	int size = SCREENHEIGHT * multiply * SCREENWIDTH * multiply;
	uint32_t *argb, *argb_start;
	argb = argb_start = alloca(size * sizeof(uint32_t));

#define CONVPAL(c)  RGB2PIXEL8888(pal.palette[c].r, pal.palette[c].g, pal.palette[c].b)
	unsigned char *input = output;
	for (i=0; i<size; i++)
	{
		*argb++ = CONVPAL(*input);
		input++;
	}
	GrArea(win, gc, 0, 0, SCREENWIDTH*multiply, SCREENHEIGHT*multiply,
		argb_start, MWPF_TRUECOLORARGB);
#elif MWPIXEL_FORMAT == MWPF_PALETTE
    GrArea(win, gc, 0, 0, SCREENWIDTH*multiply, SCREENHEIGHT*multiply, output, MWPF_PALETTE);
#else
#error MWPIXEL_FORMAT not supported.
#endif
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    int i;
	static int firsttime = 1;

	if (!firsttime)		/* only set palette once to avoid flicker*/
		return;
	firsttime = 0;
    pal.count = 256;
    for ( i=0; i<256; ++i ) {
	pal.palette[i].r = gammatable[usegamma][*palette++];
	pal.palette[i].g = gammatable[usegamma][*palette++];
	pal.palette[i].b = gammatable[usegamma][*palette++];
    }
    GrSetSystemPalette(0, &pal);
}


void I_InitGraphics(void)
{
    int w, h;
    static int	firsttime=1;

    if (!firsttime)
	return;
    firsttime = 0;

    //if (!!M_CheckParm("-fullscreen"))
        //video_flags |= SDL_FULLSCREEN;

    if (M_CheckParm("-2"))
	multiply = 2;

    if (M_CheckParm("-3"))
	multiply = 3;

    // check if the user wants to grab the mouse (quite unnice)
    grabMouse = !!M_CheckParm("-grabmouse");

    w = SCREENWIDTH * multiply;
    h = SCREENHEIGHT * multiply;

    win = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "Microwindows Doom! v1.10",
		GR_ROOT_WINDOW_ID, 10, 10, w, h, BLACK);
    GrSelectEvents(win, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_MOUSE_MOTION | 
		GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP | 
		GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP |
		GR_EVENT_MASK_EXPOSURE);
    gc = GrNewGC();
    GrMapWindow(win);

    //SDL_ShowCursor(0);

    /* Set up the screen displays */
    screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT);
    if ( screens[0] == NULL )
            I_Error("Couldn't allocate screen memory");
}
