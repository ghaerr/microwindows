/*
 * Copyright (c) 2000, 2001, 2002, 2003 Greg Haerr <greg@censoft.com>
 *
 * Loadable font demo for Microwindows
 *
 * Loads MGL, HZK, T1LIB, FREETYPE, and PCF fonts
 * Must be recompiled when src/config changes
 */
#include <stdio.h>
#include <stdlib.h>
#if UNIX | DOS_DJGPP
#include <time.h>
#endif
#define MWINCLUDECOLORS
#include "nano-X.h"

#define CLIP_POLYGON	0	/* =1 for polygonal region test */
#define HZKBIG5		1	/* =1 for big5 encoding test with HZKFONT*/

#define WIDTH 		340
#define HEIGHT 		340

#if HAVE_EUCJP_SUPPORT
#define MAXFONTS 4
#define FONT1 "k12x10.fnt"
#define FONT2 "k12x12.fnt"
#define FONT3 "k16x16.fnt"
#define FONT4 "k24x24.fnt"
#define FONT5 ""
#elif HAVE_HZK_SUPPORT
#define MAXFONTS 1
#define FONT1 "HZXFONT"
#define FONT2 ""
#define FONT3 ""
#define FONT4 ""
#define FONT5 ""
#elif HAVE_T1LIB_SUPPORT
#define MAXFONTS 5
#define FONT1 "bchr"
#define FONT2 "bchb"
#define FONT3 "dcr10"
#define FONT4 "dcbx10"
#define FONT5 "bchri"
#elif HAVE_FREETYPE_SUPPORT
#define MAXFONTS 5
#define FONT1 "lt1-r-omega-serif"
#define FONT2 "arial"
#define FONT3 "times"
#define FONT4 "cour"
#define FONT5 "timesi"
#elif HAVE_PCF_SUPPORT
#define MAXFONTS 5
#define FONT1 "/usr/lib/X11/fonts/misc/7x14.pcf.gz"
#define FONT2 "/usr/lib/X11/fonts/misc/6x13.pcf.gz"
#define FONT3 "/usr/lib/X11/fonts/misc/9x15.pcf.gz"
#define FONT4 "/usr/lib/X11/fonts/misc/vga.pcf.gz"
#define FONT5 "/usr/lib/X11/fonts/100dpi/helvB12.pcf.gz"
#elif HAVE_FNT_SUPPORT
#define MAXFONTS 2
#define FONT1 "/tmp/helvB12.fnt"
#define FONT2 "/tmp/clR6x8.fnt"
#define FONT3 ""
#define FONT4 ""
#define FONT5 ""
#else
#define MAXFONTS 5
#define FONT1 ""
#define FONT2 ""
#define FONT3 ""
#define FONT4 ""
#define FONT5 ""
#endif

#define RAND(max)	((int) (((float)(max)) * rand() / (RAND_MAX + 1.0)))

static char *names[5] = { FONT1, FONT2, FONT3, FONT4, FONT5 };

int
main(int ac, char **av)
{
	GR_WINDOW_ID window;
	GR_GC_ID gc;
	GR_FONT_ID fontid;
	int x, y;
	GR_REGION_ID regionid;
#if CLIP_POLYGON
	GR_POINT points[] = { {20, 20}, {300, 20}, {300, 300}, {20, 300} };
#else
	GR_RECT clip_rect = { 20, 20, 300, 300 };
#endif

	if (GrOpen() < 0)
		exit(1);

	window = GrNewWindowEx(GR_WM_PROPS_APPWINDOW,
		"t1demo loadable fonts (truetype, t1lib, pcf, mgl, hzk)",
		GR_ROOT_WINDOW_ID, 50, 50, WIDTH, HEIGHT, BLACK);
	GrSelectEvents(window,
		GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(window);

	gc = GrNewGC();
	GrSetGCUseBackground(gc, GR_FALSE);
	GrSetGCBackground(gc, BLACK);

#if CLIP_POLYGON
	/* polygon clip region */
	regionid = GrNewPolygonRegion(MWPOLY_EVENODD, 3, points);
#else
	/* rectangle clip region */
	regionid = GrNewRegion();
	GrUnionRectWithRegion(regionid, &clip_rect);
#endif
	GrSetGCRegion(gc, regionid);

	srand(time(0));
	while (1) {
		GR_EVENT event;

		GrCheckNextEvent(&event);
		if (event.type == GR_EVENT_TYPE_CLOSE_REQ) {
			GrClose();
			exit(0);
		}

		fontid = GrCreateFont(names[RAND(MAXFONTS)], 20, NULL);
		GrSetFontSize(fontid, RAND(80) + 1);
		GrSetFontRotation(fontid, 330);		/* 33 degrees */
		GrSetFontAttr(fontid, GR_TFKERNING | GR_TFANTIALIAS, 0);
		GrSetGCFont(gc, fontid);

		GrSetGCForeground(gc, rand() & 0xffffff);
		/*GrSetGCBackground(gc, rand() & 0xffffff); */

		x = RAND(WIDTH);
		y = RAND(HEIGHT);

#if HAVE_HZK_SUPPORT
		{
#if HZKBIG5
		/* hzk big5 unicode-16 test*/
		static unsigned short buffer[] = {
		    0x9060, 0x898b, 0x79d1, 0x6280, 0x0061, 0x0041, 0
		};
		GrText(window, gc, x, y, buffer, 7, GR_TFUC16);

		/* hzk big5 dbcs test #1*/
		x = RAND(WIDTH);
		y = RAND(HEIGHT);
		GrText(window, gc, x, y,
		       "Microwindows,舧ㄏノい璣ゅ翴皚砰", -1, GR_TFASCII);

		/* hzk big5 dbcs test #2*/
		x = RAND(WIDTH);
		y = RAND(HEIGHT);
		GrText(window, gc, x, y, "８９：", -1, GR_TFASCII);
#else
	#if 0
		/* hzk test #1*/
		static char buffer[] = {
			0x6c, 0x49, 0x73, 0x8b, 0x79,
			0xd1, 0x62, 0x80, 0x61, 0x00,
			0x41, 0x00, 0x00, 0xa1, 0x00,
			0xa6, 0x6c, 0x49, 0, 0
		};

		/***static unsigned short buffer[] = {
			0x496c, 0x8b73, 0xd179, 0x8062, 0x0061,
			0x0041, 0xa100, 0xa600, 0x496c, 0
		};***/

		GrText(window, gc, x, y, buffer, 9, GR_TFUC16);
	#endif
		/* HZK Metrix font test, includes Chinese and English */
		x = RAND(WIDTH);
		y = RAND(HEIGHT);
		GrText(window, gc, x, y,
		       "Microwindows,欢迎使用中英文点阵字体", -1, GR_TFASCII);
#endif /* HZKBIG5*/
		}
#elif HAVE_BIG5_SUPPORT
		/* encoding BIG5 test 61 B1 64 B1 64 61 */
		GrText(window, gc, x, y, "\151\261\144\261\144\151", 6, MWTF_DBCS_BIG5);
#elif HAVE_GB2312_SUPPORT
		/* encoding GB2312 test BD A1 BD A1 */
		GrText(window, gc, x, y, "\275\241\275\241", 4, MWTF_DBCS_GB);
#elif HAVE_JISX0213_SUPPORT
		/* encoding JISX0213 test A2 A1 */
		GrText(window, gc, x, y, "\242\241", 2, MWTF_DBCS_JIS);
#elif HAVE_KSC5601_SUPPORT
		/* encoding KSC5601 test B0 B0 */
		GrText(window, gc, x, y, "\273\273", 2, MWTF_DBCS_KSC);
#else
		/* ASCII test */
		GrText(window, gc, x, y, "Microwindows", -1, GR_TFASCII);
#endif
		GrFlush();
		GrDestroyFont(fontid);
	}
	GrDestroyRegion(regionid);
	GrClose();
	return 0;
}
