#include "nxlib.h"

int
XResizeWindow(Display *dpy, Window w, unsigned int width, unsigned int height)
{
	GrResizeWindow(w, width, height);
	return 1;
}

//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#define AllMaskBits (CWX|CWY|CWWidth|CWHeight|CWBorderWidth|CWSibling|CWStackMode)

int XConfigureWindow(Display *dpy, Window w, unsigned int mask, XWindowChanges *changes)
{
	GR_WINDOW_INFO wp;
	GrGetWindowInfo(w, &wp);

	DPRINTF("XConfigureWindow called...");
	//mask &= AllMaskBits;
	if (mask & (CWX|CWY)) {
		DPRINTF(" XY(%d,%d)", mask&CWX? changes->x: wp.x, mask&CWY? changes->y: wp.y);
		GrMoveWindow(w, mask&CWX? changes->x: wp.x, mask&CWY? changes->y: wp.y);
	}
	if (mask & (CWWidth|CWHeight)) {
		DPRINTF(" WH(%d,%d)", mask&CWWidth? changes->width: wp.width, mask&CWHeight? changes->height: wp.height);
		GrResizeWindow(w, mask&CWWidth? changes->width: wp.width, mask&CWHeight? changes->height: wp.height);
	}
	if (mask & CWBorderWidth) {
		DPRINTF(" BW(%d)", changes->border_width);
		XSetWindowBorderWidth(dpy, w, changes->border_width);
	}

	if (mask & CWSibling) {
		DPRINTF(" Sib(%lu)", changes->sibling);
	}
	if (mask & CWStackMode) {
		DPRINTF(" Stc(%d)", changes->stack_mode);
	}
	DPRINTF("\n");

	return 1;
}
