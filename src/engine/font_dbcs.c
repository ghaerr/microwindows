/* 
 * Double-byte font routines for Microwindows
 * Copyright (c) 2003 Greg Haerr <greg@censoft.com>
 *
 * These routines allow specially-compiled-in fonts to be
 * used for DBCS-encoded characters in an otherwise ASCII
 * character stream.  Characters not belonging to the DBCS
 * fonts (ASCII < 0x0100 only) will display out of the 
 * builtin font instead.
 */
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "devfont.h"
#include "../drivers/genfont.h"

#if HAVE_BIG5_SUPPORT
static void
big5_gettextbits(PMWFONT pfont, unsigned int ch, const MWIMAGEBITS **retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	unsigned int	CH = ch >> 8;
	unsigned int	CL = ch & 0xFF;
	int		pos;	/* ((CH - 0xA1) * 94 + (CL - 0xA1)) * 18; */
	int		i;
	int		seq = 0;
	static MWIMAGEBITS map[12];
	extern unsigned char JMT_BIG5_12X12_FONT_BITMAP[];

	/*if (CH >= 0xA1 && CH <= 0xF9 &&
	   ((CL >= 0x40 && CL <= 0x7E) || (CL >= 0xA1 && CL <= 0xFE)) ) */

	CL -= (CL < 127)? 64 : 98;

	if (CH >= 0xa4) {	/* standard font */
		seq = ((CH - 164) * 157) + CL;
		if (seq >= 5809)
			seq -= 408;
	}
	if (CH <= 0xa3)		/* special font */
		seq = (((CH - 161) * 157) + CL) + 13094;
	pos = seq * 18;

	for (i = 0; i < 6; i++) {
		unsigned char *dst = ((unsigned char *)map) + i * 4;
		unsigned char *src = JMT_BIG5_12X12_FONT_BITMAP +
			pos + i * 3;
		dst[0] = src[1];
		dst[1] = src[0];
		dst[2] = src[1] << 4;
		dst[3] = src[2];
	}

	*pwidth = 12;
	*pheight = 12;
	*pbase = 0;
	*retmap = map;
}
#endif /* HAVE_BIG5_SUPPORT*/

#if HAVE_GB2312_SUPPORT
static void
euccn_gettextbits(PMWFONT pfont, unsigned int ch, const MWIMAGEBITS **retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	unsigned int	CH = ch >> 8;
	unsigned int	CL = ch & 0xFF;
	unsigned int	pos = ((CH - 0xA1) * 94 + (CL - 0xA1)) * 18;
	int		i;
	static MWIMAGEBITS map[12];
	extern unsigned char GUO_GB2312_12X12_FONT_BITMAP[];

	/*if (CH >= 0xA1 && CH < 0xF8 && CL >= 0xA1 && CL < 0xFF) */
	for (i = 0; i < 6; i++) {
		unsigned char *dst = ((unsigned char *)map) + i * 4;
		unsigned char *src = GUO_GB2312_12X12_FONT_BITMAP +
			pos + i * 3;
		dst[0] = src[1];
		dst[1] = src[0];
		dst[2] = src[1] << 4;
		dst[3] = src[2];
	}
	*pwidth = 12;
	*pheight = 12;
	*pbase = 0;
	*retmap = map;
}
#endif /* HAVE_GB2312_SUPPORT*/

#if HAVE_JISX0213_SUPPORT
static void
jis_gettextbits(PMWFONT pfont, unsigned int ch, const MWIMAGEBITS **retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	unsigned int	CH = ch >> 8;
	unsigned int	CL = ch & 0xFF;
	unsigned int	pos = 0;
	unsigned short *map;
	extern unsigned short JP_JISX0213_12X12_FONT_BITMAP[];

		/*EUC-JISX0213 */
	if (CH >= 0xA1 && CH <= 0xFE && CL >= 0xA1 && CL <= 0xFE) {
		pos = (CH - 0xA1) * 94 + (CL - 0xA1);
	} else
		/*SHIFT-JISX0213 */
	if ((CH >= 0x81 && CH <= 0x9F) || (CH >= 0xE0 && CH <= 0xEF)) {
		unsigned int EH = 0;
		unsigned int EL = 0;

		if (CH >= 0x81 && CH <= 0x9F &&
		    CL >= 0x40 && CL <= 0xFC && CL != 0x7F) {
			EH = CH - 0x81;
			if (CL >= 0x40 && CL <= 0x7E)
				EL = CL - 0x40;
			else if (CL >= 0x80 && CL <= 0xFC)
				EL = CL - 0x40 - 1;
			pos = EH * 188 + EL;
		}
		if (CH >= 0xE0 && CH <= 0xEF &&
		    CL >= 0x40 && CL <= 0xFC && CL != 0x7F) {
			EH = CH - 0xE0 + (0x9F - 0x81);
			if (CL >= 0x40 && CL <= 0x7E)
				EL = CL - 0x40;
			else if (CL >= 0x80 && CL <= 0xFC)
				EL = CL - 0x40 - 1;
			pos = EH * 188 + EL;
		}
	}
	map = JP_JISX0213_12X12_FONT_BITMAP + pos * 12;

	*pwidth = 12;
	*pheight = 12;
	*pbase = 0;
	*retmap = (MWIMAGEBITS *)map;
}
#endif /*HAVE_JISX0213_SUPPORT*/

#if HAVE_KSC5601_SUPPORT
static void
euckr_gettextbits(PMWFONT pfont, unsigned int ch, const MWIMAGEBITS **retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	unsigned int	CH = ch >> 8;
	unsigned int	CL = ch & 0xFF;
	int		mc;
	static MWIMAGEBITS map[16];
	extern unsigned short convert_ksc_to_johab(unsigned char CH,
						   unsigned char CL);
	extern int get_han_image(int mc, char *retmap);

	/*if (CH>= 0xA1 &&  CH<= 0xFE && CL >= 0xA1 && CL <= 0xFE) */
	mc = convert_ksc_to_johab(CH, CL);
	if (mc)
		get_han_image(mc, (char *) map);

	*pwidth = 16;
	*pheight = 16;
	*pbase = 0;
	*retmap = map;
}
#endif /* HAVE_KSC5601_SUPPORT*/

void
dbcs_gettextbits(PMWFONT pfont, unsigned int ch, MWTEXTFLAGS flags,
	const MWIMAGEBITS **retmap, MWCOORD *pwidth, MWCOORD *pheight,
	MWCOORD *pbase)
{
	/* gettextbits function*/
	void (*func)(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);

	if (ch >= 0x0100) {	/* character was DBCS-encoded*/
		switch (flags & MWTF_DBCSMASK) {
#if HAVE_BIG5_SUPPORT
		case MWTF_DBCS_BIG5:
			func = big5_gettextbits;
			break;
#endif
#if HAVE_GB2312_SUPPORT
		case MWTF_DBCS_EUCCN:
			func = euccn_gettextbits;
			break;
#endif
#if HAVE_KSC5601_SUPPORT
		case MWTF_DBCS_EUCKR:
			func = euckr_gettextbits;
			break;
#endif
#if HAVE_JISX0213_SUPPORT
		case MWTF_DBCS_JIS:
			func = jis_gettextbits;
			break;
#endif
		/*case MWTF_DBCS_EUCJP:*/
		default:
			*pwidth = *pheight = *pbase = 0;
			return;
		}
	} else /* not DBCS, use standard corefont routine*/
		func = gen_gettextbits;

	/* get text bits*/
	func(pfont, ch, retmap, pwidth, pheight, pbase);
}

/*
 * Calc text size using specially-compiled-in DBCS fonts for each
 * non-ASCII character, else use normally selected font.
 */
void
dbcs_gettextsize(PMWFONT pfont, const unsigned short *str, int cc,
	MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWCFONT	pf = ((PMWCOREFONT)pfont)->cfont;
	unsigned int	c;
	int		width = 0;

	flags &= MWTF_DBCSMASK;
	while(--cc >= 0) {
		c = *str++;
		if (c >= 0x0100) {	/* character was DBCS-encoded*/
			switch (flags) {
			case MWTF_DBCS_BIG5:
			case MWTF_DBCS_EUCCN:
			case MWTF_DBCS_JIS:
			/*case MWTF_DBCS_EUCJP:*/
				width += 12;
				continue;
			case MWTF_DBCS_EUCKR:
				width += 12;
				continue;
			}
		} else if(c >= pf->firstchar && c < pf->firstchar+pf->size)
			width += pf->width? pf->width[c - pf->firstchar]:
				pf->maxwidth;
	}
	*pwidth = width;
	*pheight = pf->height;
	*pbase = pf->ascent;
}
