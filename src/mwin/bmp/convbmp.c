/*
 * Copyright (c) 1999, 2000, 2003 Greg Haerr <greg@censoft.com>
 *
 * Windows BMP to Microwindows image converter
 *
 * 9/24/2003 endian-neutral conversion
 * 05/01/2000 Michael Temari <Michael@TemWare.Com>
 * Modified to output .s ACK format for Minix
 * 6/9/1999 g haerr
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../include/device.h"

/* separators for DOS & Unix file systems */
#define OS_FILE_SEPARATOR   "/\\"

#define PIX2BYTES(n)	(((n)+7)/8)

#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L

typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

/* The Minix ACK compiler cannot pack a structure so we do it the hard way.
 *
 * We also do it this way for other platforms, since this is endian-neutral.
 * It avoids us having to detect the platform's endianness, which is
 * messy and platform-dependent.
 * 
 * This code is only used while compiling, so it is not speed- or
 * size-critical.
 */
typedef unsigned char	WORD[2];
typedef unsigned char	DWORD[4];
typedef unsigned char	LONG[4];
#define READWORD(x)  (  ((unsigned short)(x)[0])       | \
                       (((unsigned short)(x)[1]) << 8) )
#define READDWORD(x) (  ((unsigned long)(x)[0])        | \
                       (((unsigned long)(x)[1]) <<  8) | \
                       (((unsigned long)(x)[2]) << 16) | \
                       (((unsigned long)(x)[3]) << 24) )
#define READLONG(x)	((long)READDWORD(x))

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

char * StripPath(char *buffer);
int	ConvBMP(FILE *fp,char *name);
int ConvBMPFile(char *infilename, char *outfilename);
void	outline(UCHAR *linebuffer, int bitdepth, int linesize, int y);
int	DecodeRLE8(UCHAR *buf,FILE *fp);
int	DecodeRLE4(UCHAR *buf,FILE *fp);
void	put4(int b);

int	s_flag = 0;

#define MAX_FILENAME_LEN 256

int main(int argc,char *argv[])
{
int	o_flag = 0;
char 	*p;
char	name[MAX_FILENAME_LEN];
char	oname[MAX_FILENAME_LEN];

   /* skip the program name */
   argc--;
   argv++;

   /* check for -s flag */
   if(argc && strcmp(*argv, "-s") == 0) {
	s_flag = -1;
	argc--;
	argv++;
   }

   /* check for -o flag */
   if(argc && strcmp(*argv, "-o") == 0) {
	o_flag = -1;
	argc--;
	argv++;
   }

   /* need at least one file to convert */
   if((argc == 0) || (o_flag && (argc != 2))) {
	fprintf(stderr, "Usage: convbmp [-s] <bmpfile>...\n"
	                "   OR: convbmp [-s] -o <outfile> <bmpfile>\n");
	return(-1);
   }

   if (o_flag) {
   	if (strlen(argv[0]) > MAX_FILENAME_LEN - 3) {
   		fprintf(stderr, "Filename too long (%d bytes, max is %d): '%s'\n", strlen(*argv), MAX_FILENAME_LEN - 3, *argv);
   		return 1;
   	}
   	if (strlen(argv[1]) > MAX_FILENAME_LEN - 3) {
   		fprintf(stderr, "Filename too long (%d bytes, max is %d): '%s'\n", strlen(*argv), MAX_FILENAME_LEN - 3, *argv);
   		return 1;
   	}
   	if(ConvBMPFile(argv[1], argv[0])) {
   		return 1;
   	}
      return(0);
   }
   
   /* go back one since the first thing in the loop is to increment */
   argv--;
   while(argc--) {
	argv++;
	if (strlen(*argv) > MAX_FILENAME_LEN - 3) {
	   /* The 3 is for ".c\0" */
		fprintf(stderr, "Filename too long (%d bytes, max is %d): '%s'\n", strlen(*argv), MAX_FILENAME_LEN - 3, *argv);
		return 1;
	}
	strcpy(name, *argv);
	strcpy(oname, *argv);
	if((p = strrchr(oname, '.')) != (char *)NULL)
		*p = '\0';
	if(s_flag)
		strcat(oname, ".s");
	else
		strcat(oname, ".c");
	if(ConvBMPFile(name, oname)) {
		return 1;
	}
   }
   return(0);
}

int ConvBMPFile(char *infilename, char *outfilename)
{
char	namebuf[MAX_FILENAME_LEN];
char *p;
char *name;
FILE	*fp;

   strcpy(namebuf, infilename);
	if((p = strrchr(namebuf, '.')) != (char *)NULL)
		*p = '\0';
   /* 
      let's strip any directory from the path passed in.
      This prevent problems if this utility is given 
      path is not for the curent directory.
   */
   name = StripPath(namebuf);
	
	if((fp = fopen(infilename, "rb")) == (FILE *)NULL) {
		fprintf(stderr, "Can't open file: %s\n", infilename);
		return 1;
	}
	if(freopen(outfilename, "w", stdout) == (FILE *)NULL) {
		fclose(fp);
		fprintf(stderr, "Could not open output file %s\n", outfilename);
		return 1;
	}

	if(ConvBMP(fp, name)) {
		fprintf(stderr, "Conversion failed: %s\n", infilename);
		fclose(fp);
		return 1;
	}
	fclose(fp);
	
	return 0;
}

/* 
    let's strip any directory from the path passed in.
    This prevent problems if this utility is given 
    path is not for the curent directory.
    
    This function changes buffer.
    It returns a pointer to the result, which will be in
    buffer (but not always at the beginning of buffer).
*/
char * StripPath(char *buffer)
{
   char *p = buffer;
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
   return(p);
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
   if(READDWORD(bmp.BiSize) == 12) {
	bmc = (BMPCOREHEAD *)&bmp;
	cx = (int)READWORD(bmc->bcWidth);
	cy = (int)READWORD(bmc->bcHeight);
	bitdepth = READWORD(bmc->bcBitCount);
	palsize = 1 << bitdepth;
	compression = BI_RGB;
	fseek(fp, sizeof(BMPCOREHEAD), SEEK_SET);
   } else {
	cx = (int)READLONG(bmp.BiWidth);
	cy = (int)READLONG(bmp.BiHeight);
	bitdepth = READWORD(bmp.BiBitCount);
	palsize = (int)READDWORD(bmp.BiClrUsed);
	if(palsize == 0)
		palsize = 1 << bitdepth;
	compression = READDWORD(bmp.BiCompression);
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
		if(READDWORD(bmp.BiSize) != 12)
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
   fseek(fp, READDWORD(bmp.bfOffBits), SEEK_SET);
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
