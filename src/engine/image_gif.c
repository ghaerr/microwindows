/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 *
 * Image decode routine for GIF files
 *
 * !!! WARNING Not under the standard Microwindows license !!!
 * !!!         Read the following license (GPL)            !!!
 */
/* Code for GIF decoding has been adapted from XPaint:                   */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993 David Koblas.			       | */
/* | Copyright 1996 Torsten Martinsen.				       | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.	       | */
/* +-------------------------------------------------------------------+ */
/* Portions Copyright (C) 1999  Sam Lantinga*/
/* Adapted for use in SDL by Sam Lantinga -- 7/20/98 */
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

#if MW_FEATURE_IMAGES && defined(HAVE_GIF_SUPPORT)

#define	MAXCOLORMAPSIZE		256
#define	MAX_LWZ_BITS		12
#define INTERLACE		0x40
#define LOCALCOLORMAP		0x80

#define CM_RED		0
#define CM_GREEN	1
#define CM_BLUE		2

#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))
#define	ReadOK(src,buffer,len)	GdImageBufferRead(src, buffer, len)
#define LM_to_uint(a,b)		(((b)<<8)|(a))

struct {
    unsigned int Width;
    unsigned int Height;
    unsigned char ColorMap[3][MAXCOLORMAPSIZE];
    unsigned int BitPixel;
    unsigned int ColorResolution;
    unsigned int Background;
    unsigned int AspectRatio;
    int GrayScale;
} GifScreen;

static struct {
    int transparent;
    int delayTime;
    int inputFlag;
    int disposal;
} Gif89;

static int ReadColorMap(buffer_t *src, int number,
		unsigned char buffer[3][MAXCOLORMAPSIZE], int *flag);
static int DoExtension(buffer_t *src, int label);
static int GetDataBlock(buffer_t *src, unsigned char *buf);
static int GetCode(buffer_t *src, int code_size, int flag);
static int LWZReadByte(buffer_t *src, int flag, int input_code_size);
static int ReadImage(buffer_t *src, PMWIMAGEHDR pimage, int len, int height, int,
		unsigned char cmap[3][MAXCOLORMAPSIZE],
		int gray, int interlace, int ignore);

int
GdDecodeGIF(buffer_t *src, PMWIMAGEHDR pimage)
{
    unsigned char buf[16];
    unsigned char c;
    unsigned char localColorMap[3][MAXCOLORMAPSIZE];
    int grayScale;
    int useGlobalColormap;
    int bitPixel;
    int imageCount = 0;
    char version[4];
    int imageNumber = 1;
    int ok = 0;

    GdImageBufferSeekTo(src, 0UL);

    pimage->imagebits = NULL;
    pimage->palette = NULL;

    if (!ReadOK(src, buf, 6))
        return 0;		/* not gif image*/
    if (strncmp((char *) buf, "GIF", 3) != 0)
        return 0;
    strncpy(version, (char *) buf + 3, 3);
    version[3] = '\0';

    if (strcmp(version, "87a") != 0 && strcmp(version, "89a") != 0) {
	EPRINTF("GdDecodeGIF: GIF version number not 87a or 89a\n");
        return 2;		/* image loading error*/
    }
    Gif89.transparent = -1;
    Gif89.delayTime = -1;
    Gif89.inputFlag = -1;
    Gif89.disposal = 0;

    if (!ReadOK(src, buf, 7)) {
	EPRINTF("GdDecodeGIF: bad screen descriptor\n");
        return 2;		/* image loading error*/
    }
    GifScreen.Width = LM_to_uint(buf[0], buf[1]);
    GifScreen.Height = LM_to_uint(buf[2], buf[3]);
    GifScreen.BitPixel = 2 << (buf[4] & 0x07);
    GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
    GifScreen.Background = buf[5];
    GifScreen.AspectRatio = buf[6];

    if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
	if (ReadColorMap(src, GifScreen.BitPixel, GifScreen.ColorMap,
			 &GifScreen.GrayScale)) {
	    EPRINTF("GdDecodeGIF: bad global colormap\n");
            return 2;		/* image loading error*/
	}
    }

    do {
	if (!ReadOK(src, &c, 1)) {
	    EPRINTF("GdDecodeGIF: EOF on image data\n");
            goto done;
	}
	if (c == ';') {		/* GIF terminator */
	    if (imageCount < imageNumber) {
		EPRINTF("GdDecodeGIF: no image %d of %d\n", imageNumber,imageCount);
                goto done;
	    }
	}
	if (c == '!') {		/* Extension */
	    if (!ReadOK(src, &c, 1)) {
		EPRINTF("GdDecodeGIF: EOF on extension function code\n");
                goto done;
	    }
	    DoExtension(src, c);
	    continue;
	}
	if (c != ',') {		/* Not a valid start character */
	    continue;
	}
	++imageCount;

	if (!ReadOK(src, buf, 9)) {
	    EPRINTF("GdDecodeGIF: bad image size\n");
            goto done;
	}
	useGlobalColormap = !BitSet(buf[8], LOCALCOLORMAP);

	bitPixel = 1 << ((buf[8] & 0x07) + 1);

	if (!useGlobalColormap) {
	    if (ReadColorMap(src, bitPixel, localColorMap, &grayScale)) {
		EPRINTF("GdDecodeGIF: bad local colormap\n");
                goto done;
	    }
	    ok = ReadImage(src, pimage, LM_to_uint(buf[4], buf[5]),
			      LM_to_uint(buf[6], buf[7]),
			      bitPixel, localColorMap, grayScale,
			      BitSet(buf[8], INTERLACE),
			      imageCount != imageNumber);
	} else {
	    ok = ReadImage(src, pimage, LM_to_uint(buf[4], buf[5]),
			      LM_to_uint(buf[6], buf[7]),
			      GifScreen.BitPixel, GifScreen.ColorMap,
			      GifScreen.GrayScale, BitSet(buf[8], INTERLACE),
			      imageCount != imageNumber);
	}
    } while (ok == 0);

    /* set transparent color, if any*/
    pimage->transcolor = Gif89.transparent;

    if (ok)
	    return 1;		/* image load ok*/

done:
    if (pimage->imagebits)
	    free(pimage->imagebits);
    if (pimage->palette)
	    free(pimage->palette);
    return 2;			/* image load error*/
}

static int
ReadColorMap(buffer_t *src, int number, unsigned char buffer[3][MAXCOLORMAPSIZE],
    int *gray)
{
    int i;
    unsigned char rgb[3];
    int flag;

    flag = TRUE;

    for (i = 0; i < number; ++i) {
	if (!ReadOK(src, rgb, sizeof(rgb)))
	    return 1;
	buffer[CM_RED][i] = rgb[0];
	buffer[CM_GREEN][i] = rgb[1];
	buffer[CM_BLUE][i] = rgb[2];
	flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

#if 0
    if (flag)
	*gray = (number == 2) ? PBM_TYPE : PGM_TYPE;
    else
	*gray = PPM_TYPE;
#else
    *gray = 0;
#endif

    return FALSE;
}

static int
DoExtension(buffer_t *src, int label)
{
    static unsigned char buf[256];

    switch (label) {
    case 0x01:			/* Plain Text Extension */
	break;
    case 0xff:			/* Application Extension */
	break;
    case 0xfe:			/* Comment Extension */
	while (GetDataBlock(src, (unsigned char *) buf) != 0);
	return FALSE;
    case 0xf9:			/* Graphic Control Extension */
	GetDataBlock(src, (unsigned char *) buf);
	Gif89.disposal = (buf[0] >> 2) & 0x7;
	Gif89.inputFlag = (buf[0] >> 1) & 0x1;
	Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
	if ((buf[0] & 0x1) != 0)
	    Gif89.transparent = buf[3];

	while (GetDataBlock(src, (unsigned char *) buf) != 0);
	return FALSE;
    default:
	break;
    }

    while (GetDataBlock(src, (unsigned char *) buf) != 0);

    return FALSE;
}

static int ZeroDataBlock = FALSE;

static int
GetDataBlock(buffer_t *src, unsigned char *buf)
{
    unsigned char count;

    if (!ReadOK(src, &count, 1))
	return -1;
    ZeroDataBlock = count == 0;

    if ((count != 0) && (!ReadOK(src, buf, count)))
	return -1;
    return count;
}

static int
GetCode(buffer_t *src, int code_size, int flag)
{
    static unsigned char buf[280];
    static int curbit, lastbit, done, last_byte;
    int i, j, ret;
    unsigned char count;

    if (flag) {
	curbit = 0;
	lastbit = 0;
	done = FALSE;
	return 0;
    }
    if ((curbit + code_size) >= lastbit) {
	if (done) {
	    if (curbit >= lastbit)
		EPRINTF("GdDecodeGIF: bad decode\n");
	    return -1;
	}
	buf[0] = buf[last_byte - 2];
	buf[1] = buf[last_byte - 1];

	if ((count = GetDataBlock(src, &buf[2])) == 0)
	    done = TRUE;

	last_byte = 2 + count;
	curbit = (curbit - lastbit) + 16;
	lastbit = (2 + count) * 8;
    }
    ret = 0;
    for (i = curbit, j = 0; j < code_size; ++i, ++j)
	ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

    curbit += code_size;

    return ret;
}

static int
LWZReadByte(buffer_t *src, int flag, int input_code_size)
{
    int code, incode;
    register int i;
    static int fresh = FALSE;
    static int code_size, set_code_size;
    static int max_code, max_code_size;
    static int firstcode, oldcode;
    static int clear_code, end_code;
    static int table[2][(1 << MAX_LWZ_BITS)];
    static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;

    if (flag) {
	set_code_size = input_code_size;
	code_size = set_code_size + 1;
	clear_code = 1 << set_code_size;
	end_code = clear_code + 1;
	max_code_size = 2 * clear_code;
	max_code = clear_code + 2;

	GetCode(src, 0, TRUE);

	fresh = TRUE;

	for (i = 0; i < clear_code; ++i) {
	    table[0][i] = 0;
	    table[1][i] = i;
	}
	for (; i < (1 << MAX_LWZ_BITS); ++i)
	    table[0][i] = table[1][0] = 0;

	sp = stack;

	return 0;
    } else if (fresh) {
	fresh = FALSE;
	do {
	    firstcode = oldcode = GetCode(src, code_size, FALSE);
	} while (firstcode == clear_code);
	return firstcode;
    }
    if (sp > stack)
	return *--sp;

    while ((code = GetCode(src, code_size, FALSE)) >= 0) {
	if (code == clear_code) {
	    for (i = 0; i < clear_code; ++i) {
		table[0][i] = 0;
		table[1][i] = i;
	    }
	    for (; i < (1 << MAX_LWZ_BITS); ++i)
		table[0][i] = table[1][i] = 0;
	    code_size = set_code_size + 1;
	    max_code_size = 2 * clear_code;
	    max_code = clear_code + 2;
	    sp = stack;
	    firstcode = oldcode = GetCode(src, code_size, FALSE);
	    return firstcode;
	} else if (code == end_code) {
	    int count;
	    unsigned char buf[260];

	    if (ZeroDataBlock)
		return -2;

	    while ((count = GetDataBlock(src, buf)) > 0);

	    if (count != 0) {
		/*
		 * EPRINTF("missing EOD in data stream (common occurence)");
		 */
	    }
	    return -2;
	}
	incode = code;

	if (code >= max_code) {
	    *sp++ = firstcode;
	    code = oldcode;
	}
	while (code >= clear_code) {
	    *sp++ = table[1][code];
	    if (code == table[0][code])
		EPRINTF("GdDecodeGIF: circular table entry\n");
	    code = table[0][code];
	}

	*sp++ = firstcode = table[1][code];

	if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
	    table[0][code] = oldcode;
	    table[1][code] = firstcode;
	    ++max_code;
	    if ((max_code >= max_code_size) &&
		(max_code_size < (1 << MAX_LWZ_BITS))) {
		max_code_size *= 2;
		++code_size;
	    }
	}
	oldcode = incode;

	if (sp > stack)
	    return *--sp;
    }
    return code;
}

static int
ReadImage(buffer_t* src, PMWIMAGEHDR pimage, int len, int height, int cmapSize,
	  unsigned char cmap[3][MAXCOLORMAPSIZE],
	  int gray, int interlace, int ignore)
{
    unsigned char c;
    int i, v;
    int xpos = 0, ypos = 0, pass = 0;

    /*
     *	Initialize the compression routines
     */
    if (!ReadOK(src, &c, 1)) {
	EPRINTF("GdDecodeGIF: EOF on image data\n");
	return 0;
    }
    if (LWZReadByte(src, TRUE, c) < 0) {
	EPRINTF("GdDecodeGIF: error reading image\n");
	return 0;
    }

    /*
     *	If this is an "uninteresting picture" ignore it.
     */
    if (ignore) {
	while (LWZReadByte(src, FALSE, c) >= 0);
	return 0;
    }
    /*image = ImageNewCmap(len, height, cmapSize);*/
    pimage->width = len;
    pimage->height = height;
    pimage->planes = 1;
    pimage->bpp = 8;
    GdComputeImagePitch(8, len, &pimage->pitch, &pimage->bytesperpixel);
    pimage->compression = 0;
    pimage->palsize = cmapSize;
    pimage->palette = malloc(256*sizeof(MWPALENTRY));
    pimage->imagebits = malloc(height*pimage->pitch);
    if(!pimage->imagebits || !pimage->palette)
	    return 0;

    for (i = 0; i < cmapSize; i++) {
	/*ImageSetCmap(image, i, cmap[CM_RED][i],
		     cmap[CM_GREEN][i], cmap[CM_BLUE][i]);*/
	pimage->palette[i].r = cmap[CM_RED][i];
	pimage->palette[i].g = cmap[CM_GREEN][i];
	pimage->palette[i].b = cmap[CM_BLUE][i];
    }

    while ((v = LWZReadByte(src, FALSE, c)) >= 0) {
	pimage->imagebits[ypos * pimage->pitch + xpos] = v;

	++xpos;
	if (xpos == len) {
	    xpos = 0;
	    if (interlace) {
		switch (pass) {
		case 0:
		case 1:
		    ypos += 8;
		    break;
		case 2:
		    ypos += 4;
		    break;
		case 3:
		    ypos += 2;
		    break;
		}

		if (ypos >= height) {
		    ++pass;
		    switch (pass) {
		    case 1:
			ypos = 4;
			break;
		    case 2:
			ypos = 2;
			break;
		    case 3:
			ypos = 1;
			break;
		    default:
			goto fini;
		    }
		}
	    } else {
		++ypos;
	    }
	}
	if (ypos >= height)
	    break;
    }

fini:
    return 1;
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_GIF_SUPPORT)*/
