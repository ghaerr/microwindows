#include "nxlib.h"
#include <stdlib.h>

int
XFreeGC(Display *dpy, GC gc)
{
	XGCValues *vp = (XGCValues *)gc->ext_data;

	/* free current region, if any*/
	if (vp->clip_mask != None)
		GrDestroyRegion(vp->clip_mask);

	GrDestroyGC(gc->gid);
	Xfree(vp);
	Xfree(gc);

	return 1;
}
