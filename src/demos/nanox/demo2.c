#define MWINCLUDECOLORS
#include <stdio.h>
#include "nano-X.h"

#include <signal.h>

int
main(int ac,char **av)
{
	GR_WINDOW_ID 	w, w2;
	GR_GC_ID	gc;
	GR_EVENT 	event;
	GR_WM_PROPERTIES props;

	if (GrOpen() < 0) {
		printf("Can't open graphics\n");
		exit(1);
	}

	/* pass errors through main loop*/
	GrSetErrorHandler(NULL);

#define WIDTH	320
#define HEIGHT	240
	w = GrNewWindow(GR_ROOT_WINDOW_ID, 20, 20, WIDTH, HEIGHT,
		0, GREEN, BLACK);

	w2 = GrNewWindow(w, 20, 20, 40, 40, 0, WHITE, BLACK);

	props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOBACKGROUND;
	props.title = "Nano-X Demo2";
	GrSetWMProperties(w, &props);

	gc = GrNewGC();

	GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ
		| GR_EVENT_MASK_BUTTON_DOWN
		| GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP);
	GrMapWindow(w);
	GrSetFocus(w);
	/* serious bug here: when wm running, w2 is mapped anyway!!*/
	/*GrMapWindow(w2);*/

	for (;;) {
		/*GR_EVENT_KEYSTROKE *kev;*/

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
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
			break;
		case GR_EVENT_TYPE_ERROR:
			printf("\7demo2: Error (%s) ", event.error.name);
			printf(nxErrorStrings[event.error.code],event.error.id);
			break;
#if 0
		case GR_EVENT_TYPE_BUTTON_DOWN:
			/* test server error on bad syscall*/
			GrMapWindow(w2);
			GrMoveWindow(GR_ROOT_WINDOW_ID, 0, 0);
			{ GR_SCREEN_INFO sinfo; GrGetScreenInfo(&sinfo); }
			break;
#endif
#if 0
		case GR_EVENT_TYPE_KEY_DOWN:
			kev = (GR_EVENT_KEYSTROKE *)&event;
			printf("DOWN %d (%04x) %04x\n",
				kev->ch, kev->ch, kev->modifiers);
			break;
		case GR_EVENT_TYPE_KEY_UP:
			kev = (GR_EVENT_KEYSTROKE *)&event;
			printf("UP %d (%04x) %04x\n",
				kev->ch, kev->ch, kev->modifiers);
			break;
#endif
		}
	}

	GrClose();
	return 0;
}
