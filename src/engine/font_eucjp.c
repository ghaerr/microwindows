/*
 * Copyright (c) Koichi Mori
 * Copyright (c) 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * EUC Japanese text drawing using MGL fonts for Microwindows
 * Supports dynamically loading MGL font files
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#ifndef MAP_FAILED
#define MAP_FAILED  ((void *)-1)
#endif
#include "device.h"
#include "devfont.h"
#include "genfont.h"

#ifndef EUCJP_FONT_DIR
#define EUCJP_FONT_DIR	"fonts/japanese"	/* default MGLFONT .fnt font file directory*/
#endif

typedef struct MWEUCJPFONT {
	0,						/* can't scale*/
	PMWFONTPROCS fontprocs;	/* common hdr */
	int fontsize;
	int	fontwidth;
	int fontrotation;
	int fontattr;

	int width;				/* MGL font data */
	int height;
	int koffset;
	int kwidth;
	int kbytes;
	int aoffset;
	int awidth;
	int abytes;
	int fd;					/* file descriptor of font bitmap data */
	char *font_base;
} MWEUCJPFONT, *PMWEUCJPFONT;

PMWFONT eucjp_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);
static MWBOOL eucjp_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void eucjp_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
static void eucjp_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
		MWCOORD *pbase);
static void eucjp_destroyfont(PMWFONT pfont);

/* handling routines for MWEUCJPFONT*/
static MWFONTPROCS eucjp_procs = {
	MWTF_UC16,		/* routines expect unicode index*/
	NULL,			/* init*/
	eucjp_createfont,
	eucjp_getfontinfo,
	eucjp_gettextsize,
	eucjp_gettextbits,
	eucjp_destroyfont,
	corefont_drawtext,
	NULL,			/* setfontsize */
	NULL,			/* setfontrotation */
	NULL,			/* setfontattr */
	NULL			/* duplicate*/
};

/*
 * Load MGL binary font
 *
 * Many thanks to MGL fontkit/mgl_fontinfo.c
 */
PMWFONT
eucjp_createfont(const char *name, MWCOORD height, MWCOORD width, int attr)
{
	PMWEUCJPFONT pf;
	int fd, r;
	int fsize;
	int ver, mfw, mfh;
	char buf[256];
	char fname[256];

	/* allocate font structure */
	pf = (PMWEUCJPFONT) calloc(sizeof(MWEUCJPFONT), 1);
	if (!pf)
		return NULL;

	fd = open(name, O_RDONLY);
	if (fd < 0) {
		sprintf(fname, "%s/%s", EUCJP_FONT_DIR, name);
		fd = open(fname, O_RDONLY);
	}
	if (fd < 0)
		goto EUCJP_FAIL;

	r = read(fd, buf, 256);
	if (r != 256) {
		printf("FONT_EUCJP: can't read %s\n", name);
		goto EUCJP_FAIL;
	}

	fsize = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);

	if (sscanf(buf, "#MGLFONT%02d%02d%02d", &ver, &mfw, &mfh) == 3) {
	} else {
		printf("FONT_EUCJP: magic is missing.\n");
		goto EUCJP_FAIL;
	}

	pf->fontprocs = &eucjp_procs;
	pf->fontsize = height;
	pf->fontrotation = 0;
	pf->fontattr = attr;

	pf->height = mfh;
	pf->width = mfw;
	pf->aoffset = 32;
	pf->awidth = (mfw / 2 + 7) / 8;
	pf->abytes = pf->awidth * mfh;
	pf->koffset = pf->aoffset + pf->abytes * 256;
	pf->kwidth = (mfw + 7) / 8;
	pf->kbytes = pf->kwidth * mfh;	/* height; */
	pf->fd = fd;
	if (fsize != pf->koffset + pf->kbytes * 8064) {
		printf("FONT_EUCJP: Not MGL font file(filesize doesn't match).\n");
		goto EUCJP_FAIL;
	}
	pf->font_base = (char *)mmap((caddr_t) 0, pf->koffset + pf->kbytes * 8064,
			      PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	if (pf->font_base == MAP_FAILED) {
		printf("FONT_EUCJP: Can't mmap font data.\n");
		goto EUCJP_FAIL;
	}

	return (PMWFONT)pf;

EUCJP_FAIL:
	free(pf);
	close(fd);
	return NULL;
}

/* Font size */
static MWBOOL
eucjp_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWEUCJPFONT pf = (PMWEUCJPFONT) pfont;
	int i;

	pfontinfo->height = pf->height;
	pfontinfo->maxwidth = pf->width;
	pfontinfo->baseline = pf->height - 2;

	/* FIXME: calculate these properly: */
	pfontinfo->linespacing = pfontinfo->height;
	pfontinfo->descent = pfontinfo->height - pfontinfo->baseline;
	pfontinfo->maxascent = pfontinfo->baseline;
	pfontinfo->maxdescent = pfontinfo->descent;

	pfontinfo->firstchar = 0;
	pfontinfo->lastchar = 0;
	pfontinfo->fixed = TRUE;

	for (i = 0; i < 256; i++)
		pfontinfo->widths[i] = pf->width / 2;

	return TRUE;

}

/* Get the width and height of passed text string in the current font*/
static void
eucjp_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
	MWCOORD * pwidth, MWCOORD * pheight, MWCOORD * pbase)
{
	PMWEUCJPFONT pf = (PMWEUCJPFONT) pfont;

	*pwidth = strlen(text) * pf->width / 2;
	*pheight = pf->height;
	*pbase = pf->height - 2;
}

/* return character bitmap from MGL font bitmap data */
static void
eucjp_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
	MWCOORD * pwidth, MWCOORD * pheight, MWCOORD * pbase)
{
	PMWEUCJPFONT pf = (PMWEUCJPFONT) pfont;
	char *fptn;
	int i, bytewidth;
	static MWIMAGEBITS map[MAX_CHAR_HEIGHT * MAX_CHAR_WIDTH / MWIMAGE_BITSPERIMAGE];
	MWIMAGEBITS *bitmap = map;

	*retmap = map;
	*pheight = pf->height;
	*pbase = pf->height - 2;
	if (ch < 256) {
		*pwidth = pf->width / 2;
		fptn = &(pf->font_base[pf->aoffset + pf->abytes * ch]);
		bytewidth = pf->awidth;
	} else {
		int kix = (((ch >> 8) & 0xff) - 0xA1) * 96 + ((ch & 0xff) - 0xA0);
		if (kix < 0 || 8000 <= kix)
			kix = 0;

		*pwidth = pf->width;
		fptn = &(pf->font_base[pf->koffset + pf->kbytes * kix]);
		bytewidth = pf->kwidth;
	}

	for (i = 0; i < pf->height; i++) {
		MWIMAGEBITS ptn0, ptn1, ptn2, ptn3;
		switch (bytewidth) {
		case 1:
			ptn0 = *(fptn) & 0xFF;
			*bitmap++ = *(fptn) << 8;
			break;
		case 2:
			ptn0 = *(fptn) & 0xFF;
			ptn1 = *(fptn + 1) & 0xFF;
			*bitmap++ = ptn0 << 8 | ptn1;
			break;
		case 3:
			ptn0 = *(fptn) & 0xFF;
			ptn1 = *(fptn + 1) & 0xFF;
			ptn2 = *(fptn + 2) & 0xFF;
			*bitmap++ = ptn0 << 8 | ptn1;
			*bitmap++ = ptn2 << 8;
			break;
		case 4:
			ptn0 = *(fptn) & 0xFF;
			ptn1 = *(fptn + 1) & 0xFF;
			ptn2 = *(fptn + 2) & 0xFF;
			ptn3 = *(fptn + 3) & 0xFF;
			*bitmap++ = ptn0 << 8 | ptn1;
			*bitmap++ = ptn2 << 8 | ptn3;
			break;
		default:
			/*
			   Fonts that have  more than 32 pixel width cannot be displayed.
			   Please fix in case you want to display such a *huge* font.
			 */
			break;
		}
		fptn += bytewidth;
	}
}

/* Unload font */
static void
eucjp_destroyfont(PMWFONT pfont)
{
	PMWEUCJPFONT pf = (PMWEUCJPFONT) pfont;

	munmap(pf->font_base, pf->koffset + pf->kbytes * 8064);
	close(pf->fd);
}
