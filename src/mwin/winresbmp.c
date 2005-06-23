/*
 * Copyright (c) 1999, 2000, 2005 Greg Haerr <greg@censoft.com>
 *
 * Windows BMP to Microwindows image converter
 *
 * 6/9/1999 g haerr
 *
 * 09/05/2003 Gabriele Brugnoni <gabrielebrugnoni@dveprojects.com>
 * Modified to read the bitmap resource from .res file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "device.h"
#include "windows.h"


/* separators for DOS & Unix file systems */
#define OS_FILE_SEPARATOR   "/\\"

#define PIX2BYTES(n)	(((n)+7)/8)

#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L


#if !_MINIX
#define	CASTWORD
#define	CASTDWORD
#define	CASTLONG
#else
/* The Minix ACK compiler cannot pack a structure so we do it the hardway */
#undef WORD
#undef DWORD
#undef LONG
typedef unsigned char WORD[2];
typedef unsigned char DWORD[4];
typedef unsigned char LONG[4];
#define	CASTWORD	*(unsigned short *)&
#define	CASTDWORD	*(unsigned long *)&
#define	CASTLONG	*(long *)&
#endif


/* windows style*/
typedef struct {
	/* BITMAPINFOHEADER */
	DWORD RESPACKEDDATA BiSize;
	LONG RESPACKEDDATA BiWidth;
	LONG RESPACKEDDATA BiHeight;
	WORD RESPACKEDDATA BiPlanes;
	WORD RESPACKEDDATA BiBitCount;
	DWORD RESPACKEDDATA BiCompression;
	DWORD RESPACKEDDATA BiSizeImage;
	LONG RESPACKEDDATA BiXpelsPerMeter;
	LONG RESPACKEDDATA BiYpelsPerMeter;
	DWORD RESPACKEDDATA BiClrUsed;
	DWORD RESPACKEDDATA BiClrImportant;
} BMPHEAD;

#define FIXSZ_BMPHEAD	(11*4 + 4*2 + 2*1)

static int DecodeRLE8(UCHAR * buf, UCHAR ** fp, int linesize);
static int DecodeRLE4(UCHAR * buf, UCHAR ** fp, int linesize);
static void put4(int b);

/*
 *  Load a bitmap from resource file.
 */
PMWIMAGEHDR
resLoadBitmap(HINSTANCE hInst, LPCTSTR resName)
{
	PMWIMAGEHDR pImageHdr, retV;
	MWPALENTRY *cmap;
	HGLOBAL hResBmp;
	BMPHEAD *pHead;
	unsigned int cx, cy, bitdepth, linesize;
	long compression;
	int i, palsize;
	int bytesperpixel;
	HRSRC hRes;
	UCHAR *pf, *pStart, *pLine;


	retV = NULL;

	hRes = FindResource(hInst, resName, RT_BITMAP);
	if (hRes == NULL)
		return NULL;

	if (SizeofResource(hInst, hRes) < FIXSZ_BMPHEAD)
		return NULL;

	hResBmp = LoadResource(hInst, hRes);
	pHead = LockResource(hResBmp);
	if (pHead == NULL)
		return NULL;

	do {
		pf = pStart = (UCHAR *) pHead;

		cx = (int) CASTLONG pHead->BiWidth;
		cy = (int) CASTLONG pHead->BiHeight;
		bitdepth = CASTWORD pHead->BiBitCount;
		palsize = (int) CASTDWORD pHead->BiClrUsed;
		if (palsize == 0)
			palsize = 1 << bitdepth;
		compression = CASTDWORD pHead->BiCompression;
		pf += FIXSZ_BMPHEAD;

		if (bitdepth > 8)
			palsize = 0;

		/* currently only 1, 4, 8 and 24 bpp bitmaps */
		if (bitdepth > 8 && bitdepth != 24) {
			EPRINTF("Error: bits per pixel must be 1, 4, 8 or 24\n");
			break;
		}

		/* compute image line size and allocate line buffer */
		if (bitdepth == 1)
			linesize = PIX2BYTES(cx), bytesperpixel = 1;
		else if (bitdepth <= 4)
			linesize = PIX2BYTES(cx << 2), bytesperpixel = 1;
		else if (bitdepth <= 8)
			linesize = cx, bytesperpixel = 1;
		else if (bitdepth <= 16)
			linesize = cx * 2, bytesperpixel = 2;
		else if (bitdepth <= 24)
			linesize = cx * 3, bytesperpixel = 3;
		else
			linesize = cx * 4, bytesperpixel = 4;

		linesize = (linesize + 3) & ~3;

		//  Allocate image header
		pImageHdr = (PMWIMAGEHDR) malloc(sizeof(MWIMAGEHDR));
		if (pImageHdr == NULL)
			break;

		pImageHdr->palette = NULL;
		pImageHdr->imagebits = NULL;

		//  If exits from this loop, release allocated mem and return err.
		do {
			pf = pStart + CASTDWORD pHead->BiSize;

			/* get colormap */
			if (bitdepth <= 8) {
				pImageHdr->palette = (MWPALENTRY *) malloc(256 *
							      sizeof(MWPALENTRY));
				if (pImageHdr->palette == NULL) {
					EPRINTF("Error allocating mem.\n");
					break;
				}
				cmap = pImageHdr->palette;
				for (i = 0; i < palsize; i++, cmap++) {
					cmap->b = *pf++;
					cmap->g = *pf++;
					cmap->r = *pf++;
					pf++;
				}
			}


			pImageHdr->imagebits = (MWUCHAR *) malloc(sizeof(MWUCHAR) *
						   linesize * cy);
			if (pImageHdr->imagebits == NULL) {
				EPRINTF("Error allocating mem.\n");
				break;
			}

			pLine = pImageHdr->imagebits;

			/* decode image data */
			if (compression == BI_RLE8) {
				for (i = cy - 1; i >= 0;
				     i--, pLine += linesize) {
					if (!DecodeRLE8(pLine, &pf, linesize))
						break;
				}
			} else if (compression == BI_RLE4) {
				for (i = cy - 1; i >= 0;
				     i--, pLine += linesize) {
					if (!DecodeRLE4(pLine, &pf, linesize))
						break;
				}
			} else
				memcpy(pLine, pf, linesize * cy);

			pImageHdr->width = cx;
			pImageHdr->height = cy;
			pImageHdr->planes = 1;
			pImageHdr->bpp = bitdepth;
			pImageHdr->pitch = linesize;
			pImageHdr->bytesperpixel = bytesperpixel;
			pImageHdr->compression = 1;
			pImageHdr->palsize = palsize;
			pImageHdr->transcolor = -1L;
			retV = pImageHdr;
		}
		while (0);
	}
	while (0);

	UnlockResource(hResBmp);
	FreeResource(hResBmp);

	if (retV == NULL) {
		if (pImageHdr->palette)
			free(pImageHdr->palette);
		if (pImageHdr->imagebits)
			free(pImageHdr->imagebits);
		if (pImageHdr)
			free(pImageHdr);
	}
	return retV;
}


/*
 *  Free memory allocated with resLoadBitmap
 */
void
resFreeBitmap(PMWIMAGEHDR pImageHdr)
{
	if (pImageHdr->palette)
		free(pImageHdr->palette);
	if (pImageHdr->imagebits)
		free(pImageHdr->imagebits);
	free(pImageHdr);
}

#define B_FGET(fp)	( *fp++ )

/*
 * Decode one line of RLE8, return 0 when done with all bitmap data
 */
static int
DecodeRLE8(UCHAR * buf, UCHAR ** pfp, int linesize)
{
	int c, n;
	UCHAR *fp = *pfp;
	UCHAR *p = buf;
	UCHAR *end = p + linesize;

	while (p < end) {
		switch (n = B_FGET(fp)) {
		case 0:	/* 0 = escape */
			switch (n = B_FGET(fp)) {
			case 0:	/* 0 0 = end of current scan line */
			case 1:	/* 0 1 = end of data */
				*pfp = fp;
				return (1);
			case 2:	/* 0 2 xx yy delta mode - NOT SUPPORTED */
				fp += 2;
				continue;
			default:	/* 0 3..255 xx nn uncompressed data */
				for (c = 0; c < n; c++)
					*p++ = B_FGET(fp);
				if (n & 1)
					fp++;
				continue;
			}
		default:
			c = B_FGET(fp);
			while (n--)
				*p++ = c;
			continue;
		}
	}
	return 0;
}

/*
 * Decode one line of RLE4, return 0 when done with all bitmap data
 */
static UCHAR *p;
static int once;

static void
put4(int b)
{
	static int last;

	last = (last << 4) | b;
	if (++once == 2) {
		*p++ = last;
		once = 0;
	}
}

static int
DecodeRLE4(UCHAR * buf, UCHAR ** pfp, int linesize)
{
	int c, n, c1, c2;
	UCHAR *fp = *pfp;
	UCHAR *end;

	p = buf;
	end = p + linesize;
	once = 0;
	while (p < end) {
		switch (n = B_FGET(fp)) {
		case 0:	/* 0 = escape */
			switch (n = B_FGET(fp)) {
			case 0:	/* 0 0 = end of current scan line */
			case 1:	/* 0 1 = end of data */
				if (once)
					put4(0);
				*pfp = fp;
				return (1);
			case 2:	/* 0 2 xx yy delta mode - NOT SUPPORTED */
				fp += 2;
				continue;
			default:	/* 0 3..255 xx nn uncompressed data */
				c2 = (n + 3) & ~3;
				for (c = 0; c < c2; c++) {
					if ((c & 1) == 0)
						c1 = B_FGET(fp);
					if (c < n)
						put4((c1 >> 4) & 0x0f);
					c1 <<= 4;
				}
				continue;
			}
		default:
			c = B_FGET(fp);
			c1 = (c >> 4) & 0x0f;
			c2 = c & 0x0f;
			for (c = 0; c < n; c++)
				put4((c & 1) ? c2 : c1);
			continue;
		}
	}
	return 0;
}
