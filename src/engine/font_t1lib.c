/*
 * Copyright (c) 2000, 2002, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * T1lib Adobe type1 routines originally contributed by Vidar Hokstad
 * Rewritten heavily by g haerr
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <t1lib.h>
#include "device.h"
#include "devfont.h"

#if (UNIX | DOS_DJGPP)
#define strcmpi strcasecmp
#endif

/* settable parameters*/
#ifndef T1LIB_FONT_DIR
#define T1LIB_FONT_DIR			"fonts/type1"
#endif
#define T1LIB_CONFIG_FILE		"t1lib.config"
#define T1LIB_DEFAULT_ENCODING	"IsoLatin1.enc"
#define T1LIB_USE_AA_HIGH		1				/* 17 vs 5 level alpha*/

#ifndef T1LIB_VERSION
#define T1LIB_VERSION 0
#endif
#if T1LIB_VERSION < 5
#define T1_GetNoFonts	T1_Get_no_fonts	/* name change after 1.0 (tested in 5.1.2)*/
#endif

typedef struct {
	PMWFONTPROCS fontprocs;	/* common hdr*/
	MWCOORD		fontsize;	/* font height in pixels*/
	MWCOORD		fontwidth;	/* font width in pixels*/
	int			fontrotation;
	int			fontattr;		
	/* t1lib specific stuff*/
	int			fontid;
} MWT1LIBFONT, *PMWT1LIBFONT;

static int  t1lib_init(PSD psd);
PMWFONT t1lib_createfont(const char *name, MWCOORD height,MWCOORD width, int attr);

static void t1lib_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, MWTEXTFLAGS flags);
static MWBOOL t1lib_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void t1lib_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
static int t1lib_setfontsize(PMWFONT pfont, MWCOORD height, MWCOORD width);
static int t1lib_setfontattr(PMWFONT pfont, int setflags, int clrflags);
static void t1lib_destroyfont(PMWFONT pfont);

/* handling routines for MWT1LIBFONT*/
MWFONTPROCS t1lib_fontprocs = {
	MWTF_SCALEHEIGHT,	/* can scale height only*/
	MWTF_ASCII,			/* routines expect ascii*/
	t1lib_init,
	t1lib_createfont,
	t1lib_getfontinfo,
	t1lib_gettextsize,
	NULL,				/* gettextbits*/
	t1lib_destroyfont,
	t1lib_drawtext,
	t1lib_setfontsize,
	NULL,				/* setfontrotation*/
	t1lib_setfontattr,	/* setfontattr*/
	NULL,				/* duplicate*/
};

/* temp extern decls*/
extern MWPIXELVAL gr_foreground;
extern MWPIXELVAL gr_background;
extern MWBOOL gr_usebg;

static int
t1lib_init(PSD psd)
{
	char **encoding;
	char path[256];
	static int inited = 0;
#if T1LIB_USE_AA_HIGH      
	static unsigned long highblend[17] = {
	   	0x00, 0x00, 0x04, 0x0c, 0x10, 0x14, 0x18, 0x20,
	   	0x30, 0x38, 0x40, 0x50, 0x70, 0x80, 0xa0, 0xc0, 0xff };
#else
	static unsigned long lowblend[5] = { 0x00, 0x44, 0x88, 0xcc, 0xff };
#endif

	if (inited)
		return 1;

	/* set default config file directory if not otherwise specified*/
	if (!getenv("T1LIB_CONFIG")) {
		sprintf(path, "%s/%s", T1LIB_FONT_DIR, T1LIB_CONFIG_FILE);
		setenv("T1LIB_CONFIG", path, 1);
	}
	DPRINTF("setenv '%s'\n", getenv("T1LIB_CONFIG"));

	/* non-antialias mono bitmaps are byte arrays w/no padding*/
	T1_SetBitmapPad(8);

	if (!T1_InitLib(NO_LOGFILE|IGNORE_FONTDATABASE)) {
		DPRINTF("t1lib_init: library init failed error %d\n", T1_errno);
		return 0;
	}

	/* set default Latin1 encoding*/
	encoding = T1_LoadEncoding(T1LIB_DEFAULT_ENCODING);
	if (encoding)
		T1_SetDefaultEncoding(encoding);

	T1_AASetBitsPerPixel(8);		/* rasterize to 8bpp alpha values*/
#if T1LIB_USE_AA_HIGH      
	T1_AASetLevel(T1_AA_HIGH);
	T1_AAHSetGrayValues(highblend);
#else      
	T1_AASetLevel(T1_AA_LOW);
	T1_AASetGrayValues(lowblend[0],lowblend[1],lowblend[2],lowblend[3],lowblend[4]);
#endif

	inited = 1;
	return 1;
}

/* open font and allocate PMWT1LIBONT structure*/
PMWFONT
t1lib_createfont(const char *name, MWCOORD height, MWCOORD width, int attr)
{
	int			id;
	char 		*p, *fontname;
	PMWT1LIBFONT pf;
	int			ret, ret2;
	char		fontpath[256];
	char		t1name[256];

	/* ensure library is inited*/
	t1lib_init(NULL);

	/* if no extension specified, add .pfb, otherwise check for .pfb*/
	strcpy(fontpath, name);
	if ((p = strrchr(fontpath, '.')) == NULL)
		strcat(fontpath, ".pfb");
	else {	
		if (strcmpi(p+1, "pfb") != 0)
			return NULL;		/* non .pfb file specified, not type1*/
	}

	/* check path and filename for .pfb file*/
	if (access(fontpath, F_OK) != 0)
		return NULL;

	/* seperate font path and filename.pfb*/
	fontname = fontpath;
	if ((p = strrchr(fontname, '/')) != NULL) {
		*p++ = '\0';
		fontname = p;
	}

	/* check if font filename.pfb is already known*/
	for(id=0; id<T1_GetNoFonts(); ++id) {
		strcpy(t1name, T1_GetFontFileName(id));		/* returns .pfb name*/

		if(!strcmpi(fontname, t1name))
			goto found;
	}

	/* font filename.pfb exists but not found, add pathname and file to database*/
	ret = T1_AddToFileSearchPath(T1_PFAB_PATH|T1_AFM_PATH, T1_APPEND_PATH, fontpath);
	ret2 = T1_AddFont(fontname);
	DPRINTF("path %s, filename %s, ret %d, %d\n", fontpath, fontname, ret, ret2);
	DPRINTF("# fonts %d\n", T1_GetNoFonts());
		
	/* match name against t1lib font id's from t1lib FontDataBase*/
	for(id=0; id<T1_GetNoFonts(); ++id) {
		strcpy(t1name, T1_GetFontFileName(id));

		if(!strcmpi(fontname, t1name)) {
found:
			pf = (PMWT1LIBFONT)calloc(sizeof(MWT1LIBFONT), 1);
			if (!pf)
				return NULL;
			pf->fontprocs = &t1lib_fontprocs;
			pf->fontprocs->SetFontSize((PMWFONT)pf, height, width);
			pf->fontprocs->SetFontAttr((PMWFONT)pf, attr, 0);
			//pf->fontprocs->SetFontRotation((PMWFONT)pf, 0);
			pf->fontrotation = 0;
			//T1_ExtendFont(pf->fontid, (float)width_scale);
			pf->fontid = id;
			DPRINTF("t1lib_createfont: using %s/%s\n", fontpath, fontname);
			return (PMWFONT)pf;
		}
	}
	return NULL;
}

/*
 * Draw ascii text string using T1LIB type font
 */
static void
t1lib_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
	const void *text, int cc, MWTEXTFLAGS flags)
{
	PMWT1LIBFONT pf = (PMWT1LIBFONT)pfont;
	GLYPH 		*glyph; 		/* glyph structure, memory handling by T1lib */
	MWBLITPARMS parms;

	if (pf->fontattr & MWTF_ANTIALIAS) {
		parms.data_format = MWIF_ALPHABYTE;		/* data is 8bpp alpha channel*/
		parms.op = MWROP_BLENDFGBG;				/* blend fg/bg with alpha channel -> dst*/

		glyph = T1_AASetString(pf->fontid, (char *)text, cc, 0,
			(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0, (float)pf->fontsize, 0);
	} else {
		/* Do non-antialiased drawing */
		parms.data_format = MWIF_MONOBYTELSB;	/* data is 1bpp bytes, lsb first*/
		parms.op = MWROP_COPY;					/* copy to dst, 1=fg (0=bg if usebg)*/

		glyph = T1_SetString(pf->fontid, (char *)text, cc, 0,
				(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0, (float)pf->fontsize, 0);
	}

	if (glyph && glyph->bits) {
		int width = glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
		int height = glyph->metrics.ascent - glyph->metrics.descent;

		if(flags & MWTF_BASELINE)
			y -= glyph->metrics.ascent;
		else if(flags & MWTF_BOTTOM)
			y -= height - 1;

		parms.fg_color = gr_foreground;
		parms.bg_color = gr_background;
		parms.usebg = gr_usebg;
		parms.srcx = 0;
		parms.srcy = 0;
		parms.dst_pitch = 0;		/* set later in GdConversionBlit*/
		parms.data_out = 0;			/* set later in GdConversionBlit*/
		parms.dstx = x;
		parms.dsty = y;
		parms.height = height;
		parms.width = width;
		parms.data = (char *)glyph->bits;
		if (pf->fontattr & MWTF_ANTIALIAS)
			parms.src_pitch = width;
		else
			parms.src_pitch = (width + 7) >> 3;	/* pad to BYTE boundary*/
		GdConversionBlit(psd, &parms);

		if (pf->fontattr & MWTF_UNDERLINE) {
			int underliney = y + glyph->metrics.ascent;
			GdLine(psd, x, underliney, x+width, underliney, FALSE);
		}

		/* cleanup*/
		free(glyph->bits);
		glyph->bits = NULL;
	}

	GdFixCursor(psd);
}

/* Get the width and height of passed text string in the passed font*/
static void
t1lib_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWT1LIBFONT	pf = (PMWT1LIBFONT)pfont;
	GLYPH *			glyph;

	glyph = T1_SetString(pf->fontid, (char *)text, cc, 0,
			(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0, (float)pf->fontsize, 0);
	if (!glyph) {
		*pwidth = *pheight = *pbase = 20;
		return;
	}
	*pwidth = glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
	*pheight = glyph->metrics.ascent - glyph->metrics.descent;
	*pbase = glyph->metrics.ascent;

	if(glyph && glyph->bits) {
		free(glyph->bits);
		glyph->bits = NULL;
	}
#if 0
	BBox 			b;

	/* FIXME must change from char points (1000bp) to pixels*/
	b = T1_GetStringBBox(pf->fontid, text, cc, 0, (pf->fontattr&MWTF_KERNING)?T1_KERNING:0);

	printf("b.urx = %d, b.llx = %d\n",b.urx, b.llx);
	printf("b.ury = %d, b.lly = %d\n",b.ury, b.lly);

	/* NXLIB, X11*/
	*pwidth = (b.urx - b.llx);
	*pheight = (b.lly - b.ury);
#endif
}

static int
t1lib_setfontsize(PMWFONT pfont, MWCOORD height, MWCOORD width)
{
	PMWT1LIBFONT pf = (PMWT1LIBFONT)pfont;
	MWCOORD oldsize = pf->fontsize;

	pf->fontsize = height;
	pf->fontwidth = width;

	return oldsize;
}

static int
t1lib_setfontattr(PMWFONT pfont, int setflags, int clrflags)
{
	int	oldattr = pfont->fontattr;

	pfont->fontattr &= ~clrflags;
	pfont->fontattr |= setflags;

	return oldattr;
}

static void
t1lib_destroyfont(PMWFONT pfont)
{
	PMWT1LIBFONT	pf = (PMWT1LIBFONT)pfont;

	T1_DeleteAllSizes(pf->fontid);
	free(pf);
}

static MWBOOL
t1lib_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWT1LIBFONT	pf = (PMWT1LIBFONT)pfont;
	int				i;
	MWCOORD			width, height, baseline;

	/* FIXME guess all sizes*/
	pfont->fontprocs->GetTextSize(pfont, "H", 1, MWTF_ASCII, &width, &height, &baseline);
	pfontinfo->maxwidth = width;
	pfontinfo->height = height;						/* character height only, not cell height*/
	pfontinfo->baseline = baseline;

	/* FIXME even worse guesses */
	pfontinfo->descent = height - baseline;
	pfontinfo->maxdescent = pfontinfo->descent;
	pfontinfo->maxascent = baseline;
	pfontinfo->linespacing = height + 4;			/* add margin for cell height/linespacing*/

	pfontinfo->firstchar = 32;
	pfontinfo->lastchar = 255;
	pfontinfo->fixed = T1_GetIsFixedPitch(pf->fontid);

	for(i=0; i<256; ++i)
		pfontinfo->widths[i] = width;		/* FIXME lookup each width with gettextsize?*/
	return TRUE;
}
