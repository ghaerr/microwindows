//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"

// required for openoffice.org
Status XInitThreads()
{
	DPRINTF("XInitThreads called...");
	return 1;
}

// for xine
void XLockDisplay(Display *display)
{
	LockDisplay(display);
}

void XUnlockDisplay(Display *display)
{
	UnlockDisplay(display);
}
