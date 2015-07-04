#include <stdio.h>
#include <string.h>
#include "nano-X.h"
#include "nxcolors.h"

#define TEST_COPYAREA		1
#define TEST_XOR_VHLINE		0

//#define FONT	"DejaVuSans-Bold.ttf"
#define FONT	"arial.ttf"


//#define IMAGE	 "bin/tux.gif"
#define IMAGE	 "mwin/bmp/alphademo.png"
//#define IMAGE	 "mwin/bmp/earth.jpg"
//#define IMAGE	 "mwin/bmp/bigapple.gif"

GR_IMAGE_INFO image_info;
GR_WINDOW_ID pixmap;

static void draw_ellipse(GR_GC_ID gc, int x, int y, int color, char *text);
void redraw(void);
void init_stretchblit(void);

static void
draw_ellipse(GR_GC_ID gc, int x, int y, int color, char *text)
{
	GrSetGCForeground(gc, color);
	GrSetGCBackground(gc, GR_COLOR_BLACK);

	GrFillEllipse(GR_ROOT_WINDOW_ID, gc, x, y, 170, 50);

	GrSetGCForeground(gc, ~color);
	GrSetGCUseBackground(gc, 0);
	GrText(GR_ROOT_WINDOW_ID, gc, x, y, text, strlen(text), 0);
}

void
redraw(void)
{
	GR_GC_ID gc;
	int id;
	
	/* draw antialias font in filled ellipses*/
	gc = GrNewGC();
	id = GrCreateFontEx(FONT, 40, 40, NULL);

	GrSetFontAttr(id, GR_TFANTIALIAS, 0);
	GrSetGCFont(gc, id);
	//GrClearWindow(GR_ROOT_WINDOW_ID, 0);
	draw_ellipse(gc, 100, 50, GR_ARGB(100, 255, 0, 0), "red");
	draw_ellipse(gc, 100, 110, GR_ARGB(100, 0, 255, 0), "green");
	draw_ellipse(gc, 100, 170, GR_ARGB(100, 0, 0, 255), "blue");
	draw_ellipse(gc, 100, 230, GR_ARGB(100, 0, 0, 0), "black");
	draw_ellipse(gc, 100, 290, GR_ARGB(100, 255, 255, 255), "white");
	GrDestroyFont(id);

	/* copy image onto root window background using SRC_OVER*/
	GrCopyArea(GR_ROOT_WINDOW_ID, gc, 10, 10, image_info.width, image_info.height,
		pixmap, 0, 0, MWROP_SRC_OVER);

#if TEST_XOR_VHLINE
	{
	int y;
	/* test XOR vertical and horizontal line draw*/
	GrSetGCMode(gc, MWROP_XOR);
	GrSetGCForeground(gc, GR_COLOR_RED);
	for (y=10; y<20; y++)
		GrLine(GR_ROOT_WINDOW_ID, gc, 0, y, 638, y);
	}
#endif
	GrDestroyGC(gc);
}

void
init_stretchblit(void)
{
	GR_GC_ID gc;
	GR_IMAGE_ID iid;
	GR_WINDOW_ID wid;
	int x, y;
	GR_SCREEN_INFO sinfo;

	GrGetScreenInfo(&sinfo);

	/* load image from file and draw to offscreen pixmap*/
	if(!(iid = GrLoadImageFromFile(IMAGE, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", IMAGE);
		return;
	}
	GrGetImageInfo(iid, &image_info);
	pixmap = GrNewPixmapEx(image_info.width, image_info.height, MWIF_RGBA8888, NULL);
	gc = GrNewGC();
	GrDrawImageToFit(pixmap, gc, 0, 0, image_info.width, image_info.height, iid);
	GrDestroyGC(gc);
	GrFreeImage(iid);

	/* create window and set image as background pixmap*/
	x = sinfo.cols - image_info.width*4;
	y = sinfo.rows - image_info.height*4;
	wid = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "StretchBlit Demo - Drag corner to resize",
		GR_ROOT_WINDOW_ID, x, y, image_info.width/3, image_info.height/3, GR_COLOR_GREEN);
	GrSetBackgroundPixmap(wid, pixmap, GR_BACKGROUND_STRETCH);
	GrMapWindow(wid);

	/* activate close box*/
	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ);
}

int
main(int argc, char **argv)
{
	int quit = 0;

	if (GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	/* create window with background image*/
	init_stretchblit();

#if TEST_COPYAREA
	/* draw root background first time*/
	redraw();

	/* then select for redraws when exposed*/
	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE);
#endif

	while (!quit) {
		GR_EVENT event;
		GrGetNextEvent(&event);

		switch (event.type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			quit = 1;
			break;
		case GR_EVENT_TYPE_MOUSE_POSITION:
			break;
		case GR_EVENT_TYPE_BUTTON_UP:
		case GR_EVENT_TYPE_BUTTON_DOWN:
			break;
		case GR_EVENT_TYPE_EXPOSURE:
			redraw();
			break;
		case GR_EVENT_TYPE_TIMER:
			break;
		}
	}

	GrClose();
	return 0;
}
