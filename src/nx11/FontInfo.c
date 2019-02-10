//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"
#include <stdlib.h>

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
