/*
 * Portions Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *	Somewhat less shamelessly ripped from the Wine distribution
 *
 * Win32 API Region Management Routines.
 * Win32 API Complex Rectangle Routines.
 *
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice licence.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 */
#include "windows.h"
#include "device.h"
#include <stdlib.h>
#include <string.h>

/* later, error checking can be built into this get*/
#define GDI_GetObjPtr(hrgn,type)	(hrgn)

/* local functions*/
static HRGN REGION_CreateRegion(void);
/*BOOL REGION_UnionRectWithRgn( HRGN hrgn, const RECT *lpRect );*/
/*BOOL REGION_FrameRgn( HRGN hDest, HRGN hSrc, INT x, INT y );*/

#define EMPTY_REGION(pReg) { \
    (pReg)->numRects = 0; \
    (pReg)->extents.left = (pReg)->extents.top = 0; \
    (pReg)->extents.right = (pReg)->extents.bottom = 0; \
    (pReg)->type = NULLREGION; \
 }

/*
 *          Create a new empty region.
 */
static HRGN
REGION_CreateRegion(void)
{
    MWRGNOBJ *obj;
    
    obj = GdItemNew(MWRGNOBJ);
    if(!obj)
    	return NULL;
    obj->hdr.type = OBJ_REGION;
    obj->hdr.stockobj = FALSE;
    if(!(obj->rgn = GdAllocRegion())) {
	GdItemFree(obj);
	return NULL;
    }
    return (HRGN)obj;
}


INT WINAPI
OffsetRgn( HRGN hrgn, INT x, INT y )
{
    MWRGNOBJ * obj = (MWRGNOBJ *) GDI_GetObjPtr( hrgn, OBJ_REGION );

    if (obj)
    {
	GdOffsetRegion(obj->rgn, x, y);
	return obj->rgn->type;
    }
    return ERRORREGION;
}


INT WINAPI
GetRgnBox( HRGN hrgn, LPRECT rect )
{
    MWRGNOBJ * obj = (MWRGNOBJ *) GDI_GetObjPtr( hrgn, OBJ_REGION );
    if (obj)
	return GdGetRegionBox(obj->rgn, rect);
    return ERRORREGION;
}


HRGN WINAPI
CreateRectRgn(INT left, INT top, INT right, INT bottom)
{
    HRGN hrgn;

    if (!(hrgn = REGION_CreateRegion()))
	return 0;
    /*TRACE(region, "\n");*/
    SetRectRgn(hrgn, left, top, right, bottom);
    return hrgn;
}


HRGN WINAPI
CreateRectRgnIndirect( const RECT* rect )
{
    return CreateRectRgn( rect->left, rect->top, rect->right, rect->bottom );
}


/*
 * Allows either or both left and top to be greater than right or bottom.
 */
VOID WINAPI
SetRectRgn( HRGN hrgn, INT left, INT top, INT right, INT bottom )
{
    MWRGNOBJ * obj;
    MWCLIPREGION *rgn;

    /*TRACE(region, " %04x %d,%d-%d,%d\n", hrgn, left, top, right, bottom );*/
    
    if (!(obj = (MWRGNOBJ *) GDI_GetObjPtr( hrgn, OBJ_REGION ))) return;

    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    rgn = obj->rgn;
    GdSetRectRegion(rgn, left, top, right, bottom);
}


HRGN WINAPI
CreateRoundRectRgn( INT left, INT top, INT right, INT bottom,
	INT ellipse_width, INT ellipse_height )
{
    MWRGNOBJ * obj;
    HRGN hrgn;
    int asq, bsq, d, xd, yd;
    RECT rect;
    
    /* Check if we can do a normal rectangle instead */
    if (ellipse_width == 0 || ellipse_height == 0)
	return CreateRectRgn( left, top, right, bottom );

    /* Make the dimensions sensible */
    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    ellipse_width = abs(ellipse_width);
    ellipse_height = abs(ellipse_height);

    /* Create region */

    if (!(hrgn = REGION_CreateRegion()))
	    return 0;
    obj = (MWRGNOBJ *)hrgn;
    /*TRACE(region,"(%d,%d-%d,%d %dx%d): ret=%04x\n",
	       left, top, right, bottom, ellipse_width, ellipse_height, hrgn);*/

    /* Check parameters */

    if (ellipse_width > right-left) ellipse_width = right-left;
    if (ellipse_height > bottom-top) ellipse_height = bottom-top;

    /* Ellipse algorithm, based on an article by K. Porter */
    /* in DDJ Graphics Programming Column, 8/89 */

    asq = ellipse_width * ellipse_width / 4;        /* a^2 */
    bsq = ellipse_height * ellipse_height / 4;      /* b^2 */
    if (asq == 0) asq = 1;
    if (bsq == 0) bsq = 1;
    d = bsq - asq * ellipse_height / 2 + asq / 4;   /* b^2 - a^2b + a^2/4 */
    xd = 0;
    yd = asq * ellipse_height;                      /* 2a^2b */

    rect.left   = left + ellipse_width / 2;
    rect.right  = right - ellipse_width / 2;

    /* Loop to draw first half of quadrant */

    while (xd < yd)
    {
	if (d > 0)  /* if nearest pixel is toward the center */
	{
	      /* move toward center */
	    rect.top = top++;
	    rect.bottom = rect.top + 1;
	    GdUnionRectWithRegion( &rect, obj->rgn );
	    rect.top = --bottom;
	    rect.bottom = rect.top + 1;
	    GdUnionRectWithRegion( &rect, obj->rgn );
	    yd -= 2*asq;
	    d  -= yd;
	}
	rect.left--;        /* next horiz point */
	rect.right++;
	xd += 2*bsq;
	d  += bsq + xd;
    }

    /* Loop to draw second half of quadrant */

    d += (3 * (asq-bsq) / 2 - (xd+yd)) / 2;
    while (yd >= 0)
    {
	  /* next vertical point */
	rect.top = top++;
	rect.bottom = rect.top + 1;
	GdUnionRectWithRegion( &rect, obj->rgn );
	rect.top = --bottom;
	rect.bottom = rect.top + 1;
	GdUnionRectWithRegion( &rect, obj->rgn );
	if (d < 0)   /* if nearest pixel is outside ellipse */
	{
	    rect.left--;     /* move away from center */
	    rect.right++;
	    xd += 2*bsq;
	    d  += xd;
	}
	yd -= 2*asq;
	d  += asq - yd;
    }

    /* Add the inside rectangle */

    if (top <= bottom)
    {
	rect.top = top;
	rect.bottom = bottom;
	GdUnionRectWithRegion( &rect, obj->rgn );
    }
    obj->rgn->type = SIMPLEREGION; /* FIXME? */
    return hrgn;
}


HRGN WINAPI
CreateEllipticRgn( INT left, INT top, INT right, INT bottom )
{
    return CreateRoundRectRgn(left, top, right, bottom, right-left, bottom-top);
}


HRGN WINAPI
CreateEllipticRgnIndirect( const RECT *rect )
{
    return CreateRoundRectRgn( rect->left, rect->top, rect->right,
				 rect->bottom, rect->right - rect->left,
				 rect->bottom - rect->top );
}

HRGN WINAPI
CreatePolygonRgn(const POINT *points, INT count, INT mode)
{
#if POLYREGIONS
	HRGN		hrgn;
    	MWRGNOBJ * 	obj;
	MWCLIPREGION *	rgn;

    	if (!(hrgn = REGION_CreateRegion()))
		return NULL;
    	obj = (MWRGNOBJ *)GDI_GetObjPtr(hrgn, OBJ_REGION);
	if (!obj)
		return NULL;

	rgn = GdAllocPolygonRegion((POINT *)points, count, mode);
	if (!rgn)
		return hrgn;
	GdDestroyRegion(obj->rgn);
	obj->rgn = rgn;
	return hrgn;
#endif
}

DWORD WINAPI
GetRegionData(HRGN hrgn, DWORD count, LPRGNDATA rgndata)
{
    DWORD size;
    MWRGNOBJ *obj = (MWRGNOBJ *) GDI_GetObjPtr( hrgn, OBJ_REGION );
    MWCLIPREGION *rgn;
    
    /*TRACE(region," %04x count = %ld, rgndata = %p\n", hrgn, count, rgndata);*/

    if(!obj) return 0;

    rgn = obj->rgn;
    size = rgn->numRects * sizeof(RECT);
    if(count < (size + sizeof(RGNDATAHEADER)) || rgndata == NULL)
        return size + sizeof(RGNDATAHEADER);

    rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
    rgndata->rdh.iType = RDH_RECTANGLES;
    rgndata->rdh.nCount = rgn->numRects;
    rgndata->rdh.nRgnSize = size;
    rgndata->rdh.rcBound.left = rgn->extents.left;
    rgndata->rdh.rcBound.top = rgn->extents.top;
    rgndata->rdh.rcBound.right = rgn->extents.right;
    rgndata->rdh.rcBound.bottom = rgn->extents.bottom;

    memcpy( rgndata->Buffer, rgn->rects, size );

    return 1;
}


#if 0
HRGN WINAPI
ExtCreateRegion(const XFORM* lpXform, DWORD dwCount, const RGNDATA* rgndata)
{
    HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
    MWRGNOBJ *obj = (MWRGNOBJ *) GDI_GetObjPtr( hrgn, OBJ_REGION );
    RECT *pCurRect, *pEndRect;

    /*TRACE(region, " %p %ld %p. Returning %04x\n",
		lpXform, dwCount, rgndata, hrgn);*/
    if(!hrgn)
    {
        WARN(region, "Can't create a region!\n");
	return 0;
    }
    if(lpXform)
        WARN(region, "Xform not implemented - ignoring\n");
    
    if(rgndata->rdh.iType != RDH_RECTANGLES)
    {
        WARN(region, "Type not RDH_RECTANGLES\n");
	DeleteObject( hrgn );
	return 0;
    }

    pEndRect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
    for(pCurRect = (RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
        GdUnionRectWithRegion( pCurRect, obj->rgn );

    return hrgn;
}
#endif


BOOL WINAPI
PtInRegion( HRGN hrgn, INT x, INT y )
{
    MWRGNOBJ * obj;
    
    obj = (MWRGNOBJ *) GDI_GetObjPtr( hrgn, OBJ_REGION );
    if(!obj)
	    return FALSE;
    return GdPtInRegion(obj->rgn, x, y);
}

/*
 * Returns TRUE if rect is at least partly inside hrgn
 */
BOOL WINAPI
RectInRegion( HRGN hrgn, const RECT *rect )
{
    MWRGNOBJ * obj;
    
    obj = (MWRGNOBJ *) GDI_GetObjPtr( hrgn, OBJ_REGION );
    if(!obj)
	    return FALSE;
    return (GdRectInRegion(obj->rgn, rect) == MWRECT_OUT? FALSE: TRUE);
}

BOOL WINAPI
EqualRgn( HRGN hrgn1, HRGN hrgn2 )
{
    MWRGNOBJ *obj1, *obj2;

    if ((obj1 = (MWRGNOBJ *) GDI_GetObjPtr( hrgn1, OBJ_REGION ))) 
	if ((obj2 = (MWRGNOBJ *) GDI_GetObjPtr( hrgn2, OBJ_REGION ))) 
	    return GdEqualRegion(obj1->rgn, obj2->rgn);
    return FALSE;
}

#if 0
/*
 *           REGION_UnionRectWithRgn
 *           Adds a rectangle to a HRGN
 *           A helper used by scroll.c
 */
BOOL
REGION_UnionRectWithRgn( HRGN hrgn, const RECT *lpRect )
{
    MWRGNOBJ *obj = (MWRGNOBJ *)hrgn;

    if(!obj) return FALSE;
    GdUnionRectWithRegion( lpRect, obj->rgn );
    return TRUE;
}

/*
 *           REGION_FrameRgn
 * Create a region that is a frame around another region.
 * Expand all rectangles by +/- x and y, then subtract original region.
 */
BOOL
REGION_FrameRgn( HRGN hDest, HRGN hSrc, INT x, INT y )
{
    BOOL bRet;
    MWRGNOBJ *srcObj = (MWRGNOBJ*) GDI_GetObjPtr( hSrc, OBJ_REGION );

    if (srcObj->rgn->numRects != 0) 
    {
	MWRGNOBJ* destObj = (MWRGNOBJ*) GDI_GetObjPtr( hDest, OBJ_REGION );
	RECT *pRect, *pEndRect;
	RECT tempRect;

	EMPTY_REGION( destObj->rgn );
	
	pEndRect = srcObj->rgn->rects + srcObj->rgn->numRects;
	for(pRect = srcObj->rgn->rects; pRect < pEndRect; pRect++)
	{
	    tempRect.left = pRect->left - x;        
	    tempRect.top = pRect->top - y;
	    tempRect.right = pRect->right + x;
	    tempRect.bottom = pRect->bottom + y;
	    GdUnionRectWithRegion( &tempRect, destObj->rgn );
	}
	GdSubtractRegion( destObj->rgn, destObj->rgn, srcObj->rgn );
	bRet = TRUE;
    }
    else
	bRet = FALSE;
    return bRet;
}
#endif

/*
 * Note: The behavior is correct even if src and dest regions are the same.
 */
INT WINAPI
CombineRgn(HRGN hDest, HRGN hSrc1, HRGN hSrc2, INT mode)
{
    MWRGNOBJ *destObj = (MWRGNOBJ *) GDI_GetObjPtr( hDest, OBJ_REGION);
    INT result = ERRORREGION;

    /*TRACE(region, " %04x,%04x -> %04x mode=%x\n", hSrc1, hSrc2, hDest,mode);*/

    if (destObj)
    {
	MWRGNOBJ *src1Obj = (MWRGNOBJ *) GDI_GetObjPtr( hSrc1, OBJ_REGION);

	if (src1Obj)
	{
	    /*TRACE(region, "dump:\n");
	    if(TRACE_ON(region)) 
	        REGION_DumpRegion(src1Obj->rgn);*/
	    if (mode == RGN_COPY)
	    {
		GdCopyRegion( destObj->rgn, src1Obj->rgn );
		result = destObj->rgn->type;
	    }
	    else
	    {
		MWRGNOBJ *src2Obj = (MWRGNOBJ *) GDI_GetObjPtr( hSrc2, OBJ_REGION);

		if (src2Obj)
		{
		    /*TRACE(region, "dump:\n");
		    if(TRACE_ON(region)) 
		        REGION_DumpRegion(src2Obj->rgn);*/
		    switch (mode)
		    {
		    case RGN_AND:
			GdIntersectRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn);
			break;
		    case RGN_OR:
			GdUnionRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn );
			break;
		    case RGN_XOR:
			GdXorRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn );
			break;
		    case RGN_DIFF:
			GdSubtractRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn );
			break;
		    }
		    result = destObj->rgn->type;
		}
	    }
	}
	/*TRACE(region, "dump:\n");
	if(TRACE_ON(region)) 
	    REGION_DumpRegion(destObj->rgn);*/
    }
    return result;
}

/*
 * Rectangle-related functions
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *
 */
BOOL WINAPI
IntersectRect( LPRECT dest, const RECT *src1, const RECT *src2 )
{
    if (IsRectEmpty(src1) || IsRectEmpty(src2) ||
	(src1->left >= src2->right) || (src2->left >= src1->right) ||
	(src1->top >= src2->bottom) || (src2->top >= src1->bottom))
    {
	SetRectEmpty( dest );
	return FALSE;
    }
    dest->left   = MWMAX( src1->left, src2->left );
    dest->right  = MWMIN( src1->right, src2->right );
    dest->top    = MWMAX( src1->top, src2->top );
    dest->bottom = MWMIN( src1->bottom, src2->bottom );
    return TRUE;
}


BOOL WINAPI
UnionRect( LPRECT dest, const RECT *src1, const RECT *src2 )
{
    if (IsRectEmpty(src1))
    {
	if (IsRectEmpty(src2))
	{
	    SetRectEmpty( dest );
	    return FALSE;
	}
	else *dest = *src2;
    }
    else
    {
	if (IsRectEmpty(src2)) *dest = *src1;
	else
	{
	    dest->left   = MWMIN( src1->left, src2->left );
	    dest->right  = MWMAX( src1->right, src2->right );
	    dest->top    = MWMIN( src1->top, src2->top );
	    dest->bottom = MWMAX( src1->bottom, src2->bottom );	    
	}
    }
    return TRUE;
}


BOOL WINAPI
EqualRect( const RECT* rect1, const RECT* rect2 )
{
    return ((rect1->left == rect2->left) && (rect1->right == rect2->right) &&
	    (rect1->top == rect2->top) && (rect1->bottom == rect2->bottom));
}


BOOL WINAPI
SubtractRect( LPRECT dest, const RECT *src1, const RECT *src2 )
{
    RECT tmp;

    if (IsRectEmpty( src1 ))
    {
	SetRectEmpty( dest );
	return FALSE;
    }
    *dest = *src1;
    if (IntersectRect( &tmp, src1, src2 ))
    {
	if (EqualRect( &tmp, dest ))
	{
	    SetRectEmpty( dest );
	    return FALSE;
	}
	if ((tmp.top == dest->top) && (tmp.bottom == dest->bottom))
	{
	    if (tmp.left == dest->left) dest->left = tmp.right;
	    else if (tmp.right == dest->right) dest->right = tmp.left;
	}
	else if ((tmp.left == dest->left) && (tmp.right == dest->right))
	{
	    if (tmp.top == dest->top) dest->top = tmp.bottom;
	    else if (tmp.bottom == dest->bottom) dest->bottom = tmp.top;
	}
    }
    return TRUE;
}
