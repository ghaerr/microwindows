/*
 * Demonstration program for freetype truetype font support
 * Martin Jolicoeur 2000 martinj@visuaide.com.
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#define ANTIALIAS	0		/* set =1 to enable anti aliasing*/

#if HAVE_T1LIB_SUPPORT
#define FONTNAME "fonts/type1/bchr.pfb"
#elif HAVE_FREETYPE_2_SUPPORT
#define FONTNAME "lt1-r-omega-serif"
//#define FONTNAME "termcs1"
//#define FONTNAME "lucon"
//#define FONTNAME "cour"
#elif HAVE_PCF_SUPPORT
//#define FONTNAME	"jiskan24.pcf.gz"
#define FONTNAME	"helvB12.pcf.gz"
//#define FONTNAME	"helvB12_lin.pcf.gz"
//#define FONTNAME	"fonts/bdf/symb18.pcf"
#elif HAVE_FNT_SUPPORT
#define FONTNAME	"timBI18.fnt"
#elif HAVE_EUCJP_SUPPORT
#define FONTNAME	"k16x16.fnt"
#elif HAVE_HZK_SUPPORT
#define FONTNAME	"HZKFONT"
#else
#define FONTNAME GR_FONT_SYSTEM_VAR
#endif

#define MAXW 400
#define MAXH 400

GR_GC_ID gid;
GR_FONT_ID fontid, fontid2;
GR_BOOL kerning = GR_FALSE;
GR_BOOL aa = ANTIALIAS;
GR_BOOL bold = GR_FALSE;
GR_BOOL underline = GR_FALSE;
int angle = 0;
int state = GR_TFBOTTOM;
char buffer[128];
int n;
void Render(GR_WINDOW_ID window);

int main(int argc, char **argv)
{
  FILE *file;
  GR_EVENT event;
  GR_WINDOW_ID window;

  if (GrOpen() < 0) {
	fprintf(stderr, "cannot open graphics\n");
	exit(1);
  }

  window = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "ftdemo",
  	GR_ROOT_WINDOW_ID, 50,50, MAXW,MAXH, WHITE);
  GrMapWindow(window);

  gid = GrNewGC ();
  GrSelectEvents(window, GR_EVENT_MASK_KEY_DOWN |
		GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_EXPOSURE);

  if ((file = fopen("bin/ftdemo.txt", "r")) == NULL) {
	printf("Can't open text file\n");
	return (-1);
  }

  n = 0;

  if(fgets(buffer, 128, file) != NULL) {
  	for (n = 0; n < 128 && buffer[n]; n++) {
		if (buffer[n] == '\n')
			break;
	}
  }
  fclose(file);

  fontid = GrCreateFontEx(FONTNAME, 20, 20, NULL);
  fontid2 = GrCreateFontEx(FONTNAME, 36, 36, NULL);

  Render(window);
 
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
        case 'a':
          aa = !aa;
          break;
        case 'b':
          bold = !bold;
          break;
        case 'k':
          kerning = !kerning;
          break;
        case 'l':
          state = (state == GR_TFBOTTOM)?GR_TFBASELINE:
                  (state == GR_TFBASELINE)?GR_TFTOP:GR_TFBOTTOM;
          break;
        case 'u':
          underline = underline? GR_FALSE: GR_TRUE;
          break;
        default:
          continue;
          /* Unknown keystroke */
      }
      Render(window);
      break;
    case GR_EVENT_TYPE_EXPOSURE:
      Render(window);
      break;
    case GR_EVENT_TYPE_CLOSE_REQ:
      GrClose();
      exit(0);
    }
  }

  return 0;
}

void Render(GR_WINDOW_ID window)
{
   int flags = 0;
   GR_SCREEN_INFO info;

   if (aa)
   	flags |= GR_TFANTIALIAS;
   if (kerning)
   	flags |= GR_TFKERNING;
   if (underline)
   	flags |= GR_TFUNDERLINE;
   if (bold)
   	flags |= GR_TFBOLD;

// flags |= MWTF_CMAP_0;	/* termcs1 only*/

   GrGetScreenInfo(&info);
   GrSetGCBackground(gid, WHITE);
   GrSetGCForeground (gid, WHITE);
   GrSetGCUseBackground(gid, GR_FALSE);
   GrFillRect(window, gid, 0, 0, info.cols, info.rows);
   GrSetGCForeground (gid, BLACK);

//	GrSetGCForeground (gid, GREEN);
//	GrSetGCBackground(gid, BLUE);
//	GrSetGCUseBackground(gid, GR_TRUE);
 
   /* Draw menu */
   GrSetGCFont(gid, fontid);
   GrSetFontAttr(fontid, flags & ~GR_TFUNDERLINE, -1);
   GrText(window, gid, 5, 20, "+ Rotate string clockwise", 25, GR_TFASCII);
   GrText(window, gid, 5, 40, "-  Rotate string counter-clockwise", 34, GR_TFASCII);
   GrText(window, gid, 5, 60, "a Toggle anti-aliasing", 22, GR_TFASCII);
   GrText(window, gid, 5, 80, "b Toggle bold", 13, GR_TFASCII);
   GrText(window, gid, 5, 100, "k Toggle kerning", 16, GR_TFASCII);
   GrText(window, gid, 5, 120, "u Toggle underline", 18, GR_TFASCII);
   GrText(window, gid, 5, 140, "l  Toggle alignment bottom/baseline/top", 39, GR_TFASCII);
#if HAVE_KSC5601_SUPPORT
   GrText(window, gid, 5, 160, "\xB0\xA1\xB0\xA2\xB0\xA3", 6, MWTF_DBCS_EUCKR);
#endif

	/* check display of glyphs with negative leftBearing*/
//	GrText(window, gid, 5, 160, "H\xAEH\xDEH\xF2H", 6, GR_TFASCII); // leftBearing < 0 helvB12.pcf.gz

	/* sym18.pcf.gz A0 should display blank*/
//	GrText(window, gid, 5, 160, "\xA0\xDC\xA0", 3, GR_TFASCII);	// should be SP <= SP
//	GrText(window, gid, 5, 180, "\x40\x80", 2, GR_TFASCII);		// should be approxequal,SP
 

#if 0
	/* jiskan24.pcf.gz test large pcf, UC16 and default character*/
{
	unsigned short text[32];

	text[0] = 1122;				/* blank*/
	text[1] = 1123;
	text[2] = 1124;
	text[3] = 1125;
	text[4] = 1126;
	text[5] = 0x2121;			/* default char*/
	text[6] = 0x2122;
	text[7] = 0x2123;
	text[8] = 0x2124;
	text[9] = 0x2125;
	GrText(window, gid, 5, 160, text, 10, MWTF_UC16);
}
#endif

   /* Draw test string */
   GrSetGCFont(gid, fontid2);
   GrSetFontAttr(fontid2, flags, -1);
   GrSetFontRotation(fontid2, angle);
   GrText(window, gid, MAXW/2, MAXH/2, buffer, n, state|GR_TFUTF8);
 
   /* Draw arrow */
   GrLine (window, gid, (MAXW/2)-10 , MAXH/2, (MAXW/2)+10, MAXH/2);
   GrLine (window, gid, MAXW/2, (MAXH/2)-10, MAXW/2, (MAXH/2)+10);
}
