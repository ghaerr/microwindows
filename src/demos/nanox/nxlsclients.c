/*
 * Copyright (C) 2000 by VTech Informations LTD.
 * This program is released under MPL.
 *
 * Vladimir Cotfas <vladimircotfas@vtech.ca> Oct 25, 2000
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
#include "windef.h"

/* which is the maximim window id?!?!*/
#define LIMIT	10000

struct _colour {
	unsigned long colour;
	char* name;
};
typedef struct _colour COLOURS;

static COLOURS colour_table[] = {
	{ BLACK, "Black" },
	{ BLUE, "Blue" },
	{ GREEN, "Green"},
	{ CYAN, "Cyan" },
	{ RED, "Red" },
	{ MAGENTA, "Magenta" },
	{ BROWN, "Brown" },
	{ LTGRAY, "LightGray" },
	{ GRAY, "Gray" },
	{ DKGRAY, "DarkGray" },
	{ LTBLUE, "LightBlue" },
	{ LTGREEN, "LightGreen" },
	{ LTCYAN, "LightCyan" },
	{ LTRED, "LightRed" },
	{ LTMAGENTA, "LightMagenta" },
	{ YELLOW, "Yellow" },
	{ WHITE, "White" }
};
#define NR_COLOURS	(sizeof(colour_table) / sizeof(colour_table[0]))

char*
lookupColour(unsigned long c)
{
	int i;

	for (i = 0; i < NR_COLOURS; i++)
		if (c == colour_table[i].colour)
			return colour_table[i].name;

	return "UanbleToComply";
}

struct _xlat {
        unsigned event_type;
        char* event_desc;
};
typedef struct _xlat XLAT;

static XLAT event_table[] = {
        { GR_EVENT_TYPE_ERROR,          "GR_EVENT_MASK_ERROR"           },
/*      { GR_EVENT_TYPE_NONE,           "GR_EVENT_MASK_NONE"            }, */
        { GR_EVENT_TYPE_EXPOSURE,       "GR_EVENT_MASK_EXPOSURE"        },
        { GR_EVENT_TYPE_BUTTON_DOWN,    "GR_EVENT_MASK_BUTTON_DOWN"     },
        { GR_EVENT_TYPE_BUTTON_UP,      "GR_EVENT_MASK_BUTTON_UP"       },
        { GR_EVENT_TYPE_MOUSE_ENTER,    "GR_EVENT_MASK_MOUSE_ENTER"     },
        { GR_EVENT_TYPE_MOUSE_EXIT,     "GR_EVENT_MASK_MOUSE_EXIT"      },
        { GR_EVENT_TYPE_MOUSE_MOTION,   "GR_EVENT_MASK_MOUSE_MOTION"    },
        { GR_EVENT_TYPE_MOUSE_POSITION, "GR_EVENT_MASK_MOUSE_POSITION"  },
        { GR_EVENT_TYPE_KEY_DOWN,       "GR_EVENT_MASK_KEY_DOWN"        },
        { GR_EVENT_TYPE_KEY_UP,         "GR_EVENT_MASK_KEY_UP"          },
        { GR_EVENT_TYPE_FOCUS_IN,       "GR_EVENT_MASK_FOCUS_IN"        },
        { GR_EVENT_TYPE_FOCUS_OUT,      "GR_EVENT_MASK_FOCUS_OUT"       },
        { GR_EVENT_TYPE_FDINPUT,        "GR_EVENT_MASK_FDINPUT"         },
        { GR_EVENT_TYPE_UPDATE,         "GR_EVENT_MASK_UPDATE"          },
        { GR_EVENT_TYPE_CHLD_UPDATE,    "GR_EVENT_MASK_CHLD_UPDATE"     },
        { GR_EVENT_TYPE_CLOSE_REQ,      "GR_EVENT_MASK_CLOSE_REQ"       }
};
#define NR_MASKS        (sizeof(event_table) / sizeof(event_table[0]))

struct _wm_props {
	GR_WM_PROPS prop;
	char* prop_symbol;
	char* prop_descr;
};
typedef struct _wm_props WM_PROPS;

static WM_PROPS props_table[] = {
	/* Window properties */
	{ GR_WM_PROPS_NOBACKGROUND, "GR_WM_PROPS_NOBACKGROUND", "Don't draw window background" },
	{ GR_WM_PROPS_NOFOCUS, "GR_WM_PROPS_NOFOCUS", "Don't set focus to this window" },
	{ GR_WM_PROPS_NOMOVE, "GR_WM_PROPS_NOMOVE", "Don't let user move window" },
	{ GR_WM_PROPS_NORAISE, "GR_WM_PROPS_NORAISE", "Don't let user raise window" },
	{ GR_WM_PROPS_NODECORATE, "GR_WM_PROPS_NODECORATE", "Don't redecorate window" },
	{ GR_WM_PROPS_NOAUTOMOVE, "GR_WM_PROPS_NOAUTOMOVE", "Don't move window on decorating" },

	/* default decoration style */
	{ GR_WM_PROPS_APPWINDOW, "GR_WM_PROPS_APPWINDOW", "Leave appearance to WM" },
	{ GR_WM_PROPS_APPMASK, "GR_WM_PROPS_APPMASK", "Appearance mask" },
	{ GR_WM_PROPS_BORDER, "GR_WM_PROPS_BORDER", "Single line border" },
	{ GR_WM_PROPS_APPFRAME, "GR_WM_PROPS_APPFRAME", "3D app frame" },
	{ GR_WM_PROPS_CAPTION, "GR_WM_PROPS_CAPTION", "Title bar" },
	{ GR_WM_PROPS_CLOSEBOX, "GR_WM_PROPS_CLOSEBOX", "Close box" },
#if 0
	{ GR_WM_PROPS_MAXIMIZED, "GR_WM_PROPS_MAXIMIZED", "Application is maximized" }
#endif
};
#define NR_PROPS        (sizeof(props_table) / sizeof(props_table[0]))

int
main(int argc, char* argv[])
{
	GR_WINDOW_ID w;
	GR_WINDOW_INFO info;

        if (GrOpen() < 0) {
                fprintf(stderr, "nxlsclients: cannot open graphics\n");
                exit(1);
        }

	for (w = 0; w < LIMIT; w++) {
		info.wid = -1; /* self-sabotaged like CCCP */
		
		GrGetWindowInfo(w, &info);
		
		if (info.wid == -1) {
			printf("Query wid = %d, GrGetWindowInfo() is not working!\n", w);
			continue;
		}
		if (info.wid == 0) {
#if 0
			printf("Query wid = %d --> does not exist\n", w);
#endif
			continue;
		}
		printf("Window id = %d\n", info.wid);
		printf("\tAbsolute upper-left X: %d\n", info.x);
		printf("\tAbsolute upper-left Y: %d\n", info.y);
		printf("\tWidth = %d\n", info.width);
		printf("\tHeight = %d\n", info.height);
		printf("\tBorder: size = %d, colour = %s (#%06lX)\n", \
		       info.bordersize,
		       lookupColour(info.bordercolor), info.bordercolor);
		printf("\tBackground colour = %s (#%06lX)\n", \
		       lookupColour(info.background), info.background);

		printf("\tParent = %d\n", info.parent);
		printf("\tFirst child = %d\n", info.child);
		printf("\tNext sibling? = %d\n", info.sibling);

		printf("\t%sinput-only, ", (info.inputonly == TRUE)?"": "not ");
		printf("%smapped", (info.mapped == TRUE)?"": "not ");
		if (info.mapped != TRUE)
			printf(", unmapcount = %d", info.unmapcount);
		printf("\n");

		printf("\tEvent mask (0x%08lX):\n", info.eventmask);
		{
			int i, n = 0;
			GR_EVENT_MASK tmp = info.eventmask;

			for (i = 0; i < NR_MASKS; i++) {
				GR_EVENT_MASK mask = GR_EVENTMASK(event_table[i].event_type);	

				if ((tmp  & mask) == mask) {
					printf("\t\t%s\n", event_table[i].event_desc);
					n++;
				}
			}
			if (!n)
				printf("\t\tGR_EVENT_MASK_NONE (?!?!?)\n");
		}

		/* We don't use info.props, use GrGetWMProperties() intead */
		printf("\tWM Properties:\n");
		{
			GR_WM_PROPERTIES wm_props;

			GrGetWMProperties(w, &wm_props);
			
			printf("\t\tTitle: ");	
			if ((wm_props.flags & GR_WM_FLAGS_TITLE ) == GR_WM_FLAGS_TITLE) 
				printf("'%s'\n", (char*)wm_props.title?:"(null)");
			else
				printf("<untitled>\n");

			printf("\t\tBackground colour: ");
			if ((wm_props.flags & GR_WM_FLAGS_BACKGROUND) == GR_WM_FLAGS_BACKGROUND)
				printf("%s (#%06lX)\n", lookupColour(wm_props.background), 
				       wm_props.background);
			else
				printf("<unspecified>\n");

			printf("\t\tBorder size: ");
			if ((wm_props.flags & GR_WM_FLAGS_BORDERSIZE) == GR_WM_FLAGS_BORDERSIZE)
				printf("%d\n", wm_props.bordersize);
			else
				printf("<unspecified>\n");

			printf("\t\tProperty bits (0x%08lX):\n", wm_props.props);
			{
				int i, n = 0;

				for (i = 0; i < NR_PROPS; i++) {
					GR_WM_PROPS prop = props_table[i].prop;
					if ((wm_props.props & prop) == prop) {
						printf("\t\t\t%s (%s)\n", \
								props_table[i].prop_symbol, \
								props_table[i].prop_descr);
						n++;
					}
				}

				if (!n)
				       	printf("\t\tNONE (?!?!?)\n");
			}

		}
	}

	GrClose();

	return 0;
 }
