/*
 * fontdemo - freetype font demonstration program for Nano-X
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

/*#define FONTNAME	"lt1-r-omega-serif.ttf"*/
/*#define FONTNAME	"arial.ttf"*/
#define FONTNAME	"times.ttf"

#define FGCOLOR		BLACK
#define BGCOLOR		WHITE

GR_WINDOW_ID	w;
GR_GC_ID	gc;
GR_FONT_ID	font;
GR_BOOL		aa = GR_TRUE;

static void
do_paint(GR_EVENT_EXPOSURE *ep)
{
	int	i, y = 0;

	for (i=3; i<=30; ++i) {
		GR_FONT_INFO	info;
		char		buf[64];

		font = GrCreateFont(FONTNAME, i, NULL);
		if (aa)
			GrSetFontAttr(font, GR_TFANTIALIAS|GR_TFKERNING, 0);
		/*GrSetFontRotation(font, 150);*/
		GrSetGCFont(gc, font);

		sprintf(buf, "%d The Quick Brown Fox Jumped Over The Lazy Dog", i);
		GrText(w, gc, 0, y, buf, -1, GR_TFASCII|GR_TFTOP);

		GrGetFontInfo(font, &info);
		y += info.height;

		GrDestroyFont(font);
	}
}

int
main(int ac, char **av)
{
	if (GrOpen() < 0)
		exit(1);

	w = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "fontdemo", GR_ROOT_WINDOW_ID,
		10, 10, 640, 530, BGCOLOR);
	GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE|GR_EVENT_MASK_BUTTON_DOWN|
		GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(w);

	gc = GrNewGC();
	GrSetGCUseBackground(gc, GR_FALSE);
	GrSetGCForeground(gc, FGCOLOR);
	GrSetGCBackground(gc, BGCOLOR);

	while (1) {
		GR_EVENT event;

		GrGetNextEvent(&event);
		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			do_paint(&event.exposure);
			break;

		case GR_EVENT_TYPE_BUTTON_DOWN:
			{
			GR_WINDOW_INFO info;

			aa = !aa;
			GrGetWindowInfo(w, &info);
			GrSetGCForeground(gc, BGCOLOR);
			GrFillRect(w, gc, 0, 0, info.width, info.height);
			GrSetGCForeground(gc, FGCOLOR);
			do_paint(&event.exposure);	/*FIXME*/
			}
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
		}
	}
}
