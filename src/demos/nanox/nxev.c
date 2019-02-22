/*
 * Copyright (C) 2000 by VTech Informations LTD
 *
 * This program is released under MPL
 *
 * Vladimir Cotfas <vladimircotfas@vtech.ca>, Nov 7, 2000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "nano-X.h"


static	GR_WINDOW_ID	w1;		/* id for large window */
static	GR_WINDOW_ID	w2;		/* id for child window */
static	GR_GC_ID	gc1;		/* graphics context for text */

#define ALT_CURSOR

#define	WIDTH	100
#define	HEIGHT	100

#ifdef ALT_CURSOR
/*
 * Definitions to make it easy to define cursors
 */
#define _       ((unsigned) 0)          /* off bits */
#define X       ((unsigned) 1)          /* on bits */
#define MASK(a,b,c,d,e,f,g) \
        (((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
        + e) * 2) + f) * 2) + g) << 9)
#endif

struct _xlat {
	GR_UPDATE_TYPE type;
	const char* name;
};
typedef struct _xlat XLAT;

static XLAT update_types[] = {
	{ 0,			"none" },
	{ GR_UPDATE_MAP,	"GR_UPDATE_MAP" },
	{ GR_UPDATE_UNMAP,	"GR_UPDATE_UNMAP" },
	{ GR_UPDATE_MOVE, 	"GR_UPDATE_MOVE" },
	{ GR_UPDATE_SIZE,	"GR_UPDATE_SIZE" },
	{ GR_UPDATE_UNMAPTEMP,	"GR_UPDATE_UNMAPTEMP" },
	{ GR_UPDATE_ACTIVATE,	"GR_UPDATE_ACTIVATE" },
	{ GR_UPDATE_DESTROY,	"GR_UPDATE_DESTROY" }
};
#define	NR_UPDATES	7	
		
struct XLAT {
	int event_type;
	char* event_desc;
};

static struct XLAT table[] = {
	{ GR_EVENT_TYPE_ERROR,		"GR_EVENT_TYPE_ERROR"		},
	{ GR_EVENT_TYPE_NONE,		"GR_EVENT_TYPE_NONE"		},
	{ GR_EVENT_TYPE_EXPOSURE,	"GR_EVENT_TYPE_EXPOSURE"	},
	{ GR_EVENT_TYPE_BUTTON_DOWN,	"GR_EVENT_TYPE_BUTTON_DOWN"	},
	{ GR_EVENT_TYPE_BUTTON_UP,	"GR_EVENT_TYPE_BUTTON_UP"	},
	{ GR_EVENT_TYPE_MOUSE_ENTER,	"GR_EVENT_TYPE_MOUSE_ENTER"	},
	{ GR_EVENT_TYPE_MOUSE_EXIT,	"GR_EVENT_TYPE_MOUSE_EXIT"	},
	{ GR_EVENT_TYPE_MOUSE_MOTION,	"GR_EVENT_TYPE_MOUSE_MOTION"	},
	{ GR_EVENT_TYPE_MOUSE_POSITION,	"GR_EVENT_TYPE_MOUSE_POSITION"	},
	{ GR_EVENT_TYPE_KEY_DOWN,	"GR_EVENT_TYPE_KEY_DOWN"	},
	{ GR_EVENT_TYPE_KEY_UP,		"GR_EVENT_TYPE_KEY_UP"		},
	{ GR_EVENT_TYPE_FOCUS_IN,	"GR_EVENT_TYPE_FOCUS_IN"	},
	{ GR_EVENT_TYPE_FOCUS_OUT,	"GR_EVENT_TYPE_FOCUS_OUT"	},
	{ GR_EVENT_TYPE_FDINPUT,	"GR_EVENT_TYPE_FDINPUT"		},
	{ GR_EVENT_TYPE_UPDATE,		"GR_EVENT_TYPE_UPDATE"		},
	{ GR_EVENT_TYPE_CHLD_UPDATE,	"GR_EVENT_TYPE_CHLD_UPDATE"	},
	{ GR_EVENT_TYPE_CLOSE_REQ,	"GR_EVENT_TYPE_CLOSE_REQ"	},
};

#define	table_SZ	(sizeof(table) / sizeof(table[0]))

static char* 
lookupEvent(int type)
{
	int i;

	for (i = 0; i < table_SZ; i++)
		if (table[i].event_type == type)
			return table[i].event_desc;

	return NULL;
}

int
main(int argc, char **argv)
{
	GR_EVENT	event;		/* current event */
	
	if (GrOpen() < 0) {
		GrError("cannot open graphics\n");
		return 1;
	}
	
	GrReqShmCmds(65536); /* shared mem is suposed to be faster */
	
	w1 = GrNewWindow(GR_ROOT_WINDOW_ID, 
			0, 0, 
			WIDTH, HEIGHT, 
			4, 
			WHITE, BLACK);
	GrSelectEvents(w1, 0xffffffffl); /* all events :) */
	
	w2 = GrNewWindow(w1, 
			10, 10, 
			WIDTH / 4, HEIGHT / 4, 
			4, 
			WHITE, BLACK);
	GrSelectEvents(w2, 0xffffffffl); /* all events :) */


	{
	  GR_WM_PROPERTIES props;

	  props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;

	  props.props = GR_WM_PROPS_BORDER | GR_WM_PROPS_CAPTION | \
	  		GR_WM_PROPS_CAPTION | GR_WM_PROPS_CLOSEBOX;
	  props.title = "nano-X Events";
	  GrSetWMProperties(w1, &props);
	}

	GrMapWindow(w1);
	GrMapWindow(w2);

	gc1 = GrNewGC();

	GrSetGCForeground(gc1, BLACK);
	GrSetGCBackground(gc1, WHITE);


#ifdef ALT_CURSOR
	{
	        GR_BITMAP       bitmap1fg[7];   /* bitmaps for cursor */
        	GR_BITMAP       bitmap1bg[7];

		bitmap1bg[0] = MASK(X,X,X,X,X,X,X);
		bitmap1bg[1] = MASK(_,X,X,X,X,X,_);
		bitmap1bg[2] = MASK(_,_,X,X,X,_,_);
		bitmap1bg[3] = MASK(_,_,_,X,_,_,_);
		bitmap1bg[4] = MASK(_,_,X,X,X,_,_);
		bitmap1bg[5] = MASK(_,X,X,X,X,X,_);
		bitmap1bg[6] = MASK(X,X,X,X,X,X,X);

		bitmap1fg[0] = MASK(X,X,X,X,X,X,X);
		bitmap1fg[1] = MASK(_,X,X,X,X,X,_);
		bitmap1fg[2] = MASK(_,_,X,X,X,_,_);
		bitmap1fg[3] = MASK(_,_,_,X,_,_,_);
		bitmap1fg[4] = MASK(_,_,X,X,X,_,_);
		bitmap1fg[5] = MASK(_,X,X,X,X,X,_);
		bitmap1fg[6] = MASK(X,X,X,X,X,X,X);

		GrSetCursor(w1, 7, 7, 3, 3, WHITE, BLACK, bitmap1fg, bitmap1bg);
	}
#endif

	
	for (;;) {
		GrGetNextEvent(&event);

		GrError("%s (0x%x)\n", \
		       lookupEvent(event.type), event.type);
		
		switch(event.type) {
		  case GR_EVENT_TYPE_EXPOSURE:
			{
			  GrError("\twid = %d\n", event.exposure.wid);
			  GrError("\t(X, Y) = (%d, %d)\n", \
			         event.exposure.x, event.exposure.y);
			  GrError("\twidth = %d, height = %d\n", \
			  	 event.exposure.width, event.exposure.height);
			}
			break;
		  case GR_EVENT_TYPE_BUTTON_DOWN:
		  case GR_EVENT_TYPE_BUTTON_UP:
			{
			  GrError("\twid = %d\n", event.button.wid);
			  GrError("\tsub-window id = %d\n", event.button.subwid);
			  GrError("\troot window (X, Y) coordinates = (%d, %d)\n", \
			         event.button.rootx, event.button.rooty);
			  GrError("\t(X, Y) = (%d, %d)\n", \
				 event.button.x, event.button.y);
			  GrError("\tbuttons: %04X, ", event.button.buttons);
			  GrError("changed buttons: %04X\n", event.button.changebuttons);
			  GrError("\tmodifiers: %04X\n", event.button.modifiers);
			}
			break;
		  case GR_EVENT_TYPE_MOUSE_ENTER:
		  case GR_EVENT_TYPE_MOUSE_EXIT:
		  case GR_EVENT_TYPE_MOUSE_MOTION:
		  case GR_EVENT_TYPE_MOUSE_POSITION:
			{
			  GrError("\twid = %d\n", event.mouse.wid);
			  GrError("\tsub-window id = %d\n", event.mouse.subwid);
			  GrError("\troot window (X, Y) coordinates = (%d, %d)\n", \
			         event.mouse.rootx, event.mouse.rooty);
			  GrError("\t(X, Y) = (%d, %d)\n", \
				 event.mouse.x, event.mouse.y);
			  GrError("\tbuttons: %04X\n", event.mouse.buttons);
			  GrError("\tmodifiers: %04X\n", event.mouse.modifiers);
			}
			break;
		  case GR_EVENT_TYPE_KEY_DOWN:
		  case GR_EVENT_TYPE_KEY_UP:
			{
			  GrError("\twid = %d\n", event.keystroke.wid);
			  GrError("\tsub-window id = %d\n", event.keystroke.subwid);
			  GrError("\troot window (X, Y) coordinates = (%d, %d)\n", \
			         event.keystroke.rootx, event.keystroke.rooty);
			  GrError("\t(X, Y) = (%d, %d)\n", \
				 event.keystroke.x, event.keystroke.y);
			  GrError("\tbuttons: %04X\n", event.keystroke.buttons);
			  GrError("\tmodifiers: %04X\n", event.keystroke.modifiers);
			  GrError("\tUnicode-16 keyvalue: %d, ASCII: %d\n", \
				 (int)event.keystroke.ch, event.keystroke.ch);
			  GrError("\tscancode: %02X\n",
				(int)event.keystroke.scancode);
			}
			break;
		  case GR_EVENT_TYPE_FOCUS_IN:
			GrError("\twid = %d\n", event.general.wid);
			GrError("\told focus = %d\n", event.general.otherid);
			break;
		  case GR_EVENT_TYPE_FOCUS_OUT:
			GrError("\twid = %d\n", event.general.wid);
			GrError("\tnew focus = %d\n", event.general.otherid);
			break;
		  case GR_EVENT_TYPE_UPDATE:
		  case GR_EVENT_TYPE_CHLD_UPDATE:
			{
			  GrError("\twid = %d\n", event.update.wid);
			  GrError("\tsub-window id = %d\n", event.update.subwid);
			  GrError("\t(X, Y) = (%d, %d)\n", \
				 event.update.x, event.update.y);
			  GrError("\twidth = %d, height = %d\n", \
			  	 event.update.width, event.update.height);
			  {
				  GR_UPDATE_TYPE u = event.update.utype;
				  const char* p;

				  p =  (u > NR_UPDATES)? \
					  "<unknown>": \
					  update_types[u].name;

			  	  GrError("\tupdate_type: %s (%d)\n",
					  p, u);
			  }
			}
			break;
		  case GR_EVENT_TYPE_TIMEOUT:
		  	GrError("\ttimeout?\n");
			break;
		  case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			return 0;
		}
	}

	return 0;
}
