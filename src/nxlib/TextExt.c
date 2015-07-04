#include <stdio.h>
#include <string.h>
#include "nxlib.h"

static int
_nxTextWidth(XFontStruct * font, void *string, int count, int flag)
{
	int w, h, b;
	GR_GC_ID local = GrNewGC();
	GrSetGCFont(local, font->fid);

	GrGetGCTextSize(local, (void *) string, count, flag, &w, &h, &b);

	GrDestroyGC(local);
	return (w);
}

int
XTextWidth(XFontStruct * font, _Xconst char *string, int count)
{
	return _nxTextWidth(font, (void *) string, count, GR_TFASCII);
}

int
XTextWidth16(XFontStruct * font, _Xconst XChar2b * string, int count)
{
	return _nxTextWidth(font, (void *) string, count, GR_TFXCHAR2B);
}

static int
_nxTextExtents(XFontStruct * font, void *string, int count,
	int *dir, int *ascent, int *descent, XCharStruct * overall, int flag)
{
	int w, h, b;
	GR_GC_ID local = GrNewGC();

	GrSetGCFont(local, font->fid);
	GrGetGCTextSize(local, (void *) string, count, flag, &w, &h, &b);
	GrDestroyGC(local);

	*ascent = b;
	*descent = h - b;
	*dir = FontLeftToRight;

	overall->lbearing = 0;	/* Figure this out */
	overall->rbearing = w;	/* Figure this out */
	overall->width = w;
	overall->ascent = b;
	overall->descent = *descent;
	overall->attributes = 0;	/* FIXME? */
	return 1;
}

int
XTextExtents(XFontStruct * font, _Xconst char *string, int count,
	int *dir, int *ascent, int *descent, XCharStruct * overall)
{
	return _nxTextExtents(font, (void *) string, count, dir, ascent,
			      descent, overall, GR_TFASCII);
}

int
XTextExtents16(XFontStruct * font, _Xconst XChar2b * string, int count,
	int *dir, int *ascent, int *descent, XCharStruct * overall)
{
	return _nxTextExtents(font, (void *) string, count, dir, ascent,
			      descent, overall, GR_TFXCHAR2B);
}
