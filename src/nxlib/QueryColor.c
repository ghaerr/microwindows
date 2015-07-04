#include "nxlib.h"

/* convert an X11 pixelval to Microwindows colorval*/
GR_COLOR
_nxColorvalFromPixelval(Display *dpy, unsigned long pixelval)
{
	XColor c;

	/* convert pixelval to colorval*/
	c.pixel = pixelval;
	XQueryColor(dpy, _nxDefaultColormap(dpy), &c);

	return GR_RGB(c.red >> 8, c.green >> 8, c.blue >> 8);
}

static int
queryColor(Display * display, nxColormap * local, XColor * def)
{
	if (def->pixel >= local->cur_color)
		return 0;

	_nxPixel2RGB(display, local->colorval[def->pixel].value, &def->red,
		   &def->green, &def->blue);

	def->flags |= (DoRed | DoGreen | DoBlue);
	return 1;
}

int
XQueryColor(Display * display, Colormap colormap, XColor * def)
{
	nxColormap *local;

	if (XDefaultVisual(display, 0)->class == TrueColor) {
		_nxPixel2RGB(display, def->pixel, &def->red, &def->green,
			   &def->blue);
		return 1;
	}

	if (!(local = _nxFindColormap(colormap)))
		return 0;

	return queryColor(display, local, def);
}

int
XQueryColors(Display * display, Colormap colormap, XColor * def_in,
	     int ncolors)
{
	nxColormap *local;
	int i;

	if (XDefaultVisual(display, 0)->class == TrueColor) {
		for (i = 0; i < ncolors; i++)
			_nxPixel2RGB(display, def_in[i].pixel, &def_in[i].red,
				   &def_in[i].green, &def_in[i].blue);

		return 1;
	}

	if (!(local = _nxFindColormap(colormap)))
		return 0;

	for (i = 0; i < ncolors; i++)
		queryColor(display, local, &def_in[i]);

	return 1;
}
