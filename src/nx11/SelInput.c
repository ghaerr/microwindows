#include "nxlib.h"
#include <stdio.h>

static struct {
	unsigned long xevent;
	GR_EVENT_MASK mwevent;
} events[] = {
	{ KeyPressMask, GR_EVENT_MASK_KEY_DOWN },
	{ KeyReleaseMask, GR_EVENT_MASK_KEY_UP },
	{ ButtonPressMask, GR_EVENT_MASK_BUTTON_DOWN },
	{ ButtonReleaseMask, GR_EVENT_MASK_BUTTON_UP },
	{ EnterWindowMask, GR_EVENT_MASK_MOUSE_ENTER },
	{ LeaveWindowMask, GR_EVENT_MASK_MOUSE_EXIT },
	{ PointerMotionMask, GR_EVENT_MASK_MOUSE_MOTION },
	{ ExposureMask, GR_EVENT_MASK_EXPOSURE },
	{ StructureNotifyMask, (GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_CLOSE_REQ) },
	{ SubstructureNotifyMask, GR_EVENT_MASK_CHLD_UPDATE },
	{ ButtonMotionMask, GR_EVENT_MASK_MOUSE_MOTION },
	{ FocusChangeMask, (GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT) },
	{ 0, 0 }
};

GR_EVENT_MASK
_nxTranslateEventMask(unsigned long mask)
{
	unsigned long notmask = mask;
	GR_EVENT_MASK nxmask = 0L;
	int i;

	for (i = 0; events[i].xevent != 0; i++) {
		if (mask & events[i].xevent) {
			nxmask |= events[i].mwevent;
			notmask &= ~events[i].xevent;
		}
	}

	if (notmask) {
		DPRINTF("nxTranslateEventMask no handler for event mask (%08lx): ", mask);
		for (i = 0; i < 31; i++) {
			if (notmask & (1L << i))
				DPRINTF("%d ", i);
		}
		DPRINTF("\n");
	}

	return nxmask;
}

int
XSelectInput(Display * dpy, Window w, long mask)
{
	GR_EVENT_MASK nxmask;

	nxmask = _nxTranslateEventMask(mask);
	GrSelectEvents(w, nxmask);
	return 1;
}
