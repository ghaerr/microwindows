#include "nxlib.h"

int
XDestroyWindow(Display * dpy, Window w)
{

	/* FIXME - Remember to destroy all the various structures associated with this window */
	/* as well */

	/* Delete all window properties */
	_nxDelAllProperty(w);

	GrDestroyWindow(w);
	return 1;
}
