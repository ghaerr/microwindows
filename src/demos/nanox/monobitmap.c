#include <stdio.h>
#include <string.h>
#include "nano-X.h"
#include "nxcolors.h"
/*
 * 1bpp mono bitmap tests
 *
 * 1/6/2011 ghaerr
 */

#define FONT	"courz.ttf"				/* tests MWIF_MONOBYTEMSB drawing*/
//#define FONT "fonts/type1/bchb.pfb"		/* tests MWIF_MONOBYTELSB drawing*/
//#define FONT "helvB12.pcf.gz"			/* tests MWIF_MONOWORDMSB drawing*/
//#define FONT "fonts/fnt/timBI18.fnt"
//#define FONT "System"

GR_WINDOW_ID wid;

void on_init(void);
void on_paint(void);

void
on_init(void)
{
	int flags;

	flags = GR_WM_PROPS_APPWINDOW | GR_WM_PROPS_NOBACKGROUND;
	wid = GrNewWindowEx(flags, "Microwindows Mono Bitmap Demo", GR_ROOT_WINDOW_ID,
		20, 20, 320, 240, GR_COLOR_WHITE);

	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_EXPOSURE);
}

/* draw pattern on mono pixmap*/
GR_WINDOW_ID
getpixmap(void)
{
	GR_GC_ID 		gc = GrNewGC();
	GR_WINDOW_ID	pixmap;
	int				x, y;

	// create 1bpp pixmap, bytemsb allows ft2 text and is compatible with std fblin1.c driver 
	pixmap = GrNewPixmapEx(320, 240, MWIF_MONOBYTEMSB, NULL);

	// first third set all 1 bits (pixel set to bit 0 of 0xAABBGGRR MWCOLORVAL)
	GrSetGCForeground(gc, GR_COLOR_WHITE);
	GrFillRect(pixmap, gc, 0, 0, 320, 80);

	// draw dithered pattern in second third
	for (y = 80; y < 160; y++)
		for (x = y&1; x < 320; x += 2)
				GrPoint(pixmap, gc, x, y);
	// default black in third portion

	GrDestroyGC(gc);

	return pixmap;
}

void
on_paint(void)
{
	GR_GC_ID gc = GrNewGC();
	GR_FONT_ID font;
	GR_WINDOW_ID pixmap, xormap;
	char *text = "Microwindows";

	pixmap = getpixmap();

	// black square at top left
	GrSetGCForeground(gc, GR_COLOR_BLACK);
	GrFillRect(pixmap, gc, 4, 4, 50, 130);

	// draw text
	font = GrCreateFontEx(FONT, 90, 90, NULL);
	GrSetFontAttr(font, GR_TFANTIALIAS|GR_TFKERNING, 0);
	GrSetFontRotation(font, 450);
	GrSetGCFont(gc, font);
	GrSetGCForeground(gc, GR_COLOR_BLACK);
	GrSetGCUseBackground(gc, GR_FALSE);
	GrText(pixmap, gc, 40,100, text, strlen(text), MWTF_ASCII|MWTF_BOTTOM);
	GrDestroyFont(font);

	// create 1bpp pixmap for xor image
	xormap = GrNewPixmapEx(320, 240, MWIF_MONOBYTEMSB, NULL);
	// set bottom left quarter to white
	GrSetGCForeground(gc, GR_COLOR_WHITE);
	GrFillRect(xormap, gc, 0, 120, 160, 120);

	// xor entire image
	//GrCopyArea(pixmap, gc, 0, 0, 320, 240, xormap, 0, 0, MWROP_ANDREVERSE);

	// test non dword alignment masks for correctness - check 8 pixels from left
	GrCopyArea(pixmap, gc, 8, 8, 312, 232, xormap, 8, 8, MWROP_XOR);

	// check leftmost pixel
	GrCopyArea(pixmap, gc, 0, 0, 1, 239, xormap, 0, 0, MWROP_XOR);
	GrDestroyWindow(xormap);

	// copy pixmap to truecolor display mapping 0 to black and 1 to white
	GrSetGCForeground(gc, GR_COLOR_WHITE);
	GrSetGCBackground(gc, GR_COLOR_BLACK);
	GrSetGCUseBackground(gc, TRUE);
	GrCopyArea(wid, gc, 0, 0, 320, 240, pixmap, 0, 0, MWROP_COPY);
	GrDestroyWindow(pixmap);
	GrDestroyGC(gc);
}

int
main(int argc, char **argv)
{
	int quit = 0;

	if (GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	on_init();				/* create window*/
	GrMapWindow(wid);

	while (!quit) {
		GR_EVENT event;
		GrGetNextEvent(&event);

		switch (event.type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			quit = 1;
			break;

		case GR_EVENT_TYPE_UPDATE:
			switch (event.update.utype) {
			//case GR_UPDATE_MAP:		/* initial paint*/
			case GR_UPDATE_SIZE:		/* resize repaint*/
				//on_resize();
				on_paint();
			}
			break;

		case GR_EVENT_TYPE_EXPOSURE:
			on_paint();
			break;
		}
	}

	GrClose();
	return 0;
}
