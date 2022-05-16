/*
 * Copyright (c) 2000, 2002, 2003, 2005 Greg Haerr <greg@censoft.com>
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
#include <stdlib.h>
#include <string.h>
#include "uni_std.h"
#include "device.h"
#include "devfont.h"
#include "genfont.h"
#include "swap.h"
#include "intl.h"

#define DEBUG_TEXT_SHAPING	0

/* temp extern decls*/
extern MWCOREFONT *user_builtin_fonts;

static int utf8_to_utf16(const unsigned char *utf8, int cc, unsigned short *unicode16);
int uc16_to_utf8(const unsigned short *us, int cc, unsigned char *s);

#if HAVE_FILEIO
#include <stdio.h>
/*
 * Try finding filename in default font path, must contain one of
 * extensions (separated by |).  Returns full pathname if found.
 */
char *
mwfont_findpath(const char *filename, const char *defpath, const char *extension)
{
    char *ext;
    static char fullpath[128];

    DPRINTF("mwfont_findpath: check %s (%s) in %s\n", filename, extension, defpath);
    /* check allowed extensions */
    strcpy(fullpath, extension);
    for (ext=fullpath; ;) {
        char *p2 = strchr(ext, '|');
        if (p2) *p2 = '\0';
        if (strstr(filename, ext))
            goto out;
        if (p2) {
            *p2 = '|';
            ext = p2 + 1;
            if (!*ext)
                return NULL;
       } else return NULL;
    }
out:
    if (strchr(filename, '/') != NULL)
        strcpy(fullpath, filename);
    else {
        char *env = getenv("MWFONTDIR");
        sprintf(fullpath, "%s/%s", env? env: defpath, filename);
    }
    if (access(fullpath, R_OK) != 0)
        return NULL;
    DPRINTF("found: %s\n", fullpath);
    return fullpath;
}
#endif

/* check if passed fontname is aliased in mwfonts.alias file */
char *
mwfont_findalias(const char *fontname, int *height, int *width)
{
#if HAVE_FILEIO
    FILE *afp;
    char *p, *size;
    int h = 13;
    static char buf[80];

    if (!fontname)
        return NULL;
    if (*fontname == '/')       /* don't translate NX11 fonts with absolute path */
        return (char *)fontname;
    sprintf(buf, "%s/%s", MW_FONT_DIR, MWFONTSALIAS);
    afp = fopen(buf, "r");
    if (afp) {
        for (;;) {
            if (!fgets(buf, sizeof(buf), afp))
                break;
            buf[strlen(buf) - 1] = '\0';

            /* ignore blank and ! comments*/
            if (buf[0] == '\0' || buf[0] == '!')
                continue;

            /* fontname is first space separated field*/
            /* check for tab first as filename may have spaces*/
            p = strchr(buf, '\t');
            if (!p)
                p = strchr(buf, ' ');
            if (!p)
                continue;
            *p = '\0';

            if (strcmp(fontname, buf) == 0) {
                /* alias is second space separated field*/
                do ++p; while (*p == ' ' || *p == '\t');

                size = strchr(p, ',');
                if (size) {
                    *size++ = '\0';
                    h = atoi(size);
                }
                if (!*height)
                    *height = *width = h;
                DPRINTF("mwfont_findalias: %s -> %s,%d\n", fontname, p, *height);
                fclose(afp);
                return p;
            }
        }
        fclose(afp);
    }
#endif
    return (char *)fontname;
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
 * @param width    The width of the font in pixels.  Ignored if
 *                 plogfont is specified.
 * @param plogfont A structure describing the font, or NULL.
 * @return         A new font, or NULL on error.
 */
PMWFONT
GdCreateFont(PSD psd, const char *name, MWCOORD height, MWCOORD width, const PMWLOGFONT plogfont)
{
	int 		i;
	int		fontht;
	int		fontno;
 	int		fontclass;
	int		fontattr = 0;
	PMWFONT		pfont;
	PMWCOREFONT	pf = psd->builtin_fonts;
	PMWCOREFONT	upf;
	MWFONTINFO	fontinfo;
	MWSCREENINFO 	scrinfo;
	const char *	fontname;
#if !FONTMAPPER
	char 		fontmapper_fontname[MWLF_FACESIZE + 1];
#endif

	DPRINTF("GdCreateFont %s, %d,%d\n", name, height, width);

	/* check mwfonts.alias file for alias */
	name = mwfont_findalias(name, &height, &width);

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
		width = plogfont->lfWidth;
		if (plogfont->lfUnderline)
			fontattr = MWTF_UNDERLINE;
	}
	height = MWABS(height);		/* FIXME win32 height < 0 specifies character height not cell height*/

	/* check builtin fonts first for speed*/
 	if (!height && (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_BUILTIN)) {
  		for(i = 0; i < scrinfo.fonts; ++i) {
 			if(!strcasecmp(pf[i].name, fontname)) {
  				pf[i].fontsize = pf[i].cfont->height;
				pf[i].fontattr = fontattr;
				//DPRINTF("createfont: (height == 0) found builtin font %s (%d)\n", fontname, i);
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
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("builtin-createfont: %s,%d not found\n",
				fontname, height);
  	}

	/* check user builtin fonts next*/
	upf = user_builtin_fonts;
	while ( (upf != NULL) && (upf->name != NULL) ) {
		if(!strcasecmp(upf->name, fontname) && (upf->cfont->height == height) ) {
			if( upf->fontprocs == NULL )
				gen_setfontproc(upf);
			upf->fontsize = upf->cfont->height;
			upf->fontattr = fontattr;
			DPRINTF("createfont: (height != 0) found user builtin font %s (%d)\n", fontname, height);
			return (PMWFONT)upf;
		}
		upf++;
	}

	/* try to load font (regardless of height) using other renderers*/

#if HAVE_FNT_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_FNT) {
		pfont = (PMWFONT)fnt_createfont(fontname, height, width, fontattr);
		if (pfont) {
			DPRINTF("fnt_createfont: using font %s\n", fontname);
			return pfont;
		}
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("fnt_createfont: %s,%d not found\n", fontname, height);
	}
#endif

#if HAVE_PCF_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_PCF) {
		pfont = (PMWFONT)pcf_createfont(fontname, height, width, fontattr);
		if (pfont) {
			DPRINTF("pcf_createfont: using font %s\n", fontname);
			return pfont;
		}
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("pcf_createfont: %s,%d not found\n", fontname, height);
	}
#endif

#if HAVE_FREETYPE_2_SUPPORT
 	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_FREETYPE) {
		/* FIXME auto antialias for height > 14 for kaffe*/
		if (plogfont && MWABS(plogfont->lfHeight) > FT_MINAA_HEIGHT && plogfont->lfQuality)
				fontattr |= MWTF_ANTIALIAS;

		pfont = (PMWFONT)freetype2_createfont(fontname, height, width, fontattr);
		if(pfont) {
			DPRINTF("freetype_createfont: using font %s\n", fontname);
			/* FIXME kaffe kluge*/
			pfont->fontattr |= MWTF_FREETYPE;
			return pfont;
		}
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("freetype2_createfont: %s,%d not found\n", fontname, height);
  	}
#endif

#if HAVE_T1LIB_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_T1LIB) {
		pfont = (PMWFONT)t1lib_createfont(fontname, height, width, fontattr);
		if(pfont)
			return pfont;
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("t1lib_createfont: %s,%d not found\n", fontname, height);
  	}
#endif

#if HAVE_HZK_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_HZK) {
		pfont = (PMWFONT)hzk_createfont(fontname, height, width, fontattr);
		if(pfont)		
			return pfont;
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("hzk_createfont: %s,%d not found\n", fontname, height);
	}
#endif

#if HAVE_HBF_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_HZK) {
		pfont = (PMWFONT)hbf_createfont(fontname, height, width, fontattr);
		if(pfont)
			return pfont;
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("hbf_createfont: %s,%d not found\n", fontname, height);
	}
#endif

#if HAVE_EUCJP_SUPPORT
 	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_MGL) {
		pfont = (PMWFONT)eucjp_createfont(fontname, height, width, fontattr);
		if (pfont) {
			DPRINTF("eujcp_createfont: using font %s\n", fontname);
			return pfont;
		}
		if (fontclass != MWLF_CLASS_ANY)
			EPRINTF("eucjp_createfont: %s,%d not found\n", fontname, height);
	}
#endif

	if (fontclass == MWLF_CLASS_ANY) {
		EPRINTF("createfont: %s,%d not found\n", fontname, height);
		EPRINTF("  (tried "
			"builtin_createfont"
#if HAVE_FNT_SUPPORT
			", fnt_createfont"
#endif
#if HAVE_PCF_SUPPORT
			", pcf_createfont"
#endif
#if HAVE_FREETYPE_2_SUPPORT
			", freetype2_createfont"
#endif
#if HAVE_T1LIB_SUPPORT
			", t1lib_createfont"
#endif
#if HAVE_HZK_SUPPORT
			", hzk_createfont"
#endif
#if HAVE_HBF_SUPPORT
			", hbf_createfont"
#endif
#if HAVE_EUCJP_SUPPORT
			", eujcp_createfont"
#endif
			")\n");
	}

	/*
	 * No font yet found.  If the height was specified, we'll return the
	 * most close builtin font as a fallback.  Otherwise 0 will be returned.
	 */
 	if (height != 0 && (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_BUILTIN)) {
		/* find builtin font closest in height*/
		fontno = 0;
		height = MWABS(height);
		fontht = MAX_MWCOORD;
		for(i = 0; i < scrinfo.fonts; ++i) {
			pfont = (PMWFONT)&pf[i];
			GdGetFontInfo(pfont, &fontinfo);
			if(fontht > MWABS(height-fontinfo.height)) { 
				fontno = i;
				fontht = MWABS(height-fontinfo.height);
			}
		}
		pf[fontno].fontsize = pf[fontno].cfont->height;
		pf[fontno].fontattr = fontattr;
		EPRINTF("createfont: height given, using builtin font %s (%d) as fallback\n", pf[fontno].name, fontno);
		return (PMWFONT)&pf[fontno];
	}

	/* no font found: don't load any font and return 0*/
	EPRINTF("createfont: no height given, fallback search impossible, returning NULL\n");
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
GdSetFontSize(PMWFONT pfont, MWCOORD height, MWCOORD width)
{
	if (pfont->fontprocs->SetFontSize)
	    return pfont->fontprocs->SetFontSize(pfont, height, width);

	return 0;
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
	if (pfont->fontprocs->SetFontAttr)
	    return pfont->fontprocs->SetFontAttr(pfont, setflags, clrflags);
	
	return 0;
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
GdText(PSD psd, PMWFONT pfont, MWCOORD x, MWCOORD y, const void *str, int cc,MWTEXTFLAGS flags)
{
	const void *	text;
	MWTEXTFLAGS	defencoding = pfont->fontprocs->encoding;
	uint32_t *buf = NULL;

	int		force_uc16 = 0;
#if MW_FEATURE_INTL
	/*
	 * DBCS encoding is handled a little special: if the selected
	 * font is a builtin, then we'll force a conversion to UC16
	 * rather than converting to the renderer specification.  This is
	 * because we allow DBCS-encoded strings to draw using the
	 * specially-compiled-in font if the character is not ASCII.
	 * This is specially handled in gen_drawtext below.
	 *
	 * If the font is not builtin, then the drawtext routine must handle
	 * all glyph output, including ASCII.
	 */
	if (flags & MWTF_DBCSMASK) {
		/* force double-byte sequences to UC16 if builtin font only*/
		if (pfont->fontprocs == &mwfontprocs) {
			defencoding = MWTF_UC16;
			force_uc16 = 1;
		}
	}
#endif

	/* use strlen for char count when ascii or dbcs*/
	if(cc == -1) {
		MWTEXTFLAGS inflag = flags & MWTF_PACKMASK;
		if (inflag == MWTF_ASCII || inflag == MWTF_UTF8)
			cc = strlen((char *)str);
		else DPRINTF("GdText: Bad cc argument\n");
	}

	/* convert encoding if required*/
	if((flags & (MWTF_PACKMASK|MWTF_DBCSMASK)) != defencoding) {
		/* allocate enough for output string utf8/uc32 is max 4 bytes, uc16 max 2*/
		buf = ALLOCA(cc * 4);
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK;	/* keep DBCS bits for drawtext*/
		flags |= defencoding;
		text = buf;
	} else
		text = str;

	if(cc <= 0 || !pfont->fontprocs->DrawText) {
		if (buf)
			FREEA(buf);
		return;
	}

	/* draw text string, DBCS flags may still be set*/
#if HAVE_KSC5601_SUPPORT
	//if (flags & MWTF_DBCS_EUCKR)
		//;
	//else
#endif
	if (!force_uc16)	/* remove DBCS flags if not needed*/
		flags &= ~MWTF_DBCSMASK;
	pfont->fontprocs->DrawText(pfont, psd, x, y, text, cc, flags);

	if (buf)
		FREEA(buf);
}

/*
 * Draw ASCII or MWTF_UC16 text using COREFONT type font (buitin, PCF, FNT)
 */
void
gen_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
	const void *text, int cc, MWTEXTFLAGS flags)
{
	const unsigned char *str = text;
	const unsigned short *istr = text;
	MWCOORD		width;			/* width of text area */
	MWCOORD 	height;			/* height of text area */
	MWCOORD		base;			/* baseline of text*/
	MWCOORD		startx, starty;
	const MWIMAGEBITS *bitmap;		/* bitmap for characters */
	MWBOOL		bgstate = gr_usebg;
	int		clip;
	MWBLITFUNC convblit;
	MWBLITPARMS parms;

	/* fill in unchanging convblit parms*/
	parms.op = MWROP_COPY;					/* copy to dst, 1=fg (0=bg if usebg)*/
	parms.data_format = MWIF_MONOWORDMSB;	/* data is 1bpp words, msb first*/
	parms.fg_colorval = gr_foreground_rgb;
	parms.bg_colorval = gr_background_rgb;
	parms.fg_pixelval = gr_foreground;		/* for palette mask convblit*/
	parms.bg_pixelval = gr_background;
	parms.usebg = gr_usebg;
	parms.srcx = 0;
	parms.srcy = 0;
	parms.dst_pitch = psd->pitch;			/* usually set in GdConversionBlit*/
	parms.data_out = psd->addr;
	parms.srcpsd = NULL;
	convblit = GdFindConvBlit(psd, MWIF_MONOWORDMSB, MWROP_COPY);

#if MW_FEATURE_INTL
	if (flags & MWTF_DBCSMASK)
		dbcs_gettextsize(pfont, istr, cc, flags, &width, &height, &base);
	else
#endif
		pfont->fontprocs->GetTextSize(pfont, str, cc, flags, &width, &height, &base);

	/* return if nothing to draw*/
	if (width == 0 || height == 0)
		return;
	
	if (flags & MWTF_BASELINE)
		y -= base;
	else if (flags & MWTF_BOTTOM)
		y -= (height - 1);
	startx = x;
	starty = y + base;

	/* pre-clip entire text area for speed*/
	switch (clip = GdClipArea(psd, x, y, x + width - 1, y + height - 1)) {
	case CLIP_VISIBLE:
		/* fast clear background once for all characters if drawing point by point*/
		if (!convblit && gr_usebg) {
			psd->FillRect(psd, x, y, x + width - 1, y + height - 1, gr_background);
			gr_usebg = FALSE;
		}
		break;

	case CLIP_INVISIBLE:
		return;
	}

	/*
	 * Get the bitmap for each character individually, and then display
	 * them possibly using clipping for each one.
	 */
	while (--cc >= 0 && x < psd->xvirtres) {
		/*
	 	 * If the string was marked as DBCS, then we've forced the conversion
	 	 * to UC16 in GdText.  Here we special-case the non-ASCII values and
	 	 * get the bitmaps from the specially-compiled-in font.  Otherwise,
	 	 * we draw them using the normal pfont->fontprocs->GetTextBits.
	 	 */
#if MW_FEATURE_INTL
		if (flags & MWTF_DBCSMASK)
			dbcs_gettextbits(pfont, *istr++, flags, &bitmap, &width, &height, &base);
		else
#endif
		{
			int ch;

			if (pfont->fontprocs->encoding == MWTF_UC16)
				ch = *istr++;
			else ch = *str++;
			pfont->fontprocs->GetTextBits(pfont, ch, &bitmap, &width, &height, &base);
		}

		/* check bad return from GetTextBits*/
		if (width == 0 || height == 0)
			continue;

		/* use fast blit for text draw, fallback draw point-by-point*/
		if (convblit) {
			parms.dstx = x;
			parms.dsty = y;
			parms.height = height;
			parms.width = width;
			parms.src_pitch = ((width + 15) >> 4) << 1;	/* pad to WORD boundary*/
			parms.data = (char *)bitmap;
			/* skip clipping checks if fully visible*/
			if (clip == CLIP_VISIBLE)
				convblit(psd, &parms);
			else
				GdConversionBlit(psd, &parms);
		}
#if !SWIEROS
		else
			GdBitmapByPoint(psd, x, y, width, height, bitmap, clip);
#endif
		x += width;
	}

	if (pfont->fontattr & MWTF_UNDERLINE)
		GdLine(psd, startx, starty, x, starty, FALSE);

	/* restore background draw state*/
	gr_usebg = bgstate;

	GdFixCursor(psd);
}

/* null routines (need porting from freetype 1)*/
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
	const uint32_t	*istr32;
	unsigned char 		*ostr8;
	unsigned short 		*ostr16;
	uint32_t		*ostr32;
	unsigned int		ch;
	unsigned short		s;
	int			icc;
	unsigned short *buf16 = NULL;

	iflags &= MWTF_PACKMASK|MWTF_DBCSMASK;
	oflags &= MWTF_PACKMASK|MWTF_DBCSMASK;

	/* allow -1 for len with ascii or dbcs*/
	if(cc == -1 && (iflags == MWTF_ASCII))
		cc = strlen((char *)istr);

	/* first check for utf8 input encoding*/
	if(iflags == MWTF_UTF8) {
		/* allocate enough for output string, uc16 max 2*/
		if (oflags != MWTF_UC16)
			buf16 = ALLOCA(cc * 2);

		/* we've only got uc16 now so convert to uc16...*/
		cc = utf8_to_utf16((unsigned char *)istr, cc,
			oflags==MWTF_UC16?(unsigned short*) ostr: buf16);

		if(oflags == MWTF_UC16 || cc < 0) {
			if (buf16)
				FREEA(buf16);
			return cc;
		}

		/* will decode again to requested format (probably ascii)*/
		iflags = MWTF_UC16;
		istr = buf16;
	}

#if HAVE_HZK_SUPPORT
	if(iflags == MWTF_UC16 && oflags == MWTF_ASCII) {
		/* only support uc16 convert to ascii now...*/
		cc = UC16_to_GB( istr, cc, ostr);
		if (buf16)
			FREEA(buf16);
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
#if MW_FEATURE_INTL
		case MWTF_DBCS_BIG5:	/* Chinese BIG5*/
			ch = *istr8++;
#if 0 // HAVE_HBF_SUPPORT
			/* HBF decoding seems smaller range than regular BIG5 - FIXME*/
			if ((ch >= 0xA4 && ch <= 0xC5 && icc
				 && ((*istr8 >= 0x40 && *istr8 <= 0x7E) ||
				     (*istr8 >= 0xA1 && *istr8 <= 0xFE)))
			 	 || ((ch == 0xC6) && icc &&
				 	 (*istr8 >= 0x40 && *istr8 <= 0x7E)))
#else
			if (ch >= 0xA1 && ch <= 0xF9 && icc
				&& ((*istr8 >= 0x40 && *istr8 <= 0x7E) ||
				    (*istr8 >= 0xA1 && *istr8 <= 0xFE)))
			{
				ch = (ch << 8) | *istr8++;
				--icc;
			}
#endif
			break;
		case MWTF_DBCS_EUCCN:	/* Chinese EUCCN (GB2312+0x80)*/
			ch = *istr8++;
			if (ch >= 0xA1 && ch <= 0xF7 && icc && *istr8 >= 0xA1 && *istr8 <= 0xFE) {
				ch = (ch << 8) | *istr8++;
				--icc;
			}
			break;
		case MWTF_DBCS_EUCKR:	/* Korean EUCKR (KSC5601+0x80)*/
			ch = *istr8++;
			if (ch >= 0xA1 && ch <= 0xFE && icc &&
			    *istr8 >= 0xA1 && *istr8 <= 0xFE) {
#if MW_CPU_BIG_ENDIAN
				ch = (ch << 8) | *istr8++;
#else
				ch = ch | (*istr8++ << 8);
#endif
				--icc;
			}
			break;
		case MWTF_DBCS_EUCJP:	/* Japanese EUCJP*/
			ch = *istr8++;
			if (ch >= 0xA1 && ch <= 0xFE && icc && *istr8 >= 0xA1 && *istr8 <= 0xFE) {
				ch = (ch << 8) | *istr8++;
				--icc;
			}
			break;
		case MWTF_DBCS_JIS:	/* Japanese JISX0213*/
			ch = *istr8++;
			if (icc && (
				(ch >= 0xA1 && ch <= 0xFE && *istr8 >= 0xA1 && *istr8 <= 0xFE) ||
			     (((ch >= 0x81 && ch <= 0x9F) || (ch >= 0xE0 && ch <= 0xEF)) &&
			       (*istr8 >= 0x40 && *istr8 <= 0xFC && *istr8 != 0x7F)))) {
					ch = (ch << 8) | *istr8++;
					--icc;
			}

			break;
#endif /* MW_FEATURE_INTL*/
		}
		switch(oflags) {
		default:
			*ostr8++ = (unsigned char)ch;
			break;
		case MWTF_UTF8:
			s = (unsigned short)ch;
			ostr8 += uc16_to_utf8(&s, 1, ostr8);
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

	if (buf16)
		FREEA(buf16);
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
	uint32_t *buf = NULL;

#if MW_FEATURE_INTL
	int		force_uc16 = 0;
	/* DBCS handled specially: see comment in GdText*/
	if (flags & MWTF_DBCSMASK) {
		/* force double-byte sequences to UC16 if builtin font only*/
		if (pfont->fontprocs == &mwfontprocs) {
			defencoding = MWTF_UC16;
			force_uc16 = 1;
		}
	}
#endif

	/* use strlen for char count when ascii or dbcs*/
	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

	/* convert encoding if required*/
	if((flags & (MWTF_PACKMASK|MWTF_DBCSMASK)) != defencoding) {
		/* allocate enough for output string utf8/uc32 is max 4 bytes, uc16 max 2*/
		buf = ALLOCA(cc * 4);
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK; /* keep DBCS bits for gettextsize*/
		flags |= defencoding;
		text = buf;
	} else
		text = str;

	if(cc <= 0 || !pfont->fontprocs->GetTextSize) {
		*pwidth = *pheight = *pbase = 0;
		if (buf)
			FREEA(buf);
		return;
	}

#if MW_FEATURE_INTL
	/* calc height and width of string*/
	if (force_uc16)		/* if UC16 conversion forced, string is DBCS*/
		dbcs_gettextsize(pfont, text, cc, flags, pwidth, pheight, pbase);
	else
#endif
		pfont->fontprocs->GetTextSize(pfont, text, cc, flags, pwidth, pheight, pbase);

	if (buf)
		FREEA(buf);
}

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
GdCreateFontFromBuffer(PSD psd, const unsigned char *buffer, unsigned length,
	const char *format, MWCOORD height, MWCOORD width)
{
	PMWFONT pfont = NULL;

#if HAVE_FREETYPE_2_SUPPORT
	/* EPRINTF("Nano-X: Font magic = '%c%c%c%c' @ GdCreateFontFromBuffer\n",
	 * (char) buffer[0], (char) buffer[1], (char) buffer[2], (char) buffer[3]);
	 */

	/*
	 * If we had multiple font drivers, we'd have to do select one
	 * based on 'format' here.  (Suggestion: 'format' is the file
	 * extension - e.g. TTF, PFR, ...)
	 */
	pfont = (PMWFONT)freetype2_createfontfrombuffer(buffer, length, height, width);
	if (!pfont)
		EPRINTF("GdCreateFontFromBuffer: create failed.\n");
#endif
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
GdDuplicateFont(PSD psd, PMWFONT psrcfont, MWCOORD height, MWCOORD width)
{
#if HAVE_FREETYPE_2_SUPPORT
	if (psrcfont->fontprocs->Duplicate)
		return psrcfont->fontprocs->Duplicate(psrcfont, height, width);
#endif
	return psrcfont;
}

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
	uint32_t scalar;

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

/* 
 * warning: the length of output string may exceed six x the length of the input 
 */ 
int
uc16_to_utf8(const unsigned short *us, int cc, unsigned char *s)
{
	int i;
	unsigned char *t = s;
	unsigned short uc16;
	
	for (i = 0; i < cc; i++) {
		uc16 = *us++;
		if (uc16 <= 0x7F) { 
			*t++ = (char) uc16;
		} else if (uc16 <= 0x7FF) {
			*t++ = 0xC0 | (unsigned char) ((uc16 >> 6) & 0x1F); /* upper 5 bits */
			*t++ = 0x80 | (unsigned char) (uc16 & 0x3F);        /* lower 6 bits */
		} else {
			*t++ = 0xE0 | (unsigned char) ((uc16 >> 12) & 0x0F);/* upper 4 bits */
			*t++ = 0x80 | (unsigned char) ((uc16 >> 6) & 0x3F); /* next 6 bits */
			*t++ = 0x80 | (unsigned char) (uc16 & 0x3F);        /* lowest 6 bits */
		}
	}
	*t = 0;
	return (t - s);
}

/* 
   UTF8 utility: 
   This map return the expected count of bytes based on the first char 
 */
const char utf8_len_map[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

#if DEBUG_TEXT_SHAPING
/*
 *  Return the number of character (not byte) of UTF-8 string
 */
int utf8_nchar ( const char *str )
{
	int n = 0;
	int al = strlen ( str );

	while ( n < al )
		n += utf8_len_map[(unsigned char)str[n]];
	return (n < al) ? n : al;
}

static void	dumpUtf8 ( const char *str, int sz )
{
	int i, n;
	unsigned short uc16;
	const char *last = str+sz;

	DPRINTF( "UTF-8 dump:\n" );
	while ( str < last ) {
		for ( i=0, n=utf8_len_map[(unsigned char)str[0]]; i < n; i++ )
			DPRINTF( "%02X", (unsigned char)str[i] );
		utf8_to_utf16 ( str, n, &uc16 );
		DPRINTF( ": %04X\n", uc16 );
		str += n;
	}
}
#endif	

#if HAVE_SHAPEJOINING_SUPPORT
typedef struct char_shaped {
	unsigned short isolated;
	unsigned short initial;
	unsigned short medial;
	unsigned short final;
} chr_shpjoin_t;

/* This table start from a base of 0x0621, up to 0x06D3 */

#define SHAPED_TABLE_START	0x0621
#define SHAPED_TABLE_TOP	0x06D3

static const chr_shpjoin_t shaped_table[] =
{
	/*  base       s       i       m       f */
	{ /*0x0621*/ 0xFE80, 0x0000, 0x0000, 0x0000, },  /* HAMZA */
	{ /*0x0622*/ 0xFE81, 0x0000, 0x0000, 0xFE82, },  /* ALEF_MADDA */
	{ /*0x0623*/ 0xFE83, 0x0000, 0x0000, 0xFE84, },  /* ALEF_HAMZA_ABOVE */
	{ /*0x0624*/ 0xFE85, 0x0000, 0x0000, 0xFE86, },  /* WAW_HAMZA */
	{ /*0x0625*/ 0xFE87, 0x0000, 0x0000, 0xFE88, },  /* ALEF_HAMZA_BELOW */
	{ /*0x0626*/ 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A, },  /* YEH_HAMZA */
	{ /*0x0627*/ 0xFE8D, 0x0000, 0x0000, 0xFE8E, },  /* ALEF */
	{ /*0x0628*/ 0xFE8F, 0xFE91, 0xFE92, 0xFE90, },  /* BEH */
	{ /*0x0629*/ 0xFE93, 0x0000, 0x0000, 0xFE94, },  /* TEH_MARBUTA */
	{ /*0x062A*/ 0xFE95, 0xFE97, 0xFE98, 0xFE96, },  /* TEH */
	{ /*0x062B*/ 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A, },  /* THEH */
	{ /*0x062C*/ 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E, },  /* JEEM */
	{ /*0x062D*/ 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2, },  /* HAH */
	{ /*0x062E*/ 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6, },  /* KHAH */
	{ /*0x062F*/ 0xFEA9, 0x0000, 0x0000, 0xFEAA, },  /* DAL */
	{ /*0x0630*/ 0xFEAB, 0x0000, 0x0000, 0xFEAC, },  /* THAL */
	{ /*0x0631*/ 0xFEAD, 0x0000, 0x0000, 0xFEAE, },  /* REH */
	{ /*0x0632*/ 0xFEAF, 0x0000, 0x0000, 0xFEB0, },  /* ZAIN */
	{ /*0x0633*/ 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2, },  /* SEEN */
	{ /*0x0634*/ 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6, },  /* SHEEN */
	{ /*0x0635*/ 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA, },  /* SAD */
	{ /*0x0636*/ 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE, },  /* DAD */
	{ /*0x0637*/ 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2, },  /* TAH */
	{ /*0x0638*/ 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6, },  /* ZAH */
	{ /*0x0639*/ 0xFEC9, 0xFECB, 0xFECC, 0xFECA, },  /* AIN */
	{ /*0x063A*/ 0xFECD, 0xFECF, 0xFED0, 0xFECE, },  /* GHAIN */
	{ /*0x063B*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x063C*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x063D*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x063E*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x063F*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0640*/ 0x0640, 0x0640, 0x0640, 0x0640, },  /* TATWEEL */
	{ /*0x0641*/ 0xFED1, 0xFED3, 0xFED4, 0xFED2, },  /* FEH */
	{ /*0x0642*/ 0xFED5, 0xFED7, 0xFED8, 0xFED6, },  /* QAF */
	{ /*0x0643*/ 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA, },  /* KAF */
	{ /*0x0644*/ 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE, },  /* LAM */
	{ /*0x0645*/ 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2, },  /* MEEM */
	{ /*0x0646*/ 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6, },  /* NOON */
	{ /*0x0647*/ 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA, },  /* HEH */
	{ /*0x0648*/ 0xFEED, 0x0000, 0x0000, 0xFEEE, },  /* WAW */
	{ /*0x0649*/ 0xFEEF, 0xFBE8, 0xFBE9, 0xFEF0, },  /* ALEF_MAKSURA */
	{ /*0x064A*/ 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2, },  /* YEH */
	{ /*0x064B*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x064C*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x064D*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x064E*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x064F*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0650*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0651*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0652*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0653*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0654*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0655*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0656*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0657*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0658*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0659*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x065A*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x065B*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x065C*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x065D*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x065E*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x065F*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0660*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0661*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0662*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0663*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0664*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0665*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0666*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0667*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0668*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0669*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x066A*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x066B*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x066C*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x066D*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x066E*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x066F*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0670*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0671*/ 0xFB50, 0x0000, 0x0000, 0xFB51, },
	{ /*0x0672*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0673*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0674*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0675*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0676*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0677*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0678*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0679*/ 0xFB66, 0xFB68, 0xFB69, 0xFB67, },
	{ /*0x067A*/ 0xFB5E, 0xFB60, 0xFB61, 0xFB5F, },
	{ /*0x067B*/ 0xFB52, 0xFB54, 0xFB55, 0xFB53, },
	{ /*0x067C*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x067D*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x067E*/ 0xFB56, 0xFB58, 0xFB59, 0xFB57, },
	{ /*0x067F*/ 0xFB62, 0xFB64, 0xFB65, 0xFB63, },
	{ /*0x0680*/ 0xFB5A, 0xFB5C, 0xFB5D, 0xFB5B, },
	{ /*0x0681*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0682*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0683*/ 0xFB76, 0xFB78, 0xFB79, 0xFB77, },
	{ /*0x0684*/ 0xFB72, 0xFB74, 0xFB75, 0xFB73, },
	{ /*0x0685*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0686*/ 0xFB7A, 0xFB7C, 0xFB7D, 0xFB7B, },
	{ /*0x0687*/ 0xFB7E, 0xFB80, 0xFB81, 0xFB7F, },
	{ /*0x0688*/ 0xFB88, 0x0000, 0x0000, 0xFB89, },
	{ /*0x0689*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x068A*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x068B*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x068C*/ 0xFB84, 0x0000, 0x0000, 0xFB85, },
	{ /*0x068D*/ 0xFB82, 0x0000, 0x0000, 0xFB83, },
	{ /*0x068E*/ 0xFB86, 0x0000, 0x0000, 0xFB87, },
	{ /*0x068F*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0690*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0691*/ 0xFB8C, 0x0000, 0x0000, 0xFB8D, },
	{ /*0x0692*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0693*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0694*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0695*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0696*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0697*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x0698*/ 0xFB8A, 0x0000, 0x0000, 0xFB8B, },
	{ /*0x0699*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x069A*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x069B*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x069C*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x069D*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x069E*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x069F*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A0*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A1*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A2*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A3*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A4*/ 0xFB6A, 0xFB6C, 0xFB6D, 0xFB6B, },
	{ /*0x06A5*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A6*/ 0xFB6E, 0xFB70, 0xFB71, 0xFB6F, },
	{ /*0x06A7*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A8*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06A9*/ 0xFB8E, 0xFB90, 0xFB91, 0xFB8F, },
	{ /*0x06AA*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06AB*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06AC*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06AD*/ 0xFBD3, 0xFBD5, 0xFBD6, 0xFBD4, },
	{ /*0x06AE*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06AF*/ 0xFB92, 0xFB94, 0xFB95, 0xFB93, },
	{ /*0x06B0*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06B1*/ 0xFB9A, 0xFB9C, 0xFB9D, 0xFB9B, },
	{ /*0x06B2*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06B3*/ 0xFB96, 0xFB98, 0xFB99, 0xFB97, },
	{ /*0x06B4*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06B5*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06B6*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06B7*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06B8*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06B9*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06BA*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06BB*/ 0xFBA0, 0xFBA2, 0xFBA3, 0xFBA1, },
	{ /*0x06BC*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06BD*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06BE*/ 0xFBAA, 0xFBAC, 0xFBAD, 0xFBAB, },
	{ /*0x06BF*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06C0*/ 0xFBA4, 0x0000, 0x0000, 0xFBA5, },
	{ /*0x06C1*/ 0xFBA6, 0xFBA8, 0xFBA9, 0xFBA7, },
	{ /*0x06C2*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06C3*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06C4*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06C5*/ 0xFBE0, 0x0000, 0x0000, 0xFBE1, },
	{ /*0x06C6*/ 0xFBD9, 0x0000, 0x0000, 0xFBDA, },
	{ /*0x06C7*/ 0xFBD7, 0x0000, 0x0000, 0xFBD8, },
	{ /*0x06C8*/ 0xFBDB, 0x0000, 0x0000, 0xFBDC, },
	{ /*0x06C9*/ 0xFBE2, 0x0000, 0x0000, 0xFBE3, },
	{ /*0x06CA*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06CB*/ 0xFBDE, 0x0000, 0x0000, 0xFBDF, },
	{ /*0x06CC*/ 0xFBFC, 0xFBFE, 0xFBFF, 0xFBFD, },
	{ /*0x06CD*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06CE*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06CF*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06D0*/ 0xFBE4, 0xFBE6, 0xFBE7, 0xFBE5, },
	{ /*0x06D1*/ 0x0000, 0x0000, 0x0000, 0x0000, },  /* dummy filler */
	{ /*0x06D2*/ 0xFBAE, 0x0000, 0x0000, 0xFBAF, },
	{ /*0x06D3*/ 0xFBB0, 0x0000, 0x0000, 0xFBB1, },
};

#define SHAPED_TABLE2_START	0xFEF5
#define SHAPED_TABLE2_TOP	0xFEFB

/*
 * The second table is for special ligatures
 */
static const chr_shpjoin_t shaped_table2[] =
{
	{ /*0xFEF5*/ 0xFEF5, 0x0000, 0x0000, 0xFEF6, }, /* LAM_ALEF_MADDA */
	{ /*0xFEF6*/ 0x0000, 0x0000, 0x0000, 0x0000, }, /* dummy filler */
	{ /*0xFEF7*/ 0xFEF7, 0x0000, 0x0000, 0xFEF8, }, /* LAM_ALEF_HAMZA_ABOVE */
	{ /*0xFEF8*/ 0x0000, 0x0000, 0x0000, 0x0000, }, /* dummy filler */
	{ /*0xFEF9*/ 0xFEF9, 0x0000, 0x0000, 0xFEFA, }, /* LAM_ALEF_HAMZA_BELOW */
	{ /*0xFEFA*/ 0x0000, 0x0000, 0x0000, 0x0000, }, /* dummy filler */
	{ /*0xFEFB*/ 0xFEFB, 0x0000, 0x0000, 0xFEFC, }, /* LAM_ALEF */
};

#define assignShape(chr)	( ((chr) >= SHAPED_TABLE_START  && (chr) <= SHAPED_TABLE_TOP)? \
				    &shaped_table[(chr)-SHAPED_TABLE_START] : \
                                  ((chr) >= SHAPED_TABLE2_START && (chr) <= SHAPED_TABLE2_TOP)? \
				    &shaped_table2[(chr)-SHAPED_TABLE2_START] : NULL) 

#define assignShapeUtf(txt, i) ( (utf8_len_map[(unsigned char)((txt)[(i)])] > 1)? \
					doAssignShapeUtf((txt)+(i)) : NULL)

static const chr_shpjoin_t *
doAssignShapeUtf(const char *txt)
{
	unsigned short fs;

	utf8_to_utf16((const unsigned char *) txt,
		      utf8_len_map[(unsigned char) txt[0]], &fs);
	return assignShape(fs);
}


static void
storeUc_2_Utf8(char *dest, int *psz, unsigned short wch)
{
	int cb = uc16_to_utf8(&wch, 1, (unsigned char *)dest + (*psz));

	*psz = *psz + cb;
}


static void
store_Utf8(char *dest, int *psz, const char *txt)
{
	int cb = utf8_len_map[(unsigned char) txt[0]];

	memcpy(dest + (*psz), txt, cb);
	*psz = *psz + cb;
}

/*
 * Note that text is currently left to right
 */
static unsigned short *
arabicJoin_UC16(const unsigned short *text, int len, unsigned long *pAttrib)
{
	int i;
	unsigned short *new_str;
	const chr_shpjoin_t *prev = NULL;
	const chr_shpjoin_t *curr = NULL;
	const chr_shpjoin_t *next = NULL;
	unsigned long attrib = 0;

	new_str = (unsigned short *) malloc((1 + len) * sizeof(unsigned short));
	if (new_str == NULL)
		return NULL;

	for (i = 0; i < len; i++) {
		if ((curr = assignShape(text[i])) != NULL) {
			if (i < len - 1)
				next = assignShape(text[i + 1]);
			else
				next = NULL;
			if (next) {
				if (prev) {
					if (!prev->initial || !prev->medial)
						new_str[i] = curr->initial ?
							curr->initial : curr->isolated;
					else
						new_str[i] = curr->medial ?
							curr->medial : curr->final;
				} else {
					new_str[i] = curr->initial ?
						curr->initial : curr->isolated;
				}
			} else {
				if (prev) {
					if (!prev->initial || !prev->medial)
						new_str[i] = curr->isolated;
					else
						new_str[i] = curr->final ?
							curr->final : curr->isolated;
				} else {
					new_str[i] = curr->isolated;
				}
			}
			attrib |= (TEXTIP_SHAPED | TEXTIP_EXTENDED);
		} else {
			new_str[i] = text[i];
			if (text[i] <= 0xFF)
				attrib |= TEXTIP_STANDARD;
			else
				attrib |= TEXTIP_EXTENDED;
		}

		prev = curr;
	}
	new_str[i] = 0;
	if (pAttrib)
		*pAttrib = attrib;
	return new_str;
}

/*
 * Note that text is currently left to right
 */
static char *
arabicJoin_UTF8(const char *text, int len, int *pNewLen, unsigned long *pAttrib)
{
	int i, sz;
	char *new_str;
	const chr_shpjoin_t *prev = NULL;
	const chr_shpjoin_t *curr = NULL;
	const chr_shpjoin_t *next = NULL;
	unsigned long attrib = 0;

	/* Note that shaping may result in three UTF-8 bytes, due to 06xx -> FBxx translation*/
	/* two times the original buffer should be enough...*/
	new_str = (char *) malloc((1 + 2 * len) * sizeof(char));
	if (new_str == NULL)
		return NULL;

	sz = 0;

	for (i = 0; i < len;) {
		int b = utf8_len_map[(unsigned char) text[i]];
		if ((curr = assignShapeUtf(text, i)) != NULL) {
			if (i < len - b)
				next = assignShapeUtf(text, i + b);
			else
				next = NULL;
			if (next) {
				if (prev) {
					if (!prev->initial || !prev->medial)
						storeUc_2_Utf8(new_str, &sz,
							       (curr->initial ? curr->initial :
								curr->isolated));
					else
						storeUc_2_Utf8(new_str, &sz,
							       (curr->medial ? curr->medial :
								curr->final));
				} else {
					storeUc_2_Utf8(new_str, &sz, (curr->initial ?
							curr->initial : curr-> isolated));
				}
			} else {
				if (prev) {
					if (!prev->initial || !prev->medial)
						storeUc_2_Utf8(new_str, &sz, curr->isolated);
					else
						storeUc_2_Utf8(new_str, &sz,
							       (curr->final ? curr->final :
							       curr->isolated));
				} else {
					storeUc_2_Utf8(new_str, &sz, curr->isolated);
				}
			}
			attrib |= (TEXTIP_SHAPED | TEXTIP_EXTENDED);
		} else {
			store_Utf8(new_str, &sz, text + i);
			if ((unsigned char) text[i] < 0xC0)
				attrib |= TEXTIP_STANDARD;
			else
				attrib |= TEXTIP_EXTENDED;
		}

		i += b;
		prev = curr;
	}
	new_str[sz] = 0;
	if (pNewLen)
		*pNewLen = sz;
	if (pAttrib)
		*pAttrib = attrib;
#if DEBUG_TEXT_SHAPING
	if (strcmp(new_str, text))
		dumpUtf8(new_str, sz);
#endif
	return new_str;
}

unsigned short *
doCharShape_UC16(const unsigned short *text, int len, int *pNewLen, unsigned long *pAttrib)
{
	unsigned short *conv = arabicJoin_UC16(text, len, pAttrib);

	if (pNewLen)
		*pNewLen = len;
	return conv;
}

char *
doCharShape_UTF8(const char *text, int len, int *pNewLen, unsigned long *pAttrib)
{
	return arabicJoin_UTF8(text, len, pNewLen, pAttrib);
}

#else /* !HAVE_SHAPEJOINING_SUPPORT */
/* DUMMY FUNCTIONS */
unsigned short *
doCharShape_UC16(const unsigned short *text, int len, int *pNewLen, unsigned long *pAttrib)
{
	unsigned short *conv = malloc((len + 1) * sizeof(unsigned short));

	if (conv == NULL)
		return NULL;
	memcpy(conv, text, len * sizeof(unsigned short));
	conv[len] = 0;
	if (pNewLen)
		*pNewLen = len;
	if (pAttrib)
		*pAttrib = 0;
	return conv;
}

char *
doCharShape_UTF8(const char *text, int len, int *pNewLen, unsigned long *pAttrib)
{
	char *conv = malloc((len + 1) * sizeof(char));

	if (conv == NULL)
		return NULL;
	memcpy(conv, text, len * sizeof(char));
	conv[len] = 0;
	if (pNewLen)
		*pNewLen = len;
	if (pAttrib)
		*pAttrib = 0;
	return conv;
}
#endif /* HAVE_SHAPEJOINING_SUPPORT */


#if HAVE_FRIBIDI_SUPPORT
#include <fribidi/fribidi.h>

		char *
doCharBidi_UTF8(const char *text, int len, int *v2lPos, char *pDirection, unsigned long *pAttrib)
{
	FriBidiChar *ftxt, *fvirt;
	FriBidiChar localBuff[128];
	FriBidiCharType basedir;
	int cc;
	int isLocal = 0;
	char *new_str;
	int new_len;

	new_str = (char *) malloc(len + 1);
	if (new_str == NULL)
		return NULL;

	/* len may be greather than real char count, but it's ok.
	   if will fit in localBuff, we use it to improve speed */
	if (len < sizeof(localBuff) / sizeof(localBuff[0]) / 2) {
		ftxt = localBuff;
		fvirt = localBuff + sizeof(localBuff) / sizeof(localBuff[0]) / 2;
		isLocal = 1;
	} else {
		ftxt = (FriBidiChar *) malloc((len + 1) * sizeof(FriBidiChar));
		fvirt = (FriBidiChar *) malloc((len + 1) * sizeof(FriBidiChar));
	}

	if (ftxt == NULL) {
		free(new_str);
		return NULL;
	}
	if (fvirt == NULL) {
		free(new_str);
		free(ftxt);
		return NULL;
	}

	cc = fribidi_utf8_to_unicode((char *) text, len, ftxt);
	basedir = FRIBIDI_TYPE_N;
	fribidi_log2vis(ftxt, cc, &basedir, fvirt, v2lPos, NULL, pDirection);
	new_len = fribidi_unicode_to_utf8(fvirt, cc, new_str);

	if (pAttrib) {
		if (basedir & FRIBIDI_MASK_RTL)
			*pAttrib |= TEXTIP_RTOL;
	}

	if (!isLocal) {
		free(fvirt);
		free(ftxt);
	}
	new_str[new_len] = 0;
	return new_str;
}


unsigned short *
doCharBidi_UC16(const unsigned short *text, int len, int *v2lPos, char *pDirection, unsigned long *pAttrib)
{
	FriBidiChar *ftxt, *fvirt;
	FriBidiChar localBuff[128];
	FriBidiCharType basedir;
	int cc;
	int isLocal = 0;
	unsigned short *new_str;

	new_str = (unsigned short *) malloc((len + 1) * sizeof(unsigned short));
	if (new_str == NULL)
		return NULL;

	/* len may be greather than real char count, but it's ok.
	   if will fit in localBuff, we use it to improve speed */
	if (len < sizeof(localBuff) / sizeof(localBuff[0]) / 2) {
		ftxt = localBuff;
		fvirt = localBuff + sizeof(localBuff) / sizeof(localBuff[0]) / 2;
		isLocal = 1;
	} else {
		ftxt = (FriBidiChar *) malloc((len + 1) * sizeof(FriBidiChar));
		fvirt = (FriBidiChar *) malloc((len + 1) * sizeof(FriBidiChar));
	}

	if (ftxt == NULL) {
		free(new_str);
		return NULL;
	}
	if (fvirt == NULL) {
		free(new_str);
		free(ftxt);
		return NULL;
	}

	for (cc = 0; cc < len; cc++)
		ftxt[cc] = text[cc];
	basedir = FRIBIDI_TYPE_N;
	fribidi_log2vis(ftxt, cc, &basedir, fvirt, v2lPos, NULL, pDirection);
	for (cc = 0; cc < len; cc++)
		new_str[cc] = (unsigned short) fvirt[cc];
	new_str[cc] = 0;

	if (pAttrib) {
		if (basedir & FRIBIDI_MASK_RTL)
			*pAttrib |= TEXTIP_RTOL;
	}

	if (!isLocal) {
		free(fvirt);
		free(ftxt);
	}
	return new_str;
}

#else
/* DUMMY FUNCTIONS */
char *
doCharBidi_UTF8(const char *text, int len, int *v2lPos, char *pDirection, unsigned long *pAttrib)
{
	int i;
	unsigned short *conv = malloc((len + 1) * sizeof(unsigned short));

	if (conv == NULL)
		return NULL;
	memcpy(conv, text, len * sizeof(unsigned short));
	conv[len] = 0;
	if (v2lPos)
		for (i = 0; i < len; i++)
			v2lPos[i] = i;
	if (pDirection)
		memset(pDirection, 0, len * sizeof(pDirection[0]));
	return (char *) conv;
}
unsigned short *
doCharBidi_UC16(const unsigned short *text, int len, int *v2lPos, char *pDirection, unsigned long *pAttrib)
{
	int i;
	char *conv = malloc((len + 1) * sizeof(char));

	if (conv == NULL)
		return NULL;
	memcpy(conv, text, len * sizeof(char));
	conv[len] = 0;
	if (v2lPos)
		for (i = 0; i < len; i++)
			v2lPos[i] = i;
	if (pDirection)
		memset(pDirection, 0, len * sizeof(pDirection[0]));
	return (unsigned short *) conv;
}
#endif /* HAVE_FRIBIDI_SUPPORT */
