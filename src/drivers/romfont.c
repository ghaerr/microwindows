/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * PC ROM Font Routine Header (PC ROM font format)
 *
 * This file contains the PC ROM format low-level font/text
 * drawing routines.  Only fixed pitch fonts are supported.
 * The ROM character matrix is used for the text bitmaps.
 *
 * The environment variable CHARHEIGHT if set will set the assumed rom
 * font character height, which defaults to 14.
 */
#include <stdlib.h>
#include "device.h"
#include "vgaplan4.h"
#include "romfont.h"

/* local data*/
int	ROM_CHAR_HEIGHT = 14;	/* number of scan lines in fonts in ROM */
FARADDR rom_char_addr;

/* handling routines for core rom fonts*/
static MWFONTPROCS fontprocs = {
	MWTF_ASCII,		/* routines expect ascii*/
	pcrom_getfontinfo,
	pcrom_gettextsize,
	pcrom_gettextbits,
	pcrom_unloadfont,
	corefont_drawtext,
	NULL,			/* setfontsize*/
	NULL,			/* setfontrotation*/
	NULL,			/* setfontattr*/
};

/* first font is default font if no match*/
MWCOREFONT pcrom_fonts[NUMBER_FONTS] = {
	{&fontprocs, 0, 0, 0, MWFONT_OEM_FIXED, NULL}
};

/* init PC ROM routines, must be called in graphics mode*/
void
pcrom_init(PSD psd)
{
	char *	p;

	/* use INT 10h to get address of rom character table*/
	rom_char_addr = int10(FNGETROMADDR, GETROM8x14);
#if 0
	/* check bios data area for actual character height,
	 * as the returned font isn't always 14 high
	 */
	ROM_CHAR_HEIGHT = GETBYTE_FP(MK_FP(0x0040, 0x0085));
	if(ROM_CHAR_HEIGHT > MAX_ROM_HEIGHT)
		ROM_CHAR_HEIGHT = MAX_ROM_HEIGHT;
#endif
	p = getenv("CHARHEIGHT");
	if(p)
		ROM_CHAR_HEIGHT = atoi(p);
}

/*
 * PC ROM low level get font info routine.  This routine
 * returns info on a single bios ROM font.
 */
MWBOOL
pcrom_getfontinfo(PMWFONT pfont,PMWFONTINFO pfontinfo)
{
	int	i;

	pfontinfo->maxwidth = ROM_CHAR_WIDTH;
	pfontinfo->height = ROM_CHAR_HEIGHT;
	pfontinfo->baseline = ROM_CHAR_HEIGHT;
	pfontinfo->firstchar = 0;
	pfontinfo->lastchar = 255;
	pfontinfo->fixed = TRUE;
	for (i = 0; i < 256; i++)
		pfontinfo->widths[i] = ROM_CHAR_WIDTH;
	return TRUE;
}

/*
 * PC ROM low level routine to calc bounding box for text output.
 * Handles bios ROM font only.
 */
void
pcrom_gettextsize(PMWFONT pfont, const void *str, int cc,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	*pwidth = ROM_CHAR_WIDTH * cc;
	*pheight = ROM_CHAR_HEIGHT;
	*pbase = ROM_CHAR_HEIGHT;
}

/*
 * PC ROM low level routine to get the bitmap associated
 * with a character.  Handles bios ROM font only.
 */
void
pcrom_gettextbits(PMWFONT pfont, int ch, MWIMAGEBITS *retmap,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	FARADDR	bits;
	int	n;

	/* read character bits from rom*/
	bits = rom_char_addr + ch * ROM_CHAR_HEIGHT;
	for(n=0; n<ROM_CHAR_HEIGHT; ++n)
		*retmap++ = GETBYTE_FP(bits++) << 8;

	*pwidth = ROM_CHAR_WIDTH;
	*pheight = ROM_CHAR_HEIGHT;
	*pbase = ROM_CHAR_HEIGHT;
}

void
pcrom_unloadfont(PMWFONT pfont)
{
	/* rom fonts can't be unloaded*/
}

#if NOTUSED
/* 
 * Low level text draw routine, called only if no clipping
 * is required.  This routine draws ROM font characters only.
 */
void
pcrom_drawtext(PMWFONT pfont, PSD psd, COORD x, COORD y,
	const void *text, int n, PIXELVAL fg)
{
	const unsigned char *	str = text;
	COORD 			width;		/* width of character */
	COORD 			height;		/* height of character */
	IMAGEBITS 	bitmap[MAX_ROM_HEIGHT];	/* bitmap for character */

 	/* x,y is bottom left corner*/
	y -= (ROM_CHAR_HEIGHT - 1);
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
gen_drawbitmap(PSD psd,COORD x, COORD y, COORD width, COORD height,
	IMAGEBITS *table, PIXELVAL fgcolor)
{
  COORD minx;
  COORD maxx;
  IMAGEBITS bitvalue;	/* bitmap word value */
  int bitcount;			/* number of bits left in bitmap word */

  minx = x;
  maxx = x + width - 1;
  bitcount = 0;
  while (height > 0) {
	if (bitcount <= 0) {
		bitcount = IMAGE_BITSPERIMAGE;
		bitvalue = *table++;
	}
	if (IMAGE_TESTBIT(bitvalue))
		psd->DrawPixel(psd, x, y, fgcolor);
	bitvalue = IMAGE_SHIFTBIT(bitvalue);
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
