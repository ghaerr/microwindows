/* 
 * Loadable FNT font engine for Microwindows
 * Copyright (c) 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * Load a .fnt/.fnt.gz (Microwindows native) binary font, store in incore format.
 */
#include <stdio.h>
#include <string.h>
#include "device.h"
#include "devfont.h"
#include "genfont.h"

/* configurable defaults*/
#ifndef FNT_FONT_DIR
#define FNT_FONT_DIR	"fonts/fnt"		/* default .fnt file location*/
#endif

/*
 * .fnt loadable font file format definition
 *
 * format                     len	description
 * -------------------------  ----	------------------------------
 * UCHAR version[4]				4	magic number and version bytes
 * UCHAR name[64]	       		64	font name, space padded
 * UCHAR copyright[256]	      256	copyright info, space padded
 * USHORT maxwidth				2	font max width in pixels
 * USHORT height				2	font height in pixels
 * USHORT ascent				2	font ascent (baseline) in pixels
 * USHORT pad                   2       unused, pad to 32-bit boundary
 * ULONG firstchar				4	first character code in font
 * ULONG defaultchar			4	default character code in font
 * ULONG size					4	# characters in font
 * ULONG nbits					4	# words imagebits data in file
 * ULONG noffset				4	# longs offset data in file
 * ULONG nwidth					4	# bytes width data in file
 * MWIMAGEBITS bits	  			nbits*2	image bits variable data
 * [MWIMAGEBITS padded to 32-bit boundary]
 * ULONG offset         		noffset*4	offset variable data
 * UCHAR width		 			nwidth*1	width variable data
 */

/* loadable font magic and version #*/
#define VERSION		"RB11"

/* The user hase the option including ZLIB and being able to    */
/* directly read compressed .fnt files, or to omit it and save  */
/* space.  The following defines make life much easier          */
#if HAVE_FNTGZ_SUPPORT
#include <zlib.h>
#define FILEP gzFile
#define FOPEN(path, mode)           gzopen(path, mode)
#define FREAD(file, buffer, size)   gzread(file, buffer, size)
#define FSEEK(file, offset, whence) gzseek(file, offset, whence)
#define FCLOSE(file)                gzclose(file)
#else
#define FILEP  FILE *
#define FOPEN(path, mode)           fopen(path, mode)
#define FREAD(file, buffer, size)   fread(buffer, 1, size, file)
#define FSEEK(file, offset, whence) fseek(file, offset, whence)
#define FCLOSE(file)                fclose(file)
#endif

/* Handling routines for FNT fonts, use MWCOREFONT structure */
PMWFONT fnt_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);
static void fnt_unloadfont(PMWFONT font);
static PMWCFONT fnt_load_font(const char *path);

/* these procs used when font ASCII indexed*/
MWFONTPROCS fnt_fontprocs = {
	0,				/* can't scale*/
	MWTF_ASCII,		/* routines expect ascii */
	NULL,			/* init*/
	fnt_createfont,
	gen_getfontinfo,
	gen_gettextsize,
	gen_gettextbits,
	fnt_unloadfont,
#if STANDALONE
	gen16_drawtext, //FIXME
#else
	corefont_drawtext,
#endif
	NULL,			/* setfontsize */
	NULL,			/* setfontrotation */
	NULL,			/* setfontattr */
	NULL			/* duplicate not supported */
};

/* these procs used when font requires UC16 index*/
static MWFONTPROCS fnt_fontprocs16 = {
	0,				/* can't scale*/
	MWTF_UC16,		/* large font, expect UC16*/
	NULL,			/* init*/
	fnt_createfont,
	gen_getfontinfo,
	gen16_gettextsize,
	gen_gettextbits,
	fnt_unloadfont,
	gen16_drawtext,
	NULL,			/* setfontsize */
	NULL,			/* setfontrotation */
	NULL,			/* setfontattr */
	NULL			/* duplicate not supported */
};

/* load font and allocate MWCOREFONT structure*/
PMWFONT
fnt_createfont(const char *name, MWCOORD height, MWCOORD width, int attr)
{
	PMWCOREFONT	pf;
	PMWCFONT	cfont;
	int		uc16;

	/* try to open file and read in font data*/
	cfont = fnt_load_font(name);
	if (!cfont)
		return NULL;

	if (!(pf = (MWCOREFONT *) malloc(sizeof(MWCOREFONT)))) {
		free(cfont);
		return NULL;
	}

	/* determine if unicode-16 indexing required*/
	uc16 = cfont->firstchar > 255 || (cfont->firstchar + cfont->size) > 255;
	pf->fontprocs = uc16? &fnt_fontprocs16: &fnt_fontprocs;

	pf->fontsize = pf->fontrotation = pf->fontattr = 0;
	pf->name = "FNT";
	pf->cfont = cfont;
	return (PMWFONT)pf;
}

void
fnt_unloadfont(PMWFONT font)
{
	PMWCOREFONT pf = (PMWCOREFONT)font;
	PMWCFONT    pfc = pf->cfont;

	if (pfc) {
		if (pfc->width)
			free((char *)pf->cfont->width);
		if (pfc->offset)
			free((char *)pf->cfont->offset);
		if (pfc->bits)
			free((char *)pf->cfont->bits);
		if (pfc->name)
			free(pf->cfont->name);

		free(pfc);
	}

	free(font);
}

static int
READBYTE(FILEP fp, unsigned char *cp)
{
#if HAVE_FNTGZ_SUPPORT
	unsigned char buf[1];

	if (FREAD(fp, buf, 1) != 1)
		return 0;
	*cp = buf[0];
#else
	int c;

	if ((c = getc(fp)) == EOF)
		return 0;
	*cp = (unsigned char)c;
#endif
	return 1;
}

static int
READSHORT(FILEP fp, unsigned short *sp)
{
#if HAVE_FNTGZ_SUPPORT
	unsigned char buf[2];

	if (FREAD(fp, buf, 2) != 2)
		return 0;
	*sp = buf[0] | (buf[1] << 8);
#else
	int c;
	unsigned short s;

	if ((c = getc(fp)) == EOF)
		return 0;
	s = c & 0xff;
	if ((c = getc(fp)) == EOF)
		return 0;
	*sp = (c << 8) | s;
#endif
	return 1;
}

static int
READLONG(FILEP fp, uint32_t *lp)
{
#if HAVE_FNTGZ_SUPPORT
	unsigned char buf[4];

	if (FREAD(fp, buf, 4) != 4)
		return 0;
	*lp = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
#else
	int c;
	uint32_t l;

	if ((c = getc(fp)) == EOF)
		return 0;
	l = c & 0xff;
	if ((c = getc(fp)) == EOF)
		return 0;
	l |= (c << 8);
	if ((c = getc(fp)) == EOF)
		return 0;
	l |= (c << 16);
	if ((c = getc(fp)) == EOF)
		return 0;
	*lp = (c << 24) | l;
#endif
	return 1;
}

/* read count bytes*/
static int
READSTR(FILEP fp, char *buf, int count)
{
	return FREAD(fp, buf, count);
}

/* read totlen bytes, return NUL terminated string*/
/* may write 1 past buf[totlen]; removes blank pad*/
static int
READSTRPAD(FILEP fp, char *buf, int totlen)
{
	char *p;

	if (FREAD(fp, buf, totlen) != totlen)
		return 0;
	p = &buf[totlen];
	*p-- = 0;
	while (*p == ' ' && p >= buf)
		*p-- = '\0';
	return totlen;
}

/* read and load font, return incore font structure*/
static PMWCFONT
fnt_load_font(const char *path)
{
	FILEP ifp;
	PMWCFONT pf = NULL;
	int i;
	unsigned short maxwidth, height, ascent, pad;
	uint32_t firstchar, defaultchar, size;
	uint32_t nbits, noffset, nwidth;
	char version[4+1];
	char name[64+1];
	char copyright[256+1];
	char fname[256];

	ifp = FOPEN(path, "rb");
	if (!ifp) {
		sprintf(fname, "%s/%s", FNT_FONT_DIR, path);
		ifp = FOPEN(fname, "rb");
		
		/* Try to grab it from the MWFONTDIR directory */
		if (!ifp) {
			char *env = getenv("MWFONTDIR");
			if (env) {
				sprintf(fname, "%s/%s", env, path);
				
				DPRINTF("Trying to get font from %s\n", fname);
				ifp = FOPEN(fname, "rb");
			}
		}
		
	}
	if (!ifp)
		return NULL;

	/* read magic and version #*/
	if (READSTR(ifp, version, 4) != 4)
		goto errout;
	if (strcmp(version, VERSION) != 0)
		goto errout;

	pf = (PMWCFONT)calloc(1, sizeof(MWCFONT));
	if (!pf)
		goto errout;

	/* internal font name*/
	if (READSTRPAD(ifp, name, 64) != 64)
		goto errout;
	pf->name = (char *)malloc(strlen(name)+1);
	if (!pf->name)
		goto errout;
	strcpy(pf->name, name);
	/* copyright, not currently stored*/
	if (READSTRPAD(ifp, copyright, 256) != 256)
		goto errout;

	/* font info*/
	if (!READSHORT(ifp, &maxwidth))
		goto errout;
	pf->maxwidth = maxwidth;
	if (!READSHORT(ifp, &height))
		goto errout;
	pf->height = height;
	if (!READSHORT(ifp, &ascent))
		goto errout;
	pf->ascent = ascent;
	if (!READSHORT(ifp, &pad))
		goto errout;
	if (!READLONG(ifp, &firstchar))
		goto errout;
	pf->firstchar = firstchar;
	if (!READLONG(ifp, &defaultchar))
		goto errout;
	pf->defaultchar = defaultchar;
	if (!READLONG(ifp, &size))
		goto errout;
	pf->size = size;

	/* variable font data sizes*/
	/* # words of MWIMAGEBITS*/
	if (!READLONG(ifp, &nbits))
		goto errout;
	pf->bits = (MWIMAGEBITS *)malloc(nbits * sizeof(MWIMAGEBITS));
	if (!pf->bits)
		goto errout;
	pf->bits_size = nbits;

	/* # longs of offset*/
	if (!READLONG(ifp, &noffset))
		goto errout;
	if (noffset) {
		pf->offset = (uint32_t *)malloc(noffset * sizeof(uint32_t));
		if (!pf->offset)
			goto errout;
	}

	/* # bytes of width*/
	if (!READLONG(ifp, &nwidth))
		goto errout;
	if (nwidth) {
		pf->width = (unsigned char *)malloc(nwidth * sizeof(unsigned char));
		if (!pf->width)
			goto errout;
	}

	/* variable font data*/
	for (i=0; i<nbits; ++i)
		if (!READSHORT(ifp, (unsigned short *)&pf->bits[i]))
			goto errout;

	/* pad to longword boundary*/
	if (nbits & 01)
		if (!READSHORT(ifp, &pad))
			goto errout;

	if (noffset)
		for (i=0; i<pf->size; ++i)
			if (!READLONG(ifp, (uint32_t *)&pf->offset[i]))
				goto errout;
	if (nwidth)
		for (i=0; i<pf->size; ++i)
			if (!READBYTE(ifp, (unsigned char *)&pf->width[i]))
				goto errout;
	
	FCLOSE(ifp);
	return pf;	/* success!*/

errout:
	FCLOSE(ifp);
	if (!pf)
		return NULL;
	if (pf->name)
		free(pf->name);
	if (pf->bits)
		free((char *)pf->bits);
	if (pf->offset)
		free((char *)pf->offset);
	if (pf->width)
		free((char *)pf->width);
	free(pf);
	return NULL;
}
