/*
 * Copyright (c) 1999, 2005 Greg Haerr <greg@censoft.com>
 *
 * 4bpp (16 color) standard palette definition
 */
#include "device.h"

#if 1
/*
 * Standard palette for 16 color systems.
 */
const MWPALENTRY mwstdpal4[16] = {
	/* 16 EGA colors, arranged in VGA standard palette order*/
	RGBDEF( 0  , 0  , 0   ),	/* black*/
	RGBDEF( 0  , 0  , 128 ),	/* blue*/
	RGBDEF( 0  , 128, 0   ),	/* green*/
	RGBDEF( 0  , 128, 128 ),	/* cyan*/
	RGBDEF( 128, 0  , 0   ),	/* red*/
	RGBDEF( 128, 0  , 128 ),	/* magenta*/
	RGBDEF( 128, 64 , 0   ),	/* adjusted brown*/
	RGBDEF( 192, 192, 192 ),	/* ltgray*/
	RGBDEF( 128, 128, 128 ),	/* gray*/
	RGBDEF( 0  , 0  , 255 ),	/* ltblue*/
	RGBDEF( 0  , 255, 0   ),	/* ltgreen*/
	RGBDEF( 0  , 255, 255 ),	/* ltcyan*/
	RGBDEF( 255, 0  , 0   ),	/* ltred*/
	RGBDEF( 255, 0  , 255 ),	/* ltmagenta*/
	RGBDEF( 255, 255, 0   ),	/* yellow*/
	RGBDEF( 255, 255, 255 ),	/* white*/
};

#else


/*
 * CGA palette for 16 color systems.
 */
const MWPALENTRY mwstdpal4[16] = {
	/* 16 CGA colors, arranged in CGA standard palette order*/
    /* NOTE: these RGB values don't match the BLACK/RED/BLUE etc colors in mwtypes.h */
	RGBDEF( 0   , 0   , 0    ),	/* black*/
	RGBDEF( 0   , 0   , 0xAA ),	/* blue*/
	RGBDEF( 0   , 0xAA, 0    ),	/* green*/
	RGBDEF( 0   , 0xAA, 0xAA ),	/* cyan*/
	RGBDEF( 0xAA, 0   , 0    ),	/* red*/
	RGBDEF( 0xAA, 0   , 0xAA ),	/* magenta*/
	RGBDEF( 0xAA, 0x55, 0    ),	/* brown*/
	RGBDEF( 0xAA, 0xAA, 0xAA ),	/* ltgray*/
	RGBDEF( 0x55, 0x55, 0x55 ),	/* gray*/
	RGBDEF( 0x55, 0x55, 0xFF ),	/* ltblue*/
	RGBDEF( 0x55, 0xFF, 0x55 ),	/* ltgreen*/
	RGBDEF( 0x55, 0xFF, 0xFF ),	/* ltcyan*/
	RGBDEF( 0xFF, 0x55, 0x55 ),	/* ltred*/
	RGBDEF( 0xFF, 0x55, 0xFF ),	/* ltmagenta*/
	RGBDEF( 0xFF, 0xFF, 0x55 ),	/* yellow*/
	RGBDEF( 0xFF, 0xFF, 0xFF ),	/* white*/
};
#endif
