#ifndef _DEVFONT_H
#define _DEVFONT_H
/*
 * Copyright (c) 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Font engine header file
 */

/* settable parameters*/
#define T1LIB_USE_AA_HIGH

#ifdef T1LIB_USE_AA_HIGH
typedef unsigned long	OUTPIXELVAL;
#else
typedef MWPIXELVAL	OUTPIXELVAL;
#endif

#if (UNIX | DOS_DJGPP)
#define strcmpi	strcasecmp
#endif

/* public utility routines*/
void alphablend(PSD psd, OUTPIXELVAL *out, MWPIXELVAL src, MWPIXELVAL dst,
	unsigned char *alpha, int count);

/* font engine entry points*/
PMWCOREFONT fnt_createfont(const char *name, MWCOORD height, int attr);

#if HAVE_T1LIB_SUPPORT
typedef struct MWT1LIBFONT 	*PMWT1LIBFONT;
int  t1lib_init(PSD psd);
PMWT1LIBFONT t1lib_createfont(const char *name, MWCOORD height,int attr);
#endif

#if HAVE_FREETYPE_SUPPORT
typedef struct MWFREETYPEFONT 	*PMWFREETYPEFONT;
int  freetype_init(PSD psd);
PMWFREETYPEFONT freetype_createfont(const char *name, MWCOORD height, int attr);
#endif

#if HAVE_FREETYPE_2_SUPPORT
typedef struct MWFREETYPE2FONT_STRUCT MWFREETYPE2FONT;
typedef MWFREETYPE2FONT *PMWFREETYPE2FONT;
int freetype2_init(PSD psd);
PMWFREETYPE2FONT freetype2_createfont(const char *name, MWCOORD height,
				      int attr);
PMWFREETYPE2FONT freetype2_createfontfrombuffer(const unsigned char *buffer,
						unsigned length,
						MWCOORD height);
#endif

#if HAVE_PCF_SUPPORT
PMWCOREFONT pcf_createfont(const char *name, MWCOORD height, int attr);
#endif

#if HAVE_HZK_SUPPORT
typedef struct MWHZKFONT 	*PMWHZKFONT;
int  hzk_init(PSD psd);
PMWHZKFONT hzk_createfont(const char *name, MWCOORD height, int fontattr);
int UC16_to_GB(const unsigned char *uc16, int cc, unsigned char *ascii);
#endif

#if HAVE_EUCJP_SUPPORT
typedef struct MWEUCJPFONT	*PMWEUCJPFONT;
PMWEUCJPFONT eucjp_createfont(const char *name, MWCOORD height, int attr);
#endif

#if FONTMAPPER
/* entry point for font selection*/
int select_font(const PMWLOGFONT plogfont, const char **physname);
#endif

/* DBCS routines*/
void dbcs_gettextbits(PMWFONT pfont, unsigned int ch, MWTEXTFLAGS flags,
	const MWIMAGEBITS **retmap, MWCOORD *pwidth, MWCOORD *pheight,
	MWCOORD *pbase);
void dbcs_gettextsize(PMWFONT pfont, const unsigned short *str, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);

#endif /* _DEVFONT_H*/
