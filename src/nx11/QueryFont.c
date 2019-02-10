#include "nxlib.h"
#include "X11/Xatom.h"
#include <stdlib.h>

XFontStruct *
XLoadQueryFont(Display * dpy, _Xconst char *name)
{
	Font f = XLoadFont(dpy, name);
	return XQueryFont(dpy, f);
}

XFontStruct *
XQueryFont(Display * dpy, XID font_ID)
{
	int i, size;
	XFontStruct *fs = (XFontStruct *) Xcalloc(1, sizeof(XFontStruct));
	XCharStruct *cs;
	GR_FONT_INFO finfo;

	GrGetFontInfo(font_ID, &finfo);

	fs->fid = font_ID;

	/*
	 * Microwindows doesn't support rbearing, so we
	 * set it to the width component.  We also don't
	 * keep track of min bounds, so we set it the same
	 * as the max bounds. FIXME?
	 */
	fs->max_bounds.rbearing = finfo.maxwidth;
	fs->max_bounds.width = finfo.maxwidth;
	fs->max_bounds.ascent = finfo.baseline;
	fs->max_bounds.descent = finfo.height - finfo.baseline;
	fs->min_bounds.rbearing = finfo.maxwidth;
	fs->min_bounds.width = finfo.maxwidth;
	fs->min_bounds.ascent = finfo.baseline;
	fs->min_bounds.descent = finfo.height - finfo.baseline;

	fs->direction = FontLeftToRight;
	fs->ascent = finfo.baseline;
	fs->descent = finfo.height - finfo.baseline;
	//fs->all_chars_exist = ?

	fs->min_char_or_byte2 = finfo.firstchar & 255;
	fs->max_char_or_byte2 = finfo.lastchar & 255;
	fs->min_byte1 = finfo.firstchar >> 8;
	fs->max_byte1 = finfo.lastchar >> 8;
	fs->default_char = finfo.firstchar;

	/* 
	 * Allocate XCharStruct array.  The array starts
	 * at the first encoded character glyph (0-based).
	 */
	size = finfo.lastchar - finfo.firstchar + 1;

	/* temp limit to 256 chars FIXME*/
	if (size > 256)
		size = 256;
	fs->per_char = cs = (XCharStruct *)Xmalloc(size * sizeof(XCharStruct));
	if (cs) {
		for (i=finfo.firstchar; i<=finfo.lastchar; ++i) {
			if (i-finfo.firstchar >= size)
				break;
			cs->lbearing = 0;
			/* FIXME, we need a real widths table*/
			cs->rbearing = (finfo.fixed || i>255)? finfo.maxwidth: finfo.widths[i];
			cs->width = cs->rbearing;	/* FIXME*/
			cs->ascent = finfo.baseline;
			cs->descent = finfo.height - finfo.baseline;
			cs->attributes = 0;
			++cs;
		}
	}

	/* no font properties yet*/

//DPRINTF("Font %d byte1 %d,%d byte2 %d,%d\n", font_ID, fs->min_byte1, fs->max_byte1, fs->min_char_or_byte2, fs->max_char_or_byte2);

	return fs;
}

int
XFreeFont(Display *display, XFontStruct *font_struct)
{
	GrDestroyFont(font_struct->fid);

	if (font_struct->per_char)
		Xfree(font_struct->per_char);
	Xfree(font_struct);
	return 1;
}
