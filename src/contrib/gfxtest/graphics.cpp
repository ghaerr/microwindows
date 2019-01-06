extern "C" {
#include "device.h"
}
#include "graphics.h"

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
 * C++ version
 * 27 Aug 2017 ghaerr
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

// Draw a line using fast vertical or horizontal driver routine, else Bresenham
// always draws last point at x1,y1
void graphics_api::drawLine(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1)
{
	if (x0 == x1) {
		if (y0 > y1) swap(y0, y1);
		drawVLine(x0, y0, y1 - y0 + 1);
		return;
	} else if(y0 == y1) {
		if (x0 > x1) swap(x0, x1);
		drawHLine(x0, y0, x1 - x0 + 1);
		return;
    }

    MWCOORD steep = abs(y1 - y0) > abs(x1 - x0);
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
            drawPoint(y0, x0);
        } else {
            drawPoint(x0, y0);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

// Draw a circle outline
void graphics_api::drawCircle(MWCOORD x0, MWCOORD y0, MWCOORD r)
{
    MWCOORD f = 1 - r;
    MWCOORD ddF_x = 1;
    MWCOORD ddF_y = -2 * r;
    MWCOORD x = 0;
    MWCOORD y = r;

    drawPoint(x0  , y0+r);	// center bottom
    drawPoint(x0  , y0-r);	// center top
    drawPoint(x0+r, y0  );	// right center
    drawPoint(x0-r, y0  );	// left center

    while (x<y) {		// draw arc from top downward stepping x right until 45 degrees
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPoint(x0 + x, y0 + y);	// right center bottom
        drawPoint(x0 - x, y0 + y);	// left center bottom
        drawPoint(x0 + x, y0 - y);	// right center top
        drawPoint(x0 - x, y0 - y);	// left center top
        drawPoint(x0 + y, y0 + x);	// bottom right center
        drawPoint(x0 - y, y0 + x);	// bottom left center
        drawPoint(x0 + y, y0 - x);	// top right center
        drawPoint(x0 - y, y0 - x);	// top left center
    }
}

// Used to fill circles and roundrects
void graphics_api::fillCircleHelper(MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername, MWCOORD delta)
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
            drawVLine(x0+x, y0-y, 2*y+1+delta);	// right center top vertical
            drawVLine(x0+y, y0-x, 2*x+1+delta);	// top right center vertical
        }
        if (cornername & 0x2) {				// left side fill
            drawVLine(x0-x, y0-y, 2*y+1+delta);	// left center top vertical
            drawVLine(x0-y, y0-x, 2*x+1+delta);	// top left center vertical
        }
    }
}

// Yep, fill a circle
void graphics_api::fillCircle(MWCOORD x0, MWCOORD y0, MWCOORD r)
{
    drawVLine(x0, y0-r, 2*r+1);			// center top vertical
    fillCircleHelper(x0, y0, r, 3, 0);		// left and right vertical sides
}

// Fill a rounded rectangle
void graphics_api::fillRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
    // fill interior rect
    fillRect(x+r, y, w-2*r, h);

    // draw four corners
    fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1);	// right vertical side
    fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1);	// left vertical side
}

// used to draw round rects
void graphics_api::drawCircleHelper(MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername)
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
            drawPoint(x0 + x, y0 + y);	// right center bottom
            drawPoint(x0 + y, y0 + x);	// bottom right center
        }
        if (cornername & 0x2) {			// top right corner
            drawPoint(x0 + x, y0 - y);	// right center top
            drawPoint(x0 + y, y0 - x);	// top right center
        }
        if (cornername & 0x8) {			// bottom left corner
            drawPoint(x0 - y, y0 + x);	// bottom left center
            drawPoint(x0 - x, y0 + y);	// left center bottom
        }
        if (cornername & 0x1) {			// top left corner
            drawPoint(x0 - y, y0 - x);	// top left center
            drawPoint(x0 - x, y0 - y);	// left center top
        }
    }
}

// Draw a rounded rectangle
void graphics_api::drawRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
    drawHLine(x+r  , y    , w-2*r); // Top
    drawHLine(x+r  , y+h-1, w-2*r); // Bottom
    drawVLine(x    , y+r  , h-2*r); // Left
    drawVLine(x+w-1, y+r  , h-2*r); // Right

    // draw four corners
    drawCircleHelper(x+r    , y+r    , r, 1);	// top left
    drawCircleHelper(x+w-r-1, y+r    , r, 2);	// top right
    drawCircleHelper(x+w-r-1, y+h-r-1, r, 4);	// bottom right
    drawCircleHelper(x+r    , y+h-r-1, r, 8);	// bottom left
}

// Draw a triangle
void graphics_api::drawTriangle(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2)
{
    drawLine(x0, y0, x1, y1);
    drawLine(x1, y1, x2, y2);
    drawLine(x2, y2, x0, y0);
}

// Fill a triangle
void graphics_api::fillTriangle(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2)
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
        drawHLine(a, y0, b-a+1);
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
        drawHLine(a, y, b-a+1);
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
        drawHLine(a, y, b-a+1);
    }
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
void graphics_api::regionCircleHelper(MWCLIPREGION *rgn, MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername, MWCOORD delta)
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
void graphics_api::regionCircle(MWCLIPREGION *rgn, MWCOORD x0, MWCOORD y0, MWCOORD r)
{
    regionUnionVLine(rgn, x0, y0-r, 2*r+1);
    regionCircleHelper(rgn, x0, y0, r, 3, 0);
}

// Create rounded rectangle region
void graphics_api::regionRoundRect(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
    regionUnionRect(rgn, x+r, y, w-2*r, h);

    // draw four corners
    regionCircleHelper(rgn, x+w-r-1, y+r, r, 1, h-2*r-1);
    regionCircleHelper(rgn, x+r    , y+r, r, 2, h-2*r-1);
}

// draw arc segment only (requires float)
void graphics_api::drawArc(MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle, MWCOORD thickness)
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
				drawLine(px, py, cx, cy);
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
		drawLine(px, py, cx, cy);
	}
}

// draw arc segment and lines to center (requires float)
void graphics_api::drawPie(MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle)
{
	MWCOORD px, py, cx, cy;
	MWCOORD d;

	startAngle -= 90;
	endAngle   -= 90;
	if (startAngle > endAngle)
		startAngle -= 360;
	
	px = x + cos((startAngle*M_PI)/180) * r;
	py = y + sin((startAngle*M_PI)/180) * r;
	drawLine(x, y, px, py);
	for (d=startAngle+1; d<endAngle+1; d++)
	{
		cx = x + cos((d*M_PI)/180) * r;
		cy = y + sin((d*M_PI)/180) * r;
		drawLine(px, py, cx, cy);
		px = cx;
		py = cy;
	}
	drawLine(x, y, px, py);
}

// used to draw 3d inset round rects
void graphics_api::draw3dCircleHelper(MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername,
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
			stroke(crTop);
            drawPoint(x0 + x, y0 + y);	// right center bottom
            drawPoint(x0 + y, y0 + x);	// bottom right center
        }
        if (cornername & 0x2) {			// top right corner
			stroke((x >= y)? crBottom: crTop);
            drawPoint(x0 + x, y0 - y);	// right center top
			stroke((x >= y)? crTop: crBottom);
            drawPoint(x0 + y, y0 - x);	// top right center
        }
        if (cornername & 0x8) {			// bottom left corner
			stroke((x >= y)? crBottom: crTop);
            drawPoint(x0 - y, y0 + x);	// bottom left center
			stroke((x >= y)? crTop: crBottom);
            drawPoint(x0 - x, y0 + y);	// left center bottom
        }
        if (cornername & 0x1) {			// top left corner
			stroke(crTop);
            drawPoint(x0 - y, y0 - x);	// top left center
            drawPoint(x0 - x, y0 - y);	// left center top
        }
    }
}

// Draw a 3d rounded rectangle
void graphics_api::draw3dRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r,
	MWCOLORVAL crTop, MWCOLORVAL crBottom)
{
	stroke(crTop);
    drawHLine(x+r  , y    , w-2*r); // Top
    drawVLine(x    , y+r  , h-2*r); // Left
    draw3dCircleHelper(x+r    , y+r    , r, 1, crTop, crTop);	// top left

	stroke(crBottom);
    drawHLine(x+r  , y+h-1, w-2*r); // Bottom
    drawVLine(x+w-1, y+r  , h-2*r); // Right
    draw3dCircleHelper(x+w-r-1, y+h-r-1, r, 4, crBottom, crBottom);	// bottom right

    draw3dCircleHelper(x+r    , y+h-r-1, r, 8, crTop, crBottom);	// bottom left
    draw3dCircleHelper(x+w-r-1, y+r    , r, 2, crTop, crBottom);	// top right
}

/*
 * draw3dBox
 *
 *	TTTTTTTTTTTTTTB
 *	T             B
 *	T             B
 *	BBBBBBBBBBBBBBB
 */
void graphics_api::draw3dBox(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOLORVAL crTop, MWCOLORVAL crBottom)
{
	//MoveToEx( hDC, x, y+h-2, NULL);
	//LineTo( hDC, x, y);				// left side
	//MoveToEx( hDC, x, y, NULL);
	//LineTo( hDC, x+w-1, y);			// top side

	stroke(crTop);
	drawVLine(x, y+1, h-2);		// left side
	drawHLine(x, y, w-1);			// top side

	//MoveToEx( hDC, x+w-1, y, NULL);
	//LineTo( hDC, x+w-1, y+h-1);		// right side
	//LineTo( hDC, x-1, y+h-1);			// bottom side

	stroke(crBottom);
	drawVLine(x+w-1, y, h-1);		// right side
	drawHLine(x, y+h-1, w);		// bottom side
}

// draw 2 line deep 3d inset box
void graphics_api::draw3dInsetBox(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
{
	MWCOLORVAL BTNSHADOW = MWRGB(162, 141, 104);
	MWCOLORVAL BTNHIGHLIGHT = MWRGB(234, 230, 221);
	MWCOLORVAL WINDOWFRAME = MWRGB(  0,   0,   0);
	MWCOLORVAL C3DLIGHT = MWRGB(213, 204, 187);
	MWPIXELVAL orgPixelVal = gr_foreground;

	draw3dBox(x, y, w, h, BTNSHADOW, BTNHIGHLIGHT);
	++x; ++y; w -= 2; h -= 2;
	draw3dBox(x, y, w, h, WINDOWFRAME, C3DLIGHT);

	gr_foreground = orgPixelVal;
}

// draw 2 line deep 3d inset roundrect
void graphics_api::draw3dInsetRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
{
	MWCOLORVAL BTNSHADOW = MWRGB(162, 141, 104);
	MWCOLORVAL BTNHIGHLIGHT = MWRGB(234, 230, 221);
	MWCOLORVAL WINDOWFRAME = MWRGB(  0,   0,   0);
	MWCOLORVAL C3DLIGHT = MWRGB(213, 204, 187);
	MWPIXELVAL orgPixelVal = gr_foreground;

	draw3dRoundRect(x, y, w, h, r, BTNSHADOW, BTNHIGHLIGHT);
	++x; ++y; w -= 2; h -= 2;
	draw3dRoundRect(x, y, w, h, r-2, WINDOWFRAME, C3DLIGHT);

	gr_foreground = orgPixelVal;
}
