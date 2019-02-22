/* extra_winfont.h*/
#pragma once
#include <math.h> 

typedef struct tagWCRANGE {
	WCHAR  wcLow;
	USHORT cGlyphs;
} WCRANGE, *PWCRANGE, *LPWCRANGE;

typedef struct tagGLYPHSET {
	DWORD   cbThis;
	DWORD   flAccel;
	DWORD   cGlyphsSupported;
	DWORD   cRanges;
	WCRANGE ranges[1];
} GLYPHSET, *PGLYPHSET, *LPGLYPHSET;

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static inline INT GDI_ROUND(double val)
{
	return (int)floor(val + 0.5);
}

/* Device -> World size conversion */

/* Performs a device to world transformation on the specified width (which
 * is in integer format).
 */
static inline INT INTERNAL_XDSTOWS(HDC dc, INT width)
{
	double floatWidth;

	/* Perform operation with floating point */
	floatWidth = (double)width * dc->xformVport2World.eM11;
	/* Round to integers */
	return GDI_ROUND(floatWidth);
}

/* Performs a device to world transformation on the specified size (which
 * is in integer format).
 */
static inline INT INTERNAL_YDSTOWS(HDC dc, INT height)
{
	double floatHeight;

	/* Perform operation with floating point */
	floatHeight = (double)height * dc->xformVport2World.eM22;
	/* Round to integers */
	return GDI_ROUND(floatHeight);
}

/* scale width and height but don't mirror them */

static inline INT width_to_LP(HDC dc, INT width)
{
	return GDI_ROUND((double)width * fabs(dc->xformVport2World.eM11));
}

static inline INT height_to_LP(HDC dc, INT height)
{
	double em22 = fabs((double)dc->xformVport2World.eM22);
	return GDI_ROUND((double)height * em22);
}

HFONT WINAPI CreateFontIndirectW(CONST LOGFONTW *lplf);
