//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"

int XFreeFontInfo(char **names, XFontStruct *info, int actualCount)
{
	int i;
	if (names) {
		Xfree(names[0]-1);
		for (i = 1; i < actualCount; i++) {
			Xfree(names[i]);
		}
		Xfree((char*)names);
	}
	if (info) {
		for (i = 0; i < actualCount; i++) {
			if (info[i].per_char)
				Xfree((char*)info[i].per_char);
			if (info[i].properties)
				Xfree((char*)info[i].properties);
		}
		Xfree((char*)info);
	}
	return 1;
}

#include "X11/Xlcint.h"
#include "X11/Xlib.h"
XFontSetExtents *XExtentsOfFontSet(XFontSet font_set)
{
	//DPRINTF("XExtentsOfFontSet called...\n");
	if (!font_set) return NULL;
	return &font_set->core.font_set_extents;
}
