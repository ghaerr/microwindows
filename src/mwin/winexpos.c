/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Non-Win32 API helper routines for window showing/hiding/exposing
 */
#include "windows.h"
#include "wintern.h"
#include <stdlib.h>

/* Redraw all windows*/
void
MwRedrawScreen(void)
{
	/* redraw all windows except desktop window*/
	MwExposeArea(rootwp, 0, 0, rootwp->winrect.right,
		rootwp->winrect.bottom);

	/* redraw desktop window*/
	PostMessage(rootwp, WM_PAINT, 0, 0L);
}

/*
 * Hide the window to make it and its children invisible on the screen.
 * This is a recursive routine which increments the unmapcount values for
 * this window and all of its children, and causes exposure events for
 * windows which are newly uncovered.
 */
void
MwHideWindow(HWND hwnd,BOOL bChangeFocus, BOOL bSendMsg)
{
	HWND	wp = hwnd;
	HWND	pwp;		/* parent window */
	HWND	sibwp;		/* sibling window */
	HWND	childwp;	/* child window */

	if (wp == rootwp)
		return;

	++mwpaintNC;		/* experimental NC paint handling*/

	/* send hide message if currently visible*/
	if(bSendMsg && wp->unmapcount == 0)
		SendMessage(wp, WM_SHOWWINDOW, FALSE, 0L);

	wp->unmapcount++;

	for (childwp = wp->children; childwp; childwp = childwp->siblings)
		MwHideWindow(childwp, bChangeFocus, bSendMsg);

	if (wp == mousewp) {
		MwCheckMouseWindow();
		MwCheckCursor();
	}

	if (bChangeFocus && wp == focuswp)
		SetFocus(rootwp->children? rootwp->children: rootwp);

	/*
	 * If the parent window is still unmapped, then we are all done.
	 */
	if (wp->parent->unmapcount)
		return;

	/*
	 * Clear the area in the parent for this window, causing an
	 * exposure event for it.
	 */
	pwp = wp->parent;
	MwClearWindow(pwp, wp->winrect.left - pwp->winrect.left,
		wp->winrect.top - pwp->winrect.top,
		wp->winrect.right - wp->winrect.left,
		wp->winrect.bottom - wp->winrect.top, TRUE);

	/*
	 * Finally clear and redraw all parts of our lower sibling
	 * windows that were covered by this window.
	 */
	sibwp = wp;
	while (sibwp->siblings) {
		sibwp = sibwp->siblings;
		MwExposeArea(sibwp, wp->winrect.left, wp->winrect.top,
			wp->winrect.right - wp->winrect.left,
			wp->winrect.bottom - wp->winrect.top);
	}
}

/*
 * Map the window to possibly make it and its children visible on the screen.
 * This is a recursive routine which decrements the unmapcount values for
 * this window and all of its children, and causes exposure events for
 * those windows which become visible.
 */
void
MwShowWindow(HWND hwnd, BOOL bSendMsg)
{
	HWND	wp = hwnd;

	if (wp == rootwp)
		return;

	++mwpaintNC;		/* experimental NC paint handling*/

	if (wp->unmapcount)
		wp->unmapcount--;

	if (wp->unmapcount == 0) {
		SendMessage(wp, WM_SHOWWINDOW, TRUE, 0L);
		MwCheckMouseWindow();
		MwCheckCursor();
	}

	/*
	 * If the window just became visible,
	 * then draw its border, clear it to the background color, and
	 * generate an exposure event.
	 */
	if (wp->unmapcount == 0) {
		/*MwDrawBorder(wp);*/
		MwClearWindow(wp, 0, 0, wp->winrect.right - wp->winrect.left,
			wp->winrect.bottom - wp->winrect.top, TRUE);
	}

	/*
	 * Do the same thing for the children.
	 */
	for (wp = wp->children; wp; wp = wp->siblings)
		MwShowWindow(wp, bSendMsg);
}

/*
 * Raise a window to the highest level among its siblings.
 */
void
MwRaiseWindow(HWND hwnd)
{
	HWND	wp = hwnd;
	HWND	prevwp;
	BOOL	overlap;

	if (!wp || wp == rootwp)
		return;

	++mwpaintNC;		/* experimental NC paint handling*/

	/*
	 * If this is already the highest window then we are done.
	 */
	prevwp = wp->parent->children;
	if (prevwp == wp)
		return;

	/*
	 * Find the sibling just before this window so we can unlink it.
	 * Also, determine if any sibling ahead of us overlaps the window.
	 * Remember that for exposure events.
	 */
	overlap = FALSE;
	while (prevwp->siblings != wp) {
		overlap |= MwCheckOverlap(prevwp, wp);
		prevwp = prevwp->siblings;
	}
	overlap |= MwCheckOverlap(prevwp, wp);

	/*
	 * Now unlink the window and relink it in at the front of the
	 * sibling chain.
	 */
	prevwp->siblings = wp->siblings;
	wp->siblings = wp->parent->children;
	wp->parent->children = wp;

	/*
	 * Finally redraw the window if necessary.
	 */
	if (overlap) {
		/*MwDrawBorder(wp);*/
		MwExposeArea(wp, wp->winrect.left, wp->winrect.top,
			wp->winrect.right - wp->winrect.left,
			wp->winrect.bottom - wp->winrect.top);
	}
}

/*
 * Lower a window to the lowest level among its siblings.
 */
void
MwLowerWindow(HWND hwnd)
{
	HWND	wp = hwnd;
	HWND	prevwp;
	HWND	sibwp;		/* sibling window */
	HWND	expwp;		/* siblings being exposed */

	if (!wp || wp == rootwp || !wp->siblings)
		return;

	++mwpaintNC;		/* experimental NC paint handling*/

	/*
	 * Find the sibling just before this window so we can unlink us.
	 */
	prevwp = wp->parent->children;
	if (prevwp != wp) {
		while (prevwp->siblings != wp)
			prevwp = prevwp->siblings;
	}

	/*
	 * Remember the first sibling that is after us, so we can
	 * generate exposure events for the remaining siblings.  Then
	 * walk down the sibling chain looking for the last sibling.
	 */
	expwp = wp->siblings;
	sibwp = wp;
	while (sibwp->siblings)
		sibwp = sibwp->siblings;

	/*
	 * Now unlink the window and relink it in at the end of the
	 * sibling chain.
	 */
	if (prevwp == wp)
		wp->parent->children = wp->siblings;
	else
		prevwp->siblings = wp->siblings;
	sibwp->siblings = wp;

	wp->siblings = NULL;

	/*
	 * Finally redraw the sibling windows which this window covered
	 * if they overlapped our window.
	 */
	while (expwp && (expwp != wp)) {
		if (MwCheckOverlap(wp, expwp))
			MwExposeArea(expwp, wp->winrect.left, wp->winrect.top,
				wp->winrect.right - wp->winrect.left,
				wp->winrect.bottom - wp->winrect.top);
		expwp = expwp->siblings;
	}
}

/*
 * Check to see if the first window overlaps the second window.
 */
BOOL
MwCheckOverlap(HWND topwp, HWND botwp)
{
	MWCOORD	minx1;
	MWCOORD	miny1;
	MWCOORD	maxx1;
	MWCOORD	maxy1;
	MWCOORD	minx2;
	MWCOORD	miny2;
	MWCOORD	maxx2;
	MWCOORD	maxy2;

	if (topwp->unmapcount || botwp->unmapcount)
		return FALSE;

	minx1 = topwp->winrect.left;
	miny1 = topwp->winrect.top;
	maxx1 = topwp->winrect.right - 1;
	maxy1 = topwp->winrect.bottom - 1;

	minx2 = botwp->winrect.left;
	miny2 = botwp->winrect.top;
	maxx2 = botwp->winrect.right - 1;
	maxy2 = botwp->winrect.bottom - 1;

	if (minx1 > maxx2 || minx2 > maxx1 || miny1 > maxy2 || miny2 > maxy1)
		return FALSE;

	return TRUE;
}

/*
 * Clear the specified area of a window and possibly make an exposure event.
 * This sets the area window to its background color.  If the exposeflag is
 * nonzero, then this also creates an exposure event for the window.
 */
void
MwClearWindow(HWND wp, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	BOOL exposeflag)
{
	if (wp->unmapcount)
		return;

	/*
	 * Reduce the arguments so that they actually lie within the window.
	 */
	if (x < 0) {
		width += x;
		x = 0;
	}
	if (y < 0) {
		height += y;
		y = 0;
	}
	if (x + width > wp->winrect.right - wp->winrect.left)
		width = (wp->winrect.right - wp->winrect.left) - x;
	if (y + height > wp->winrect.bottom - wp->winrect.top)
		height = (wp->winrect.bottom - wp->winrect.top) - y;

	/*
	 * Now see if the region is really in the window.  If not, then
	 * do nothing.
	 */
	if (x >= (wp->winrect.right - wp->winrect.left) ||
	    y >= (wp->winrect.bottom - wp->winrect.top) ||
	    width <= 0 || height <= 0)
		return;

	/*
	 * Now do the exposure if required.
	 */
	if (exposeflag)
		MwDeliverExposureEvent(wp, x, y, width, height);
}

/*
 * Handle the exposing of the specified absolute region of the screen,
 * starting with the specified window.  That window and all of its
 * children will be redrawn and/or exposure events generated if they
 * overlap the specified area.  This is a recursive routine.
 */
void
MwExposeArea(HWND wp, MWCOORD rootx, MWCOORD rooty, MWCOORD width,
	MWCOORD height)
{
	if (wp->unmapcount)
		return;

	++mwpaintNC;		/* experimental NC paint handling*/

	/*
	 * First see if the area overlaps the window including the border.
	 * If not, then there is nothing more to do.
	 */
	if (rootx >= wp->winrect.right || rooty >= wp->winrect.bottom ||
	    (rootx + width) <= wp->winrect.left ||
	    (rooty + height) <= wp->winrect.top)
		return;

#if 0
	/*
	 * The area does overlap the window.  See if the area overlaps
	 * the border, and if so, then redraw it.
	 */
	if (rootx < wp->winrect.left || rooty < wp->winrect.top ||
		(rootx + width) > wp->winrect.right ||
		(rooty + height) > wp->winrect.bottom)
			MwDrawBorder(wp);
#endif

	/*
	 * Now clear the window itself in the specified area,
	 * which might cause an exposure event.
	 */
	MwClearWindow(wp, rootx - wp->winrect.left, rooty - wp->winrect.top,
		width, height, TRUE);

	/*
	 * Now do the same for all the children.
	 */
	for (wp = wp->children; wp; wp = wp->siblings)
		MwExposeArea(wp, rootx, rooty, width, height);
}
