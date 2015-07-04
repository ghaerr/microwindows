#include "nxlib.h"
#include <stdlib.h>
#include <string.h>

/*
 * Conversion from RGB to MWPIXELVAL
 */
/* create 24 bit 8/8/8 format pixel (0x00RRGGBB) from RGB triplet*/
#define RGB2PIXEL888(r,g,b)	\
	(((r) << 16) | ((g) << 8) | (b))

/* create 16 bit 5/6/5 format pixel from RGB triplet */
#define RGB2PIXEL565(r,g,b)	\
	((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

/* create 16 bit 5/5/5 format pixel from RGB triplet */
#define RGB2PIXEL555(r,g,b)	\
	((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3))

/* create 8 bit 3/3/2 format pixel from RGB triplet*/
#define RGB2PIXEL332(r,g,b)	\
	(((r) & 0xe0) | (((g) & 0xe0) >> 3) | (((b) & 0xc0) >> 6))

/*
 * Conversion from MWPIXELVAL to red, green or blue components
 */
/* return 8/8/8 bit r, g or b component of 24 bit pixelval*/
#define PIXEL888RED(pixelval)		(((pixelval) >> 16) & 0xff)
#define PIXEL888GREEN(pixelval)		(((pixelval) >> 8) & 0xff)
#define PIXEL888BLUE(pixelval)		((pixelval) & 0xff)

/* return 5/6/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL565RED(pixelval)		(((pixelval) >> 11) & 0x1f)
#define PIXEL565GREEN(pixelval)		(((pixelval) >> 5) & 0x3f)
#define PIXEL565BLUE(pixelval)		((pixelval) & 0x1f)

/* return 5/5/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL555RED(pixelval)		(((pixelval) >> 10) & 0x1f)
#define PIXEL555GREEN(pixelval)		(((pixelval) >> 5) & 0x1f)
#define PIXEL555BLUE(pixelval)		((pixelval) & 0x1f)

/* return 3/3/2 bit r, g or b component of 8 bit pixelval*/
#define PIXEL332RED(pixelval)		(((pixelval) >> 5) & 0x07)
#define PIXEL332GREEN(pixelval)		(((pixelval) >> 2) & 0x07)
#define PIXEL332BLUE(pixelval)		((pixelval) & 0x03)

/* return a pixel value's component r, g, b parts*/
void
_nxPixel2RGB(Display * display, unsigned long color,
	   unsigned short *red, unsigned short *green, unsigned short *blue)
{
	/* the PIXELxxx macros below only return the actual color bits
	 * in the pixelval.  Thus, they need left shifting
	 * in order to become a normal r,g,b value.  In addition,
	 * they need to be shifted left another 8 bits to become
	 * X11 compatible colors.
	 */
	switch (display->screens[0].root_depth) {
	case 8:
		*red = PIXEL332RED(color) << (8 + 5);
		*green = PIXEL332GREEN(color) << (8 + 5);
		*blue = PIXEL332BLUE(color) << (8 + 6);
		break;
	case 16:
		if (display->screens[0].root_visual->bits_per_rgb == 5) {
			*red = PIXEL555RED(color) << (8 + 3);
			*green = PIXEL555GREEN(color) << (8 + 3);
			*blue = PIXEL555BLUE(color) << (8 + 3);
		} else {
			*red = PIXEL565RED(color) << (8 + 3);
			*green = PIXEL565GREEN(color) << (8 + 2);
			*blue = PIXEL565BLUE(color) << (8 + 3);;
		}
		break;
	case 24:
	case 32:
		*red = PIXEL888RED(color) << (8 + 0);
		*green = PIXEL888GREEN(color) << (8 + 0);
		*blue = PIXEL888BLUE(color) << (8 + 0);
		break;
	}
//printf("pix2RGB: pix %x r %x g %x b %x\n", color, *red, *green, *blue);
}

int
XFreeColors(Display * display, Colormap colormap, unsigned long pixels[],
	    int npixels, unsigned long planes)
{
	int n;
	nxColormap *local = _nxFindColormap(colormap);

	if (!local)
		return 0;

	/* FIXME:  We should be smart and pack the array, blah, blah blah.... */
	/* I just stick this in so it will be easier later                    */

	for (n = 0; n < npixels; n++) {
		unsigned long index = pixels[n];

		if (index >= local->cur_color || local->colorval[index].ref==0)
			continue;
		--local->colorval[index].ref;
	}

	return 1;
}

/*
 * Convert a color from RGB to pixel format.  If hw is non-palette
 * this routine will never allocate a colormap entry.  (FIXME?)
 * This works for now since we special case XPutImage for truecolor
 * hardware.
 */
int
XAllocColor(Display * display, Colormap colormap, XColor * in_out)
{
	int 		i;
	Visual *	vp = display->screens[0].root_visual;
	nxColormap *	local;
	unsigned long 	rgb;
	unsigned int	red = in_out->red >> 8;
	unsigned int	green = in_out->green >> 8;
	unsigned int	blue = in_out->blue >> 8;

	if (vp->class == TrueColor) {
		switch (display->screens[0].root_depth) {
		case 8:
			in_out->pixel = RGB2PIXEL332(red, green, blue);
			break;
		case 16:
			if (vp->bits_per_rgb == 5)
				in_out->pixel = RGB2PIXEL555(red, green, blue);
			else
				in_out->pixel = RGB2PIXEL565(red, green, blue);
			break;
		case 24:
		case 32:
			/* create 0x00RRGGBB pixelval from RGB triplet*/
			in_out->pixel = RGB2PIXEL888(red, green, blue);
			break;
		}
//printf("XAllocColor in %x,%x,%x out %x\n", in_out->red, in_out->green, in_out->blue, in_out->pixel);
		return 1;

	}

	/* not truecolor hardware, allocate a colormap index*/
	rgb = GR_RGB(red, green, blue);

	local = _nxFindColormap(colormap);
	if (!local)
		return 0;

	for (i = 0; i < local->cur_color; i++) {
		if (local->colorval[i].value == rgb) {
			local->colorval[i].ref++;
			in_out->pixel = i;
			return 1;
		}
	}

	/* Otherwise allocate a new item */
	if (i >= local->color_alloc) {
		nxColorval *cv = Xrealloc(local->colorval,
			(local->color_alloc + 32) * sizeof(nxColorval));
		if (!cv)
			return 0;

		local->colorval = cv;
		local->color_alloc += 32;
	}

	in_out->pixel = local->cur_color;
	local->colorval[local->cur_color].value = rgb;
	local->colorval[local->cur_color++].ref = 1;
	return 1;
}
