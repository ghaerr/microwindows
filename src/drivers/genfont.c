/*
 * Copyright (c) 1999, 2000, 2003 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows Proportional Fonts & Routines (proportional font format)
 *
 * Modify this file to add/subtract builtin fonts from Microwindows
 *
 * This file contains the generalized low-level font/text
 * drawing routines.  Both fixed and proportional fonts are
 * supported, with fixed pitch structure allowing much smaller
 * font files.
 */
#include <stdio.h>
#include "device.h"
#include "genfont.h"

/* compiled in fonts*/
extern MWCFONT font_winFreeSansSerif11x13;	/* new MWFONT_SYSTEM_VAR (was MWFONT_GUI_VAR)*/
extern MWCFONT font_X6x13;			/* MWFONT_SYSTEM_FIXED (should be ansi)*/
/*extern MWCFONT font_winFreeSystem14x16;*/	/* deprecated MWFONT_SYSTEM_VAR*/
/*extern MWCFONT font_rom8x16;*/		/* deprecated MWFONT_OEM_FIXED*/
/*extern MWCFONT font_rom8x8, font_X5x7;*/	/* unused*/

/* handling routines for MWCOREFONT*/
static MWFONTPROCS fontprocs = {
	MWTF_ASCII,		/* routines expect ascii*/
	gen_getfontinfo,
	gen_gettextsize,
	gen_gettextbits,
	gen_unloadfont,
	corefont_drawtext,
	NULL,			/* setfontsize*/
	NULL,			/* setfontrotation*/
	NULL,			/* setfontattr*/
};

/*
 * Starting in v0.89pl12, we've moved to just two standard fonts,
 * MWFONT_SYSTEM_VAR, and MWFONT_SYSTEM_FIXED in order to reduce
 * the core Microwindows size.  The original, slightly smaller
 * MWFONT_GUI_VAR (ansi) is used as MWFONT_SYSTEM_VAR.  The original
 * MWFONT_SYSTEM_VAR (ansi) and MWFONT_OEM_FIXED are removed.
 * However, we redirect requests for the deprecated MWFONT_GUI_VAR
 * and MWFONT_OEM_FIXED to the included system variable and fixed
 * pitch fonts, respectively, to keep older programs running as expected.
 *
 * Additional builtin fonts can be added here by extending the 
 * gen_fonts array.  An better alternative, if running on a filesystem
 * is to use the new HAVE_FNT_SUPPORT .fnt file loader, which operates
 * internally exactly the same way after the fonts are loaded.  BDF
 * fonts can be converted to either .c format for use here or .fnt 
 * format for use by the .fnt loader using the fonts/convbdf.c program.
 */

/* first font is default font*/
MWCOREFONT gen_fonts[NUMBER_FONTS] = {
	{&fontprocs, 0, 0, 0, MWFONT_SYSTEM_VAR,   &font_winFreeSansSerif11x13},
	{&fontprocs, 0, 0, 0, MWFONT_SYSTEM_FIXED, &font_X6x13},
	/* deprecated redirections for the time being*/
	{&fontprocs, 0, 0, 0, "Helvetica",         &font_winFreeSansSerif11x13}, /* redirect*/
	{&fontprocs, 0, 0, 0, "Terminal",          &font_X6x13}	/* redirect*/
};

/*
 * Generalized low level get font info routine.  This
 * routine works with fixed and proportional fonts.
 */
MWBOOL
gen_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWCFONT	pf = ((PMWCOREFONT)pfont)->cfont;
	int		i;

	pfontinfo->maxwidth = pf->maxwidth;
	pfontinfo->height = pf->height;
	pfontinfo->baseline = pf->ascent;
	pfontinfo->firstchar = pf->firstchar;
	pfontinfo->lastchar = pf->firstchar + pf->size - 1;
	pfontinfo->fixed = pf->width == NULL? TRUE: FALSE;
	for(i=0; i<256; ++i) {
		if(pf->width == NULL)
			pfontinfo->widths[i] = pf->maxwidth;
		else {
			if(i<pf->firstchar || i >= pf->firstchar+pf->size)
				pfontinfo->widths[i] = 0;
			else pfontinfo->widths[i] = pf->width[i-pf->firstchar];
		}
	}
	return TRUE;
}

/*
 * Generalized low level routine to calc bounding box for text output.
 * Handles both fixed and proportional fonts.  Passed ascii string.
 */
void
gen_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWCFONT		pf = ((PMWCOREFONT)pfont)->cfont;
	const unsigned char *	str = text;
	unsigned int		c;
	int			width;

	if(pf->width == NULL)
		width = cc * pf->maxwidth;
	else {
		width = 0;
		while(--cc >= 0) {
			c = *str++;
			if(c >= pf->firstchar && c < pf->firstchar+pf->size)
				width += pf->width[c - pf->firstchar];
		}
	}
	*pwidth = width;
	*pheight = pf->height;
	*pbase = pf->ascent;
}

#if HAVE_FNT_SUPPORT | HAVE_PCF_SUPPORT
/*
 * Routine to calc bounding box for text output.
 * Handles both fixed and proportional fonts.  Passed MWTF_UC16 string.
 */
void
gen16_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWCFONT		pf = ((PMWCOREFONT) pfont)->cfont;
	const unsigned short *	str = text;
	unsigned		int c;
	int			width;

	if (pf->width == NULL)
		width = cc * pf->maxwidth;
	else {
		width = 0;
		while (--cc >= 0) {
			c = *str++;
			if (c >= pf->firstchar && c < pf->firstchar + pf->size)
				width += pf->width[c - pf->firstchar];
		}
	}
	*pwidth = width;
	*pheight = pf->height;
	*pbase = pf->ascent;
}
#endif /* HAVE_FNT_SUPPORT | HAVE_PCF_SUPPORT*/

/*
 * Generalized low level routine to get the bitmap associated
 * with a character.  Handles fixed and proportional fonts.
 */
void
gen_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWCFONT		pf = ((PMWCOREFONT)pfont)->cfont;
	int 			count, width;
	const MWIMAGEBITS *	bits;

	/* if char not in font, map to first character by default*/
	if(ch < pf->firstchar || ch >= pf->firstchar+pf->size)
		ch = pf->firstchar;

	ch -= pf->firstchar;

	/* get font bitmap depending on fixed pitch or not*/
	bits = pf->bits + (pf->offset? pf->offset[ch]: (pf->height * ch));
 	width = pf->width ? pf->width[ch] : pf->maxwidth;
	count = MWIMAGE_WORDS(width) * pf->height; 

	*retmap = bits;

	/* return width depending on fixed pitch or not*/
	*pwidth = width; 
	*pheight = pf->height;
	*pbase = pf->ascent; 
}

void
gen_unloadfont(PMWFONT pfont)
{
	/* builtins can't be unloaded*/
}

#if NOTUSED
/* 
 * Generalized low level text draw routine, called only
 * if no clipping is required
 */
void
gen_drawtext(PMWFONT pfont,PSD psd,MWCOORD x,MWCOORD y,const void *text,
	int n,MWPIXELVAL fg)
{
	PMWCFONT		pf = ((PMWCOREFONT)pfont)->cfont;
	const unsigned char *	str = text;
	MWCOORD 		width;		/* width of character */
	MWCOORD 		height;		/* height of character */
	const MWIMAGEBITS *	bitmap;

	/* x, y is bottom left corner*/
	y -= pf->height - 1;
	while (n-- > 0) {
		pfont->GetTextBits(pfont, *s++, &bitmap, &width, &height);
		gen_drawbitmap(psd, x, y, width, height, bitmap, fg);
		x += width;
	}
}

/*
 * Generalized low level bitmap output routine, called
 * only if no clipping is required.  Only the set bits
 * in the bitmap are drawn, in the foreground color.
 */
void
gen_drawbitmap(PSD psd,MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	MWIMAGEBITS *table, PIXELVAL fgcolor)
{
  MWCOORD minx;
  MWCOORD maxx;
  MWIMAGEBITS bitvalue;	/* bitmap word value */
  int bitcount;		/* number of bits left in bitmap word */

  minx = x;
  maxx = x + width - 1;
  bitcount = 0;
  while (height > 0) {
	if (bitcount <= 0) {
		bitcount = MWIMAGE_BITSPERIMAGE;
		bitvalue = *table++;
	}
	if (MWIMAGE_TESTBIT(bitvalue))
		psd->DrawPixel(psd, x, y, fgcolor);
	bitvalue = MWIMAGE_SHIFTBIT(bitvalue);
	--bitcount;
	if (x++ == maxx) {
		x = minx;
		++y;
		--height;
		bitcount = 0;
	}
  }
}
#endif /* NOTUSED*/
