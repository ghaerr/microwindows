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
#include <stdlib.h>
#include <string.h>
#include "device.h"
#include "genfont.h"

#define USE_HBF_FONT	1

#ifdef USE_HBF_FONT
#include "hbf.h"

static char *locale;
HBF *font_big5_24;
HBF *font_big5_16;
int hbf_init=0;
#endif

/* compiled in fonts*/
extern MWCFONT font_arial20;
extern MWCFONT font_arial24;
extern MWCFONT font_arial28;
extern MWCFONT font_arial16;
extern MWCFONT font_rom8x16, font_rom8x8;
extern MWCFONT font_winFreeSansSerif11x13;
extern MWCFONT font_winFreeSystem14x16;
extern MWCFONT font_winSystem14x16;
extern MWCFONT font_winMSSansSerif11x13;
extern MWCFONT font_winTerminal8x12;
extern MWCFONT font_helvB10, font_helvB12, font_helvR10;
extern MWCFONT font_X5x7, font_X6x13;

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

/* first font is default font if no match*/
MWCOREFONT gen_fonts[NUMBER_FONTS] = {
#if HAVEMSFONTS
#error
	{&mwfontprocs, 0, 0, 0, "arial20", &font_arial20},
	{&mwfontprocs, 0, 0, 0, "arial24", &font_arial24},
	{&mwfontprocs, 0, 0, 0, "arial28", &font_arial28},
	{&mwfontprocs, 0, 0, 0, MWFONT_GUI_VAR, &font_winMSSansSerif11x13},
	{&mwfontprocs, 0, 0, 0, MWFONT_OEM_FIXED, &font_winTerminal8x12},
	{&mwfontprocs, 0, 0, 0, MWFONT_SYSTEM_FIXED, &font_X6x13}
#else
	{&mwfontprocs, 0, 0, 0, 0, "arial16", &font_arial16},
	{&mwfontprocs, 0, 0, 0, 0, "arial20", &font_arial20},
	{&mwfontprocs, 0, 0, 0, 0, "arial24", &font_arial24},
	{&mwfontprocs, 0, 0, 0, 0, "arial28", &font_arial28},
#endif
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
#if HAVE_BIG5_SUPPORT
            /* chinese big5 decoding*/
            if ((c >= 0xA4 && c <= 0xC5 && cc >= 1 &&
                ((*str >= 0x40 && *str <= 0x7E) ||
                (*str >= 0xA1 && *str <= 0xFE)))||(c==0xC6&&(*str >= 0x40 && *str <= 0x7E) )) {
                    --cc;
                    ++str;
                    width += (pf->maxwidth < 20) ? 16 : 24; /* FIXME */
            } else
#endif
#if HAVE_GB2312_SUPPORT
            /* chinese gb2312 decoding*/
            if (c >= 0xA1 && c < 0xF8 && cc >= 1 &&
                *str >= 0xA1 && *str < 0xFF) {
                    --cc;
                    ++str;
                    width += (pf->maxwidth < 20) ? 16 : 28; /* FIXME */
            } else
#endif
#if HAVE_KSC5601_SUPPORT
            /* Korean KSC5601 decoding */
            if (c >= 0xA1 && c <= 0xFE && cc >= 1 &&
                (*str >= 0xA1 && *str <= 0xFE)) {
                    --cc;
                    ++str;
                    width += 16;    /* FIXME*/
            } else
#endif
                if(c >= pf->firstchar && c < pf->firstchar+pf->size)
                    width += pf->width[c - pf->firstchar];
		}
	}
	*pwidth = width;
	*pheight = pf->height;
	*pbase = pf->ascent;
}

#ifdef USE_HBF_FONT
static int Font_Init()
{
    if(getenv("LOCALE")==NULL){
        setenv("LOCALE","tc",1);
    }
    locale=getenv("LOCALE");
    char font16[20];
    char font24[20];
    sprintf(font16,"/font/%s16.hbf",locale);
    sprintf(font24,"/font/%s24.hbf",locale);
    if((font_big5_16 = hbfOpen(font16))==NULL){
        return 0;
    }
    if((font_big5_24 = hbfOpen(font24))==NULL){
        return 0;
    }
    hbf_init = 1;
    return 1;
}
#endif

/*
 * Generalized low level routine to get the bitmap associated
 * with a character.  Handles fixed and proportional fonts.
 */
void
gen_gettextbits(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
    PMWCFONT            pf = ((PMWCOREFONT)pfont)->cfont;
    int                 width;
	const MWIMAGEBITS  *bits;
	static MWIMAGEBITS  map[MAX_CHAR_HEIGHT * MAX_CHAR_WIDTH / MWIMAGE_BITSPERIMAGE];

    *retmap = map;
#ifdef USE_HBF_FONT
    int no_hbf_font = 0;

    if (hbf_init==0) {
        if(Font_Init()==0){
            no_hbf_font = 1;
        }
    }
#endif

#if HAVE_BIG5_SUPPORT
#ifdef USE_HBF_FONT
    if(no_hbf_font == 0){
        int CH = ((unsigned int)ch) >> 8, CL = ((unsigned int)ch) & 0xFF;
        if ((CH >= 0xA4 && CH <= 0xC5 && ((CL >= 0x40 && CL <= 0x7E) || (CL >= 0xA1 && CL <= 0xFE)))||
            (CH==0xC6&&(CL >= 0x40 && CL <= 0x7E)) )
        {
            int i;
            int seq;
            {
                seq=0;
                /* ladd=loby-(if(loby<127)?64:98) */
                CL/*c2*/-=(CL/*c2*/<127?64:98);

                if (CH <= 0xC6) seq = (CH - 0xA1) * 157 + CL;
                else if (CH >= 0xC9)
                    seq = (CH - 0xC9) * 157 + CL + (0xC6 - 0xA1) * 157 + 0x3E;
            }

            if (pf->maxwidth < 20) {
                unsigned char *bitmap;
                *pwidth = hbfBitmapBBox(font_big5_16)->hbf_width;
                *pheight = hbfBitmapBBox(font_big5_16)->hbf_height;
                *pbase = 0;
                bitmap = (unsigned char *)hbfGetBitmap(font_big5_16,(HBF_CHAR)ch);

                if(bitmap==NULL) {
                    memset(map,0xff,32);
                }
                else
                {
                    for (i = 0; i < *pheight; i++) {
                        unsigned char *DstBitmap  = ((unsigned char *)map) + i * 2;
                        unsigned char *FontBitmap =bitmap+i*2;
                        DstBitmap[0] = FontBitmap[1];
                        DstBitmap[1] = FontBitmap[0];
                    }
                }
            } else {
                unsigned char *bitmap;
                *pwidth = hbfBitmapBBox(font_big5_24)->hbf_width;
                *pheight = hbfBitmapBBox(font_big5_24)->hbf_height;
                *pbase = 0;
                bitmap = (unsigned char *)hbfGetBitmap(font_big5_24,(HBF_CHAR)ch);
                if(bitmap==NULL) {
                    memset(map, 0xff, 112);
                }
                else
                {
                    for (i = 0; i < *pheight; i++) {
                        unsigned char *DstBitmap  = ((unsigned char *)map) + i * 4;
                        unsigned char *FontBitmap =bitmap+i*3;
                        DstBitmap[0] = FontBitmap[1];
                        DstBitmap[1] = FontBitmap[0];
                        DstBitmap[2] = 0;
                        DstBitmap[3] = FontBitmap[2];
                    }
                }
            }
            return;
        }
    }
#endif
#endif /* HAVE_BIG5_SUPPORT*/
	/* if char not in font, map to first character by default*/
	if(ch < pf->firstchar || ch >= pf->firstchar+pf->size)
		ch = pf->firstchar;

	ch -= pf->firstchar;

	/* get font bitmap depending on fixed pitch or not*/
	bits = pf->bits + (pf->offset? pf->offset[ch]: (pf->height * ch));
 	width = pf->width ? pf->width[ch] : pf->maxwidth;
//	count = MWIMAGE_WORDS(width) * pf->height; 

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
