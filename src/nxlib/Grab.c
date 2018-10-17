//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"

// required for Xt
int XGrabButton(Display *display, unsigned int button, unsigned int modifiers,
	Window grab_window, Bool owner_events, unsigned int event_mask,
	int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor)
{
	DPRINTF("XGrabButton called...\n");
	return 0;
}

// required for rasp
int XGrabKey(Display *dpy, int key, unsigned int modifiers, Window grab_window,
	Bool owner_events, int pointer_mode, int keyboard_mode)

{
	DPRINTF("XGrabKey called...\n");
	return 0;
}

/*int XGrabKeyboard(Display *display, Window w, Bool owner_events,
	int pointer_mode, int keyboard_mode, Time time)
{
	DPRINTF("XGrabKeyboard called...\n");
	return 0;
}*/
