#include <stdio.h>
#include <string.h>
#include "nano-X.h"
#include "nxcolors.h"
/*
 * Compositing demo
 *
 * 12/11/2010 g haerr
 */

#define BUFFERED_WINDOWS		1
#define TEST_DRAWIMAGEPARTIAL	0		/* =1 to use GrDrawImagePartToFit*/

//#define FONT	"DejaVuSans-Bold.ttf"
#define FONT	"arial.ttf"
#define IMAGE	 "mwin/bmp/alphademo.png"

GR_WINDOW_ID wid = 0, p1, p2 = 0;
GR_IMAGE_INFO image_info;
GR_WINDOW_INFO winfo;

void resize_demo(void);
void init_demo(void);
void redraw_demo(void);

void
resize_demo(void)
{
	int width, height;
	GR_GC_ID gc;
	GR_FONT_ID font;
	GR_IMAGE_ID iid;
	char *text = "Microwindows Compositing";

	gc = GrNewGC();

	/* load image from file into RGBA format*/
	if(!(iid = GrLoadImageFromFile(IMAGE, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", IMAGE);
		return;
	}
	GrGetImageInfo(iid, &image_info);

	if (wid) {
		GrGetWindowInfo(wid, &winfo);
		width = winfo.width;
		height = winfo.height;
	} else {
		width = image_info.width;
		height = image_info.height;
	}
	/* create 32bpp pixmap and draw image w/alpha on it*/
	p1 = GrNewPixmapEx(width, height, MWIF_RGBA8888, NULL);
#if TEST_DRAWIMAGEPARTIAL
	/* this will expand the image into the same space using stretchblit*/
	GrDrawImagePartToFit(p1, gc, 0,0, width, height, 0,0, image_info.width/2, image_info.height/2, iid);
#else
	GrDrawImageToFit(p1, gc, 0, 0, width, height, iid);
#endif
	GrFreeImage(iid);

	if (p2)
		GrDestroyWindow(p2);
	/* create another pixmap p2 of same size and bpp with green background*/
	p2 = GrNewPixmapEx(width, height, MWIF_RGBA8888, NULL);
	GrSetGCForeground(gc, GR_COLOR_SEAGREEN);
	GrFillRect(p2, gc, 0, 0, width, height);

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
	GrCopyArea(p2, gc, 0, 0, width, height, p1, 0, 0, MWROP_SRC_OVER);

	/* cleanup*/
	GrDestroyWindow(p1);
}

void
init_demo(void)
{
	int flags;

	/* create window for display with no background erase to stop blink*/
	flags = GR_WM_PROPS_APPWINDOW | GR_WM_PROPS_NOBACKGROUND;
#if BUFFERED_WINDOWS
	flags |= GR_WM_PROPS_BUFFERED;
#endif
	wid = GrNewWindowEx(flags, "Microwindows Compositing Demo", GR_ROOT_WINDOW_ID,
		20, 20, image_info.width, image_info.height, GR_COLOR_WHITE);

	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_EXPOSURE);
}

void
redraw_demo(void)
{
	GR_GC_ID gc = GrNewGC();

	GrGetWindowInfo(wid, &winfo);
	/* copy p2 onto display*/
	GrCopyArea(wid, gc, 0, 0, winfo.width, winfo.height, p2, 0, 0, MWROP_COPY);
	GrDestroyGC(gc);
#if BUFFERED_WINDOWS
	GrFlushWindow(wid);
#endif
}

int
main(int argc, char **argv)
{
	int quit = 0;

	if (GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	/* create pixmap with background image*/
	resize_demo();				/* create contents*/
	init_demo();				/* create window*/
#if BUFFERED_WINDOWS
	redraw_demo();				/* prevent any initial blink*/
#endif
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
#if BUFFERED_WINDOWS
			case GR_UPDATE_MAP:			/* initial paint*/
#endif
			case GR_UPDATE_SIZE:		/* resize repaint*/
				resize_demo();
				redraw_demo();
			}
			break;

#if !BUFFERED_WINDOWS
		case GR_EVENT_TYPE_EXPOSURE:
			printf("Expose event id %d\n", event.exposure.wid);
			redraw_demo();				/* copy contents to window*/
			break;
#endif
		}
	}

	GrClose();
	return 0;
}
