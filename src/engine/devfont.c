/*
 * Copyright (c) 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 */
/**
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 *
 *
 * Device-independent font and text drawing routines
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"
#include "devfont.h"
#include "../drivers/genfont.h"

/**
 * The current font.
 */
static PMWFONT	gr_pfont;

/* temp extern decls*/
extern MWPIXELVAL gr_foreground;
extern MWPIXELVAL gr_background;
extern MWBOOL gr_usebg;

void corefont_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, MWTEXTFLAGS flags);
static int  utf8_to_utf16(const unsigned char *utf8, int cc,
		unsigned short *unicode16);

/**
 * Set the font for future calls.
 *
 * @param pfont The font to use.  If NULL, the font is not changed.
 * @return The old font.
 */
PMWFONT
GdSetFont(PMWFONT pfont)
{
	PMWFONT	oldfont = gr_pfont;

	if (pfont)
		gr_pfont = pfont;
	return oldfont;
}

/**
 * Select a font, based on various parameters.
 * If plogfont is specified, name and height parms are ignored
 * and instead used from MWLOGFONT.
 * 
 * If height is 0, match based on passed name, trying
 * builtins first for speed, then other font renderers.
 * If not found, return 0.  If height=0 is used for
 * scalable font renderers, a call to GdSetFontSize with
 * requested height must occur before rendering.
 *
 * If height not 0, perform same match based on name,
 * but return builtin font best match from height if
 * not otherwise found.
 *
 * @param psd      The screen that this font will be displayed on.
 * @param name     The name of the font.  May be NULL.  Ignored if
 *                 plogfont is specified.
 * @param height   The height of the font in pixels.  Ignored if
 *                 plogfont is specified.
 * @param plogfont A structure describing the font, or NULL.
 * @return         A new font, or NULL on error.
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
	const char *	fontname;
#if !FONTMAPPER
	char 		fontmapper_fontname[MWLF_FACESIZE + 1];
#endif

	GdGetScreenInfo(psd, &scrinfo);

	/* if plogfont not specified, use passed name, height and any class*/
	if (!plogfont) {
		/* if name not specified, use first builtin*/
		if (!name || !name[0])
			name = pf->name;
		fontname = name;
		fontclass = MWLF_CLASS_ANY;
	} else {
		/* otherwise, use MWLOGFONT name, height and class*/
#if FONTMAPPER
		fontname = NULL; /* Paranoia */
 		fontclass = select_font(plogfont, &fontname);
#else
		/* Copy the name from plogfont->lfFaceName to fontmapper_fontname
		 * Note that it may not be NUL terminated in the source string,
		 * so we're careful to NUL terminate it here.
		 */
		strncpy(fontmapper_fontname, plogfont->lfFaceName, MWLF_FACESIZE);
		fontmapper_fontname[MWLF_FACESIZE] = '\0';
		fontname = fontmapper_fontname;
		if (!fontname[0])	/* if name not specified, use first builtin*/
			fontname = pf->name;
		fontclass = plogfont->lfClass;
#endif
		height = plogfont->lfHeight;
		if (plogfont->lfUnderline)
			fontattr = MWTF_UNDERLINE;
	}
	height = abs(height);

	/* check builtin fonts first for speed*/
 	if (!height && (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_BUILTIN)) {
  		for(i = 0; i < scrinfo.fonts; ++i) {
 			if(!strcmpi(pf[i].name, fontname)) {
  				pf[i].fontsize = pf[i].cfont->height;
				pf[i].fontattr = fontattr;
DPRINTF("createfont: (height == 0) found builtin font %s (%d)\n", fontname, i);
  				return (PMWFONT)&pf[i];
  			}
  		}
		/* 
		 * Specified height=0 and no builtin font matched name.
		 * if not font found with other renderer, no font
		 * will be loaded, and 0 returned.
		 *
		 * We used to return the first builtin font.  If a font
		 * return needs to be guaranteed, specify a non-zero
		 * height, and the closest builtin font to the specified
		 * height will always be returned.
		 */
  	}

	/* try to load font (regardless of height) using other renderers*/

#ifdef HAVE_FNT_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_FNT) {
		pfont = (PMWFONT) fnt_createfont(fontname, height, fontattr);
		if (pfont) {
			DPRINTF("fnt_createfont: using font %s\n", fontname);
			return(pfont);
		}
		EPRINTF("fnt_createfont: %s,%d not found\n", fontname, height);
	}
#endif

#ifdef HAVE_PCF_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_PCF) {
		pfont = (PMWFONT) pcf_createfont(fontname, height, fontattr);
		if (pfont) {
			DPRINTF("pcf_createfont: using font %s\n", fontname);
			return(pfont);
		}
		EPRINTF("pcf_createfont: %s,%d not found\n", fontname, height);
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
 			EPRINTF("freetype_createfont: %s,%d not found\n",
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
			EPRINTF("freetype2_createfont: %s,%d not found\n",
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
			EPRINTF("t1lib_createfont: %s,%d not found\n",
				fontname, height);
		}
  	}
#endif

#if HAVE_HZK_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_HZK) {
		/* Make sure the library is initialized */
		if (hzk_init(psd)) {
			pfont = (PMWFONT)hzk_createfont(fontname, height, fontattr);
			if(pfont)		
				return pfont;
			EPRINTF("hzk_createfont: %s,%d not found\n", fontname, height);
		}
	}
#endif

#if HAVE_EUCJP_SUPPORT
 	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_MGL) {
		pfont = (PMWFONT)eucjp_createfont(fontname, height, fontattr);
		if (pfont) {
			DPRINTF("eujcp_createfont: using font %s\n", fontname);
			return pfont;
		}
		EPRINTF("eucjp_createfont: %s,%d not found\n", fontname, height);
	}
#endif

	/*
	 * No font yet found.  If height specified, we'll return
	 * a builtin font.  Otherwise 0 will be returned.
	 */
 	if (height != 0 && (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_BUILTIN)) {
		/* find builtin font closest in height*/
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
DPRINTF("createfont: (height != 0) using builtin font %s (%d)\n", pf[fontno].name, fontno);
		return (PMWFONT)&pf[fontno];
	}

	/* no font matched: don't load font, return 0*/
DPRINTF("createfont: no font found, returning NULL\n");
	return 0;
}

/**
 * Set the size of a font.
 *
 * @param pfont    The font to modify.
 * @param fontsize The new size.
 * @return         The old size.
 */
MWCOORD
GdSetFontSize(PMWFONT pfont, MWCOORD fontsize)
{
	MWCOORD oldfontsize = pfont->fontsize;
	pfont->fontsize = fontsize;

	if (pfont->fontprocs->SetFontSize)
	    pfont->fontprocs->SetFontSize(pfont, fontsize);

	return oldfontsize;
}

/**
 * Set the rotation angle of a font.
 *
 * @param pfont        The font to modify.
 * @param tenthdegrees The new rotation angle, in tenths of degrees.
 * @return             The old rotation angle, in tenths of degrees.
 */
int
GdSetFontRotation(PMWFONT pfont, int tenthdegrees)
{
	MWCOORD oldrotation = pfont->fontrotation;
	pfont->fontrotation = tenthdegrees;

	if (pfont->fontprocs->SetFontRotation)
	    pfont->fontprocs->SetFontRotation(pfont, tenthdegrees);
	
	return oldrotation;
}

/**
 * Set/reset font attributes (MWTF_KERNING, MWTF_ANTIALIAS)
 * for a font.
 *
 * @param pfont    The font to modify.
 * @param setflags The flags to set.  Overrides clrflags.
 * @param clrflags The flags to clear.
 * @return         The old font attributes.
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

/**
 * Unload and deallocate a font.  Do not use the font once it has been
 * destroyed.
 *
 * @param pfont The font to deallocate.
 */
void
GdDestroyFont(PMWFONT pfont)
{
	if (pfont->fontprocs->DestroyFont)
		pfont->fontprocs->DestroyFont(pfont);
}

/**
 * Return information about a specified font.
 *
 * @param pfont The font to query.
 * @param pfontinfo Recieves the result of the query
 * @return TRUE on success, FALSE on error.
 */
MWBOOL
GdGetFontInfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	if(!pfont || !pfont->fontprocs->GetFontInfo)
		return FALSE;

	return pfont->fontprocs->GetFontInfo(pfont, pfontinfo);
}

/**
 * Draws text onto a drawing surface (e.g. the screen or a double-buffer).
 * Uses the current font, current foreground color, and possibly the
 * current background color.  Applies clipping if necessary.
 * The background color is only drawn if the gr_usebg flag is set.
 *
 * @param psd   The destination drawing surface.  Non-NULL.
 * @param x     The X co-ordinate to draw the text.
 * @param y     The Y co-ordinate to draw the text.  The flags specify
 *              whether this is the top (MWTF_TOP), bottom (MWTF_BOTTOM),
 *              or baseline (MWTF_BASELINE) of the text.
 * @param str   The string to display.  Non-NULL.
 * @param cc    The length of str.  For Asian DBCS encodings, this is
 *              specified in bytes.  For all other encodings such as ASCII,
 *              UTF8 and UC16, it is specified in characters.  For ASCII
 *              and DBCS encodings, this may be set to -1, and the length
 *              will be calculated automatically.
 * @param flags Flags specifying the encoding of str and the position of the
 *              text.  Specifying the vertical position is mandatory.
 *              The encoding of str defaults to ASCII if not specified.
 */
void
GdText(PSD psd, MWCOORD x, MWCOORD y, const void *str, int cc,MWTEXTFLAGS flags)
{
	const void *	text;
	int		defencoding = gr_pfont->fontprocs->encoding;
	int		force_uc16 = 0;
	unsigned long	buf[256];

	/*
	 * DBCS encoding is handled a little special: if the selected
	 * font is a builtin, then we'll force a conversion to UC16
	 * rather than converting to the renderer specification.  This is
	 * because we allow DBCS-encoded strings to draw using the
	 * specially-compiled-in font if the character is not ASCII.
	 * This is specially handled in corefont_drawtext below.
	 *
	 * If the font is not builtin, then the drawtext routine must handle
	 * all glyph output, including ASCII.
	 */
	if (flags & MWTF_DBCSMASK) {
		/* force double-byte sequences to UC16 if builtin font only*/
		if (gr_pfont->fontprocs->GetTextBits == gen_gettextbits &&
		    gr_pfont->fontprocs->DrawText == corefont_drawtext) {
			defencoding = MWTF_UC16;
			force_uc16 = 1;
		}
	}

	/* convert encoding if required*/
	if((flags & (MWTF_PACKMASK|MWTF_DBCSMASK)) != defencoding) {
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK;	/* keep DBCS bits for drawtext*/
		flags |= defencoding;
		text = buf;
	} else text = str;

	/* use strlen for char count when ascii or dbcs*/
	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

	if(cc <= 0 || !gr_pfont->fontprocs->DrawText)
		return;

	/* draw text string, DBCS flags may still be set*/
	if (!force_uc16)	/* remove DBCS flags if not needed*/
		flags &= ~MWTF_DBCSMASK;
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
	const unsigned short *istr = text;
	MWCOORD		width;			/* width of text area */
	MWCOORD 	height;			/* height of text area */
	MWCOORD		base;			/* baseline of text*/
	MWCOORD		startx, starty;
	const MWIMAGEBITS *bitmap;		/* bitmap for characters */
	MWBOOL		bgstate;
	int		clip;

	if (flags & MWTF_DBCSMASK)
		dbcs_gettextsize(pfont, istr, cc, flags, &width, &height, &base);
	else pfont->fontprocs->GetTextSize(pfont, str, cc, flags, &width, &height, &base);
	
	if (flags & MWTF_BASELINE)
		y -= base;
	else if (flags & MWTF_BOTTOM)
		y -= (height - 1);
	startx = x;
	starty = y + base;
	bgstate = gr_usebg;

	switch (clip = GdClipArea(psd, x, y, x + width - 1, y + height - 1)) {
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

	/*
	 * If the string was marked as DBCS, then we've forced the conversion
	 * to UC16 in GdText.  Here we special-case the non-ASCII values and
	 * get the bitmaps from the specially-compiled-in font.  Otherwise,
	 * we draw them using the normal pfont->fontprocs->GetTextBits.
	 */
	while (--cc >= 0 && x < psd->xvirtres) {
		if (flags & MWTF_DBCSMASK)
			dbcs_gettextbits(pfont, *istr++, flags, &bitmap, &width,
				&height, &base);
		else {
			int ch;

			if (pfont->fontprocs->encoding == MWTF_UC16)
				ch = *istr++;
			else ch = *str++;
			pfont->fontprocs->GetTextBits(pfont, ch, &bitmap, &width,
				&height, &base);
		}

		if (clip == CLIP_VISIBLE)
			drawbitmap(psd, x, y, width, height, bitmap);
		else
			GdBitmap(psd, x, y, width, height, bitmap);
		x += width;
	}

	if (pfont->fontattr & MWTF_UNDERLINE)
		GdLine(psd, startx, starty, x, starty, FALSE);

	/* restore background draw state*/
	gr_usebg = bgstate;

	GdFixCursor(psd);
}

#if HAVE_FNT_SUPPORT | HAVE_PCF_SUPPORT
/*
 * Draw MWTF_UC16 text using COREFONT type font.
 */
void
gen16_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
	const void *text, int cc, MWTEXTFLAGS flags)
{
	const unsigned short *str = text;
	MWCOORD		width;			/* width of text area */
	MWCOORD		height;			/* height of text area */
	MWCOORD		base;			/* baseline of text */
	MWCOORD		startx, starty;
	const MWIMAGEBITS *bitmap;		/* bitmap for characters */
	MWBOOL		bgstate;
	int		clip;

	pfont->fontprocs->GetTextSize(pfont, str, cc, flags, &width, &height, &base);

	if (flags & MWTF_BASELINE)
		y -= base;
	else if (flags & MWTF_BOTTOM)
		y -= (height - 1);
	startx = x;
	starty = y + base;
	bgstate = gr_usebg;

	switch (clip = GdClipArea(psd, x, y, x + width - 1, y + height - 1)) {
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
	 * them using clipping for each one.
	 */
	while (--cc >= 0 && x < psd->xvirtres) {
		unsigned int ch = *str++;
		pfont->fontprocs->GetTextBits(pfont, ch, &bitmap, &width,
			&height, &base);

		if (clip == CLIP_VISIBLE)
			drawbitmap(psd, x, y, width, height, bitmap);
		else
			GdBitmap(psd, x, y, width, height, bitmap);
		x += width;
	}

	if (pfont->fontattr & MWTF_UNDERLINE)
		GdLine(psd, startx, starty, x, starty, FALSE);

	/* restore background draw state*/
	gr_usebg = bgstate;

	GdFixCursor(psd);
}
#endif /* HAVE_FNT_SUPPORT | HAVE_PCF_SUPPORT*/

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

/**
 * Convert text from one encoding to another.
 * Input cc and returned cc is character count, not bytes.
 * Return < 0 on error or can't translate.
 *
 * @param istr   Input string.
 * @param iflags Encoding of istr, as MWTF_xxx constants.
 * @param cc     The length of istr.  For Asian DBCS encodings, this is
 *               specified in bytes.  For all other encodings such as ASCII,
 *               UTF8 and UC16, it is specified in characters.  For ASCII
 *               and DBCS encodings, this may be set to -1, and the length
 *               will be calculated automatically.
 * @param ostr   Output string.
 * @param oflags Encoding of ostr, as MWTF_xxx constants.
 * @return       Number of characters (not bytes) converted.
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

	iflags &= MWTF_PACKMASK|MWTF_DBCSMASK;
	oflags &= MWTF_PACKMASK|MWTF_DBCSMASK;

	/* allow -1 for len with ascii or dbcs*/
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
	cc = 0;
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
			break;
		case MWTF_DBCS_BIG5:	/* Chinese BIG5*/
			ch = *istr8++;
			if (ch >= 0xA1 && ch <= 0xF9 && icc &&
			    ((*istr8 >= 0x40 && *istr8 <= 0x7E) ||
			     (*istr8 >= 0xA1 && *istr8 <= 0xFE))) {
				ch = (ch << 8) | *istr8++;
				--icc;
			}
			break;
		case MWTF_DBCS_EUCCN:	/* Chinese EUCCN (GB2312+0x80)*/
			ch = *istr8++;
			if (ch >= 0xA1 && ch <= 0xF7 && icc &&
			    *istr8 >= 0xA1 && *istr8 <= 0xFE) {
				ch = (ch << 8) | *istr8++;
				--icc;
			}
			break;
		case MWTF_DBCS_EUCKR:	/* Korean EUCKR (KSC5601+0x80)*/
			ch = *istr8++;
			if (ch >= 0xA1 && ch <= 0xFE && icc &&
			    *istr8 >= 0xA1 && *istr8 <= 0xFE) {
				ch = (ch << 8) | *istr8++;
				--icc;
			}
			break;
		case MWTF_DBCS_EUCJP:	/* Japanese EUCJP*/
			ch = *istr8++;
			if (ch >= 0xA1 && ch <= 0xFE && icc &&
			    *istr8 >= 0xA1 && *istr8 <= 0xFE) {
				ch = (ch << 8) | *istr8++;
				--icc;
			}
			break;
		case MWTF_DBCS_JIS:	/* Japanese JISX0213*/
			ch = *istr8++;
			if (icc && (
			    (ch >= 0xA1 && ch <= 0xFE && *istr8 >= 0xA1 && *istr8 <= 0xFE)
			    ||
			    (((ch >= 0x81 && ch <= 0x9F) || (ch >= 0xE0 && ch <= 0xEF)) &&
			     (*istr8 >= 0x40 && *istr8 <= 0xFC && *istr8 != 0x7F))
			            )) {
					ch = (ch << 8) | *istr8++;
					--icc;
			}

			break;
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
			break;
		}
		++cc;
	}
	return cc;
}

/**
 * Gets the size of some text in a specified font.
 *
 * @param pfont   The font to measure.  Non-NULL.
 * @param str     The string to measure.  Non-NULL.
 * @param cc      The length of str.  For Asian DBCS encodings, this is
 *                specified in bytes.  For all other encodings such as ASCII,
 *                UTF8 and UC16, it is specified in characters.  For ASCII
 *                and DBCS encodings, this may be set to -1, and the length
 *                will be calculated automatically.
 * @param pwidth  On return, holds the width of the text.
 * @param pheight On return, holds the height of the text.
 * @param pbase   On return, holds the baseline of the text.
 * @param flags   Flags specifying the encoding of str and the position of the
 *                text.  Specifying the vertical position is mandatory.
 *                The encoding of str defaults to ASCII if not specified.
 */
void
GdGetTextSize(PMWFONT pfont, const void *str, int cc, MWCOORD *pwidth,
	MWCOORD *pheight, MWCOORD *pbase, MWTEXTFLAGS flags)
{
	const void *	text;
	MWTEXTFLAGS	defencoding = pfont->fontprocs->encoding;
	int		force_uc16 = 0;
	unsigned long	buf[256];

	/* DBCS handled specially: see comment in GdText*/
	if (flags & MWTF_DBCSMASK) {
		/* force double-byte sequences to UC16 if builtin font only*/
		if (pfont->fontprocs->GetTextBits == gen_gettextbits &&
		    pfont->fontprocs->DrawText == corefont_drawtext) {
			defencoding = MWTF_UC16;
			force_uc16 = 1;
		}
	}

	/* convert encoding if required*/
	if((flags & (MWTF_PACKMASK|MWTF_DBCSMASK)) != defencoding) {
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK; /* keep DBCS bits for gettextsize*/
		flags |= defencoding;
		text = buf;
	} else text = str;

	/* use strlen for char count when ascii or dbcs*/
	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

	if(cc <= 0 || !pfont->fontprocs->GetTextSize) {
		*pwidth = *pheight = *pbase = 0;
		return;
	}

	/* calc height and width of string*/
	if (force_uc16)		/* if UC16 conversion forced, string is DBCS*/
		dbcs_gettextsize(pfont, text, cc, flags, pwidth, pheight, pbase);
	else pfont->fontprocs->GetTextSize(pfont, text, cc, flags, pwidth, pheight, pbase);
}

#if HAVE_FREETYPE_2_SUPPORT
/**
 * Create a new font, from a buffer.
 *
 * @param psd    Drawing surface.
 * @param buffer The data to create the font from.  This should be an
 *               in-memory copy of a font file.
 * @param length The length of the buffer, in bytes.
 * @param format Buffer format, or NULL or "" to auto-detect.
 *               Currently unused, since only FreeType 2 fonts are
 *               currently supported, and FreeType 2 always
 *               autodetects.
 * @param height The font height in pixels.
 * @return       New font, or NULL on error.
 */
PMWFONT
GdCreateFontFromBuffer(PSD psd, const unsigned char *buffer,
		       unsigned length, const char *format, MWCOORD height)
{
	PMWFONT pfont = NULL;

	//assert(buffer);

	/* EPRINTF("Nano-X: Font magic = '%c%c%c%c' @ GdCreateFontFromBuffer\n",
	 * (char) buffer[0], (char) buffer[1], (char) buffer[2], (char) buffer[3]);
	 */

	/*
	 * If we had multiple font drivers, we'd have to do select one
	 * based on 'format' here.  (Suggestion: 'format' is the file
	 * extension - e.g. TTF, PFR, ...)
	 */

	if (freetype2_init(psd)) {
		pfont = (PMWFONT)freetype2_createfontfrombuffer(buffer, length, height);
	}
	if (!pfont)
		EPRINTF("GdCreateFontFromBuffer: create failed.\n");

	return pfont;
}

/**
 * Create a new font, which is a copy of an old font.
 *
 * @param psd      Drawing surface.
 * @param psrcfont Font to copy from.
 * @param fontsize Size of new font, or 0 for unchanged.
 * @return         New font.
 */
PMWFONT
GdDuplicateFont(PSD psd, PMWFONT psrcfont, MWCOORD fontsize)
{
	//assert(psd);
	//assert(psrcfont);

	if (psrcfont->fontprocs->Duplicate)
		return psrcfont->fontprocs->Duplicate(psrcfont, fontsize);

	return psrcfont;
}
#endif /*HAVE_FREETYPE_2_SUPPORT*/

/**
 * UTF-8 to UTF-16 conversion.  Surrogates are handeled properly, e.g.
 * a single 4-byte UTF-8 character is encoded into a surrogate pair.
 * On the other hand, if the UTF-8 string contains surrogate values, this
 * is considered an error and returned as such.
 *
 * The destination array must be able to hold as many Unicode-16 characters
 * as there are ASCII characters in the UTF-8 string.  This in case all UTF-8
 * characters are ASCII characters.  No more will be needed.
 *
 * This function will also accept Java's variant of UTF-8.  This encodes
 * U+0000 as two characters rather than one, so the UTF-8 does not contain
 * any zeroes.
 *
 * @author Copyright (c) 2000 Morten Rolland, Screen Media
 *
 * @param utf8      Input string in UTF8 format.
 * @param cc        Number of bytes to convert.
 * @param unicode16 Destination buffer.
 * @return          Number of characters converted, or -1 if input is not
 *                  valid UTF8.
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
			if ( (scalar != 0) && (scalar < 0x80) )
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
