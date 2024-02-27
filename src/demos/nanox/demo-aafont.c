/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2019 Greg Haerr <greg@censoft.com>
 *
 * demo-aafont: Microwindows font antialiasing and rotation demonstration program
 * Browse fonts, rotate, alias and set baseline, kerning and underline
 *
 * Use left/right arrow to select font, a to toggle antialiasing, q to quit
 * Uses buffered windows for automatic double buffering and no blink!
 *
 * Original version by Martin Jolicoeur 2000 martinj@visuaide.com.
 * Rewritten by Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

/* fontlist table structure*/
typedef struct {
	int group;
	char *fontname;
	int	encoding;
	void *text;
	int textlen;
} textdata;

/* UTF8 encoded text to display*/
unsigned char basetext[] = { "gEût été ôté de là..." };

/* jiskan24 PCF font test 1122 blank, 0x2121 default char*/
unsigned short jiskan24[] = { 1122, 1123, 1124, 1125, 1126,
							  0x2121, 0x2122, 0x2123, 0x2124, 0x2125 };

/* fonts and text to display*/
textdata fontlist[] = {
#if HAVE_FREETYPE_2_SUPPORT
#if EMSCRIPTEN
	0, "courb.ttf",			MWTF_UTF8,	basetext,			22,
	0, "arial.ttf",			MWTF_UTF8,	basetext,			22,
	0, "times.ttf",			MWTF_UTF8,	basetext,			22,
	0, "cour.ttf",			MWTF_UTF8,	basetext,			22,
	0, "DejaVuSans.ttf",	MWTF_UTF8,	basetext,			22,
#else
	0, "lt1-r-omega-serif.ttf",MWTF_UTF8,basetext,			22,
	0, "arial.ttf",			MWTF_UTF8,	basetext,			22,
	0, "times.ttf",			MWTF_UTF8,	basetext,			22,
	0, "cour.ttf",			MWTF_UTF8,	basetext,			22,
	0, "timesi.ttf",		MWTF_UTF8,	basetext,			22,
#endif
#endif
#if HAVE_PCF_SUPPORT
	//1, "lubI24.pcf",		MWTF_UTF8,	basetext,			22,
	1, "9x15.pcf.gz",		MWTF_UTF8,	basetext,			22,
	1, "helvB12.pcf.gz",	MWTF_UTF8,	basetext,			22,
	1, "symb18.pcf",		MWTF_UTF8,	basetext,			22,
//	1, "jiskan24.pcf.gz",	MWTF_UC16,	jiskan24,			10,
#endif
#if HAVE_FNT_SUPPORT
	2, "helvB12.fnt",		MWTF_UTF8,	basetext,			22,
	2, "timBI18.fnt",		MWTF_UTF8,	basetext,			22,
#endif
#if HAVE_T1LIB_SUPPORT
	4, "bchr.pfb",			MWTF_UTF8,	basetext,			22,
#endif
#if HAVE_EUCJP_SUPPORT
	3, "k16x16.fnt",		MWTF_UTF8,	basetext,			22,
	3, "k24x24.fnt",		MWTF_UTF8,	basetext,			22,
	3, "k12x12.fnt",		MWTF_UTF8,	basetext,			22,
	3, "k12x10.fnt",		MWTF_UTF8,	basetext,			22,
#endif
#if HAVE_HZK_SUPPORT
	6, "HZXFONT",			MWTF_UTF8, 	basetext,			22,
	6, "HZKFONT",			MWTF_UTF8, 	basetext,			22,
#endif
#if HAVE_HBF_SUPPORT
//	5, "chinese16.hbf",		MWTF_UTF8,	basetext,			22,
#endif
#if defined(__ANDROID__)
	0, "Georgia-Italic.ttf",MWTF_UTF8,	basetext,			22,
	0, "Times.ttf",			MWTF_UTF8,	basetext,			22,
	0, "Arial-Italic.ttf",	MWTF_UTF8,	basetext,			22,
	0, "Cour.ttf",			MWTF_UTF8,	basetext,			22,
	0, "Chococooky.ttf",	MWTF_UTF8,	basetext,			22,
#endif
	-1, 0,					0,			0,					0
};
#define NUMFONTS	((sizeof(fontlist)/sizeof(textdata)) - 1)

#define MAXW 480
#define MAXH 480
#define MAXSIZE	200		/* max font size*/

GR_FONT_ID fontid, fontid2;
GR_GC_ID gid;
GR_BOOL aa = 1;
GR_BOOL kerning = 0;
GR_BOOL bold = 0;
GR_BOOL underline = 0;
GR_BOOL flipcolors = 1;
int angle = 0;
int state = GR_TFBOTTOM;
int entry = 0;
int fontsize = 36;
static void Render(GR_WINDOW_ID window);

int
main(int argc, char **argv)
{
  GR_EVENT event;
  GR_WINDOW_ID window;

  if (GrOpen() < 0) {
	GrError("cannot open graphics\n");
	return 1;
  }
#if defined(NUKLEAR) && !NUKLEAR
  flipcolors = 0;
#endif

  window = GrNewBufferedWindow(GR_WM_PROPS_APPWINDOW, "Antialias Fonts Demo",
  	GR_ROOT_WINDOW_ID, 50,50, MAXW,MAXH, WHITE);
  GrMapWindow(window);

  gid = GrNewGC ();
  GrSelectEvents(window, GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_UPDATE);

  while (1) {
    GrGetNextEvent(&event);

    switch (event.type) {
    case GR_EVENT_TYPE_KEY_DOWN:
      switch(event.keystroke.ch) {
        case 171: /* + */
        case '+':
        case '=':
          angle += 100; /* Increase 10 degrees */
          angle %= 3600;
          break;
        case 173: /* - */
        case '-':
        case '_':
          angle -= 100; /* Decrease 10 degrees */
          angle %= 3600;
          break;
		case MWKEY_RIGHT:
			if (++entry >= NUMFONTS)
				entry = 0;
			break;
		case MWKEY_LEFT:
			if (--entry < 0)
				entry = NUMFONTS-1;
			break;
		case MWKEY_UP:
			if (++fontsize > MAXSIZE)
				fontsize = 1;
			break;
		case MWKEY_DOWN:
			if (--fontsize <= 0)
				fontsize = MAXSIZE;
			break;
        case 'a':
          aa = !aa;
          break;
        case 'b':
          bold = !bold;
          break;
        case 'k':
          kerning = !kerning;
          break;
        case 'u':
          underline = !underline;
          break;
        case 'f':
          flipcolors = !flipcolors;
          break;
        case 'l':
          state = (state == GR_TFBOTTOM)?	GR_TFBASELINE:
		  		  (state == GR_TFBASELINE)?	GR_TFTOP:
					GR_TFBOTTOM;
          break;
		case 'q':
			GrClose();
			return 0;
        default:
          continue;
      }
      Render(window);
      break;
    case GR_EVENT_TYPE_UPDATE:
		switch (event.update.utype) {
		case GR_UPDATE_MAP:			/* initial paint*/
		case GR_UPDATE_SIZE:		/* resize repaint*/
			Render(window);
			break;
		}
      break;
    case GR_EVENT_TYPE_CLOSE_REQ:
      GrClose();
      return 0;
    }
  }

  return 0;
}

static void
Render(GR_WINDOW_ID window)
{
   int flags = 0;
   GR_SCREEN_INFO info;
   char title[128];

   if (aa)
   	flags |= GR_TFANTIALIAS;
   if (kerning)
   	flags |= GR_TFKERNING;
   if (underline)
   	flags |= GR_TFUNDERLINE;
   if (bold)
   	flags |= GR_TFBOLD;
// flags |= MWTF_CMAP_0;	/* termcs1 only*/

  sprintf(title, "Microwindows Fonts Antialias/Rotation Demo (%s)",
		fontlist[entry].fontname);
  GrSetWindowTitle(window, title);

  fontid = GrCreateFontEx(fontlist[entry].fontname, 20, 20, NULL);
  fontid2 = GrCreateFontEx(fontlist[entry].fontname, fontsize, fontsize, NULL);

   GrGetScreenInfo(&info);
   GrSetGCForeground (gid, GrGetSysColor(flipcolors? GR_COLOR_APPTEXT: GR_COLOR_APPWINDOW));
   GrSetGCUseBackground(gid, GR_FALSE);
   GrFillRect(window, gid, 0, 0, info.cols, info.rows);
   GrSetGCForeground (gid, GrGetSysColor(flipcolors? GR_COLOR_APPWINDOW: GR_COLOR_APPTEXT));

//	GrSetGCForeground (gid, GREEN);
//	GrSetGCBackground(gid, BLUE);
//	GrSetGCUseBackground(gid, GR_TRUE);
 
   /* Draw menu */
   GrSetGCFont(gid, fontid);
   GrSetFontAttr(fontid, flags & ~GR_TFUNDERLINE, -1);
   GrText(window, gid, 5, 20, "= Rotate string clockwise", 25, GR_TFASCII);
   GrText(window, gid, 5, 40, "-  Rotate string counter-clockwise", 34, GR_TFASCII);
   GrText(window, gid, 5, 60, "a Toggle anti-aliasing", 22, GR_TFASCII);
   GrText(window, gid, 5, 80, "b Toggle bold", 13, GR_TFASCII);
   GrText(window, gid, 5, 100, "k Toggle kerning", 16, GR_TFASCII);
   GrText(window, gid, 5, 120, "u Toggle underline", 18, GR_TFASCII);
   GrText(window, gid, 5, 140, "f Toggle foreground/background", -1, GR_TFASCII);
   GrText(window, gid, 5, 160, "l  Toggle alignment bottom/baseline/top", 39, GR_TFASCII);
   GrText(window, gid, 5, 180, "   Arrow keys select next font and size", -1, GR_TFASCII);

#if HAVE_KSC5601_SUPPORT
   //GrText(window, gid, 5, 160, "\xB0\xA1\xB0\xA2\xB0\xA3", 6, MWTF_DBCS_EUCKR);
#endif

	/* check display of glyphs with negative leftBearing*/
	/* leftBearing < 0 helvB12.pcf.gz*/
//	GrText(window, gid, 5, 160, "H\xAEH\xDEH\xF2H", 6, GR_TFASCII);

	/* sym18.pcf.gz A0 should display blank*/
//	GrText(window, gid, 5, 160, "\xA0\xDC\xA0", 3, GR_TFASCII);	// should be SP <= SP
//	GrText(window, gid, 5, 180, "\x40\x80", 2, GR_TFASCII);		// should be approxequal,SP
 

   /* Draw text*/
   GrSetGCFont(gid, fontid2);
   GrSetFontAttr(fontid2, flags, -1);
   GrSetFontRotation(fontid2, angle);
   GrText(window, gid, MAXW/2, MAXH/2, fontlist[entry].text, fontlist[entry].textlen,
		state | fontlist[entry].encoding);
 
   /* Draw arrow */
   GrLine (window, gid, (MAXW/2)-10 , MAXH/2, (MAXW/2)+10, MAXH/2);
   GrLine (window, gid, MAXW/2, (MAXH/2)-10, MAXW/2, (MAXH/2)+10);

   GrDestroyFont(fontid);
   GrDestroyFont(fontid2);
   GrFlushWindow(window);
}
