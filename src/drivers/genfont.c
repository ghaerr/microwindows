/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows Proportional Font Routines (proportional font format)
 *
 * This file contains the generalized low-level font/text
 * drawing routines.  Both fixed and proportional fonts are
 * supported, with fixed pitch structure allowing much smaller
 * font files.
 */
#include <stdio.h>
#include "device.h"
#include "genfont.h"

/* Make compiler happy */
#if HAVE_KSC5601_SUPPORT
extern unsigned short convert_ksc_to_johab( unsigned char CH, unsigned char CL);
extern int	get_han_image(int mc, char *retmap );
#endif

/* compiled in fonts*/
extern MWCFONT font_rom8x16, font_rom8x8;
extern MWCFONT font_winFreeSansSerif11x13;
extern MWCFONT font_winFreeSystem14x16;
extern MWCFONT font_winSystem14x16;
extern MWCFONT font_winMSSansSerif11x13;
extern MWCFONT font_winTerminal8x12;
extern MWCFONT font_helvB10, font_helvB12, font_helvR10;
extern MWCFONT font_X5x7, font_X6x13;

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

/* first font is default font if no match*/
MWCOREFONT gen_fonts[NUMBER_FONTS] = {
#if HAVEMSFONTS
	{&fontprocs, 0, 0, 0, MWFONT_SYSTEM_VAR, &font_winSystem14x16},
	{&fontprocs, 0, 0, 0, MWFONT_GUI_VAR, &font_winMSSansSerif11x13},
	{&fontprocs, 0, 0, 0, MWFONT_OEM_FIXED, &font_winTerminal8x12},
	{&fontprocs, 0, 0, 0, MWFONT_SYSTEM_FIXED, &font_X6x13}
#else
	{&fontprocs, 0, 0, 0, MWFONT_SYSTEM_VAR, &font_winFreeSystem14x16},
	{&fontprocs, 0, 0, 0, MWFONT_GUI_VAR, &font_winFreeSansSerif11x13},
	{&fontprocs, 0, 0, 0, MWFONT_OEM_FIXED, &font_rom8x16},
	{&fontprocs, 0, 0, 0, MWFONT_SYSTEM_FIXED, &font_X6x13}
#endif
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
gen_gettextsize(PMWFONT pfont, const void *text, int cc,
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
			if (c >= 0xA1 && c <= 0xF9 && cc >= 1 &&
				((*str >= 0x40 && *str <= 0x7E) ||
			 	(*str >= 0xA1 && *str <= 0xFE)) ) {
					--cc;
					++str;
					width += 12;	/* FIXME*/
			} else
#endif
#if HAVE_GB2312_SUPPORT
			/* chinese gb2312 decoding*/
			if (c >= 0xA1 && c < 0xF8 && cc >= 1 &&
				*str >= 0xA1 && *str < 0xFF) {
					--cc;
					++str;
					width += 12;	/* FIXME*/
			} else
#endif
#if HAVE_KSC5601_SUPPORT
			/* Korean KSC5601 decoding */
			if (c >= 0xA1 && c <= 0xFE && cc >= 1 &&
			 	(*str >= 0xA1 && *str <= 0xFE)) {
					--cc;
					++str;
					width += 16;	/* FIXME*/
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

/*
 * Generalized low level routine to get the bitmap associated
 * with a character.  Handles fixed and proportional fonts.
 */
void
gen_gettextbits(PMWFONT pfont, int ch, MWIMAGEBITS *retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWCFONT	pf = ((PMWCOREFONT)pfont)->cfont;
	int 		n, count, width;
	MWIMAGEBITS *	bits;

#if HAVE_BIG5_SUPPORT
	/* decode chinese big5*/
	int CH = ((unsigned int)ch) >> 8, CL = ((unsigned int)ch) & 0xFF;
	if (CH >= 0xA1 && CH <= 0xF9 && ((CL >= 0x40 && CL <= 0x7E) || (CL >= 0xA1 && CL <= 0xFE)) ) 
	{
	    int Pos;	/* != ((CH - 0xA1) * 94 + (CL - 0xA1)) * 18; */

	    int i;
	    extern unsigned char JMT_BIG5_12X12_FONT_BITMAP[];

	    int seq;
	    {
		seq=0;
		/* ladd=loby-(if(loby<127)?64:98) */
		CL/*c2*/-=(CL/*c2*/<127?64:98);   

		/* hadd=(hiby-164)*157 */
		if (CH/*c1*/>=0xa4)	/* standard font */
		{
			seq=(((CH/*c1*/-164)*157)+CL/*c2*/);
			if (seq>=5809) seq-=408;
		}

		/* hadd=(hiby-161)*157 */
		if (CH/*c1*/<=0xa3)	/* special font */
			seq=(((CH/*c1*/-161)*157)+CL/*c2*/)+13094;
	    }
	    Pos=seq*18;

	    *pwidth = width = 12;
	    *pheight = 12;
	    *pbase = 0;

	    for (i = 0; i < 6; i++) {
		unsigned char *DstBitmap  = ((unsigned char *)retmap) + i * 4;
		unsigned char *FontBitmap = JMT_BIG5_12X12_FONT_BITMAP +
			Pos + i * 3;
		DstBitmap[0] = FontBitmap[1];
		DstBitmap[1] = FontBitmap[0];
		DstBitmap[2] = FontBitmap[1] << 4;
		DstBitmap[3] = FontBitmap[2];
	    }
	    return;
	}
#endif /* HAVE_BIG5_SUPPORT*/

#if HAVE_GB2312_SUPPORT
	/* decode chinese gb2312*/
	int CH = ((unsigned int)ch) >> 8, CL = ((unsigned int)ch) & 0xFF;
	if (CH >= 0xA1 && CH < 0xF8 && CL >= 0xA1 && CL < 0xFF) {
	    int Pos = ((CH - 0xA1) * 94 + (CL - 0xA1)) * 18;
	    int i;
	    extern unsigned char GUO_GB2312_12X12_FONT_BITMAP[];

	    *pwidth = width = 12;
	    *pheight = 12;
	    *pbase = 0;

	    for (i = 0; i < 6; i++) {
		unsigned char *DstBitmap  = ((unsigned char *)retmap) + i * 4;
		unsigned char *FontBitmap = GUO_GB2312_12X12_FONT_BITMAP +
			Pos + i * 3;
		DstBitmap[0] = FontBitmap[1];
		DstBitmap[1] = FontBitmap[0];
		DstBitmap[2] = FontBitmap[1] << 4;
		DstBitmap[3] = FontBitmap[2];
	    }
	    return;
	}
#endif /* HAVE_GB2312_SUPPORT*/

#if HAVE_KSC5601_SUPPORT
	int CH = ((unsigned int)ch) >> 8, CL = ((unsigned int)ch) & 0xFF;
	int	mc;

	if ( CH>= 0xA1 &&  CH<= 0xFE && (CL >= 0xA1 && CL <= 0xFE))
	{
		mc = convert_ksc_to_johab( CH, CL);
		if ( mc )	
			get_han_image(mc, retmap );

		/* Fix me */
		*pwidth = width = 16;
		*pheight = 16;
		*pbase = 0;
		return;
	}
#endif

	/* if char not in font, map to first character by default*/
	if(ch < pf->firstchar || ch >= pf->firstchar+pf->size)
		ch = pf->firstchar;

	ch -= pf->firstchar;

	/* get font bitmap depending on fixed pitch or not*/
	bits = pf->bits + (pf->offset? pf->offset[ch]: (pf->height * ch));
	width = pf->width ? pf->width[ch] : pf->maxwidth;
	count = MWIMAGE_WORDS(width) * pf->height;
	for(n=0; n<count; ++n)
		*retmap++ = *bits++;

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
	IMAGEBITS   bitmap[MAX_CHAR_HEIGHT*MAX_CHAR_WIDTH/MWIMAGE_BITSPERIMAGE];

	/* x, y is bottom left corner*/
	y -= pf->height - 1;
	while (n-- > 0) {
		pfont->GetTextBits(pfont, *s++, bitmap, &width, &height);
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
  int bitcount;			/* number of bits left in bitmap word */

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
