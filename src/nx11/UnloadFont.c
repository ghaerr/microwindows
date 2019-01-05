#include "nxlib.h"

int
XUnloadFont(Display *dpy, Font font)
{
	GrDestroyFont(font);
	return 1;
}
