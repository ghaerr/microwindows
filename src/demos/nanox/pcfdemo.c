/*
 * pcfdemo - demonstrate PCF font loading for Nano-X
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

GR_FONT_ID font = 0;
GR_WINDOW_ID main_wid;
GR_FONT_INFO finfo;

static void
draw_string(void)
{
	int count = 0;
	int x = 0;
	int y = 10;
	unsigned char ch;
	GR_GC_ID gc = GrNewGC();

	GrSetGCFont(gc, font);

	GrSetGCForeground(gc, GR_RGB(255, 255, 255));
	GrSetGCBackground(gc, GR_RGB(0, 0, 0));

	printf("First char = %d, last char = %d\n", finfo.firstchar,
	       finfo.lastchar);
	printf("Max width = %d, max height = %d\n", finfo.maxwidth,
	       finfo.height);

	for (ch = 0; ch < 255; ch++) {
		if (ch < finfo.firstchar || ch > finfo.lastchar)
			GrFillRect(main_wid, gc, x, y, finfo.maxwidth,
				   finfo.height);
		else
			GrText(main_wid, gc, x, y, &ch, 1,
			       GR_TFTOP | GR_TFASCII);

		if (++count >= 16) {
			x = 0;
			y += finfo.height;
			count = 0;
		} else
			x += finfo.maxwidth + 2;
	}

	GrDestroyGC(gc);
}

int
main(int argc, char **argv)
{
	int width, height;

	if (argc < 2)
		return (-1);

	if (GrOpen() == -1)
		return (-1);

	font = GrCreateFont(argv[1], 12, 0);
	if (!font)
		printf("Unable to load %s\n", argv[1]);

	GrGetFontInfo(font, &finfo);

	width = ((finfo.maxwidth + 2) * 16);
	height =
		(((finfo.lastchar - finfo.firstchar) / 16) +
		 5) * finfo.height;

	main_wid = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "pcfdemo",
			GR_ROOT_WINDOW_ID, 0, 0, width, height, BLACK);
	GrSelectEvents(main_wid, GR_EVENT_MASK_EXPOSURE|GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(main_wid);

	while (1) {
		GR_EVENT event;
		GrGetNextEvent(&event);

		if (event.type == GR_EVENT_TYPE_EXPOSURE)
			draw_string();

	        if(event.type == GR_EVENT_TYPE_CLOSE_REQ) {
			GrClose();
			exit(0);
	      }
	}
}
