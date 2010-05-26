#ifndef _DEVFONT_H
#define _DEVFONT_H
/*
 * Copyright (c) 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Font engine header file
 */

/* font engine entry points*/
PMWFONT fnt_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);

#if HAVE_T1LIB_SUPPORT
PMWFONT t1lib_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);
#endif

#if HAVE_FREETYPE_2_SUPPORT
PMWFONT freetype2_createfont(const char *name,MWCOORD height,MWCOORD width,int attr);
PMWFONT freetype2_createfontfrombuffer(const unsigned char *buffer, unsigned length,
	MWCOORD height, MWCOORD width);
#endif

#if HAVE_PCF_SUPPORT
PMWFONT pcf_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);
#endif

#if HAVE_HZK_SUPPORT
PMWFONT hzk_createfont(const char *name, MWCOORD height, MWCOORD width, int fontattr);
int UC16_to_GB(const unsigned char *uc16, int cc, unsigned char *ascii);
#endif

#if HAVE_EUCJP_SUPPORT
PMWFONT eucjp_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);
#endif

#if HAVE_FREETYPE_SUPPORT		// DEPRECATED
typedef MWPIXELVAL OUTPIXELVAL;
void alphablend(PSD psd, OUTPIXELVAL *out, MWPIXELVAL src, MWPIXELVAL dst,
	unsigned char *alpha, int count);
int  freetype_init(PSD psd);
PMWFONT freetype_createfont(const char *name, MWCOORD height, MWCOORD width, int attr);
#endif

#if FONTMAPPER
/* entry point for font selection*/
int select_font(const PMWLOGFONT plogfont, const char **physname);
#endif

/* DBCS routines*/
void dbcs_gettextbits(PMWFONT pfont, int ch, MWTEXTFLAGS flags,
	const MWIMAGEBITS **retmap, MWCOORD *pwidth, MWCOORD *pheight,
	MWCOORD *pbase);
void dbcs_gettextsize(PMWFONT pfont, const unsigned short *str, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);

#endif /* _DEVFONT_H*/
