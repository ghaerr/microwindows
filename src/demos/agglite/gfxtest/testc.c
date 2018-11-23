#include <stdio.h>
#include <math.h>		// float required for sin/cos in drawArc/drawPie
#include "device.h"
/*
 * AdaFruit AGFX compatible draw routines for Microwindows
 *
 * Initial Port - Draw & Fill Circles, RoundRects, Triangles
 * 6 Aug 2017 ghaerr
 *
 * Add Circle and RoundRect Regions
 * 7 Aug 2017 ghaerr
 *
 * Add 3d Inset Rects and RoundRects
 * 23 Aug 2017 ghaerr
 *
 * Portions Copyright (c) 2017 Greg Haerr, <greg@censoft.com>
 *
 * Based on Adafruit GFX library ported to microwindows engine
 *
 * Copyright (c) 2013 Adafruit Industries.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// draw routines
void fasthline(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w);
void fastvline(PSD psd, MWCOORD x, MWCOORD y, MWCOORD h);
void drawRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h);
void fillRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h);
void fillScreen(PSD psd);
void drawLine(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1);
void drawCircle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD r);
void fillCircle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD r);
void drawRoundRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
void fillRoundRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
void drawTriangle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2);
void fillTriangle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2);
void draw3dInsetBox(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h);
void draw3dInsetRoundRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
// region routines
void regionCircle(MWCLIPREGION *rgn, MWCOORD x0, MWCOORD y0, MWCOORD r);
void regionRoundRect(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
// float required for these
void drawArc(PSD psd, MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle, MWCOORD thickness);
void drawPie(PSD psd, MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle);

#define swap(a, b) { MWCOORD t = a; a = b; b = t; }
#define min(a,b) ((a)<(b)?(a):(b))

// Draw horizontal line
void fasthline(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w)
{
	drawrow(psd, x, x+w-1, y);
	// calling routine must call GdFixCursor() afterwards
}

// Draw vertical line
void fastvline(PSD psd, MWCOORD x, MWCOORD y, MWCOORD h)
{
	drawcol(psd, x, y, y+h-1);
	// calling routine must call GdFixCursor() afterwards
}

// Draw a rectangle
void drawRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
{
    fasthline(psd, x, y, w);
    fasthline(psd, x, y+h-1, w);
    fastvline(psd, x, y, h);
    fastvline(psd, x+w-1, y, h);
    //GdRect(psd, x, y, w, h);
    GdFixCursor(psd);
}

void fillRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
{
    //MWCOORD i;
    //for (i=x; i<x+w; i++)
        //fastvline(psd, i, y, h);
    GdFillRect(psd, x, y, w, h);
}

void fillScreen(PSD psd)
{
    fillRect(psd, 0, 0, psd->xvirtres, psd->yvirtres);
}

// Bresenham's algorithm - thx wikpedia
// always draws last point at x1,y1
void drawLine(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1)
{
    MWCOORD steep;

    if (x0 == x1) {
        if (y0 > y1) swap(y0, y1);
        fastvline(psd, x0, y0, y1 - y0 + 1);
	GdFixCursor(psd);
	return;
    } else if(y0 == y1) {
        if (x0 > x1) swap(x0, x1);
        fasthline(psd, x0, y0, x1 - x0 + 1);
	GdFixCursor(psd);
	return;
    }

    steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    MWCOORD dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    MWCOORD err = dx / 2;
    MWCOORD ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            GdPoint(psd, y0, x0);
        } else {
            GdPoint(psd, x0, y0);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
    GdFixCursor(psd);
}

// Draw a circle outline
void drawCircle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD r)
{
    MWCOORD f = 1 - r;
    MWCOORD ddF_x = 1;
    MWCOORD ddF_y = -2 * r;
    MWCOORD x = 0;
    MWCOORD y = r;

    GdPoint(psd, x0  , y0+r);	// center bottom
    GdPoint(psd, x0  , y0-r);	// center top
    GdPoint(psd, x0+r, y0  );	// right center
    GdPoint(psd, x0-r, y0  );	// left center

    while (x<y) {		// draw arc from top downward stepping x right until 45 degrees
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        GdPoint(psd, x0 + x, y0 + y);	// right center bottom
        GdPoint(psd, x0 - x, y0 + y);	// left center bottom
        GdPoint(psd, x0 + x, y0 - y);	// right center top
        GdPoint(psd, x0 - x, y0 - y);	// left center top
        GdPoint(psd, x0 + y, y0 + x);	// bottom right center
        GdPoint(psd, x0 - y, y0 + x);	// bottom left center
        GdPoint(psd, x0 + y, y0 - x);	// top right center
        GdPoint(psd, x0 - y, y0 - x);	// top left center
    }
}

// Used to fill circles and roundrects
static void fillCircleHelper(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername, MWCOORD delta)
{
    MWCOORD f     = 1 - r;
    MWCOORD ddF_x = 1;
    MWCOORD ddF_y = -2 * r;
    MWCOORD x     = 0;
    MWCOORD y     = r;

    while (x<y) {			// fill downward stepping x right until 45 degrees
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x1) {				// right side fill
            fastvline(psd, x0+x, y0-y, 2*y+1+delta);	// right center top vertical
            fastvline(psd, x0+y, y0-x, 2*x+1+delta);	// top right center vertical
        }
        if (cornername & 0x2) {				// left side fill
            fastvline(psd, x0-x, y0-y, 2*y+1+delta);	// left center top vertical
            fastvline(psd, x0-y, y0-x, 2*x+1+delta);	// top left center vertical
        }
    }
}

// Yep, fill a circle
void fillCircle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD r)
{
    fastvline(psd, x0, y0-r, 2*r+1);			// center top vertical
    fillCircleHelper(psd, x0, y0, r, 3, 0);		// left and right vertical sides
    GdFixCursor(psd);
}

// Fill a rounded rectangle
void fillRoundRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
    // fill interior rect
    fillRect(psd, x+r, y, w-2*r, h);

    // draw four corners
    fillCircleHelper(psd, x+w-r-1, y+r, r, 1, h-2*r-1);	// right vertical side
    fillCircleHelper(psd, x+r    , y+r, r, 2, h-2*r-1);	// left vertical side
    GdFixCursor(psd);
}

// used to draw round rects
static void drawCircleHelper(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername)
{
    MWCOORD f     = 1 - r;
    MWCOORD ddF_x = 1;
    MWCOORD ddF_y = -2 * r;
    MWCOORD x     = 0;
    MWCOORD y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {			// bottom right corner
            GdPoint(psd, x0 + x, y0 + y);	// right center bottom
            GdPoint(psd, x0 + y, y0 + x);	// bottom right center
        }
        if (cornername & 0x2) {			// top right corner
            GdPoint(psd, x0 + x, y0 - y);	// right center top
            GdPoint(psd, x0 + y, y0 - x);	// top right center
        }
        if (cornername & 0x8) {			// bottom left corner
            GdPoint(psd, x0 - y, y0 + x);	// bottom left center
            GdPoint(psd, x0 - x, y0 + y);	// left center bottom
        }
        if (cornername & 0x1) {			// top left corner
            GdPoint(psd, x0 - y, y0 - x);	// top left center
            GdPoint(psd, x0 - x, y0 - y);	// left center top
        }
    }
}

// Draw a rounded rectangle
void drawRoundRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
    fasthline(psd, x+r  , y    , w-2*r); // Top
    fasthline(psd, x+r  , y+h-1, w-2*r); // Bottom
    fastvline(psd, x    , y+r  , h-2*r); // Left
    fastvline(psd, x+w-1, y+r  , h-2*r); // Right

    // draw four corners
    drawCircleHelper(psd, x+r    , y+r    , r, 1);	// top left
    drawCircleHelper(psd, x+w-r-1, y+r    , r, 2);	// top right
    drawCircleHelper(psd, x+w-r-1, y+h-r-1, r, 4);	// bottom right
    drawCircleHelper(psd, x+r    , y+h-r-1, r, 8);	// bottom left
    GdFixCursor(psd);
}

// Draw a triangle
void drawTriangle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2)
{
    drawLine(psd, x0, y0, x1, y1);
    drawLine(psd, x1, y1, x2, y2);
    drawLine(psd, x2, y2, x0, y0);
}

// Fill a triangle
void fillTriangle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2)
{

    MWCOORD a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        swap(y0, y1); swap(x0, x1);
    }
    if (y1 > y2) {
        swap(y2, y1); swap(x2, x1);
    }
    if (y0 > y1) {
        swap(y0, y1); swap(x0, x1);
    }

    if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)      a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a)      a = x2;
        else if(x2 > b) b = x2;
        fasthline(psd, a, y0, b-a+1);
        GdFixCursor(psd);
        return;
    }

    MWCOORD
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;

    long
    sa   = 0,
    sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it

    for(y=y0; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) swap(a,b);
        fasthline(psd, a, y, b-a+1);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y<=y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) swap(a,b);
        fasthline(psd, a, y, b-a+1);
    }
    GdFixCursor(psd);
}

#define regionUnionRect(rgn,x,y,w,h) \
{ \
    MWRECT rc; \
    rc.left = (x); \
    rc.top = (y); \
    rc.right = (x)+(w); \
    rc.bottom = (y)+(h); \
    GdUnionRectWithRegion(&rc, (rgn)); \
    printf("numRects %d\n", rgn->numRects); \
}
#define regionUnionVLine(rgn,x,y,h) 	regionUnionRect(rgn,x,y,1,h)

// Used for circle and roundrect regions
static void regionCircleHelper(MWCLIPREGION *rgn, MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername, MWCOORD delta)
{
    MWCOORD f     = 1 - r;
    MWCOORD ddF_x = 1;
    MWCOORD ddF_y = -2 * r;
    MWCOORD x     = 0;
    MWCOORD y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x1) {
            regionUnionVLine(rgn, x0+x, y0-y, 2*y+1+delta);
            regionUnionVLine(rgn, x0+y, y0-x, 2*x+1+delta);
        }
        if (cornername & 0x2) {
            regionUnionVLine(rgn, x0-x, y0-y, 2*y+1+delta);
            regionUnionVLine(rgn, x0-y, y0-x, 2*x+1+delta);
        }
    }
}

// Create circular region
void regionCircle(MWCLIPREGION *rgn, MWCOORD x0, MWCOORD y0, MWCOORD r)
{
    regionUnionVLine(rgn, x0, y0-r, 2*r+1);
    regionCircleHelper(rgn, x0, y0, r, 3, 0);
}

// Create rounded rectangle region
void regionRoundRect(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
    regionUnionRect(rgn, x+r, y, w-2*r, h);

    // draw four corners
    regionCircleHelper(rgn, x+w-r-1, y+r, r, 1, h-2*r-1);
    regionCircleHelper(rgn, x+r    , y+r, r, 2, h-2*r-1);
}

// draw arc segment only (requires float)
void drawArc(PSD psd, MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle, MWCOORD thickness)
{
	MWCOORD rDelta = -(thickness/2);
	MWCOORD px, py, cx, cy;
	MWCOORD i, d;

	startAngle -= 90;
	endAngle   -= 90;
if (startAngle > endAngle)
	startAngle -= 360;
	
printf("Arc %d, %d\n", startAngle, endAngle);
	if (startAngle != endAngle)
	{
		for (i=0; i<thickness; i++)
		{
			px = x + cos((startAngle*M_PI)/180) * (r+rDelta+i);
			py = y + sin((startAngle*M_PI)/180) * (r+rDelta+i);
			for (d=startAngle+1; d<endAngle+1; d++)
			{
				cx = x + cos((d*M_PI)/180) * (r+rDelta+i);
				cy = y + sin((d*M_PI)/180) * (r+rDelta+i);
				drawLine(psd, px, py, cx, cy);
				px = cx;
				py = cy;
			}
		}
	}
	else
	{
		px = x + cos((startAngle*M_PI)/180) * (r+rDelta);
		py = y + sin((startAngle*M_PI)/180) * (r+rDelta);
		cx = x + cos((startAngle*M_PI)/180) * (r-rDelta);
		cy = y + sin((startAngle*M_PI)/180) * (r-rDelta);
		drawLine(psd, px, py, cx, cy);
	}
	GdFixCursor(psd);
}

// draw arc segment and lines to center (requires float)
void drawPie(PSD psd, MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle)
{
	MWCOORD px, py, cx, cy;
	MWCOORD d;

	startAngle -= 90;
	endAngle   -= 90;
	if (startAngle > endAngle)
		startAngle -= 360;
	
	px = x + cos((startAngle*M_PI)/180) * r;
	py = y + sin((startAngle*M_PI)/180) * r;
	drawLine(psd, x, y, px, py);
	for (d=startAngle+1; d<endAngle+1; d++)
	{
		cx = x + cos((d*M_PI)/180) * r;
		cy = y + sin((d*M_PI)/180) * r;
		drawLine(psd, px, py, cx, cy);
		px = cx;
		py = cy;
	}
	drawLine(psd, x, y, px, py);
	GdFixCursor(psd);
}

// used to draw 3d inset round rects
static void draw3dCircleHelper(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername,
	MWCOLORVAL crTop, MWCOLORVAL crBottom)
{
    MWCOORD f     = 1 - r;
    MWCOORD ddF_x = 1;
    MWCOORD ddF_y = -2 * r;
    MWCOORD x     = 0;
    MWCOORD y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {			// bottom right corner
			GdSetForegroundColor(psd, crTop);
            GdPoint(psd, x0 + x, y0 + y);	// right center bottom
            GdPoint(psd, x0 + y, y0 + x);	// bottom right center
        }
        if (cornername & 0x2) {			// top right corner
			GdSetForegroundColor(psd, (x >= y)? crBottom: crTop);
            GdPoint(psd, x0 + x, y0 - y);	// right center top
			GdSetForegroundColor(psd, (x >= y)? crTop: crBottom);
            GdPoint(psd, x0 + y, y0 - x);	// top right center
        }
        if (cornername & 0x8) {			// bottom left corner
			GdSetForegroundColor(psd, (x >= y)? crBottom: crTop);
            GdPoint(psd, x0 - y, y0 + x);	// bottom left center
			GdSetForegroundColor(psd, (x >= y)? crTop: crBottom);
            GdPoint(psd, x0 - x, y0 + y);	// left center bottom
        }
        if (cornername & 0x1) {			// top left corner
			GdSetForegroundColor(psd, crTop);
            GdPoint(psd, x0 - y, y0 - x);	// top left center
            GdPoint(psd, x0 - x, y0 - y);	// left center top
        }
    }
}

// Draw a 3d rounded rectangle
static void draw3dRoundRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r,
	MWCOLORVAL crTop, MWCOLORVAL crBottom)
{
	GdSetForegroundColor(psd, crTop);
    fasthline(psd, x+r  , y    , w-2*r); // Top
    fastvline(psd, x    , y+r  , h-2*r); // Left
    draw3dCircleHelper(psd, x+r    , y+r    , r, 1, crTop, crTop);	// top left

	GdSetForegroundColor(psd, crBottom);
    fasthline(psd, x+r  , y+h-1, w-2*r); // Bottom
    fastvline(psd, x+w-1, y+r  , h-2*r); // Right
    draw3dCircleHelper(psd, x+w-r-1, y+h-r-1, r, 4, crBottom, crBottom);	// bottom right

    draw3dCircleHelper(psd, x+r    , y+h-r-1, r, 8, crTop, crBottom);	// bottom left
    draw3dCircleHelper(psd, x+w-r-1, y+r    , r, 2, crTop, crBottom);	// top right
}

/*
 * draw3dBox
 *
 *	TTTTTTTTTTTTTTB
 *	T             B
 *	T             B
 *	BBBBBBBBBBBBBBB
 */
static void draw3dBox(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOLORVAL crTop, MWCOLORVAL crBottom)
{
	//MoveToEx( hDC, x, y+h-2, NULL);
	//LineTo( hDC, x, y);				// left side
	//MoveToEx( hDC, x, y, NULL);
	//LineTo( hDC, x+w-1, y);			// top side

	GdSetForegroundColor(psd, crTop);
	fastvline(psd, x, y+1, h-2);		// left side
	fasthline(psd, x, y, w-1);			// top side

	//MoveToEx( hDC, x+w-1, y, NULL);
	//LineTo( hDC, x+w-1, y+h-1);		// right side
	//LineTo( hDC, x-1, y+h-1);			// bottom side

	GdSetForegroundColor(psd, crBottom);
	fastvline(psd, x+w-1, y, h-1);		// right side
	fasthline(psd, x, y+h-1, w);		// bottom side
}

// draw 2 line deep 3d inset box
void draw3dInsetBox(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
{
	MWCOLORVAL BTNSHADOW = MWRGB(162, 141, 104);
	MWCOLORVAL BTNHIGHLIGHT = MWRGB(234, 230, 221);
	MWCOLORVAL WINDOWFRAME = MWRGB(  0,   0,   0);
	MWCOLORVAL C3DLIGHT = MWRGB(213, 204, 187);
	MWPIXELVAL orgPixelVal = gr_foreground;

	draw3dBox(psd, x, y, w, h, BTNSHADOW, BTNHIGHLIGHT);
	++x; ++y; w -= 2; h -= 2;
	draw3dBox(psd, x, y, w, h, WINDOWFRAME, C3DLIGHT);

	gr_foreground = orgPixelVal;
	GdFixCursor(psd);
}

// draw 2 line deep 3d inset roundrect
void draw3dInsetRoundRect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
	MWCOLORVAL BTNSHADOW = MWRGB(162, 141, 104);
	MWCOLORVAL BTNHIGHLIGHT = MWRGB(234, 230, 221);
	MWCOLORVAL WINDOWFRAME = MWRGB(  0,   0,   0);
	MWCOLORVAL C3DLIGHT = MWRGB(213, 204, 187);
	MWPIXELVAL orgPixelVal = gr_foreground;

	draw3dRoundRect(psd, x, y, w, h, r, BTNSHADOW, BTNHIGHLIGHT);
	++x; ++y; w -= 2; h -= 2;
	draw3dRoundRect(psd, x, y, w, h, r-2, WINDOWFRAME, C3DLIGHT);

	gr_foreground = orgPixelVal;
	GdFixCursor(psd);
}

// uses float
static int Random(int max)
{
    int r = (rand() << 15) | rand();
    return (int)(((r & 0xFFFFFFF) / (float)(0xFFFFFFF + 1)) * (max - 0) + 0);
}

static void draw2(PSD psd)
{
  int x1,x2,x3,y1,y2,y3,r,as,ae,i;

int max_x = psd->xvirtres;
int max_y = psd->yvirtres;

#if 0
// Draw some random triangles
  for (i=0; i<50; i++)
  {
    x1=Random(max_x);
    y1=Random(max_y);
    x2=Random(max_x);
    y2=Random(max_y);
    x3=Random(max_x);
    y3=Random(max_y);
    drawTriangle(psd, x1, y1, x2, y2, x3, y3);
  }
#endif
#if 0
// Draw some random filled triangles
  for (i=0; i<50; i++)
  {
    x1=Random(max_x);
    y1=Random(max_y);
    x2=Random(max_x);
    y2=Random(max_y);
    x3=Random(max_x);
    y3=Random(max_y);
    fillTriangle(psd, x1, y1, x2, y2, x3, y3);
  }
#endif

#if 1
// Draw some random pies
  for (i=0; i<5; i++)
  {
    x1=30+Random(max_x-60);
    y1=30+Random(max_y-60);
    r=10+Random(50);
r = 30;
    as=Random(360);
    ae=Random(360);
//ae = as;
    drawArc(psd, x1, y1, r, as, ae, 3);
    drawPie(psd, x1+50, y1, r, as, ae);
  }
#endif
}

void draw(PSD psd)
{
	MWCOORD w, h, cw, ch, r;
extern MWCLIPREGION *clipregion;
MWCLIPREGION *rgn;
MWRECT rc;
	w = psd->xvirtres;
	h = psd->yvirtres;
	cw = w/2;
	ch = h/2;

	GdSetForegroundColor(psd, WHITE);

#if 0
#define setrect(rc,x,y,w,h) {rc.left=(x); rc.top=(y); rc.right=(x)+(w); rc.bottom=(y)+(h); }
	setrect(rc, 190, 190, 200, 200);
	rgn = GdAllocRectRegion(0, 0, psd->xvirtres, psd->yvirtres);
	GdSubtractRectFromRegion(&rc, rgn);
	GdSetClipRegion(psd, rgn);
#endif
#if 0
	rgn = GdAllocRegion();
	regionRoundRect(rgn, 190, 190, 200, 200, 20);
	GdSetClipRegion(psd, rgn);
#endif
#if 0
	rgn = GdAllocRegion();
	regionCircle(rgn, 190, 190, 200);
	GdSetClipRegion(psd, rgn);
#endif

#if 0
//GdFillRect(psd, 180, 180, 400, 400);

for (r=80; r>0; --r)
{
	drawCircle(psd, 100, 100, r);
}
	//fasthline(psd, 10, 10, 100);
	//GdFillRect(psd, cw, ch, 100, 100);
	GdLine(psd, 0, 0, w, h, FALSE);

	// AGFX
	GdSetForegroundColor(psd, BLUE);
//fillRoundRect(psd, 180, 180, 200, 200, 50);

	drawLine(psd, 0, 0, w, h);
	//fillRect(psd, 30, 30, 100, 100);
	//fillScreen(psd);
	drawCircle(psd, 100, 100, 80);
	fillCircle(psd, 300, 300, 90);

	//drawRect(psd, cw, ch, 200, 100);
	//drawRoundRect(psd, cw, ch, 200, 100, 8);
	r = min(200, 100) / 4; // Corner radius
	fillRoundRect(psd, cw, ch, 200, 100, r);
	drawTriangle(psd, 30, 180, 150, 250, 120, 450);
	fillTriangle(psd, 80, 180, 150, 250, 120, 450);

GdSetForegroundColor(psd, WHITE);
draw2(psd);
#endif

	// 3d inset on white background
	//GdSetForegroundColor(psd, BLACK);
	GdSetBackgroundColor(psd, WHITE);

	fillRect(psd, 10+2, 10+2, 200-4, 100-4);
	draw3dInsetBox(psd, 10, 10, 200, 100);
	fillRoundRect(psd, 10, 120, 200, 100, 15);
	draw3dInsetRoundRect(psd, 10, 120, 200, 100, 15);

	GdFixCursor(psd);		// for fastvline/fasthline
}

int main(int ac, char **av)
{
	static MWCURSOR cursor = { 16, 16, 0, 0, WHITE, BLACK,
	      { 0xe000, 0x9800, 0x8600, 0x4180,
	      0x4060, 0x2018, 0x2004, 0x107c,
	      0x1020, 0x0910, 0x0988, 0x0544,
	      0x0522, 0x0211, 0x000a, 0x0004 },
	      { 0xe000, 0xf800, 0xfe00, 0x7f80,
		  0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
	      0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
	      0x073e, 0x021f, 0x000e, 0x0004 }
	};
	extern void sleep();

	PSD psd = GdOpenScreenExt(FALSE);	// open with no clear to black
	if (!psd)
		exit(1);
	// clear background only once
	psd->FillRect(psd, 0, 0, psd->xvirtres-1, psd->yvirtres-1, GdFindColor(psd, MWRGB(0, 128, 128)));

#if 0
	// open mouse driver and init cursor
	GdOpenMouse();
	GdShowCursor(psd);
	GdSetCursor(&cursor);
	GdMoveCursor(psd->xvirtres / 2, psd->yvirtres / 2);
	GdRestrictMouse(0, 0, psd->xvirtres - 1, psd->yvirtres - 1);
	GdMoveMouse(psd->xvirtres / 2, psd->yvirtres / 2);
#endif

#if 0
	// set portrait mode
	GdSetPortraitMode(psd, MWPORTRAIT_LEFT);
	// must reset clip region when changing rotation
	GdSetClipRegion(psd, GdAllocRectRegion(0, 0, psd->xvirtres, psd->yvirtres));
#endif

	draw(psd);					// draw stuff

	psd->PreSelect(psd);		// update display
	sleep(120);
	GdCloseScreen(psd);
	GdCloseMouse();
	return 0;
}
