/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Framebuffer data to .bmp file converter
 *
 * 10/4/1999 g haerr
 */
#include <stdio.h>
#include <unistd.h>

#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;
typedef long		LONG;

#pragma pack(1)
/* windows style*/
typedef struct {
	/* BITMAPFILEHEADER*/
	BYTE	bfType[2];
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
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
} BMPHEAD;
#pragma pack()

int		MakeBMP(FILE *ifp,FILE *ofp);

int
main(int ac,char **av)
{
	FILE	*ifp;
	FILE	*ofp;

	if(ac < 3) {
		fprintf(stderr, "Usage: makebmp <infile> <outfile>\n");
		exit(1);
	}
	ifp = fopen(av[1], "rb");
	if(!ifp) {
		fprintf(stderr, "Can't open file: %s\n", av[1]);
		exit(1);
	}
	ofp = fopen(av[2], "wb");
	if(!ofp) {
		fprintf(stderr, "Can't create file: %s\n", av[2]);
		exit(1);
	}
	if(!MakeBMP(ifp, ofp)) {
		fprintf(stderr, "Conversion failed: %s\n", av[2]);
		fclose(ofp);
		unlink(av[2]);
		exit(1);
	}
	fclose(ifp);
	fclose(ofp);
	return 0;
}

/* create a bmp file*/
int
MakeBMP(FILE *ifp, FILE *ofp)
{
	BMPHEAD	bmp;
	int	i, j;
	int	cx, cy, extra, bitdepth, ncolors;

	cx = 640;
	cy = 480;
	extra = (cx + 3) & 3;
	bitdepth = 8;
	if(bitdepth <= 8)
		ncolors = 1<<bitdepth;
	else ncolors = 0;

	memset(&bmp, 0, sizeof(bmp));
	bmp.bfType[0] = 'B';
	bmp.bfType[1] = 'M';
	bmp.bfSize = sizeof(bmp) + ncolors*4 + (long)cx*cy;
	bmp.bfOffBits = sizeof(bmp) + ncolors*4;
	bmp.BiSize = 40;
	bmp.BiWidth = cx;
	bmp.BiHeight = cy;
	bmp.BiPlanes = 1;
	bmp.BiBitCount = bitdepth;
	bmp.BiCompression = BI_RGB;
	//bmp.BiSizeImage = ??;
	bmp.BiClrUsed = ncolors;
	bmp.BiClrImportant = ncolors;

	/* write header*/
	fwrite(&bmp, sizeof(bmp), 1, ofp);

	/* write palette*/
	if(bitdepth <= 8) {
		for(i=0; i<ncolors; i++) {
			unsigned char r, g, b;
			r = getc(ifp);
			g = getc(ifp);
			b = getc(ifp);
			putc(b, ofp);
			putc(g, ofp);
			putc(r, ofp);
			putc(0, ofp);
		}
	}

	/* write image data, upside down ;)*/
	for(i=cy-1; i>0; --i) {
		long base = sizeof(bmp) + ncolors*4 + (long)i*cx;
		fseek(ofp, base, SEEK_SET);
		for(j=0; j<cx; ++j)
			putc(getc(ifp), ofp);
		for(j=0; j<extra; ++j)
			putc(0, ofp);		/* DWORD pad each line*/
	}
	return 1;
}
