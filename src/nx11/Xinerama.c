/* 
 * Xinerama support 
 * 2019 Georg Potthast
 */

#include "nxlib.h"
#include <stdlib.h>

#include <X11/extensions/Xinerama.h>

int 
XineramaIsActive (Display *dpy) 
{ 
	return 1; /*is active */
} 

XineramaScreenInfo* 
XineramaQueryScreens (Display *dpy, int *number)
{
	GR_SCREEN_INFO sinfo;
	GrGetScreenInfo(&sinfo);

	XineramaScreenInfo *xsi = (XineramaScreenInfo *) Xcalloc(1, sizeof(XineramaScreenInfo));
	xsi->x_org  = 0;
	xsi->y_org  = 0;
	xsi->width  = sinfo.cols;
	xsi->height = sinfo.rows;

	return xsi;
}
 
