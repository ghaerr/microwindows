/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows null font stub
 */
#include <stdio.h>
#include "device.h"
#include "genfont.h"

/* no compiled in fonts*/

static MWCFONT nullfont = {
	"", 0, 0, 0, 0, 0, NULL, NULL, NULL
};

/* handling routines for MWCOREFONT*/
MWFONTPROCS mwfontprocs = {
	0,				/* capabilities*/
	MWTF_ASCII,		/* routines expect ascii*/
	NULL,			/* init*/
	NULL,			/* createfont*/
	NULL,			/* getfontinfo*/
	NULL,			/* gettextsize*/
	NULL,			/* gettextbits*/
	NULL,			/* unloadfont*/
	NULL,			/* xxx_drawtext*/
	NULL,			/* setfontsize*/
	NULL,			/* setfontrotation*/
	NULL,			/* setfontattr*/
	NULL			/* duplicate*/
};

/* first font is default font if no match*/
MWCOREFONT gen_fonts[NUMBER_FONTS] = {
	{&mwfontprocs, 0, 0, 0, 0, MWFONT_SYSTEM_VAR, &nullfont},
	{&mwfontprocs, 0, 0, 0, 0, MWFONT_SYSTEM_VAR, &nullfont},
	{&mwfontprocs, 0, 0, 0, 0, MWFONT_SYSTEM_VAR, &nullfont},
	{&mwfontprocs, 0, 0, 0, 0, MWFONT_SYSTEM_VAR, &nullfont},
};
