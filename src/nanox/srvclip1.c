/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include <stdio.h>
#include <stdlib.h>
#include "serv.h"

/*
 * Macro to distinguish cases of clipping.
 */
#define	GAPVAL(leftgap, rightgap, topgap, bottomgap) \
	(((leftgap) << 3) + ((rightgap) << 2) + ((topgap) << 1) + (bottomgap))

static GR_COUNT	GsSplitClipRect(MWCLIPRECT *srcrect, MWCLIPRECT *destrect,
			GR_COORD minx, GR_COORD miny, GR_COORD maxx,
			GR_COORD maxy);
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
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*sibwp;		/* sibling windows */
	MWCLIPRECT	*clip;		/* first clip rectangle */
	GR_COUNT	count;		/* number of clip rectangles */
	GR_COUNT	newcount;	/* number of new rectangles */
	GR_COUNT	i;		/* current index */
	GR_COORD	minx;		/* minimum clip x coordinate */
	GR_COORD	miny;		/* minimum clip y coordinate */
	GR_COORD	maxx;		/* maximum clip x coordinate */
	GR_COORD	maxy;		/* maximum clip y coordinate */
	GR_COORD	diff;		/* difference in coordinates */
	GR_SIZE		bs;		/* border size */
	GR_BOOL		toomany;	/* TRUE if too many clip rects */
	MWCLIPRECT	cliprects[MAX_CLIPRECTS];	/* clip rectangles */

	if (wp->unmapcount || !wp->output || (wp == clipwp))
		return;

	clipwp = wp;

	/*
	 * Start with the rectangle for the complete window.
	 * We will then cut pieces out of it as needed.
	 */
	count = 1;
	clip = cliprects;
	clip->x = wp->x;
	clip->y = wp->y;
	clip->width = wp->width;
	clip->height = wp->height;

	/*
	 * First walk upwards through all parent windows,
	 * and restrict the visible part of this window to the part
	 * that shows through all of those parent windows.
	 */
	pwp = wp;
	while (pwp != rootwp) {
		pwp = pwp->parent;

		diff = pwp->x - clip->x;
		if (diff > 0) {
			clip->width -= diff;
			clip->x = pwp->x;
		}

		diff = (pwp->x + pwp->width) - (clip->x + clip->width);
		if (diff < 0)
			clip->width += diff;

		diff = pwp->y - clip->y;
		if (diff > 0) {
			clip->height -= diff;
			clip->y = pwp->y;
		}

		diff = (pwp->y + pwp->height) - (clip->y + clip->height);
		if (diff < 0)
			clip->height += diff;
	}

	/*
	 * If the window is completely clipped out of view, then
	 * set the clipping region to indicate that.
	 */
	if ((clip->width <= 0) || (clip->height <= 0)) {
		GdSetClipRects(clipwp->psd, 1, cliprects);
		return;
	}

	/*
	 * Now examine all windows that obscure this window, and
	 * for each obscuration, break up the clip rectangles into
	 * the smaller pieces that are still visible.  The windows
	 * that can obscure us are the earlier siblings of all of
	 * our parents.
 	 */
	toomany = GR_FALSE;
	pwp = wp;
/*while (pwp != rootwp) {*/
	while (pwp != NULL) {
		wp = pwp;
		pwp = wp->parent;

		if(!pwp) {
			/* We're clipping the root window*/
			sibwp = rootwp->children;
			wp = NULL;
		} else
			sibwp = pwp->children;

		for (; sibwp != wp; sibwp = sibwp->siblings) {
			if (sibwp->unmapcount || !sibwp->output)
				continue;

			bs = sibwp->bordersize;
			minx = sibwp->x - bs;
			miny = sibwp->y - bs;
			maxx = sibwp->x + sibwp->width + bs - 1;
			maxy = sibwp->y + sibwp->height + bs - 1;

			newcount = count;
			for (i = 0; i < count; i++) {
				if (newcount > MAX_CLIPRECTS - 3) {
					toomany = GR_TRUE;
					break;
				}
				newcount += GsSplitClipRect(&cliprects[i],
					&cliprects[newcount],
					minx, miny, maxx, maxy);
			}
			count = newcount;
		}
if(pwp == rootwp)
break;
	}

	if (toomany) {
		GsError(GR_ERROR_TOO_MUCH_CLIPPING, wp->id);
		clip->x = 0;
		clip->y = 0;
		clip->width = -1;
		clip->height = -1;
		count = 1;
	}

	/*
	 * Set the clip rectangles.
	 */
	GdSetClipRects(clipwp->psd, count, cliprects);
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
static GR_COUNT
GsSplitClipRect(MWCLIPRECT *srcrect, MWCLIPRECT *destrect, GR_COORD minx,
	GR_COORD miny, GR_COORD maxx, GR_COORD maxy)
{
	GR_COORD	x;
	GR_COORD	y;
	GR_SIZE		width;
	GR_SIZE		height;
	GR_COORD	dx;
	GR_COORD	dy;
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
