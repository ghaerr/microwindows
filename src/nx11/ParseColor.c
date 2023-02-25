#include "nxlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parse colors of format:
 * #RGB #RRGGBB #RRRRGGGGBBBB
 */
static unsigned long
_parseColorStr(_Xconst char **str, int size)
{
	unsigned long val;
	char parse[5];

	strncpy(parse, *str, size);
	parse[size] = '\0';
	val = strtol(parse, 0, 16);
	*str += size;

	return val;
}

Status
XParseColor(Display * display, Colormap colormap, _Xconst char *spec,
	XColor *exact)
{
	int r, g, b;

	/* This is the new and preferred way */
	if (strncmp(spec, "rgb:", 4) == 0) {
		sscanf(spec + 4, "%x/%x/%x", &r, &g, &b);
	} else {
		if (spec[0] != '#') {
			/* try to parse the color name */
			if (GrGetColorByName((char *) spec, &r, &g, &b) == 0) {
				DPRINTF("XParseColor: passed '%s' is unknown color name\n", spec);
				return 0;
			}
		} else {
			_Xconst char *p = spec + 1;
			unsigned long val;

			switch (strlen(p)) {
			case 3:		/* #RGB*/
				r = _parseColorStr(&p, 1);
				g = _parseColorStr(&p, 1);
				b = _parseColorStr(&p, 1);
				break;

			case 6:		/* #RRGGBB*/
				val = strtol(p, 0, 16);
				r = (val >> 16) & 0xFF;
				g = (val >> 8) & 0xFF;
				b = (val & 0xFF);
				break;

			case 12:	/* #RRRRGGGGBBBB*/
				r = _parseColorStr(&p, 4) >> 8;
				g = _parseColorStr(&p, 4) >> 8;
				b = _parseColorStr(&p, 4) >> 8;
				break;

			default:
				DPRINTF("XParseColor: passed '%s' is invalid format\n", spec);
				return 0;
			}
		}
	}

	exact->red = r << 8;
	exact->green = g << 8;
	exact->blue = b << 8;
	exact->flags |= (DoRed | DoGreen | DoBlue);
	return 1;
}

Status
XLookupColor(Display * display, Colormap colormap, _Xconst char *spec,
	     XColor * exact, XColor * screen)
{
	Status stat = XParseColor(display, colormap, spec, exact);

	if (!stat)
		return stat;

	/* FIXME:  Should do a system look up for the right color */
	/* This will come back and haunt you on palettized machines */

	screen->red = exact->red;
	screen->green = exact->green;
	screen->blue = exact->blue;

	screen->flags |= (DoRed | DoGreen | DoBlue);

	return stat;
}
