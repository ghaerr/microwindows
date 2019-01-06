#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
/*
 * demo application to test multi-threading
 */

GR_WINDOW_ID wid;
GR_GC_ID gc;
GR_SCREEN_INFO si;

void init();
void handle_events(GR_EVENT * event);
void handle_graphics();

void
init()
{
	GrGetScreenInfo(&si);
	wid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, si.cols, si.rows, 1, GRAY,
			  WHITE);
	GrSelectEvents(wid, GR_EVENT_MASK_KEY_DOWN);
	GrMapWindow(wid);

	gc = GrNewGC();
	GrSetGCForeground(gc, RED);
}

void
handle_events(GR_EVENT * event)
{
	while (1) {
		printf("------------------------------------------------------\n");
		GrGetNextEvent(event);
		if (event != NULL) {
			switch (event->type) {
			case GR_EVENT_TYPE_KEY_DOWN:
				printf("KEY PRESSED \n");
				break;
			}
		}
		printf("------------------------------------------------------\n");
	}
}

void
handle_graphics()
{
	int i;

	printf("Inside handle_graphics\n");

	for (i = 1; i < 10; i++) {
		GrFillRect(wid, gc, 250 - i * 20, 230, 10, 50);
		GrFlush();
		sleep(1);
	}
}

int
main(int argc, char **argv)
{
	GR_EVENT event;
	pthread_t p_thread_events;
	pthread_t p_thread_graphics;

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}

	init();
	pthread_create(&p_thread_events, NULL, (void *) handle_events,
		       &event);
	pthread_create(&p_thread_graphics, NULL, (void *) handle_graphics,
		       NULL);
	sleep(100);
	GrClose();
}
