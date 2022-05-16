/*
 * Default XLFD font directory list, add directories if desired.
 * This file may need to be modified on a per-installation basis.
 *
 * Each directory in this list with a fonts.dir file will be 
 * included with font enumeration.  The font can then be loaded
 * using either the XLFD specification or the filename spec in the fonts.dir file.
 * This is the case for almost all .pcf, .pcf.gz and .ttf files.
 *
 * For files not included in fonts.dir files:
 * Truetype files will be found by filename.ttf if in a directory
 * PCF files will be found by filename.pcf or filename.pcf.gz if in a directory
 * Adobe Type1 files will be found by filename.pfb if in a directory
 */
#include <stdio.h>

#ifndef XLOCALFONTPATH      /* specified in Makefile_nr */
#define XLOCALFONTPATH ""   /* will default to microwindows/src/fonts relative paths */
#endif

/* default system font paths, must end with '/' if multiple paths specified */
#define MACOSX_SYSTEM_PATH  "/opt/X11/share/fonts/"
#define LINUX_SYSTEM_PATH   "/usr/share/fonts/"
#define MSDOS_SYSTEM_PATH   "/usr/share/fonts"      /* fldisk central fonts */
#define ANDROID_SYSTEM_PATH "/system/fonts"

char *FONT_DIR_LIST[] = {									
    XLOCALFONTPATH,                             /* path to mwin fonts directory*/
    XLOCALFONTPATH "/pcf",                      /* mwin pcf fonts w/fonts.dir*/
    XLOCALFONTPATH "/truetype",                 /* mwin .ttf/.otf fonts w/fonts.dir*/
#if MACOSX && HAVE_SYSTEM_FONT_PATHS
    MACOSX_SYSTEM_PATH "TTF",
    MACOSX_SYSTEM_PATH "truetype",
    MACOSX_SYSTEM_PATH "100dpi",
    MACOSX_SYSTEM_PATH "misc",
#endif
#if LINUX && HAVE_SYSTEM_FONT_PATHS
    LINUX_SYSTEM_PATH "X11/misc",               /* pcf fonts w/fonts.dir*/
    LINUX_SYSTEM_PATH "X11/100dpi",
    LINUX_SYSTEM_PATH "truetype",               /* truetype fonts, Suse 64bit distro*/
    LINUX_SYSTEM_PATH "truetype/freefont",      /* truetype fonts, no fonts.dir*/
    LINUX_SYSTEM_PATH "truetype/ttf-dejavu",
    LINUX_SYSTEM_PATH "truetype/openoffice",
    LINUX_SYSTEM_PATH "X11/Type1",              /* t1lib type1 .pfb fonts, w/fonts.dir*/
#endif
#if MSDOS && HAVE_SYSTEM_FONT_PATHS
    MSDOS_SYSTEM_PATH,
#endif
#if ANDROID && HAVE_SYSTEM_FONT_PATHS
    ANDROID_SYSTEM_PATH,
#endif
#if OTHER
    "/var/lib/defoma/x-ttcidfont-conf.d/dirs/TrueType",
#endif
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
