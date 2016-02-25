/*
 * fontdemo - freetype font demonstration program for Nano-X
 *
 * Uses buffered windows for automatic double buffering and no blink!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#if defined(__ANDROID__)
  #define FONTNAME      "Roboto-MediumItalic.ttf"; //if android
#else
  /*#define FONTNAME	"lt1-r-omega-serif.ttf"*/
  /*#define FONTNAME	"arial.ttf"*/
  #define FONTNAME	"times.ttf"
#endif

#define FGCOLOR		BLACK
#define BGCOLOR		WHITE

GR_WINDOW_ID	w;
GR_BOOL		aa = GR_TRUE;
char		fontname[200] = FONTNAME;

static void
do_paint(void)
{
	int	i, y = 0;
	GR_GC_ID	gc;
	GR_FONT_ID	font;
	GR_WINDOW_INFO winfo;

	GrGetWindowInfo(w, &winfo);

	gc = GrNewGC();
	GrSetGCUseBackground(gc, GR_FALSE);

	GrSetGCForeground(gc, BGCOLOR);
	GrFillRect(w, gc, 0, 0, winfo.width, winfo.height);

	GrSetGCForeground(gc, FGCOLOR);

	for (i=3; i<=30; ++i) {
		int 	width, height;
		char	buf[64];
		GR_FONT_INFO	finfo;

		height = i * winfo.height / 530;
		width = i * winfo.width / 640;
		font = GrCreateFontEx(fontname, height, width, NULL);

		GrSetFontAttr(font, aa? (GR_TFANTIALIAS|GR_TFKERNING): 0, -1);
		/*GrSetFontRotation(font, 150);*/
		GrSetGCFont(gc, font);

		sprintf(buf, "%d/%d The Quick Brown Fox Jumps Over The Lazy Dog", height, width);
		GrText(w, gc, 0, y, buf, -1, GR_TFASCII|GR_TFTOP);

		GrGetFontInfo(font, &finfo);
		y += finfo.height;

		GrDestroyFont(font);
	}
	GrDestroyGC(gc);

	GrFlushWindow(w);
}

int
main(int ac, char **av)
{
	if (ac > 1)
		strcpy(fontname, av[1]);

	if (GrOpen() < 0)
		exit(1);

	w = GrNewBufferedWindow(GR_WM_PROPS_APPWINDOW, "fontdemo", GR_ROOT_WINDOW_ID,
		10, 10, 640, 530, BGCOLOR);
	GrSelectEvents(w, GR_EVENT_MASK_BUTTON_DOWN|GR_EVENT_MASK_UPDATE|
		GR_EVENT_MASK_KEY_DOWN|GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(w);

	while (1) {
		GR_EVENT event;

		GrGetNextEvent(&event);
		switch (event.type) {
		case GR_EVENT_TYPE_UPDATE:
			switch (event.update.utype) {
			case GR_UPDATE_MAP:			/* initial paint*/
			case GR_UPDATE_SIZE:		/* resize repaint*/
				do_paint();
			}
			break;

		case GR_EVENT_TYPE_BUTTON_DOWN:
			do_paint();
			break;

    	case GR_EVENT_TYPE_KEY_DOWN:
      		switch(event.keystroke.ch) {
        	case 'a':
				aa = !aa;
				do_paint();
          		break;
			}
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
		}
	}
}
