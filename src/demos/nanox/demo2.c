#define MWINCLUDECOLORS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"

int
main(int ac,char **av)
{
	GR_WINDOW_ID 	w;
	GR_GC_ID	gc;
	GR_FONT_ID	font;
	GR_WINDOW_INFO wi;

	if (GrOpen() < 0) {
		printf("Can't open graphics\n");
		exit(1);
	}

	//w = GrNewWindowEx(GR_WM_PROPS_APPWINDOW|GR_WM_PROPS_NOBACKGROUND, "Nano-X Demo2",
		//GR_ROOT_WINDOW_ID, 20, 20, 320, 240, BLACK);
	w = GrNewWindowEx(GR_WM_PROPS_BORDER|GR_WM_PROPS_NOBACKGROUND, "Nano-X Demo2",
		GR_ROOT_WINDOW_ID, 20, 20, 320, 240, BLACK);

	gc = GrNewGC();
	//font = GrCreateFontEx("lubI24.pcf", 0, 0, NULL);
	font = GrCreateFontEx("helvB12.pcf.gz", 0, 0, NULL);
	GrSetGCFont(gc, font);

	GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ
		| GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP);
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
			printf("Exposure:wi.width:%d,wi.height:%d,wi.x:%d,wi.y:%d,wi.parent:%d\n",wi.width,wi.height,wi.x,wi.y,wi.parent);
			break;

		case GR_EVENT_TYPE_BUTTON_DOWN:
			// FIXME unmap window is broken
			//GrUnmapWindow(w);
			//GrFlush();
			//GrMapWindow(w);

			//uncomment to test server error on bad syscall
			//GrMoveWindow(GR_ROOT_WINDOW_ID, 0, 0);

			GrGetWindowInfo(w, &wi);
			printf("Button:  wi.width:%d,wi.height:%d,wi.x:%d,wi.y:%d,wi.parent:%d\n",wi.width,wi.height,wi.x,wi.y,wi.parent);
			break;

		case GR_EVENT_TYPE_ERROR:
			printf("demo2: Error (%s) ", event.error.name);
			printf(nxErrorStrings[event.error.code], event.error.id);
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
		}
	}
	return 0;
}
