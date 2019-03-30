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

#define DEBUG_TEXT_SHAPING	0

/* temp extern decls*/
extern MWCOREFONT *user_builtin_fonts;

static int utf8_to_utf16(const unsigned char *utf8, int cc, unsigned short *unicode16);
int uc16_to_utf8(const unsigned short *us, int cc, unsigned char *s);


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
	char 		fontmapper_fontname[MWLF_FACESIZE + 1];

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

	if (fontclass == MWLF_CLASS_ANY) {
		EPRINTF("createfont: %s,%d not found\n", fontname, height);
		EPRINTF("  (tried "
			"builtin_createfont"
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

	/* use strlen for char count when ascii or dbcs*/
	if(cc == -1) {
		MWTEXTFLAGS inflag = flags & MWTF_PACKMASK;
		if (inflag == MWTF_ASCII || inflag == MWTF_UTF8)
			cc = strlen((char *)str);
		else DPRINTF("GdText: Bad cc argument\n");
	}

		text = str;

	if(cc <= 0 || !pfont->fontprocs->DrawText) {
		if (buf)
			FREEA(buf);
		return;
	}

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
		//else
			//GdBitmapByPoint(psd, x, y, width, height, bitmap, clip);
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

	/* use strlen for char count when ascii or dbcs*/
	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

		text = str;

	if(cc <= 0 || !pfont->fontprocs->GetTextSize) {
		*pwidth = *pheight = *pbase = 0;
		if (buf)
			FREEA(buf);
		return;
	}

		pfont->fontprocs->GetTextSize(pfont, text, cc, flags, pwidth, pheight, pbase);

	if (buf)
		FREEA(buf);
}
