#include "nxlib.h"

/* Flush all buffered output requests. */
/* NOTE: NOT necessary when calling any of the Xlib routines. */

int
XFlush (Display *dpy)
{
	GrFlush();
#if __EMSCRIPTEN__
extern void GsSelect(GR_TIMEOUT timeout);
	GsSelect(0);		/* flush and handle events*/
#endif
	return 1;
}


    

