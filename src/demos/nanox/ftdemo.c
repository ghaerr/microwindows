/*
 * Demonstration program for freetype truetype font support
 * Martin Jolicoeur 2000 martinj@visuaide.com.
 */
#include <stdio.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#if HAVE_T1LIB_SUPPORT
#define FONTNAME "bchr"
#if 0
#define FONTNAME "bchb"
#define FONTNAME "dcr10"
#define FONTNAME "dcbx10"
#endif
#elif HAVE_FREETYPE_SUPPORT
#define FONTNAME "lt1-r-omega-serif"
#if 0
#define FONTNAME "times"
#define FONTNAME "cour"
#endif
#else
#define FONTNAME GR_FONT_SYSTEM_VAR
#endif

#define MAXW 400
#define MAXH 400

GR_GC_ID gid;
GR_FONT_ID fontid, fontid2;
GR_BOOL kerning = GR_FALSE;
GR_BOOL aa = GR_TRUE;
GR_BOOL underline = GR_FALSE;
int angle = 0;
int state = GR_TFBOTTOM;
char buffer[128];
int n;
void Render(GR_WINDOW_ID window);

int
main()
{
  FILE *file;
  GR_EVENT event;
  GR_WINDOW_ID window;

  if (GrOpen() < 0) {
	fprintf(stderr, "cannot open graphics\n");
	exit(1);
  }

  window = GrNewWindow(GR_ROOT_WINDOW_ID, 50,50, MAXW,MAXH, 4, BLACK, WHITE);
  GrMapWindow(window);

  gid = GrNewGC ();
  GrSelectEvents(window, GR_EVENT_MASK_KEY_DOWN |
		GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_EXPOSURE);

  if ((file = fopen("ftdemo.txt", "r")) == NULL) {
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

  fontid = GrCreateFont(FONTNAME, 20, NULL);
  fontid2 = GrCreateFont(FONTNAME, 36, NULL);

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
          aa = (aa == GR_FALSE)?GR_TRUE:GR_FALSE;
          break;
        case 'k':
          kerning = (kerning == GR_FALSE)?GR_TRUE:GR_FALSE;
          break;
        case 'l':
          state = (state == GR_TFBOTTOM)?GR_TFBASELINE: \
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
   GrSetGCBackground(gid, WHITE);
   GrSetGCForeground (gid, WHITE);
   GrSetGCUseBackground(gid, GR_FALSE);
   GrFillRect(window, gid, 0, 0, MAXW, MAXH);
   GrSetGCForeground (gid, BLACK);
 
   /* Draw menu */
   GrSetGCFont(gid, fontid);
   GrSetFontAttr(fontid, GR_TFKERNING | GR_TFANTIALIAS, 0);
   GrText(window, gid, 5, 20, "+ Rotate string clockwise", 25, GR_TFASCII);
   GrText(window, gid, 5, 40, "-  Rotate string counter-clockwise", 34, GR_TFASCII);
   GrText(window, gid, 5, 60, "a Toggle anti-aliasing", 22, GR_TFASCII);
   GrText(window, gid, 5, 80, "k Toggle kerning", 16, GR_TFASCII);
   GrText(window, gid, 5, 100, "u Toggle underline", 18, GR_TFASCII);
   GrText(window, gid, 5, 120, "l  Toggle alignment bottom/baseline/top", 39, GR_TFASCII);
 
   /* Draw test string */
   GrSetGCFont(gid, fontid2);
   GrSetFontAttr(fontid2, (kerning?GR_TFKERNING:0) | (aa?GR_TFANTIALIAS:0) |
	(underline?GR_TFUNDERLINE: 0), -1);
   GrSetFontRotation(fontid2, angle);
   GrText(window, gid, MAXW/2, MAXH/2, buffer, n, state|GR_TFUTF8);
 
   /* Draw arrow */
   GrLine (window, gid, (MAXW/2)-10 , MAXH/2, (MAXW/2)+10, MAXH/2);
   GrLine (window, gid, MAXW/2, (MAXH/2)-10, MAXW/2, (MAXH/2)+10);
}
