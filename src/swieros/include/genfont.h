/*
 * Copyright (c) 1999, 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows Proportional Font Routine Header (proportional font format)
 *
 * These routines are screen driver entry points.
 */

#define NUMBER_FONTS	4	/* number of compiled-in fonts*/

/* entry points*/
void	gen_setfontproc(MWCOREFONT *pf);
MWBOOL	gen_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
void	gen_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
		MWCOORD *pbase);
void	gen_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
void	gen_unloadfont(PMWFONT pfont);

void 	gen_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, MWTEXTFLAGS flags);
void	gen16_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
		MWCOORD *pbase);

/* local data*/
extern MWCOREFONT gen_fonts[NUMBER_FONTS];
extern MWFONTPROCS mwfontprocs;	/* builtin fontprocs - for special DBCS handling*/
