/*
 * fontlist.c - font directory list for pcf/truetype/type1 fonts
 * Default setup for Ubuntu desktop (for testing)
 *
 * This file must be modified on a per-installation basis.
 */
#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <unistd.h>

/*
 * Default pcf/truetype/type1 font directory list, add directories if desired.
 *
 * Each directory in this list with a fonts.dir file will be 
 * included with font enumeration.  The font can then be loaded
 * using either the XLFD specification or the filename spec in the fonts.dir file.
 * This is the case for almost all .pcf and .pcf.gz files.
 *
 * For files not included in fonts.dir files:
 * Truetype files will be found by filename.ttf if in a directory
 * PCF files will be found by filename.pcf or filename.pcf.gz if in a directory
 * Adobe Type1 files will be found by filename.pfb if in a directory
 */
char *FONT_DIR_LIST[] = {
	"fonts",									/* local font dir w/fonts.alias*/
	"/usr/share/fonts/X11/misc",				/* pcf fonts w/fonts.dir*/
	"/usr/share/fonts/X11/100dpi",
	"/usr/share/fonts/truetype/freefont",		/* truetype fonts, no fonts.dir*/
	"/usr/share/fonts/truetype/ttf-dejavu",
	"/usr/share/fonts/truetype/openoffice",
	"/var/lib/defoma/x-ttcidfont-conf.d/dirs/TrueType",	/* truetype fonts, w/fonts.dir & fonts.scale*/
	"/usr/share/fonts/X11/Type1",				/* t1lib type1 .pfb fonts, w/fonts.dir*/
	0
};

#if HAVE_STATICFONTS
extern unsigned char dejavusansbold[572908];
extern unsigned char dejavusans[622020];
extern unsigned char dejavusansmono[320812];

nxStaticFontList staticFontList[] = {
	{"DejaVuSans.ttf","-misc-helvetica-medium-r-normal--0-0-0-0-p-0-iso10646-1",dejavusans,sizeof(dejavusans)},
	{"DejaVuSans-Bold.ttf","-misc-helvetica-bold-r-normal--0-0-0-0-p-0-iso10646-1",dejavusansbold,sizeof(dejavusansbold)},
	{"DejaVuSans-Mono.ttf","-misc-courier-medium-r-normal--0-0-0-0-p-0-iso10646-1",dejavusansmono,sizeof(dejavusansmono)},
	{NULL,NULL,NULL,0}
};
#endif
