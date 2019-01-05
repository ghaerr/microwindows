#include "nxlib.h"

/* Flush all buffered output requests. */
/* NOTE: NOT necessary when calling any of the Xlib routines. */

int
XFlush (Display *dpy)
{
	GrFlush();
	return 1;
}


    

