#include <stdio.h>
#include <stdlib.h>
#include "device.h"
/*
 * Microwindows polygon outline and fill routines.
 * Copyright (c) 1999, 2000, 2001 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 *
 * There are currently three implementations of the polygon
 * fill routine.  The version from X11 most properly
 * fills polygons that must also be outlined as well. All are
 * controlled with #if directive in this file.
 */

/* extern definitions*/
void drawpoint(PSD psd,MWCOORD x, MWCOORD y);
void drawrow(PSD psd,MWCOORD x1,MWCOORD x2,MWCOORD y);
extern int 	  gr_mode; 	      /* drawing mode */

/* Draw a polygon in the foreground color, applying clipping if necessary.
 * The polygon is only closed if the first point is repeated at the end.
 * Some care is taken to plot the endpoints correctly if the current
 * drawing mode is XOR.  However, internal crossings are not handled
 * correctly.
 */
void
GdPoly(PSD psd, int count, MWPOINT *points)
{
  MWCOORD firstx;
  MWCOORD firsty;
  MWBOOL didline;

  if (count < 2)
	  return;
  firstx = points->x;
  firsty = points->y;
  didline = FALSE;

  while (count-- > 1) {
	if (didline && (gr_mode == MWMODE_XOR))
		drawpoint(psd, points->x, points->y);
	/* note: change to drawline*/
	GdLine(psd, points[0].x, points[0].y, points[1].x, points[1].y, TRUE);
	points++;
	didline = TRUE;
  }
  if (gr_mode == MWMODE_XOR) {
	  points--;
	  if (points->x == firstx && points->y == firsty)
		drawpoint(psd, points->x, points->y);
  }
  GdFixCursor(psd);
}

#if 1 /* improved convex polygon fill routine*/
/***********************************************************
Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/

/*
 *     Written by Brian Kelleher; Dec. 1985.
 *     Adapted for Microwindows Sep 2001 by Greg Haerr <greg@censoft.com>
 *
 *     Fill a convex polygon in the fg color, with clipping.
 *     If the given polygon
 *     is not convex, then the result is undefined.
 *     The algorithm is to order the edges from smallest
 *     y to largest by partitioning the array into a left
 *     edge list and a right edge list.  The algorithm used
 *     to traverse each edge is an extension of Bresenham's
 *     line algorithm with y as the major axis.
 *
 *     This file contains a few macros to help track
 *     the edge of a filled object.  The object is assumed
 *     to be filled in scanline order, and thus the
 *     algorithm used is an extension of Bresenham's line
 *     drawing algorithm which assumes that y is always the
 *     major axis.
 *
 *  In scan converting polygons, we want to choose those pixels
 *  which are inside the polygon.  Thus, we add .5 to the starting
 *  x coordinate for both left and right edges.  Now we choose the
 *  first pixel which is inside the pgon for the left edge and the
 *  first pixel which is outside the pgon for the right edge.
 *  Draw the left pixel, but not the right.
 *
 *  How to add .5 to the starting x coordinate:
 *      If the edge is moving to the right, then subtract dy from the
 *  error term from the general form of the algorithm.
 *      If the edge is moving to the left, then add dy to the error term.
 *
 *  The reason for the difference between edges moving to the left
 *  and edges moving to the right is simple:  If an edge is moving
 *  to the right, then we want the algorithm to flip immediately.
 *  If it is moving to the left, then we don't want it to flip until
 *  we traverse an entire pixel.
 */
#define BRESINITPGON(dy, x1, x2, xStart, d, m, m1, incr1, incr2) { \
    int dx;      /* local storage */ \
\
    /* \
     *  if the edge is horizontal, then it is ignored \
     *  and assumed not to be processed.  Otherwise, do this stuff. \
     */ \
    if ((dy) != 0) { \
        xStart = (x1); \
        dx = (x2) - xStart; \
        if (dx < 0) { \
            m = dx / (dy); \
            m1 = m - 1; \
            incr1 = -2 * dx + 2 * (dy) * m1; \
            incr2 = -2 * dx + 2 * (dy) * m; \
            d = 2 * m * (dy) - 2 * dx - 2 * (dy); \
        } else { \
            m = dx / (dy); \
            m1 = m + 1; \
            incr1 = 2 * dx - 2 * (dy) * m1; \
            incr2 = 2 * dx - 2 * (dy) * m; \
            d = -2 * m * (dy) + 2 * dx; \
        } \
    } \
}

#define BRESINCRPGON(d, minval, m, m1, incr1, incr2) { \
    if (m1 > 0) { \
        if (d > 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } else {\
        if (d >= 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } \
}

/*
 *     Find the index of the point with the smallest y.
 */
static int
getPolyYBounds(MWPOINT *pts, int n, int *by, int *ty)
{
    MWPOINT *ptMin;
    int ymin, ymax;
    MWPOINT *ptsStart = pts;

    ptMin = pts;
    ymin = ymax = (pts++)->y;

    while (--n > 0) {
        if (pts->y < ymin)
	{
            ptMin = pts;
            ymin = pts->y;
        }
	if(pts->y > ymax)
            ymax = pts->y;

        pts++;
    }

    *by = ymin;
    *ty = ymax;
    return(ptMin-ptsStart);
}

void
GdFillPoly(PSD psd, int count, MWPOINT *pointtable)
{
    MWCOORD xl = 0, xr = 0;     /* x vals of left and right edges */
    int dl = 0, dr = 0;         /* decision variables             */
    int ml = 0, m1l = 0;        /* left edge slope and slope+1    */
    int mr = 0, m1r = 0;        /* right edge slope and slope+1   */
    int incr1l = 0, incr2l = 0; /* left edge error increments     */
    int incr1r = 0, incr2r = 0; /* right edge error increments    */
    int dy;                     /* delta y                        */
    MWCOORD y;                  /* current scanline               */
    int left, right;            /* indices to first endpoints     */
    int i;                      /* loop counter                   */
    int nextleft, nextright;    /* indices to second endpoints    */
    MWPOINT *ptsOut, *FirstPoint;/* output buffer                 */
    MWCOORD *width, *FirstWidth;/* output buffer                  */
    int imin;                   /* index of smallest vertex (in y)*/
    int ymin;                   /* y-extents of polygon           */
    int ymax;

    /*
     *  find leftx, bottomy, rightx, topy, and the index
     *  of bottomy.
     */
    imin = getPolyYBounds(pointtable, count, &ymin, &ymax);

    dy = ymax - ymin + 1;
    if ((count < 3) || (dy < 0))
	return;
    ptsOut = FirstPoint = (MWPOINT *)ALLOCA(sizeof(MWPOINT) * dy);
    width = FirstWidth = (MWCOORD *)ALLOCA(sizeof(MWCOORD) * dy);
    if(!FirstPoint || !FirstWidth)
    {
	if (FirstWidth) FREEA(FirstWidth);
	if (FirstPoint) FREEA(FirstPoint);
	return;
    }

    nextleft = nextright = imin;
    y = pointtable[nextleft].y;

    /*
     *  loop through all edges of the polygon
     */
    do {
        /*
         *  add a left edge if we need to
         */
        if (pointtable[nextleft].y == y) {
            left = nextleft;

            /*
             *  find the next edge, considering the end
             *  conditions of the array.
             */
            nextleft++;
            if (nextleft >= count)
                nextleft = 0;

            /*
             *  now compute all of the random information
             *  needed to run the iterative algorithm.
             */
            BRESINITPGON(pointtable[nextleft].y-pointtable[left].y,
                         pointtable[left].x,pointtable[nextleft].x,
                         xl, dl, ml, m1l, incr1l, incr2l);
        }

        /*
         *  add a right edge if we need to
         */
        if (pointtable[nextright].y == y) {
            right = nextright;

            /*
             *  find the next edge, considering the end
             *  conditions of the array.
             */
            nextright--;
            if (nextright < 0)
                nextright = count-1;

            /*
             *  now compute all of the random information
             *  needed to run the iterative algorithm.
             */
            BRESINITPGON(pointtable[nextright].y-pointtable[right].y,
                         pointtable[right].x,pointtable[nextright].x,
                         xr, dr, mr, m1r, incr1r, incr2r);
        }

        /*
         *  generate scans to fill while we still have
         *  a right edge as well as a left edge.
         */
        i = MWMIN(pointtable[nextleft].y, pointtable[nextright].y) - y;
	/* in case we're called with non-convex polygon */
	if(i < 0)
        {
	    FREEA(FirstWidth);
	    FREEA(FirstPoint);
	    return;
	}
        while (i-- > 0) 
        {
            ptsOut->y = y;

            /*
             *  reverse the edges if necessary
             */
            if (xl < xr) 
            {
                *(width++) = xr - xl;
                (ptsOut++)->x = xl;
            }
            else 
            {
                *(width++) = xl - xr;
                (ptsOut++)->x = xr;
            }
            y++;

            /* increment down the edges */
            BRESINCRPGON(dl, xl, ml, m1l, incr1l, incr2l);
            BRESINCRPGON(dr, xr, mr, m1r, incr1r, incr2r);
        }
    }  while (y != ymax);

    /*
     * Finally, fill the spans
     */
    i = ptsOut-FirstPoint;
    ptsOut = FirstPoint;
    width = FirstWidth;
    while (--i >= 0) {
	/* calc x extent from width*/
	int e = *width++ - 1;
	if (e >= 0) {
    	    drawrow(psd, ptsOut->x, ptsOut->x + e, ptsOut->y);
	}
	++ptsOut;
    }

    FREEA(FirstWidth);
    FREEA(FirstPoint);
    GdFixCursor(psd);
}
#endif

#if 0 /* original convex only polygon fill routine*/
/*
 * Fill a polygon in the foreground color, applying clipping if necessary.
 * The last point may be a duplicate of the first point, but this is
 * not required.
 * Note: this routine currently only correctly fills convex polygons.
 */

/* Utility routine for filling polygons.  Find the intersection point (if
 * any) of a horizontal line with an arbitrary line, and extend the current
 * minimum and maximum x values as needed to include the intersection point.
 * Input parms:
 *	y 	row to check for intersection
 *	x1, y1	first endpoint
 *	x2, y2	second enpoint
 *	minxptr	address of current minimum x
 *	maxxptr	address of current maximum x
 */
static void
extendrow(MWCOORD y,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
	MWCOORD *minxptr,MWCOORD *maxxptr)
{
  MWCOORD x;			/* x coordinate of intersection */
  typedef long NUM;
  NUM num;			/* numerator of fraction */

  /* First make sure the specified line segment includes the specified
   * row number.  If not, then there is no intersection.
   */
  if (((y < y1) || (y > y2)) && ((y < y2) || (y > y1)))
	return;

  /* If a horizontal line, then check the two endpoints. */
  if (y1 == y2) {
	if (*minxptr > x1) *minxptr = x1;
	if (*minxptr > x2) *minxptr = x2;
	if (*maxxptr < x1) *maxxptr = x1;
	if (*maxxptr < x2) *maxxptr = x2;
	return;
  }

  /* If a vertical line, then check the x coordinate. */
  if (x1 == x2) {
	if (*minxptr > x1) *minxptr = x1;
	if (*maxxptr < x1) *maxxptr = x1;
	return;
  }

  /* An arbitrary line.  Calculate the intersection point using the
   * formula x = x1 + (y - y1) * (x2 - x1) / (y2 - y1).
   */
  num = ((NUM) (y - y1)) * (x2 - x1);
  x = x1 + num / (y2 - y1);
  if (*minxptr > x) *minxptr = x;
  if (*maxxptr < x) *maxxptr = x;
}

void
GdFillPoly(PSD psd, int count, MWPOINT *points)
{
  MWPOINT *pp;		/* current point */
  MWCOORD miny;		/* minimum row */
  MWCOORD maxy;		/* maximum row */
  MWCOORD minx;		/* minimum column */
  MWCOORD maxx;		/* maximum column */
  int i;		/* counter */

  if (count <= 0)
	  return;

  /* First determine the minimum and maximum rows for the polygon. */
  pp = points;
  miny = pp->y;
  maxy = pp->y;
  for (i = count; i-- > 0; pp++) {
	if (miny > pp->y) miny = pp->y;
	if (maxy < pp->y) maxy = pp->y;
  }
  if (miny < 0)
	  miny = 0;
  if (maxy >= psd->yvirtres)
	  maxy = psd->yvirtres - 1;
  if (miny > maxy)
	  return;

  /* Now for each row, scan the list of points and determine the
   * minimum and maximum x coordinate for each line, and plot the row.
   * The last point connects with the first point automatically.
   */
  for (; miny <= maxy; miny++) {
	minx = MAX_MWCOORD;
	maxx = MIN_MWCOORD;
	pp = points;
	for (i = count; --i > 0; pp++)
		extendrow(miny, pp[0].x, pp[0].y, pp[1].x, pp[1].y,
			&minx, &maxx);
	extendrow(miny, pp[0].x, pp[0].y, points[0].x, points[0].y,
		&minx, &maxx);

	if (minx <= maxx)
		drawrow(psd, minx, maxx, miny);
  }
  GdFixCursor(psd);
}
#endif

#if 0	/* irregular polygon fill, uses edge table, malloc, qsort*/
/*
 * Fill a polygon in the foreground color, applying clipping if necessary.
 * The last point may be a duplicate of the first point, but this is
 * not required.
 * Note: this routine correctly draws convex, concave, regular, 
 * and irregular polygons.
 */
#define USE_FLOAT	HAVEFLOAT	/* set to use floating point*/

#define swap(a,b) do { a ^= b; b ^= a; a ^= b; } while (0)

typedef struct {
	int     x1, y1, x2, y2;
#if USE_FLOAT
	double  x, m;
#else
	int     cx, fn, mn, d;
#endif
} edge_t;

static int 
edge_cmp(const void *lvp, const void *rvp)
{
	/* convert from void pointers to structure pointers */
	const edge_t *lp = (const edge_t *)lvp;
	const edge_t *rp = (const edge_t *)rvp;

	/* if the minimum y values are different, sort on minimum y */
	if (lp->y1 != rp->y1)
		return lp->y1 - rp->y1;

	/* if the current x values are different, sort on current x */
#if USE_FLOAT
	if (lp->x < rp->x)
		return -1;
	else if (lp->x > rp->x)
		return +1;
#else
	if (lp->cx != rp->cx)
		return lp->cx - rp->cx;
#endif

	/* otherwise they are equal */
	return 0;
}

void
GdFillPoly(PSD psd, int count, MWPOINT * pointtable)
{
	edge_t *get;		/* global edge table */
	int     nge = 0;	/* num global edges */
	int     cge = 0;	/* cur global edge */

	edge_t *aet;		/* active edge table */
	int     nae = 0;	/* num active edges */

	int     i, y;

	if (count < 3) {
		/* error, polygons require at least three edges (a triangle) */
		return;
	}
	get = (edge_t *) calloc(count, sizeof(edge_t));
	aet = (edge_t *) calloc(count, sizeof(edge_t));

	if ((get == 0) || (aet == 0)) {
		/* error, couldn't allocate one or both of the needed tables */
		if (get)
			free(get);
		if (aet)
			free(aet);
		return;
	}
	/* setup the global edge table */
	for (i = 0; i < count; ++i) {
		get[nge].x1 = pointtable[i].x;
		get[nge].y1 = pointtable[i].y;
		get[nge].x2 = pointtable[(i + 1) % count].x;
		get[nge].y2 = pointtable[(i + 1) % count].y;
		if (get[nge].y1 != get[nge].y2) {
			if (get[nge].y1 > get[nge].y2) {
				swap(get[nge].x1, get[nge].x2);
				swap(get[nge].y1, get[nge].y2);
			}
#if USE_FLOAT
			get[nge].x = get[nge].x1;
			get[nge].m = get[nge].x2 - get[nge].x1;
			get[nge].m /= get[nge].y2 - get[nge].y1;
#else
			get[nge].cx = get[nge].x1;
			get[nge].mn = get[nge].x2 - get[nge].x1;
			get[nge].d = get[nge].y2 - get[nge].y1;
			get[nge].fn = get[nge].mn / 2;
#endif
			++nge;
		}
	}

	qsort(get, nge, sizeof(get[0]), edge_cmp);

	/* start with the lowest y in the table */
	y = get[0].y1;

	do {

		/* add edges to the active table from the global table */
		while ((nge > 0) && (get[cge].y1 == y)) {
			aet[nae] = get[cge++];
			--nge;
			aet[nae++].y1 = 0;
		}

		qsort(aet, nae, sizeof(aet[0]), edge_cmp);

		/* using odd parity, render alternating line segments */
		for (i = 1; i < nae; i += 2) {
#if USE_FLOAT
			int     l = (int)aet[i - 1].x;
			int     r = (int)aet[i].x;
#else
			int     l = (int)aet[i - 1].cx;
			int     r = (int)aet[i].cx;
#endif
			if (r > l)
				drawrow(psd, l, r - 1, y);
		}

		/* prepare for the next scan line */
		++y;

		/* remove inactive edges from the active edge table */
		/* or update the current x position of active edges */
		for (i = 0; i < nae; ++i) {
			if (aet[i].y2 == y)
				aet[i--] = aet[--nae];
			else {
#if USE_FLOAT
				aet[i].x += aet[i].m;
#else
				aet[i].fn += aet[i].mn;
				if (aet[i].fn < 0) {
					aet[i].cx += aet[i].fn / aet[i].d - 1;
					aet[i].fn %= aet[i].d;
					aet[i].fn += aet[i].d;
				}
				if (aet[i].fn >= aet[i].d) {
					aet[i].cx += aet[i].fn / aet[i].d;
					aet[i].fn %= aet[i].d;
				}
#endif
			}
		}

		/* keep doing this while there are any edges left */
	} while ((nae > 0) || (nge > 0));

	/* all done, free the edge tables */
	free(get);
	free(aet);

	GdFixCursor(psd);
}
#endif
