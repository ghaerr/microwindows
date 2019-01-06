extern "C" {
#include "device.h"
}
#include <math.h>		// float required for sin/cos in drawArc/drawPie
#include "agg.h"		// for aggprocessing class only

/*
 * AdaFruit AGFX compatible draw routines for Microwindows
 * Processing AGG compatible draw routines for Microwindows
 *
 *  6 Aug 2017 Initial Port - Draw & Fill Circles, RoundRects, Triangles
 *  7 Aug 2017 Add Circle and RoundRect Regions
 * 23 Aug 2017 Add 3d Inset Rects and RoundRects
 * 27 Aug 2017 Raster graphics & processing classes
 * 28 Aug 2018 Agg processing classes
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

// RASTER portable graphics class across OS & GFX library
class graphics_api
{
public:
	// derived class must implement these
	virtual void drawPoint(MWCOORD x, MWCOORD y) {}
	virtual void fill(MWCOLORVAL rgbColor) {}
	virtual void stroke(MWCOLORVAL rgbColor) {}
	virtual void fillScreen() {} 	// needs width/height to implement here

	// following routines should be implemented in derived class for speed
	virtual void drawHLine(MWCOORD x, MWCOORD y, MWCOORD w)
	{
		while (--w >= 0)
			drawPoint(x++, y);
	}

	virtual void drawVLine(MWCOORD x, MWCOORD y, MWCOORD h)
	{
		while (--h >= 0)
			drawPoint(x, y++);
	}

	virtual void drawRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
	{
   		drawHLine(x, y, w);
   		drawHLine(x, y+h-1, w);
   		drawVLine(x, y, h);
   		drawVLine(x+w-1, y, h);
	}

	virtual void fillRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
	{
   		//while (--w >= 0)
       		//drawVLine(x++, y, h);		// note slow vertical fill, but matches circle fill
   		while (--h >= 0)
       		drawHLine(x, y++, w);
	}

	virtual void update() {}

	// draw routines
	void drawLine(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1);
	void drawCircle(MWCOORD x0, MWCOORD y0, MWCOORD r);
	void fillCircle(MWCOORD x0, MWCOORD y0, MWCOORD r);
	void drawRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
	void fillRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
	void drawTriangle(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2);
	void fillTriangle(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2);
	void draw3dInsetBox(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h);
	void draw3dInsetRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
	// region routines FIXME move to another file
	void regionCircle(MWCLIPREGION *rgn, MWCOORD x0, MWCOORD y0, MWCOORD r);
	void regionRoundRect(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r);
	// float required for these
	void drawArc(MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle, MWCOORD thickness);
	void drawPie(MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle);

private:
	void fillCircleHelper(MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername, MWCOORD delta);
	void drawCircleHelper(MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername);
	void regionCircleHelper(MWCLIPREGION *rgn, MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername, MWCOORD delta);
	void draw3dCircleHelper(MWCOORD x0, MWCOORD y0, MWCOORD r, int cornername,
		MWCOLORVAL crTop, MWCOLORVAL crBottom);
	void draw3dRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r,
		MWCOLORVAL crTop, MWCOLORVAL crBottom);
	void draw3dBox(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOLORVAL crTop, MWCOLORVAL crBottom);

	inline void swap(MWCOORD &a, MWCOORD &b)
	{
		MWCOORD t = a; a = b; b = t;
	}
};

// RASTER graphics class based on Microwindows
class graphics : public graphics_api
{
public:
	// constructor
	graphics() {}
	~graphics() {}
	
	void init()
	{
		psd = &scrdev;
		// default stroke and fill
		stroke(WHITE);
		fill(BLACK);

		// set portrait mode
		//GdSetPortraitMode(psd, MWPORTRAIT_LEFT);
		// must reset clip region when changing rotation
		//GdSetClipRegion(psd, GdAllocRectRegion(0, 0, psd->xvirtres, psd->yvirtres));
	}

	// update display and fix cursor
	virtual void update()
	{
		GdFixCursor(psd);				// for drawVLine/drawHLine
		psd->PreSelect(psd);
	}

	void fill(MWCOLORVAL rgbColor)
	{
		bkColor = rgbColor;		// save for later fills, drawCircle/Triangle etc broken!
		GdSetBackgroundColor(psd, bkColor);		// not used except by font draw
	}

	void stroke(MWCOLORVAL rgbColor)
	{
		fgColor = rgbColor;
		GdSetForegroundColor(psd, rgbColor);
	}

	// draw a point
	inline virtual void drawPoint(MWCOORD x, MWCOORD y)
	{
		GdPoint(psd, x, y);
	}

	// draw horizontal line
	virtual void drawHLine(MWCOORD x, MWCOORD y, MWCOORD w)
	{
		drawrow(psd, x, x+w-1, y);
	}

	// draw vertical line
	virtual void drawVLine(MWCOORD x, MWCOORD y, MWCOORD h)
	{
		drawcol(psd, x, y, y+h-1);
	}

	// draw a rectangle
	virtual void drawRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
	{
   		GdRect(psd, x, y, w, h);
	}

	// fill a rectangle
	virtual void fillRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
	{
		MWPIXELVAL save = GdSetForegroundPixelVal(psd, gr_background);

   		GdFillRect(psd, x, y, w, h);

		GdSetForegroundPixelVal(psd, save);
	}

	// fill entire screen
	virtual void fillScreen(void)
	{
   		GdFillRect(psd, 0, 0, psd->xvirtres, psd->yvirtres);
	}

	// following routines need background color switched into foreground
	void fillCircle(MWCOORD x0, MWCOORD y0, MWCOORD r)
	{
		MWPIXELVAL save = GdSetForegroundPixelVal(psd, gr_background);

   		graphics_api::fillCircle(x0, y0, r);

		GdSetForegroundPixelVal(psd, save);
	}

	void fillRoundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
	{
		MWPIXELVAL save = GdSetForegroundPixelVal(psd, gr_background);

   		graphics_api::fillRoundRect(x, y, w, h, r);

		GdSetForegroundPixelVal(psd, save);
	}

	void fillTriangle(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2)
	{
		MWPIXELVAL save = GdSetForegroundPixelVal(psd, gr_background);

   		graphics_api::fillTriangle(x0, y0, x1, y1, x2, y2);

		GdSetForegroundPixelVal(psd, save);
	}

private:
	PSD 		psd;
	MWCOLORVAL	bkColor;
	MWCOLORVAL	fgColor;
};

// RASTER processing style graphics class
class processing : public graphics
{
public:
	// constructor
	processing() {}
	~processing() {}

	void init()
	{
		graphics::init();
		stroke(BLACK);
		fill(WHITE);
	}

	// color management
	void fill(MWCOLORVAL rgbColor)
	{
		m_dofill = true;
		graphics::fill(rgbColor);
	}

	void noFill()
	{
		m_dofill = false;
	}

	void stroke(MWCOLORVAL rgbColor)
	{
		m_dostroke = true;
		graphics::stroke(rgbColor);
	}

	void noStroke()
	{
		m_dostroke = false;
	}

	void strokeWeight(MWCOORD thickness)
	{
		// ignored in raster class
	}

	// draw a point
	void point(MWCOORD x, MWCOORD y)
	{
		drawPoint(x, y);
	}

	// draw a horizontal, vertical or bresenham line
	void line(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1)
	{
		drawLine(x0, y0, x1, y1);
	}

	// draw a rectangle, circle, roundRect or triangle stroked or filled
	void rect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h)
	{
		if (m_dofill)
   			fillRect(x, y, w, h);
		if (m_dostroke)
			drawRect(x, y, w, h);
	}

	void circle(MWCOORD x, MWCOORD y, MWCOORD r)
	{
		if (m_dofill)
   			fillCircle(x, y, r);
		if (m_dostroke)
			drawCircle(x, y, r);
	}

	void ellipse(MWCOORD x, MWCOORD y, MWCOORD rx, MWCOORD ry)
	{
		//FIXME raster always draws circle
		if (m_dofill)
   			fillCircle(x, y, rx);
		if (m_dostroke)
			drawCircle(x, y, rx);
	}

	void roundRect(MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, MWCOORD r)
	{
		if (m_dofill)
   			fillRoundRect(x, y, w, h, r);
		if (m_dostroke)
			drawRoundRect(x, y, w, h, r);
	}

	void triangle(MWCOORD x0, MWCOORD y0, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2)
	{
		// buggy: fill and stroke aren't matched!!
		if (m_dofill)
   			fillTriangle(x0, y0, x1, y1, x2, y2);
		if (m_dostroke)
			drawTriangle(x0, y0, x1, y1, x2, y2);
	}

	// float required for these
	void arc(MWCOORD x, MWCOORD y, MWCOORD r, MWCOORD startAngle, MWCOORD endAngle)
	{
		if (m_dofill)			// no real fill on arcs, just draw pie segments (twice)
			drawPie(x, y, r, startAngle, endAngle);
		if (m_dostroke)
   			drawArc(x, y, r, startAngle, endAngle, 1);	// thickness always 1
	}

private:
	bool m_dostroke;
	bool m_dofill;
};

// AGG processing style graphics class
// FIXME no clipping in agglite!!
// FIXME add noSmooth() and call graphics_api
class aggprocessing
{
	typedef double	FCOORD;
private:
	PSD		psd;
	FCOORD	m_thickness;
	bool	m_dostroke;
	bool	m_dofill;
	agg::rgba8	m_strokeColor;
	agg::rgba8	m_fillColor;
	agg::rendering_buffer rbuf;
	agg::renderer<agg::span_rgba32> ren;
	agg::rasterizer ras;

public:
	MWCOORD	width;
	MWCOORD height;

	// constructor
	aggprocessing() {}
	~aggprocessing() {}

	void init()
	{
		psd = &scrdev;
		width = psd->xvirtres;
		height = psd->yvirtres;

		// default stroke and fill
		strokeWeight(1.0);
		stroke(BLACK);
		fill(WHITE);

		rbuf.attach(psd->addr, width, height, width * 4);
		ren.attach(rbuf);

		// Setup the rasterizer
		ras.gamma(1.3);
		ras.filling_rule(agg::fill_even_odd);
		//ren.clear(agg::rgba8(255, 255, 255));
	}

	void update()
	{
		// restore cursor
		GdFixCursor(psd);

		// mark entire display for transfer
		psd->Update(psd, 0, 0, width, height);

    	// send image to display
		psd->PreSelect(psd);
	}

	// color management
	void fill(MWCOLORVAL rgbColor)
	{
		m_dofill = true;
		m_fillColor = unsigned(rgbColor);
	}

	void noFill()
	{
		m_dofill = false;
	}

	void stroke(MWCOLORVAL rgbColor)
	{
		m_dostroke = true;
		m_strokeColor = unsigned(rgbColor);
	}

	void noStroke()
	{
		m_dostroke = false;
	}

	void strokeWeight(FCOORD thickness)
	{
		m_thickness = thickness;
	}

	// draw a point
	void point(MWCOORD x, MWCOORD y)
	{
		if (!m_dostroke)
			return;
		GdCheckCursor(psd, x, y, x, y);
		//fillRect(x, y, 1.0, 1.0);
		const FCOORD w = 1.0, h = 1.0;
   		ras.move_to_d(x, y);
		ras.line_to_d(x+w, y);
   		ras.line_to_d(x+w, y+h);
		ras.line_to_d(x, y+h);
   		ras.line_to_d(x, y);
		GdEraseCursor(psd);
        ras.render(ren, m_strokeColor);
	}

	// draw a line
	void line(FCOORD x1, FCOORD y1, FCOORD x2, FCOORD y2)
	{
		if (!m_dostroke)
			return;
    	FCOORD dx = x2 - x1;
    	FCOORD dy = y2 - y1;
    	FCOORD d = sqrt(dx*dx + dy*dy);
    
    	dx = m_thickness * (y2 - y1) / d;
    	dy = m_thickness * (x2 - x1) / d;

    	ras.move_to_d(x1 - dx,  y1 + dy);
    	ras.line_to_d(x2 - dx,  y2 + dy);
    	ras.line_to_d(x2 + dx,  y2 - dy);
    	ras.line_to_d(x1 + dx,  y1 - dy);
		GdEraseCursor(psd);
        ras.render(ren, m_strokeColor);
	}

	// draw a rectangle, circle, roundRect or triangle stroked or filled
	void rect(FCOORD x, FCOORD y, FCOORD w, FCOORD h)
	{
		if (m_dofill)
   			fillRect(x, y, w, h);
		if (m_dostroke)
			strokeRect(x, y, w, h);
	}

private:
	void strokeRect(FCOORD x, FCOORD y, FCOORD w, FCOORD h)
	{
		// draw outer rect
		x -= m_thickness / 2.0;
		y -= m_thickness / 2.0;
		w += m_thickness;
		h += m_thickness;
   		ras.move_to_d(x, y);
		ras.line_to_d(x+w, y);
   		ras.line_to_d(x+w, y+h);
		ras.line_to_d(x, y+h);
   		ras.line_to_d(x, y);

    	// draw inner rect in opposite direction
    	x += m_thickness;
    	y += m_thickness;
		w -= m_thickness * 2.0;
		h -= m_thickness * 2.0;
   		ras.move_to_d(x, y);
		ras.line_to_d(x, y+h);
		ras.line_to_d(x+w, y+h);
		ras.line_to_d(x+w, y);
		ras.line_to_d(x, y);
		GdEraseCursor(psd);
		ras.render(ren, m_strokeColor);
	}

	void fillRect(FCOORD x, FCOORD y, FCOORD w, FCOORD h)
	{
   		ras.move_to_d(x, y);
		ras.line_to_d(x+w, y);
   		ras.line_to_d(x+w, y+h);
		ras.line_to_d(x, y+h);
   		ras.line_to_d(x, y);
		GdEraseCursor(psd);
        ras.render(ren, m_fillColor);
	}

public:
	void circle(FCOORD x, FCOORD y, FCOORD rx)
	{
		if (m_dofill)
   			fillEllipse(x, y, rx, rx);
		if (m_dostroke)
			strokeEllipse(x, y, rx, rx);
	}

	void ellipse(FCOORD x, FCOORD y, FCOORD rx, FCOORD ry)
	{
		if (m_dofill)
   			fillEllipse(x, y, rx, ry);
		if (m_dostroke)
			strokeEllipse(x, y, rx, ry);
	}

private:
	void strokeEllipse(FCOORD x, FCOORD y, FCOORD rx, FCOORD ry)
	{
    	int		i;

    	// draw outer circle
    	rx += m_thickness / 2.0;
    	ry += m_thickness / 2.0;
    	ras.move_to_d(x + rx, y);

    	for(i = 1; i < 360; i++)
    	{
        	FCOORD a = FCOORD(i) * M_PI / 180.0;
        	ras.line_to_d(x + cos(a) * rx, y + sin(a) * ry);
    	}

    	// draw inner circle in opposite direction
    	rx -= m_thickness;
    	ry -= m_thickness;
    	ras.move_to_d(x + rx, y);

    	for(i = 359; i > 0; i--)
    	{
        	FCOORD a = FCOORD(i) * M_PI / 180.0;
        	ras.line_to_d(x + cos(a) * rx, y + sin(a) * ry);
    	}
		GdEraseCursor(psd);
        ras.render(ren, m_strokeColor);
	}

	void fillEllipse(FCOORD x, FCOORD y, FCOORD rx, FCOORD ry)
	{
    	ras.move_to_d(x + rx, y);

    	// fixed number of approximation steps, could be smarter
    	for(int i = 1; i < 360; i++)
    	{
        	FCOORD a = FCOORD(i) * M_PI / 180.0;
        	ras.line_to_d(x + cos(a) * rx, y + sin(a) * ry);
    	}
		GdEraseCursor(psd);
        ras.render(ren, m_fillColor);
	}

public:
	void roundRect(FCOORD x, FCOORD y, FCOORD w, FCOORD h, FCOORD r)
	{
		if (m_dofill)
   			fillRoundRect(x, y, w, h, r);
		if (m_dostroke)
			strokeRoundRect(x, y, w, h, r);
	}

private:
	void drawCircleHelper(FCOORD x, FCOORD y, FCOORD r, int startAngle, int endAngle)
	{
    	for(int i = startAngle; i < endAngle; i++)
    	{
        	FCOORD a = FCOORD(i) * M_PI / 180.0;
        	ras.line_to_d(x + cos(a) * r, y + sin(a) * r);
    	}
	}

	void drawCircleHelper2(FCOORD x, FCOORD y, FCOORD r, int startAngle, int endAngle)
	{
    	for(int i = endAngle; i > startAngle; i--)
    	{
        	FCOORD a = FCOORD(i) * M_PI / 180.0;
        	ras.line_to_d(x + cos(a) * r, y + sin(a) * r);
    	}
	}

	void rightRoundRect(FCOORD x, FCOORD y, FCOORD w, FCOORD h, FCOORD r)
	{
    	ras.move_to_d(x+r  , y    ); 						// top horz
		ras.line_to_d(x+r+w-2*r, y);
    	drawCircleHelper(x+w-r, y+r    , r, 270, 360);		// top right

    	ras.line_to_d(x+w, y+r    ); 						// right vert
		ras.line_to_d(x+w, y+r+h-2*r);
    	drawCircleHelper(x+w-r, y+h-r, r, 0, 90);			// bottom right

    	ras.line_to_d(x+r+w-2*r  , y+h); 					// bottom horz
		ras.line_to_d(x+r,         y+h);
    	drawCircleHelper(x+r    , y+h-r, r, 90, 180);		// bottom left

    	ras.line_to_d(x    , y+r+h-2*r); 					// left vert
		ras.line_to_d(x    , y+r);
    	drawCircleHelper(x+r    , y+r    , r, 180, 270);	// top left

	}

	void leftRoundRect(FCOORD x, FCOORD y, FCOORD w, FCOORD h, FCOORD r)
	{
    	ras.move_to_d(x    , y+r    ); 						// left vert
		ras.line_to_d(x    , y+r+h-r*2);
    	drawCircleHelper2(x+r    , y+h-r, r, 90, 180);		// bottom left

		ras.line_to_d(x+r       , y+h);						// bottom horz
		ras.line_to_d(x+r+w-2*r , y+h);
    	drawCircleHelper2(x+w-r, y+h-r, r, 0, 90);			// bottom right

		ras.line_to_d(x+w, y+r+h-2*r);						// right vert
		ras.line_to_d(x+w, y+r);
    	drawCircleHelper2(x+w-r, y+r    , r, 270, 360);		// top right

		ras.line_to_d(x+r+w-2*r, y);						// top horz
		ras.line_to_d(x+r, y);
    	drawCircleHelper2(x+r    , y+r    , r, 180, 270);	// top left
	}

public:
	void fillRoundRect(FCOORD x, FCOORD y, FCOORD w, FCOORD h, FCOORD r)
	{
		rightRoundRect(x, y, w, h, r);
		GdEraseCursor(psd);
        ras.render(ren, m_fillColor);
	}

	void strokeRoundRect(FCOORD x, FCOORD y, FCOORD w, FCOORD h, FCOORD r)
	{
		// draw outer rect
		x -= m_thickness / 2.0;
		y -= m_thickness / 2.0;
		w += m_thickness;
		h += m_thickness;
		//r += m_thickness;
		leftRoundRect(x, y, w, h, r);

    	// draw inner rect in opposite direction
    	x += m_thickness;
    	y += m_thickness;
		w -= m_thickness * 2.0;
		h -= m_thickness * 2.0;
		//r -= m_thickness * 2.0;
		rightRoundRect(x, y, w, h, r);
		GdEraseCursor(psd);
        ras.render(ren, m_strokeColor);
	}

	void triangle(FCOORD x0, FCOORD y0, FCOORD x1, FCOORD y1, FCOORD x2, FCOORD y2)
	{
		if (m_dofill)
		{
   			ras.move_to_d(x0, y0);
			ras.line_to_d(x1, y1);
   			ras.line_to_d(x2, y2);
   			ras.line_to_d(x0, y0);
			GdEraseCursor(psd);
        	ras.render(ren, m_fillColor);
		}
		if (m_dostroke)
		{
			line(x0, y0, x1, y1);
			line(x1, y1, x2, y2);
			line(x2, y2, x0, y0);
		}
	}

	// draw a filled pie with possible stroking or just an arc if noFill
	// note no thickness argument
	void arc(FCOORD x, FCOORD y, FCOORD r, int startAngle, int endAngle)
	{
		// draw filled pie
		if (m_dofill)
		{
			ras.move_to_d(x, y);
    		for(int i = startAngle; i <= endAngle; i++)
    		{
        		FCOORD a = FCOORD(i) * M_PI / 180.0;
        		ras.line_to_d(x + cos(a) * r, y + sin(a) * r);
    		}
			ras.line_to_d(x, y);
			GdEraseCursor(psd);
        	ras.render(ren, m_fillColor);
		}
		// draw arc
		if (m_dostroke) {
			bool first = true;
			FCOORD x1, y1, x2, y2;

			// draw outer arc
			x -= m_thickness / 2.0;
			y -= m_thickness / 2.0;
			//ras.line_to_d(x, y);
    		for(int i = startAngle; i <= endAngle; i++)
    		{
        		FCOORD a = FCOORD(i) * M_PI / 180.0;
				if (first)
				{
        			 x1 = x + cos(a) * r;
					 y1 = y + sin(a) * r;
        			 ras.move_to_d(x1, y1);
					 first = false;
				}
        		else ras.line_to_d(x + cos(a) * r, y + sin(a) * r);
    		}
			//ras.line_to_d(x, y);

    		// draw inner rect in opposite direction
			first = true;
    		x += m_thickness;
    		y += m_thickness;
    		for(int i = endAngle; i >= startAngle; i--)
    		{
        		FCOORD a = FCOORD(i) * M_PI / 180.0;
        		ras.line_to_d(x + cos(a) * r, y + sin(a) * r);
    		}
			ras.line_to_d(x1, y1);

			// if filling, stroke full pie otherwise arc only is drawn
			if (m_dofill)
			{
				line(x, y, x1, y1);
				line(x, y, x+cos(endAngle*M_PI/180.0)*r, y+sin(endAngle*M_PI/180.0)*r);
			}
			GdEraseCursor(psd);
        	ras.render(ren, m_strokeColor);
		}
	}
};
