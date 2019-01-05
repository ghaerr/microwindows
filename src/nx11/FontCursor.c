#include "nxlib.h"

#define X_CURSOR_FONT "cursor"

Cursor
XCreateFontCursor(Display * display, unsigned int shape)
{
	static XColor fg = { 0, 0, 0, 0};	/* black*/
	static XColor bg = { 0, 65535, 65535, 65535};	/* white*/

	if (_nxCursorFont == None) {
		_nxCursorFont = XLoadFont(display, X_CURSOR_FONT);
		if (_nxCursorFont == None)
			return 0;
	}

	return XCreateGlyphCursor(display, _nxCursorFont, _nxCursorFont,
			shape, shape+1, &fg, &bg);
}

/* FIXME: must play with _Xconst because old Xlib.h proto differs*/
/* Assign an arbitrary char to the cursor */
Cursor
XCreateGlyphCursor(Display * display, Font source_font, Font mask_font,
		   unsigned int source_char, unsigned int mask_char,
		   XColor XCONST * foreground, XColor XCONST * background)
{
	Cursor		ret;
	int		tw[2], th[2], tb[2];
	unsigned char	ch[2];
	GR_GC_ID	gc;
	GR_WINDOW_ID	cursor;
	GR_COLOR	fc, bc;
	GR_RECT		cbb, mbb;
	GR_FONT_INFO	srcinfo, maskinfo;

	gc = GrNewGC();
	/*GrSetGCUseBackground(gc, GR_FALSE);*/ /* assume NewGC defaults TRUE*/
	GrSetGCForeground(gc, GR_RGB(255, 255, 255));
	GrSetGCBackground(gc, GR_RGB(  0,   0,   0));

	/* Draw both the fonts into their appropriate pixmap, and create the cursor */
	GrGetFontInfo((GR_FONT_ID) source_font, &srcinfo);
	GrGetFontInfo((GR_FONT_ID) mask_font, &maskinfo);

	ch[0] = srcinfo.firstchar + source_char;
	ch[1] = maskinfo.firstchar + mask_char;

	/* Use the mask as the determining size */
	GrSetGCFont(gc, (GR_FONT_ID) mask_font);

	GrGetGCTextSize(gc, &ch[0], 1, GR_TFTOP, &tw[0], &th[0], &tb[0]);
	GrGetGCTextSize(gc, &ch[1], 1, GR_TFTOP, &tw[1], &th[1], &tb[1]);

	cursor = GrNewPixmap(tw[1] * 2, th[1], 0);
	//cursor = GrNewPixmapEx(tw[1] * 2, th[1], 0, 0);

	/* Draw the mask first, to avoid having to switch fonts in the GC */
	GrText(cursor, gc, tw[1], 0, &ch[1], 1, GR_TFTOP|GR_TFASCII);

	/* Offset the first char by 1 1 */
	GrSetGCFont(gc, (GR_FONT_ID) source_font);
	GrText(cursor, gc, 1, 1, &ch[0], 1, GR_TFTOP|GR_TFASCII);

	/* Calculate the bounding box */
	cbb.x = 0;
	cbb.y = 0;
	cbb.width = tw[0];
	cbb.height = th[0];

	mbb.x = tw[1];
	mbb.y = 0;
	mbb.width = tw[1];
	mbb.height = th[1];

	fc = GR_RGB(foreground->red >> 8, foreground->green >> 8,
			foreground->blue >> 8);
	bc = GR_RGB(background->red >> 8, background->green >> 8,
			background->blue >> 8);
	/* cursor hotspot is (leftbearing, ascent)*/
	ret = _nxCreateCursor(cursor, &cbb, cursor, &mbb, 0, tb[1],
			fc, bc);

	GrDestroyWindow(cursor);
	GrDestroyGC(gc);
	return ret;
}
