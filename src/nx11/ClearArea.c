#include "nxlib.h"

int
XClearArea (Display *dpy, Window w, int x, int y,
	unsigned int width, unsigned int height, Bool exposures)
{
	GrClearArea(w, x, y, width, height, exposures);
	return 1;
}
