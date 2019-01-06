#include "nxlib.h"

int
XCloseDisplay (Display *dpy)
{
	Screen *sp = &dpy->screens[0];

	XFreeGC(dpy, sp->default_gc);

	if (_nxCursorFont != None) {
		XUnloadFont(dpy, _nxCursorFont);
		_nxCursorFont = None;
	}

	GrFlush();
	GrClose();
	_XFreeDisplayStructure(dpy);
	return 0;
}
