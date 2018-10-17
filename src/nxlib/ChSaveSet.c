//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"

// required for rasp
int XChangeSaveSet(Display *dpy, Window win, int mode)
{
	DPRINTF("XChangeSaveSet called...\n");
	return 1;
}

int XAddToSaveSet(Display *dpy, Window win)
{
	return XChangeSaveSet(dpy, win, SetModeInsert);
}

int XRemoveFromSaveSet(Display *dpy, Window win)
{
	return XChangeSaveSet(dpy, win, SetModeDelete);
}
