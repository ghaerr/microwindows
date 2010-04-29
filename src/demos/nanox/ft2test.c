#include <stdio.h>
#include <string.h>
#include "nano-X.h"
#include "nxcolors.h"

//#define FONT          "DejaVuSans-Bold.ttf"
#define FONT		"arial.ttf"

GR_GC_ID gc;

void
draw_ellipse(int x, int y, int color, char *text)
{
	GrSetGCForeground(gc, color);
	GrSetGCBackground(gc, GR_COLOR_BLACK);

	GrFillEllipse(GR_ROOT_WINDOW_ID, gc, x, y, 170, 50);

	GrSetGCForeground(gc, ~color);
	GrSetGCUseBackground(gc, 0);
	GrText(GR_ROOT_WINDOW_ID, gc, x, y, text, strlen(text), 0);
}

int
main(void)
{
	GR_EVENT event;
	int quit = 0;
	int id;

	if (GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	gc = GrNewGC();

	id = GrCreateFont(FONT, 40, NULL);
	GrSetFontAttr(id, GR_TFANTIALIAS, 0);
	GrSetGCFont(gc, id);

	GrClearWindow(GR_ROOT_WINDOW_ID, 0);

	draw_ellipse(100, 50, GR_ARGB(100, 255, 0, 0), "red");
	draw_ellipse(100, 110, GR_ARGB(100, 0, 255, 0), "green");
	draw_ellipse(100, 170, GR_ARGB(100, 0, 0, 255), "blue");
	draw_ellipse(100, 230, GR_ARGB(100, 0, 0, 0), "black");
	draw_ellipse(100, 290, GR_ARGB(100, 255, 255, 255), "white");

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
