/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include "windows.h"
#include "wintern.h"

/*
 * Macro to distinguish cases of clipping.
 */
#define	GAPVAL(leftgap, rightgap, topgap, bottomgap) \
	(((leftgap) << 3) + ((rightgap) << 2) + ((topgap) << 1) + (bottomgap))

static BOOL MwExcludeClipRect(int minx,int miny,int maxx,int maxy,int *count,
		MWCLIPRECT *cliprects);
static int  MwSplitClipRect(MWCLIPRECT *srcrect, MWCLIPRECT *destrect,
		MWCOORD minx, MWCOORD miny, MWCOORD maxx, MWCOORD maxy);

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
	MWCLIPRECT	*clip;		/* first clip rectangle */
	int		count;		/* number of clip rectangles */
	MWCOORD		diff;		/* difference in coordinates */
	BOOL		toomany;	/* TRUE if too many clip rects */
	PRECT		prc;		/* client or window rectangle*/
	MWCLIPRECT	cliprects[MAX_CLIPRECTS];	/* clip rectangles */

	if (wp->unmapcount)
		return;

	/*
	 * Start with the rectangle for the complete window.
	 * We will then cut pieces out of it as needed.
	 */
	prc = MwIsClientDC(hdc)? &wp->clirect: &wp->winrect;
	count = 1;
	clip = cliprects;
	clip->x = prc->left;
	clip->y = prc->top;
	clip->width = prc->right - prc->left;
	clip->height = prc->bottom - prc->top;

	/*
	 * First walk upwards through all parent windows,
	 * and restrict the visible part of this window to the part
	 * that shows through all of those parent windows client areas.
	 */
	pwp = wp;
	while (pwp != rootwp) {
		pwp = pwp->parent;

		diff = pwp->clirect.left - clip->x;
		if (diff > 0) {
			clip->width -= diff;
			clip->x = pwp->clirect.left;
		}

		diff = pwp->clirect.right - (clip->x + clip->width);
		if (diff < 0)
			clip->width += diff;

		diff = pwp->clirect.top - clip->y;
		if (diff > 0) {
			clip->height -= diff;
			clip->y = pwp->clirect.top;
		}

		diff = pwp->clirect.bottom - (clip->y + clip->height);
		if (diff < 0)
			clip->height += diff;
	}

	/*
	 * If the window is completely clipped out of view, then
	 * set the clipping region to indicate that.
	 */
	if (clip->width <= 0 || clip->height <= 0) {
		GdSetClipRects(hdc->psd, 1, cliprects);
		return;
	} 

	/*
	 * Now examine all windows that obscure this window, and
	 * for each obscuration, break up the clip rectangles into
	 * the smaller pieces that are still visible.  The windows
	 * that can obscure us are the earlier siblings of all of
	 * our parents. When clipping the root window, search all children.
 	 */
	toomany = FALSE;
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

			toomany |= MwExcludeClipRect(sibwp->winrect.left,
				sibwp->winrect.top, sibwp->winrect.right-1,
				sibwp->winrect.bottom-1, &count, cliprects);

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

			toomany |= MwExcludeClipRect(sibwp->winrect.left,
				sibwp->winrect.top, sibwp->winrect.right-1,
				sibwp->winrect.bottom-1, &count, cliprects);
		}
	}

	if (toomany) {
		/*GsError(GR_ERROR_TOO_MUCH_CLIPPING, wp->id);*/
		clip->x = 0;
		clip->y = 0;
		clip->width = -1;
		clip->height = -1;
		count = 1;
	}

	/*
	 * Set the clip rectangles.
	 */
	GdSetClipRects(hdc->psd, count, cliprects);
}

static BOOL
MwExcludeClipRect(int minx,int miny,int maxx,int maxy,int *count,
	MWCLIPRECT *cliprects)
{
	int	i;		/* current index */
	int	newcount;	/* number of new rectangles */
	BOOL	toomany = FALSE;/* TRUE if too many clip rects */

	newcount = *count;
	for (i = 0; i < *count; i++) {
		if (newcount > MAX_CLIPRECTS - 3) {
			toomany = TRUE;
			break;
		}
		newcount += MwSplitClipRect(&cliprects[i],
			&cliprects[newcount],
			minx, miny, maxx, maxy);
	}
	*count = newcount;
	return toomany;
}

/*
 * Check the specified clip rectangle against the specified rectangular
 * region, and reduce it or split it up into multiple clip rectangles
 * such that the specified region is not contained in any of the clip
 * rectangles.  The source clip rectangle can be modified in place, and
 * in addition more clip rectangles can be generated, which are placed in
 * the indicated destination location.  The maximum number of new clip
 * rectangles needed is 3.  Returns the number of clip rectangles added.
 * If the source clip rectangle is totally obliterated, it is set to an
 * impossible region and 0 is returned.  When splits are done, we prefer
 * to create wide regions instead of high regions.
 */
static int
MwSplitClipRect(MWCLIPRECT *srcrect, MWCLIPRECT *destrect, MWCOORD minx,
	MWCOORD miny, MWCOORD maxx, MWCOORD maxy)
{
	MWCOORD		x;
	MWCOORD		y;
	MWCOORD		width;
	MWCOORD		height;
	MWCOORD		dx;
	MWCOORD		dy;
	int		gaps;

	/*
	 * First see if there is any overlap at all.
	 * If not, then nothing to do.
	 */
	x = srcrect->x;
	y = srcrect->y;
	width = srcrect->width;
	height = srcrect->height;

	if ((minx > maxx) || (miny > maxy) || (maxx < x) || (maxy < y) ||
		(x + width <= minx) || (y + height <= miny))
			return 0;

	/*
	 * There is an overlap.  Calculate a value to differentiate
	 * various cases, and then handle each case separately.  The
	 * cases are classified on whether there are gaps on the left,
	 * right, top, and bottom sides of the clip rectangle.
	 */
	gaps = 0;
	if (x < minx)
		gaps |= GAPVAL(1, 0, 0, 0);
	if (x + width - 1 > maxx)
		gaps |= GAPVAL(0, 1, 0, 0);
	if (y < miny)
		gaps |= GAPVAL(0, 0, 1, 0);
	if (y + height - 1 > maxy)
		gaps |= GAPVAL(0, 0, 0, 1);

	switch (gaps) {
		case GAPVAL(0, 0, 0, 0):	/* no gaps at all */
			srcrect->x = 0;
			srcrect->y = 0;
			srcrect->width = 0;
			srcrect->height = 0;
			return 0;

		case GAPVAL(0, 0, 0, 1):	/* gap on bottom */
			dy = maxy - y + 1;
			srcrect->y += dy;
			srcrect->height -= dy;
			return 0;

		case GAPVAL(0, 0, 1, 0):	/* gap on top */
			srcrect->height = miny - y;
			return 0;

		case GAPVAL(0, 0, 1, 1):	/* gap on top, bottom */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 1;

		case GAPVAL(0, 1, 0, 0):	/* gap on right */
			dx = maxx - x + 1;
			srcrect->x += dx;
			srcrect->width -= dx;
			return 0;

		case GAPVAL(0, 1, 0, 1):	/* gap on right, bottom */
			dx = maxx - x + 1;
			srcrect->x += dx;
			srcrect->width -= dx;
			srcrect->height = maxy - y + 1;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 1;

		case GAPVAL(0, 1, 1, 0):	/* gap on right, top */
			dx = maxx - x + 1;
			srcrect->height = miny - y;
			destrect->x = x + dx;
			destrect->width = width - dx;
			destrect->y = miny;
			destrect->height = y + height - miny;
			return 1;

		case GAPVAL(0, 1, 1, 1):	/* gap on right, top, bottom */
			dx = maxx - x + 1;
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			destrect++;
			destrect->x = x + dx;
			destrect->width = width - dx;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			return 2;

		case GAPVAL(1, 0, 0, 0):	/* gap on left */
			srcrect->width = minx - x;
			return 0;

		case GAPVAL(1, 0, 0, 1):	/* gap on left, bottom */
			srcrect->width = minx - x;
			srcrect->height = maxy - y + 1;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 1;

		case GAPVAL(1, 0, 1, 0):	/* gap on left, top */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = y + height - miny;
			return 1;

		case GAPVAL(1, 0, 1, 1):	/* gap on left, top, bottom */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			destrect++;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 2;

		case GAPVAL(1, 1, 0, 0):	/* gap on left, right */
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = y;
			destrect->height = height;
			srcrect->width = minx - x;
			return 1;

		case GAPVAL(1, 1, 0, 1):	/* gap on left, right, bottom */
			dy = maxy - y + 1;
			srcrect->y += dy;
			srcrect->height -= dy;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = y;
			destrect->height = dy;
			destrect++;
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = y;
			destrect->height = dy;
			return 2;

		case GAPVAL(1, 1, 1, 0):	/* gap on left, right, top */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = y + height - miny;
			destrect++;
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = miny;
			destrect->height = y + height - miny;
			return 2;

		case GAPVAL(1, 1, 1, 1):	/* gap on all sides */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			destrect++;
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			destrect++;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 3;
	}
	return 0; /* NOTREACHED */
}
