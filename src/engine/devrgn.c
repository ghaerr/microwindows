/*
 * Portions Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *	Somewhat less shamelessly ripped from the Wine distribution
 *
 * Device-independent multi-rectangle clipping routines.
 *
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice licence.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 */
/************************************************************************

Copyright (c) 1987, 1988  X Consortium

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


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

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

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"

typedef void (*voidProcp)();

/*  1 if two RECTs overlap.
 *  0 if two RECTs do not overlap.
 */
#define EXTENTCHECK(r1, r2) \
	((r1)->right > (r2)->left && \
	 (r1)->left < (r2)->right && \
	 (r1)->bottom > (r2)->top && \
	 (r1)->top < (r2)->bottom)

/*
 *   Check to see if there is enough memory in the present region.
 */
#define MEMCHECK(reg, rect, firstrect){\
        if ((reg)->numRects >= ((reg)->size - 1)){\
          (firstrect) = realloc(\
           (firstrect), (2 * (sizeof(MWRECT)) * ((reg)->size)));\
          if ((firstrect) == 0)\
            return;\
          (reg)->size *= 2;\
          (rect) = &(firstrect)[(reg)->numRects];\
         }\
       }

#define REGION_NOT_EMPTY(pReg) pReg->numRects

#define EMPTY_REGION(pReg) { \
    (pReg)->numRects = 0; \
    (pReg)->extents.left = (pReg)->extents.top = 0; \
    (pReg)->extents.right = (pReg)->extents.bottom = 0; \
    (pReg)->type = MWREGION_NULL; \
 }

#define INRECT(r, x, y) \
      ( ( ((r).right >  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >  y)) && \
        ( ((r).top <= y)) )

/* return TRUE if point is in region*/
MWBOOL
GdPtInRegion(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y)
{
    int i;

    if (rgn->numRects > 0 && INRECT(rgn->extents, x, y))
	for (i = 0; i < rgn->numRects; i++)
	    if (INRECT (rgn->rects[i], x, y))
		return TRUE;
    return FALSE;
}

/* return whether rectangle is all in, partly in, or out of region*/
int 
GdRectInRegion(MWCLIPREGION *rgn, const MWRECT *rect)
{
    MWRECT *	pCurRect;
    MWRECT *	pRectEnd;
    MWCOORD	rx, ry;
    MWBOOL	partIn, partOut;

    /* this is (just) a useful optimization */
    if (!rgn->numRects || !EXTENTCHECK(&rgn->extents, rect))
        return MWRECT_OUT;

    partOut = FALSE;
    partIn = FALSE;
    rx = rect->left;
    ry = rect->top;

    /* 
     * can stop when both partOut and partIn are TRUE,
     * or we reach rect->bottom
     */
    for (pCurRect = rgn->rects, pRectEnd = pCurRect + rgn->numRects;
		 pCurRect < pRectEnd; pCurRect++) {

	if (pCurRect->bottom <= ry)
	   continue;		/* not far enough down yet*/

	if (pCurRect->top > ry) {
	   partOut = TRUE;	/* missed part of rectangle above */
	   if (partIn || (pCurRect->top >= rect->bottom))
	      break;
	   ry = pCurRect->top;	/* x guaranteed to be == rect->left */
	}

	if (pCurRect->right <= rx)
	   continue;		/* not far enough over yet */

	if (pCurRect->left > rx) {
	   partOut = TRUE;	/* missed part of rectangle to left */
	   if (partIn)
	      break;
	}

	if (pCurRect->left < rect->right) {
	    partIn = TRUE;	/* definitely overlap */
	    if (partOut)
	       break;
	}

	if (pCurRect->right >= rect->right) {
	   ry = pCurRect->bottom;	/* finished with this band */
	   if (ry >= rect->bottom)
	      break;
	   rx = rect->left;	/* reset x out to left again */
	} else {
	    /*
	     * Because boxes in a band are maximal width, if the first box
	     * to overlap the rectangle doesn't completely cover it in that
	     * band, the rectangle must be partially out, since some of it
	     * will be uncovered in that band. partIn will have been set true
	     * by now...
	     */
	    break;
	}
    }

    return(partIn ? ((ry < rect->bottom) ? MWRECT_PARTIN : MWRECT_ALLIN) : 
		MWRECT_OUT);
}

#if 0000
/* Returns TRUE if rect is at least partly inside rgn*/
MWBOOL
GdRectInRegion(MWCLIPREGION *rgn, const MWRECT *rect)
{
    MWRECT *pCurRect, *pRectEnd;
    MWBOOL ret = FALSE;

    /* this is (just) a useful optimization */
	if ((rgn->numRects > 0) && EXTENTCHECK(&rgn->extents, rect))
	{
	    for (pCurRect = rgn->rects, pRectEnd = pCurRect +
	     rgn->numRects; pCurRect < pRectEnd; pCurRect++)
	    {
	        if (pCurRect->bottom <= rect->top)
			continue;             /* not far enough down yet */

		if (pCurRect->top >= rect->bottom) {
		    ret = FALSE;          /* too far down */
		    break;
		}

		if (pCurRect->right <= rect->left)
		    continue;              /* not far enough over yet */

		if (pCurRect->left >= rect->right) {
		    continue;
		}

		ret = TRUE;
		break;
	    }
	}
	return ret;
}
#endif

static MWBOOL
EQUALRECT(MWRECT *r1, MWRECT *r2)
{
	return ((r1->left == r2->left) && (r1->right == r2->right) &&
		(r1->top == r2->top) && (r1->bottom == r2->bottom));
}

MWBOOL
GdEqualRegion(MWCLIPREGION *r1, MWCLIPREGION *r2)
{
	int	i;

	if (r1->numRects != r2->numRects)
		return FALSE;
	if (r1->numRects == 0)
		return TRUE;
	if (!EQUALRECT(&r1->extents, &r2->extents))
		return FALSE;
	for (i = 0; i < r1->numRects; i++) {
		if (!EQUALRECT(r1->rects + i, r2->rects + i))
			return FALSE;
	}
	return TRUE;
}

MWBOOL
GdEmptyRegion(MWCLIPREGION *rgn)
{
	return rgn->numRects == 0;
}

/*
 *            Create a new empty MWCLIPREGION.
 */
MWCLIPREGION *
GdAllocRegion(void)    
{
    MWCLIPREGION *rgn;

    if ((rgn = malloc(sizeof( MWCLIPREGION ))))
    {
	if ((rgn->rects = malloc(sizeof( MWRECT ))))
	{
	    rgn->size = 1;
	    EMPTY_REGION(rgn);
	    return rgn;
	}
	free(rgn);
    }
    return NULL;
}

MWCLIPREGION *
GdAllocRectRegion(MWCOORD left, MWCOORD top, MWCOORD right, MWCOORD bottom)
{
	MWCLIPREGION *rgn;

	rgn = GdAllocRegion();
	if (rgn)
		GdSetRectRegion(rgn, left, top, right, bottom);
	return rgn;
}

MWCLIPREGION *
GdAllocRectRegionIndirect(MWRECT *prc)
{
	return GdAllocRectRegion(prc->left, prc->top, prc->right, prc->bottom);
}

void
GdSetRectRegion(MWCLIPREGION *rgn, MWCOORD left, MWCOORD top, MWCOORD right,
	MWCOORD bottom)
{
	if (left != right && top != bottom) {
		rgn->rects->left = rgn->extents.left = left;
		rgn->rects->top = rgn->extents.top = top;
		rgn->rects->right = rgn->extents.right = right;
		rgn->rects->bottom = rgn->extents.bottom = bottom;
		rgn->numRects = 1;
		rgn->type = MWREGION_SIMPLE;
	} else
		EMPTY_REGION(rgn);
}

void
GdSetRectRegionIndirect(MWCLIPREGION *rgn, MWRECT *prc)
{
	GdSetRectRegion(rgn, prc->left, prc->top, prc->right, prc->bottom);
}

void
GdDestroyRegion(MWCLIPREGION *rgn)
{
	if(rgn) {
		free(rgn->rects);
		free(rgn);
	}
}

void
GdOffsetRegion(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y)
{
	int	nbox = rgn->numRects;
	MWRECT *pbox = rgn->rects;

	if(nbox && (x || y)) {
		while(nbox--) {
			pbox->left += x;
			pbox->right += x;
			pbox->top += y;
			pbox->bottom += y;
			pbox++;
		}
		rgn->extents.left += x;
		rgn->extents.right += x;
		rgn->extents.top += y;
		rgn->extents.bottom += y;
	}
}

/* get bounding box for region, return region type*/
int
GdGetRegionBox(MWCLIPREGION *rgn, MWRECT *prc)
{
	*prc = rgn->extents;
	return rgn->type;
}

/***********************************************************************
 *           GdUnionRectWithRegion
 *           Adds a rectangle to a MWCLIPREGION
 */
void
GdUnionRectWithRegion(const MWRECT *rect, MWCLIPREGION *rgn)
{
    MWCLIPREGION region;

    region.rects = &region.extents;
    region.numRects = 1;
    region.size = 1;
    region.type = MWREGION_SIMPLE;
    region.extents = *rect;
    GdUnionRegion(rgn, rgn, &region);
}

/***********************************************************************
 *           GdSubtractRectWithRegion
 *           Subtracts a rectangle from a MWCLIPREGION
 */
void
GdSubtractRectFromRegion(const MWRECT *rect, MWCLIPREGION *rgn)
{
    MWCLIPREGION region;

    region.rects = &region.extents;
    region.numRects = 1;
    region.size = 1;
    region.type = MWREGION_SIMPLE;
    region.extents = *rect;
    GdSubtractRegion(rgn, rgn, &region);
}


/***********************************************************************
 *           GdCopyRegion
 */
void
GdCopyRegion(MWCLIPREGION *dst, MWCLIPREGION *src)
{
    if (dst != src) /*  don't want to copy to itself */
    {  
	if (dst->size < src->numRects)
	{
	    if (! (dst->rects = realloc( dst->rects, src->numRects * sizeof(MWRECT))))
		return;
	    dst->size = src->numRects;
	}
	dst->numRects = src->numRects;
	dst->extents.left = src->extents.left;
	dst->extents.top = src->extents.top;
	dst->extents.right = src->extents.right;
	dst->extents.bottom = src->extents.bottom;
	dst->type = src->type;

	memcpy((char *) dst->rects, (char *) src->rects,
	       (int) (src->numRects * sizeof(MWRECT)));
    }
}


/***********************************************************************
 *           REGION_SetExtents
 *           Re-calculate the extents of a region
 */
static void
REGION_SetExtents (MWCLIPREGION *pReg)
{
    MWRECT *pRect, *pRectEnd, *pExtents;

    if (pReg->numRects == 0)
    {
	pReg->extents.left = 0;
	pReg->extents.top = 0;
	pReg->extents.right = 0;
	pReg->extents.bottom = 0;
	return;
    }

    pExtents = &pReg->extents;
    pRect = pReg->rects;
    pRectEnd = &pRect[pReg->numRects - 1];

    /*
     * Since pRect is the first rectangle in the region, it must have the
     * smallest top and since pRectEnd is the last rectangle in the region,
     * it must have the largest bottom, because of banding. Initialize left and
     * right from pRect and pRectEnd, resp., as good things to initialize them
     * to...
     */
    pExtents->left = pRect->left;
    pExtents->top = pRect->top;
    pExtents->right = pRectEnd->right;
    pExtents->bottom = pRectEnd->bottom;

    while (pRect <= pRectEnd)
    {
	if (pRect->left < pExtents->left)
	    pExtents->left = pRect->left;
	if (pRect->right > pExtents->right)
	    pExtents->right = pRect->right;
	pRect++;
    }
}


/***********************************************************************
 *           REGION_Coalesce
 *
 *      Attempt to merge the rects in the current band with those in the
 *      previous one. Used only by REGION_RegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their bottom fields
 *            altered.
 *          - pReg->numRects will be decreased.
 *
 */
static MWCOORD
REGION_Coalesce (
	     MWCLIPREGION *pReg, /* Region to coalesce */
	     MWCOORD prevStart,  /* Index of start of previous band */
	     MWCOORD curStart    /* Index of start of current band */
) {
    MWRECT *pPrevRect;          /* Current rect in previous band */
    MWRECT *pCurRect;           /* Current rect in current band */
    MWRECT *pRegEnd;            /* End of region */
    MWCOORD curNumRects;          /* Number of rectangles in current band */
    MWCOORD prevNumRects;         /* Number of rectangles in previous band */
    MWCOORD bandtop;               /* top coordinate for current band */

    pRegEnd = &pReg->rects[pReg->numRects];

    pPrevRect = &pReg->rects[prevStart];
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in REGION_RegionOp
     * at the end when one region has been exhausted.
     */
    pCurRect = &pReg->rects[curStart];
    bandtop = pCurRect->top;
    for (curNumRects = 0;
	 (pCurRect != pRegEnd) && (pCurRect->top == bandtop);
	 curNumRects++)
    {
	pCurRect++;
    }
    
    if (pCurRect != pRegEnd)
    {
	/*
	 * If more than one band was added, we have to find the start
	 * of the last band added so the next coalescing job can start
	 * at the right place... (given when multiple bands are added,
	 * this may be pointless -- see above).
	 */
	pRegEnd--;
	while (pRegEnd[-1].top == pRegEnd->top)
	{
	    pRegEnd--;
	}
	curStart = pRegEnd - pReg->rects;
	pRegEnd = pReg->rects + pReg->numRects;
    }
	
    if ((curNumRects == prevNumRects) && (curNumRects != 0)) {
	pCurRect -= curNumRects;
	/*
	 * The bands may only be coalesced if the bottom of the previous
	 * matches the top scanline of the current.
	 */
	if (pPrevRect->bottom == pCurRect->top)
	{
	    /*
	     * Make sure the bands have rects in the same places. This
	     * assumes that rects have been added in such a way that they
	     * cover the most area possible. I.e. two rects in a band must
	     * have some horizontal space between them.
	     */
	    do
	    {
		if ((pPrevRect->left != pCurRect->left) ||
		    (pPrevRect->right != pCurRect->right))
		{
		    /*
		     * The bands don't line up so they can't be coalesced.
		     */
		    return (curStart);
		}
		pPrevRect++;
		pCurRect++;
		prevNumRects -= 1;
	    } while (prevNumRects != 0);

	    pReg->numRects -= curNumRects;
	    pCurRect -= curNumRects;
	    pPrevRect -= curNumRects;

	    /*
	     * The bands may be merged, so set the bottom of each rect
	     * in the previous band to that of the corresponding rect in
	     * the current band.
	     */
	    do
	    {
		pPrevRect->bottom = pCurRect->bottom;
		pPrevRect++;
		pCurRect++;
		curNumRects -= 1;
	    } while (curNumRects != 0);

	    /*
	     * If only one band was added to the region, we have to backup
	     * curStart to the start of the previous band.
	     *
	     * If more than one band was added to the region, copy the
	     * other bands down. The assumption here is that the other bands
	     * came from the same region as the current one and no further
	     * coalescing can be done on them since it's all been done
	     * already... curStart is already in the right place.
	     */
	    if (pCurRect == pRegEnd)
	    {
		curStart = prevStart;
	    }
	    else
	    {
		do
		{
		    *pPrevRect++ = *pCurRect++;
		} while (pCurRect != pRegEnd);
	    }
	    
	}
    }
    return (curStart);
}

/***********************************************************************
 *           REGION_RegionOp
 *
 *      Apply an operation to two regions. Called by GdUnion,
 *      GdXor, GdSubtract, GdIntersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 * Notes:
 *      The idea behind this function is to view the two regions as sets.
 *      Together they cover a rectangle of area that this function divides
 *      into horizontal bands where points are covered only by one region
 *      or by both. For the first case, the nonOverlapFunc is called with
 *      each the band and the band's upper and lower extents. For the
 *      second, the overlapFunc is called to process the entire band. It
 *      is responsible for clipping the rectangles in the band, though
 *      this function provides the boundaries.
 *      At the end of each band, the new region is coalesced, if possible,
 *      to reduce the number of rectangles in the region.
 *
 */
static void
REGION_RegionOp(
	    MWCLIPREGION *newReg, /* Place to store result */
	    MWCLIPREGION *reg1,   /* First region in operation */
            MWCLIPREGION *reg2,   /* 2nd region in operation */
	    void (*overlapFunc)(),     /* Function to call for over-lapping bands */
	    void (*nonOverlap1Func)(), /* Function to call for non-overlapping bands in region 1 */
	    void (*nonOverlap2Func)()  /* Function to call for non-overlapping bands in region 2 */
) {
    MWRECT *r1;                         /* Pointer into first region */
    MWRECT *r2;                         /* Pointer into 2d region */
    MWRECT *r1End;                      /* End of 1st region */
    MWRECT *r2End;                      /* End of 2d region */
    MWCOORD ybot;                         /* Bottom of intersection */
    MWCOORD ytop;                         /* Top of intersection */
    MWRECT *oldRects;                   /* Old rects for newReg */
    MWCOORD prevBand;                     /* Index of start of
						 * previous band in newReg */
    MWCOORD curBand;                      /* Index of start of current
						 * band in newReg */
    MWRECT *r1BandEnd;                  /* End of current band in r1 */
    MWRECT *r2BandEnd;                  /* End of current band in r2 */
    MWCOORD top;                          /* Top of non-overlapping band */
    MWCOORD bot;                          /* Bottom of non-overlapping band */
    
    /*
     * Initialization:
     *  set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = reg1->rects;
    r2 = reg2->rects;
    r1End = r1 + reg1->numRects;
    r2End = r2 + reg2->numRects;
    

    /*
     * newReg may be one of the src regions so we can't empty it. We keep a 
     * note of its rects pointer (so that we can free them later), preserve its
     * extents and simply set numRects to zero. 
     */

    oldRects = newReg->rects;
    newReg->numRects = 0;

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually.
     */
    newReg->size = MWMAX(reg1->numRects,reg2->numRects) * 2;

    if (! (newReg->rects = malloc( sizeof(MWRECT) * newReg->size )))
    {
	newReg->size = 0;
	return;
    }
    
    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->extents.top < reg2->extents.top)
	ybot = reg1->extents.top;
    else
	ybot = reg2->extents.top;
    
    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;
    
    do
    {
	curBand = newReg->numRects;

	/*
	 * This algorithm proceeds one source-band (as opposed to a
	 * destination band, which is determined by where the two regions
	 * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
	 * rectangle after the last one in the current band for their
	 * respective regions.
	 */
	r1BandEnd = r1;
	while ((r1BandEnd != r1End) && (r1BandEnd->top == r1->top))
	{
	    r1BandEnd++;
	}
	
	r2BandEnd = r2;
	while ((r2BandEnd != r2End) && (r2BandEnd->top == r2->top))
	{
	    r2BandEnd++;
	}
	
	/*
	 * First handle the band that doesn't intersect, if any.
	 *
	 * Note that attention is restricted to one band in the
	 * non-intersecting region at once, so if a region has n
	 * bands between the current position and the next place it overlaps
	 * the other, this entire loop will be passed through n times.
	 */
	if (r1->top < r2->top)
	{
	    top = MWMAX(r1->top,ybot);
	    bot = MWMIN(r1->bottom,r2->top);

	    if ((top != bot) && (nonOverlap1Func != (void (*)())NULL))
	    {
		(* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
	    }

	    ytop = r2->top;
	}
	else if (r2->top < r1->top)
	{
	    top = MWMAX(r2->top,ybot);
	    bot = MWMIN(r2->bottom,r1->top);

	    if ((top != bot) && (nonOverlap2Func != (void (*)())NULL))
	    {
		(* nonOverlap2Func) (newReg, r2, r2BandEnd, top, bot);
	    }

	    ytop = r1->top;
	}
	else
	{
	    ytop = r1->top;
	}

	/*
	 * If any rectangles got added to the region, try and coalesce them
	 * with rectangles from the previous band. Note we could just do
	 * this test in miCoalesce, but some machines incur a not
	 * inconsiderable cost for function calls, so...
	 */
	if (newReg->numRects != curBand)
	{
	    prevBand = REGION_Coalesce (newReg, prevBand, curBand);
	}

	/*
	 * Now see if we've hit an intersecting band. The two bands only
	 * intersect if ybot > ytop
	 */
	ybot = MWMIN(r1->bottom, r2->bottom);
	curBand = newReg->numRects;
	if (ybot > ytop)
	{
	    (* overlapFunc) (newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

	}
	
	if (newReg->numRects != curBand)
	{
	    prevBand = REGION_Coalesce (newReg, prevBand, curBand);
	}

	/*
	 * If we've finished with a band (bottom == ybot) we skip forward
	 * in the region to the next band.
	 */
	if (r1->bottom == ybot)
	{
	    r1 = r1BandEnd;
	}
	if (r2->bottom == ybot)
	{
	    r2 = r2BandEnd;
	}
    } while ((r1 != r1End) && (r2 != r2End));

    /*
     * Deal with whichever region still has rectangles left.
     */
    curBand = newReg->numRects;
    if (r1 != r1End)
    {
	if (nonOverlap1Func != (void (*)())NULL)
	{
	    do
	    {
		r1BandEnd = r1;
		while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top))
		{
		    r1BandEnd++;
		}
		(* nonOverlap1Func) (newReg, r1, r1BandEnd,
				     MWMAX(r1->top,ybot), r1->bottom);
		r1 = r1BandEnd;
	    } while (r1 != r1End);
	}
    }
    else if ((r2 != r2End) && (nonOverlap2Func != (void (*)())NULL))
    {
	do
	{
	    r2BandEnd = r2;
	    while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top))
	    {
		 r2BandEnd++;
	    }
	    (* nonOverlap2Func) (newReg, r2, r2BandEnd,
				MWMAX(r2->top,ybot), r2->bottom);
	    r2 = r2BandEnd;
	} while (r2 != r2End);
    }

    if (newReg->numRects != curBand)
    {
	(void) REGION_Coalesce (newReg, prevBand, curBand);
    }

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...).
     */
    if (newReg->numRects < (newReg->size >> 1))
    {
	if (REGION_NOT_EMPTY(newReg))
	{
	    MWRECT *prev_rects = newReg->rects;
	    newReg->size = newReg->numRects;
	    newReg->rects = realloc( newReg->rects, sizeof(MWRECT) * newReg->size );
	    if (! newReg->rects)
		newReg->rects = prev_rects;
	}
	else
	{
	    /*
	     * No point in doing the extra work involved in an Xrealloc if
	     * the region is empty
	     */
	    newReg->size = 1;
	    free( newReg->rects );
	    newReg->rects = malloc( sizeof(MWRECT) );
	}
    }
    free( oldRects );
}

/***********************************************************************
 *          Region Intersection
 ***********************************************************************/


/***********************************************************************
 *	     REGION_IntersectO
 *
 * Handle an overlapping band for REGION_Intersect.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles may be added to the region.
 *
 */
static void
REGION_IntersectO(MWCLIPREGION *pReg,  MWRECT *r1, MWRECT *r1End,
			MWRECT *r2, MWRECT *r2End, MWCOORD top, MWCOORD bottom)

{
    MWCOORD       left, right;
    MWRECT      *pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
	left = MWMAX(r1->left, r2->left);
	right =	MWMIN(r1->right, r2->right);

	/*
	 * If there's any overlap between the two rectangles, add that
	 * overlap to the new region.
	 * There's no need to check for subsumption because the only way
	 * such a need could arise is if some region has two rectangles
	 * right next to each other. Since that should never happen...
	 */
	if (left < right)
	{
	    MEMCHECK(pReg, pNextRect, pReg->rects);
	    pNextRect->left = left;
	    pNextRect->top = top;
	    pNextRect->right = right;
	    pNextRect->bottom = bottom;
	    pReg->numRects += 1;
	    pNextRect++;
	}

	/*
	 * Need to advance the pointers. Shift the one that extends
	 * to the right the least, since the other still has a chance to
	 * overlap with that region's next rectangle, if you see what I mean.
	 */
	if (r1->right < r2->right)
	{
	    r1++;
	}
	else if (r2->right < r1->right)
	{
	    r2++;
	}
	else
	{
	    r1++;
	    r2++;
	}
    }
}

/***********************************************************************
 *	     GdIntersectRegion
 */
void
GdIntersectRegion(MWCLIPREGION *newReg, MWCLIPREGION *reg1, MWCLIPREGION *reg2)
{
   /* check for trivial reject */
    if ( (!(reg1->numRects)) || (!(reg2->numRects))  ||
	(!EXTENTCHECK(&reg1->extents, &reg2->extents)))
	newReg->numRects = 0;
    else
	REGION_RegionOp (newReg, reg1, reg2, 
	 (voidProcp) REGION_IntersectO, (voidProcp) NULL, (voidProcp) NULL);
    
    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents(newReg);
    newReg->type = (newReg->numRects) ? MWREGION_COMPLEX : MWREGION_NULL ;
}

/***********************************************************************
 *	     Region Union
 ***********************************************************************/

/***********************************************************************
 *	     REGION_UnionNonO
 *
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg->numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 */
static void
REGION_UnionNonO(MWCLIPREGION *pReg,MWRECT *r,MWRECT *rEnd,MWCOORD top,
	MWCOORD bottom)
{
    MWRECT *pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    while (r != rEnd)
    {
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->left = r->left;
	pNextRect->top = top;
	pNextRect->right = r->right;
	pNextRect->bottom = bottom;
	pReg->numRects += 1;
	pNextRect++;
	r++;
    }
}

/***********************************************************************
 *	     REGION_UnionO
 *
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles are overwritten in pReg->rects and pReg->numRects will
 *      be changed.
 *
 */
static void
REGION_UnionO(MWCLIPREGION *pReg, MWRECT *r1, MWRECT *r1End,
	MWRECT *r2, MWRECT *r2End, MWCOORD top, MWCOORD bottom)
{
    MWRECT *pNextRect;
    
    pNextRect = &pReg->rects[pReg->numRects];

#define MERGERECT(r) \
    if ((pReg->numRects != 0) &&  \
	(pNextRect[-1].top == top) &&  \
	(pNextRect[-1].bottom == bottom) &&  \
	(pNextRect[-1].right >= r->left))  \
    {  \
	if (pNextRect[-1].right < r->right)  \
	{  \
	    pNextRect[-1].right = r->right;  \
	}  \
    }  \
    else  \
    {  \
	MEMCHECK(pReg, pNextRect, pReg->rects);  \
	pNextRect->top = top;  \
	pNextRect->bottom = bottom;  \
	pNextRect->left = r->left;  \
	pNextRect->right = r->right;  \
	pReg->numRects += 1;  \
	pNextRect += 1;  \
    }  \
    r++;
    
    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r1->left < r2->left)
	{
	    MERGERECT(r1);
	}
	else
	{
	    MERGERECT(r2);
	}
    }
    
    if (r1 != r1End)
    {
	do
	{
	    MERGERECT(r1);
	} while (r1 != r1End);
    }
    else while (r2 != r2End)
    {
	MERGERECT(r2);
    }
}

/***********************************************************************
 *	     GdUnionRegion
 */
void
GdUnionRegion(MWCLIPREGION *newReg, MWCLIPREGION *reg1, MWCLIPREGION *reg2)
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->numRects)) )
    {
	if (newReg != reg2)
	    GdCopyRegion(newReg, reg2);
	return;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->numRects))
    {
	if (newReg != reg1)
	    GdCopyRegion(newReg, reg1);
	return;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if ((reg1->numRects == 1) && 
	(reg1->extents.left <= reg2->extents.left) &&
	(reg1->extents.top <= reg2->extents.top) &&
	(reg1->extents.right >= reg2->extents.right) &&
	(reg1->extents.bottom >= reg2->extents.bottom))
    {
	if (newReg != reg1)
	    GdCopyRegion(newReg, reg1);
	return;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if ((reg2->numRects == 1) && 
	(reg2->extents.left <= reg1->extents.left) &&
	(reg2->extents.top <= reg1->extents.top) &&
	(reg2->extents.right >= reg1->extents.right) &&
	(reg2->extents.bottom >= reg1->extents.bottom))
    {
	if (newReg != reg2)
	    GdCopyRegion(newReg, reg2);
	return;
    }

    REGION_RegionOp (newReg, reg1, reg2, (voidProcp) REGION_UnionO, 
		(voidProcp) REGION_UnionNonO, (voidProcp) REGION_UnionNonO);

    newReg->extents.left = MWMIN(reg1->extents.left, reg2->extents.left);
    newReg->extents.top = MWMIN(reg1->extents.top, reg2->extents.top);
    newReg->extents.right = MWMAX(reg1->extents.right, reg2->extents.right);
    newReg->extents.bottom = MWMAX(reg1->extents.bottom, reg2->extents.bottom);
    newReg->type = (newReg->numRects) ? MWREGION_COMPLEX : MWREGION_NULL ;
}

/***********************************************************************
 *	     Region Subtraction
 ***********************************************************************/

/***********************************************************************
 *	     REGION_SubtractNonO1
 *
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may be affected.
 *
 */
static void
REGION_SubtractNonO1(MWCLIPREGION *pReg, MWRECT *r, MWRECT *rEnd,
	MWCOORD top, MWCOORD bottom)
{
    MWRECT *pNextRect;
	
    pNextRect = &pReg->rects[pReg->numRects];
	
    while (r != rEnd)
    {
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->left = r->left;
	pNextRect->top = top;
	pNextRect->right = r->right;
	pNextRect->bottom = bottom;
	pReg->numRects += 1;
	pNextRect++;
	r++;
    }
}


/***********************************************************************
 *	     REGION_SubtractO
 *
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may have rectangles added to it.
 *
 */
static void
REGION_SubtractO(MWCLIPREGION *pReg, MWRECT *r1, MWRECT *r1End,
	MWRECT *r2, MWRECT *r2End, MWCOORD top, MWCOORD bottom)
{
    MWRECT *pNextRect;
    MWCOORD left;
    
    left = r1->left;
    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r2->right <= left)
	{
	    /*
	     * Subtrahend missed the boat: go to next subtrahend.
	     */
	    r2++;
	}
	else if (r2->left <= left)
	{
	    /*
	     * Subtrahend preceeds minuend: nuke left edge of minuend.
	     */
	    left = r2->right;
	    if (left >= r1->right)
	    {
		/*
		 * Minuend completely covered: advance to next minuend and
		 * reset left fence to edge of new minuend.
		 */
		r1++;
		if (r1 != r1End)
		    left = r1->left;
	    }
	    else
	    {
		/*
		 * Subtrahend now used up since it doesn't extend beyond
		 * minuend
		 */
		r2++;
	    }
	}
	else if (r2->left < r1->right)
	{
	    /*
	     * Left part of subtrahend covers part of minuend: add uncovered
	     * part of minuend to region and skip to next subtrahend.
	     */
	    MEMCHECK(pReg, pNextRect, pReg->rects);
	    pNextRect->left = left;
	    pNextRect->top = top;
	    pNextRect->right = r2->left;
	    pNextRect->bottom = bottom;
	    pReg->numRects += 1;
	    pNextRect++;
	    left = r2->right;
	    if (left >= r1->right)
	    {
		/*
		 * Minuend used up: advance to new...
		 */
		r1++;
		if (r1 != r1End)
		    left = r1->left;
	    }
	    else
	    {
		/*
		 * Subtrahend used up
		 */
		r2++;
	    }
	}
	else
	{
	    /*
	     * Minuend used up: add any remaining piece before advancing.
	     */
	    if (r1->right > left)
	    {
		MEMCHECK(pReg, pNextRect, pReg->rects);
		pNextRect->left = left;
		pNextRect->top = top;
		pNextRect->right = r1->right;
		pNextRect->bottom = bottom;
		pReg->numRects += 1;
		pNextRect++;
	    }
	    r1++;
	    left = r1->left;
	}
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->left = left;
	pNextRect->top = top;
	pNextRect->right = r1->right;
	pNextRect->bottom = bottom;
	pReg->numRects += 1;
	pNextRect++;
	r1++;
	if (r1 != r1End)
	{
	    left = r1->left;
	}
    }
}
	
/***********************************************************************
 *	     GdSubtractRegion
 *
 *      Subtract regS from regM and leave the result in regD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *      TRUE.
 *
 * Side Effects:
 *      regD is overwritten.
 *
 */
void
GdSubtractRegion(MWCLIPREGION *regD, MWCLIPREGION *regM, MWCLIPREGION *regS )
{
   /* check for trivial reject */
    if ( (!(regM->numRects)) || (!(regS->numRects))  ||
	(!EXTENTCHECK(&regM->extents, &regS->extents)) )
    {
	GdCopyRegion(regD, regM);
	return;
    }
 
    REGION_RegionOp (regD, regM, regS, (voidProcp) REGION_SubtractO, 
		(voidProcp) REGION_SubtractNonO1, (voidProcp) NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents (regD);
    regD->type = (regD->numRects) ? MWREGION_COMPLEX : MWREGION_NULL ;
}

/***********************************************************************
 *	     GdXorRegion
 */
void
GdXorRegion(MWCLIPREGION *dr, MWCLIPREGION *sra, MWCLIPREGION *srb)
{
    MWCLIPREGION *tra, *trb;

    if ((! (tra = GdAllocRegion())) || (! (trb = GdAllocRegion())))
	return;
    GdSubtractRegion(tra,sra,srb);
    GdSubtractRegion(trb,srb,sra);
    GdUnionRegion(dr,tra,trb);
    GdDestroyRegion(tra);
    GdDestroyRegion(trb);
}

#if 0
/***********************************************************************
 *            DumpRegion
 *            Outputs the contents of a MWCLIPREGION
 */
void
DumpRegion(MWCLIPREGION *pReg)
{
    MWRECT *pRect, *pRectEnd = pReg->rects + pReg->numRects;

    DPRINTF("Region %p: %d,%d - %d,%d %d rects\n", pReg,
	    pReg->extents.left, pReg->extents.top,
	    pReg->extents.right, pReg->extents.bottom, pReg->numRects);
    for(pRect = pReg->rects; pRect < pRectEnd; pRect++)
        DPRINTF("\t%d,%d - %d,%d\n", pRect->left, pRect->top,
		pRect->right, pRect->bottom);
}
#endif
