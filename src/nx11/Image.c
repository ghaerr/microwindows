/*
 * X Image routines.  Messy.  Not done yet.
 *
 * Portions Copyright 2003, Jordan Crouse (jordan@cosmicpenguin.net)
 */
#define MWINCLUDECOLORS
#include "nxlib.h"
#include <stdlib.h>
#include <string.h>
#include "uni_std.h"

#define READ_BIT(image, x, y) \
	((image->bitmap_bit_order == LSBFirst)? \
		(image->data[(y * image->bytes_per_line) + (x >> 3) ] & (1 << (x & 7))) : \
		(image->data[(y * image->bytes_per_line) + (x >> 3) ] & (1 << ((7-x) & 7))))

static int
destroy_image(XImage *image)
{
	if (image->data)
		Xfree(image->data);
	Xfree(image);
	return 1;
}

static unsigned long
get_pixel1(XImage *image, int x, int y)
{
	return READ_BIT(image, x, y) != 0;
}

static int
put_pixel1(XImage *image, int x, int y, unsigned long pixel)
{
	static unsigned char mask[] =
		{ 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

	if (pixel)
		image->data[(y * image->bytes_per_line) + (x >> 3) ] |= mask[x&7];
	else image->data[(y * image->bytes_per_line) + (x >> 3) ] &= ~mask[x&7];
	return 1;
}

static unsigned long
get_pixel8(XImage *image, int x, int y)
{
	unsigned char *src = (unsigned char *)image->data + (y * image->bytes_per_line) + x;

	return *src;
}

static int
put_pixel8(XImage *image, int x, int y, unsigned long pixel)
{
	unsigned char *src = (unsigned char *)image->data + (y * image->bytes_per_line) + x;

	*src = pixel;
	return 1;
}

static unsigned long
get_pixel16(XImage *image, int x, int y)
{
	unsigned short *src = (unsigned short *)(image->data + (y * image->bytes_per_line) + (x << 1));

	return *src;
}

static int
put_pixel16(XImage *image, int x, int y, unsigned long pixel)
{
	unsigned short *src = (unsigned short *)(image->data + (y * image->bytes_per_line) + (x << 1));

	*src = pixel;
	return 1;
}

static unsigned long
get_pixel32(XImage *image, int x, int y)
{
	uint32_t *src = (uint32_t *)(image->data + (y * image->bytes_per_line) + (x << 2));

	return *src;
}

static int
put_pixel32(XImage *image, int x, int y, unsigned long pixel)
{
	uint32_t *src = (uint32_t *)(image->data + (y * image->bytes_per_line) + (x << 2));

	*src = pixel;
	return 1;
}

/*
 * compute image line size and bytes per pixel
 * from bits per pixel and width
 */
#define PIX2BYTES(n)    (((n)+7)/8)
static void
computePitch(int bpp, int width, int *pitch, int *bytesperpixel)
{
	int linesize;
	int bytespp = 1;

	if (bpp == 1)
		linesize = PIX2BYTES(width);
	else if (bpp <= 4)
		linesize = PIX2BYTES(width << 2);
	else if (bpp <= 8)
		linesize = width;
	else if (bpp <= 16) {
		linesize = width * 2;
		bytespp = 2;
	} else if (bpp <= 24) {
		linesize = width * 3;
		bytespp = 3;
	} else {
		linesize = width * 4;
		bytespp = 4;
	}

	/* rows are DWORD right aligned */
	*pitch = (linesize + 3) & ~3;

	*bytesperpixel = bytespp;
}

static void setImageFunc(XImage *image)
{
	image->f.create_image = XCreateImage;
	image->f.destroy_image = destroy_image;
	image->f.sub_image = 0;		//FIXME
	image->f.add_pixel = 0;
	switch (image->bits_per_pixel) {
	case 1:
		image->f.get_pixel = get_pixel1;
		image->f.put_pixel = put_pixel1;
		break;
	case 8:
		image->f.get_pixel = get_pixel8;
		image->f.put_pixel = put_pixel8;
		break;
	case 16:
		image->f.get_pixel = get_pixel16;
		image->f.put_pixel = put_pixel16;
		break;
	case 32:
		image->f.get_pixel = get_pixel32;
		image->f.put_pixel = put_pixel32;
		break;
	default:
		DPRINTF("createImageStruct: unsupported bpp %d\n", image->bits_per_pixel);
	}
}

static XImage *
createImageStruct(unsigned int width, unsigned int height, unsigned int depth,
	int format, int bytes_per_line, int bitmap_pad, unsigned long red_mask,
	unsigned long green_mask, unsigned long blue_mask)
{
	XImage *image = (XImage *) Xcalloc(1, sizeof(XImage));
	if (!image) return 0;

	image->width = width;
	image->height = height;
	image->format = format;
	image->depth = depth;

	/* note: if these are changed by application, we'll likely fail*/
	image->byte_order = LSBFirst;
	image->bitmap_bit_order = LSBFirst;
	image->bitmap_unit = bitmap_pad;
	image->bitmap_pad = bitmap_pad;

	if (bytes_per_line)
		image->bytes_per_line = bytes_per_line;
	else {
		int bytespp, pitch;

		computePitch(depth, image->width, &pitch, &bytespp);
		image->bytes_per_line = pitch;
	}

	image->bits_per_pixel = depth;

	DPRINTF("createImage: %d,%d format %d depth %d bytespl %d\n", width, height, format, depth, image->bytes_per_line);

	// FIXME check
	image->red_mask = red_mask;
	image->green_mask = green_mask;
	image->blue_mask = blue_mask;

#if 0
	image->f.create_image = XCreateImage;
	image->f.destroy_image = destroy_image;
	image->f.sub_image = 0;		//FIXME
	image->f.add_pixel = 0;
	switch (image->bits_per_pixel) {
	case 1:
		image->f.get_pixel = get_pixel1;
		image->f.put_pixel = put_pixel1;
		break;
	case 8:
		image->f.get_pixel = get_pixel8;
		image->f.put_pixel = put_pixel8;
		break;
	case 16:
		image->f.get_pixel = get_pixel16;
		image->f.put_pixel = put_pixel16;
		break;
	case 32:
		image->f.get_pixel = get_pixel32;
		image->f.put_pixel = put_pixel32;
		break;
	default:
		DPRINTF("createImageStruct: unsupported bpp\n");
	}
#endif
	setImageFunc(image);

	return image;
}

/*
 * Create an image in either, preferably in hw format.
 */
XImage *
XCreateImage(Display * display, Visual * visual, unsigned int depth,
	int format, int offset, char *data, unsigned int width,
	unsigned int height, int bitmap_pad, int bytes_per_line)
{
	XImage *image;
	unsigned long red_mask = 0, green_mask = 0, blue_mask = 0;
	
	if (depth == 24) {
		DPRINTF("XCreateImage: changing depth to GR_PIXELVAL\n");
		depth = sizeof(GR_PIXELVAL) * 8;
	}
	if (depth != display->screens[0].root_depth) {
		DPRINTF("XCreateImage: depth[%d] != hw_format[%d]\n", depth, display->screens[0].root_depth);
		//if (depth == 1)	//FIXME
			//depth = sizeof(GR_PIXELVAL) * 8;
	}

	if (visual) {
		red_mask = visual->red_mask;
		green_mask = visual->green_mask;
		blue_mask = visual->blue_mask;
	}
	image = createImageStruct(width, height, depth, format,
			   bytes_per_line, bitmap_pad, red_mask,
			   blue_mask, green_mask);
	if (image)
		image->data = data;

	return image;
}

/*unsigned int Ones(uint32_t mask)
{
	register uint32_t y;
	y = (mask >> 1) &033333333333;
	y = mask - y - ((y >>1) & 033333333333);
	return ((unsigned int) (((y + (y >> 3)) & 030707070707) % 077));
}*/

/*
 * Create an image, (always in GR_PIXELVAL format), and initialize
 * using GrReadArea.
 */
XImage *
XGetImage(Display * display, Drawable d, int x, int y,
	unsigned int width, unsigned int height,
	unsigned long plane_mask, int format)
{
	int depth, drawsize, r, src_rowsize;
	char *dst, *src, *buffer;
	Visual *visual;
	XImage *image;
	GR_WINDOW_INFO winfo;

	/* Ensure that the block is entirely within the drawable */
	GrGetWindowInfo(d, &winfo);
	if (x < 0 || (x + width) > winfo.width || y < 0 || (y + height) > winfo.height) {
		/* Error - BadMatch */
		DPRINTF("XGetImage: Image out of bounds\n");
		DPRINTF("    %d %d - %d %d is out of bounds on %d, %d - %d %d\n",
		       x, y, width, height, 0, 0, winfo.width, winfo.height);
		return NULL;
	}

	if (format == XYPixmap) {
		/*depth = Ones(plane_mask &
			(((uint32_t)0xFFFFFFFF) >> (32 - sizeof(GR_PIXELVAL)*8)));*/
		//depth = sizeof(GR_PIXELVAL) * 8;
		depth = 1;	// for Qt (Mask)
		DPRINTF("XGetImage warning: broken for XYPixmap (bpp %d)\n", depth);

		visual = XDefaultVisual(display, 0);
		image = createImageStruct(width, height, 1, format, 0, 0,
			visual->red_mask, visual->green_mask, visual->blue_mask);
		if (!image) return NULL;
		image->data = (char *)malloc(width/*image->bytes_per_line*/ * height);
		memset(image->data, 0xff, width/*image->bytes_per_line*/ * height);
		return image;
	} else {
		/*
		* create XImage in GrReadArea compatible format,
		* which is always sizeof(GR_PIXELVAL), not hw display format
		*/
		depth = sizeof(GR_PIXELVAL) * 8;
	}
#if 0
	if (depth <= 8)
		drawsize = 1;
	if (depth > 8 && depth <= 16)
		drawsize = 2;
	else
		drawsize = 4;
#endif
	drawsize = sizeof(GR_PIXELVAL);

	visual = XDefaultVisual(display, 0);
	image = createImageStruct(width, height, depth, format, 0, 0/*display->bitmap_pad*/,
		visual->red_mask, visual->green_mask, visual->blue_mask);
	if (!image)
		return NULL;

	src_rowsize = width * drawsize;		/* bytes per line of image*/
	image->data = (char *) Xcalloc(1, src_rowsize * height);
	GrReadArea(d, x, y, width, height, (void *) image->data);

	/* createImage may have padded image width, may have to copy/re-pad*/
	if(image->bytes_per_line != src_rowsize) {
		int pad = image->bytes_per_line - src_rowsize, i;
		dst = buffer = (char *)Xmalloc(image->bytes_per_line * height);
		src = image->data;

		/* Copy each row to the buffer */
		for(r = 0; r < height; r++) {
			memcpy(dst, src, src_rowsize);
			dst += src_rowsize;

			/* pad with zeros*/
			for(i=pad; --i>=0; )
				*dst++ = 0;

			/* Move to the end of the line on src image*/
			src += src_rowsize;
		}
		Xfree(image->data);
		image->data = buffer;
	}

	if (format == XYPixmap && plane_mask != 0xFFFFFFFF)
		DPRINTF("XGetImage: plane_mask ignored\n");

	return image;
}

#if 0
int _XSetImage(XImage *srcimg, XImage *dstimg, int x, int y)
{
	unsigned long pixel;
	int row, col;
	int width, height, startrow, startcol;

	if (x < 0) {
		startcol = -x;
		x = 0;
	} else
		startcol = 0;
	if (y < 0) {
		startrow = -y;
		y = 0;
	} else
		startrow = 0;
	width = dstimg->width - x;
	if (srcimg->width < width) width = srcimg->width;
	height = dstimg->height - y;
	if (srcimg->height < height) height = srcimg->height;

	/* this is slow, will do better later */
	for (row = startrow; row < height; row++) {
		for (col = startcol; col < width; col++) {
			//pixel = XGetPixel(srcimg, col, row);
			//XPutPixel(dstimg, x + col, y + row, pixel);
			dstimg->f.put_pixel(dstimg, x + col, y + row, srcimg->f.get_pixel(srcimg, col, row));
		}
	}
	return 1;
}
#endif

XImage *XGetSubImage(Display *dpy, Drawable d, int x, int y,
	unsigned int width, unsigned int height, unsigned long plane_mask,
	int format, XImage *dest_image, int dest_x, int dest_y)
{
	XImage *img;
	int i, n;
	char *p, *s;

	DPRINTF("XGetSubImage src %d,%d wxh %d,%d dst %d,%d\n",
		x, y, width, height, dest_x, dest_y);
	img = XGetImage(dpy, d, x, y, width, height, plane_mask, format);
	if (!img) return NULL;

	// Both routines are fine
	//_XSetImage(img, dest_image, dest_x, dest_y);
	n = width * sizeof(GR_PIXELVAL);
	s = img->data;
	p = dest_image->data + (dest_y * dest_image->bytes_per_line) + (dest_x * dest_image->depth/8);
	for (i=0; i<height; i++) {
		memcpy(p, s, n);
		p += dest_image->bytes_per_line;
		s += img->bytes_per_line;
	}
	//XDestroyImage(img);
	destroy_image(img);

	return dest_image;
}

/* This takes a portion of the image buffer and rearranges it to keep from 
   freaking out GrArea. This will take into account a shifted src_x 
   (or a width that is not as large as the declared image width).
*/
static void
showPartialImage(Display *display, GR_WINDOW_ID d, GR_GC_ID gc, GR_RECT *srect,
	GR_RECT *drect, char *src, int pixtype, int pad)
{
	char *dst, *buffer;
	char *ptr = src;  /* This will already be adjusted to the inital X and Y of the image */
	int r, size = 0;

	switch(pixtype) {
	case MWPF_TRUECOLOR332: 
	case MWPF_TRUECOLOR233: 
		size = 1;
		break;
	case MWPF_TRUECOLOR555:
	case MWPF_TRUECOLOR565:
		size = 2;
		break;
	case MWPF_TRUECOLORRGB:
		size = 3;
		break;
	case MWPF_TRUECOLORARGB:
	case MWPF_TRUECOLORABGR:
		size = 4;
		break;
	case MWPF_HWPIXELVAL:
		size = display->screens[0].root_depth / 8;
		break;
	}

	/* Allocate a local buffer - this is much faster than doing N GrArea calls */
	dst = buffer = (char *)ALLOCA( (drect->width * size) * drect->height);
	if (!dst)
		return;

	/* Copy each row to the buffer */
	for(r = 0; r < drect->height; r++) {
		memcpy(dst, ptr, drect->width * size); 
		dst += (drect->width * size);

		/* Move to the end of the line on the real buffer */
		ptr += (srect->width - srect->x) * size + pad;

		/* And then offset ourselves accordingly          */
		ptr += (srect->x * size);
	}
	       
	GrArea(d, gc, drect->x, drect->y, drect->width, drect->height, buffer, pixtype);
	FREEA(buffer);
}

/*
 * Output a truecolor (non-palettized) image using GrArea.
 * Image bits may need Nano-X conversion if not in hw pixel format.
 */
static int
putTrueColorImage(Display * display, Drawable d, GC gc, XImage *image,
	int src_x, int src_y, int dest_x, int dest_y,
	unsigned int width, unsigned int height)
{
	int		pixtype = MWPF_HWPIXELVAL;	/* assume hw pixel format*/
	int		drawsize, pad;
	char 	*src;

	/*DPRINTF("putTruecolorImage: bpp %d %d,%d -> %d,%d %d,%d\n", image->depth,
		src_x, src_y, dest_x, dest_y, width, height);*/

	/* convert pixtype if image bpp not hw format*/
	switch (image->bits_per_pixel) {
	case 1:
		DPRINTF("putTruecolorImage bpp 1 FIXME\n");
		src = image->data;
		return 0; // must return, will crash server major FIXME
	case 8:
		if (display->screens[0].root_depth != 8)
			pixtype = MWPF_TRUECOLOR332;
		src = image->data + (src_y * image->bytes_per_line) + src_x;
		break;
	case 16:
		if (display->screens[0].root_depth != 16) {
			/* we don't check 565 vs 555 here, just assume hw format*/
			if (display->screens[0].root_visual->bits_per_rgb == 5)
				pixtype = MWPF_TRUECOLOR555;
			else
				pixtype = MWPF_TRUECOLOR565;
		}
		src = image->data + (src_y * image->bytes_per_line) + (src_x << 1);
		break;
	case 24:
		if (display->screens[0].root_depth != 24)
			pixtype = MWPF_TRUECOLORRGB;
		src = image->data + (src_y * image->bytes_per_line) + (src_x * 3);
		break;
	case 32:
		if (display->screens[0].root_depth != 32)
			pixtype = MWPF_TRUECOLORARGB;		// FIXME could be MWPF_TRUECOLORABGR
		src = image->data + (src_y * image->bytes_per_line) + (src_x << 2);
		break;
	default:
		DPRINTF("XPutImage: unsupported bpp %d\n", image->bits_per_pixel);
		return 0;
	}
	drawsize = image->bits_per_pixel / 8;

DPRINTF("putTrueColorImage depth %d pixtype %d src %d,%d wxh %d,%d dst %d,%d\n",
	       image->depth, pixtype, src_x, src_y, width, height, dest_x, dest_y);

	/* X11 draws backgrounds on pixmaps but not text*/
	GrSetGCUseBackground(gc->gid, GR_TRUE);

	/*
	 * We can only do a direct GrArea if the width is the same as the width
	 * of our image buffer.  Otherwise, we need to move to a slower path.
	 */
	pad = image->bytes_per_line - (image->width * drawsize);
	if (!pad && (src_x == 0) && (width == image->width))
		GrArea((GR_WINDOW_ID)d, (GR_GC_ID)gc->gid, dest_x, dest_y, width, height, src, pixtype);
	else {
		GR_RECT srect, drect;

		srect.x = src_x;
		srect.y = src_y;
		srect.width = image->width;
		srect.height = image->height;

		drect.x = dest_x;
		drect.y = dest_y;
		drect.width = width;
		drect.height = height;

		showPartialImage(display, (GR_WINDOW_ID)d, (GR_GC_ID)gc->gid,
			&srect, &drect, src, pixtype, pad);
	}

	/* turn background drawing back off... */
	GrSetGCUseBackground(gc->gid, GR_FALSE);

	return 1;
}

/*
 * Output a palette-oriented image.  Must have properly defined colormap.
 * These images are defined in any bpp but contain colormap indices.
 */
static int
putImage(Display * display, Drawable d, GC gc, XImage * image,
	int src_x, int src_y, int dest_x, int dest_y,
	unsigned int width, unsigned int height)
{
	unsigned int x, y;
	MWPIXELVAL *buffer, *dst;
	nxColormap *colormap;
	unsigned char *src = (unsigned char *)image->data
		+ ((src_y * (image->bytes_per_line)) + src_x);

	/*DPRINTF("putImage: bpp %d %d,%d -> %d,%d %d,%d\n", image->depth,
		src_x, src_y, dest_x, dest_y, width, height);*/

	if (image->bits_per_pixel >= 8) {
		colormap = _nxFindColormap(XDefaultColormap(display, 0));
		if (!colormap)
			return 0;
		DPRINTF("curcolor %x\n", colormap->cur_color);
	}

	buffer = ALLOCA(width * height * sizeof(MWPIXELVAL));

	for (y = src_y; y < src_y + height; y++) {
		dst = buffer + y * width;
		for (x = src_x; x < src_x + width; x++, dst++) {
			unsigned short cl;

			/* get colormap index bits from image*/
			switch (image->bits_per_pixel) {
			case 32:
				cl = (unsigned short) *((MWPIXELVAL *) src);
				src += 4;
				break;
			case 24:
				cl = src[0];        /* B */
				cl |= src[1] << 8;  /* G */
				cl |= src[2] << 16; /* R */
				src += 3;
				break;
			case 16:
				cl = (unsigned short) *((unsigned short *) src);
				src += 2;
				break;

			case 8:
				cl = (unsigned short) *((unsigned char *) src);
				src += 1;
				break;

			case 1:
				if (READ_BIT(image, x, y)) {
					//cl = colormap->colorval[1].value;
					//cl = ((XGCValues *)gc->ext_data)->foreground;
					*dst = WHITE;
				} else {
					//cl = colormap->colorval[0].value;
					*dst = BLACK;
				}
				continue;

			default:
				cl = 0;
				break;
			}

			if (cl < colormap->cur_color)
				*dst = (unsigned long) colormap->colorval[cl].value;
			else {
				// FIXME colors kluged as if truecolor here, no colormap entries
				DPRINTF("XPutImage: unknown color index %x\n", cl);
				*dst = 0;
			}
		}
	}

	GrArea((GR_WINDOW_ID) d, (GR_GC_ID) gc->gid, dest_x, dest_y, width, height, buffer, MWPF_RGB);

	FREEA(buffer);
	return 1;
}

int XPutImage(Display *display, Drawable d, GC gc, XImage *image,
	int src_x, int src_y, int dest_x, int dest_y, unsigned int width,
	unsigned int height)
{
#if 0
	// Why is scrolling going wrong
	if (src_x<0) {
		width += src_x;
		src_x = 0;
	}
	if (src_y<0) {
		height += src_y;
		src_y = 0;
	}
	if (dest_x<0) {
		DPRINTF("XPutImage warning src(%d,%d) wxh(%d,%d) dst(%d,%d)\n", src_x, src_y, width, height, dest_x, dest_y);
		width += dest_x;
		if (width <= 0) return 0;
		dest_x = 0;
	}
	if (dest_y<0) {
		DPRINTF("XPutImage warning src(%d,%d) wxh(%d,%d) dst(%d,%d)\n", src_x, src_y, width, height, dest_x, dest_y);
		height += dest_y;
		if (height <= 0) return 0;
		dest_y = 0;
	}
#endif

	if (display->screens[0].root_visual->class == TrueColor && image->depth != 1)
		return putTrueColorImage(display, d, gc, image, src_x, src_y,
			dest_x, dest_y, width, height);
	return putImage(display, d, gc, image, src_x, src_y, dest_x, dest_y,
			width, height);
}

#if 0
/*
 * This routine initializes the image object function pointers.  The
 * intent is to provide native (i.e. fast) routines for native format images
 * only using the generic (i.e. slow) routines when fast ones don't exist.
 * However, with the current rather botched external interface, clients may
 * have to mung image attributes after the image gets created, so the fast
 * routines always have to check to make sure the optimization is still
 * valid, and reinit the functions if not.
 */
void _XInitImageFuncPtrs(XImage *image)
{
	image->f.create_image = XCreateImage;
	image->f.destroy_image = _XDestroyImage;
	if ((image->format == ZPixmap) && (image->bits_per_pixel == 8)) {
		image->f.get_pixel = _XGetPixel8;
		image->f.put_pixel = _XPutPixel8;
	} else if (((image->bits_per_pixel | image->depth) == 1) &&
		(image->byte_order == image->bitmap_bit_order)) {
		image->f.get_pixel = _XGetPixel1;
		image->f.put_pixel = _XPutPixel1;
	} else if ((image->format == ZPixmap) &&
		(image->bits_per_pixel == 32)) {
		image->f.get_pixel = _XGetPixel32;
		image->f.put_pixel = _XPutPixel32;
	} else if ((image->format == ZPixmap) &&
		(image->bits_per_pixel == 16)) {
		image->f.get_pixel = _XGetPixel16;
		image->f.put_pixel = _XPutPixel16;
	} else {
		image->f.get_pixel = _XGetPixel;
		image->f.put_pixel = _XPutPixel;
	}
	image->f.sub_image = _XSubImage;
/*	image->f.set_image = _XSetImage;*/
	image->f.add_pixel = _XAddPixel;
}
#endif

// required for gtk
#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad)-1)) / (pad)) * ((pad)>>3))
Status XInitImage(XImage *image)
{
	int min_bytes_per_line;
	DPRINTF("XInitImage depth %d bpp %d\n", image->depth, image->bits_per_pixel);

	if (!image->bits_per_pixel)
		image->bits_per_pixel = image->depth;

	if (image->depth == 0 || image->depth > 32 ||
		image->bits_per_pixel > 32 || image->bitmap_unit > 32 ||
		image->bits_per_pixel < 0 || image->bitmap_unit < 0 ||
		(image->format != XYBitmap && image->format != XYPixmap &&
		image->format != ZPixmap) ||
		(image->format == XYBitmap && image->depth != 1) ||
		(image->bitmap_pad != 8 && image->bitmap_pad != 16 &&
		image->bitmap_pad != 32) || image->xoffset < 0)
		return 0;

	// compute per line accelerator.
	if (image->format == ZPixmap) {
		min_bytes_per_line =
			ROUNDUP((image->bits_per_pixel * image->width), image->bitmap_pad);
	} else {
		min_bytes_per_line =
			ROUNDUP((image->width + image->xoffset), image->bitmap_pad);
	}

	DPRINTF("Image %d,%d min_bytes_line %d bytes_line %d\n",
		image->width, image->height, min_bytes_per_line, image->bytes_per_line);

	if (image->bytes_per_line == 0)
		image->bytes_per_line = min_bytes_per_line;

	if (image->bytes_per_line < min_bytes_per_line)
		return 0;

	//_XInitImageFuncPtrs(image);
	setImageFunc(image);

	return 1;
}
