/*
 * Copyright (c) 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Device-independent font and text drawing routines
 *
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"
#include "devfont.h"

static PMWFONT	gr_pfont;            	/* current font*/

/* temp extern decls*/
extern MWPIXELVAL gr_foreground;
extern MWPIXELVAL gr_background;
extern MWBOOL gr_usebg;

static int  utf8_to_utf16(const unsigned char *utf8, int cc,
		unsigned short *unicode16);

/*
 * Set the font for future calls.
 */
PMWFONT
GdSetFont(PMWFONT pfont)
{
	PMWFONT	oldfont = gr_pfont;

	gr_pfont = pfont;
	return oldfont;
}

/*
 * Select a font, based on various parameters.
 * If plogfont is specified, name and height parms are ignored
 * and instead used from MWLOGFONT.
 * 
 * If height is 0, return builtin font from passed name.
 * Otherwise find builtin font best match based on height.
 */
PMWFONT
GdCreateFont(PSD psd, const char *name, MWCOORD height,
	const PMWLOGFONT plogfont)
{
	int 		i;
	int		fontht;
	int		fontno;
 	int		fontclass;
	int		fontattr = 0;
	PMWFONT		pfont;
	PMWCOREFONT	pf = psd->builtin_fonts;
	MWFONTINFO	fontinfo;
	MWSCREENINFO 	scrinfo;
 	char		fontname[128];

	GdGetScreenInfo(psd, &scrinfo);

	/* if plogfont not specified, use name and height*/
	if (!plogfont) {
		if (!name)
			name = MWFONT_SYSTEM_VAR;
		strcpy(fontname, name);
		fontclass = MWLF_CLASS_ANY;
	} else {
#if FONTMAPPER
		/* otherwise, use MWLOGFONT name and height*/
 		fontclass = select_font(plogfont, fontname);
#else
		name = plogfont->lfFaceName;
		if (!name)
			name = MWFONT_SYSTEM_VAR;
		strcpy(fontname, name);
		fontclass = MWLF_CLASS_ANY;
#endif
		height = plogfont->lfHeight;
		if (plogfont->lfUnderline)
			fontattr = MWTF_UNDERLINE;
	}
	height = abs(height);
 
 	if (!fontclass)
 		goto first;
 
	/* use builtin screen fonts, FONT_xxx, if height is 0 */
 	if (height == 0 || fontclass == MWLF_CLASS_ANY ||
	    fontclass == MWLF_CLASS_BUILTIN) {
  		for(i = 0; i < scrinfo.fonts; ++i) {
 			if(!strcmpi(pf[i].name, fontname)) {
  				pf[i].fontsize = pf[i].cfont->height;
				pf[i].fontattr = fontattr;
printf("createfont: (height == 0) using builtin font %s (%d)\n", fontname, i);
  				return (PMWFONT)&pf[i];
  			}
  		}
 
		/* return first builtin font*/
		if (height == 0 || fontclass == MWLF_CLASS_BUILTIN)
			goto first;
  	}

#if HAVE_HZK_SUPPORT
        /* Make sure the library is initialized */
	if (hzk_init(psd)) {
		pfont = (PMWFONT)hzk_createfont(fontname, height, fontattr);
		if(pfont)		
			return pfont;
		printf("hzk_createfont: %s,%d not found\n", fontname, height);
	}
#endif

#if HAVE_EUCJP_SUPPORT
        {
		pfont = (PMWFONT)eucjp_createfont(fontname, height, fontattr);
		if (pfont)             
			return pfont;
		printf("eucjp_createfont: %s,%d not found\n", fontname, height);
	}
#endif

#if HAVE_FREETYPE_SUPPORT
 	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_FREETYPE) {
		if (freetype_init(psd)) {
			/* FIXME auto antialias for height > 14 for kaffe*/
			if (plogfont && plogfont->lfHeight > 14 &&
				plogfont->lfQuality)
					fontattr |= MWTF_ANTIALIAS;

			pfont = (PMWFONT)freetype_createfont(fontname, height,
					fontattr);
			if(pfont) {
				/* FIXME kaffe kluge*/
				pfont->fontattr |= MWTF_FREETYPE;
				return pfont;
			}
 			printf("freetype_createfont: %s,%d not found\n",
				fontname, height);
		}
  	}
#endif

#if HAVE_FREETYPE_2_SUPPORT
 	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_FREETYPE) {
		if (freetype2_init(psd)) {
			/* FIXME auto antialias for height > 14 for kaffe*/
			if (plogfont && plogfont->lfHeight > 14 &&
				plogfont->lfQuality)
					fontattr |= MWTF_ANTIALIAS;

			pfont = (PMWFONT)freetype2_createfont(fontname, height,
					fontattr);
			if(pfont) {
				/* FIXME kaffe kluge*/
				pfont->fontattr |= MWTF_FREETYPE;
				return pfont;
			}
 			printf("freetype2_createfont: %s,%d not found\n",
				fontname, height);
		}
  	}
#endif

#if HAVE_T1LIB_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_T1LIB) {
		if (t1lib_init(psd)) {
			pfont = (PMWFONT)t1lib_createfont(fontname, height,
					fontattr);
			if(pfont)
				return pfont;
			printf("t1lib_createfont: %s,%d not found\n",
				fontname, height);
		}
  	}
#endif

#ifdef HAVE_PCF_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_PCF) {
		pfont = (PMWFONT) pcf_createfont(fontname, height, fontattr);
		if (pfont) {
			printf("pcf_createfont: using font %s\n", fontname);
			return(pfont);
		}
		printf("pcf_createfont: %s,%d not found\n", fontname, height);
	}
#endif

	/* find builtin font closest in height*/
	if(height != 0) {
		fontno = 0;
		height = abs(height);
		fontht = MAX_MWCOORD;
		for(i = 0; i < scrinfo.fonts; ++i) {
			pfont = (PMWFONT)&pf[i];
			GdGetFontInfo(pfont, &fontinfo);
			if(fontht > abs(height-fontinfo.height)) { 
				fontno = i;
				fontht = abs(height-fontinfo.height);
			}
		}
		pf[fontno].fontsize = pf[fontno].cfont->height;
		pf[fontno].fontattr = fontattr;
printf("createfont: (height != 0) using builtin font %s (%d)\n", pf[fontno].name, fontno);
		return (PMWFONT)&pf[fontno];
	}

first:
	/* Return first builtin font*/
	pf->fontsize = pf->cfont->height;
	pf->fontattr = fontattr;
printf("createfont: (first) using builtin font %s\n", pf[0].name);
	return (PMWFONT)&pf[0];
}

/* Set the font size for the passed font*/
MWCOORD
GdSetFontSize(PMWFONT pfont, MWCOORD fontsize)
{
	MWCOORD	oldfontsize = pfont->fontsize;

	pfont->fontsize = fontsize;

	if (pfont->fontprocs->SetFontSize)
	    pfont->fontprocs->SetFontSize(pfont, fontsize);

	return oldfontsize;
}

/* Set the font rotation angle in tenths of degrees for the passed font*/
int
GdSetFontRotation(PMWFONT pfont, int tenthdegrees)
{
	MWCOORD	oldrotation = pfont->fontrotation;

	pfont->fontrotation = tenthdegrees;

	if (pfont->fontprocs->SetFontRotation)
	    pfont->fontprocs->SetFontRotation(pfont, tenthdegrees);
	
	return oldrotation;
}

/*
 * Set/reset font attributes (MWTF_KERNING, MWTF_ANTIALIAS)
 * for the passed font.
 */
int
GdSetFontAttr(PMWFONT pfont, int setflags, int clrflags)
{
	MWCOORD	oldattr = pfont->fontattr;

	pfont->fontattr &= ~clrflags;
	pfont->fontattr |= setflags;

	if (pfont->fontprocs->SetFontAttr)
	    pfont->fontprocs->SetFontAttr(pfont, setflags, clrflags);
	
	return oldattr;
}

/* Unload and deallocate font*/
void
GdDestroyFont(PMWFONT pfont)
{
	if (pfont->fontprocs->DestroyFont)
		pfont->fontprocs->DestroyFont(pfont);
}

/* Return information about a specified font*/
MWBOOL
GdGetFontInfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	if(!pfont || !pfont->fontprocs->GetFontInfo)
		return FALSE;

	return pfont->fontprocs->GetFontInfo(pfont, pfontinfo);
}

/* Draw a text string at a specifed coordinates in the foreground color
 * (and possibly the background color), applying clipping if necessary.
 * The background color is only drawn if the gr_usebg flag is set.
 * Use the current font.
 */
void
GdText(PSD psd, MWCOORD x, MWCOORD y, const void *str, int cc,MWTEXTFLAGS flags)
{
	const void *	text;
	unsigned long	buf[256];
	int		defencoding = gr_pfont->fontprocs->encoding;

	/* convert encoding if required*/
	if((flags & MWTF_PACKMASK) != defencoding) {
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK;
		flags |= defencoding;
		text = buf;
	} else text = str;

	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

	if(cc <= 0 || !gr_pfont->fontprocs->DrawText)
		return;

	/* draw text string*/
	gr_pfont->fontprocs->DrawText(gr_pfont, psd, x, y, text, cc, flags);
}

/*
 * Draw ascii text using COREFONT type font.
 */
void
corefont_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
	const void *text, int cc, MWTEXTFLAGS flags)
{
	const unsigned char *str = text;
	MWCOORD		width;			/* width of text area */
	MWCOORD 	height;			/* height of text area */
	MWCOORD		base;			/* baseline of text*/
	MWCOORD		startx, starty;
	const MWIMAGEBITS *bitmap;		/* bitmap for characters */
	MWBOOL		bgstate;

	pfont->fontprocs->GetTextSize(pfont, str, cc, &width, &height, &base);
	
	if (flags & MWTF_BASELINE)
		y -= base;
	else if (flags & MWTF_BOTTOM)
		y -= (height - 1);
	startx = x;
	starty = y + base;
	bgstate = gr_usebg;

	switch (GdClipArea(psd, x, y, x + width - 1, y + height - 1)) {
	case CLIP_VISIBLE:
		/* clear background once for all characters*/
		if (gr_usebg)
			psd->FillRect(psd, x, y, x + width - 1, y + height - 1,
				gr_background);

		/* FIXME if we had a low-level text drawer, plug in here:
		psd->DrawText(psd, x, y, str, cc, gr_foreground, pfont);
		GdFixCursor(psd);
		return;
		*/

		/* save state for combined routine below*/
		bgstate = gr_usebg;
		gr_usebg = FALSE;
		break;

	case CLIP_INVISIBLE:
		return;
	}

	/* Get the bitmap for each character individually, and then display
	 * them possibly using clipping for each one.
	 */
	while (--cc >= 0 && x < psd->xvirtres) {
		unsigned int ch = *str++;
#if HAVE_BIG5_SUPPORT
		/* chinese big5 decoding*/
		if (ch >= 0xA1 && ch <= 0xF9 && cc >= 1 &&
			((*str >= 0x40 && *str <= 0x7E) ||
			 (*str >= 0xA1 && *str <= 0xFE)) ) {
				ch = (ch << 8) | *str++;
				--cc;
		}
#endif
#if HAVE_GB2312_SUPPORT
		/* chinese gb2312 decoding*/
		if (ch >= 0xA1 && ch < 0xF8 && cc >= 1 &&
			*str >= 0xA1 && *str < 0xFF) {
				ch = (ch << 8) | *str++;
				--cc;
		}
#endif
#if HAVE_JISX0213_SUPPORT
	/* decode Japanese JISX0213*/
		if (ch >= 0xA1 && ch <= 0xFE && cc >= 1 &&
			*str >= 0xA1 && *str <= 0xFE) {
				ch = (ch << 8) | *str++;
				--cc;
		}else
		if (((ch >= 0x81 && ch <= 0x9F) || (ch >= 0xE0 && ch <= 0xEF)) && cc >= 1)
				if (*str >= 0x40 && *str <= 0xFC && (*str != 0x7F)){
				ch = (ch << 8) | *str++;
				--cc;
		}

#endif

#if HAVE_KSC5601_SUPPORT
		/* Korean KSC5601 decoding */
		if (ch >= 0xA1 && ch <= 0xFE && cc >= 1 &&
			 (*str >= 0xA1 && *str <= 0xFE)) {
				ch = (ch << 8) | *str++;
				--cc;
		}
#endif
#if HAVE_EUCJP_SUPPORT
                /* EUC JP Kanji decoding */
                if (ch >= 0xA1 && ch <= 0xFE && cc >= 1 &&
                         (*str >= 0xA1 && *str <= 0xFE)) {
                                ch = (ch << 8) | *str++;
                                --cc;
                }
#endif
		pfont->fontprocs->GetTextBits(pfont, ch, &bitmap, &width,
			&height, &base);

		/* note: change to bitmap*/
		GdBitmap(psd, x, y, width, height, bitmap);
		x += width;
	}

	if (pfont->fontattr & MWTF_UNDERLINE)
		GdLine(psd, startx, starty, x, starty, FALSE);

	/* restore background draw state*/
	gr_usebg = bgstate;

	GdFixCursor(psd);
}

#if HAVE_T1LIB_SUPPORT | HAVE_FREETYPE_SUPPORT
/*
 * Produce blend table from src and dst based on passed alpha table
 * Used because we don't quite yet have GdArea with alphablending,
 * so we pre-blend fg/bg colors for fade effect.
 */
void
alphablend(PSD psd, OUTPIXELVAL *out, MWPIXELVAL src, MWPIXELVAL dst,
	unsigned char *alpha, int count)
{
	unsigned int	a, d;
	unsigned char	r, g, b;
	MWCOLORVAL	palsrc, paldst;
	extern MWPALENTRY gr_palette[256];

	while (--count >= 0) {
	    a = *alpha++;

#define BITS(pixel,shift,mask)	(((pixel)>>shift)&(mask))
	    if(a == 0)
		*out++ = dst;
	    else if(a == 255)
		*out++ = src;
	    else 
		switch(psd->pixtype) {
	        case MWPF_TRUECOLOR0888:
	        case MWPF_TRUECOLOR888:
		    d = BITS(dst, 16, 0xff);
		    r = (unsigned char)(((BITS(src, 16, 0xff) - d)*a)>>8) + d;
		    d = BITS(dst, 8, 0xff);
		    g = (unsigned char)(((BITS(src, 8, 0xff) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0xff);
		    b = (unsigned char)(((BITS(src, 0, 0xff) - d)*a)>>8) + d;
		    *out++ = (r << 16) | (g << 8) | b;
		    break;

	        case MWPF_TRUECOLOR565:
		    d = BITS(dst, 11, 0x1f);
		    r = (unsigned char)(((BITS(src, 11, 0x1f) - d)*a)>>8) + d;
		    d = BITS(dst, 5, 0x3f);
		    g = (unsigned char)(((BITS(src, 5, 0x3f) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0x1f);
		    b = (unsigned char)(((BITS(src, 0, 0x1f) - d)*a)>>8) + d;
		    *out++ = (r << 11) | (g << 5) | b;
		    break;

	        case MWPF_TRUECOLOR555:
		    d = BITS(dst, 10, 0x1f);
		    r = (unsigned char)(((BITS(src, 10, 0x1f) - d)*a)>>8) + d;
		    d = BITS(dst, 5, 0x1f);
		    g = (unsigned char)(((BITS(src, 5, 0x1f) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0x1f);
		    b = (unsigned char)(((BITS(src, 0, 0x1f) - d)*a)>>8) + d;
		    *out++ = (r << 10) | (g << 5) | b;
		    break;

	        case MWPF_TRUECOLOR332:
		    d = BITS(dst, 5, 0x07);
		    r = (unsigned char)(((BITS(src, 5, 0x07) - d)*a)>>8) + d;
		    d = BITS(dst, 2, 0x07);
		    g = (unsigned char)(((BITS(src, 2, 0x07) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0x03);
		    b = (unsigned char)(((BITS(src, 0, 0x03) - d)*a)>>8) + d;
		    *out++ = (r << 5) | (g << 2) | b;
		    break;

	        case MWPF_PALETTE:
		    /* reverse lookup palette entry for blend ;-)*/
		    palsrc = GETPALENTRY(gr_palette, src);
		    paldst = GETPALENTRY(gr_palette, dst);
		    d = REDVALUE(paldst);
		    r = (unsigned char)(((REDVALUE(palsrc) - d)*a)>>8) + d;
		    d = GREENVALUE(paldst);
		    g = (unsigned char)(((GREENVALUE(palsrc) - d)*a)>>8) + d;
		    d = BLUEVALUE(paldst);
		    b = (unsigned char)(((BLUEVALUE(palsrc) - d)*a)>>8) + d;
		    *out++ = GdFindNearestColor(gr_palette, (int)psd->ncolors,
				MWRGB(r, g, b));
		    break;
	  	}
	}
}
#endif /*HAVE_T1LIB_SUPPORT | HAVE_FREETYPE_SUPPORT*/

#if !HAVE_FREETYPE_SUPPORT
int
GdGetTextSizeEx(PMWFONT pfont, const void *str, int cc,int nMaxExtent,
	int* lpnFit, int* alpDx,MWCOORD *pwidth,MWCOORD *pheight,
	MWCOORD *pbase, MWTEXTFLAGS flags)
{
	*pwidth = *pheight = *pbase = 0;
	return 0;
}

void
GdFreeFontList(MWFONTLIST ***fonts, int n)
{
}

void
GdGetFontList(MWFONTLIST ***fonts, int *numfonts)
{
	*numfonts = -1;
}
#endif /* !HAVE_FREETYPE_SUPPORT*/

/*
 * Convert from one encoding to another
 * Input cc and returned cc is character count, not bytes
 * Return < 0 on error or can't translate
 */
int
GdConvertEncoding(const void *istr, MWTEXTFLAGS iflags, int cc, void *ostr,
	MWTEXTFLAGS oflags)
{
	const unsigned char 	*istr8;
	const unsigned short 	*istr16;
	const unsigned long	*istr32;
	unsigned char 		*ostr8;
	unsigned short 		*ostr16;
	unsigned long		*ostr32;
	unsigned int		ch;
	int			icc;
	unsigned short		buf16[512];

	iflags &= MWTF_PACKMASK;
	oflags &= MWTF_PACKMASK;

	/* allow -1 for len with ascii*/
	if(cc == -1 && (iflags == MWTF_ASCII))
		cc = strlen((char *)istr);

	/* first check for utf8 input encoding*/
	if(iflags == MWTF_UTF8) {
		/* we've only got uc16 now so convert to uc16...*/
		cc = utf8_to_utf16((unsigned char *)istr, cc,
			oflags==MWTF_UC16?(unsigned short*) ostr: buf16);

		if(oflags == MWTF_UC16 || cc < 0)
			return cc;

		/* will decode again to requested format (probably ascii)*/
		iflags = MWTF_UC16;
		istr = buf16;
	}

#if HAVE_HZK_SUPPORT
	if(iflags == MWTF_UC16 && oflags == MWTF_ASCII) {
		/* only support uc16 convert to ascii now...*/
		cc = UC16_to_GB( istr, cc, ostr);
		return cc;
	}
#endif

	icc = cc;
	istr8 = istr;
	istr16 = istr;
	istr32 = istr;
	ostr8 = ostr;
	ostr16 = ostr;
	ostr32 = ostr;

	/* Convert between formats.  Note that there's no error
	 * checking here yet.
	 */
	while(--icc >= 0) {
		switch(iflags) {
		default:
			ch = *istr8++;
			break;
		case MWTF_UC16:
			ch = *istr16++;
			break;
		case MWTF_XCHAR2B:
			ch = *istr8++ << 8;
			ch |= *istr8++;
			break;
		case MWTF_UC32:
			ch = *istr32++;
		}
		switch(oflags) {
		default:
			*ostr8++ = (unsigned char)ch;
			break;
		case MWTF_UC16:
			*ostr16++ = (unsigned short)ch;
			break;
		case MWTF_XCHAR2B:
			*ostr8++ = (unsigned char)(ch >> 8);
			*ostr8++ = (unsigned char)ch;
			break;
		case MWTF_UC32:
			*ostr32++ = ch;
		}
	}
	return cc;
}

/* Get the width and height of passed text string in the passed font*/
void
GdGetTextSize(PMWFONT pfont, const void *str, int cc, MWCOORD *pwidth,
	MWCOORD *pheight, MWCOORD *pbase, MWTEXTFLAGS flags)
{
	const void *	text;
	unsigned long	buf[256];
	MWTEXTFLAGS	defencoding = pfont->fontprocs->encoding;

	/* convert encoding if required*/
	if((flags & MWTF_PACKMASK) != defencoding) {
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK;
		flags |= defencoding;
		text = buf;
	} else text = str;

	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

	if(cc <= 0 || !pfont->fontprocs->GetTextSize) {
		*pwidth = *pheight = *pbase = 0;
		return;
	}

	/* calc height and width of string*/
	pfont->fontprocs->GetTextSize(pfont, text, cc, pwidth, pheight, pbase);
}

/* UTF-8 to UTF-16 conversion.  Surrogates are handeled properly, e.g.
 * a single 4-byte UTF-8 character is encoded into a surrogate pair.
 * On the other hand, if the UTF-8 string contains surrogate values, this
 * is considered an error and returned as such.
 *
 * The destination array must be able to hold as many Unicode-16 characters
 * as there are ASCII characters in the UTF-8 string.  This in case all UTF-8
 * characters are ASCII characters.  No more will be needed.
 *
 * Copyright (c) 2000 Morten Rolland, Screen Media
 */
static int
utf8_to_utf16(const unsigned char *utf8, int cc, unsigned short *unicode16)
{
	int count = 0;
	unsigned char c0, c1;
	unsigned long scalar;

	while(--cc >= 0) {
		c0 = *utf8++;
		/*DPRINTF("Trying: %02x\n",c0);*/

		if ( c0 < 0x80 ) {
			/* Plain ASCII character, simple translation :-) */
			*unicode16++ = c0;
			count++;
			continue;
		}

		if ( (c0 & 0xc0) == 0x80 )
			/* Illegal; starts with 10xxxxxx */
			return -1;

		/* c0 must be 11xxxxxx if we get here => at least 2 bytes */
		scalar = c0;
		if(--cc < 0)
			return -1;
		c1 = *utf8++;
		/*DPRINTF("c1=%02x\n",c1);*/
		if ( (c1 & 0xc0) != 0x80 )
			/* Bad byte */
			return -1;
		scalar <<= 6;
		scalar |= (c1 & 0x3f);

		if ( !(c0 & 0x20) ) {
			/* Two bytes UTF-8 */
			if ( scalar < 0x80 )
				return -1;	/* Overlong encoding */
			*unicode16++ = scalar & 0x7ff;
			count++;
			continue;
		}

		/* c0 must be 111xxxxx if we get here => at least 3 bytes */
		if(--cc < 0)
			return -1;
		c1 = *utf8++;
		/*DPRINTF("c1=%02x\n",c1);*/
		if ( (c1 & 0xc0) != 0x80 )
			/* Bad byte */
			return -1;
		scalar <<= 6;
		scalar |= (c1 & 0x3f);

		if ( !(c0 & 0x10) ) {
			/*DPRINTF("####\n");*/
			/* Three bytes UTF-8 */
			if ( scalar < 0x800 )
				return -1;	/* Overlong encoding */
			if ( scalar >= 0xd800 && scalar < 0xe000 )
				return -1;	/* UTF-16 high/low halfs */
			*unicode16++ = scalar & 0xffff;
			count++;
			continue;
		}

		/* c0 must be 1111xxxx if we get here => at least 4 bytes */
		c1 = *utf8++;
		if(--cc < 0)
			return -1;
		/*DPRINTF("c1=%02x\n",c1);*/
		if ( (c1 & 0xc0) != 0x80 )
			/* Bad byte */
			return -1;
		scalar <<= 6;
		scalar |= (c1 & 0x3f);

		if ( !(c0 & 0x08) ) {
			/* Four bytes UTF-8, needs encoding as surrogates */
			if ( scalar < 0x10000 )
				return -1;	/* Overlong encoding */
			scalar -= 0x10000;
			*unicode16++ = ((scalar >> 10) & 0x3ff) + 0xd800;
			*unicode16++ = (scalar & 0x3ff) + 0xdc00;
			count += 2;
			continue;
		}

		return -1;	/* No support for more than four byte UTF-8 */
	}
	return count;
}
