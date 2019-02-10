//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"
#include "X11/Xutil.h"
#include "X11/Xatom.h"
#include <string.h>
#include <stdio.h>			/* for BUFSIZ*/
#include <stdlib.h>

XClassHint *XAllocClassHint(void)
{
	return (XClassHint *) Xcalloc(1, sizeof(XClassHint));
}

// required for xine
Status XGetClassHint(Display *dpy, Window w, XClassHint *classhint)
{
	int len_name, len_class;

	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long leftover;
	unsigned char *data = NULL;

	DPRINTF("XGetClassHint called...\n");
	if (XGetWindowProperty(dpy, w, XA_WM_CLASS, 0L, (long)BUFSIZ, False,
		XA_STRING, &actual_type, &actual_format,
			&nitems, &leftover, &data) != Success) return 0;

	if ( (actual_type == XA_STRING) && (actual_format == 8) ) {
		len_name = strlen((char *) data);
		if (! (classhint->res_name = Xmalloc((unsigned)(len_name+1)))) {
			Xfree((char *) data);
			return 0;
		}
		strcpy(classhint->res_name, (char *) data);
		if (len_name == nitems) len_name--;
		len_class = strlen((char *) (data+len_name+1));
		if (! (classhint->res_class = Xmalloc((unsigned)(len_class+1)))) {
			Xfree(classhint->res_name);
			classhint->res_name = (char *) NULL;
			Xfree((char *) data);
			return 0;
		}
		strcpy(classhint->res_class, (char *) (data+len_name+1));
		Xfree( (char *) data);
		return 1;
	}
	if (data) Xfree( (char *) data);
	return 0;
}

// required for rasp
Status XGetTransientForHint(Display *dpy, Window w, Window *propWindow)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long leftover;
	Window *data = NULL;

	DPRINTF("XGetTransientForHint called..\n");
	if (XGetWindowProperty(dpy, w, XA_WM_TRANSIENT_FOR, 0L, 1L, False,
		XA_WINDOW, &actual_type, &actual_format,
		&nitems, &leftover, (unsigned char **)&data) != Success) {
		*propWindow = None;
		return 0;
	}
	if ((actual_type==XA_WINDOW) && (actual_format==32) && (nitems != 0)) {
		*propWindow = *data;
		Xfree((char*)data);
		return 1;
	}

	*propWindow = None;
	if (data) Xfree((char*)data);
	return 0;
}

// required for xloadimage
#define safe_strlen(s) ((s) ? strlen(s) : 0)
int XSetClassHint(Display *dpy, Window w, XClassHint *classhint)
{
	char *class_string;
	char *s;
	int len_nm, len_cl;

	DPRINTF("XSetClassHint called..\n");
	len_nm = safe_strlen(classhint->res_name);
	len_cl = safe_strlen(classhint->res_class);
	if ((class_string = s = Xmalloc((unsigned) (len_nm + len_cl + 2)))) {
		if (len_nm) {
			strcpy(s, classhint->res_name);
			s += len_nm + 1;
		} else {
			*s++ = '\0';
		}
		if (len_cl) strcpy(s, classhint->res_class);
		else *s = '\0';
		XChangeProperty(dpy, w, XA_WM_CLASS, XA_STRING, 8,
			PropModeReplace, (unsigned char *)class_string,
			len_nm+len_cl+2);
		Xfree(class_string);
	}
	return 1;
}

// old routine
int XSetNormalHints(Display *dpy, Window w, XSizeHints *hints)
{
	DPRINTF("XSetNormalHints called...\n");
	return 0;
	//return XSetSizeHints(dpy, w, hints, XA_WM_NORMAL_HINTS);
}

int XSetTransientForHint(Display *dpy, Window w, Window propWindow)
{
	DPRINTF("XSetTransientForHint called..\n");
	return XChangeProperty(dpy, w, XA_WM_TRANSIENT_FOR, XA_WINDOW, 32,
		PropModeReplace, (unsigned char*)&propWindow, 1);
}
