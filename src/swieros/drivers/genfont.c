/*
 * Copyright (c) 1999, 2000, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
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

/* compiled in fonts*/
extern MWCFONT font_winFreeSansSerif11x13;	/* new MWFONT_SYSTEM_VAR (was MWFONT_GUI_VAR)*/
extern MWCFONT font_X6x13;			/* MWFONT_SYSTEM_FIXED (should be ansi)*/
/*extern MWCFONT font_winFreeSystem14x16;*/	/* deprecated MWFONT_SYSTEM_VAR*/
/*extern MWCFONT font_rom8x16;*/		/* deprecated MWFONT_OEM_FIXED*/
/*extern MWCFONT font_rom8x8, font_X5x7;*/	/* unused*/

/* handling routines for MWCOREFONT*/
MWFONTPROCS mwfontprocs = {
	0,				/* capabilities*/
	MWTF_ASCII,		/* routines expect ascii*/
	NULL,			/* init*/
	NULL,			/* createfont*/
	gen_getfontinfo,
	gen_gettextsize,
	gen_gettextbits,
	gen_unloadfont,
	gen_drawtext,
	NULL,			/* setfontsize*/
	NULL,			/* setfontrotation*/
	NULL,			/* setfontattr*/
	NULL			/* duplicate*/
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
	{&mwfontprocs, 0, 0, 0, 0, MWFONT_SYSTEM_VAR,   &font_winFreeSansSerif11x13},
	{&mwfontprocs, 0, 0, 0, 0, MWFONT_SYSTEM_FIXED, &font_X6x13},
	/* deprecated redirections for the time being*/
	{&mwfontprocs, 0, 0, 0, 0, "Helvetica",         &font_winFreeSansSerif11x13}, /* redirect*/
	{&mwfontprocs, 0, 0, 0, 0, "Terminal",          &font_X6x13}	/* redirect*/
};

/* Pointer to an user builtin font table. */
MWCOREFONT *user_builtin_fonts = NULL;

/*  Sets the fontproc to fontprocs.  */
void
gen_setfontproc(MWCOREFONT *pf)
{
	pf->fontprocs = &mwfontprocs;
}

/*
 * Generalized low level get font info routine.  This
 * routine works with fixed and proportional fonts.
 */
MWBOOL
gen_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWCFONT	pf = ((PMWCOREFONT)pfont)->cfont;
	int			i;

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
 * Handles both fixed and proportional fonts.  Passed ASCII or UC16 string.
 */
void
gen_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWCFONT		pf = ((PMWCOREFONT)pfont)->cfont;
	const unsigned char *str = text;
	const unsigned short *istr = text;
	int				width;

	if(pf->width == NULL)
		width = cc * pf->maxwidth;
	else {
		width = 0;
		while(--cc >= 0) {
			unsigned int	c;

			if (pfont->fontprocs->encoding == MWTF_UC16)
				c = *istr++;
			else c = *str++;

			/* if char not in font, map to first character by default*/
			if(c < pf->firstchar || c >= pf->firstchar+pf->size)
				c = pf->firstchar;

			/*if(c >= pf->firstchar && c < pf->firstchar+pf->size)*/
				width += pf->width[c - pf->firstchar];
		}
	}
	*pwidth = width;
	*pheight = pf->height;
	*pbase = pf->ascent;
}

/*
 * Generalized low level routine to get the bitmap associated
 * with a character.  Handles fixed and proportional fonts.
 */
void
gen_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWCFONT		pf = ((PMWCOREFONT)pfont)->cfont;
	int 			width;
	const MWIMAGEBITS *	bits;

	/* if char not in font, map to first character by default*/
	if(ch < pf->firstchar || ch >= pf->firstchar+pf->size)
		ch = pf->firstchar;

	ch -= pf->firstchar;

	/* get font bitmap depending on fixed pitch or not*/
	/* automatically detect if offset is 16 or 32 bit */
	if( pf->offset ) {
		if( ((uint32_t *)pf->offset)[0] >= 0x00010000 )
			bits = pf->bits + ((unsigned short *)pf->offset)[ch];
		else
			bits = pf->bits + ((uint32_t *)pf->offset)[ch];
	} else
		bits = pf->bits + (pf->height * ch);
		
 	width = pf->width ? pf->width[ch] : pf->maxwidth;

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
