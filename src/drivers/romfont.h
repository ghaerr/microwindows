/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 *
 * PC ROM Font Routine Header (PC ROM font format)
 *
 * These routines are screen driver entry points.
 */

/* compiled in fonts*/
#define NUMBER_FONTS	1	/* rom font only for now*/

#define	ROM_CHAR_WIDTH	8	/* number of pixels for character width */
#define MAX_ROM_HEIGHT	16	/* max rom character height*/
#define	FONT_CHARS	256	/* number of characters in font tables */

/* int10 functions*/
#define FNGETROMADDR	0x1130	/* function for address of rom character table*/
#define GETROM8x14	0x0200	/* want address of ROM 8x14 char table*/

/* entry points*/
void	pcrom_init(PSD psd);
MWBOOL	pcrom_getfontinfo(PMWFONT pfont,PMWFONTINFO pfontinfo);
void	pcrom_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
void	pcrom_gettextbits(PMWFONT pfont, int ch, MWIMAGEBITS *retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
void	pcrom_unloadfont(PMWFONT pfont);

void	corefont_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, int flags);

/* local data*/
extern int	ROM_CHAR_HEIGHT; /* number of scan lines in fonts in ROM */
extern FARADDR 	rom_char_addr;
extern MWCOREFONT pcrom_fonts[NUMBER_FONTS];

/* the following aren't used yet*/
void	pcrom_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int n, MWPIXELVAL fg);
void 	gen_drawbitmap(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,
		MWIMAGEBITS *table, MWPIXELVAL fgcolor);
