/*
 * Parse rgb.txt file and return color by name
 */
#include "nxlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uni_std.h"

#define ISBLANK(c)	((c) == ' ' || (c) == '\t')
#define ISDIGIT(c)	((c) >= '0' && (c) <= '9')
#define ISEOL(c)	((c) == '\r' || (c) == '\n')

/* get next int from buffer, return next buf position*/
static char *
strInt(unsigned int *retint, char *buf)
{
	char *	q;
	char	tmp[256];

	while (ISBLANK(*buf))
		++buf;
	q = tmp;
	while (ISDIGIT(*buf))
		*q++ = *buf++;
	*q = 0;
	*retint = atoi(tmp);
	return buf;
}

/* return string to end of line from buffer*/
static void
strEol(char *retbuf, char *buf)
{
	while (ISBLANK(*buf))
		++buf;
	while (!ISEOL(*buf))
		*retbuf++ = *buf++;
	*retbuf = 0;
}

GR_COLOR
GrGetColorByName(char *colorname, int *retr, int *retg, int *retb)
{
	FILE *fp;
	unsigned int r = 0, g = 0, b = 0;
	char buf[256];

	fp = fopen(X11_RGBTXT, "r");
	if (!fp)
		return 0;

	while (fgets(buf, 256, fp) != NULL) {
		if (buf[0] != '!') {
			char *p;
			char name[256];
	
			p = strInt(&r, buf);
			p = strInt(&g, p);
			p = strInt(&b, p);
			strEol(name, p);

			if (strcasecmp(name, colorname) == 0) {
				if (retr)
					*retr = r;
				if (retg)
					*retg = g;
				if (retb)
					*retb = b;
				return GR_RGB(r, g, b);
			}
		}
	}
	fclose(fp);

	return 0;
}

Status
XAllocNamedColor(Display * dpy, Colormap cmap, _Xconst char *colorname,
		 XColor * hard_def, XColor * exact_def)
{
	GR_COLOR c;
	int r = 0, g = 0, b = 0;

	if (!strncmp(colorname, "rgb:", 4)) {
		sscanf(&colorname[4], "%x/%x/%x", &r, &g, &b);
	} else {
		/* first look up color in rgb.txt color database */
		c = GrGetColorByName((char *) colorname, &r, &g, &b);
	}
//DPRINTF("XAllocNamedColor %s = %x\n", colorname, c);

	hard_def->red = exact_def->red = r << 8;
	hard_def->green = exact_def->green = g << 8;
	hard_def->blue = exact_def->blue = b << 8;

	/* Do an XAllocColor on the hardware color */
	XAllocColor(dpy, cmap, hard_def);

	return 1;
}
