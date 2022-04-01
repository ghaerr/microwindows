/*
 * Frame Buffer Emulator
 * 	1, 2, 4, 8, 15, 16, 24 and 32bpp supported
 *	MSB and LSB first bit order supported for 1, 2 and 4bpp
 *
 * Copyright (c) 2010, 2019 by Greg Haerr <greg@censoft.com>
 * Copyright (c) 2004 by Vince Busam <vince@sixpak.org>
 * Copyright (c) 2000 by Thomas Gallenkamp <tgkamp@users.sourceforge.net>
 * Credits: Used the X11/Xlib demo application xscdemo v2.2b 
 *          (c) 1992 by Sudarshan Karkada, as a starting point for this program.
 *
 * 4/27/2010 modified g haerr for microwindows from fbe.c 1.4 12/03/2010
 *			added support for fblin1rev.c reverse bit order 1bpp (-r)
 *			added support for 4bpp gray palette (-g)
 *			added support for 2, 4, 15, 16, 24 and 32bpp
 * 5/19/2019 added mouse and keyboard fifo drivers, fix screen draw bugs
 *
 * Original from picoTK project
 * 
 * Instructions:
 * This is a frame buffer emulator on X11.
 * To use set SCREEN=FBE in the src/config file and run "bin/fbe".
 *
 * Can also be used with SCREEN=FB (linux framebuffer):
 *   To run set the environment variable: FRAMEBUFFER=/tmp/fb0 
 *   and execute: bin/fbe -d<bpp>
 *   a reverse bit order is supported for 1,2,4 bpp (-r option)
 *
 *   see this help text coded below:
 *     Usage:  fbe  [-<options>]
 *        Options:\n"
 *            -x   X size            [%3d]
 *            -y   Y Size            [%3d]
 *            -t   Total X Size      [%3d]
 *            -d   Color depths bpp  [%d] (1,2,4,8,15,16,24,32)
 *            -z   Zoom factor       [%d]
 *	          -r   Reverse bit order (1,2,4bpp LSB first)
 *	          -g   Gray palette (4bpp only)
 *	          -c   Force create new framebuffer
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
	TODO
	assumes 32bpp X server (fix)
	add 8bpp 3/3/2, 2/2/3 truecolor
	add 32bpp BGRA truecolor -d option
	kill nano-X on exit flag
	read/write colormap from nano-X
	get colormap in scr_fb.c
	get bpp via ioctl from fbe for nano-X?
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "../include/mwtypes.h"
#include "../include/device.h"

#define PROGNAME "fbe"
#define VERSION  "1.1"

#define MAX_CRTX 2048
#define MAX_CRTY 2048
#define CHUNKX 1024
#define CHUNKY 1024

#define INVERT2BPP	0	/* inverted palette for 2 bits/pixel*/

#if !defined(SCREEN_DEPTH) && (MWPIXEL_FORMAT == MWPF_PALETTE)
/* SCREEN_DEPTH is used only for palette modes*/
#error SCREEN_DEPTH not defined - must be set for palette modes
#endif

int CRTX = SCREEN_WIDTH;			/* default x size*/
int CRTY = SCREEN_HEIGHT;			/* default y size*/
int BITS_PER_PIXEL;					/* bpp set by MWPIXEL_FORMAT (SCREEN_PIXTYPE)*/

/* flags*/
int ZOOM = 1;			/* integral zoom factor*/
int force_create = 0;	/* force create new framebuffer*/
int rev_bitorder = 0;	/* reverse bit order, for fblinXrev.c drivers*/
int gray_palette = 0;	/* use gray palette in 4bpp*/
int redocmap = 0;

int CRTX_TOTAL = 0;
int PITCH;
int PIXELS_PER_BYTE;
int PIXEL_MASK;
int PIXELS_PER_LONG;
char *host = NULL;
char *cmapbuf;
uint32_t *crtbuf;

Display *display;
Colormap colormap;
GC gc;
Window window, root, parent;
int depth, screen, visibility;
int repaint;
Pixmap pixmap;
int mouse_fd;
int keyboard_fd;

uint32_t crcs[(MAX_CRTX + CHUNKX - 1) / CHUNKX][(MAX_CRTY + CHUNKY - 1) / CHUNKY];
uint32_t colors_X11[256];	/* contains X11 variants, either 8,16 or 24 bits */
uint32_t colors_24[256];	/* contains 24 bit (00rrggbb) rgb color mappings */

void X11_init(void);

#define RGBDEF(r,g,b)	((uint32_t) ((b) | ((g)<<8) | ((r)<<16)))

/* return 5/6/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL565RED(pixelval)		(((pixelval) >> 11) & 0x1f)
#define PIXEL565GREEN(pixelval)		(((pixelval) >> 5) & 0x3f)
#define PIXEL565BLUE(pixelval)		((pixelval) & 0x1f)

/* return 5/5/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL555RED(pixelval)		(((pixelval) >> 10) & 0x1f)
#define PIXEL555GREEN(pixelval)		(((pixelval) >> 5) & 0x1f)
#define PIXEL555BLUE(pixelval)		((pixelval) & 0x1f)

/* Standard palette for 1bpp (2 color/monochrome) systems. */
uint32_t mwstdpal1[2] = {
	RGBDEF( 0  , 0  , 0   ),	/* black*/
	RGBDEF( 255, 255, 255 )		/* white*/
};

/* Standard palette for 2bpp (4 color) systems. */
uint32_t mwstdpal2[4] = {
#if INVERT2BPP
	RGBDEF( 255, 255, 255 ),	/* white*/
	RGBDEF( 192, 192, 192 ),	/* ltgray*/
	RGBDEF( 128, 128, 128 ),	/* gray*/
	RGBDEF( 0  , 0  , 0   )		/* black*/
#else
	RGBDEF( 0  , 0  , 0   ),	/* black*/
	RGBDEF( 128, 128, 128 ),	/* gray*/
	RGBDEF( 192, 192, 192 ),	/* ltgray*/
	RGBDEF( 255, 255, 255 )		/* white*/
#endif
};

/* Standard palette for 4bpp (16 color) systems. */
uint32_t mwstdpal4[16] = {
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

/* Gray palette for 4bpp (16 color) systems. */
uint32_t mwstdpal4gray[16] = {
    RGBDEF( 0, 0, 0 ),
    RGBDEF( 17, 17, 17 ),
    RGBDEF( 34, 34, 34 ),
    RGBDEF( 51, 51, 51 ),
    RGBDEF( 68, 68, 68 ),
    RGBDEF( 85, 85, 85 ),
    RGBDEF( 102, 102, 102 ),
    RGBDEF( 119, 119, 119 ),
    RGBDEF( 136, 136, 136 ),
    RGBDEF( 153, 153, 153 ),
    RGBDEF( 170, 170, 170 ),
    RGBDEF( 187, 187, 187 ),
    RGBDEF( 204, 204, 204 ),
    RGBDEF( 221, 221, 221 ),
    RGBDEF( 238, 238, 238 ),
    RGBDEF( 255, 255, 255 )
};

/*
 * Special palette for supporting 48 Windows colors and a 216 color
 * uniform color distribution.
 * Note: the first 20 colors are used internally as system colors.
 */
uint32_t mwstdpal8[256] = {
	/* 16 EGA colors, arranged for direct predefined palette indexing*/
	RGBDEF( 0  , 0  , 0   ),	/* black*/
	RGBDEF( 0  , 0  , 128 ),	/* blue*/
	RGBDEF( 0  , 128, 0   ),	/* green*/
	RGBDEF( 0  , 128, 128 ),	/* cyan*/ /* COLOR_BACKGROUND*/
	RGBDEF( 128, 0  , 0   ),	/* red*/  /* COLOR_ACTIVECAPTION A*/
	RGBDEF( 128, 0  , 128 ),	/* magenta*/ /* COLOR_ACTIVECAPTION B*/
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

	/* 32 basic windows colors (first 8 are most important)*/
	RGBDEF( 32 , 32 , 32  ),	/* DKGRAY_BRUSH*/
	RGBDEF( 128, 128, 0   ),	/* non-adjusted brown*/
	RGBDEF( 223, 223, 223 ),	/* COLOR_3DLIGHT B*/
	RGBDEF( 160, 160, 160 ), 	/* COLOR_3DLIGHT C*/

	RGBDEF( 234, 230, 221 ),	/* COLOR_BTNHIGHLIGHT A*/
	RGBDEF( 213, 204, 187 ),	/* COLOR_BTNFACE A*/
	RGBDEF( 162, 141, 104 ),	/* COLOR_BTNSHADOW A*/
	RGBDEF( 0  , 64 , 128 ),	/* COLOR_INACTIVECAPTION C*/
	/*RGBDEF( 0  , 0  , 64  ),*/
	/*RGBDEF( 0  , 64 , 0   ),*/
	/*RGBDEF( 0  , 64 , 64  ),*/
	/*RGBDEF( 0  , 128, 64  ),*/
	RGBDEF( 0  , 128, 255 ),
	RGBDEF( 0  , 255, 128 ),
	RGBDEF( 64 , 0  , 0   ),
	RGBDEF( 64 , 0  , 64  ),
	RGBDEF( 64 , 0  , 128 ),
	RGBDEF( 64 , 128, 128 ),
	RGBDEF( 128, 0  , 64  ),
	RGBDEF( 128, 0  , 255 ),
	RGBDEF( 128, 64 , 64  ),
	RGBDEF( 128, 128, 64  ),
	RGBDEF( 128, 128, 192 ),
	RGBDEF( 128, 128, 255 ),
	RGBDEF( 128, 255, 0   ),
	RGBDEF( 128, 255, 255 ),
	RGBDEF( 164, 200, 240 ),
	RGBDEF( 192, 220, 192 ),
	RGBDEF( 255, 0  , 128 ),
	RGBDEF( 255, 128, 0   ),
	RGBDEF( 255, 128, 192 ),
	RGBDEF( 255, 128, 255 ),
	RGBDEF( 255, 128, 128 ),
	RGBDEF( 255, 255, 128 ),
	RGBDEF( 255, 251, 240 ),
	RGBDEF( 255, 255, 232 ),

	/* 216 colors spread uniformly across rgb spectrum*/
	/* 8 colors removed that are duplicated above*/
	/*RGBDEF( 0x00, 0x00, 0x00 ),*/
	RGBDEF( 0x00, 0x00, 0x33 ),
	RGBDEF( 0x00, 0x00, 0x66 ),
	RGBDEF( 0x00, 0x00, 0x99 ),
	RGBDEF( 0x00, 0x00, 0xcc ),
	/*RGBDEF( 0x00, 0x00, 0xff ),*/
	RGBDEF( 0x33, 0x00, 0x00 ),
	RGBDEF( 0x33, 0x00, 0x33 ),
	RGBDEF( 0x33, 0x00, 0x66 ),
	RGBDEF( 0x33, 0x00, 0x99 ),
	RGBDEF( 0x33, 0x00, 0xcc ),
	RGBDEF( 0x33, 0x00, 0xff ),
	RGBDEF( 0x66, 0x00, 0x00 ),
	RGBDEF( 0x66, 0x00, 0x33 ),
	RGBDEF( 0x66, 0x00, 0x66 ),
	RGBDEF( 0x66, 0x00, 0x99 ),
	RGBDEF( 0x66, 0x00, 0xcc ),
	RGBDEF( 0x66, 0x00, 0xff ),
	RGBDEF( 0x99, 0x00, 0x00 ),
	RGBDEF( 0x99, 0x00, 0x33 ),
	RGBDEF( 0x99, 0x00, 0x66 ),
	RGBDEF( 0x99, 0x00, 0x99 ),
	RGBDEF( 0x99, 0x00, 0xcc ),
	RGBDEF( 0x99, 0x00, 0xff ),
	RGBDEF( 0xcc, 0x00, 0x00 ),
	RGBDEF( 0xcc, 0x00, 0x33 ),
	RGBDEF( 0xcc, 0x00, 0x66 ),
	RGBDEF( 0xcc, 0x00, 0x99 ),
	RGBDEF( 0xcc, 0x00, 0xcc ),
	RGBDEF( 0xcc, 0x00, 0xff ),
	/*RGBDEF( 0xff, 0x00, 0x00 ),*/
	RGBDEF( 0xff, 0x00, 0x33 ),
	RGBDEF( 0xff, 0x00, 0x66 ),
	RGBDEF( 0xff, 0x00, 0x99 ),
	RGBDEF( 0xff, 0x00, 0xcc ),
	/*RGBDEF( 0xff, 0x00, 0xff ),*/
	RGBDEF( 0x00, 0x33, 0x00 ),
	RGBDEF( 0x00, 0x33, 0x33 ),
	RGBDEF( 0x00, 0x33, 0x66 ),
	RGBDEF( 0x00, 0x33, 0x99 ),
	RGBDEF( 0x00, 0x33, 0xcc ),
	RGBDEF( 0x00, 0x33, 0xff ),
	RGBDEF( 0x33, 0x33, 0x00 ),
	RGBDEF( 0x33, 0x33, 0x33 ),
	RGBDEF( 0x33, 0x33, 0x66 ),
	RGBDEF( 0x33, 0x33, 0x99 ),
	RGBDEF( 0x33, 0x33, 0xcc ),
	RGBDEF( 0x33, 0x33, 0xff ),
	RGBDEF( 0x66, 0x33, 0x00 ),
	RGBDEF( 0x66, 0x33, 0x33 ),
	RGBDEF( 0x66, 0x33, 0x66 ),
	RGBDEF( 0x66, 0x33, 0x99 ),
	RGBDEF( 0x66, 0x33, 0xcc ),
	RGBDEF( 0x66, 0x33, 0xff ),
	RGBDEF( 0x99, 0x33, 0x00 ),
	RGBDEF( 0x99, 0x33, 0x33 ),
	RGBDEF( 0x99, 0x33, 0x66 ),
	RGBDEF( 0x99, 0x33, 0x99 ),
	RGBDEF( 0x99, 0x33, 0xcc ),
	RGBDEF( 0x99, 0x33, 0xff ),
	RGBDEF( 0xcc, 0x33, 0x00 ),
	RGBDEF( 0xcc, 0x33, 0x33 ),
	RGBDEF( 0xcc, 0x33, 0x66 ),
	RGBDEF( 0xcc, 0x33, 0x99 ),
	RGBDEF( 0xcc, 0x33, 0xcc ),
	RGBDEF( 0xcc, 0x33, 0xff ),
	RGBDEF( 0xff, 0x33, 0x00 ),
	RGBDEF( 0xff, 0x33, 0x33 ),
	RGBDEF( 0xff, 0x33, 0x66 ),
	RGBDEF( 0xff, 0x33, 0x99 ),
	RGBDEF( 0xff, 0x33, 0xcc ),
	RGBDEF( 0xff, 0x33, 0xff ),
	RGBDEF( 0x00, 0x66, 0x00 ),
	RGBDEF( 0x00, 0x66, 0x33 ),
	RGBDEF( 0x00, 0x66, 0x66 ),
	RGBDEF( 0x00, 0x66, 0x99 ),
	RGBDEF( 0x00, 0x66, 0xcc ),
	RGBDEF( 0x00, 0x66, 0xff ),
	RGBDEF( 0x33, 0x66, 0x00 ),
	RGBDEF( 0x33, 0x66, 0x33 ),
	RGBDEF( 0x33, 0x66, 0x66 ),
	RGBDEF( 0x33, 0x66, 0x99 ),
	RGBDEF( 0x33, 0x66, 0xcc ),
	RGBDEF( 0x33, 0x66, 0xff ),
	RGBDEF( 0x66, 0x66, 0x00 ),
	RGBDEF( 0x66, 0x66, 0x33 ),
	RGBDEF( 0x66, 0x66, 0x66 ),
	RGBDEF( 0x66, 0x66, 0x99 ),
	RGBDEF( 0x66, 0x66, 0xcc ),
	RGBDEF( 0x66, 0x66, 0xff ),
	RGBDEF( 0x99, 0x66, 0x00 ),
	RGBDEF( 0x99, 0x66, 0x33 ),
	RGBDEF( 0x99, 0x66, 0x66 ),
	RGBDEF( 0x99, 0x66, 0x99 ),
	RGBDEF( 0x99, 0x66, 0xcc ),
	RGBDEF( 0x99, 0x66, 0xff ),
	RGBDEF( 0xcc, 0x66, 0x00 ),
	RGBDEF( 0xcc, 0x66, 0x33 ),
	RGBDEF( 0xcc, 0x66, 0x66 ),
	RGBDEF( 0xcc, 0x66, 0x99 ),
	RGBDEF( 0xcc, 0x66, 0xcc ),
	RGBDEF( 0xcc, 0x66, 0xff ),
	RGBDEF( 0xff, 0x66, 0x00 ),
	RGBDEF( 0xff, 0x66, 0x33 ),
	RGBDEF( 0xff, 0x66, 0x66 ),
	RGBDEF( 0xff, 0x66, 0x99 ),
	RGBDEF( 0xff, 0x66, 0xcc ),
	RGBDEF( 0xff, 0x66, 0xff ),
	RGBDEF( 0x00, 0x99, 0x00 ),
	RGBDEF( 0x00, 0x99, 0x33 ),
	RGBDEF( 0x00, 0x99, 0x66 ),
	RGBDEF( 0x00, 0x99, 0x99 ),
	RGBDEF( 0x00, 0x99, 0xcc ),
	RGBDEF( 0x00, 0x99, 0xff ),
	RGBDEF( 0x33, 0x99, 0x00 ),
	RGBDEF( 0x33, 0x99, 0x33 ),
	RGBDEF( 0x33, 0x99, 0x66 ),
	RGBDEF( 0x33, 0x99, 0x99 ),
	RGBDEF( 0x33, 0x99, 0xcc ),
	RGBDEF( 0x33, 0x99, 0xff ),
	RGBDEF( 0x66, 0x99, 0x00 ),
	RGBDEF( 0x66, 0x99, 0x33 ),
	RGBDEF( 0x66, 0x99, 0x66 ),
	RGBDEF( 0x66, 0x99, 0x99 ),
	RGBDEF( 0x66, 0x99, 0xcc ),
	RGBDEF( 0x66, 0x99, 0xff ),
	RGBDEF( 0x99, 0x99, 0x00 ),
	RGBDEF( 0x99, 0x99, 0x33 ),
	RGBDEF( 0x99, 0x99, 0x66 ),
	RGBDEF( 0x99, 0x99, 0x99 ),
	RGBDEF( 0x99, 0x99, 0xcc ),
	RGBDEF( 0x99, 0x99, 0xff ),
	RGBDEF( 0xcc, 0x99, 0x00 ),
	RGBDEF( 0xcc, 0x99, 0x33 ),
	RGBDEF( 0xcc, 0x99, 0x66 ),
	RGBDEF( 0xcc, 0x99, 0x99 ),
	RGBDEF( 0xcc, 0x99, 0xcc ),
	RGBDEF( 0xcc, 0x99, 0xff ),
	RGBDEF( 0xff, 0x99, 0x00 ),
	RGBDEF( 0xff, 0x99, 0x33 ),
	RGBDEF( 0xff, 0x99, 0x66 ),
	RGBDEF( 0xff, 0x99, 0x99 ),
	RGBDEF( 0xff, 0x99, 0xcc ),
	RGBDEF( 0xff, 0x99, 0xff ),
	RGBDEF( 0x00, 0xcc, 0x00 ),
	RGBDEF( 0x00, 0xcc, 0x33 ),
	RGBDEF( 0x00, 0xcc, 0x66 ),
	RGBDEF( 0x00, 0xcc, 0x99 ),
	RGBDEF( 0x00, 0xcc, 0xcc ),
	RGBDEF( 0x00, 0xcc, 0xff ),
	RGBDEF( 0x33, 0xcc, 0x00 ),
	RGBDEF( 0x33, 0xcc, 0x33 ),
	RGBDEF( 0x33, 0xcc, 0x66 ),
	RGBDEF( 0x33, 0xcc, 0x99 ),
	RGBDEF( 0x33, 0xcc, 0xcc ),
	RGBDEF( 0x33, 0xcc, 0xff ),
	RGBDEF( 0x66, 0xcc, 0x00 ),
	RGBDEF( 0x66, 0xcc, 0x33 ),
	RGBDEF( 0x66, 0xcc, 0x66 ),
	RGBDEF( 0x66, 0xcc, 0x99 ),
	RGBDEF( 0x66, 0xcc, 0xcc ),
	RGBDEF( 0x66, 0xcc, 0xff ),
	RGBDEF( 0x99, 0xcc, 0x00 ),
	RGBDEF( 0x99, 0xcc, 0x33 ),
	RGBDEF( 0x99, 0xcc, 0x66 ),
	RGBDEF( 0x99, 0xcc, 0x99 ),
	RGBDEF( 0x99, 0xcc, 0xcc ),
	RGBDEF( 0x99, 0xcc, 0xff ),
	RGBDEF( 0xcc, 0xcc, 0x00 ),
	RGBDEF( 0xcc, 0xcc, 0x33 ),
	RGBDEF( 0xcc, 0xcc, 0x66 ),
	RGBDEF( 0xcc, 0xcc, 0x99 ),
	RGBDEF( 0xcc, 0xcc, 0xcc ),
	RGBDEF( 0xcc, 0xcc, 0xff ),
	RGBDEF( 0xff, 0xcc, 0x00 ),
	RGBDEF( 0xff, 0xcc, 0x33 ),
	RGBDEF( 0xff, 0xcc, 0x66 ),
	RGBDEF( 0xff, 0xcc, 0x99 ),
	RGBDEF( 0xff, 0xcc, 0xcc ),
	RGBDEF( 0xff, 0xcc, 0xff ),
	/*RGBDEF( 0x00, 0xff, 0x00 ),*/
	RGBDEF( 0x00, 0xff, 0x33 ),
	RGBDEF( 0x00, 0xff, 0x66 ),
	RGBDEF( 0x00, 0xff, 0x99 ),
	RGBDEF( 0x00, 0xff, 0xcc ),
	/*RGBDEF( 0x00, 0xff, 0xff ),*/
	RGBDEF( 0x33, 0xff, 0x00 ),
	RGBDEF( 0x33, 0xff, 0x33 ),
	RGBDEF( 0x33, 0xff, 0x66 ),
	RGBDEF( 0x33, 0xff, 0x99 ),
	RGBDEF( 0x33, 0xff, 0xcc ),
	RGBDEF( 0x33, 0xff, 0xff ),
	RGBDEF( 0x66, 0xff, 0x00 ),
	RGBDEF( 0x66, 0xff, 0x33 ),
	RGBDEF( 0x66, 0xff, 0x66 ),
	RGBDEF( 0x66, 0xff, 0x99 ),
	RGBDEF( 0x66, 0xff, 0xcc ),
	RGBDEF( 0x66, 0xff, 0xff ),
	RGBDEF( 0x99, 0xff, 0x00 ),
	RGBDEF( 0x99, 0xff, 0x33 ),
	RGBDEF( 0x99, 0xff, 0x66 ),
	RGBDEF( 0x99, 0xff, 0x99 ),
	RGBDEF( 0x99, 0xff, 0xcc ),
	RGBDEF( 0x99, 0xff, 0xff ),
	RGBDEF( 0xcc, 0xff, 0x00 ),
	RGBDEF( 0xcc, 0xff, 0x33 ),
	RGBDEF( 0xcc, 0xff, 0x66 ),
	RGBDEF( 0xcc, 0xff, 0x99 ),
	RGBDEF( 0xcc, 0xff, 0xcc ),
	RGBDEF( 0xcc, 0xff, 0xff ),
	/*RGBDEF( 0xff, 0xff, 0x00 ),*/
	RGBDEF( 0xff, 0xff, 0x33 ),
	RGBDEF( 0xff, 0xff, 0x66 ),
	RGBDEF( 0xff, 0xff, 0x99 ),
	RGBDEF( 0xff, 0xff, 0xcc ),
	/*RGBDEF( 0xff, 0xff, 0xff )*/
};

void
fbe_setcolors(void)
{
	int i;
	unsigned short c;
	unsigned short *cmap = (unsigned short *)cmapbuf;
	uint32_t *pal;

#if 0
	for (i = 0; i < 256; i++) {
		c = cmap[i];
		colors_24[i] = ((c & 0xF800) << 8) | (((c >> 5) & 0x002F) << 10) | ((c & 0x001F) << 3);
	}
#endif

	switch (BITS_PER_PIXEL) {
	case 1:
		pal = mwstdpal1;
		break;
	case 2:
		pal = mwstdpal2;
		break;
	case 4:
		pal = gray_palette? mwstdpal4gray: mwstdpal4;
		break;
	case 8:
		pal = mwstdpal8;
		break;
	default:
		return;
	}

	/* copy initial palette*/
	for (i=0; i < (1<<BITS_PER_PIXEL); ++i)
		colors_24[i] = pal[i];
}

void
fbe_calcX11colors(void)
{
	int i;
	uint32_t c24;

	if (BITS_PER_PIXEL > 8)
		return;

	/*
	 * Calculate X11 pixel values from 24 bit rgb (00rrggbb) color and
	 * store them in colors_X11 array for fast lookup. Mapping of rgb
	 * colors to pixel values depends on the X11 Server color depth 
	 */
	for (i = 0; i < 256; i++) {
		XColor xc;

		c24 = colors_24[i];

		xc.red =   ((c24 & 0xff0000) >> 16) * 0x0101;
		xc.green = ((c24 & 0x00ff00) >> 8) * 0x0101;
		xc.blue =   (c24 & 0x0000ff) * 0x0101;
		xc.flags = 0;
		XAllocColor(display, colormap, &xc);
		colors_X11[i] = xc.pixel;
	}
}

void
X11_init(void)
{
	XSetWindowAttributes attr;
	char name[80];
	XWMHints xwmhints;
	Pixmap cur_empty;
	Cursor cursor;
	XColor color;
	XSizeHints *sizehints;
	int width = CRTX * ZOOM;
	int height = CRTY * ZOOM;

	if (host == NULL) {
		if ((host = (char *) getenv("DISPLAY")) == NULL)
			fprintf(stderr, "%s", "Error: No environment variable DISPLAY\n");
	}
	if ((display = XOpenDisplay(host)) == NULL) {
		fprintf(stderr, "Error: Connection could not be made.\n");
		exit(1);
	}

	screen = DefaultScreen(display);
	colormap = DefaultColormap(display, screen);
	parent = root = RootWindow(display, screen);
	depth = DefaultDepth(display, screen);

	XSelectInput(display, root, SubstructureNotifyMask);

	attr.background_pixel = BlackPixel(display, screen);

	window = XCreateWindow(display, root, 0, 0, width, height,
		0, depth, InputOutput, DefaultVisual(display, screen), CWBackPixel, &attr);

	sizehints = XAllocSizeHints();
	if (sizehints != NULL) {
		sizehints->flags = PMinSize | PMaxSize;
		sizehints->min_width = width;
		sizehints->min_height = height;
		sizehints->max_width = width;
		sizehints->max_height = height;
		XSetWMNormalHints(display, window, sizehints);
		XFree(sizehints);
	}

	//xwmhints.icon_pixmap = iconPixmap;
	xwmhints.initial_state = NormalState;
	xwmhints.flags = StateHint; /* | IconPixmapHint*/
	XSetWMHints(display, window, &xwmhints);

	sprintf(name, "fbe %dx%dx%dbpp", CRTX, CRTY, BITS_PER_PIXEL);
	XChangeProperty(display, window, XA_WM_NAME, XA_STRING, 8,
		PropModeReplace, (unsigned char *)name, strlen(name));

	XSelectInput(display, window, ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
		KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask);
	XMapWindow(display, window);

	gc = XCreateGC(display, window, 0, NULL);

	/* Create an empty (invisible) cursor.  This is because Microwindows will draw it's own cursor.*/
	cur_empty = XCreateBitmapFromData(display, window, "\0", 1, 1);
	cursor = XCreatePixmapCursor(display, cur_empty, cur_empty, &color, &color, 0, 0);
	XDefineCursor(display, window, cursor);

	XClearWindow(display, window);
	XSync(display, 0);
}

uint32_t
calc_patch_crc(int ix, int iy)
{
	uint32_t crc = 0x8154711;
	int x, y, off;

	switch (BITS_PER_PIXEL) {
	default:
		off = (ix * CHUNKX) / PIXELS_PER_LONG + iy * CHUNKY * (CRTX_TOTAL / PIXELS_PER_LONG);
		break;
	case 15:
	case 16:
		off = (ix * CHUNKX + iy * CHUNKY * CRTX_TOTAL) * 2;
		break;
	case 24:
		off = (ix * CHUNKX + iy * CHUNKY * CRTX_TOTAL) * 3;
		break;
	case 32:
		off = (ix * CHUNKX + iy * CHUNKY * CRTX_TOTAL) * 4;
		break;
	}

	for (x = 0; x < CHUNKX / PIXELS_PER_LONG; x++) {
		if (ix == CRTX/CHUNKX && x >= CRTX%CHUNKX)
			continue;
		for (y = 0; y < CHUNKY; y++) {
			uint32_t dat;

			if (iy == CRTY/CHUNKY && y >= CRTY%CHUNKY)
				continue;
			if (BITS_PER_PIXEL <= 8)
				dat = crtbuf[off + x + y*CRTX_TOTAL/PIXELS_PER_LONG];
			else {
				unsigned char *data;
				unsigned char a, r, g, b, h, l;

				switch (BITS_PER_PIXEL) {
				case 15:
				case 16:
					data = ((unsigned char *)crtbuf) + off + (x + y*CRTX_TOTAL)*2;
					l = *data++;
					h = *data;
					dat = l | (h<<16);
					break;
				case 24:
					data = ((unsigned char *)crtbuf) + off + (x + y*CRTX_TOTAL)*3;
					b = *data++;
					g = *data++;
					r = *data;
					dat = b | (g<<8) | (r<<16);
					break;
				case 32:
					data = ((unsigned char *)crtbuf) + off + (x + y*CRTX_TOTAL)*4;
					b = *data++;
					g = *data++;
					r = *data++;
					a = *data;
					dat = b | (g<<8) | (r<<16) | (a<<24);
					break;
				}
			}

			crc += (crc % 211 + dat);
			/* crc^=((crc^dat)<<1)^((dat&0x8000) ? 0x1048:0); */
			/* crc=(crc<<1)+((crc&0x80000000) ? 1:0);  */
		}
	}
	return crc;
}

void
check_and_paint(int ix, int iy)
{
	uint32_t crc;
	int x, y, off;
	int color;

	crc = calc_patch_crc(ix, iy);
	if (!repaint && crc == crcs[ix][iy])
		return;
	crcs[ix][iy] = crc;

	switch (BITS_PER_PIXEL) {
	default:
		off = ix * (CHUNKX / PIXELS_PER_BYTE) + iy * CHUNKY * (CRTX_TOTAL / PIXELS_PER_BYTE);
		break;
	case 15:
	case 16:
		off = (ix * CHUNKX + iy * CHUNKY * CRTX_TOTAL) * 2;
		break;
	case 24:
		off = (ix * CHUNKX + iy * CHUNKY * CRTX_TOTAL) * 3;
		break;
	case 32:
		off = (ix * CHUNKX + iy * CHUNKY * CRTX_TOTAL) * 4;
		break;
	}

	XSetForeground(display, gc, 0x000000);
	XFillRectangle(display, pixmap, gc, 0, 0, CHUNKX * ZOOM, CHUNKY * ZOOM);

	for (y = 0; y < CHUNKY; y++) {
		if (iy == CRTY/CHUNKY && y >= CRTY%CHUNKY)
			continue;
		for (x = 0; x < CHUNKX; x++) {
			if (ix == CRTX/CHUNKX && x >= CRTX%CHUNKX)
				continue;
			if (BITS_PER_PIXEL <= 8) {
				unsigned char data =
					((unsigned char *)crtbuf)[off + x/PIXELS_PER_BYTE + y*(CRTX_TOTAL/PIXELS_PER_BYTE)];

				if (rev_bitorder)
					color = (data >> 
						((x & (PIXELS_PER_BYTE-1))) * BITS_PER_PIXEL) & PIXEL_MASK;
				else
					color = (data >>
						(((PIXELS_PER_BYTE-1) - (x & (PIXELS_PER_BYTE-1)))*BITS_PER_PIXEL)) & PIXEL_MASK;
				XSetForeground(display, gc, colors_X11[color]);
			} else {
				unsigned char *data;
				unsigned char a, r, g, b, h, l;
				uint32_t dat;
				
				switch (BITS_PER_PIXEL) {
				case 15:
					data = ((unsigned char *)crtbuf) + off + (x + y*CRTX_TOTAL)*2;
					l = *data++;
					h = *data;
					dat = l | (h<<8);
					r = PIXEL555RED(dat) << 3;
					g = PIXEL555GREEN(dat) << 3;
					b = PIXEL555BLUE(dat) << 3;
					dat = b | (g<<8) | (r<<16);
					break;
				case 16:
					data = ((unsigned char *)crtbuf) + off + (x + y*CRTX_TOTAL)*2;
					l = *data++;
					h = *data;
					dat = l | (h<<8);
					r = PIXEL565RED(dat) << 3;
					g = PIXEL565GREEN(dat) << 2;
					b = PIXEL565BLUE(dat) << 3;
					dat = b | (g<<8) | (r<<16);
					break;
				case 24:
					data = ((unsigned char *)crtbuf) + off + (x + y*CRTX_TOTAL)*3;
					b = *data++;
					g = *data++;
					r = *data++;
					dat = b | (g<<8) | (r<<16);
					break;
				case 32:
					data = ((unsigned char *)crtbuf) + off + (x + y*CRTX_TOTAL)*4;
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
					r = *data++;
					g = *data++;
					b = *data++;
					a = *data;
#else /* MWPF_TRUECOLORARGB*/
					b = *data++;
					g = *data++;
					r = *data++;
					a = *data;
#endif
					dat = b | (g<<8) | (r<<16);
					break;
				}
				XSetForeground(display, gc, dat);
			}
			if (ZOOM > 1)
				XFillRectangle(display, pixmap, gc, x * ZOOM, y * ZOOM, ZOOM, ZOOM);
			else
				XDrawPoint(display, pixmap, gc, x, y);
		}
	}
	XCopyArea(display, pixmap, window, gc, 0, 0, CHUNKX * ZOOM, CHUNKY * ZOOM,
		ix * CHUNKX * ZOOM, iy * CHUNKY * ZOOM);
}

static void
handle_keyboard(XEvent *ev)
{
	MWKEY mwkey;
	MWKEYMOD key_modstate;
	char ignored_char;
	unsigned int mods;
	Window w;
	int i;
	char buf[8];

	KeySym sym = XKeycodeToKeysym(display, ev->xkey.keycode, 0);
	if (sym == NoSymbol)
	    return;

#define X_CAP_MASK 0x02
#define X_NUM_MASK 0x10
#define X_SCR_MASK 0x80
	/* calculate kbd modifiers*/
	key_modstate = mods = 0;
	XQueryPointer(display, window, &w, &w, &i, &i, &i, &i, &mods);
	if (mods & ControlMask)
		key_modstate |= MWKMOD_CTRL; 
	if (mods & ShiftMask)
		key_modstate |= MWKMOD_SHIFT;
	if (mods & (Mod1Mask | 0x2000))		/* OSX alt/option returns 0x2000*/
		key_modstate |= MWKMOD_ALT;
	if (mods & X_CAP_MASK)
		key_modstate |= MWKMOD_CAPS;
	if (mods & X_SCR_MASK)
		key_modstate |= MWKMOD_SCR;
	if (mods & X_NUM_MASK)
		key_modstate |= MWKMOD_NUM;

	/* map X11 keys to mwin keys*/
	switch (sym) {
	case XK_Escape:		mwkey = MWKEY_ESCAPE;		break;
	case XK_Delete:		mwkey = MWKEY_DELETE;		break;
	case XK_Home:		mwkey = MWKEY_HOME;			break;
	case XK_Left:		mwkey = MWKEY_LEFT;			break;
	case XK_Up:			mwkey = MWKEY_UP;			break;
	case XK_Right:		mwkey = MWKEY_RIGHT;		break;
	case XK_Down:		mwkey = MWKEY_DOWN;			break;
	case XK_Page_Up:	mwkey = MWKEY_PAGEUP;		break;
	case XK_Page_Down:	mwkey = MWKEY_PAGEDOWN;		break;
	case XK_End:		mwkey = MWKEY_END;			break;
	case XK_Insert:		mwkey = MWKEY_INSERT;		break;
	case XK_Pause:
	case XK_Break:
	case XK_F15:		mwkey = MWKEY_QUIT;			break;
	case XK_Print:
	case XK_Sys_Req:	mwkey = MWKEY_PRINT;		break;
	case XK_Menu:		mwkey = MWKEY_MENU;			break;
	case XK_Cancel:		mwkey = MWKEY_CANCEL;		break;
	case XK_KP_Enter:	mwkey = MWKEY_KP_ENTER;		break;
	case XK_KP_Home:	mwkey = MWKEY_KP7;			break;
	case XK_KP_Left:	mwkey = MWKEY_KP4;			break;
	case XK_KP_Up:		mwkey = MWKEY_KP8;			break;
	case XK_KP_Right:	mwkey = MWKEY_KP6;			break;
	case XK_KP_Down:	mwkey = MWKEY_KP2;			break;
	case XK_KP_Page_Up:	mwkey = MWKEY_KP9;			break;
	case XK_KP_Page_Down:mwkey = MWKEY_KP3;			break;
	case XK_KP_End:		mwkey = MWKEY_KP1;			break;
	case XK_KP_Insert:	mwkey = MWKEY_KP0;			break;
	case XK_KP_Delete:	mwkey = MWKEY_KP_PERIOD;	break;
	case XK_KP_Equal:	mwkey = MWKEY_KP_EQUALS;	break;
	case XK_KP_Multiply:mwkey = MWKEY_KP_MULTIPLY;	break;
	case XK_KP_Add:		mwkey = MWKEY_KP_PLUS;		break;
	case XK_KP_Subtract:mwkey = MWKEY_KP_MINUS;		break;
	case XK_KP_Decimal:	mwkey = MWKEY_KP_PERIOD;	break;
	case XK_KP_Divide:	mwkey = MWKEY_KP_DIVIDE;	break;
	case XK_KP_5:
	case XK_KP_Begin:	mwkey = MWKEY_KP5;			break;
	case XK_F1:			mwkey = MWKEY_F1;			break;
	case XK_F2:			mwkey = MWKEY_F2;			break;
	case XK_F3:			mwkey = MWKEY_F3;			break;
	case XK_F4:			mwkey = MWKEY_F4;			break;
	case XK_F5:			mwkey = MWKEY_F5;			break;
	case XK_F6:			mwkey = MWKEY_F6;			break;
	case XK_F7:			mwkey = MWKEY_F7;			break;
	case XK_F8:			mwkey = MWKEY_F8;			break;
	case XK_F9:			mwkey = MWKEY_F9;			break;
	case XK_F10:		mwkey = MWKEY_F10;			break;
	case XK_F11:		mwkey = MWKEY_F11;			break;
	case XK_F12:		mwkey = MWKEY_F12;			break;
	case XK_Shift_L:	mwkey = MWKEY_LSHIFT;		break;
	case XK_Shift_R:	mwkey = MWKEY_RSHIFT;		break;
	case XK_Control_L:	mwkey = MWKEY_LCTRL;		break;
	case XK_Control_R:	mwkey = MWKEY_RCTRL;		break;
	case XK_Mode_switch:/* MACOSX left or right option/alt*/
	case XK_Alt_L:		mwkey = MWKEY_LALT;			break;
	case XK_Alt_R:		mwkey = MWKEY_RALT;			break;
	case XK_Meta_L:
	case XK_Super_L:
	case XK_Hyper_L:	mwkey = MWKEY_LMETA;		break;
	case XK_Meta_R:
	case XK_Super_R:
	case XK_Hyper_R:	mwkey = MWKEY_RMETA;		break;
	case XK_BackSpace:	mwkey = MWKEY_BACKSPACE;	break;
	case XK_Tab:		mwkey = MWKEY_TAB;			break;
	case XK_Return:		mwkey = MWKEY_ENTER;		break;
	/* state modifiers*/
	case XK_Num_Lock:
		/* not sent, used only for state*/
		if (ev->xkey.type == KeyRelease)
			key_modstate ^= MWKMOD_NUM;
		return;
	case XK_Shift_Lock:
	case XK_Caps_Lock:
		/* not sent, used only for state*/
		if (ev->xkey.type == KeyRelease)
			key_modstate ^= MWKMOD_CAPS;
		return;
	case XK_Scroll_Lock:
		/* not sent, used only for state*/
		if (ev->xkey.type == KeyRelease)
			key_modstate ^= MWKMOD_SCR;
		return;
	default:
		if (sym & 0xFF00)
			fprintf(stderr, PROGNAME ": Unhandled X11 keysym: %04x\n", (int)sym);
		XLookupString(&ev->xkey, &ignored_char, 1, &sym, NULL );

		if (key_modstate & MWKMOD_CTRL)
			mwkey = sym & 0x1f;	/* Control code */ 
		else
			mwkey = sym & 0xff;	/* ASCII*/
		break;
	}

	if (key_modstate & MWKMOD_NUM) {
		switch (mwkey) {
		case MWKEY_KP0:
		case MWKEY_KP1:
		case MWKEY_KP2:
		case MWKEY_KP3:
		case MWKEY_KP4:
		case MWKEY_KP5:
		case MWKEY_KP6:
		case MWKEY_KP7:
		case MWKEY_KP8:
		case MWKEY_KP9:			mwkey = mwkey - MWKEY_KP0 + '0';	break;
		case MWKEY_KP_PERIOD:	mwkey = '.';						break;
		case MWKEY_KP_DIVIDE:	mwkey = '/';						break;
		case MWKEY_KP_MULTIPLY:	mwkey = '*';						break;
		case MWKEY_KP_MINUS:	mwkey = '-';						break;
		case MWKEY_KP_PLUS:		mwkey = '+';						break;
		case MWKEY_KP_ENTER:	mwkey = MWKEY_ENTER;				break;
		case MWKEY_KP_EQUALS:	mwkey = '-';						break;
		}
	}

	/* write FBE keyboard protocol of eight bytes*/
	buf[0] = 0xF5;
	buf[1] = (ev->xkey.type == KeyPress)? KBD_KEYPRESS : KBD_KEYRELEASE;
	buf[2] = mwkey;					/* MWKEY*/
	buf[3] = mwkey >> 8;
	buf[4] = key_modstate;			/* MWKEYMOD*/
	buf[5] = key_modstate >> 8;
	buf[6] = ev->xkey.keycode;		/* MWSCANCODE*/
	buf[7] = ev->xkey.keycode >> 8;

	//printf("key %d mods: 0x%x  scan: 0x%x  key: 0x%x\n", buf[1], key_modstate, ev->xkey.keycode, mwkey);

	if (write(keyboard_fd, buf, 8) != 8)
		fprintf(stderr, PROGNAME ": Keyboard write fail\n");
}

static void
handle_mouse(XEvent *ev)
{
	int button = 0;
	int released = 0;
	int x, y;
	char buf[6];

	switch (ev->type) {
	case MotionNotify:
		if (ev->xmotion.window != window)
			return;
		x = ev->xmotion.x;
		y = ev->xmotion.y;
		if (ev->xmotion.state & Button1Mask)
				button |= MWBUTTON_L;
		if (ev->xmotion.state & Button2Mask)
				button |= MWBUTTON_M;
		if (ev->xmotion.state & Button3Mask)
				button |= MWBUTTON_R;
		if (ev->xmotion.state & Button4Mask)
				button |= MWBUTTON_SCROLLUP;
		if (ev->xmotion.state & Button5Mask)
				button |= MWBUTTON_SCROLLDN;
		break;

	case ButtonPress:
		if (ev->xbutton.window != window)
			return;
		/* Get pressed button */
		if(ev->xbutton.button == 1)
			button = MWBUTTON_L;
		else if(ev->xbutton.button == 2)
			button = MWBUTTON_M;
		else if(ev->xbutton.button == 3)
			button = MWBUTTON_R;
		else if(ev->xbutton.button == 4)
			button = MWBUTTON_SCROLLUP;
		else if(ev->xbutton.button == 5)
			button = MWBUTTON_SCROLLDN;

		/* Get any other buttons that might be already held */
		if (ev->xbutton.state & Button1Mask)
			button |= MWBUTTON_L;
		if (ev->xbutton.state & Button2Mask)
			button |= MWBUTTON_M;
		if (ev->xbutton.state & Button3Mask)
			button |= MWBUTTON_R;
		if (ev->xbutton.state & Button4Mask)
			button |= MWBUTTON_SCROLLUP;
		if (ev->xbutton.state & Button5Mask)
			button |= MWBUTTON_SCROLLDN;
		x = ev->xbutton.x;
		y = ev->xbutton.y;
		break;

	case ButtonRelease:
		if (ev->xbutton.window != window)
			return;
		/* Get released button */
		if(ev->xbutton.button == 1)
			released = MWBUTTON_L;
		else if(ev->xbutton.button == 2)
			released = MWBUTTON_M;
		else if(ev->xbutton.button == 3)
			released = MWBUTTON_R;
		else if(ev->xbutton.button == 4)
			released = MWBUTTON_SCROLLUP;
		else if(ev->xbutton.button == 5)
			released = MWBUTTON_SCROLLDN;

		/* Get any other buttons that might be already held */
		if (ev->xbutton.state & Button1Mask)
			button |= MWBUTTON_L;
		if (ev->xbutton.state & Button2Mask)
			button |= MWBUTTON_M;
		if (ev->xbutton.state & Button3Mask)
			button |= MWBUTTON_R;
		if (ev->xbutton.state & Button4Mask)
			button |= MWBUTTON_SCROLLUP;
		if (ev->xbutton.state & Button5Mask)
			button |= MWBUTTON_SCROLLDN;

		/* We need to remove the released button from the button mask*/
		button &= ~released; 

		x = ev->xbutton.x;
		y = ev->xbutton.y;
		break;

	default:
		return;
	}

	/* adjust mouse position for zoom factor*/
	x /= ZOOM;
	y /= ZOOM;

	//printf("mouse %x %d,%d\n", button, x, y);

	/* write FBE mouse protocol of six bytes*/
	buf[0] = 0xF4;
	buf[1] = button;
	buf[2] = x;
	buf[3] = x >> 8;
	buf[4] = y;
	buf[5] = y >> 8;
	if (write(mouse_fd, buf, 6) != 6)
		fprintf(stderr, PROGNAME ": Mouse write fail\n");
}

static void
fbe_loop(void)
{
	pixmap = XCreatePixmap(display, window, CHUNKX * ZOOM, CHUNKY * ZOOM, depth);

	repaint = 1;
	while (1) {
		int x, y;

		/*
		   Check if to force complete repaint because of window 
		   expose event
		 */
		while ((XPending(display) > 0)) {
			XEvent event;
			XNextEvent(display, &event);
			switch (event.type) {
			case Expose:
				repaint = 1;
				break;
			case ConfigureNotify:
				if (event.xconfigure.window == window)
					repaint = 1;
				break;
			case MotionNotify:
			case ButtonPress:
			case ButtonRelease:
				handle_mouse(&event);
				break;
			case KeyPress:
			case KeyRelease:
				handle_keyboard(&event);
				break;
			}
		}

		/* 
		   Sample all chunks for changes in shared memory buffer and
		   eventually repaint individual chunks. Repaint everything if
		   repaint is true (see above)
		 */
		for (y = 0; y < (CRTY+CHUNKY-1) / CHUNKY; y++)
			for (x = 0; x < (CRTX+CHUNKX-1) / CHUNKX; x++)
				check_and_paint(x, y);
		repaint = 0;
		usleep(2000);

		/* re-set color map */
		if (redocmap) {
			fbe_setcolors();
			fbe_calcX11colors();
			redocmap = 0;
		}
	}
}

void
usr1_handler(int sig)
{
	redocmap = 1;
}

int
main(int argc, char **argv)
{
	int fd = -1, cfd;
	int i, extra, size;
	int leave, ok = 1, help, bpp = 0;
	char *arg, *argp, buf[64];
	FILE *fp;
	struct stat st;

	help = 0;
	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if (arg[0] == '-') {
			arg++;
			leave = 0;
			do
				switch (*arg) {
					/* Non Parameter options */
				case 'h':
					help = 1;
					break;
				case 'c':
					force_create = 1;
					break;
				case 'r':
					rev_bitorder = 1;
					break;
				case 'g':
					gray_palette = 1;
					break;
				default:
					leave = 1;
				}
			while (!leave && *(++arg));

			/* Prepare for parameter option */
			if (*arg) {
				if (arg[1])
					argp = arg + 1;
				else if (i < argc - 1 && argv[i + 1][0])
					argp = argv[++i];
				else if (strchr("xytdz", *arg)) {
					fprintf(stderr, PROGNAME ": Use option -%c with parameter\n", *arg);
					return 1;
				}
				ok = 0;
				switch (*arg) {
					/* Parameter options */
				case 'x':
					ok = sscanf(argp, "%d", &CRTX) == 1;
					break;
				case 'y':
					ok = sscanf(argp, "%d", &CRTY) == 1;
					break;
				case 't':
					ok = sscanf(argp, "%d", &CRTX_TOTAL) == 1;
					break;
				case 'd':
					ok = sscanf(argp, "%d", &bpp) == 1;
					break;
				case 'z':
					ok = sscanf(argp, "%d", &ZOOM) == 1;
					break;
				default:
					fprintf(stderr, PROGNAME ": Use of unrecognized option -%c\n", *arg);
					help = 1;
					ok = 1;
					break;
				}

				if (!ok) {
					fprintf(stderr, PROGNAME ": Illegal option parameter %s for option -%c\n", argp, *arg);
					return 1;
				}

			}
		}		/* else
				   fname = arg; */
	}

	/* use config SCREEN_PIXTYPE to set bpp unless -d<bpp> specified*/
	if (bpp)
		BITS_PER_PIXEL = bpp;
	else
	switch (MWPIXEL_FORMAT) {
	case MWPF_TRUECOLORARGB:
	case MWPF_TRUECOLORABGR:
	default:
		BITS_PER_PIXEL = 32;
		break;

	case MWPF_TRUECOLORRGB:
		BITS_PER_PIXEL = 24;
		break;

	case MWPF_TRUECOLOR565:
	case MWPF_TRUECOLOR555:
		BITS_PER_PIXEL = 16;
		break;

	case MWPF_TRUECOLOR332:
		BITS_PER_PIXEL = 8;
		break;

#if MWPIXEL_FORMAT == MWPF_PALETTE
	case MWPF_PALETTE:
		BITS_PER_PIXEL = SCREEN_DEPTH;
		break;
#endif
	}

	if (!CRTX_TOTAL)
		CRTX_TOTAL = CRTX;
	if (BITS_PER_PIXEL > 8) {
		PIXELS_PER_BYTE = 1;		/* not used*/
		PIXELS_PER_LONG = 1;		/* not used*/
		PITCH = CRTX_TOTAL * ((BITS_PER_PIXEL+1) / 8);	/* + 1 to make 15bpp work*/
	} else {
		PIXELS_PER_BYTE = (8 / BITS_PER_PIXEL);
		PIXELS_PER_LONG = PIXELS_PER_BYTE * 4;
		PITCH = CRTX_TOTAL / PIXELS_PER_BYTE;
		PIXEL_MASK = ((unsigned char *)"\x00\x01\x03\x07\x0f\x1f\x3f\x7f\xff")[BITS_PER_PIXEL];
	}

	if (!ok || help) {
		printf(PROGNAME " " VERSION " "
		       " Frame Buffer Emulator\n"
		       "\n"
		       "Usage: " PROGNAME " [-<options>]\n"
		       "   Options:\n"
		       "       -x   X size            [%3d]\n"
		       "       -y   Y Size            [%3d]\n"
		       "       -t   Total X Size      [%3d]\n"
		       "       -d   Color depths bpp  [%d] (1,2,4,8,15,16,24,32)\n"
		       "       -z   Zoom factor       [%d]\n"
			   "       -r   Reverse bit order (1,2,4bpp LSB first)\n"
			   "       -g   Gray palette (4bpp only)\n"
			   "       -c   Force create new framebuffer\n",
		       CRTX, CRTY, CRTX_TOTAL, BITS_PER_PIXEL, ZOOM);
		return 1;
	}

	printf("%dx%dx%dbpp pitch %d\n", CRTX, CRTY, BITS_PER_PIXEL, PITCH);

	/* Create virtual framebuffer and palette*/
	if (force_create) {
		unlink(MW_PATH_FBE_FRAMEBUFFER);
		unlink(MW_PATH_FBE_COLORMAP);
		unlink(MW_PATH_FBE_MOUSE);
		unlink(MW_PATH_FBE_KEYBOARD);
	}

	/* open and mmap virtual framebuffer, but recreate if missing or wrong size*/
	extra = getpagesize() - 1;
	size = ((CRTY * PITCH) + extra) & ~extra;		/* extend to page boundary*/
	if (!stat(MW_PATH_FBE_FRAMEBUFFER, &st) && st.st_size == size)
		fd = open(MW_PATH_FBE_FRAMEBUFFER, O_RDWR);
	if (fd < 0) {
		if ((fd = open(MW_PATH_FBE_FRAMEBUFFER, O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0) {
			fprintf(stderr, PROGNAME ": Can't create %s\n", MW_PATH_FBE_FRAMEBUFFER);
			exit(1);
		}
		lseek(fd, size - 1, SEEK_SET);
		write(fd, "", 1);
	}
	crtbuf = mmap(NULL, CRTY * PITCH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	/* open and mmap virtual palette*/
	cfd = open(MW_PATH_FBE_COLORMAP, O_RDWR);
	if (cfd < 0) {
		if ((cfd = open(MW_PATH_FBE_COLORMAP, O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0) {
			fprintf(stderr, PROGNAME ": Can't create %s\n", MW_PATH_FBE_COLORMAP);
			exit(1);
		}
		lseek(cfd, 511, SEEK_SET);
		write(cfd, "", 1);
	}
	cmapbuf = mmap(NULL, 512, PROT_READ | PROT_WRITE, MAP_SHARED, cfd, 0);
	close(cfd);
	signal(SIGUSR1, usr1_handler);

	/* ignore pipe signal sent on fifo writes when no readers*/
	signal(SIGPIPE, SIG_IGN);

	/* open mouse named pipe*/
	if (mkfifo(MW_PATH_FBE_MOUSE, 0666) < 0 && errno != EEXIST)
		fprintf(stderr, PROGNAME ": mkfifo %s error %d ('%s')\n", MW_PATH_FBE_MOUSE, errno, strerror(errno));
	if ((mouse_fd = open(MW_PATH_FBE_MOUSE, O_RDWR | O_NONBLOCK)) < 0)
		fprintf(stderr, PROGNAME ": open %s error %d ('%s')\n", MW_PATH_FBE_MOUSE, errno, strerror(errno));

	/* open keyboard named pipe*/
	if (mkfifo(MW_PATH_FBE_KEYBOARD, 0666) < 0 && errno != EEXIST)
		fprintf(stderr, PROGNAME ": mkfifo %s error %d ('%s')\n", MW_PATH_FBE_KEYBOARD, errno, strerror(errno));
	if ((keyboard_fd = open(MW_PATH_FBE_KEYBOARD, O_RDWR | O_NONBLOCK)) < 0)
		fprintf(stderr, PROGNAME ": open %s error %d ('%s')\n", MW_PATH_FBE_KEYBOARD, errno, strerror(errno));

	X11_init();
	fbe_setcolors();
	fbe_calcX11colors();

	fbe_loop();

	return 0;
}
