/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * DYNAMICREGIONS MwSetClipWindow
 */
#include "windows.h"
#include "wintern.h"

/*
 * Set the clip rectangles for a window taking into account other
 * windows that may be obscuring it.  The windows that may be obscuring
 * this one are the siblings of each direct ancestor which are higher
 * in priority than those ancestors.  Also, each parent limits the visible
 * area of the window.
 */
void
MwSetClipWindow(HDC hdc)
{
	HWND		wp = hdc->hwnd;
	HWND		pwp;		/* parent window */
	HWND		sibwp;		/* sibling windows */
	MWCOORD		diff;		/* difference in coordinates */
	PRECT		prc;		/* client or window rectangle*/
	MWCLIPREGION	*vis, *r;
	MWCOORD		x, y, width, height;

	/*
	 * Start with the rectangle for the complete window.
	 * We will then cut pieces out of it as needed.
	 */
	prc = MwIsClientDC(hdc)? &wp->clirect: &wp->winrect;
	x = prc->left;
	y = prc->top;
	width = prc->right - prc->left;
	height = prc->bottom - prc->top;

	/*
	 * First walk upwards through all parent windows,
	 * and restrict the visible part of this window to the part
	 * that shows through all of those parent windows client areas.
	 */
	pwp = wp;
	while (pwp != rootwp) {
		pwp = pwp->parent;

		diff = pwp->clirect.left - x;
		if (diff > 0) {
			width -= diff;
			x = pwp->clirect.left;
		}

		diff = pwp->clirect.right - (x + width);
		if (diff < 0)
			width += diff;

		diff = pwp->clirect.top - y;
		if (diff > 0) {
			height -= diff;
			y = pwp->clirect.top;
		}

		diff = pwp->clirect.bottom - (y + height);
		if (diff < 0)
			height += diff;
	}

	/*
	 * If the window is completely clipped out of view, then
	 * set the clipping region to indicate that.
	 */
	if (width <= 0 || height <= 0) {
		GdSetClipRegion(hdc->psd, NULL);
		return;
	} 

	/*
	 * Allocate initial vis region to parent-clipped size of window
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
	 * our parents. When clipping the root window, search all children.
 	 */
	pwp = wp;
	while (pwp != NULL) {
		wp = pwp;
		pwp = wp->parent;
		if(!pwp) {
			/* We're clipping the root window*/
			if(hdc->flags & DCX_CLIPCHILDREN)
				/* start with root's children*/
				sibwp = rootwp->children;
			else sibwp = NULL;	/* no search*/
			wp = NULL;		/* search all root's children*/
		} else {
			if(hdc->flags & DCX_CLIPSIBLINGS)
				sibwp = pwp->children;
			else sibwp = wp;	/* no search*/
		}
		for (; sibwp != wp; sibwp = sibwp->siblings) {
			if (sibwp->unmapcount)
				continue;

			GdSetRectRegionIndirect(r, &sibwp->winrect);
			GdSubtractRegion(vis, vis, r);
		}

		/* if not clipping the root window, stop when you reach it*/
		if(pwp == rootwp)
			break;
	}

	/*
	 * If not the root window and we're going to be drawing
	 * in the client area, clip all children.  This is
	 * required for non-special paint handling for child windows.
	 * Non-client dc's don't clip children in order to get
	 * proper border clipping in the case of border-clipped children.
	 */
	wp = hdc->hwnd;
	if(wp != rootwp && MwIsClientDC(hdc)) {
		for (sibwp=wp->children; sibwp; sibwp = sibwp->siblings) {
			if (sibwp->unmapcount)
				continue;

			GdSetRectRegionIndirect(r, &sibwp->winrect);
			GdSubtractRegion(vis, vis, r);
		}
	}

#if UPDATEREGIONS
	/*
	 * Intersect with update region, unless requested not to.
	 */
	if(!(hdc->flags & DCX_EXCLUDEUPDATE))
		GdIntersectRegion(vis, vis, wp->update);
#endif
	/*
	 * Intersect with user region, if set.
	 */
	if (hdc->region)
		GdIntersectRegion(vis, vis, hdc->region->rgn);
	/*
	 * Set the clip region (later destroy handled by GdSetClipRegion)
	 */
	GdSetClipRegion(hdc->psd, vis);

	/*
	 * Destroy temp region
	 */
	GdDestroyRegion(r);
}
