/*
 * Copyright (c) 1999, 2000, 2003, 2019 Greg Haerr <greg@censoft.com>
 *
 * Chinese Hanzi Bitmap Font drawing using loadable HBF files for Microwindows
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uni_std.h"
#include "device.h"
#include "devfont.h"
#include "genfont.h"
#include "../drivers/hbf.h"

#define MAX_CHAR_SIZE	32	/* maximum character cell width and height*/

PMWFONT hbf_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);
static MWBOOL hbf_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void hbf_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
static void hbf_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
		MWCOORD *pbase);
static void hbf_destroyfont(PMWFONT pfont);

typedef struct MWHBFFONT {
	PMWFONTPROCS fontprocs;	/* common hdr */
	int fontsize;
	int	fontwidth;
	int fontrotation;
	int fontattr;
	int	height;
	int width;
	int baseline;
	HBF *hbf_font;
} MWHBFFONT, *PMWHBFFONT;

/* handling routines for MWHBFFONT*/
static MWFONTPROCS hbf_procs = {
	0,				/* can't scale*/
	MWTF_UC16,		/* routines expect unicode index*/
	NULL,			/* init*/
	hbf_createfont,
	hbf_getfontinfo,
	hbf_gettextsize,
	hbf_gettextbits,
	hbf_destroyfont,
	gen_drawtext,
	NULL,			/* setfontsize */
	NULL,			/* setfontrotation */
	NULL,			/* setfontattr */
	NULL			/* duplicate*/
};

/*
 * Load HBF font
 */
PMWFONT
hbf_createfont(const char *name, MWCOORD height, MWCOORD width, int attr)
{
	PMWHBFFONT pf;
	char *p;
	char fontname[256];
	char fontpath[256];

	/* if no extension specified, add .hbf, otherwise check for .hbf*/
	strcpy(fontname, name);
	if ((p = strrchr(fontname, '.')) == NULL)
		strcat(fontname, ".hbf");
	else {	
		if (strcasecmp(p+1, "hbf") != 0)
			return NULL;		/* non .hfb file specified*/
	}

	/* check path and filename for .hbf file*/
	strcpy(fontpath, fontname);
	if (access(fontpath, F_OK) != 0) {
		sprintf(fontpath, "%s/%s", HBF_FONT_DIR, fontname);
		if (access(fontpath, F_OK) != 0)
			return NULL;
	}

	/* allocate font structure */
	pf = (PMWHBFFONT)calloc(sizeof(MWHBFFONT), 1);
	if (!pf)
		return NULL;

    if ((pf->hbf_font = hbfOpen(fontpath)) == NULL) {
        free(pf);
		return NULL;
	}

	pf->fontprocs = &hbf_procs;
	pf->fontsize = height;
	pf->fontwidth = width;
	pf->fontrotation = 0;
	pf->fontattr = attr;

	pf->height = 16;			/* FIXME only works with 16x16 char cell*/
	pf->width = 16;
	pf->baseline = pf->height - 2;
	return (PMWFONT)pf;
}

/* Font size */
static MWBOOL
hbf_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWHBFFONT pf = (PMWHBFFONT)pfont;
	int i;

	pfontinfo->height = pf->height;
	pfontinfo->maxwidth = pf->width;
	pfontinfo->baseline = pf->baseline;

	/* FIXME: calculate these properly: */
	pfontinfo->linespacing = pfontinfo->height;
	pfontinfo->descent = pfontinfo->height - pfontinfo->baseline;
	pfontinfo->maxascent = pfontinfo->baseline;
	pfontinfo->maxdescent = pfontinfo->descent;

	pfontinfo->firstchar = 0;
	pfontinfo->lastchar = 0;
	pfontinfo->fixed = TRUE;

	for (i = 0; i < 256; i++)
		pfontinfo->widths[i] = pf->width;

	return TRUE;

}

/* Get the width and height of passed text string in the current font*/
static void
hbf_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
				MWCOORD * pwidth, MWCOORD * pheight, MWCOORD * pbase)
{
	PMWHBFFONT pf = (PMWHBFFONT)pfont;

	*pwidth = pf->width * cc;
	*pheight = pf->height;
	*pbase = pf->baseline;
}

/* return character bitmap from MGL font bitmap data */
static void
hbf_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
	MWCOORD * pwidth, MWCOORD * pheight, MWCOORD * pbase)
{
	PMWHBFFONT pf = (PMWHBFFONT)pfont;
	int i, CH, CL;
	unsigned char *bitmap;
	static MWIMAGEBITS map[MAX_CHAR_SIZE * MAX_CHAR_SIZE / MWIMAGE_BITSPERIMAGE];

    *retmap = map;

	CH = ((unsigned int)ch) >> 8;
	CL = ((unsigned int)ch) & 0xFF;
	if ((CH >= 0xA4 && CH <= 0xC5
		&& ((CL >= 0x40 && CL <= 0x7E) || (CL >= 0xA1 && CL <= 0xFE)))
		|| (CH == 0xC6 && (CL >= 0x40 && CL <= 0x7E)) )
	{
		*pwidth = hbfBitmapBBox(pf->hbf_font)->hbf_width;
		*pheight = hbfBitmapBBox(pf->hbf_font)->hbf_height;
		*pbase = pf->baseline;
		bitmap = (unsigned char *)hbfGetBitmap(pf->hbf_font, (HBF_CHAR)ch);
		if (bitmap == NULL)
			memset(map, 0xff, sizeof(map));
		else if (pf->width < 20) {
				for (i = 0; i < *pheight; i++) {
					unsigned char *DstBitmap  = ((unsigned char *)map) + i * 2;
					unsigned char *FontBitmap = bitmap+i*2;
					DstBitmap[0] = FontBitmap[1];
					DstBitmap[1] = FontBitmap[0];
				}
		} else {
				for (i = 0; i < *pheight; i++) {
					unsigned char *DstBitmap  = ((unsigned char *)map) + i * 4;
					unsigned char *FontBitmap = bitmap+i*3;
					DstBitmap[0] = FontBitmap[1];
					DstBitmap[1] = FontBitmap[0];
					DstBitmap[2] = 0;
					DstBitmap[3] = FontBitmap[2];
				}
		}
		return;
	}

	*pwidth = 0;
	*pheight = 0;
	*pbase = 0;
}

/* Unload font */
static void
hbf_destroyfont(PMWFONT pfont)
{

	PMWHBFFONT pf = (PMWHBFFONT)pfont;

	hbfClose(pf->hbf_font);
	free(pf);
}
