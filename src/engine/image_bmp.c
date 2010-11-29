/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Martin Jolicoeur <martinj@visuaide.com>
 *
 * Image decode routine for BMP files
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

#if MW_FEATURE_IMAGES && defined(HAVE_BMP_SUPPORT)

/* BMP stuff*/
#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef uint32_t	DWORD;
typedef long		LONG;

typedef struct {
	/* BITMAPFILEHEADER*/
	BYTE	bfType[2];
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BMPFILEHEAD;

#define FILEHEADSIZE 14

/* windows style*/
typedef struct {
	/* BITMAPINFOHEADER*/
	DWORD	BiSize;
	DWORD	BiWidth;
	DWORD	BiHeight;
	WORD	BiPlanes;
	WORD	BiBitCount;
	DWORD	BiCompression;
	DWORD	BiSizeImage;
	DWORD	BiXpelsPerMeter;
	DWORD	BiYpelsPerMeter;
	DWORD	BiClrUsed;
	DWORD	BiClrImportant;
} BMPINFOHEAD;

#define INFOHEADSIZE 40

/* os/2 style*/
typedef struct {
	/* BITMAPCOREHEADER*/
	DWORD	bcSize;
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} BMPCOREHEAD;

#define COREHEADSIZE 12

static int	DecodeRLE8(MWUCHAR *buf, buffer_t *src);
static int	DecodeRLE4(MWUCHAR *buf, buffer_t *src);
static void	put4(int b);

void convblit_bgr888_rgb888(unsigned char *data, int width, int height, int pitch);
void convblit_bgrx8888_rgba8888(unsigned char *data, int width, int height, int pitch);

/*
 * Conversion blit 24bpp BGR to 24bpp RGB
 */
void convblit_bgr888_rgb888(unsigned char *data, int width, int height, int pitch)
{
	unsigned char *src = data;

	while (--height >= 0)
	{
		register unsigned char *s = src;
		int w = width;

		while (--w >= 0)
		{
			/* swap R and B*/
			unsigned char b = s[0];
			s[0] = s[2];
			s[2] = b;

			s += 3;
		}
		src += pitch;
	}
}

/*
 * Conversion blit 32bpp BGRX to 32bpp RGBA 255 alpha
 */
void convblit_bgrx8888_rgba8888(unsigned char *data, int width, int height, int pitch)
{
	unsigned char *src = data;

	while (--height >= 0)
	{
		register unsigned char *s = src;
		int w = width;

		while (--w >= 0)
		{
			/* swap R and B*/
			unsigned char b = s[0];
			s[0] = s[2];
			s[2] = b;
			s[3] = 255;		/* alpha*/

			s += 4;
		}
		src += pitch;
	}
}

/*
 * BMP decoding routine
 */
int
GdDecodeBMP(buffer_t *src, PMWIMAGEHDR pimage)
{
	int		h, i, compression;
	int		headsize;
	MWUCHAR		*imagebits;
	BMPFILEHEAD	bmpf;
	BMPINFOHEAD	bmpi;
	BMPCOREHEAD	bmpc;
	MWUCHAR 	headbuffer[INFOHEADSIZE];

	GdImageBufferSeekTo(src, 0UL);

	pimage->imagebits = NULL;
	pimage->palette = NULL;

	/* read BMP file header*/
	if (GdImageBufferRead(src, &headbuffer, FILEHEADSIZE) != FILEHEADSIZE)
		return 0;

	bmpf.bfType[0] = headbuffer[0];
	bmpf.bfType[1] = headbuffer[1];

	/* Is it really a bmp file ? */
	if (*(WORD*)&bmpf.bfType[0] != wswap(0x4D42)) /* 'BM' */
		return 0;	/* not bmp image*/

	/*bmpf.bfSize = dwswap(dwread(&headbuffer[2]));*/
	bmpf.bfOffBits = dwswap(dwread(&headbuffer[10]));

	/* Read remaining header size */
	if (GdImageBufferRead(src,&headsize,sizeof(DWORD)) != sizeof(DWORD))
		return 0;	/* not bmp image*/
	headsize = dwswap(headsize);

	/* might be windows or os/2 header */
	if(headsize == COREHEADSIZE) {

		/* read os/2 header */
		if(GdImageBufferRead(src, &headbuffer, COREHEADSIZE-sizeof(DWORD)) !=
			COREHEADSIZE-sizeof(DWORD))
				return 0;	/* not bmp image*/

		/* Get data */
		bmpc.bcWidth = wswap(*(WORD*)&headbuffer[0]);
		bmpc.bcHeight = wswap(*(WORD*)&headbuffer[2]);
		bmpc.bcPlanes = wswap(*(WORD*)&headbuffer[4]);
		bmpc.bcBitCount = wswap(*(WORD*)&headbuffer[6]);
		
		pimage->width = (int)bmpc.bcWidth;
		pimage->height = (int)bmpc.bcHeight;
		pimage->bpp = bmpc.bcBitCount;
		if (pimage->bpp <= 8)
			pimage->palsize = 1 << pimage->bpp;
		else pimage->palsize = 0;
		compression = BI_RGB;
	} else {
		/* read windows header */
		if(GdImageBufferRead(src, &headbuffer, INFOHEADSIZE-sizeof(DWORD)) !=
			INFOHEADSIZE-sizeof(DWORD))
				return 0;	/* not bmp image*/

		/* Get data */
		bmpi.BiWidth = dwswap(*(DWORD*)&headbuffer[0]);
		bmpi.BiHeight = dwswap(*(DWORD*)&headbuffer[4]);
		bmpi.BiPlanes = wswap(*(WORD*)&headbuffer[8]);
		bmpi.BiBitCount = wswap(*(WORD*)&headbuffer[10]);
		bmpi.BiCompression = dwswap(*(DWORD*)&headbuffer[12]);
		bmpi.BiSizeImage = dwswap(*(DWORD*)&headbuffer[16]);
		bmpi.BiXpelsPerMeter = dwswap(*(DWORD*)&headbuffer[20]);
		bmpi.BiYpelsPerMeter = dwswap(*(DWORD*)&headbuffer[24]);
		bmpi.BiClrUsed = dwswap(*(DWORD*)&headbuffer[28]);
		bmpi.BiClrImportant = dwswap(*(DWORD*)&headbuffer[32]);

		pimage->width = (int)bmpi.BiWidth;
		pimage->height = (int)bmpi.BiHeight;
		pimage->bpp = bmpi.BiBitCount;
		pimage->palsize = (int)bmpi.BiClrUsed;
		if (pimage->palsize > 256)
			pimage->palsize = 0;
		else if(pimage->palsize == 0 && pimage->bpp <= 8)
			pimage->palsize = 1 << pimage->bpp;
		compression = bmpi.BiCompression;
	}
printf("bmp bpp %d\n", pimage->bpp);
	pimage->compression = MWIMAGE_RGB;	/* right side up, BGR order will be converted to RGB*/
	pimage->data_format = 0;		/* force GdDrawImage for now*/
	pimage->planes = 1;

	/* only 1, 4, 8, 16, 24 and 32 bpp bitmaps*/
	switch(pimage->bpp) {
	case 1:
	case 4:
	case 8:
	case 16:
	case 24:
	case 32:
		break;
	default:
		EPRINTF("GdDecodeBMP: image bpp not 1, 4, 8, 16, 24 or 32\n");
		return 2;	/* image loading error*/
	}

	/* compute byte line size and bytes per pixel*/
	GdComputeImagePitch(pimage->bpp, pimage->width, &pimage->pitch, &pimage->bytesperpixel);

	/* Allocate image */
	if( (pimage->imagebits = malloc(pimage->pitch*pimage->height)) == NULL)
		goto err;
	if( (pimage->palette = malloc(256*sizeof(MWPALENTRY))) == NULL)
		goto err;

	/* get colormap*/
	if(pimage->bpp <= 8) {
		for(i=0; i<pimage->palsize; i++) {
			pimage->palette[i].b = GdImageBufferGetChar(src);
			pimage->palette[i].g = GdImageBufferGetChar(src);
			pimage->palette[i].r = GdImageBufferGetChar(src);
			if(headsize != COREHEADSIZE)
				GdImageBufferGetChar(src);
		}
	}

	/* determine 16bpp 5/5/5 or 5/6/5 format*/
	if (pimage->bpp == 16) {
		uint32_t format = 0x7c00;		/* default is 5/5/5*/

		if (compression == BI_BITFIELDS) {
			MWUCHAR	buf[4];

			if (GdImageBufferRead(src, &buf, sizeof(DWORD)) != sizeof(DWORD))
				goto err;
			format = dwswap(dwread(buf));
		}
		if (format == 0x7c00)
			pimage->compression |= MWIMAGE_555;
		/* else it's 5/6/5 format, no flag required*/
	}

//	printf("BMP format %d bpp", pimage->bpp);
//	if (pimage->compression & MWIMAGE_555) printf(" 5/5/5");
//	printf("\n");

	/* decode image data*/
	GdImageBufferSeekTo(src, bmpf.bfOffBits);

	h = pimage->height;
	/* For every row ... */
	while (--h >= 0) {
		/* turn image rightside up*/
		imagebits = pimage->imagebits + h*pimage->pitch;

		/* Get row data from file */
		if(compression == BI_RLE8) {
			if(!DecodeRLE8(imagebits, src))
				break;
		} else if(compression == BI_RLE4) {
			if(!DecodeRLE4(imagebits, src))
				break;
		} else {
			if(GdImageBufferRead(src, imagebits, pimage->pitch) != pimage->pitch)
					goto err;
		}
	}

	/* conv BGR -> RGB*/
	if (pimage->bpp == 24)
		convblit_bgr888_rgb888(imagebits, pimage->width, pimage->height, pimage->pitch);
	else if (pimage->bpp == 32)
		convblit_bgrx8888_rgba8888(imagebits, pimage->width, pimage->height, pimage->pitch);
	return 1;		/* bmp image ok*/
	
err:
	EPRINTF("GdDecodeBMP: image loading error\n");
	if(pimage->imagebits)
		free(pimage->imagebits);
	if(pimage->palette)
		free(pimage->palette);
	return 2;		/* bmp image error*/
}

/*
 * Decode one line of RLE8, return 0 when done with all bitmap data
 */
static int
DecodeRLE8(MWUCHAR * buf, buffer_t * src)
{
	int c, n;
	MWUCHAR *p = buf;

	for (;;) {
		switch (n = GdImageBufferGetChar(src)) {
		case EOF:
			return 0;
		case 0:	/* 0 = escape */
			switch (n = GdImageBufferGetChar(src)) {
			case 0:		/* 0 0 = end of current scan line */
				return 1;
			case 1:		/* 0 1 = end of data */
				return 1;
			case 2:		/* 0 2 xx yy delta mode NOT SUPPORTED */
				(void) GdImageBufferGetChar(src);
				(void) GdImageBufferGetChar(src);
				continue;
			default:	/* 0 3..255 xx nn uncompressed data */
				for (c = 0; c < n; c++)
					*p++ = GdImageBufferGetChar(src);
				if (n & 1)
					(void) GdImageBufferGetChar(src);
				continue;
			}
		default:
			c = GdImageBufferGetChar(src);
			while (n--)
				*p++ = c;
			continue;
		}
	}
}

/*
 * Decode one line of RLE4, return 0 when done with all bitmap data
 */
static MWUCHAR *p;
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
DecodeRLE4(MWUCHAR * buf, buffer_t * src)
{
	int c, n, c1, c2;

	p = buf;
	once = 0;
	c1 = 0;

	for (;;) {
		switch (n = GdImageBufferGetChar(src)) {
		case EOF:
			return 0;
		case 0:	/* 0 = escape */
			switch (n = GdImageBufferGetChar(src)) {
			case 0:		/* 0 0 = end of current scan line */
				if (once)
					put4(0);
				return 1;
			case 1:		/* 0 1 = end of data */
				if (once)
					put4(0);
				return 1;
			case 2:		/* 0 2 xx yy delta mode NOT SUPPORTED */
				(void) GdImageBufferGetChar(src);
				(void) GdImageBufferGetChar(src);
				continue;
			default:	/* 0 3..255 xx nn uncompressed data */
				c2 = (n + 3) & ~3;
				for (c = 0; c < c2; c++) {
					if ((c & 1) == 0)
						c1 = GdImageBufferGetChar(src);
					if (c < n)
						put4((c1 >> 4) & 0x0f);
					c1 <<= 4;
				}
				continue;
			}
		default:
			c = GdImageBufferGetChar(src);
			c1 = (c >> 4) & 0x0f;
			c2 = c & 0x0f;
			for (c = 0; c < n; c++)
				put4((c & 1) ? c2 : c1);
			continue;
		}
	}
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_BMP_SUPPORT) */
