/*
 * nxkbd.c - Software popup keyboard for Nano-X
 *
 * Copyright (C) 2000 by Greg Haerr <greg@censoft.com>
 *	linked-in bitmaps, redrawn keymaps
 *	enhanced shift/control function, fnkeys
 *	supports all ascii values 1-127
 * Copyright (C) 2000 by VTech Informations LTD.
 * Vladimir Cotfas <vladimircotfas@vtech.ca> Aug 31, 2000
 *   initial port to Nano-X
 * Copyright (C) 2000 by Jay Carlson
 *   initial soft kbd for W
 *
 * This code is licensed with the same license as Microwindows.
 * 
 * #define KBDPIPE in srvconn.c for named pipe keyboard driver.
 * Otherwise, the GrInjectKeyboardEvent method is used.
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#define TITLE 		"Soft Keyboard"
#define DISKIMAGES	0		/* =0 use linked-in images*/
#define _SOFTKBD_DEBUG	0

/* kbd states, each with unique bitmap*/
#define NORM		1000
#define CNTRL		1001
#define SHIFT		1002
#define NUM 		1003
#define INTL 		1004

/* special chars*/
#define BS		'\b'		/* value of <- on kbd*/
#define F1		2000
#define NONE		3000

/* number of charcodes per bitmap*/
#define SCANCODES	41

/* size of bitmaps*/
#define BM_WIDTH 	160
#define BM_HEIGHT 	61

struct keycolumn {
        short xoffset;
        short scancode;
};

struct keyrow {
        short yoffset;
        short height;
        struct keycolumn columns[12];
};

/* fixed layout for each scancode location*/
struct keyrow keyrows[4] = {
        {0, 15, 
         {{0, 0}, {14, 1}, {28, 2}, {42, 3}, {56, 4}, {70, 5}, {84, 6}, {98, 7}, {112, 8}, {126, 9}, {140, 10}, {999, -1}}},
        {15, 15, 
         {{0, 11}, {14, 12}, {28, 13}, {42, 14}, {56, 15}, {70, 16}, {84, 17}, {98, 18}, {112, 19}, {126, 20}, {140, 21}, {999, -1}}},
        {30, 15, 
         {{0, 22}, {19, 23}, {33, 24}, {47, 25}, {61, 26}, {75, 27}, {89, 28}, {103, 29}, {117, 30}, {131, 31}, {145, 32}, {999, -1}} },
        {45, 15, 
         {{0, 33}, {21, 34}, {36, 35}, {85, 36}, {103, 37}, {117, 38}, {131, 39}, {145, 40}, {999, -1}}}
};

#define C(x)	((x)&0x1f)

/* charcode mappings per kbd state*/
static short normal[SCANCODES] = {
 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', BS,
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '-', '\r',
 CNTRL, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', ';',
 SHIFT, INTL, ' ', NUM, '\'', '=', '\\', '/'
};

static short control[SCANCODES] = {
 C('q'),C('w'),C('e'),C('r'),C('t'),C('y'),C('u'),C('i'),C('o'),C('p'),'\033', 
 C('a'),C('s'),C('d'),C('f'),C('g'),C('h'),C('j'),C('k'),C('l'),C('_'),'\r',
 CNTRL,C('z'),C('x'),C('c'),C('v'),C('b'),C('n'),C('m'),C('\\'),C(']'),C('^'),
 SHIFT,INTL,' ',NUM,NONE,NONE,NONE,'\177'
};

static short shift[SCANCODES] = {
 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', BS,
 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '_', '\r',
 CNTRL, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', ':',
 SHIFT, INTL, ' ', NUM, '"', '+', '|', '?'
};

static short num[SCANCODES] = {
 '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', BS,
 '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '\r',
 CNTRL, F1, F1+1, F1+2, F1+3, F1+4, F1+5, F1+6, F1+7, '`', '~',
 SHIFT, INTL, ' ', NUM, '[', ']', '{', '}'};

static char *fnkey[] = {
	"\033OP", "\033OQ", "\033OR", "\033OS", "\03315~",
	"\03317~", "\03318~", "\03319~", "\03320~", "\03321~"
};

struct layout_state {
        char *filename;
        GR_IMAGE_HDR *imagehdr;
        short *scancode_translations;
        short sshift;
        short ctrl_layout, shift_layout, num_layout;
};

/* auto-converted .bmp files for internal linking*/
extern GR_IMAGE_HDR image_keynorm;
extern GR_IMAGE_HDR image_keyctrl;
extern GR_IMAGE_HDR image_keyshft;
extern GR_IMAGE_HDR image_keynum;

static struct layout_state layout_states[] = {
        { /* 0 */
                "keynorm.bmp",
                &image_keynorm,
                normal,
                0,
                2, 1, 3
        }, 
        { /* 1 */
                "keyshft.bmp",
                &image_keyshft,
                shift,
                0,
                2, 0, 3
        }, 
        { /* 2 */
                "keyctrl.bmp",
                &image_keyctrl,
                control,
                1, 		/* ctrl is single shift*/
                0, 1, 3
        },
        { /* 3 */
                "keynum.bmp",
                &image_keynum,
                num,
                0,
                2, 1, 0
        }
};
#define N_LAYOUT_STATES (sizeof(layout_states) / sizeof(layout_states[0]))

static GR_WINDOW_ID	w;             
static GR_GC_ID		gc;            /* graphics context for text */
static int		current_layout = 0;
#if DISKIMAGES
static GR_IMAGE_ID	layout_images[N_LAYOUT_STATES];
static int		layout_images_loaded[N_LAYOUT_STATES];
#endif

extern int KbdWrite(int c);
extern int KbdOpen(void);
extern void KbdClose(void);

static void 
push_character(int c)
{
#if _SOFTKBD_DEBUG
	fprintf(stderr, "pushed %d (0x%x) '%c'\n", c, c, c);
#endif
	KbdWrite(c);
}

static void
display_layout(int layout) 
{
#if DISKIMAGES
#define LIBDIR "."	/* "/etc/nxkbd.d" */
	if (!layout_images_loaded[layout] ) {
        	char buf[128];

		/* 	 
		 * OK, load image on the server-side ;-) 
		 * DON'T check for errors ;(
		 */     
		sprintf(buf, "%s/%s", LIBDIR, layout_states[layout].filename);
		layout_images[layout] = GrLoadImageFromFile(buf, 0);
		layout_images_loaded[layout] = 1;
	}
	GrDrawImageToFit(w, gc, 0, 0, -1, -1, layout_images[layout]);
#else
	GrDrawImageBits(w, gc, 0, 0, layout_states[layout].imagehdr);
#endif
}

static void
process_scancode(int scancode)
{
        int c;
	
        c = layout_states[current_layout].scancode_translations[scancode];
#if _SOFTKBD_DEBUG
	printf("scancode = %d ", scancode);
	printf("current_layout = %d ('%s'), scancode (translated) = %d\n", 
	        current_layout, layout_states[current_layout].filename, c);
#endif

        switch (c) {
        default:
		if (c < 256)		/* normal character*/
			break;

		/* handle special functions*/
		if (c >= F1 && c < F1+10) {
			char *p = fnkey[c-F1];
			while (*p)
				push_character(*p++);
			return;
		}

		/* no action for NONE*/
		if (c == NONE)
			return;
                fprintf(stderr, "nxkbd: key with unknown translation pressed\n");
                return;
        case CNTRL:
                current_layout = layout_states[current_layout].ctrl_layout;
                display_layout(current_layout);
                return;
        case SHIFT:
                current_layout = layout_states[current_layout].shift_layout;
                display_layout(current_layout);
                return;
        case NUM:
                current_layout = layout_states[current_layout].num_layout;
                display_layout(current_layout);
                return;
	case INTL:
#if _SOFTKBD_DEBUG
		printf("INTL not yet implemented\n");
#endif
		return;
        }

        if (layout_states[current_layout].sshift) {
                current_layout = 0;
                display_layout(current_layout);
        }

        push_character(c);
}


static void 
mouse_hit(int x, int y) 
{
        int row, column;

        for (row = 0; row < 4; row++) {
                if (y >= keyrows[row].yoffset &&
		    y < keyrows[row].yoffset+keyrows[row].height) {
                        for (column = 0; column < 12; column++) {
                                if (keyrows[row].columns[column].xoffset == 999) {
                                        fprintf(stderr, "off end of row\n");
                                        return;
                                }
                                if (x < keyrows[row].columns[column + 1].xoffset) {
                                        int scancode = keyrows[row].columns[column].scancode;
                                        process_scancode(scancode);
                                        return;
                                }
                        }
                }
        }

        fprintf(stderr, "nxkbd: off bottom\n");
}
                                
int
main(int argc, char* argv[])
{
        GR_EVENT        event;          /* current event */
	GR_WM_PROPERTIES props;

        if (GrOpen() < 0) {
                fprintf(stderr, "nxkbd: cannot open graphics\n");
                exit(1);
        }

	if (KbdOpen() < 0) {
                fprintf(stderr, "nxkbd: cannot open kbd named pipe\n");
#if 0
                exit(1);
#endif
        }
    
#if !DISKIMAGES
        GrReqShmCmds(4096); 		/* fast image copy*/
#endif
        w = GrNewWindow(GR_ROOT_WINDOW_ID, 
                        0, 0, BM_WIDTH, BM_HEIGHT, 
                        0, WHITE, BLACK);

        GrSelectEvents(w, GR_EVENT_MASK_CLOSE_REQ |
                          GR_EVENT_MASK_EXPOSURE |
			  /*GR_EVENT_MASK_FOCUS_IN |*/
			  /*GR_EVENT_MASK_KEY_DOWN |*/	/* required for focus*/
                          GR_EVENT_MASK_BUTTON_DOWN);

#if 0	/* this code fails when link-app-into-server */
	if (props.title)	/* can't free with link-into-server*/
		free(props.title);

	/* title must be alloc'd and copied*/
	props.title = malloc(18);
	if (props.title)
		strcpy(props.title, TITLE);

	props.flags =
		GR_WM_FLAG_NORESIZE   | /* don't let user resize window */
		GR_WM_FLAG_NOBORDERS  | /* don't draw any window borders */
		GR_WM_FLAG_NOTITLEBAR | /* don't draw a title bar */
		GR_WM_FLAG_NOFOCUS;     /* don't set focus to this window*/
#endif

	props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOFOCUS;
	props.props |= /*GR_WM_PROPS_NOMOVE |*/ GR_WM_PROPS_NORAISE |
		GR_WM_PROPS_BORDER | GR_WM_PROPS_CAPTION | GR_WM_PROPS_CLOSEBOX;
	props.title = TITLE;
	GrSetWMProperties(w, &props);

        GrMapWindow(w);

        gc = GrNewGC();

        current_layout = 0;
        for (;;) {
                GrGetNextEvent(&event);
		
                switch(event.type) {
			case GR_EVENT_TYPE_CLOSE_REQ:
#if DISKIMAGES
				{
					int i;
				
					for(i=0; i < N_LAYOUT_STATES; i++) {
						if( !layout_images_loaded[i] )
							continue;
						GrFreeImage(layout_images[i]);
					}
				}
#endif
				GrClose();
				exit(0);
				/* no return*/
			case GR_EVENT_TYPE_EXPOSURE:
				display_layout(current_layout);
				break;
			case GR_EVENT_TYPE_BUTTON_DOWN:
				mouse_hit(event.button.x, event.button.y);
				break;
#if 0
			case GR_EVENT_TYPE_FOCUS_IN:
				if (event.general.otherid != 1) {
					int lastfocus = event.general.otherid;
					GrSetFocus(lastfocus);
				}
				break;
#endif
		}
	}
	
	/*NOTREACHED*/
	return 0;
 }
