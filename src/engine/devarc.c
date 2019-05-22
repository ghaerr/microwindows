/*
 * Copyright (c) 2000-2007 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Device-independent arc, pie and ellipse routines.
 * GdArc is integer only and requires start/end points.
 * New GdArcAngle no floating point required, #define NEWARCANGLE
 *	Old GdArcAngle requires floating point and uses angles.
 *	Old GdArcAngle uses qsin() and qcos() instead of sin() / cos() 
 *	so no math lib needed.
 * Note: New GdArcAngle doesn't use same draw/fill routine as GdArc/GdEllipse
 *
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Arc line clipping and integer qsin/qcos routines used by permission:
 * Copyright (C) 1997-1998 by Eero Tamminen
 * Bugfixed by Greg Haerr
 */

#include "device.h"

#define NEWARCANGLE	1	/* =1 uses new integer-only GdArcAngle*/

extern int        gr_fillmode;

#if NEWARCANGLE

/* integer sin/cos tables*/
static short icos[360] = {
  1024, 1023, 1023, 1022, 1021, 1020, 1018, 1016, 1014, 1011,
  1008, 1005, 1001, 997, 993, 989, 984, 979, 973, 968,
  962, 955, 949, 942, 935, 928, 920, 912, 904, 895,
  886, 877, 868, 858, 848, 838, 828, 817, 806, 795,
  784, 772, 760, 748, 736, 724, 711, 698, 685, 671,
  658, 644, 630, 616, 601, 587, 572, 557, 542, 527,
  512, 496, 480, 464, 448, 432, 416, 400, 383, 366,
  350, 333, 316, 299, 282, 265, 247, 230, 212, 195,
  177, 160, 142, 124, 107, 89, 71, 53, 35, 17,
  0, -17, -35, -53, -71, -89, -107, -124, -142, -160,
  -177, -195, -212, -230, -247, -265, -282, -299, -316, -333,
  -350, -366, -383, -400, -416, -432, -448, -464, -480, -496,
  -512, -527, -542, -557, -572, -587, -601, -616, -630, -644,
  -658, -671, -685, -698, -711, -724, -736, -748, -760, -772,
  -784, -795, -806, -817, -828, -838, -848, -858, -868, -877,
  -886, -895, -904, -912, -920, -928, -935, -942, -949, -955,
  -962, -968, -973, -979, -984, -989, -993, -997, -1001, -1005,
  -1008, -1011, -1014, -1016, -1018, -1020, -1021, -1022, -1023, -1023,
  -1024, -1023, -1023, -1022, -1021, -1020, -1018, -1016, -1014, -1011,
  -1008, -1005, -1001, -997, -993, -989, -984, -979, -973, -968,
  -962, -955, -949, -942, -935, -928, -920, -912, -904, -895,
  -886, -877, -868, -858, -848, -838, -828, -817, -806, -795,
  -784, -772, -760, -748, -736, -724, -711, -698, -685, -671,
  -658, -644, -630, -616, -601, -587, -572, -557, -542, -527,
  -512, -496, -480, -464, -448, -432, -416, -400, -383, -366,
  -350, -333, -316, -299, -282, -265, -247, -230, -212, -195,
  -177, -160, -142, -124, -107, -89, -71, -53, -35, -17,
  0, 17, 35, 53, 71, 89, 107, 124, 142, 160,
  177, 195, 212, 230, 247, 265, 282, 299, 316, 333,
  350, 366, 383, 400, 416, 432, 448, 464, 480, 496,
  512, 527, 542, 557, 572, 587, 601, 616, 630, 644,
  658,
  671, 685, 698, 711, 724, 736, 748, 760, 772, 784,
  795, 806, 817, 828, 838, 848, 858, 868, 877, 886,
  895, 904, 912, 920, 928, 935, 942, 949, 955, 962,
  968, 973, 979, 984, 989, 993, 997, 1001, 1005, 1008,
  1011, 1014, 1016, 1018, 1020, 1021, 1022, 1023, 1023
  };

static short isin[360] = {
  0, 17, 35, 53, 71, 89, 107, 124, 142, 160,
  177, 195, 212, 230, 247, 265, 282, 299, 316, 333,
  350, 366, 383, 400, 416, 432, 448, 464, 480, 496,
  512, 527, 542, 557, 572, 587, 601, 616, 630, 644,
  658, 671, 685, 698, 711, 724, 736, 748, 760, 772,
  784, 795, 806, 817, 828, 838, 848, 858, 868, 877,
  886, 895, 904, 912, 920, 928, 935, 942, 949, 955,
  962, 968, 973, 979, 984, 989, 993, 997, 1001, 1005,
  1008, 1011, 1014, 1016, 1018, 1020, 1021, 1022, 1023, 1023,
  1024, 1023, 1023, 1022, 1021, 1020, 1018, 1016, 1014, 1011,
  1008, 1005, 1001, 997, 993, 989, 984, 979, 973, 968,
  962, 955, 949, 942, 935, 928, 920, 912, 904, 895,
  886, 877, 868, 858, 848, 838, 828, 817, 806, 795,
  784, 772, 760, 748, 736, 724, 711, 698, 685, 671,
  658, 644, 630, 616, 601, 587, 572, 557, 542, 527,
  512, 496, 480, 464, 448, 432, 416, 400, 383, 366,
  350, 333, 316, 299, 282, 265, 247, 230, 212, 195,
  177, 160, 142, 124, 107, 89, 71, 53, 35, 17,
  0, -17, -35, -53, -71, -89, -107, -124, -142, -160,
  -177, -195, -212, -230, -247, -265, -282, -299, -316, -333,
  -350, -366, -383, -400, -416, -432, -448, -464, -480, -496,
  -512, -527, -542, -557, -572, -587, -601, -616, -630, -644,
  -658, -671, -685, -698, -711, -724, -736, -748, -760, -772,
  -784, -795, -806, -817, -828, -838, -848, -858, -868, -877,
  -886, -895, -904, -912, -920, -928, -935, -942, -949, -955,
  -962, -968, -973, -979, -984, -989, -993, -997, -1001, -1005,
  -1008, -1011, -1014, -1016, -1018, -1020, -1021, -1022, -1023, -1023,
  -1024, -1023, -1023, -1022, -1021, -1020, -1018, -1016, -1014, -1011,
  -1008, -1005, -1001, -997, -993, -989, -984, -979, -973, -968,
  -962, -955, -949, -942, -935, -928, -920, -912, -904, -895,
  -886, -877, -868, -858, -848, -838, -828, -817, -806, -795,
  -784, -772, -760, -748, -736, -724, -711, -698, -685, -671,
  -658, -644, -630, -616, -601, -587, -572, -557, -542, -527,
  -512, -496, -480, -464, -448, -432, -416, -400, -383, -366,
  -350, -333, -316, -299, -282, -265, -247, -230, -212, -195,
  -177, -160, -142, -124, -107, -89, -71, -53, -35, -17
};

/**
 * Draw an arc or pie, angles are specified in 64th's of a degree.
 *
 * @param psd Destination surface.
 * @param x0 Center of arc (X co-ordinate).
 * @param y0 Center of arc (Y co-ordinate).
 * @param rx Radius of arc in X direction.
 * @param ry Radius of arc in Y direction.
 * @param angle1 Start of arc, in 64ths of a degree,
 * 	anticlockwise from the +x axis.
 * @param angle2 End of arc, in 64ths of a degree,
 * 	anticlockwise from the +x axis.
 * @param type Type of arc:
 * MWARC is a curved line.
 * MWARCOUTLINE is a curved line plus straight lines joining the ends
 * to the center of the arc.
 * MWPIE is a filled shape, like a section of a pie chart.
 */
void
GdArcAngle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD rx, MWCOORD ry,
	MWCOORD angle1, MWCOORD angle2, int type)
{
	int s = angle1 / 64;
	int e = angle2 / 64;
	int x = 0, y = 0;
	int fx = 0, fy = 0;
	int lx = 0, ly = 0;
	int i;
	MWPOINT	pts[3];

	if ((s% 360) == (e % 360)) {
		s = 0;
		e = 360;
	} else {
		if (s > 360)
			s %= 360;
		if (e > 360)
			e %= 360;
		while (s < 0)
			s += 360;
		while (e < s)
			e += 360;
		if (s == e) {
			s = 0;
			e = 360;
		}
	}

	/* generate arc points*/
	for (i = s; i <= e; ++i) {
		/* add 1 to rx/ry to smooth small radius arcs*/
		x = ((long)  icos[i % 360] * (long) (rx + 1) / 1024) + x0;
		y = ((long) -isin[i % 360] * (long) (ry + 1) / 1024) + y0;
		if (i != s) {
			if (type == MWPIE) {
				/* use poly fill for expensive filling!*/
				pts[0].x = lx;
				pts[0].y = ly;
				pts[1].x = x;
				pts[1].y = y;
				pts[2].x = x0;
				pts[2].y = y0;
				/* note: doesn't handle patterns... FIXME*/
				GdFillPoly(psd, 3, pts);
			} else	/* MWARC*/
				GdLine(psd, lx, ly, x, y, TRUE);
		} else {
			fx = x;
			fy = y;
		}
		lx = x;
		ly = y;
	}

	if (type & MWOUTLINE) {
		/* draw two lines from center to arc endpoints*/
		GdLine(psd, x0, y0, fx, fy, TRUE);
		GdLine(psd, x0, y0, lx, ly, TRUE);
	}

	GdFixCursor(psd);
}
#endif /* NEWARCANGLE*/

/* argument holder for pie, arc and ellipse functions*/
typedef struct {
	PSD	psd;
	MWCOORD	x0, y0;		/* center*/
	MWCOORD	rx, ry;		/* radii*/
	MWCOORD	ax, ay;		/* start point*/
	MWCOORD	bx, by;		/* end point*/
	int	adir;		/* start pt: 1=bottom half, -1=top half*/
	int	bdir;		/* end pt:  -1=bottom half,  1=top half*/
	int	type;		/* MWARC, MWARCOUTLINE, MWPIE, MWELLIPSE etc*/
} SLICE;

/*
 * Clip a line segment for arc or pie drawing.
 * Returns 0 if line is clipped or on acceptable side, 1 if it's vertically
 * on other side, otherwise 3.
 */
static int
clip_line(SLICE *slice, MWCOORD xe, MWCOORD ye, int dir, MWCOORD y, MWCOORD *x0,
	MWCOORD *x1)
{
#if 1
	/*
	 * kluge: handle 180 degree case
	 */
	if (y >= 0 && ye == 0) {
/*printf("cl %d,%d %d,%d %d,%d %d,%d %d,%d\n", xe, ye, y, dir,
slice->ax, slice->ay, slice->bx, slice->by, slice->adir, slice->bdir);*/
		/* bottom 180*/
		if (slice->adir < 0) {
			if (slice->ay || slice->by)
				return 1;
			if (slice->ax == -slice->bx)
				return 0;
		}
		return 3;
	}
#endif
	/* hline on the same vertical side with the given edge? */
	if ((y >= 0 && ye >= 0) || (y < 0 && ye < 0)) {
		MWCOORD x;

		if (ye == 0) x = xe; else
		x = (MWCOORD)(long)xe * y / ye;

		if (x >= *x0 && x <= *x1) {
			if (dir > 0)
				*x0 = x;
			else
				*x1 = x;
			return 0;
		} else {
			if (dir > 0) {
				if (x <= *x0)
					return 0;
			} else {
				if (x >= *x1)
					return 0;
			}
		}
		return 3;
	}
	return 1;
}

/* relative offsets, direction from left to right. */
/* Mode indicates if we are in fill mode (1) or line mode (0) */

static void
draw_line(SLICE *slice, MWCOORD x0, MWCOORD y, MWCOORD x1, int mode)
{
	int	dbl = (slice->adir > 0 && slice->bdir < 0);
	int 	discard, ret;
	MWCOORD	x2 = x0, x3 = x1;

	if (y == 0) {
		if (slice->type != MWPIE)
			return;
		/* edges on different sides */
		if ((slice->ay <= 0 && slice->by >= 0) ||
		    (slice->ay >= 0 && slice->by <= 0)) {
			if (slice->adir < 0)  {
				if (x1 > 0)
					x1 = 0;
			}
			if (slice->bdir > 0) {
				if (x0 < 0)
					x0 = 0;
			}
		} else {
			if (!dbl) {
				/* FIXME leaving in draws dot in center*/

			        if (gr_fillmode != MWFILL_SOLID && mode) 
				  ts_drawpoint(slice->psd, slice->x0, slice->y0);
				else 
				  drawpoint(slice->psd, slice->x0, slice->y0);
				return;
			}
		}
		if (gr_fillmode != MWFILL_SOLID && mode)
		  ts_drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1, slice->y0);
		else
		  drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1, slice->y0);
		return;
	}

	/* clip left edge / line */
	ret = clip_line(slice, slice->ax, slice->ay, slice->adir, y, &x0, &x1);

	if (dbl) {
		if (!ret) {
			/* edges separate line to two parts */
		        if (gr_fillmode != MWFILL_SOLID && mode)
			  ts_drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1,
				     slice->y0 + y);
			else
			  drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1,
				  slice->y0 + y);

			x0 = x2;
			x1 = x3;
		}
	} else {
		if (ret > 1) {
			return;
		}
	}

	discard = ret;
	ret = clip_line(slice, slice->bx, slice->by, slice->bdir, y, &x0, &x1);

	discard += ret;
	if (discard > 2 && !(dbl && ret == 0 && discard == 3)) {
		return;
	}
	if (discard == 2) {
		/* line on other side than slice */
		if (slice->adir < 0 || slice->bdir > 0) {
			return;
		}
	}
	if (gr_fillmode != MWFILL_SOLID && mode)
	  ts_drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1, slice->y0 + y);
	else
	  drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1, slice->y0 + y);
}

/*
 * draw one line segment or set of points, called from drawarc routine
 *
 * Note that this is called for all rows in one quadrant of the ellipse.
 * It mirrors vertically & horizontally to get the entire ellipse.
 *
 * It passes on co-ordinates for the *entire* ellipse - for pie and
 * arc, clipping is done later to ensure that only the requested angle
 * gets drawn.
 */
static void
drawarcsegment(SLICE *slice, MWCOORD xp, MWCOORD yp, int drawon)
{
	uint32_t dm = 0;
	int dc = 0;

	switch (slice->type) {
	case MWELLIPSEFILL:
		/* draw ellipse fill segment*/
                /* First, save the dash settings, because we don't want to use them here */

	  if (gr_fillmode != MWFILL_SOLID) {
	    ts_drawrow(slice->psd, slice->x0-xp, slice->x0+xp, slice->y0-yp);
	    ts_drawrow(slice->psd, slice->x0-xp, slice->x0+xp, slice->y0+yp);
	  }
	  else {
	    GdSetDash(&dm, &dc); /* Must turn off the dash settings because of drawrow() */
	    drawrow(slice->psd, slice->x0-xp, slice->x0+xp, slice->y0-yp);
	    drawrow(slice->psd, slice->x0-xp, slice->x0+xp, slice->y0+yp);
	    GdSetDash(&dm, &dc);
	  }

	  return;

	case MWELLIPSE:
	  if (!drawon) return;
		/* set four points symmetrically situated around a point*/
		drawpoint(slice->psd, slice->x0 + xp, slice->y0 + yp);
		drawpoint(slice->psd, slice->x0 - xp, slice->y0 + yp);
		drawpoint(slice->psd, slice->x0 + xp, slice->y0 - yp);
		drawpoint(slice->psd, slice->x0 - xp, slice->y0 - yp);
		return;

	case MWPIE:
		/* draw top and bottom halfs of pie*/
	        if (gr_fillmode == MWFILL_SOLID) GdSetDash(&dm, &dc);
		draw_line(slice, -xp, -yp, +xp, 1);
		draw_line(slice, -xp, +yp, +xp, 1);
		if (gr_fillmode == MWFILL_SOLID) GdSetDash(&dm, &dc);
		return;

	default:	/* MWARC, MWARCOUTLINE*/
		/* set four points symmetrically around a point and clip*/

		draw_line(slice, +xp, +yp, +xp, 0);
		draw_line(slice, -xp, +yp, -xp, 0);
		draw_line(slice, +xp, -yp, +xp, 0);
		draw_line(slice, -xp, -yp, -xp, 0);
		return;
	}
}

/* General routine to plot points on an arc.  Used by arc, pie and ellipse*/
static void
drawarc(SLICE *slice)
{
	extern uint32_t gr_dashmask;     
	extern uint32_t gr_dashcount;    

	MWCOORD xp, yp;		/* current point (based on center) */
	MWCOORD rx, ry;
	long Asquared;		/* square of x semi axis */
	long TwoAsquared;
	long Bsquared;		/* square of y semi axis */
	long TwoBsquared;
	long d;
	long dx, dy;

	int bit  = 0;
	int drawon = 1;

	rx = slice->rx;
	ry = slice->ry;

	xp = 0;
	yp = ry;
	Asquared = rx * rx;
	TwoAsquared = 2 * Asquared;
	Bsquared = ry * ry;
	TwoBsquared = 2 * Bsquared;
	d = Bsquared - Asquared * ry + (Asquared >> 2);
	dx = 0;
	dy = TwoAsquared * ry;

	if (gr_fillmode != MWFILL_SOLID)
	  set_ts_origin(slice->x0 - rx, slice->y0 - ry);

	while (dx < dy) {

		/*
		 * Only draw if one of the following conditions holds:
		 * - We're drawing an outline - i.e. slice->type is
		 *   not MWPIE or MWELLIPSEFILL
		 * - We're about to move on to the next Y co-ordinate
		 *   (i.e. we're drawing a filled shape and we're at
		 *   the widest point for this Y co-ordinate).
		 *   This is the case if d (the error term) is >0
		 * Otherwise, we draw multiple times, which messes up
		 * with SRC_OVER or XOR modes.
		 */
		if ((d > 0) || ((slice->type != MWPIE) && (slice->type != MWELLIPSEFILL))) {
			if (gr_dashcount) {
				drawon = (gr_dashmask & (1 << bit)) ? 1 : 0;
				bit = (bit + 1) % gr_dashcount;
			} else
				drawon = 1;

			drawarcsegment(slice, xp, yp, drawon);
		}

		if (d > 0) {
			yp--;
			dy -= TwoAsquared;
			d -= dy;
		}
		xp++;
		dx += TwoBsquared;
		d += (Bsquared + dx);
	}

	d += ((3L * (Asquared - Bsquared) / 2L - (dx + dy)) >> 1);

	while (yp >= 0) {
	        if (gr_dashcount) {
	          drawon = (gr_dashmask & (1 << bit)) ? 1 : 0;
		  bit = (bit + 1) % gr_dashcount;
		}
	        else drawon = 1;

		drawarcsegment(slice, xp, yp, drawon);
		if (d < 0) {
			xp++;
			dx += TwoBsquared;
			d += dx;
		}
		yp--;
		dy -= TwoAsquared;
		d += (Asquared - dy);
	}

}

/**
 * Draw an arc or pie using start/end points.
 * Integer only routine.  To specify start/end angles,
 * use GdArcAngle.
 *
 * @param psd Destination surface.
 * @param x0 Center of arc (X co-ordinate).
 * @param y0 Center of arc (Y co-ordinate).
 * @param rx Radius of arc in X direction.
 * @param ry Radius of arc in Y direction.
 * @param ax Start of arc (X co-ordinate).
 * @param ay Start of arc (Y co-ordinate).
 * @param bx End of arc (X co-ordinate).
 * @param by End of arc (Y co-ordinate).
 * @param type Type of arc:
 * MWARC is a curved line.
 * MWARCOUTLINE is a curved line plus straight lines joining the ends
 * to the center of the arc.
 * MWPIE is a filled shape, like a section of a pie chart.
 *
 * FIXME: Buggy w/small angles
 */
void
GdArc(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD rx, MWCOORD ry,
	MWCOORD ax, MWCOORD ay, MWCOORD bx, MWCOORD by, int type)
{
	MWCOORD	adir, bdir;
	SLICE	slice;

	if (rx <= 0 || ry <= 0)
		return;

	/*
	 * Calculate right/left side clipping, based on quadrant.
	 * dir is positive when right side is filled and negative when
	 * left side is to be filled.
	 *
	 * >= 0 is bottom half
	 */
	adir = (ay >= 0)?  1: -1;
	bdir = (by >= 0)? -1:  1;
#if 1
	/*
	 * The clip_line routine has problems around the 0 and
	 * 180 degree axes.
	 * This <fix> is required to make the clip_line algorithm
	 * work.  Getting these routines to work for all angles is
	 * a bitch.  And they're still buggy.  Doing this causes
	 * half circles to be outlined with a slightly bent line
	 * on the x axis. FIXME
	 */
	if (ay == 0) ++ay;
	if (by == 0) ++by;
#endif
	/* swap rightmost edge first */
	if (bx > ax) {
		MWCOORD swap;

		swap = ax;
		ax = bx;
		bx = swap;

		swap = ay;
		ay = by;
		by = swap;

		swap = adir;
		adir = bdir;
		bdir = swap;
	}

	/* check for entire area clipped, draw with per-point clipping*/
	if (GdClipArea(psd, x0-rx, y0-ry, x0+rx, y0+ry) == CLIP_INVISIBLE)
		return;

	slice.psd = psd;
	slice.x0 = x0;
	slice.y0 = y0;
	slice.rx = rx;
	slice.ry = ry;
	slice.ax = ax;
	slice.ay = ay;
	slice.bx = bx;
	slice.by = by;
	slice.adir = adir;
	slice.bdir = bdir;
	slice.type = type;

	drawarc(&slice);

	if (type & MWOUTLINE) {
		/* draw two lines from rx,ry to arc endpoints*/
		GdLine(psd, x0, y0, x0+ax, y0+ay, TRUE);
		GdLine(psd, x0, y0, x0+bx, y0+by, TRUE);
	}

	GdFixCursor(psd);
}

/**
 * Draw an ellipse using the current clipping region and foreground color.
 * This draws in the outline of the ellipse, or fills it.
 * Integer only routine.
 *
 * @param psd Destination surface.
 * @param x Center of ellipse (X co-ordinate).
 * @param y Center of ellipse (Y co-ordinate).
 * @param rx Radius of ellipse in X direction.
 * @param ry Radius of ellipse in Y direction.
 * @param fill Nonzero for a filled ellipse, zero for an outline.
 */
void
GdEllipse(PSD psd, MWCOORD x, MWCOORD y, MWCOORD rx, MWCOORD ry, MWBOOL fill)
{
	SLICE	slice;

	if (rx < 0 || ry < 0)
		return;

	/* Check if the ellipse bounding box is either totally visible
	 * or totally invisible.  Draw with per-point clipping.
	 */
	switch (GdClipArea(psd, x - rx, y - ry, x + rx, y + ry)) {
	case CLIP_VISIBLE:
		/*
		 * For size considerations, there's no low-level ellipse
		 * draw, so we've got to draw all ellipses
		 * with per-point clipping for the time being
		psd->DrawEllipse(psd, x, y, rx, ry, fill, gr_foreground);
		GdFixCursor(psd);
		return;
		 */
		break;

	case CLIP_INVISIBLE:
		return;
  	}

	slice.psd = psd;
	slice.x0 = x;
	slice.y0 = y;
	slice.rx = rx;
	slice.ry = ry;
	slice.type = fill? MWELLIPSEFILL: MWELLIPSE;
	/* other elements unused*/

	drawarc(&slice);

	GdFixCursor(psd);
}

#if !NEWARCANGLE		/* was HAVE_FLOAT*/
#define HIGHPRECISION	0	/* =1 for high precision angles, uses mathlib*/

#if !HIGHPRECISION
typedef float	FLOAT;
/*
 * qsin/qcos - calculate sin() and cos() approximations from a lookup table
 *
 * This uses a cosine lookup table of 0-90 degrees at one degree steps
 * with the difference between successive values used for interpolation.
 * The achieved accuracy should be about +/-0.0001.  If you want more
 * accuracy, use doubles and smaller steps.  If you want more speed, use
 * fixed point arithmetics.
 */
static const float cosine[91][2] = {
	{ 1.000000, -1.523048e-04 },
	{ 0.999848, -4.568681e-04 },
	{ 0.999391, -7.612923e-04 },
	{ 0.998630, -1.065484e-03 },
	{ 0.997564, -1.369352e-03 },
	{ 0.996195, -1.672803e-03 },
	{ 0.994522, -1.975744e-03 },
	{ 0.992546, -2.278083e-03 },
	{ 0.990268, -2.579728e-03 },
	{ 0.987688, -2.880588e-03 },
	{ 0.984808, -3.180570e-03 },
	{ 0.981627, -3.479583e-03 },
	{ 0.978148, -3.777536e-03 },
	{ 0.974370, -4.074339e-03 },
	{ 0.970296, -4.369900e-03 },
	{ 0.965926, -4.664130e-03 },
	{ 0.961262, -4.956940e-03 },
	{ 0.956305, -5.248240e-03 },
	{ 0.951057, -5.537941e-03 },
	{ 0.945519, -5.825955e-03 },
	{ 0.939693, -6.112194e-03 },
	{ 0.933580, -6.396572e-03 },
	{ 0.927184, -6.679001e-03 },
	{ 0.920505, -6.959396e-03 },
	{ 0.913545, -7.237671e-03 },
	{ 0.906308, -7.513741e-03 },
	{ 0.898794, -7.787522e-03 },
	{ 0.891007, -8.058931e-03 },
	{ 0.882948, -8.327886e-03 },
	{ 0.874620, -8.594303e-03 },
	{ 0.866025, -8.858103e-03 },
	{ 0.857167, -9.119205e-03 },
	{ 0.848048, -9.377528e-03 },
	{ 0.838671, -9.632995e-03 },
	{ 0.829038, -9.885528e-03 },
	{ 0.819152, -1.013505e-02 },
	{ 0.809017, -1.038148e-02 },
	{ 0.798636, -1.062476e-02 },
	{ 0.788011, -1.086479e-02 },
	{ 0.777146, -1.110152e-02 },
	{ 0.766044, -1.133486e-02 },
	{ 0.754710, -1.156475e-02 },
	{ 0.743145, -1.179112e-02 },
	{ 0.731354, -1.201390e-02 },
	{ 0.719340, -1.223302e-02 },
	{ 0.707107, -1.244841e-02 },
	{ 0.694658, -1.266001e-02 },
	{ 0.681998, -1.286775e-02 },
	{ 0.669131, -1.307158e-02 },
	{ 0.656059, -1.327142e-02 },
	{ 0.642788, -1.346722e-02 },
	{ 0.629320, -1.365892e-02 },
	{ 0.615661, -1.384645e-02 },
	{ 0.601815, -1.402977e-02 },
	{ 0.587785, -1.420882e-02 },
	{ 0.573576, -1.438353e-02 },
	{ 0.559193, -1.455387e-02 },
	{ 0.544639, -1.471977e-02 },
	{ 0.529919, -1.488119e-02 },
	{ 0.515038, -1.503807e-02 },
	{ 0.500000, -1.519038e-02 },
	{ 0.484810, -1.533806e-02 },
	{ 0.469472, -1.548106e-02 },
	{ 0.453990, -1.561935e-02 },
	{ 0.438371, -1.575289e-02 },
	{ 0.422618, -1.588162e-02 },
	{ 0.406737, -1.600551e-02 },
	{ 0.390731, -1.612454e-02 },
	{ 0.374607, -1.623864e-02 },
	{ 0.358368, -1.634781e-02 },
	{ 0.342020, -1.645199e-02 },
	{ 0.325568, -1.655116e-02 },
	{ 0.309017, -1.664529e-02 },
	{ 0.292372, -1.673435e-02 },
	{ 0.275637, -1.681831e-02 },
	{ 0.258819, -1.689715e-02 },
	{ 0.241922, -1.697084e-02 },
	{ 0.224951, -1.703936e-02 },
	{ 0.207912, -1.710270e-02 },
	{ 0.190809, -1.716082e-02 },
	{ 0.173648, -1.721371e-02 },
	{ 0.156434, -1.726136e-02 },
	{ 0.139173, -1.730376e-02 },
	{ 0.121869, -1.734088e-02 },
	{ 0.104528, -1.737272e-02 },
	{ 0.087156, -1.739927e-02 },
	{ 0.069756, -1.742052e-02 },
	{ 0.052336, -1.743646e-02 },
	{ 0.034899, -1.744709e-02 },
	{ 0.017452, -1.745241e-02 },
	{ 0.000000, -1.745241e-02 }
};

static float
qcos(FLOAT angle)
{
	short a, b, c;

	a = angle;
	if (a < 0) {
		angle = a - angle;
		a = -a;
	} else {
		angle = angle - a;
	}
	b = a / 90;
	c = a - b * 90;

	/* interpolate according to angle */
	switch(b&3) {
		case 3:
			c = 90 - c;
			return cosine[c][0] - cosine[c-1][1] * angle;
		case 2:
			return -(cosine[c][0] + cosine[c][1] * angle);
		case 1:
			c = 90 - c;
			return cosine[c-1][1] * angle - cosine[c][0];
		default:
			return cosine[c][0] + cosine[c][1] * angle;
	}
}

static float
qsin(FLOAT angle)
{
	short a, b, c;

	/* change to cosine by subtracting 90 */
	a = (int)angle - 90;
	if (a < 0) {
		angle = (a + 90) - angle;
		a = -a;
	} else {
		angle = angle - (a + 90);
	}
	b = a / 90;
	c = a - b * 90;

	/* interpolate according to angle */
	switch(b&3) {
		case 3:
			c = 90 - c;
			return cosine[c][0] - cosine[c-1][1] * angle;
		case 2:
			return -(cosine[c][0] + cosine[c][1] * angle);
		case 1:
			c = 90 - c;
			return cosine[c-1][1] * angle - cosine[c][0];
		default:
			return cosine[c][0] + cosine[c][1] * angle;
	}
}
#else /* HIGHPRECISION*/

#include <math.h>
#define qcos	QCOS
#define qsin	QSIN
typedef double	FLOAT;

FLOAT QCOS(FLOAT a)
{
	return cos(a * M_PI / 180.);
}

FLOAT QSIN(FLOAT a)
{
	return sin(a * M_PI / 180.);
}
#endif /* HIGHPRECISION*/

/*
 * Old ArcAngle routine, requires floating point.
 * Buggy w/small angles (GdArc)
 */
void
GdArcAngle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD rx, MWCOORD ry,
	MWCOORD angle1, MWCOORD angle2, int type)
{
	MWCOORD	ax, ay, bx, by;
	FLOAT	a, b, c, d;

	/* calculate pie edge offsets from center to the ellipse rim */
	a = qcos(angle1/64.);
	c = -qsin(angle1/64.);
	ax = a * rx;
	ay = c * ry;

	b = qcos(angle2/64.);
	d = -qsin(angle2/64.);
	bx = b * rx;
	by = d * ry;

#if 1
	/* fix nxlib error with very small angles becoming whole circle*/
	if (ax == bx && ay == by && angle1 != angle2)
		return;
#endif
	/* call integer routine*/
	GdArc(psd, x0, y0, rx, ry, ax, ay, bx, by, type);
}
#endif /* !NEWARCANGLE*/

