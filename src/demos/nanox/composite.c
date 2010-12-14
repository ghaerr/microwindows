#include <stdio.h>
#include <string.h>
#include "nano-X.h"
#include "nxcolors.h"
/*
 * Compositing demo
 *
 * 12/11/2010 g haerr
 */

//#define FONT	"DejaVuSans-Bold.ttf"
#define FONT	"arial.ttf"
#define IMAGE	 "mwin/bmp/alphademo.png"

GR_WINDOW_ID wid, p1, p2;
GR_IMAGE_INFO image_info;

void init_demo(void);
void redraw_demo(void);

void
init_demo(void)
{
	GR_GC_ID gc;
	int x, y;
	GR_FONT_ID font;
	GR_IMAGE_ID iid;
	GR_SCREEN_INFO sinfo;
	GR_WM_PROPERTIES props;
	char *text = "Microwindows Compositing";

	GrGetScreenInfo(&sinfo);
	gc = GrNewGC();

	/* load image from file into RGBA format*/
	if(!(iid = GrLoadImageFromFile(IMAGE, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", IMAGE);
		return;
	}
	GrGetImageInfo(iid, &image_info);

	/* create 32bpp pixmap and draw image w/alpha on it*/
	p1 = GrNewPixmapEx(image_info.width, image_info.height, MWIF_RGBA8888, NULL);
	GrDrawImageToFit(p1, gc, 0, 0, image_info.width, image_info.height, iid);
	GrFreeImage(iid);

	/* create another pixmap p2 of same size and bpp with green background*/
	p2 = GrNewPixmapEx(image_info.width, image_info.height, MWIF_RGBA8888, NULL);
	GrSetGCForeground(gc, GR_COLOR_SEAGREEN);
	GrFillRect(p2, gc, 0, 0, image_info.width, image_info.height);

	/* draw text onto p2*/
	font = GrCreateFontEx(FONT, 80, 60, NULL);
	GrSetFontAttr(font, GR_TFANTIALIAS|GR_TFKERNING, 0);
	GrSetFontRotation(font, 450);
	GrSetGCFont(gc, font);
	GrSetGCForeground(gc, GR_COLOR_WHITE);
	GrSetGCUseBackground(gc, GR_FALSE);
	GrText(p2, gc, 40, 80, text, strlen(text), MWTF_ASCII);
	GrDestroyFont(font);

	/* composite p1 onto p2*/
	GrCopyArea(p2, gc, 0, 0, image_info.width, image_info.height, p1, 0, 0, MWROP_SRC_OVER);

	/* cleanup*/
	GrDestroyGC(gc);
	GrDestroyWindow(p1);

	/* create window for display with no background erase to stop blink*/
	x = sinfo.cols - image_info.width*4;
	y = sinfo.rows - image_info.height*4;
	wid = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, NULL, GR_ROOT_WINDOW_ID,
		x, y, image_info.width, image_info.height, GR_COLOR_WHITE);
	props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOBACKGROUND;
	props.title = "Microwindows Compositing Demo";
	GrSetWMProperties(wid, &props);


	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_EXPOSURE);
	GrMapWindow(wid);
}

void
redraw_demo(void)
{
	GR_GC_ID gc = GrNewGC();

	/* copy p2 onto display*/
	GrCopyArea(wid, gc, 0, 0, image_info.width, image_info.height, p2, 0, 0, MWROP_COPY);

	GrDestroyGC(gc);
}

int
main(void)
{
	int quit = 0;

	if (GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	/* create pixmap with background image*/
	init_demo();

	while (!quit) {
		GR_EVENT event;
		GrGetNextEvent(&event);

		switch (event.type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			quit = 1;
			break;

		case GR_EVENT_TYPE_EXPOSURE:
			redraw_demo();
			break;
		}
	}

	GrClose();
	return 0;
}
