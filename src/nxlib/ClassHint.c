#include "nxlib.h"
#include "X11/Xutil.h"

XClassHint *
XAllocClassHint(void)
{
	return (XClassHint *) Xcalloc(1, sizeof(XClassHint));
}

