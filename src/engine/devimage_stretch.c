/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 *
 * StretchImage - Resize an image
 *
 * !!! WARNING Not under the standard Microwindows license !!!
 * !!!         Read the following license (GPL)            !!!
 *
 * Major portions from SDL Simple DirectMedia Layer by Sam Lantinga
 * Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga <slouken@devolution.com>
 * This a stretch blit implementation based on ideas given to me by
 * Tomasz Cejner - thanks! :)
 */
/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "device.h"
#include "swap.h"

#if MW_FEATURE_IMAGES /* whole file */

#define DEFINE_COPY_ROW(name, type)					\
static void name(type *src, int src_w, type *dst, int dst_w)		\
{									\
	int i;								\
	int pos, inc;							\
	type pixel = 0;							\
									\
	pos = 0x10000;							\
	inc = (src_w << 16) / dst_w;					\
	for ( i=dst_w; i>0; --i ) {					\
		while ( pos >= 0x10000L ) {				\
			pixel = *src++;					\
			pos -= 0x10000L;				\
		}							\
		*dst++ = pixel;						\
		pos += inc;						\
	}								\
}

DEFINE_COPY_ROW(copy_row1, unsigned char)
DEFINE_COPY_ROW(copy_row2, unsigned short)
DEFINE_COPY_ROW(copy_row4, unsigned long)

static void copy_row3(unsigned char *src, int src_w, unsigned char *dst,
	int dst_w)
{
	int i;
	int pos, inc;
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	pos = 0x10000;
	inc = (src_w << 16) / dst_w;
	for ( i=dst_w; i>0; --i ) {
		while ( pos >= 0x10000L ) {
			b = *src++;
			g = *src++;
			r = *src++;
			pos -= 0x10000L;
		}
		*dst++ = b;
		*dst++ = g;
		*dst++ = r;
		pos += inc;
	}
}

/**
 * Perform a stretch blit between two image structs of the same format.
 *
 * @param src Source image.
 * @param srcrect Source rectangle.
 * @param dst Destination image.
 * @param dstrect Destination rectangle.
 */
void
GdStretchImage(PMWIMAGEHDR src, MWCLIPRECT *srcrect, PMWIMAGEHDR dst,
	MWCLIPRECT *dstrect)
{
	int pos, inc;
	int bytesperpixel;
	int dst_maxrow;
	int src_row, dst_row;
	MWUCHAR *srcp = 0;
	MWUCHAR *dstp;
	MWCLIPRECT full_src;
	MWCLIPRECT full_dst;

	if ( src->bytesperpixel != dst->bytesperpixel ) {
		EPRINTF("GdStretchImage: bytesperpixel mismatch\n");
		return;
	}

	/* Verify the blit rectangles */
	if ( srcrect ) {
		if ( (srcrect->x < 0) || (srcrect->y < 0) ||
		     ((srcrect->x+srcrect->width) > src->width) ||
		     ((srcrect->y+srcrect->height) > src->height) ) {
			EPRINTF("GdStretchImage: invalid source rect\n");
			return;
		}
	} else {
		full_src.x = 0;
		full_src.y = 0;
		full_src.width = src->width;
		full_src.height = src->height;
		srcrect = &full_src;
	}
	if ( dstrect ) {
		/* if stretching to nothing, return*/
		if (!dstrect->width || !dstrect->height)
			return;
		if ( (dstrect->x < 0) || (dstrect->y < 0) ||
		     ((dstrect->x+dstrect->width) > dst->width) ||
		     ((dstrect->y+dstrect->height) > dst->height) ) {
			EPRINTF("GdStretchImage: invalid dest rect\n");
			return;
		}
	} else {
		full_dst.x = 0;
		full_dst.y = 0;
		full_dst.width = dst->width;
		full_dst.height = dst->height;
		dstrect = &full_dst;
	}

	/* Set up the data... */
	pos = 0x10000;
	inc = (srcrect->height << 16) / dstrect->height;
	src_row = srcrect->y;
	dst_row = dstrect->y;
	bytesperpixel = dst->bytesperpixel;

	/* Perform the stretch blit */
	for ( dst_maxrow = dst_row+dstrect->height; dst_row<dst_maxrow;
								++dst_row ) {
		dstp = (MWUCHAR *)dst->imagebits + (dst_row*dst->pitch)
				    + (dstrect->x*bytesperpixel);
		while ( pos >= 0x10000L ) {
			srcp = (MWUCHAR *)src->imagebits + (src_row*src->pitch)
				    + (srcrect->x*bytesperpixel);
			++src_row;
			pos -= 0x10000L;
		}

		switch (bytesperpixel) {
		case 1:
			copy_row1(srcp, srcrect->width, dstp, dstrect->width);
			break;
		case 2:
			copy_row2((unsigned short *)srcp, srcrect->width,
				(unsigned short *)dstp, dstrect->width);
			break;
		case 3:
			copy_row3(srcp, srcrect->width, dstp, dstrect->width);
			break;
		case 4:
			copy_row4((unsigned long *)srcp, srcrect->width,
				(unsigned long *)dstp, dstrect->width);
			break;
		}

		pos += inc;
	}
}
#endif /* MW_FEATURE_IMAGES*/
