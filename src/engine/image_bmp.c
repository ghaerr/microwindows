/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Martin Jolicoeur <martinj@visuaide.com>
 *
 * Image decode routine for BMP files
 */
#include <stdio.h>
#include <string.h>
#include "uni_std.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "device.h"
#include "../drivers/genmem.h"
#include "convblit.h"
#include "swap.h"

/* BMP stuff*/
#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef uint32_t		DWORD;
typedef int32_t			LONG;

typedef struct {
	/* BITMAPFILEHEADER*/
	BYTE	bfType[2];
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} MWPACKED BMPFILEHEAD;

/* windows style*/
typedef struct {
	/* BITMAPINFOHEADER*/
	DWORD	BiSize;
	LONG	BiWidth;
	LONG	BiHeight;
	WORD	BiPlanes;
	WORD	BiBitCount;
	DWORD	BiCompression;
	DWORD	BiSizeImage;
	LONG	BiXpelsPerMeter;
	LONG	BiYpelsPerMeter;
	DWORD	BiClrUsed;
	DWORD	BiClrImportant;
} MWPACKED BMPINFOHEAD;

/* os/2 style*/
typedef struct {
	/* BITMAPCOREHEADER*/
	DWORD	bcSize;
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} MWPACKED BMPCOREHEAD;

#if MW_FEATURE_IMAGES && HAVE_BMP_SUPPORT

static int	DecodeRLE8(MWUCHAR *buf, buffer_t *src);
static int	DecodeRLE4(MWUCHAR *buf, buffer_t *src);
static void	put4(int b);

/**
 * Convert 32-bit little endian number at addr,
 * possibly not aligned, to host CPU format.
 */
static inline void little_endian_to_host_32(void *addr)
{
#if MW_CPU_BIG_ENDIAN
	unsigned char b0 = ((unsigned char *)addr)[0];
	unsigned char b1 = ((unsigned char *)addr)[1];
	unsigned char b2 = ((unsigned char *)addr)[2];
	((unsigned char *)addr)[0] = ((unsigned char *)addr)[3];
	((unsigned char *)addr)[1] = b2;
	((unsigned char *)addr)[2] = b1;
	((unsigned char *)addr)[3] = b0;
#endif
}

/**
 * Convert 16-bit little endian number at addr,
 * possibly not aligned, to host CPU format.
 */
static inline void little_endian_to_host_16(void *addr)
{
#if MW_CPU_BIG_ENDIAN
	unsigned char b0 = ((unsigned char *)addr)[0];
	((unsigned char *)addr)[0] = ((unsigned char *)addr)[1];
	((unsigned char *)addr)[1] = b0;
#endif
}

/*
 * BMP decoding routine
 */
PSD
GdDecodeBMP(buffer_t *src, MWBOOL readfilehdr)
{
	int			h, i, compression, width, height, bpp, data_format, palsize;
	PSD			pmd;
	DWORD		hdrsize;
	BMPFILEHEAD	bmpf;

	GdImageBufferSeekTo(src, 0L);

	/* read BMP file header*/
	if (readfilehdr) {
		if (GdImageBufferRead(src, &bmpf, sizeof(bmpf)) != sizeof(bmpf))
			return NULL;

		/* check magic bytes*/
		if (bmpf.bfType[0] != 'B' || bmpf.bfType[1] != 'M')
			return NULL;		/* not bmp image*/

		little_endian_to_host_32(&bmpf.bfOffBits);
	}

	/* Read header size to determine header type*/
	if (GdImageBufferRead(src, &hdrsize, sizeof(hdrsize)) != sizeof(hdrsize))
		return 0;				/* not bmp image*/
	little_endian_to_host_32(&hdrsize);

	/* might be windows or os/2 header */
	if(hdrsize == sizeof(BMPCOREHEAD)) {
		BMPCOREHEAD	bmpc;

		/* read os/2 header */
		if (GdImageBufferRead(src, &bmpc.bcWidth, sizeof(bmpc)-sizeof(DWORD)) !=
			sizeof(bmpc)-sizeof(DWORD))
				return NULL;	/* not bmp image*/

		little_endian_to_host_16(&bmpc.bcWidth);
		little_endian_to_host_16(&bmpc.bcHeight);
		little_endian_to_host_16(&bmpc.bcBitCount);
		little_endian_to_host_16(&bmpc.bcWidth);
		
		compression = BI_RGB;
		width = bmpc.bcWidth;
		height = bmpc.bcHeight;
		bpp = bmpc.bcBitCount;
		if (bpp <= 8) palsize = 1 << bpp;
		else palsize = 0;
	} else {
		BMPINFOHEAD	bmpi;

		/* read windows header */
		if (GdImageBufferRead(src, &bmpi.BiWidth, sizeof(bmpi)-sizeof(DWORD))
			!= sizeof(bmpi)-sizeof(DWORD))
				return NULL;	/* not bmp image*/

		little_endian_to_host_32(&bmpi.BiWidth);
		little_endian_to_host_32(&bmpi.BiHeight);
		little_endian_to_host_16(&bmpi.BiBitCount);
		little_endian_to_host_32(&bmpi.BiCompression);
		little_endian_to_host_32(&bmpi.BiClrUsed);

		compression = bmpi.BiCompression;
		width = bmpi.BiWidth;
		height = bmpi.BiHeight;
		bpp = bmpi.BiBitCount;
		palsize = bmpi.BiClrUsed;
		if (palsize > 256) palsize = 0;
		else if (palsize == 0 && bpp <= 8) palsize = 1 << bpp;
	}
DPRINTF("bmp bpp %d pal %d\n", bpp, palsize);

	/* only 1, 4, 8, 16, 24 and 32 bpp bitmaps*/
	switch(bpp) {
	case 1:
		data_format = MWIF_PAL1;
		break;
	case 4:
		data_format = MWIF_PAL4;
		break;
	case 8:
		data_format = MWIF_PAL8;
		break;
	case 16:
		data_format = MWIF_RGB565;
		break;
	case 24:
		data_format = MWIF_RGB888;		/* BGR will be converted to RGB*/
		break;
	case 32:
		data_format = MWIF_RGBA8888;	/* converted to 32bpp RGBA w/255 alpha*/
		break;
	default:
		EPRINTF("GdDecodeBMP: image bpp not 1, 4, 8, 16, 24 or 32\n");
		return NULL;
	}

	pmd = GdCreatePixmap(&scrdev, width, height, data_format, NULL, palsize);

	/* get colormap*/
	if (bpp <= 8) {
		for (i=0; i<palsize; i++) {
			pmd->palette[i].b = GdImageBufferGetChar(src);
			pmd->palette[i].g = GdImageBufferGetChar(src);
			pmd->palette[i].r = GdImageBufferGetChar(src);
			if (hdrsize != sizeof(BMPCOREHEAD))
				GdImageBufferGetChar(src);
		}
	}

	/* determine 16bpp 5/5/5 or 5/6/5 format*/
	if (bpp == 16) {
		DWORD format = 0x7c00;		/* default is 5/5/5*/

		if (compression == BI_BITFIELDS) {
			if (GdImageBufferRead(src, &format, sizeof(format)) != sizeof(format))
				goto err;
			little_endian_to_host_32(&format);
		}
		if (format == 0x7c00)
			pmd->data_format = MWIF_RGB555;
		/* else it's 5/6/5 format, no flag required*/
	}

	/* decode image data*/
	if (readfilehdr)
		GdImageBufferSeekTo(src, bmpf.bfOffBits);

	h = height;
	/* For every row ... */
	while (--h >= 0) {
		/* turn image rightside up*/
		MWUCHAR *imagebits = ((unsigned char *)pmd->addr) + h * pmd->pitch;

		/* Get row data from file, images are DWORD right aligned */
		if(compression == BI_RLE8) {
			if(!DecodeRLE8(imagebits, src))
				break;
		} else if(compression == BI_RLE4) {
			if(!DecodeRLE4(imagebits, src))
				break;
		} else {
			if(GdImageBufferRead(src, imagebits, pmd->pitch) != pmd->pitch)
				goto err;
		}
	}

	/* conv BGR -> RGB*/
	if (bpp == 24)
		convblit_bgr888_rgb888(pmd->addr, width, height, pmd->pitch);
	else if (bpp == 32)
		convblit_bgrx8888_rgba8888(pmd->addr, width, height, pmd->pitch);
	return pmd;
	
err:
	EPRINTF("GdDecodeBMP: image loading error\n");
	GdFreePixmap(pmd);
	return NULL;
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
#endif /* MW_FEATURE_IMAGES && HAVE_BMP_SUPPORT*/

#if DEBUG
/* little-endian storage of shortword*/
static void
putsw(uint32_t dw, FILE *ofp)
{
	fputc((unsigned char)dw, ofp);
	dw >>= 8;
	fputc((unsigned char)dw, ofp);
}

/* little-endian storage of longword*/
static void
putdw(uint32_t dw, FILE *ofp)
{
	fputc((unsigned char)dw, ofp);
	dw >>= 8;
	fputc((unsigned char)dw, ofp);
	dw >>= 8;
	fputc((unsigned char)dw, ofp);
	dw >>= 8;
	fputc((unsigned char)dw, ofp);
}

/**
 * Create .bmp file from framebuffer data
 * 1, 4, 8, 16, 24 and 32 bpp supported
 *
 * @param path Output file.
 * @return 0 on success, nonzero on error.
 */
int
GdCaptureScreen(PSD psd, char *pathname)
{
	FILE *ofp;
	int	w, h, i, cx, cy, extra, bpp, bytespp, ncolors, sizecolortable;
	int hdrsize, imagesize, filesize, compression, colorsused;
	BMPFILEHEAD	bmpf;
	BMPINFOHEAD bmpi;
	MWSCREENINFO sinfo;
	extern MWPALENTRY gr_palette[256];    /* current palette*/

	ofp = fopen(pathname, "wb");
	if (!ofp)
		return 1;

	if (!psd)
		psd = &scrdev;
	GdGetScreenInfo(psd, &sinfo);

	cx = psd->xres;
	cy = psd->yres;
	bpp = psd->bpp;
	bytespp = (bpp+7)/8;

	/* dword right padded*/
	extra = (cx*bytespp) & 3;
	if (extra)
		extra = 4 - extra;

	ncolors = (bpp <= 8)? (1<<bpp): 0;

	/* color table is either palette or 3 longword r/g/b masks*/
	sizecolortable = ncolors? ncolors*4: 3*4;
	if (bpp == 24)
		sizecolortable = 0;	/* special case 24bpp has no table*/

	hdrsize = sizeof(bmpf) + sizeof(bmpi) + sizecolortable;
	imagesize = (cx + extra) * cy * bytespp;
	filesize =  hdrsize + imagesize;
	compression = (bpp == 16 || bpp == 32)? BI_BITFIELDS: BI_RGB;
	colorsused = (bpp <= 8)? ncolors: 0;

	/* fill out headers*/
	memset(&bmpf, 0, sizeof(bmpf));
	bmpf.bfType[0] = 'B';
	bmpf.bfType[1] = 'M';
	bmpf.bfSize = host_to_little_endian_32(filesize);
	bmpf.bfOffBits = host_to_little_endian_32(hdrsize);

	memset(&bmpi, 0, sizeof(bmpi));
	bmpi.BiSize = host_to_little_endian_32(sizeof(BMPINFOHEAD));
	bmpi.BiWidth = host_to_little_endian_32(cx);
	bmpi.BiHeight = host_to_little_endian_32(cy);
	bmpi.BiPlanes = host_to_little_endian_16(1);
	bmpi.BiBitCount = host_to_little_endian_16(bpp);
	bmpi.BiCompression = host_to_little_endian_32(compression);
	bmpi.BiSizeImage = host_to_little_endian_32(imagesize);
	bmpi.BiClrUsed = host_to_little_endian_32(colorsused);

	/* write headers*/
	fwrite(&bmpf, sizeof(bmpf), 1, ofp);
	fwrite(&bmpi, sizeof(bmpi), 1, ofp);

	/* write colortable*/
	if (sizecolortable) {
		if(bpp <= 8) {
			/* write palette*/
			for(i=0; i<ncolors; i++) {
				fputc(gr_palette[i].b, ofp);
				fputc(gr_palette[i].g, ofp);
				fputc(gr_palette[i].r, ofp);
				fputc(0, ofp);
			}
		} else {
			/* write 3 r/g/b masks*/
			if (psd->data_format == MWIF_RGBA8888) {
				/* RGBA bmp format not supported, will convert to BGRA*/
				putdw(RMASKBGRA, ofp);
				putdw(GMASKBGRA, ofp);
				putdw(BMASKBGRA, ofp);
			} else {
				putdw(sinfo.rmask, ofp);
				putdw(sinfo.gmask, ofp);
				putdw(sinfo.bmask, ofp);
			}
		}
	}

	/* write image data, upside down ;)*/
	for(h=cy-1; h>=0; --h) {
		unsigned char *src = ((unsigned char *)psd->addr) + h * cx * bytespp;
		unsigned char *cptr;
		unsigned short *sptr;
		uint32_t *lptr, c;

		switch (psd->data_format) {
		case MWIF_BGRA8888:
			lptr = (uint32_t *)src;
			for(w=0; w<cx; ++w)
				putdw(*lptr++, ofp);
			break;
		case MWIF_RGBA8888:
			lptr = (uint32_t *)src;
			for(w=0; w<cx; ++w) {
				c = *lptr++;
				putdw(PIXELABGR2PIXEL8888(c), ofp);	/* convert RGBA image pixel to BGRA image pixel*/
			}
			break;
		case MWIF_BGR888:
			cptr = (unsigned char *)src;
			for(w=0; w<cx; ++w) {
				fputc(*cptr++, ofp);
				fputc(*cptr++, ofp);
				fputc(*cptr++, ofp);
			}
			break;
		case MWIF_RGB565:
		case MWIF_RGB555:
			sptr = (unsigned short *)src;
			for(w=0; w<cx; ++w)
				putsw(*sptr++, ofp);
			break;
		case MWIF_RGB332:
		case MWIF_BGR233:
		case MWPF_PALETTE:
		default:
			cptr = (unsigned char *)src;
			for(w=0; w<cx; ++w)
				fputc(*cptr++, ofp);
			break;
		}
		for(w=0; w<extra; ++w)
			fputc(0, ofp);		/* DWORD pad each line*/
	}

	fclose(ofp);
	return 0;
}
#endif /* DEBUG*/
