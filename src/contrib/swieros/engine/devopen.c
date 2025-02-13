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

/*
 * The following define can change depending on the window manager
 * usage of colors and layout of the 8bpp palette devpal8.c.
 * Color entries below this value won't be overwritten by user
 * programs or bitmap display conversion tables.
 */
MWPIXELVAL gr_foreground;	/* current foreground color */
MWPIXELVAL gr_background;	/* current background color */
MWBOOL 	gr_usebg;    	    /* TRUE if background drawn in pixmaps */
int 	gr_mode = MWROP_COPY; 	    /* drawing mode */
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

	/* init local vars*/
	GdSetMode(MWROP_COPY);
	GdSetFillMode(MWFILL_SOLID);  /* Set the fill mode to solid */

	GdSetForegroundColor(psd, MWRGB(255, 255, 255));	/* WHITE*/
	GdSetBackgroundColor(psd, MWRGB(0, 0, 0));		/* BLACK*/
	GdSetUseBackground(TRUE);
	/* select first builtin font (usually MWFONT_SYSTEM_VAR)*/
	//GdSetFont(GdCreateFont(psd, NULL, 0, 0, NULL));

	GdSetDash(0, 0);  /* No dashing to start */

	GdSetClipRects(psd, 0, NULL);

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
	//switch(psd->pixtype) {
	//case MWPF_TRUECOLOR8888:
		/* create 32 bit ARGB pixel (0xAARRGGBB) from ABGR colorval (0xAABBGGRR)*/
		//return COLOR2PIXEL8888(c);
		return ((((c) & 0xff) << 16) | ((c) & 0xff00ff00ul) | (((c) & 0xff0000) >> 16));
	//}

	/* handle 1bpp pixmaps, not running in palette mode*/
	//if (psd->ncolors == 2 && scrdev.pixtype != MWPF_PALETTE)
		//return c & 1;
	//return 0;
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
	//switch (psd->pixtype) {
	//case MWPF_TRUECOLOR8888:
		//return PIXEL8888TOCOLORVAL(pixel);
		return (((pixel & 0xff0000ul) >> 16) | (pixel & 0xff00ff00ul) | ((pixel & 0xffu) << 16) | 0xff000000ul);
	//}
	return 0;
}
