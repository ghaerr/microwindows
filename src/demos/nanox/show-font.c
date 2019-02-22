/*
 * pcfdemo - demonstrate PCF font loading/display for Nano-X
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

GR_FONT_ID font = 0;
GR_FONT_INFO finfo;

unsigned int first_char = 0;
unsigned int lines = 8;
unsigned int line_width = 32;
unsigned int chars_to_show;
unsigned int border = 10;
unsigned int spacer = 2;

static void
draw_string(GR_WINDOW_ID wid)
{
	int count = 0;
	int x = border;
	int y = border;
	unsigned int start, end;
	unsigned int ch;
	GR_GC_ID gc = GrNewGC();

	GrSetGCBackground(gc, GR_RGB(0, 0, 0));
	GrSetGCFont(gc, font);

	if (first_char > finfo.lastchar)
		first_char = 0;
	if (first_char + chars_to_show <= finfo.firstchar)
		first_char = (finfo.firstchar / line_width) * line_width;

	start = first_char;
	end = first_char + chars_to_show;

	GrError("drawing chars %d to %d\n", start, end - 1);

	for (ch = start; ch < end; ch++) {
		GrSetGCForeground(gc, GR_RGB(64, 64, 64));
		GrFillRect(wid, gc, x-1, y-1, finfo.maxwidth+2, finfo.height+2);
		GrSetGCForeground(gc, GR_RGB(255, 255, 255));

		if (ch >= finfo.firstchar && ch <= finfo.lastchar)
			GrText(wid, gc, x, y, &ch, 1, GR_TFTOP | GR_TFUC32);

		if (++count >= line_width) {
			x = border;
			y += finfo.height + spacer;
			count = 0;
		} else
			x += finfo.maxwidth + spacer;
	}

	GrDestroyGC(gc);
}

int
main(int argc, char **argv)
{
	GR_WINDOW_ID main_wid;
	GR_TIMEOUT timeout;
	int width, height;

	if (argc != 2)
		return 1;

	if (GrOpen() < 0)
		return 1;

	font = GrCreateFontEx(argv[1], 12, 12, 0);
	if (!font)
		GrError("Unable to load %s\n", argv[1]);

	GrGetFontInfo(font, &finfo);

	GrError("font_id = %d\n", font);
	GrError("First char = %d, last char = %d\n", finfo.firstchar, finfo.lastchar);
	GrError("Max width = %d, max height = %d\n", finfo.maxwidth, finfo.height);
	GrError("baseline = ascent = %d, descent = %d\n", finfo.baseline, finfo.descent);
	GrError("max ascent = %d, max descent = %d\n", finfo.maxascent, finfo.maxdescent);
	GrError("linespacing = %d, fixed = %s\n", finfo.linespacing, finfo.fixed? "yes": "no");

//	finfo.firstchar = 0;	/* force display of undefined chars, test with jiskan24.pcf.gz*/

	/* determine window metrics*/
	width = (finfo.maxwidth + spacer) * line_width + 2 * border - spacer;
	if (width > 640) {
		line_width /= 2;
		lines *= 2;
		width = (finfo.maxwidth + 2) * line_width + 2 * border - spacer;
    }
	height = lines * (finfo.height + spacer) + 2 * border - spacer;
	chars_to_show = lines * line_width;

	/* create the main application window*/
	main_wid = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, argv[1],
			GR_ROOT_WINDOW_ID, 0, 0, width, height, BLACK);
	GrSelectEvents(main_wid, GR_EVENT_MASK_EXPOSURE|GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(main_wid);

	if (finfo.lastchar >= chars_to_show)
		timeout = 8 * 1000;
    else timeout = 0;

	while (1) {
		GR_EVENT event;
		GrGetNextEventTimeout(&event, timeout);

		if (event.type == GR_EVENT_TYPE_TIMEOUT) {
			first_char += chars_to_show;
            draw_string(main_wid);
		}
		if (event.type == GR_EVENT_TYPE_EXPOSURE)
			draw_string(main_wid);
		if(event.type == GR_EVENT_TYPE_CLOSE_REQ) {
			GrClose();
			return 0;
	   }
	}
}
