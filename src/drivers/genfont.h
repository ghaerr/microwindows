/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * MicroWindows Proportional Font Routine Header (proportional font format)
 *
 * These routines are screen driver entry points.
 */

#define NUMBER_FONTS	4	/* number of compiled-in fonts*/

/* entry points*/
MWBOOL	gen_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
void	gen_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
void	gen_gettextbits(PMWFONT pfont, int ch, MWIMAGEBITS *retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
void	gen_unloadfont(PMWFONT pfont);

void	corefont_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, int flags);

/* local data*/
extern MWCOREFONT gen_fonts[NUMBER_FONTS];

/* the following aren't used yet*/
void	gen_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int n, MWPIXELVAL fg);
void 	gen_drawbitmap(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,
		MWIMAGEBITS *table, MWPIXELVAL fgcolor);
