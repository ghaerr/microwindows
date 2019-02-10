//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"
#include "X11/Xutil.h"
#include "X11/Xatom.h"
#include <stdlib.h>

// from GetTxtProp.c
// required for kaffeine
Status XGetTextProperty(Display *display, Window window, XTextProperty *tp, Atom property)
{
	Atom actual_type;
	int actual_format = 0;
	unsigned long nitems = 0L, leftover = 0L;
	unsigned char *prop = NULL;

	DPRINTF("XGetTextProperty called..\n");
	if (XGetWindowProperty(display, window, property, 0L, 1000000L, False,
		AnyPropertyType, &actual_type, &actual_format, &nitems, &leftover,
		&prop) == Success && actual_type != None) {
		/* okay, fill it in */
		tp->value = prop;
		tp->encoding = actual_type;
		tp->format = actual_format;
		tp->nitems = nitems;
		return True;
	}

	tp->value = NULL;
	tp->encoding = None;
	tp->format = 0;
	tp->nitems = 0;
	return False;
}

Status XGetWMName(Display *dpy, Window w, XTextProperty *tp)
{
	return XGetTextProperty(dpy, w, tp, XA_WM_NAME);
}

Status XGetWMIconName(Display *dpy, Window w, XTextProperty *tp)
{
	return XGetTextProperty(dpy, w, tp, XA_WM_ICON_NAME);
}

Status XGetWMClientMachine(Display *dpy, Window w, XTextProperty *tp)
{
	return XGetTextProperty(dpy, w, tp, XA_WM_CLIENT_MACHINE);
}

#define NumPropWMHintsElements 9 /* number of elements in this structure */
XWMHints *XGetWMHints(Display *dpy, Window w)
{
	XWMHints *prop;
	//XWMHints *hints;
	Atom actual_type;
	int actual_format;
	unsigned long leftover;
	unsigned long nitems;

	DPRINTF("XGetWMHints called..\n");
	if (XGetWindowProperty(dpy, w, XA_WM_HINTS, 0L,
		NumPropWMHintsElements, False, XA_WM_HINTS, &actual_type,
		&actual_format, &nitems, &leftover, (unsigned char **)&prop)
		!= Success) return NULL;

	/* If the property is undefined on the window, return null pointer. */
	/* pre-R3 bogusly truncated window_group, don't fail on them */
	if ((actual_type != XA_WM_HINTS) ||
		(nitems < (NumPropWMHintsElements-1)) || (actual_format != 32)) {
		if (prop) Xfree((char*)prop);
		return NULL;
	}
	return prop;
#if 0
	/* static copies not allowed in library, due to reentrancy constraint*/
	if ((hints = (XWMHints*)Xcalloc(1, (unsigned)sizeof(XWMHints)))) {
		hints->flags = prop->flags;
		hints->input = (prop->input ? True : False);
		hints->initial_state = cvtINT32toInt(prop->initialState);
		hints->icon_pixmap = prop->iconPixmap;
		hints->icon_window = prop->iconWindow;
		hints->icon_x = cvtINT32toInt(prop->iconX);
		hints->icon_y = cvtINT32toInt(prop->iconY);
		hints->icon_mask = prop->iconMask;
		if (nitems >= NumPropWMHintsElements)
			hints->window_group = prop->windowGroup;
		else
			hints->window_group = 0;
	}
	Xfree((char *)prop);
	return hints;
#endif
}

Status XGetCommand(Display *dpy, Window w, char ***argvp, int *argcp)
{
	XTextProperty tp;
	int argc;
	char **argv;

	DPRINTF("XGetCommand called..\n");
	if (!XGetTextProperty(dpy, w, &tp, XA_WM_COMMAND)) return 0;

	if (tp.encoding != XA_STRING || tp.format != 8) {
		if (tp.value) Xfree((char*) tp.value);
		return 0;
	}

	// ignore final <NUL> if present since UNIX WM_COMMAND is nul-terminated
	if (tp.nitems && (tp.value[tp.nitems - 1] == '\0')) tp.nitems--;

	// create a string list and return if successful
	if (!XTextPropertyToStringList(&tp, &argv, &argc)) {
		if (tp.value) Xfree((char*) tp.value);
		return 0;
	}

	if (tp.value) Xfree((char*) tp.value);
	*argvp = argv;
	*argcp = argc;
	return 1;
}
