/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2019 Greg Haerr <greg@censoft.com>
 *
 * demo-font: Microwindows font demonstration program
 * Displays examples of all configured fonts, dynamically loaded and compiled-in
 *
 * Use up/down arrow to speed up/slow down, c to clear, r region clip, q to quit
 * 
 * Loads FNT, PCF, FREETYPE, T1LIB, MGL, HZK and HBF fonts
 * Must be recompiled when src/config changes
 */
#include <stdio.h>
#include <stdlib.h>
#if UNIX | DOS_DJGPP | EMSCRIPTEN
#include <time.h>
#endif
#define MWINCLUDECOLORS
#include "nano-X.h"
#pragma GCC diagnostic ignored "-Winvalid-source-encoding"

#define WIDTH 		340
#define HEIGHT 		340

#define RAND(max)	((int) (((float)(max)) * rand() / (RAND_MAX + 1.0)))

/* fontlist table structure*/
typedef struct {
	int group;
	char *fontname;
	int	encoding;
	void *text;
	int textlen;
} textdata;

/* font types by group*/
char *groupname[] = {
	"Truetype TTF",
	"X11 PCF",
	"Mwin FNT",
	"EUCJP MGL",
	"Adobe Type1 AFM/PFB",
	"Hanzi Bitmap Format HBF",
	"Han Zi Ku HZK",
	"DBCS Big5",
	"DBCS GB2312",
	"DBCS JISX0213",
	"DBCS KSC5601"
};

/* UC16 encoded text strings*/
unsigned short jiskan24[] =	{ 0x213a, 0x213b, 0x2170, 0x2276, 0x2339 };	/* japanese*/
unsigned short gb24st[] =	{ 0x2129, 0x212a, 0x212b, 0x212c, 0x212d };	/* chinese*/
//unsigned short gb24st[] =	{ 0x7765, 0x7766, 0x7767, 0x777a, 0x777e };	/* broken*/
unsigned short gbk16_xke[] = { 0x8144, 0x8147, 0x8148, 0xfe4e, 0xfe4f };/* chinese*/
unsigned short hzk_uc161[] = { 0x9060, 0x898b, 0x79d1, 0x6280, 0x0061, 0x0041, 0 };
unsigned short hzk_uc162[] = { 0x496c, 0x8b73, 0xd179, 0x8062, 0x0061,
							   0x0041, 0xa100, 0xa600, 0x496c, 0 };

/* HZK BIG5 encoded strings (pass as MWTF_ASCII)*/
unsigned char hzk_asc1[] = { "Microwindows,Åwªï¨Ï¥Î¤¤­^¤åÂI°}¦rÅé" };
unsigned char hzk_asc2[] = { "£t£u£v£w£¸£¹£º" };
unsigned char hzk_asc3[] = { "Microwindows,»¶Ó­Ê¹ÓÃÖÐÓ¢ÎÄµãÕó×ÖÌå" };

/* DBCS EUCJP encoded string*/
unsigned char dbcs_eucjp[] =
	{ "ï¿½Þ¥ï¿½ï¿½ï¿½ï¿½í¥¦ï¿½ï¿½ï¿½ï¿½ï¿½É¥ï¿½ï¿½ï¿½ï¿½Ø¤è¤¦ï¿½ï¿½ï¿½ï¿½!" };

/* DBCS BIG5 encoded 61 B1 64 B1 64 61 */
unsigned char dbcs_big5[] = { "\151\261\144\261\144\151" };

/* DBCS GB2312 encoded BD A1 BD A1 */
unsigned char dbcs_gb2312[] = { "\275\241\275\241" };

/* DBCS JISX0213 encoded A2 A1 */
unsigned char dbcs_jisx0213[] = { "\242\241" };

/* DBCS KSC5601 encoded B0 B0 */
unsigned char dbcs_ksc5601[] = { "\273\273" };

/* fonts and text to display randomly*/
textdata fontlist[] = {
#if HAVE_FREETYPE_2_SUPPORT
#if EMSCRIPTEN
	0, "courb.ttf",			MWTF_ASCII,"Microwindows",		-1,
	0, "arial.ttf",			MWTF_ASCII, "Microwindows",		-1,
	0, "times.ttf",			MWTF_ASCII, "Microwindows",		-1,
	0, "cour.ttf",			MWTF_ASCII, "Microwindows",		-1,
	0, "DejaVuSans.ttf",	MWTF_ASCII, "Microwindows",		-1,
#else
	0, "lt1-r-omega-serif.ttf",MWTF_ASCII,"Microwindows",	-1,
	0, "arial.ttf",			MWTF_ASCII, "Microwindows",		-1,
	0, "times.ttf",			MWTF_ASCII, "Microwindows",		-1,
	0, "cour.ttf",			MWTF_ASCII, "Microwindows",		-1,
	0, "timesi.ttf",		MWTF_ASCII, "Microwindows",		-1,
#endif
#endif
#if HAVE_PCF_SUPPORT
/* note: large PCF fonts require XCHAR2B, this is not figured out yet for these fonts*/
	//1, "lubI24.pcf",		MWTF_ASCII, "Microwindows",		-1,
	1, "9x15.pcf.gz",		MWTF_ASCII, "Microwindows",		-1,
	1, "helvB12.pcf.gz",	MWTF_ASCII, "Microwindows",		-1,
	1, "symb18.pcf",		MWTF_ASCII, "Microwindows",		-1,
	1, "jiskan24.pcf.gz",	MWTF_UC16,  jiskan24,			5,
	1, "gb24st.pcf.gz",		MWTF_UC16,	gb24st,				5,
#endif
#if HAVE_FNT_SUPPORT
	2, "helvB12.fnt",		MWTF_ASCII,	"Microwindows",		-1,
	2, "timBI18.fnt",		MWTF_ASCII,	"Microwindows",		-1,
	2, "jiskan24.fnt.gz",	MWTF_UC16,	jiskan24,			5,
	2, "jiskan16-2000-1.fnt.gz",MWTF_UC16,	jiskan24,		5,
	2, "gbk16-xke.fnt.gz",	MWTF_UC16,	gbk16_xke,			5,
#endif
#if HAVE_EUCJP_SUPPORT
	3, "k12x10.fnt",		MWTF_DBCS_EUCJP, dbcs_eucjp,	-1,
	3, "k12x12.fnt",		MWTF_DBCS_EUCJP, dbcs_eucjp,	-1,
	3, "k16x16.fnt",		MWTF_DBCS_EUCJP, dbcs_eucjp,	-1,
	3, "k24x24.fnt",		MWTF_DBCS_EUCJP, dbcs_eucjp,	-1,
#endif
#if HAVE_T1LIB_SUPPORT
	4, "bchr.pfb",			MWTF_ASCII,	"Microwindows",		-1,
	4, "bchb.pfb",			MWTF_ASCII,	"Microwindows",		-1,
	4, "dcr10.pfb",			MWTF_ASCII,	"Microwindows",		-1,
	4, "dcbx10.pfb",		MWTF_ASCII,	"Microwindows",		-1,
	4, "bchri.pfb",			MWTF_ASCII,	"Microwindows",		-1,
#endif
#if HAVE_HBF_SUPPORT
	5, "chinese16.hbf",		MWTF_DBCS_BIG5, dbcs_big5,		6,
	5, "chinese16.hbf",		MWTF_DBCS_EUCCN, dbcs_gb2312,	4,
#endif
#if HAVE_HZK_SUPPORT
	6, "HZXFONT",			MWTF_UC16, 	hzk_uc161,			7,
	6, "HZXFONT",			MWTF_UC16, 	hzk_uc162,			9,
	6, "HZXFONT",			MWTF_ASCII,	hzk_asc1,			-1,
	6, "HZXFONT",			MWTF_ASCII,	hzk_asc2,			-1,
	6, "HZXFONT",			MWTF_ASCII,	hzk_asc3,			-1,
	6, "HZKFONT",			MWTF_ASCII,	hzk_asc1,			-1,
#endif
#if HAVE_BIG5_SUPPORT
	7, "",					MWTF_DBCS_BIG5, dbcs_big5,		6,
#endif
#if HAVE_GB2312_SUPPORT
	8, "",					MWTF_DBCS_EUCCN, dbcs_gb2312,	4,
#endif
#if HAVE_JISX0213_SUPPORT
	9, "",					MWTF_DBCS_JIS, dbcs_jisx0213,	2,
#endif
#if HAVE_KSC5601_SUPPORT
	10,"",					MWTF_DBCS_EUCKR, dbcs_ksc5601,	2,
#endif
	-1, 0,					0,			0,					0
};
#define NUMFONTS	((sizeof(fontlist)/sizeof(textdata)) - 1)

int
main(int ac, char **av)
{
	GR_WINDOW_ID window;
	GR_GC_ID gc;
	GR_FONT_ID fontid;
	GR_TIMEOUT timeout = 40;
	GR_REGION_ID regionid;
	int x, y, entry;
	int polyregion = 0;
	GR_RECT cliprect = { 20, 20, 300, 300 };	/* inset square*/
	GR_POINT poly_points[7];	/* arrow*/
	poly_points[0].x =  70;
	poly_points[0].y = 150;
	poly_points[1].x = 230;
	poly_points[1].y = 150;
	poly_points[2].x = 230;
	poly_points[2].y = 120;
	poly_points[3].x = 280;
	poly_points[3].y = 170;
	poly_points[4].x = 230;
	poly_points[4].y = 220;
	poly_points[5].x = 230;
	poly_points[5].y = 190;
	poly_points[6].x =  70;
	poly_points[6].y = 190;

	if (GrOpen() < 0)
		return 1;

	window = GrNewWindowEx(GR_WM_PROPS_APPWINDOW,
		"Microwindows Font Demo [Use up/down keys, c, r, q]",
		GR_ROOT_WINDOW_ID, 50, 50, WIDTH, HEIGHT, BLACK);
	GrSelectEvents(window, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ |
		GR_EVENT_MASK_KEY_DOWN);
	GrMapWindow(window);

	gc = GrNewGC();
	GrSetGCUseBackground(gc, GR_FALSE);
	GrSetGCBackground(gc, BLACK);

	/* start with rectangle clip region */
	regionid = GrNewRegion();
	GrUnionRectWithRegion(regionid, &cliprect);
	GrSetGCRegion(gc, regionid);

	srand(time(0));
	while (1) {
		GR_EVENT event;

		GrGetNextEventTimeout(&event, timeout);
		switch (event.type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			return 0;
		case GR_EVENT_TYPE_KEY_DOWN:
			switch (event.keystroke.ch) {
			case MWKEY_UP:					/* speed up*/
			case MWKEY_RIGHT:
				timeout -= 20;
				if ((int)timeout <= 0)
					timeout = -1;			/* fastest poll*/
				break;
			case MWKEY_DOWN:				/* slow down*/
			case MWKEY_LEFT:
				timeout += 20;
				break;
			case 'c':						/* clear screen*/
				GrFillRect(window, gc, 0, 0, WIDTH, HEIGHT);
				break;
			case 'r':						/* toggle clip region*/
				GrSetGCForeground(gc, BLACK);
				GrFillRect(window, gc, 0, 0, WIDTH, HEIGHT);
				polyregion = !polyregion;
				GrDestroyRegion(regionid);
				if (polyregion) {
					regionid = GrNewPolygonRegion(MWPOLY_EVENODD, 7, poly_points);
				} else {
					regionid = GrNewRegion();
					GrUnionRectWithRegion(regionid, &cliprect);
				}
				GrSetGCRegion(gc, regionid);
				break;
			case 'q':						/* quit*/
				GrClose();
				return 0;
			}
			break;
		}

		/* pick random entry from table*/
		entry = RAND(NUMFONTS);

		fontid = GrCreateFontEx(fontlist[entry].fontname, 0, 0, NULL);
		GrSetFontSizeEx(fontid, RAND(80) + 1, RAND(80) + 1);
		GrSetFontRotation(fontid, 330);		/* 33 degrees */
		GrSetFontAttr(fontid, GR_TFKERNING | GR_TFANTIALIAS, 0);
		GrSetGCFont(gc, fontid);
		x = RAND(WIDTH);
		y = RAND(HEIGHT);

		GrSetGCForeground(gc, rand() & 0xffffff);
		//GrSetGCBackground(gc, rand() & 0xffffff);

		//FIXME: setting window title causes GrGetNextEventTimeout to ignore timeout
		//sprintf(title, "Microwindows Font Demo %s %s",
			//groupname[fontlist[entry].group], fontlist[entry].fontname);
		//GrSetWindowTitle(window, title);

		GrText(window, gc, x, y, fontlist[entry].text, fontlist[entry].textlen,
			fontlist[entry].encoding);

		GrDestroyFont(fontid);
		GrFlush();
	}

	GrClose();
	return 0;
}
