/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * DYNAMICREGIONS GsSetClipWindow
 */
#include <stdio.h>
#include <stdlib.h>
#include "serv.h"

/*
 * Set the clip rectangles for a window taking into account other
 * windows that may be obscuring it.  The windows that may be obscuring
 * this one are the siblings of each direct ancestor which are higher
 * in priority than those ancestors.  Also, each parent limits the visible
 * area of the window.  The clipping is not done if it is already up to
 * date of if the window is not outputtable.
 */
void
GsSetClipWindow(GR_WINDOW *wp, MWCLIPREGION *userregion, int flags)
{
	GR_WINDOW	*orgwp;		/* original window pointer */
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*sibwp;		/* sibling windows */
	GR_COORD	minx;		/* minimum clip x coordinate */
	GR_COORD	miny;		/* minimum clip y coordinate */
	GR_COORD	maxx;		/* maximum clip x coordinate */
	GR_COORD	maxy;		/* maximum clip y coordinate */
	GR_COORD	diff;		/* difference in coordinates */
	GR_SIZE		bs;		/* border size */
	GR_COORD	x, y, width, height;
	MWCLIPREGION	*vis, *r;

	if (wp->unmapcount || !wp->output)
		return;

	clipwp = wp;

	/*
	 * Start with the rectangle for the complete window.
	 * We will then cut pieces out of it as needed.
	 */
	x = wp->x;
	y = wp->y;
	width = wp->width;
	height = wp->height;

	/*
	 * First walk upwards through all parent windows,
	 * and restrict the visible part of this window to the part
	 * that shows through all of those parent windows.
	 */
	pwp = wp;
	while (pwp != rootwp) {
		pwp = pwp->parent;

		diff = pwp->x - x;
		if (diff > 0) {
			width -= diff;
			x = pwp->x;
		}

		diff = (pwp->x + pwp->width) - (x + width);
		if (diff < 0)
			width += diff;

		diff = pwp->y - y;
		if (diff > 0) {
			height -= diff;
			y = pwp->y;
		}

		diff = (pwp->y + pwp->height) - (y + height);
		if (diff < 0)
			height += diff;
	}

	/*
	 * If the window is completely clipped out of view, then
	 * set the clipping region to indicate that.
	 */
	if (width <= 0 || height <= 0) {
		GdSetClipRegion(clipwp->psd, NULL);
		return;
	}

	/*
	 * Allocate region to clipped size of window
	 */
	vis = GdAllocRectRegion(x, y, x+width, y+height);

	/* 
	 * Allocate temp region
	 */
	r = GdAllocRegion();

	/*
	 * Now examine all windows that obscure this window, and
	 * for each obscuration, break up the clip rectangles into
	 * the smaller pieces that are still visible.  The windows
	 * that can obscure us are the earlier siblings of all of
	 * our parents.
 	 */
	orgwp = wp;
	pwp = wp;
	while (pwp != NULL) {
		wp = pwp;
		pwp = wp->parent;

		if(!pwp) {
			/* We're clipping the root window*/
			if (!(flags & GR_MODE_EXCLUDECHILDREN))
				/* start with root's children*/
				sibwp = rootwp->children;
			else sibwp = NULL;	 /* no search*/
			wp = NULL;		 /* search all root's children*/
		} else {
			sibwp = pwp->children;	 /* clip siblings*/
		}

		for (; sibwp != wp; sibwp = sibwp->siblings) {
			if (sibwp->unmapcount || !sibwp->output)
				continue;

			bs = sibwp->bordersize;
			minx = sibwp->x - bs;
			miny = sibwp->y - bs;
			maxx = sibwp->x + sibwp->width + bs;
			maxy = sibwp->y + sibwp->height + bs;

			GdSetRectRegion(r, minx, miny, maxx, maxy);
			GdSubtractRegion(vis, vis, r);
		}

		/* if not clipping the root window, stop when you reach it*/
		if (pwp == rootwp)
			break;
	}

	wp = orgwp;
	/*
	 * If not the root window, clip all children.
	 * (Root window's children are are clipped above)
	 */
	if(wp != rootwp && !(flags & GR_MODE_EXCLUDECHILDREN)) {
		for (sibwp=wp->children; sibwp; sibwp = sibwp->siblings) {
			if (sibwp->unmapcount || !sibwp->output)
				continue;

			bs = sibwp->bordersize;
			minx = sibwp->x - bs;
			miny = sibwp->y - bs;
			maxx = sibwp->x + sibwp->width + bs;
			maxy = sibwp->y + sibwp->height + bs;

			GdSetRectRegion(r, minx, miny, maxx, maxy);
			GdSubtractRegion(vis, vis, r);
		}
	}

	/*
	 * Intersect with user region, if set.
	 */
	if (userregion) {
		/* temporarily offset region by window coordinates*/
		GdOffsetRegion(userregion, wp->x, wp->y);
		GdIntersectRegion(vis, vis, userregion);
		GdOffsetRegion(userregion, -wp->x, -wp->y);
	}

	/*
	 * Set the clip region (later destroy handled by GdSetClipRegion)
	 */
	GdSetClipRegion(clipwp->psd, vis);

	/*
	 * Destroy temp region
	 */
	GdDestroyRegion(r);
}
