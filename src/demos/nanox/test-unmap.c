#define MWINCLUDECOLORS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"
/*
 * Demo program showing workaround for unmap/map without event wait
 */
int
main(int ac,char **av)
{
	GR_WINDOW_ID 	w;
	GR_GC_ID	gc;
	GR_FONT_ID	font;
	GR_WINDOW_INFO wi;

	if (GrOpen() < 0) {
		GrError("Can't open graphics\n");
		return 1;
	}

	w = GrNewWindowEx(GR_WM_PROPS_APPWINDOW|GR_WM_PROPS_NOBACKGROUND, "Nano-X Demo2",
		GR_ROOT_WINDOW_ID, 20, 20, 320, 240, BLACK);

	gc = GrNewGC();
	font = GrCreateFontEx("helvB12.pcf.gz", 0, 0, NULL);
	GrSetGCFont(gc, font);

	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP);
	GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ
		| GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP);
	GrMapWindow(w);
	GrSetFocus(w);

	// pass errors through main loop
	GrSetErrorHandler(NULL);

	for (;;) {
		GR_EVENT 	event;

		GrGetNextEvent(&event);
		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			GrSetGCForeground(gc,GrGetSysColor(GR_COLOR_APPWINDOW));
			GrFillRect(w, gc, event.exposure.x, event.exposure.y,
				event.exposure.width, event.exposure.height);
			GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPTEXT));
			GrSetGCUseBackground(gc, GR_FALSE);
			GrText(w, gc, 10, 30, "Hello World", -1, GR_TFASCII);
            GrRect(w, gc, 5, 5, 300, 60);
			GrGetWindowInfo(w, &wi);
			break;

		case GR_EVENT_TYPE_BUTTON_DOWN:
			GrUnmapWindow(w);

			/* GrCheckNextEvent is REQUIRED to have nanowm inspect previous unmap
			 * before next map request, or server bug and no map results.
			 */
			GrCheckNextEvent(&event);
printf("possible loss of event type %d\n", event.type);
			GrMapWindow(w);
			break;
		case GR_EVENT_TYPE_BUTTON_UP:
			//GrMapWindow(w);
			break;

		case GR_EVENT_TYPE_ERROR:
			GrError("test-unmap: Error (%s) ", event.error.name);
			GrError(nxErrorStrings[event.error.code], event.error.id);
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			return 0;
		}
	}
	return 0;
}
