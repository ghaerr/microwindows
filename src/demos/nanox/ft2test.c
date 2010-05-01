#include <stdio.h>
#include <string.h>
#include "nano-X.h"
#include "nxcolors.h"

//#define FONT	"DejaVuSans-Bold.ttf"
#define FONT	"arial.ttf"

#define IMAGE	 "bin/tux.gif"

void draw_ellipse(GR_GC_ID gc, int x, int y, int color, char *text);
void test_ft2_antialias(void);
void test_stretchblit(void);

void
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
test_ft2_antialias(void)
{
	GR_GC_ID gc = GrNewGC();
	int id = GrCreateFont((GR_CHAR *)FONT, 40, NULL);

	GrSetFontAttr(id, GR_TFANTIALIAS, 0);
	GrSetGCFont(gc, id);

	GrClearWindow(GR_ROOT_WINDOW_ID, 0);

	draw_ellipse(gc, 100, 50, GR_ARGB(100, 255, 0, 0), "red");
	draw_ellipse(gc, 100, 110, GR_ARGB(100, 0, 255, 0), "green");
	draw_ellipse(gc, 100, 170, GR_ARGB(100, 0, 0, 255), "blue");
	draw_ellipse(gc, 100, 230, GR_ARGB(100, 0, 0, 0), "black");
	draw_ellipse(gc, 100, 290, GR_ARGB(100, 255, 255, 255), "white");

}

void
test_stretchblit(void)
{
	GR_GC_ID gc;
	GR_IMAGE_ID iid;
	GR_WINDOW_ID wid = GR_ROOT_WINDOW_ID;
	GR_WINDOW_ID pid;
	GR_IMAGE_INFO image_info;
	GR_SCREEN_INFO sinfo;
	int x, y;

	GrGetScreenInfo(&sinfo);

	if(!(iid = GrLoadImageFromFile(IMAGE, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", IMAGE);
		return;
	}
	GrGetImageInfo(iid, &image_info);
	pid = GrNewPixmap(image_info.width, image_info.height, NULL);
	gc = GrNewGC();
	GrDrawImageToFit(pid, gc, 0, 0, image_info.width, image_info.height, iid);
	GrDestroyGC(gc);
	GrFreeImage(iid);

	x = sinfo.cols - image_info.width*4;
	y = sinfo.rows - image_info.height*4;
	wid = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, NULL,
		GR_ROOT_WINDOW_ID, x, y, image_info.width*3, image_info.height*3, GR_COLOR_GREEN);

	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ |
			GR_EVENT_MASK_MOUSE_POSITION |
			GR_EVENT_MASK_BUTTON_UP |
			GR_EVENT_MASK_BUTTON_DOWN);

	GrSetBackgroundPixmap(wid, pid, GR_BACKGROUND_STRETCH);
	GrMapWindow(wid);
}

int
main(void)
{
	GR_EVENT event;
	int quit = 0;

	if (GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	test_ft2_antialias();
	//test_stretchblit();

	while (!quit) {
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
			break;
		case GR_EVENT_TYPE_TIMER:
			break;
		default:
			break;
		}
	}

	GrClose();

	return 0;
}
