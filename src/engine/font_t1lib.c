/*
 * Copyright (c) 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * T1lib Adobe type1 routines originally contributed by Vidar Hokstad
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <t1lib.h>
#include "device.h"
#include "devfont.h"

typedef struct MWT1LIBFONT {
	PMWFONTPROCS	fontprocs;	/* common hdr*/
	MWCOORD		fontsize;
	int		fontrotation;
	int		fontattr;		

	int		fontid;		/* t1lib stuff*/
} MWT1LIBFONT;

int  t1lib_init(PSD psd);
PMWT1LIBFONT t1lib_createfont(const char *name, MWCOORD height,int attr);

static void t1lib_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, MWTEXTFLAGS flags);
static MWBOOL t1lib_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void t1lib_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
		MWCOORD *pbase);
static void t1lib_destroyfont(PMWFONT pfont);

/* handling routines for MWT1LIBFONT*/
static MWFONTPROCS t1lib_procs = {
	MWTF_ASCII,			/* routines expect ascii*/
	t1lib_getfontinfo,
	t1lib_gettextsize,
	NULL,				/* gettextbits*/
	t1lib_destroyfont,
	t1lib_drawtext,
	NULL,				/* setfontsize*/
	NULL,				/* setfontrotation*/
	NULL,				/* setfontattr*/
	NULL,				/* duplicate not yet implemented */
};

/* temp extern decls*/
extern MWPIXELVAL gr_foreground;
extern MWPIXELVAL gr_background;
extern MWBOOL gr_usebg;

int
t1lib_init(PSD psd)
{
	char **encoding;
	static int inited = 0;

	if (inited)
		return 1;

	T1_SetBitmapPad(8);
	if (!T1_InitLib(0))
		return 0;

	/* set default Latin1 encoding*/
	encoding = T1_LoadEncoding("IsoLatin1.enc");
	T1_SetDefaultEncoding(encoding);

#ifdef T1LIB_USE_AA_HIGH	 
	T1_AASetLevel(T1_AA_HIGH);
#else
	T1_AASetLevel(T1_AA_LOW);
#endif	 
#if 0
	/* kluge: this is required if 16bpp drawarea driver is used*/
	if(psd->bpp == 16)
		T1_AASetBitsPerPixel(16);
	else
#endif
		T1_AASetBitsPerPixel(sizeof(MWPIXELVAL)*8);

	inited = 1;
	return 1;
}

PMWT1LIBFONT
t1lib_createfont(const char *name, MWCOORD height, int attr)
{
	PMWT1LIBFONT	pf;
	int		id;
	char *		p;
	char		buf[256];

	/* match name against t1 font id's from t1 font database*/
	for(id=0; id<T1_Get_no_fonts(); ++id) {
		strncpy(buf, T1_GetFontFileName(id), sizeof(buf));

		/* remove extension*/
		for(p=buf; *p; ++p) {
			if(*p == '.') {
				*p = 0;
				break;
			}
		}

		if(!strcmpi(name, buf)) {
			/* allocate font structure*/
			pf = (PMWT1LIBFONT)calloc(sizeof(MWT1LIBFONT), 1);
			if (!pf)
				return NULL;
			pf->fontprocs = &t1lib_procs;
			GdSetFontSize((PMWFONT)pf, height);
			GdSetFontRotation((PMWFONT)pf, 0);
			GdSetFontAttr((PMWFONT)pf, attr, 0);
			pf->fontid = id;
			return pf;
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
	PMWT1LIBFONT	pf = (PMWT1LIBFONT)pfont;
	const unsigned char *str = text;
   	MWCOORD		width;			/* width of text area */
	MWCOORD 	height;			/* height of text area */
	MWCOORD		underliney;
        GLYPH * g; /* T1lib glyph structure. Memory handling by T1lib */
#ifdef T1LIB_USE_AA_HIGH   
        OUTPIXELVAL	gvals[17];

        /* Blending array for antialiasing. The steeper the values increase
	 * near the end, the sharper the characters look, but also more jagged
	 */
        static unsigned char blend[17] = {
	   0x00, 0x00, 0x04, 0x0c, 0x10, 0x14, 0x18, 0x20,
	   0x30, 0x38, 0x40, 0x50, 0x70, 0x80, 0xa0, 0xc0, 0xff
	};
#else   
        OUTPIXELVAL	gvals[5];
        static unsigned char blend[5] = { 0x00, 0x44, 0x88, 0xcc, 0xff };
#endif   

        /* Check if we should throw out some fonts */

        if (pf->fontattr&MWTF_ANTIALIAS) {
#ifdef T1LIB_USE_AA_HIGH      
	   alphablend(psd, gvals, gr_foreground, gr_background, blend, 17);
           T1_AAHSetGrayValues(gvals);
#else      
	   alphablend(psd, gvals, gr_foreground, gr_background, blend, 5);
           T1_AASetGrayValues(gvals[0],gvals[1],gvals[2],gvals[3],gvals[4]);
#endif
	   g = T1_AASetString(pf->fontid,(char *)str,cc,0,
		(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0,
		pf->fontsize * 1.0, 0);

	   if (g && g->bits) {
	      /*MWPIXELVAL save = gr_background;*/
	      width = g->metrics.rightSideBearing - g->metrics.leftSideBearing;
	      height = g->metrics.ascent - g->metrics.descent;

	      if(flags & MWTF_BASELINE)
		y -= g->metrics.ascent;
	      else if(flags & MWTF_BOTTOM)
		y -= (height - 1);
	      underliney = y + g->metrics.ascent;

	      /* FIXME: Looks damn ugly if usebg is false.
	       * Will be handled when using alphablending in GdArea...
	       */
	      /* clipping handled in GdArea*/
	      /*FIXME kluge for transparency*/
	      /*gr_background = gr_foreground + 1;*/
	      /*gr_usebg = 0;*/
	      GdArea(psd,x,y, width, height, g->bits, MWPF_PIXELVAL);
	      /*gr_background = save;*/

	      if (pf->fontattr & MWTF_UNDERLINE)
		   GdLine(psd, x, underliney, x+width, underliney, FALSE);

	   }
	} else {
	   /* Do non-aa drawing */
	   g = T1_SetString(pf->fontid,(char *)str,cc,0,
			(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0,
			pf->fontsize * 1.0, 0);

	   if (g && g->bits) {
	      unsigned char * b;
	      int xoff;
	      int maxy;
	      int xmod;
	      
	      /* I'm sure this sorry excuse for a bitmap rendering routine can
	       * be optimized quite a bit ;)
	       */
	      width = g->metrics.rightSideBearing - g->metrics.leftSideBearing;
	      height = g->metrics.ascent - g->metrics.descent;

	      if(flags & MWTF_BASELINE)
		y -= g->metrics.ascent;
	      else if(flags & MWTF_BOTTOM)
		y -= (height - 1);
	      underliney = y + g->metrics.ascent;
	      
	      b = g->bits;
	      maxy = y + height;
	      
/*	      if ((x + width) > psd->xvirtres) {
		 xmod = (x + width - psd->xvirtres + 7) >> 3;
		 width = width + x + width - psd->xvirtres;
	      } else xmod = 0;
*/
	      xmod = 0;
	      while (y < maxy) {
		 unsigned char data;
		 xoff = 0;
		 while (xoff < width ) {
		    if (!(xoff % 8)) {
		       data = *b;
		       b++;
		    }
		    
		    if (GdClipPoint(psd, x+xoff,y)) {
		       if (gr_usebg) {
	 		  psd->DrawPixel(psd,x+xoff,y,
			      data & (1 << (xoff % 8)) ?
			            gr_foreground : gr_background);
		       } else if (data & (1 << (xoff % 8))) {
			  psd->DrawPixel(psd,x+xoff,y, gr_foreground);
		       }
		    }
		    xoff++;
		 }
		 b += xmod;
		 y++;
	      }
	      if (pf->fontattr & MWTF_UNDERLINE)
		   GdLine(psd, x, underliney, x+xoff, underliney, FALSE);
	   }
        }

   if (g && g->bits) {
	   /* Save some memory */
	   free(g->bits);
           g->bits = 0; /* Make sure T1lib doesnt try to free it again */
   }

   GdFixCursor(psd);
}

static MWBOOL
t1lib_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	int	i;
	MWCOORD	width, height, baseline;

	/* FIXME, guess all sizes*/
	GdGetTextSize(pfont, "A", 1, &width, &height, &baseline, MWTF_ASCII);
	pfontinfo->height = height;
	pfontinfo->maxwidth = width;
	pfontinfo->baseline = baseline;

	/* FIXME: Even worse guesses */
	pfontinfo->linespacing = pfontinfo->height;
	pfontinfo->descent = pfontinfo->height - pfontinfo->baseline;
	pfontinfo->maxascent = pfontinfo->baseline;
	pfontinfo->maxdescent = pfontinfo->descent;

	pfontinfo->firstchar = 32;
	pfontinfo->lastchar = 255;
	pfontinfo->fixed = TRUE;
	for(i=0; i<256; ++i)
		pfontinfo->widths[i] = width;
	return TRUE;
}

/* Get the width and height of passed text string in the current font*/
static void
t1lib_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWT1LIBFONT		pf = (PMWT1LIBFONT)pfont;
	const unsigned char *	str = text;
	GLYPH *			g;

	g = T1_SetString(pf->fontid, (char *)str, cc, 0,
			(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0, pf->fontsize * 1.0, 0);
	*pwidth = g->metrics.rightSideBearing - g->metrics.leftSideBearing;
	*pheight = g->metrics.ascent - g->metrics.descent;
	*pbase = g->metrics.ascent;
	if(g && g->bits) {
		free(g->bits);
		g->bits = 0;
	}
#if 0
	BBox 			b;

	/* FIXME. Something is *VERY* wrong here */
	b = T1_GetStringBBox(pf->fontid, str, cc, 0, (pf->fontattr&MWTF_KERNING)?T1_KERNING:0);

	DPRINTF("b.urx = %d, b.llx = %d\n",b.urx, b.llx);
	DPRINTF("b.ury = %d, b.lly = %d\n",b.ury, b.lly);
	*pwidth = (b.urx - b.llx);
	*pheight = (b.lly - b.ury);
#endif
}

static void
t1lib_destroyfont(PMWFONT pfont)
{
	PMWT1LIBFONT	pf = (PMWT1LIBFONT)pfont;

	T1_DeleteAllSizes(pf->fontid);
	free(pf);
}
