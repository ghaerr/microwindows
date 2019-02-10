#include "nxlib.h"
#include <string.h>
#include <stdlib.h>
#include "X11/Xutil.h"
#include "X11/Xatom.h"

void XSetTextProperty(Display *dpy, Window w, XTextProperty *tp, Atom property)
{
	DPRINTF("XSetTextProperty called [%s]\n", tp?tp->value:(NULL));
	XChangeProperty(dpy, w, property, tp->encoding, tp->format,
		PropModeReplace, tp->value, tp->nitems);
}

void XSetWMClientMachine(Display *dpy, Window w, XTextProperty *tp)
{
	XSetTextProperty(dpy, w, tp, XA_WM_CLIENT_MACHINE);
}

// ClassHint.c
/*int XSetTransientForHint(Display* display, Window w, Window prop_window)
{
	XChangeProperty(display, w, XA_WM_TRANSIENT_FOR, XA_WINDOW, 32,
		PropModeReplace, (unsigned char *)&prop_window, 1);
	return 1;
}*/

/*void XSetWMName(Display *dpy, Window w, XTextProperty *tp)
{
	XSetTextProperty(dpy, w, tp, XA_WM_NAME);
}*/
/*void XSetWMName(Display *display, Window w, XTextProperty *name)
{
	GR_WM_PROPERTIES props;

	if (!name || !name->value) return;

	props.flags = GR_WM_FLAGS_TITLE;
	props.title = name->value;
	GrSetWMProperties(w, &props);

	XSetTextProperty(display, w, name, XA_WM_NAME);	// Add
}*/
void
XSetWMName(Display * display, Window w, XTextProperty * name)
{
	XChangeProperty(display, w, XA_WM_NAME, XA_STRING, 8,
		PropModeReplace, (unsigned char *)name->value, name->nitems);
}

void XSetWMIconName(Display *dpy, Window w, XTextProperty *tp)
{
	XSetTextProperty(dpy, w, tp, XA_WM_ICON_NAME);
}

#define NumPropSizeElements 18		/* ICCCM version 1 */
void XSetWMSizeHints(Display *dpy, Window w, XSizeHints *hints, Atom prop)
{
	DPRINTF("XSetWMSizeHints called..\n");
	XChangeProperty(dpy, w, prop, XA_WM_SIZE_HINTS, 32, PropModeReplace,
		(unsigned char*)hints, NumPropSizeElements);
}

void XSetWMNormalHints(Display *dpy, Window w, XSizeHints *hints)
{
	//DPRINTF("XSetWMNormalHints called\n");
	XSetWMSizeHints(dpy, w, hints, XA_WM_NORMAL_HINTS);
}

#define NumPropWMHintsElements 9 /* number of elements in this structure */
int XSetWMHints(Display *dpy, Window w, XWMHints *hints)
{
	DPRINTF("XSetWMHints called..\n");
	//return 1;
	return XChangeProperty(dpy, w, XA_WM_HINTS, XA_WM_HINTS, 32,
		PropModeReplace, (unsigned char*)hints, NumPropWMHintsElements);
}

/* Not used */
void
XSetWMClassHints(Display * display, Window w, XClassHint * hints)
{
	DPRINTF("XSetWMClassHints called\n");
}

XSizeHints *
XAllocSizeHints(void)
{
	return (XSizeHints *)Xcalloc(1, (unsigned)sizeof(XSizeHints));
}

XWMHints *
XAllocWMHints(void)
{
	return (XWMHints *)Xcalloc(1, (unsigned)sizeof(XWMHints));
}

void
XSetWMProperties(Display * display, Window w, XTextProperty * window_name,
	XTextProperty * icon_name, char **argv, int argc,
	XSizeHints * normal_hints, XWMHints * wm_hints, XClassHint *class_hints)
{
	if (window_name)
		XSetWMName(display, w, window_name);
	if (icon_name)
		XSetWMIconName(display, w, icon_name);
	if (normal_hints)
		XSetWMNormalHints(display, w, normal_hints);
	if (wm_hints)
		XSetWMHints(display, w, wm_hints);
	if (class_hints)
		XSetWMClassHints(display, w, class_hints);
}

void
XmbSetWMProperties(Display *dpy, Window w, _Xconst char *windowName,
	_Xconst char *iconName, char **argv, int argc, XSizeHints * sizeHints,
	XWMHints * wmHints, XClassHint * classHints)
{
	XTextProperty wname, iname;
	XTextProperty *wprop = NULL;
	XTextProperty *iprop = NULL;

	/* fake up XTextProperty struct members for XSetWMName*/
	if (windowName) {
		wname.value = (unsigned char *)windowName;
		wname.encoding = XA_STRING;
		wname.format = 8;
		wname.nitems = strlen(windowName);
		wprop = &wname;
	}
	if (iconName) {
		iname.value = (unsigned char *)iconName;
		iname.encoding = XA_STRING;
		iname.format = 8;
		iname.nitems = strlen(iconName);
		iprop = &iname;
	}
	XSetWMProperties(dpy, w, wprop, iprop, argv, argc, sizeHints, wmHints,
		classHints);
}

void
Xutf8SetWMProperties(Display *dpy, Window w, _Xconst char *windowName,
	_Xconst char *iconName, char **argv, int argc, XSizeHints * sizeHints,
	XWMHints * wmHints, XClassHint * classHints)
{
	XmbSetWMProperties(dpy, w, windowName, iconName, argv, argc,
		sizeHints, wmHints, classHints);
}
