/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Windows BMP to Microwindows image converter
 *
 * 6/9/1999 g haerr
 *
 * 05/01/2000 Michael Temari <Michael@TemWare.Com>
 * Modified to output .s ACK format for Minix
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "device.h"

/* separators for DOS & Unix file systems */
#define OS_FILE_SEPARATOR   "/\\"

#define PIX2BYTES(n)	(((n)+7)/8)

#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L

typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

#if !_MINIX
typedef unsigned short	WORD;
typedef unsigned long	DWORD;
typedef long		LONG;
#define	CASTWORD
#define	CASTDWORD
#define	CASTLONG
#else
/* The Minix ACK compiler cannot pack a structure so we do it the hardway */
typedef unsigned char	WORD[2];
typedef unsigned char	DWORD[4];
typedef unsigned char	LONG[4];
#define	CASTWORD	*(unsigned short *)&
#define	CASTDWORD	*(unsigned long *)&
#define	CASTLONG	*(long *)&
#endif

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

/* os/2 style*/
typedef struct {
	/* BITMAPFILEHEADER*/
	BYTE	bfType[2];
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
	/* BITMAPCOREHEADER*/
	DWORD	bcSize;
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} BMPCOREHEAD;
#pragma pack()

int	ConvBMP(FILE *fp,char *name);
void	outline(UCHAR *linebuffer, int bitdepth, int linesize, int y);
int	DecodeRLE8(UCHAR *buf,FILE *fp);
int	DecodeRLE4(UCHAR *buf,FILE *fp);
void	put4(int b);

int	s_flag = 0;

int main(int argc,char *argv[])
{
FILE	*fp;
char 	*p;
char	name[64];
char	oname[64];

   /* skip the program name */
   argc--;
   argv++;

   /* check for -s flag */
   if(argc && strcmp(*argv, "-s") == 0) {
	s_flag = -1;
	argc--;
	argv++;
   }

   /* need at least one file to convert */
   if(argc == 0) {
	fprintf(stderr, "Usage: convbmp [-s] <bmpfile>\n");
	return(-1);
   }

   /* go back one since the first thing in the loop is to increment */
   argv--;
   while(argc--) {
	argv++;
	if((p = strrchr(*argv, '.')) != (char *)NULL)
		*p = '\0';
	strcpy(name, *argv);
	strcpy(oname, *argv);
	if(s_flag)
		strcat(oname, ".s");
	else
		strcat(oname, ".c");
	if(p != (char *)NULL)
		*p = '.';
	if((fp = fopen(*argv, "rb")) == (FILE *)NULL) {
		fprintf(stderr, "Can't open file: %s\n", *argv);
		continue;
	}
	if(freopen(oname, "w", stdout) == (FILE *)NULL) {
		fclose(fp);
		fprintf(stderr, "Could not open output file %s\n", oname);
		continue;
	}

   /* 
      let's strip any directory from the path passed in.
      This prevent problems if this utility is given 
      path is not for the curent directory.
   */
   p = name;
   while( 1 )
   {
      char *p1 = p;
      p1 = strpbrk( p, OS_FILE_SEPARATOR );
      if( p1 )
      {
         p = p1+1;
      }
      else
      {
          break;
      }
   }
	if(ConvBMP(fp, p)) {
		fprintf(stderr, "Conversion failed: %s\n", *argv);
		fclose(fp);
		continue;
	}
	fclose(fp);
   }
   return(0);
}

/* decode a bmp file*/
int ConvBMP(FILE *fp, char *name)
{
BMPHEAD		bmp;
BMPCOREHEAD	*bmc;
int		i, palsize;
UCHAR		*linebuffer = NULL;
unsigned int	cx, cy, bitdepth, linesize;
long		compression;
MWPALENTRY	cmap[256];
long l;
int g;
int bytesperpixel;
UCHAR *p = (UCHAR *)&l;

   /* read BMP header*/
   if(fread(&bmp, 1, sizeof(BMPHEAD), fp) != sizeof(BMPHEAD)) {
   	fprintf(stderr, "Error reading .bmp file header\n");
	return(-1);
   }

   /* might be windows or os/2 header*/
   if(CASTDWORD bmp.BiSize == 12) {
	bmc = (BMPCOREHEAD *)&bmp;
	cx = (int)CASTWORD bmc->bcWidth;
	cy = (int)CASTWORD bmc->bcHeight;
	bitdepth = CASTWORD bmc->bcBitCount;
	palsize = 1 << bitdepth;
	compression = BI_RGB;
	fseek(fp, sizeof(BMPCOREHEAD), SEEK_SET);
   } else {
	cx = (int)CASTLONG bmp.BiWidth;
	cy = (int)CASTLONG bmp.BiHeight;
	bitdepth = CASTWORD bmp.BiBitCount;
	palsize = (int)CASTDWORD bmp.BiClrUsed;
	if(palsize == 0)
		palsize = 1 << bitdepth;
	compression = CASTDWORD bmp.BiCompression;
   }

   if(bitdepth > 8)
	   palsize = 0;

   /* currently only 1, 4, 8 and 24 bpp bitmaps*/
   if(bitdepth > 8 && bitdepth != 24) {
	fprintf(stderr, "Error: bits per pixel must be 1, 4, 8 or 24\n");
	return(-1);
   }

   /* compute image line size and allocate line buffer*/
   if(bitdepth == 1) {
	linesize = PIX2BYTES(cx);
	bytesperpixel = 1;
   } else if(bitdepth <= 4) {
	linesize = PIX2BYTES(cx<<2);
	bytesperpixel = 1;
   } else if(bitdepth <= 8) {
	linesize = cx;
	bytesperpixel = 1;
   } else if(bitdepth <= 16) {
	linesize = cx * 2;
	bytesperpixel = 2;
   } else if(bitdepth <= 24) {
	linesize = cx * 3;
	bytesperpixel = 3;
   } else {
	linesize = cx * 4;
	bytesperpixel = 4;
   }

   linesize = (linesize+3) & ~3;

   if((linebuffer = malloc(linesize)) == (UCHAR *)NULL) {
   	fprintf(stderr, "Error with malloc(%d)\n", linesize);
	return(-1);
   }

   if(!s_flag) {
	printf("/* Generated by convbmp*/\n");
	printf("#include \"device.h\"\n\n");
	printf("/* MWIMAGEHDR image_%s converted from %s.bmp*/\n\n",
		name, name);
   }

   /* get colormap*/
   if(bitdepth <= 8) {
	for(i=0; i<palsize; i++) {
		cmap[i].b = fgetc(fp);
		cmap[i].g = fgetc(fp);
		cmap[i].r = fgetc(fp);
		if(CASTDWORD bmp.BiSize != 12)
			fgetc(fp);
	}

	if(!s_flag) {
		/* extract palette*/
		printf("static MWPALENTRY palette[%d] = {\n", palsize);
		for(i=0; i<palsize; ++i)
			printf("  RGBDEF( %3d, %3d, %3d ),\t/* pal %d*/\n",
				cmap[i].r, cmap[i].g, cmap[i].b, i);
		printf("};\n\n");
	} else {
		printf(".sect .text; .sect .rom; .sect .data; .sect .bss\n");
		printf(".sect .data\n");
		printf("__II0:\n");
		l = 0L; g = 0; p = (UCHAR *)&l;
		for(i=0; i<palsize; ++i) {
			p[g++] = cmap[i].r;
			if(g == 4)
				{ g = 0; printf(".data4\t%ld\n", l); l = 0L; }
			p[g++] = cmap[i].g;
			if(g == 4)
				{ g = 0; printf(".data4\t%ld\n", l); l = 0L; }
			p[g++] = cmap[i].b;
			if(g == 4)
				{ g = 0; printf(".data4\t%ld\n", l); l = 0L; }
		}
		if(g)
			printf(".data4\t%ld\n", l);
	}
   }

   if(!s_flag) {
	printf("static MWUCHAR imagebits[] = {\n");
   } else {
	printf("__II1:\n");
	l = 0L; g = 0;
   }

   /* decode image data*/
   fseek(fp, CASTDWORD bmp.bfOffBits, SEEK_SET);
   if(compression == BI_RLE8) {
	for(i = cy-1; i>= 0; i--) {
		if(!DecodeRLE8(linebuffer, fp))
			break;
		outline(linebuffer, bitdepth, linesize, i);
	}
   } else if(compression == BI_RLE4) {
	for(i = cy-1; i>= 0; i--) {
		if(!DecodeRLE4(linebuffer, fp))
			break;
		outline(linebuffer, bitdepth, linesize, i);
	}
   } else {
	for(i=0; i<cy; i++) {
		if(fread(linebuffer, 1, linesize, fp) != (size_t)linesize) {
			free(linebuffer);
			fprintf(stderr, "Error fread\n");
			return(-1);
		}
		outline(linebuffer, bitdepth, linesize, cy-i-1);
	}
   }

   if(!s_flag) {
	printf("};\n\n");

	printf("MWIMAGEHDR image_%s = {\n", name);
	printf("  %d, %d,\t/* width, height*/\n", cx, cy);
	printf("  %d, %d,\t\t/* planes, bpp*/\n", 1, bitdepth);
	printf("  %d, %d,\t/* pitch, bytesperpixel*/\n",
		linesize, bytesperpixel);
	printf("  %d, %d,\t/* compression, palsize*/\n", 1, palsize);
	printf("  %ldL,\t\t/* transcolor*/\n", -1L);
	if(palsize)
		printf("  palette,\n");
	else
		printf("  0,\n");
	printf("  imagebits,\n");
	printf("};\n");
   } else {
	printf(".extern _image_%s\n", name);
	printf("_image_%s:\n", name);
	printf(".data4\t%ld\n",(long)cx);
	printf(".data4\t%ld\n",(long)cy);
	printf(".data4\t%ld\n",1L);
	printf(".data4\t%ld\n",(long)bitdepth);
	printf(".data4\t%ld\n",(long)linesize);
	printf(".data4\t%ld\n",(long)bytesperpixel);
	printf(".data4\t%ld\n",1L);
	printf(".data4\t%ld\n",(long)palsize);
	printf(".data4\t%ld\n",-1L);
	printf(".data4\t__II0\n");
	if(palsize)
		printf(".data4\t__II1\n");
	else
		printf(".data4\t0\n");
	printf(".sect .text\n");
   }

   free(linebuffer);

   return(0);
}

void outline(UCHAR *linebuffer, int bitdepth, int linesize, int y)
{
static int bc = 0;
static long l = 0;
static char *p = (char *)&l;
int	n;

   switch(bitdepth) {
	case 1:
	case 4:
		for(n=0; n<linesize; ++n) {
			if(!s_flag)
				printf("0x%02x,", linebuffer[n]);
			else {
				p[bc++] = linebuffer[n];
				if(bc == 4) {
					bc = 0;
					printf(".data4\t%ld\n", l);
					l = 0L;
				}
			}
		}
		break;
	case 8:
	default:
		for(n=0; n<linesize; ++n) {
			if(!s_flag)
				printf("%d,", linebuffer[n]);
			else {
				p[bc++] = linebuffer[n];
				if(bc == 4) {
					bc = 0;
					printf(".data4\t%ld\n", l);
					l = 0L;
				}
			}
		}
   }
   if(!s_flag)
	printf("\n");
}

/*
 * Decode one line of RLE8, return 0 when done with all bitmap data
 */
int DecodeRLE8(UCHAR *buf,FILE *fp)
{
int		c, n;
UCHAR *	p = buf;

   while(1) {
	switch(n = fgetc(fp)) {
		case EOF:
			return(0);
		case 0:					/* 0 = escape*/
			switch(n = fgetc(fp)) {
			case 0: 			/* 0 0 = end of current scan line*/
				return(1);
			case 1:				/* 0 1 = end of data*/
				return(1);
			case 2:				/* 0 2 xx yy delta mode - NOT SUPPORTED*/
				(void)fgetc(fp);
				(void)fgetc(fp);
				continue;
			default:			/* 0 3..255 xx nn uncompressed data*/
				for(c=0; c<n; c++)
					*p++ = fgetc(fp);
				if(n & 1)
					(void)fgetc(fp);
				continue;
			}
		default:
			c = fgetc(fp);
			while(n--)
				*p++ = c;
			continue;
		}
	}
}

/*
 * Decode one line of RLE4, return 0 when done with all bitmap data
 */
static UCHAR *p;
static int	once;

void put4(int b)
{
static int	last;

   last = (last << 4) | b;
   if(++once == 2) {
	*p++ = last;
	once = 0;
   }
}

int DecodeRLE4(UCHAR *buf, FILE *fp)
{
int		c, n, c1, c2;

   p = buf;
   once = 0;
   while(1) {
	switch(n = fgetc(fp)) {
		case EOF:
			return(0);
		case 0:					/* 0 = escape*/
		switch(n = fgetc(fp)) {
			case 0: 			/* 0 0 = end of current scan line*/
				if(once)
					put4(0);
				return(1);
			case 1:				/* 0 1 = end of data*/
				if(once)
					put4(0);
				return(1);
			case 2:				/* 0 2 xx yy delta mode - NOT SUPPORTED*/
				(void)fgetc(fp);
				(void)fgetc(fp);
				continue;
			default:			/* 0 3..255 xx nn uncompressed data*/
				c2 = (n+3) & ~3;
				for(c=0; c<c2; c++) {
					if((c & 1) == 0)
							c1 = fgetc(fp);
					if(c < n)
							put4((c1 >> 4) & 0x0f);
					c1 <<= 4;
				}
				continue;
			}
		default:
			c = fgetc(fp);
			c1 = (c >> 4) & 0x0f;
			c2 = c & 0x0f;
			for(c=0; c<n; c++)
				put4((c&1)? c2: c1);
			continue;
		}
	}
}
