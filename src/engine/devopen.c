/*
 * Copyright (c) 1999, 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Device-independent mid level screen device init routines
 *
 * These routines implement the smallest Microwindows engine level
 * interface to the screen driver.  By setting the NOFONTSORCLIPPING
 * config option, only these routines will be included, which can
 * be used to generate a low-level interface to the screen drivers
 * without dragging in any other GdXXX routines.
 */
#include <stdlib.h>
#include "device.h"

#if MSDOS | ELKS
#define NOSTDPAL8
#endif

/*
 * The following define can change depending on the window manager
 * usage of colors and layout of the 8bpp palette devpal8.c.
 * Color entries below this value won't be overwritten by user
 * programs or bitmap display conversion tables.
 */
#define FIRSTUSERPALENTRY	24  /* first writable pal entry over 16 color*/

MWPIXELVAL gr_foreground;	/* current foreground color */
MWPIXELVAL gr_background;	/* current background color */
MWBOOL 	gr_usebg;    	    /* TRUE if background drawn in pixmaps */
int 	gr_mode = MWROP_COPY; 	    /* drawing mode */
/*static*/ MWPALENTRY	gr_palette[256];    /* current palette*/
/*static*/ int	gr_firstuserpalentry;/* first user-changable palette entry*/
/*static*/ int 	gr_nextpalentry;    /* next available palette entry*/
MWCOLORVAL gr_foreground_rgb;	/* current fg color in 0xAARRGGBB format for mono convblits*/
MWCOLORVAL gr_background_rgb;	/* current background color */

uint32_t gr_dashmask;     /* An actual bitmask of the dash values */
uint32_t gr_dashcount;    /* The number of bits defined in the dashmask */

int        gr_fillmode;
MWSTIPPLE  gr_stipple;
MWTILE     gr_tile;

MWPOINT    gr_ts_offset;

/**
 * Open low level graphics driver and optionally clear screen.
 *
 * @return The screen drawing surface.
 */
PSD
GdOpenScreenExt(MWBOOL clearflag)
{
	PSD			psd;
	MWPALENTRY *		stdpal;

	psd = scrdev.Open(&scrdev);
	if (!psd)
		return NULL;

#if MW_FEATURE_PALETTE
	/* assume no user changable palette entries*/
	gr_firstuserpalentry = (int)psd->ncolors;

	/* set palette according to system colors and devpalX.c*/
	switch((int)psd->ncolors) {

#if !defined(NOSTDPAL1) /* don't require stdpal1 if not needed */
	case 2:		/* 1bpp*/
	{
		extern MWPALENTRY	mwstdpal1[2];
		stdpal = mwstdpal1;
	}
	break;
#endif

#if !defined(NOSTDPAL2) /* don't require stdpal2 if not needed */
	case 4:		/* 2bpp*/
	{
		extern MWPALENTRY	mwstdpal2[4];
		stdpal = mwstdpal2;
	}
	break;
#endif

#if !defined(NOSTDPAL4)
	/* don't require stdpal4 if not needed */
	case 8:		/* 3bpp - not fully supported*/
	case 16:	/* 4bpp*/
	{
		extern MWPALENTRY	mwstdpal4[16];
		stdpal = mwstdpal4;
	}
	break;
#endif

#if !defined(NOSTDPAL8) /* don't require large stdpal8 if not needed */
	case 256:	/* 8bpp*/
	{
		extern MWPALENTRY	mwstdpal8[256];
#if UNIFORMPALETTE
		/* don't change uniform palette if alpha blending*/
		gr_firstuserpalentry = 256;
#else
		/* start after last system-reserved color*/
		gr_firstuserpalentry = FIRSTUSERPALENTRY;
#endif
		stdpal = mwstdpal8;
	} 
	break;
#endif	/* !defined(NOSTDPAL8)*/

	default:	/* truecolor*/
		/* no palette*/
		gr_firstuserpalentry = 0;
		stdpal = NULL;
	}

	/* reset next user palette entry, write hardware palette*/
	GdResetPalette();
	GdSetPalette(psd, 0, (int)psd->ncolors, stdpal);
#endif /* MW_FEATURE_PALETTE*/

	/* init local vars*/
	GdSetMode(MWROP_COPY);
	GdSetFillMode(MWFILL_SOLID);  /* Set the fill mode to solid */

	GdSetForegroundColor(psd, MWRGB(255, 255, 255));	/* WHITE*/
	GdSetBackgroundColor(psd, MWRGB(0, 0, 0));		/* BLACK*/
	GdSetUseBackground(TRUE);
	/* select first builtin font (usually MWFONT_SYSTEM_VAR)*/
	//GdSetFont(GdCreateFont(psd, NULL, 0, 0, NULL));

	GdSetDash(0, 0);  /* No dashing to start */
#if MW_FEATURE_SHAPES
	GdSetStippleBitmap(0,0,0);  /* No stipple to start */
#endif

#if !NOCLIPPING
#if DYNAMICREGIONS
	GdSetClipRegion(psd, GdAllocRectRegion(0, 0, psd->xvirtres, psd->yvirtres));
#else
	GdSetClipRects(psd, 0, NULL);
#endif /* DYNAMICREGIONS*/
#endif /* NOCLIPPING*/

	/* fill black (actually fill to first palette entry or truecolor 0*/
	if (clearflag)
		psd->FillRect(psd, 0, 0, psd->xvirtres-1, psd->yvirtres-1, 0);
	return psd;
}

/**
 * Open low level graphics driver and clear screen.
 *
 * @return The screen drawing surface.
 */
PSD
GdOpenScreen(void)
{
	return GdOpenScreenExt(TRUE);
}

/**
 * Close low level graphics driver
 *
 * @param psd Screen drawing surface.
 */
void 
GdCloseScreen(PSD psd)
{
	psd->Close(psd);
}

/**
 * Set dynamic screen portrait mode, return new mode
 *
 * @param psd Screen drawing surface.
 * @param portraitmode New portrait mode requested.
 * @return New portrait mode actually set.
 */
int
GdSetPortraitMode(PSD psd, int portraitmode)
{
	/* set portrait mode if supported*/
	if (psd->SetPortrait)
		psd->SetPortrait(psd, portraitmode);
	return psd->portrait;
}

/**
 * Get information about the screen (resolution etc).
 *
 * @param psd Screen drawing surface.
 * @param psi Destination for screen information.
 */
void
GdGetScreenInfo(PSD psd, PMWSCREENINFO psi)
{
	psd->GetScreenInfo(psd, psi);
	GdGetButtonInfo(&psi->buttons);
	GdGetModifierInfo(&psi->modifiers, NULL);
	GdGetCursorPos(&psi->xpos, &psi->ypos);
}

#if MW_FEATURE_PALETTE
/**
 *
 * reset palette to empty except for system colors
 */
void
GdResetPalette(void)
{
	/* note: when palette entries are changed, all 
	 * windows may need to be redrawn
	 */
	gr_nextpalentry = gr_firstuserpalentry;
}

/**
 * Set the system palette section to the passed palette entries
 *
 * @param psd Screen device.
 * @param first First palette entry to set.
 * @param count Number of palette entries to set.
 * @param palette New palette entries (array of size @param count).
 */
void
GdSetPalette(PSD psd, int first, int count, MWPALENTRY *palette)
{
	int	i;

	/* no palette management needed if running truecolor*/
	if(psd->pixtype != MWPF_PALETTE)
		return;

	/* bounds check against # of device color entries*/
	if(first + count > (int)psd->ncolors)
		count = (int)psd->ncolors - first;
	if(count >= 0 && first < (int)psd->ncolors) {
		psd->SetPalette(psd, first, count, palette);

		/* copy palette for GdFind*Color*/
		for(i=0; i<count; ++i)
			gr_palette[i+first] = palette[i];
	}
}

/**
 * Get system palette entries
 *
 * @param psd Screen device.
 * @param first First palette index to get.
 * @param count Number of palette entries to retrieve.
 * @param palette Recieves palette entries (array of size @param count).
 */
int
GdGetPalette(PSD psd, int first, int count, MWPALENTRY *palette)
{
	int	i;

	/* no palette if running truecolor*/
	if(psd->pixtype != MWPF_PALETTE)
		return 0;

	/* bounds check against # of device color entries*/
	if(first + count > (int)psd->ncolors)
		if( (count = (int)psd->ncolors - first) <= 0)
			return 0;

	for(i=0; i<count; ++i)
		*palette++ = gr_palette[i+first];

	return count;
}

/**
 * Search a palette to find the nearest color requested.
 * Uses a weighted squares comparison.
 *
 * @param pal Palette to search.
 * @param size Size of palette (number of entries).
 * @param cr Color to look for.
 */
MWPIXELVAL
GdFindNearestColor(MWPALENTRY *pal, int size, MWCOLORVAL cr)
{
	MWPALENTRY *	rgb;
	int		r, g, b;
	int		R, G, B;
	int32_t		diff = 0x7fffffffL;
	int32_t		sq;
	int		best = 0;

	r = REDVALUE(cr);
	g = GREENVALUE(cr);
	b = BLUEVALUE(cr);
	for(rgb=pal; diff && rgb < &pal[size]; ++rgb) {
		R = rgb->r - r;
		G = rgb->g - g;
		B = rgb->b - b;
#if 1
		/* speedy linear distance method*/
		sq = MWABS(R) + MWABS(G) + MWABS(B);
#else
		/* slower distance-cubed with luminance adjustment*/
		/* gray is .30R + .59G + .11B*/
		/* = (R*77 + G*151 + B*28)/256*/
		sq = (int32_t)R*R*30*30 + (int32_t)G*G*59*59 + (int32_t)B*B*11*11;
#endif

		if(sq < diff) {
			best = rgb - pal;
			if((diff = sq) == 0)
				return best;
		}
	}
	return best;
}
#endif /* MW_FEATURE_PALETTE*/

/**
 * Convert a palette-independent value to a hardware color
 *
 * @param psd Screen device
 * @param c 24-bit RGB color.
 * @return Hardware-specific color.
 */
MWPIXELVAL
GdFindColor(PSD psd, MWCOLORVAL c)
{
	/*
	 * Handle truecolor displays.
	 */
	switch(psd->pixtype) {
	case MWPF_TRUECOLOR8888:
		/* create 32 bit ARGB pixel (0xAARRGGBB) from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXEL8888(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXEL8888(c);

	case MWPF_TRUECOLORABGR:
		/* create 32 bit ABGR pixel (0xAABBGGRR) from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXELABGR(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXELABGR(c);

	case MWPF_TRUECOLOR888:
		/* create 24 bit 0RGB pixel (0x00RRGGBB) from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXEL888(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXEL888(c);

	case MWPF_TRUECOLOR565:
		/* create 16 bit RGB5/6/5 format pixel from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXEL565(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXEL565(c);

	case MWPF_TRUECOLOR555:
		/* create 16 bit RGB5/5/5 format pixel from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXEL555(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXEL555(c);

	case MWPF_TRUECOLOR1555:
		/* create 16 bit RGB5/5/5 format pixel from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXEL1555(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXEL1555(c);

	case MWPF_TRUECOLOR332:
		/* create 8 bit RGB3/3/2 format pixel from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXEL332(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXEL332(c);
	case MWPF_TRUECOLOR233:
		/* create 8 bit BGR2/3/3 format pixel from ABGR colorval (0xAABBGGRR)*/
		/*RGB2PIXEL332(REDVALUE(c), GREENVALUE(c), BLUEVALUE(c))*/
		return COLOR2PIXEL233(c);
        }

	/* case MWPF_PALETTE: must be running 1, 2, 4 or 8 bit palette*/

	/* handle 1bpp pixmaps, not running in palette mode*/
	if (psd->ncolors == 2 && scrdev.pixtype != MWPF_PALETTE)
		return c & 1;

#if MW_FEATURE_PALETTE
	/* search palette for closest match*/
	return GdFindNearestColor(gr_palette, (int)psd->ncolors, c);
#else
	return 0;
#endif
}

/**
 * Convert a color from a driver-dependent PIXELVAL to a COLORVAL.
 *
 * @param psd Screen device.
 * @param pixel Hardware-specific color.
 * @return 24-bit RGB color.
 */
MWCOLORVAL
GdGetColorRGB(PSD psd, MWPIXELVAL pixel)
{
	switch (psd->pixtype) {
	case MWPF_TRUECOLOR8888:
		return PIXEL8888TOCOLORVAL(pixel);

	case MWPF_TRUECOLORABGR:
		return PIXELABGRTOCOLORVAL(pixel);

	case MWPF_TRUECOLOR888:
		return PIXEL888TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR565:
		return PIXEL565TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR555:
		return PIXEL555TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR1555:
 	        return PIXEL1555TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR332:
		return PIXEL332TOCOLORVAL(pixel);

	case MWPF_TRUECOLOR233:
		return PIXEL233TOCOLORVAL(pixel);

#if MW_FEATURE_PALETTE
	case MWPF_PALETTE:
		return GETPALENTRY(gr_palette, pixel);
#endif
	}
	return 0;
}

#if DEBUG
void GdPrintBitmap(PMWBLITPARMS gc, int SSZ)
{
	unsigned char *src;
	int height;
	unsigned int v;

	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + gc->srcx * SSZ;

	DPRINTF("Image %d,%d SSZ %d\n", gc->width, gc->height, SSZ);
	height = gc->height;
	while (--height >= 0)
	{
		register unsigned char *s = src;
		int w = gc->width;

		while (--w >= 0)
		{
			switch (SSZ) {
			case 2:
				v = s[0] | (s[1] << 8);
				v = PIXEL565RED(v) + PIXEL565GREEN(v) + PIXEL565BLUE(v);
				DPRINTF("%c", "_.:;oVM@X"[v]);
				break;
			case 3:
				v = (s[0] + s[1] + s[2]) / 3;
				DPRINTF("%c", "_.:;oVM@X"[v >> 5]);
				break;
			case 4:
				//if (s[4])
					v = (s[0] + s[1] + s[2]) / 3;
				//else v = 256;
				DPRINTF("%c", "_.:;oVM@X"[v >> 5]);
				break;
			}
			s += SSZ;				/* src: next pixel right*/
		}
		DPRINTF("\n");
		src += gc->src_pitch;		/* src: next line down*/
	}
}
#endif /* DEBUG*/
