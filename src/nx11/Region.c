#include "nxlib.h"
#include <stdlib.h>
#include "uni_std.h"
#include "X11/Xutil.h"	/* typedef struct _XRegion *Region */

/*
 * X11 -> Nano-X Region routines
 *
 * Regions are currently kept only on the server side.
 */
#define CLIENTREGIONS	0		/* not yet implemented*/

typedef struct {
	short x1, x2, y1, y2;
} BOX;

struct _XRegion {
#if CLIENTREGIONS
	long size;
	long numRects;
	BOX *rects;
	BOX extents;
#endif
	GR_REGION_ID rid;
};


#if CLIENTREGIONS
static void
rectToBox(GR_RECT * rect, BOX * box)
{
	box->x1 = rect->x;
	box->x2 = rect->x + rect->width;
	box->y1 = rect->y;
	box->y2 = rect->y + rect->height;
}
#endif

Region
XCreateRegion(void)
{
	Region		region;

	region = (Region)Xcalloc(1, sizeof(Region *));
	if (!region)
		return NULL;

	region->rid = GrNewRegion();
	return region;
}

int
XDestroyRegion(Region r)
{
	GrDestroyRegion(r->rid);
	Xfree(r);

	return 1;
}

int
XUnionRectWithRegion(XRectangle *rect, Region source, Region dest)
{
	GR_RECT gr_rect;

	if (!rect->width || !rect->height)
		return 0;

	if (source != dest)
		DPRINTF("XUnionRectWithRegion - Source and dest different FIXME\n");

	/* copy rect since dimensions differ*/
	gr_rect.x = rect->x;
	gr_rect.y = rect->y;
	gr_rect.width = rect->width;
	gr_rect.height = rect->height;

	if (source != dest) {
		/*Region r = XCreateRegion();
		if (!r) return 0;
		//GrUnionRectWithRegion(r->rid, &gr_rect);
		GrSetRectRegionIndirect(r->rid, &gr_rect);
		XUnionRegion(r, source, dest);
		XDestroyRegion(r);*/

		DPRINTF("XUnionRectWithRegion - Source and dest different FIXME\n");
		GrUnionRectWithRegion(dest->rid, &gr_rect);
	} else {
		GrUnionRectWithRegion(dest->rid, &gr_rect);
	}

#if CLIENTREGIONS
	GR_RECT extents;
	/* Get the new extents area */
	GrGetRegionBox(dest->rid, &extents);
	rectToBox(&extents, &dest->extents);
#endif
	return 1;
}
/*int XUnionRectWithRegion(XRectangle *rect, Region source, Region dest)
{
	REGION region;

	if (!rect->width || !rect->height) return 0;
	region.rects = &region.extents;
	region.numRects = 1;
	region.extents.x1 = rect->x;
	region.extents.y1 = rect->y;
	region.extents.x2 = rect->x + rect->width;
	region.extents.y2 = rect->y + rect->height;
	region.size = 1;

	return XUnionRegion(&region, source, dest);
}*/

int
XPointInRegion(Region region, int x, int y)
{
	return GrPointInRegion(region->rid, x, y);
}

int
XRectInRegion(Region region, int rx, int ry, unsigned int rwidth,
	unsigned int rheight)
{
	/* note: this is dependent on NX and X11 return values identical*/
	return GrRectInRegion(region->rid, rx, ry, rwidth, rheight);
}

int
XSubtractRegion(Region regM, Region regS, Region regD)
{
	GrSubtractRegion(regD->rid, regM->rid, regS->rid);

#if CLIENTREGIONS
	GR_RECT extents;
	GrGetRegionBox(regD->rid, &extents);
	rectToBox(&extents, &regD->extents);
#endif
	return 1;
}

int
XUnionRegion(Region reg1, Region reg2, Region newReg)
{
	GrUnionRegion(newReg->rid, reg1->rid, reg2->rid);
#if CLIENTREGIONS
	GR_RECT extents;
	GrGetRegionBox(newReg->rid, &extents);
	rectToBox(&extents, &newReg->extents);
#endif
	return 1;
}

int
XIntersectRegion(Region reg1, Region reg2, Region newReg)
{
	GrIntersectRegion(newReg->rid, reg1->rid, reg2->rid);
#if CLIENTREGIONS
	GR_RECT extents;
	GrGetRegionBox(newReg->rid, &extents);
	rectToBox(&extents, &newReg->extents);
#endif
	return 1;
}

int
XXorRegion(Region sra, Region srb, Region dr)
{
	GrXorRegion(dr->rid, sra->rid, srb->rid);
#if CLIENTREGIONS
	GR_RECT extents;
	GrGetRegionBox(dr->rid, &extents);
	rectToBox(&extents, &dr->extents);
#endif
	return 0;
}

int
XEqualRegion(Region r1, Region r2)
{
	return GrEqualRegion(r1->rid, r2->rid);
}

int
XEmptyRegion(Region r)
{
	GrEmptyRegion(r->rid);
#if CLIENTREGIONS
	r->extents.x1 = 0;
	r->extents.y1 = 0;
	r->extents.x2 = 0;
	r->extents.y2 = 0;
	r->size = 0;
	r->numRects = 0;
	r->rects = NULL;	/* FIXME: must free*/
#endif
	return 1;
}

int 
XOffsetRegion(Region region, int x, int y)
{
	GrOffsetRegion(region->rid, x, y);
	return 1;
}

int
XSetRegion(Display * display, GC gc, Region r)
{
	/*GrCopyRegion(tmp, r->rid); FIXME need to copy region here*/
	GrSetGCRegion(gc->gid, r->rid);

	return 1;
}

int
XClipBox(Region r, XRectangle *ret)
{
	GR_RECT rect;

	GrGetRegionBox(r->rid, &rect);

	/* must copy rect since dimensions differ*/
	ret->x = rect.x;
	ret->y = rect.y;
	ret->width = rect.width;
	ret->height = rect.height;
	return 1;
}

Region
XPolygonRegion(XPoint *points, int n, int rule)
{
	Region		region;
	int		i;
	GR_POINT *	local;

	region = (Region)Xcalloc(1, sizeof(Region *));
	if (!region)
		return NULL;

	/* must copy points, since dimensions differ*/
	local = ALLOCA(n * sizeof(GR_POINT));
	if (!local) {
		Xfree(region);
		return 0;
	}

	for (i=0; i < n; i++) {
		local[i].x = points[i].x;
		local[i].y = points[i].y;
	}

	/* convert rule to NX format*/
	rule = (rule == EvenOddRule)? GR_POLY_EVENODD: GR_POLY_WINDING;

	region->rid = GrNewPolygonRegion(rule, n, local);
	FREEA(local);

	return region;
}

#if 0
XSetRegion( dpy, gc, r )
    Display *dpy;
    GC gc;
    register Region r;
{
    register int i;
    register XRectangle *xr, *pr;
    register BOX *pb;
    unsigned long total;
    extern void _XSetClipRectangles();

    LockDisplay (dpy);
    total = r->numRects * sizeof (XRectangle);
    if (xr = (XRectangle *) _XAllocTemp(dpy, total)) {
	for (pr = xr, pb = r->rects, i = r->numRects; --i >= 0; pr++, pb++) {
	    pr->x = pb->x1;
	    pr->y = pb->y1;
	    pr->width = pb->x2 - pb->x1;
	    pr->height = pb->y2 - pb->y1;
	}
    }
    if (xr || !r->numRects)
	_XSetClipRectangles(dpy, gc, 0, 0, xr, r->numRects, YXBanded);
    if (xr)
	_XFreeTemp(dpy, (char *)xr, total);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}


XOffsetRegion(pRegion, x, y)
    register Region pRegion;
    register int x;
    register int y;
{
    register int nbox;
    register BOX *pbox;

    pbox = pRegion->rects;
    nbox = pRegion->numRects;

    while(nbox--)
    {
	pbox->x1 += x;
	pbox->x2 += x;
	pbox->y1 += y;
	pbox->y2 += y;
	pbox++;
    }
    pRegion->extents.x1 += x;
    pRegion->extents.x2 += x;
    pRegion->extents.y1 += y;
    pRegion->extents.y2 += y;
    return 1;
}

XShrinkRegion(r, dx, dy)
    Region r;
    int dx, dy;
{
    Region s, t;
    int grow;

    if (!dx && !dy) return 0;
    if ((! (s = XCreateRegion()))  || (! (t = XCreateRegion()))) return 0;
    if (grow = (dx < 0)) dx = -dx;
    if (dx) Compress(r, s, t, (unsigned) 2*dx, TRUE, grow);
    if (grow = (dy < 0)) dy = -dy;
    if (dy) Compress(r, s, t, (unsigned) 2*dy, FALSE, grow);
    XOffsetRegion(r, dx, dy);
    XDestroyRegion(s);
    XDestroyRegion(t);
    return 0;
}


XIntersectRegion(reg1, reg2, newReg)
    Region 	  	reg1;
    Region	  	reg2;          /* source regions     */
    register Region 	newReg;               /* destination Region */
{
   /* check for trivial reject */
    if ( (!(reg1->numRects)) || (!(reg2->numRects))  ||
	(!EXTENTCHECK(&reg1->extents, &reg2->extents)))
        newReg->numRects = 0;
    else
	miRegionOp (newReg, reg1, reg2, 
    		(voidProcp) miIntersectO, (voidProcp) NULL, (voidProcp) NULL);
    
    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents(newReg);
    return 1;
}

XUnionRegion(reg1, reg2, newReg)
    Region 	  reg1;
    Region	  reg2;             /* source regions     */
    Region 	  newReg;                  /* destination Region */
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->numRects)) )
    {
        if (newReg != reg2)
            miRegionCopy(newReg, reg2);
        return 1;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->numRects))
    {
        if (newReg != reg1)
            miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if ((reg1->numRects == 1) && 
	(reg1->extents.x1 <= reg2->extents.x1) &&
	(reg1->extents.y1 <= reg2->extents.y1) &&
	(reg1->extents.x2 >= reg2->extents.x2) &&
	(reg1->extents.y2 >= reg2->extents.y2))
    {
        if (newReg != reg1)
            miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if ((reg2->numRects == 1) && 
	(reg2->extents.x1 <= reg1->extents.x1) &&
	(reg2->extents.y1 <= reg1->extents.y1) &&
	(reg2->extents.x2 >= reg1->extents.x2) &&
	(reg2->extents.y2 >= reg1->extents.y2))
    {
        if (newReg != reg2)
            miRegionCopy(newReg, reg2);
        return 1;
    }

    miRegionOp (newReg, reg1, reg2, (voidProcp) miUnionO, 
    		(voidProcp) miUnionNonO, (voidProcp) miUnionNonO);

    newReg->extents.x1 = MWMIN(reg1->extents.x1, reg2->extents.x1);
    newReg->extents.y1 = MWMIN(reg1->extents.y1, reg2->extents.y1);
    newReg->extents.x2 = MWMAX(reg1->extents.x2, reg2->extents.x2);
    newReg->extents.y2 = MWMAX(reg1->extents.y2, reg2->extents.y2);

    return 1;
}

XXorRegion( sra, srb, dr )
    Region sra, srb, dr;
{
    Region tra, trb;

    if ((! (tra = XCreateRegion())) || (! (trb = XCreateRegion())))
	return 0;
    (void) XSubtractRegion(sra,srb,tra);
    (void) XSubtractRegion(srb,sra,trb);
    (void) XUnionRegion(tra,trb,dr);
    XDestroyRegion(tra);
    XDestroyRegion(trb);
    return 0;
}

/*
 * Check to see if the region is empty.  Assumes a region is passed 
 * as a parameter
 */
int 
XEmptyRegion( r )
    Region r;
{
    if( r->numRects == 0 ) return TRUE;
    else  return FALSE;
}

/*
 *	Check to see if two regions are equal	
 */
int 
XEqualRegion( r1, r2 )
    Region r1, r2;
{
    int i;

    if( r1->numRects != r2->numRects ) return FALSE;
    else if( r1->numRects == 0 ) return TRUE;
    else if ( r1->extents.x1 != r2->extents.x1 ) return FALSE;
    else if ( r1->extents.x2 != r2->extents.x2 ) return FALSE;
    else if ( r1->extents.y1 != r2->extents.y1 ) return FALSE;
    else if ( r1->extents.y2 != r2->extents.y2 ) return FALSE;
    else for( i=0; i < r1->numRects; i++ ) {
    	if ( r1->rects[i].x1 != r2->rects[i].x1 ) return FALSE;
    	else if ( r1->rects[i].x2 != r2->rects[i].x2 ) return FALSE;
    	else if ( r1->rects[i].y1 != r2->rects[i].y1 ) return FALSE;
    	else if ( r1->rects[i].y2 != r2->rects[i].y2 ) return FALSE;
    }
    return TRUE;
}

int 
XPointInRegion( pRegion, x, y )
    Region pRegion;
    int x, y;
{
    int i;

    if (pRegion->numRects == 0)
        return FALSE;
    if (!INBOX(pRegion->extents, x, y))
        return FALSE;
    for (i=0; i<pRegion->numRects; i++)
    {
        if (INBOX (pRegion->rects[i], x, y))
	    return TRUE;
    }
    return FALSE;
}
#endif
