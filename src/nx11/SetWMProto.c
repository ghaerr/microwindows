//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"
#include "X11/Xatom.h"
#include <stdlib.h>

// XSetWMProtocols sets the property
//	WM_PROTOCOLS 	type: ATOM	format: 32
Status XSetWMProtocols(Display *dpy, Window w, Atom *protocols, int count)
{
	Atom prop;

	prop = XInternAtom(dpy, "WM_PROTOCOLS", False);
	if (prop == None) return False;

	XChangeProperty(dpy, w, prop, XA_ATOM, 32,
			PropModeReplace, (unsigned char *)protocols, count);
	return True;
}

Status XGetWMProtocols(Display *dpy, Window w, Atom **protocols, int *countReturn)
{
	Atom *data = NULL;
	Atom actual_type;
	Atom prop;
	int actual_format;
	unsigned long leftover, nitems;

	prop =  XInternAtom(dpy, "WM_PROTOCOLS", False);
	if (prop == None) return False;

	/* get the property */
	if (XGetWindowProperty(dpy, w, prop, 0L, 1000000L, False,
		XA_ATOM, &actual_type, &actual_format,
		&nitems, &leftover, (unsigned char **)&data)
		!= Success)
	return False;

	if (actual_type != XA_ATOM || actual_format != 32) {
		if (data) Xfree((char*)data);
		return False;
	}

	*protocols = (Atom*)data;
	*countReturn = (int)nitems;
	return True;
}
