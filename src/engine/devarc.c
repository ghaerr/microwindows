/*
 * Copyright (c) 2000-2001 Greg Haerr <greg@censoft.com>
 *
 * Device-independent arc, pie and ellipse routines.
 * GdArc is integer only and requires start/end points.
 * GdArcAngle requires floating point and uses angles.
 * GdArcAngle uses qsin() and qcos() instead of sin() / cos() 
 * so no math lib needed.
 *
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Arc line clipping and integer qsin/qcos routines used by permission:
 * Copyright (C) 1997-1998 by Eero Tamminen
 * Bugfixed by Greg Haerr
 */

#include <stdio.h>
#include "device.h"

#if HAVEFLOAT			/* =1 compiles in GdArcAngle*/
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
static float cosine[91][2] = {
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
#endif /* HAVEFLOAT*/

/* 
 * Draw an arc or pie, angles are specified in 64th's of a degree.
 * This function requires floating point, use GdArc for integer only.
 */
void
GdArcAngle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD rx, MWCOORD ry,
	MWCOORD angle1, MWCOORD angle2, int type)
{
#if HAVEFLOAT
	MWCOORD	ax, ay, bx, by;
	FLOAT	a, b;

	/* calculate pie edge offsets from center to the ellipse rim */
	ax = qcos(angle1/64.) * rx;
	bx = qcos(angle2/64.) * rx;

	a = -qsin(angle1/64.);
	b = -qsin(angle2/64.);
	ay = a * ry;
	by = b * ry;

	/* call integer routine*/
	GdArc(psd, x0, y0, rx, ry, ax, ay, bx, by, type);
#endif /* HAVEFLOAT*/
}

/* argument holder for pie, arc and ellipse functions*/
typedef struct {
	PSD	psd;
	MWCOORD	x0, y0;
	MWCOORD	rx, ry;
	MWCOORD	ax, ay;
	MWCOORD	bx, by;
	int	adir, bdir;
	int	type;
} SLICE;

extern void drawpoint(PSD psd, MWCOORD x, MWCOORD y);
extern void drawrow(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y);

/*
 * Clip a line segment for arc or pie drawing.
 * Returns 0 if line is clipped or on acceptable side, 1 if it's vertically
 * on other side, otherwise 3.
 */
static int
clip_line(SLICE *slice, MWCOORD xe, MWCOORD ye, int dir, MWCOORD y, MWCOORD *x0,
	MWCOORD *x1)
{
#if 0
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
static void
draw_line(SLICE *slice, MWCOORD x0, MWCOORD y, MWCOORD x1)
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
				drawpoint(slice->psd, slice->x0, slice->y0);
				return;
			}
		}
		drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1, slice->y0);
		return;
	}

	/* clip left edge / line */
	ret = clip_line(slice, slice->ax, slice->ay, slice->adir, y, &x0, &x1);

	if (dbl) {
		if (!ret) {
			/* edges separate line to two parts */
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
	drawrow(slice->psd, slice->x0 + x0, slice->x0 + x1, slice->y0 + y);
}

/* draw one line segment or set of points, called from drawarc routine*/
static void
drawarcsegment(SLICE *slice, MWCOORD xp, MWCOORD yp)
{
	switch (slice->type) {
	case MWELLIPSEFILL:
		/* draw ellipse fill segment*/
		drawrow(slice->psd, slice->x0-xp, slice->x0+xp, slice->y0-yp);
		drawrow(slice->psd, slice->x0-xp, slice->x0+xp, slice->y0+yp);
		return;

	case MWELLIPSE:
		/* set four points symmetrically situated around a point*/
		drawpoint(slice->psd, slice->x0 + xp, slice->y0 + yp);
		drawpoint(slice->psd, slice->x0 - xp, slice->y0 + yp);
		drawpoint(slice->psd, slice->x0 + xp, slice->y0 - yp);
		drawpoint(slice->psd, slice->x0 - xp, slice->y0 - yp);
		return;

	case MWPIE:
		/* draw top and bottom halfs of pie*/
		draw_line(slice, -xp, -yp, +xp);
		draw_line(slice, -xp, +yp, +xp);
		return;

	default:	/* MWARC, MWARCOUTLINE*/
		/* set four points symmetrically around a point and clip*/
		draw_line(slice, +xp, +yp, +xp);
		draw_line(slice, -xp, +yp, -xp);
		draw_line(slice, +xp, -yp, +xp);
		draw_line(slice, -xp, -yp, -xp);
		return;
	}
}

/* General routine to plot points on an arc.  Used by arc, pie and ellipse*/
static void
drawarc(SLICE *slice)
{
	MWCOORD xp, yp;		/* current point (based on center) */
	MWCOORD rx, ry;
	long Asquared;		/* square of x semi axis */
	long TwoAsquared;
	long Bsquared;		/* square of y semi axis */
	long TwoBsquared;
	long d;
	long dx, dy;

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

	while (dx < dy) {
		drawarcsegment(slice, xp, yp);
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
		drawarcsegment(slice, xp, yp);
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

/* 
 * Draw an arc or pie using start/end points.
 * Integer only routine.  To specify start/end angles, 
 * use GdArcAngle, which requires floating point.
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
	if (ay >= 0)
		adir = 1;
	else
		adir = -1;

	if (by >= 0)
		bdir = -1;
	else
		bdir = 1;

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

/*
 * Draw an ellipse using the current clipping region and foreground color.
 * This draws in the outline of the ellipse, or fills it.
 * Integer only routine.
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
