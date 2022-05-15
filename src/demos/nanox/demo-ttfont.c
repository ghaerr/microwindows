/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2019 Greg Haerr <greg@censoft.com>
 *
 * demo-ttfont: Microwindows Truetype font demonstration program
 * Browse through Truetype fonts and displays each font in a variety of sizes
 *
 * Use left/right arrow to select font, a to toggle antialiasing, q to quit
 * 
 * Uses buffered windows for automatic double buffering and no blink!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

char *fontlist[] = {
#if defined(__ANDROID__)
	"Roboto-MediumItalic.ttf",
#elif EMSCRIPTEN
	"times.ttf",
	"arial.ttf",
	"cour.ttf",
	"courb.ttf",
	"Accidental Presidency.ttf",
	"Cyberbit.ttf",
	"VAGRounded Lt.ttf",
	"DejaVuSans.ttf",
	"DejaVuSans-Bold.ttf",
	"DejaVuSans-BoldOblique.ttf",
	"DejaVuSans-Oblique.ttf",
#else
#if HAVE_FREETYPE_2_SUPPORT
	"times.ttf",
	"timesb.ttf",
	"timesz.ttf",
	"timesi.ttf",
	"arial.ttf",
	"arialb.ttf",
	"ariblk.ttf",
	"arialz.ttf",
	"ariali.ttf",
	"comic.ttf",
	"comicbd.ttf",
	"cour.ttf",
	"courb.ttf",
	"courz.ttf",
	"couri.ttf",
	"fogblack.otf",
	"accid___.ttf",
	"VAGROLN.ttf",
	"Cyberbit.ttf",
	"DejaVuSans.ttf",
	"DejaVuSansB.ttf",
	"DejaVuSansMono.ttf",
	"DejaVuSansMonoB.ttf",
	"DejaVuSansMonoO.ttf",
	"DejaVuSansMonoBO.ttf",
	"DroidSerif.ttf",
	"DroidSerifB.ttf",
	"DroidSerifI.ttf",
	"DroidSerifBI.ttf",
	"LiberationMono.ttf",
	"LiberationSansB.ttf",
	"LiberationMonoI.ttf",
	"LiberationSansBI.ttf",
	"lucida.ttf",
	"lucidab.ttf",
	"lucidasans.ttf",
	"lucidasansb.ttf",
	"impact.ttf",
	"lucon.ttf",
	"lt1-r-omega-serif.ttf",
	"lt2-r-omega-serif.ttf",
	"lt3-r-omega-serif.ttf",
	"lt4-r-omega-serif.ttf",
	"lt5-r-omega-serif.ttf",
	"lt1-b-omega-serif.ttf",
	"lt2-b-omega-serif.ttf",
	"lt3-b-omega-serif.ttf",
	"lt4-b-omega-serif.ttf",
	"lt5-b-omega-serif.ttf",
	"lt1-i-omega-serif.ttf",
	"lt2-i-omega-serif.ttf",
	"lt3-i-omega-serif.ttf",
	"lt4-i-omega-serif.ttf",
	"lt5-i-omega-serif.ttf",
	"lt1-bi-omega-serif.ttf",
	"lt2-bi-omega-serif.ttf",
	"lt3-bi-omega-serif.ttf",
	"lt4-bi-omega-serif.ttf",
	"lt5-bi-omega-serif.ttf",
	"viscii-omega-serif.ttf",
#endif
#if HAVE_T1LIB_SUPPORT
	"bchb.pfb",
	"bchbi.pfb",
	"bchr.pfb",
	"bchri.pfb",
	"c0419bt_.pfb",
	"c0582bt_.pfb",
	"c0583bt_.pfb",
	"c0611bt_.pfb",
	"c0632bt_.pfb",
	"c0633bt_.pfb",
	"c0648bt_.pfb",
	"c0649bt_.pfb",
	"dcbx10.pfb",
	"dcbxti10.pfb",
	"dcr10.pfb",
	"dcti10.pfb",
	"eufm10.pfb",
#endif
#if HAVE_FNT_SUPPORT
	"helvB12.fnt",
	"timBI18.fnt",
	"symb18.fnt",
#endif
#if HAVE_PCF_SUPPORT
	"7x14.pcf.gz",
	"9x15.pcf.gz",
	"vga.pcf.gz",
#endif
#endif /* ANDROID, EMSCRIPTEN */
	0
};
#define NUMFONTS	((sizeof(fontlist)/sizeof(char *)) - 1)

#define FGCOLOR		GrGetSysColor(flipcolors? GR_COLOR_WINDOW: GR_COLOR_APPTEXT)
#define BGCOLOR		GrGetSysColor(flipcolors? GR_COLOR_APPTEXT: GR_COLOR_APPWINDOW)

GR_WINDOW_ID	w;
GR_BOOL		aa = GR_TRUE;
GR_BOOL		flipcolors = 1;
int			entry = 0;

static void
do_paint(void)
{
	int	i, y = 0;
	GR_GC_ID	gc;
	GR_FONT_ID	font;
	GR_WINDOW_INFO winfo;
	char title[128];

	GrGetWindowInfo(w, &winfo);

	gc = GrNewGC();
	GrSetGCUseBackground(gc, GR_FALSE);

	GrSetGCForeground(gc, BGCOLOR);
	GrFillRect(w, gc, 0, 0, winfo.width, winfo.height);

	GrSetGCForeground(gc, FGCOLOR);

	sprintf(title, "Microwindows Truetype Font Demo (%s) [Use left/right keys, a, f, q]",
		fontlist[entry]);
	GrSetWindowTitle(w, title);

	for (i=3; i<=30; ++i) {
		int 	width, height;
		char	buf[64];
		GR_FONT_INFO	finfo;

		height = i * winfo.height / 530;
		width = i * winfo.width / 640;
		font = GrCreateFontEx(fontlist[entry], height, width, NULL);

		GrSetFontAttr(font, aa? (GR_TFANTIALIAS|GR_TFKERNING): 0, -1);
		/*GrSetFontRotation(font, 150);*/
		GrSetGCFont(gc, font);

		sprintf(buf, "%d/%d The Quick Brown Fox Jumps Over The Lazy Dog", height, width);
		GrText(w, gc, 0, y, buf, -1, GR_TFASCII|GR_TFTOP);

		GrGetFontInfo(font, &finfo);
		y += finfo.height;

		GrDestroyFont(font);
	}
	GrDestroyGC(gc);
	GrFlushWindow(w);
}

int
main(int ac, char **av)
{
	if (ac > 1)
		fontlist[0] = av[1];

	if (GrOpen() < 0) {
		GrError("cannot open graphics\n");
		return 1;
	}
#if defined(NUKLEAR) && !NUKLEAR
	flipcolors = 0;
#endif

	w = GrNewBufferedWindow(GR_WM_PROPS_APPWINDOW, "",
		GR_ROOT_WINDOW_ID, 10, 10, 640, 530, BGCOLOR);
	GrSelectEvents(w, GR_EVENT_MASK_BUTTON_DOWN|GR_EVENT_MASK_UPDATE|
		GR_EVENT_MASK_KEY_DOWN|GR_EVENT_MASK_CLOSE_REQ);
	GrMapWindow(w);

	while (1) {
		GR_EVENT event;

		GrGetNextEvent(&event);
		switch (event.type) {
		case GR_EVENT_TYPE_UPDATE:
			switch (event.update.utype) {
			case GR_UPDATE_MAP:			/* initial paint*/
			case GR_UPDATE_SIZE:		/* resize repaint*/
				do_paint();
			}
			break;

		case GR_EVENT_TYPE_BUTTON_DOWN:
			do_paint();
			break;

    	case GR_EVENT_TYPE_KEY_DOWN:
      		switch(event.keystroke.ch) {
        	case 'a':
				aa = !aa;
				do_paint();
          		break;
        	case 'f':
				flipcolors = !flipcolors;
				do_paint();
          		break;
			case MWKEY_RIGHT:
				if (++entry >= NUMFONTS)
					entry = 0;
				do_paint();
				break;
			case MWKEY_LEFT:
				if (--entry < 0)
					entry = NUMFONTS-1;
				do_paint();
				break;
			case 'q':
				GrClose();
				return 0;
			}
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			return 0;
		}
	}
}
