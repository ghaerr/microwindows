#include "nxlib.h"

/* Synchronize with errors and events, optionally discarding pending events */
int
XSync(Display *dpy, Bool discard)
{
	GrFlush();

	if (discard) {
		int count = GrQueueLength();
		GR_EVENT ev;

		while (--count >= 0)
			GrCheckNextEvent(&ev);
	}
	return 1;
}


int
XNoOp(Display *dpy) 
{
	return 1;
}
