#include "nxlib.h"
#include <stdlib.h>
#include "X11/Xutil.h"

/* only one visual to match, no looping required*/
Status
XMatchVisualInfo(Display * dpy, int screen, int depth, int class,
		 XVisualInfo * vinfo)
{
	Depth *dp = &dpy->screens[0].depths[0];
	Visual *vp = dp->visuals;

	if (screen != 0 || dp->depth != depth || vp->class != class) {
		DPRINTF("XmatchVisualInfo failed: want depth %d class %d\n", depth, class);
		return 0;
	}

	vinfo->visual = vp;
	vinfo->visualid = vp->visualid;
	vinfo->screen = screen;
	vinfo->depth = depth;
	vinfo->class = class;
	vinfo->red_mask = vp->red_mask;
	vinfo->green_mask = vp->green_mask;
	vinfo->blue_mask = vp->blue_mask;
	vinfo->colormap_size = vp->map_entries;
	vinfo->bits_per_rgb = vp->bits_per_rgb;
	return 1;
}

XVisualInfo *
XGetVisualInfo(Display *dpy, long visual_info_mask,
	XVisualInfo *visual_info_template, int *nitems)
{
	Depth *dp = &dpy->screens[0].depths[0];
	Visual *vp = dp->visuals;
	XVisualInfo *vip;
	
	*nitems = 0;

	if ((visual_info_mask & VisualScreenMask) &&
	    visual_info_template->screen != 0)
		return NULL;

	if ((visual_info_mask & VisualDepthMask) &&
	    (dp->depth != visual_info_template->depth))
		return NULL;

	if ((visual_info_mask & VisualIDMask) &&
	    (vp->visualid != visual_info_template->visualid))
	    	return NULL;

	if ((visual_info_mask & VisualClassMask) &&
	    (vp->class != visual_info_template->class))
	    	return NULL;

	if ((visual_info_mask & VisualRedMaskMask) &&
	    (vp->red_mask != visual_info_template->red_mask))
	    	return NULL;

	if ((visual_info_mask & VisualGreenMaskMask) &&
	    (vp->green_mask != visual_info_template->green_mask))
	    	return NULL;

	if ((visual_info_mask & VisualBlueMaskMask) &&
	    (vp->blue_mask != visual_info_template->blue_mask))
	    	return NULL;

	if ((visual_info_mask & VisualColormapSizeMask) &&
	    (vp->map_entries != visual_info_template->colormap_size))
	    	return NULL;

	if ((visual_info_mask & VisualBitsPerRGBMask) &&
	    (vp->bits_per_rgb != visual_info_template->bits_per_rgb))
	    	return NULL;

	vip = (XVisualInfo *)Xmalloc(sizeof(XVisualInfo));
	if (!vip)
		return NULL;

	vip->visual = vp;
	vip->visualid = vp->visualid;
	vip->screen = 0;
	vip->depth = dp->depth;
	vip->class = vp->class;
	vip->red_mask = vp->red_mask;
	vip->green_mask = vp->green_mask;
	vip->blue_mask = vp->blue_mask;
	vip->colormap_size = vp->map_entries;
	vip->bits_per_rgb = vp->bits_per_rgb;
	*nitems = 1;
	return vip;
}
