/*
 * Copyright (c) 1999, 2000, 2002 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (C) 1999 Bradley D. LaRonde (brad@ltc.com)
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Device-independent top level mouse and cursor routines
 *
 * Reads data from mouse driver and tracks real position on the screen.
 * Intersection detection for cursor with auto removal
 *
 * Bradley D. LaRonde added absolute coordinates and z (pen pressure) Oct-1999
 */
#include <string.h>
#include "device.h"

/*
 * The following define specifies whether returned mouse
 * driver coordinates are adjusted when running in portrait
 * mode.  If the mouse driver doesn't adjust returned values
 * when in portrait mode (as is the case for the iPAQ), then
 * this define should be set on.
 */
#ifndef FLIP_MOUSE_IN_PORTRAIT_MODE
#define FLIP_MOUSE_IN_PORTRAIT_MODE	1
#endif

static MWCOORD	xpos;		/* current x position of mouse */
static MWCOORD	ypos;		/* current y position of mouse */
static MWCOORD	minx;		/* minimum allowed x position */
static MWCOORD	maxx;		/* maximum allowed x position */
static MWCOORD	miny;		/* minimum allowed y position */
static MWCOORD	maxy;		/* maximum allowed y position */
static int	scale;		/* acceleration scale factor */
static int	thresh;		/* acceleration threshhold */
static int	buttons;	/* current state of buttons */
static MWBOOL	changed;	/* mouse state has changed */

static MWCOORD 	curminx;	/* minimum x value of cursor */
static MWCOORD 	curminy;	/* minimum y value of cursor */
static MWCOORD 	curmaxx;	/* maximum x value of cursor */
static MWCOORD 	curmaxy;	/* maximum y value of cursor */
static int	curvisible;	/* >0 if cursor is visible*/
static MWBOOL 	curneedsrestore;/* cursor needs restoration after drawing*/
static MWCOORD 	cursavx;	/* saved cursor location*/
static MWCOORD 	cursavy;
static MWCOORD	cursavx2;
static MWCOORD	cursavy2;
static MWPIXELVAL curfg;		/* foreground color of cursor */
static MWPIXELVAL curbg;		/* background color of cursor */
static MWPIXELVAL cursavbits[MWMAX_CURSOR_SIZE * MWMAX_CURSOR_SIZE];
static MWIMAGEBITS cursormask[MWMAX_CURSOR_BUFLEN];
static MWIMAGEBITS cursorcolor[MWMAX_CURSOR_BUFLEN];

extern int gr_mode;

/* Advance declarations */
static int filter_relative(int, int, int, int *, int *, int, int);
static int filter_transform(int, int *, int *);
#if FLIP_MOUSE_IN_PORTRAIT_MODE
static int filter_relrotate(int, int *xpos, int *ypos, int x, int y);
static int filter_absrotate(int, int *xpos, int *ypos);
#endif

/**
 * Initialize the mouse.
 * This sets its position to (0, 0) with no boundaries and no buttons pressed.
 *
 * @return < 0 on error, or mouse fd on success
 */
int
GdOpenMouse(void)
{
	int fd;

	/* init mouse position info*/
	buttons = 0;
	xpos = 0;
	ypos = 0;
	minx = MIN_MWCOORD;
	miny = MIN_MWCOORD;
	maxx = MAX_MWCOORD;
	maxy = MAX_MWCOORD;
	changed = TRUE;

	/* init cursor position and size info*/
	curvisible = 0;
	curneedsrestore = FALSE;
	curminx = minx;
	curminy = miny;
	curmaxx = curminx + MWMAX_CURSOR_SIZE - 1;
	curmaxy = curminy + MWMAX_CURSOR_SIZE - 1;

	if ((fd = mousedev.Open(&mousedev)) == -1)
		return -1;

	/* get default acceleration settings*/
	mousedev.GetDefaultAccel(&scale, &thresh);

	/* Force all mice to !RAW mode until the values are set */
	mousedev.flags &= ~MOUSE_RAW;
	
	/* handle null mouse driver by hiding cursor*/
	if(fd == -2)
		GdHideCursor(&scrdev);

	return fd;
}

/**
 *
 * Terminate the use of the mouse.
 */
void
GdCloseMouse(void)
{
	mousedev.Close();
}

/**
 * Gets the buttons which are supported by the mouse.
 *
 * @param buttons On return, a bitmask indicating the buttons which are
 * supported by the mouse.
 */
void
GdGetButtonInfo(int *buttons)
{
	*buttons = mousedev.GetButtonInfo();
}

/**
 * Restrict the coordinates of the mouse to the specified coordinates.
 *
 * @param newminx Minimum X co-ordinate.
 * @param newminy Minimum Y co-ordinate.
 * @param newmaxx Maximum X co-ordinate.
 * @param newmaxy Maximum Y co-ordinate.
 */
void
GdRestrictMouse(MWCOORD newminx, MWCOORD newminy, MWCOORD newmaxx,
	MWCOORD newmaxy)
{
	minx = newminx;
	miny = newminy;
	maxx = newmaxx;
	maxy = newmaxy;

	GdMoveMouse(xpos, ypos);
}

/**
 * Set the acceleration threshhold and scale factors.
 * Acceleration makes the cursor move further for faster movements.
 * Basically, at mouse speeds above the threshold, the excess distance
 * moved is multiplied by the scale factor.  For example, with a threshhold
 * of 5 and a scale of 3, the following gives examples of the original and
 * modified mouse movements:
 *	input:		0	4	5	6	9	20
 *	output:		0	4	5	8	17	50
 *
 * @param newthresh Threshold where acceleration starts
 * @param newscale Acceleration factor.
 */
void
GdSetAccelMouse(int newthresh, int newscale)
{
	if (newthresh < 0)
		newthresh = 0;
	if (newscale < 0)
		newscale = 0;
	thresh = newthresh;
	scale = newscale;
}

/**
 * Move the mouse to the specified coordinates.
 * The location is limited by the current mouse coordinate restrictions.
 *
 * @param newx New X co-ordinate.
 * @param newy New Y co-ordinate.
 */
void
GdMoveMouse(MWCOORD newx, MWCOORD newy)
{
	if (!(mousedev.flags & MOUSE_RAW)) {
		if (newx < minx)
			newx = minx;
		if (newx > maxx)
			newx = maxx;
		if (newy < miny)
			newy = miny;
		if (newy > maxy)
			newy = maxy;
	}

	if (newx == xpos && newy == ypos)
		return;

	changed = TRUE;

	xpos = newx;
	ypos = newy;
}

/**
 * Read the current location and button states of the mouse.
 * Returns -1 on read error.
 * Returns 0 if no new data is available from the mouse driver,
 * or if the new data shows no change in button state or position.
 * Returns 1 if the mouse driver tells us a changed button state
 * or position. Button state and position are always both returned,
 * even if only one or the other changes.
 * Do not block.
 *
 * @param px On return, holds the mouse X co-ordinate.
 * @param py On return, holds the mouse Y co-ordinate.
 * @param pb On return, holds the buttons status.
 * @return -1 on error, 0 if mouse has not moved, 1 if mouse has moved.
 */
int
GdReadMouse(MWCOORD *px, MWCOORD *py, int *pb)
{
	MWCOORD	x, y, z;
	int	newbuttons;	/* new button state */
	int	status;		/* status of reading mouse */
	MWCOORD dx, dy;

	*px = xpos;
	*py = ypos;
	*pb = buttons;

	if (changed) {
		changed = FALSE;
		return 1;
	}

	/* read the mouse position */
	status = mousedev.Read(&x, &y, &z, &newbuttons);
	if (status < 0)
		return -1;

	/* no new info from the mouse driver? */
	if (status == 0)
		return 0;

	/* Relative mice have their own process */
	if (status == 1) {
		int rx, ry;
		filter_relative(status, thresh, scale, &rx, &ry, x, y);
#if FLIP_MOUSE_IN_PORTRAIT_MODE
		dx = xpos;
		dy = ypos;
		filter_relrotate(status, &dx, &dy, x, y);
#else
		dx = xpos + rx;
		dy = ypos + ry;
#endif
	} else {
		dx = x;
		dy = y;

		/* Transformed coords should already have their rotation handled */
		if (mousedev.flags & MOUSE_TRANSFORM)  {
			if (!filter_transform(status, &dx, &dy))
				return 0;
		}
#if FLIP_MOUSE_IN_PORTRAIT_MODE
		if (!(mousedev.flags & MOUSE_RAW))
			filter_absrotate(status, &dx, &dy);
#endif	
	}

	/* 
	 * At this point, we should have a valid mouse point. Check the button
	 * state and set the flags accordingly.  We do this *after* the filters,
	 * because some of the filters (like the transform) need to be called
	 * several times before we get valid data.
	 */
	if (buttons != newbuttons) {
		changed = TRUE;
		buttons = newbuttons;
	}

	/* Finally, move the mouse */
	if (status != 3)
		GdMoveMouse(dx, dy);

	/* anything change? */
	if (!changed)
		return 0;

	/* report new mouse data */
	changed = FALSE;
	*px = xpos;
	*py = ypos;
	*pb = buttons;

	return 1;
}

/**
 * Set the cursor position.
 *
 * @param newx New X co-ordinate.
 * @param newy New Y co-ordinate.
 */
void
GdMoveCursor(MWCOORD newx, MWCOORD newy)
{
	MWCOORD shiftx;
	MWCOORD shifty;

	shiftx = newx - curminx;
	shifty = newy - curminy;
	if(shiftx == 0 && shifty == 0)
		return;
	curminx += shiftx;
	curmaxx += shiftx;
	curminy += shifty;
	curmaxy += shifty;

	/* Restore the screen under the mouse pointer*/
	GdHideCursor(&scrdev);

	/* Draw the new pointer*/
	GdShowCursor(&scrdev);
}

/**
 * return current mouse position in x, y
 *
 * @param px On return, holds the cursor X co-ordinate.
 * @param py On return, holds the cursor Y co-ordinate.
 * @return TRUE iff cursor is visible.
 */
MWBOOL
GdGetCursorPos(MWCOORD *px, MWCOORD *py)
{
	*px = xpos;
	*py = ypos;
	return curvisible > 0;	/* return TRUE if visible*/
}

/**
 * Set the cursor size and bitmaps.
 *
 * @param pcursor New mouse cursor.
 */
void
GdSetCursor(PMWCURSOR pcursor)
{
	int	bytes;

	GdHideCursor(&scrdev);
	curmaxx = curminx + pcursor->width - 1;
	curmaxy = curminy + pcursor->height - 1;

	curfg = GdFindColor(&scrdev, pcursor->fgcolor);
	curbg = GdFindColor(&scrdev, pcursor->bgcolor);

	bytes = MWIMAGE_SIZE(pcursor->width, pcursor->height) * sizeof(MWIMAGEBITS);

	memcpy(cursorcolor, pcursor->image, bytes);
	memcpy(cursormask, pcursor->mask, bytes);

	GdShowCursor(&scrdev);
}


/**
 * Draw the mouse pointer.  Save the screen contents underneath
 * before drawing. Returns previous cursor state.
 *
 * @param psd Drawing surface.
 * @return 1 iff the cursor was visible, else <= 0
 */
int
GdShowCursor(PSD psd)
{
	MWCOORD 		x;
	MWCOORD 		y;
	MWPIXELVAL *	saveptr;
	MWIMAGEBITS *	cursorptr;
	MWIMAGEBITS *	maskptr;
	MWIMAGEBITS 	curbit, cbits = 0, mbits = 0;
	MWPIXELVAL 	oldcolor;
	MWPIXELVAL 	newcolor;
	int 		oldmode;
	int		prevcursor = curvisible;

	if(++curvisible != 1)
		return prevcursor;
	oldmode = gr_mode;
	gr_mode = MWMODE_COPY;

	saveptr = cursavbits;
	cursavx = curminx;
	cursavy = curminy;
	cursavx2 = curmaxx;
	cursavy2 = curmaxy;
	cursorptr = cursorcolor;
	maskptr = cursormask;

	/*
	 * Loop through bits, resetting to firstbit at end of each row
	 */
	curbit = 0;
	for (y = curminy; y <= curmaxy; y++) {
		if (curbit != MWIMAGE_FIRSTBIT) {
			cbits = *cursorptr++;
			mbits = *maskptr++;
			curbit = MWIMAGE_FIRSTBIT;
		}
		for (x = curminx; x <= curmaxx; x++) {
			if(x >= 0 && x < psd->xvirtres &&
			   y >= 0 && y < psd->yvirtres) {
				oldcolor = psd->ReadPixel(psd, x, y);
				if (curbit & mbits) {
					newcolor = (curbit&cbits)? curbg: curfg;
					if (oldcolor != newcolor)
						psd->DrawPixel(psd, x, y, newcolor);
				}
				*saveptr++ = oldcolor;
			}
			curbit = MWIMAGE_NEXTBIT(curbit);
			if (!curbit) {	/* check > one MWIMAGEBITS wide*/
				cbits = *cursorptr++;
				mbits = *maskptr++;
				curbit = MWIMAGE_FIRSTBIT;
			}
		}
	}

	gr_mode = oldmode;
	return prevcursor;
}

/**
 * Restore the screen overwritten by the cursor.
 *
 * @param psd Drawing surface.
 * @return 1 iff the cursor was visible, else <= 0
 */
int
GdHideCursor(PSD psd)
{
	MWPIXELVAL *	saveptr;
	MWCOORD 		x, y;
	int 		oldmode;
	int		prevcursor = curvisible;

	if(curvisible-- <= 0)
		return prevcursor;
	oldmode = gr_mode;
	gr_mode = MWMODE_COPY;

	saveptr = cursavbits;
	for (y = cursavy; y <= cursavy2; y++) {
		for (x = cursavx; x <= cursavx2; x++) {
			if(x >= 0 && x < psd->xvirtres &&
			   y >= 0 && y < psd->yvirtres) {
				psd->DrawPixel(psd, x, y, *saveptr++);
			}
		}
	}
 	gr_mode = oldmode;
	return prevcursor;
}

/**
 * Check to see if the mouse pointer is about to be overwritten.
 * If so, then remove the cursor so that the graphics operation
 * works correctly.  If the cursor is removed, then this fact will
 * be remembered and a later call to GdFixCursor will restore it.
 *
 * @param psd Drawing surface.  If it is not onscreen, this call has
 * no effect.
 * @param x1 Left edge of rectangle to check.
 * @param y1 Top edge of rectangle to check.
 * @param x2 Right edge of rectangle to check.
 * @param y2 Bottom edge of rectangle to check.
 */
void
GdCheckCursor(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2)
{
	MWCOORD temp;

	if (curvisible <= 0 || (psd->flags & PSF_SCREEN) == 0)
		return;

	if (x1 > x2) {
		temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2) {
		temp = y1;
		y1 = y2;
		y2 = temp;
	}
	if (x1 > curmaxx || x2 < curminx || y1 > curmaxy || y2 < curminy)
		return;

	GdHideCursor(psd);
	curneedsrestore = TRUE;
}


/**
 * Redisplay the cursor if it was removed because of a graphics operation.
 *
 * @param psd Drawing surface.  If it is not onscreen, this call has
 * no effect.
 */
void
GdFixCursor(PSD psd)
{
	if (curneedsrestore && (psd->flags & PSF_SCREEN)) {
		GdShowCursor(psd);
		curneedsrestore = FALSE;
	}
}

/* Input filter routines - global mouse filtering is cool */
#define JITTER_SHIFT_BITS	2
#define JITTER_DEPTH		(1 << (JITTER_SHIFT_BITS))

static MWTRANSFORM g_trans;	/* current transform*/

static struct {
	int x;
	int y;
	int count;
} jitter = { 0, 0, 0};

/* set mouse transform, raw mode if no transform specified*/
void
GdSetTransform(MWTRANSFORM *trans)
{
	if (!(mousedev.flags & MOUSE_TRANSFORM))
		return;

	if (trans) {
		g_trans = *trans;
		/*memcpy(&g_trans, trans, sizeof(MWTRANSFORM));*/
		mousedev.flags &= ~MOUSE_RAW;
	} else
		mousedev.flags |= MOUSE_RAW;
}

/* These are the mouse input filters */
static int
filter_relative(int state, int thresh, int scale, int *xpos, int *ypos, int x,
	int y)
{
	int sign = 1;

	if (state != 1)
		return 0;

	if (x < 0) {
		sign = -1;
		x = -x;
	}

	if (x > thresh)
		x = thresh + (x - thresh) * scale;
	x *= sign;

	sign = 1;

	if (y < 0) {
		sign = -1;
		y = -y;
	}

	if (y > thresh)
		y = thresh + (y - thresh) * scale;
	y *= sign;

	*xpos = x;
	*ypos = y;
	return 0;
}

#if FLIP_MOUSE_IN_PORTRAIT_MODE
static int
filter_relrotate(int state, int *xpos, int *ypos, int x, int y)
{
	if (state == 3)
		return 0;

	switch (scrdev.portrait) {
	case MWPORTRAIT_RIGHT:
		*xpos += y;
		*ypos -= x;
		break;

	case MWPORTRAIT_LEFT:
		*xpos -= y;
		*ypos += x;
		break;

	case MWPORTRAIT_DOWN:
		*xpos += x;
		*ypos -= y;
		break;

	default:
		*xpos += x;
		*ypos += y;
	}

	return 0;
}

static int
filter_absrotate(int state, int *xpos, int *ypos)
{
	int x = *xpos;
	int y = *ypos;

	if (state == 3)
		return 0;

	switch (scrdev.portrait) {
	case MWPORTRAIT_RIGHT:
		*xpos = y;
		*ypos = scrdev.xres - x - 1;
		break;

	case MWPORTRAIT_LEFT:
		*xpos = scrdev.yres - y - 1;
		*ypos = x;
		break;

	case MWPORTRAIT_DOWN:
		*xpos = x;
		*ypos = scrdev.yres - y - 1;
		break;

	default:
		*xpos = x;
		*ypos = y;
	}

	return 0;
}
#endif /* FLIP_MOUSE_IN_PORTRAIT_MODE*/

static int
filter_transform(int state, int *xpos, int *ypos)
{
	/* No transform data is available, just return the raw values */
	if (state == 3) {
		jitter.count = jitter.x = jitter.y = 0;
		return 1;
	} else if (state == 1)
		return 1;

	if (jitter.count == JITTER_DEPTH) {
		jitter.x = jitter.x >> JITTER_SHIFT_BITS;
		jitter.y = jitter.y >> JITTER_SHIFT_BITS;

		/* No translation if transform not setup yet*/
		if (g_trans.s && !(mousedev.flags & MOUSE_RAW)) {
			*xpos = ((g_trans.a * jitter.x +
				  g_trans.b * jitter.y +
				  g_trans.c) / g_trans.s);
			*ypos = ((g_trans.d * jitter.x +
				  g_trans.e * jitter.y +
				  g_trans.f) / g_trans.s);

			jitter.count = jitter.x = jitter.y = 0;
			return 1;
		} else {
			*xpos = jitter.x;
			*ypos = jitter.y;

			jitter.count = jitter.x = jitter.y = 0;
			return 2;
		}
	}

	jitter.x += *xpos;
	jitter.y += *ypos;
	++jitter.count;
	return 0;		/* In other words, don't use the returned value */
}
