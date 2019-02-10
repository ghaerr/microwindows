/*
 * Microwindows configurable options header file
 *
 * These options are normally set through config file/Arch.rules/Makefile.rules
 * but can be set through a visual make environment instead.
 * If an option isn't defined, this file controls the default behaviour.
 *
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 */

/* Changeable limits and options*/
#define UNIFORMPALETTE	1		/* =1 for 256 entry uniform palette,*/
								/* (required for palette alpha blending)*/
#define POLYREGIONS		1		/* =1 includes polygon regions*/
#define FT_MINAA_HEIGHT	0		/* min height for FT antialias with win32 plogfont*/
#define TRANSLATE_ESCAPE_SEQUENCES  1		/* =1 to parse fnkeys w/tty driver*/
#define USE_ALLOCA		1		/* =1 if alloca() is available*/
#define DEBUG_EXPOSE	0		/* =1 to flash yellow before painting expose areas*/
#define DEBUG_BLIT		0		/* =1 to flash brown before painting areas with convblit*/
#define DEBUG_NANOWM	0		/* =1 for window manager debug printfs*/

/* the following defines are set=0 in Arch.rules based on ARCH= setting*/
#ifndef HAVE_SELECT
#define HAVE_SELECT		1		/* =1 has select system call*/
#endif

#ifndef HAVE_SIGNAL
#define HAVE_SIGNAL		1		/* =1 has signal system call*/
#endif

#ifndef HAVE_MMAP
#define HAVE_MMAP       1       /* =1 has mmap system call*/
#endif

#ifndef HAVE_FLOAT
#define HAVE_FLOAT		1		/* =1 incl float, GdArcAngle*/
#endif

#ifndef MW_FEATURE_TIMERS
#define MW_FEATURE_TIMERS 1		/* =1 to include MWTIMER support */
#endif

/* the following enable/disable Microwindows features, set from config or Arch.rules*/
#ifndef NONETWORK
#define NONETWORK		0		/* =1 to link Nano-X apps with server for standalone*/
#endif

#ifndef HAVE_FILEIO
#define HAVE_FILEIO		1		/* =1 to include libc stdio and image reading routines*/
#endif

/* image reading support*/
#ifndef MW_FEATURE_IMAGES
#define MW_FEATURE_IMAGES 1		/* =1 to enable GdLoadImage/GdDrawImage etc*/
#endif

#ifndef HAVE_BMP_SUPPORT
#define HAVE_BMP_SUPPORT		1	/* BMP image support*/
#endif

#ifndef HAVE_GIF_SUPPORT
#define HAVE_GIF_SUPPORT		1	/* GIF image support*/
#endif

#ifndef HAVE_PNM_SUPPORT
#define HAVE_PNM_SUPPORT		1	/* PNM image support*/
#endif

#ifndef HAVE_XPM_SUPPORT
#define HAVE_XPM_SUPPORT		1	/* XPM image support*/
#endif

#ifndef HAVE_JPEG_SUPPORT
#define HAVE_JPEG_SUPPORT		0	/* JPEG image support*/
#endif

#ifndef HAVE_PNG_SUPPORT
#define HAVE_PNG_SUPPORT		0	/* PNG image support*/
#endif

#ifndef HAVE_TIFF_SUPPORT
#define HAVE_TIFF_SUPPORT		0	/* TIFF image support*/
#endif

/* font support*/
#ifndef HAVE_PCF_SUPPORT
#define HAVE_PCF_SUPPORT		1	/* PCF font support*/
#endif

#ifndef HAVE_PCFGZ_SUPPORT
#define HAVE_PCFGZ_SUPPORT		0	/* gzipped PCF font support*/
#endif

#ifndef HAVE_FREETYPE_2_SUPPORT
#define HAVE_FREETYPE_2_SUPPORT	0	/* Truetype font support*/
#endif

#ifndef HAVE_HARFBUZZ_SUPPORT
#define HAVE_HARFBUZZ_SUPPORT	0
#endif

#ifndef HAVE_T1LIB_SUPPORT
#define HAVE_T1LIB_SUPPORT		0	/* T1 library font support*/
#endif

#ifndef HAVE_FNT_SUPPORT
#define HAVE_FNT_SUPPORT		1	/* Microwindows FNT font support*/
#endif

#ifndef HAVE_FNTGZ_SUPPORT
#define HAVE_FNTGZ_SUPPORT		0	/* Microwindows gzipped FNT font support*/
#endif

#ifndef HAVE_HZK_SUPPORT
#define HAVE_HZK_SUPPORT		0
#endif

#ifndef HAVE_BIG5_SUPPORT
#define HAVE_BIG5_SUPPORT		0
#endif

#ifndef HAVE_GB2312_SUPPORT
#define HAVE_GB2312_SUPPORT		0
#endif

#ifndef HAVE_HBF_SUPPORT
#define HAVE_HBF_SUPPORT		0
#endif

#ifndef HAVE_JISX0213_SUPPORT
#define HAVE_JISX0213_SUPPORT	0
#endif

#ifndef HAVE_EUCJP_SUPPORT
#define HAVE_EUCJP_SUPPORT		0
#endif

#ifndef HAVE_KSC5601_SUPPORT
#define HAVE_KSC5601_SUPPORT	0
#endif

#ifndef HAVE_FRIBIDI_SUPPORT
#define HAVE_FRIBIDI_SUPPORT	0
#endif

#ifndef HAVE_SHAPEJOINING_SUPPORT
#define HAVE_SHAPEJOINING_SUPPORT 0
#endif

/* other options*/
#ifndef HAVE_SHAREDMEM_SUPPORT
#define HAVE_SHAREDMEM_SUPPORT 0 /* =1 to use shared memory between NX client/server*/
#endif

#ifndef UPDATEREGIONS
#define UPDATEREGIONS	1		/* =1 win32 api paints only in updated regions*/
#endif

#ifndef ERASEMOVE
#define ERASEMOVE		1		/* =1 win32 erases background during window moves*/
#endif

#ifndef NANOWM
#define NANOWM			1		/* =1 for builtin nano-X window manager*/
#endif

#ifndef THREADSAFE
#define THREADSAFE		0		/* =1 for thread safe nano-X server*/
#endif

#ifndef NOCLIPPING
#define NOCLIPPING		0		/* =1 to generate engine with no clipping*/
#endif

#ifndef NOFONTS
#define NOFONTS			0		/* =1 to generate engine with no builtin fonts*/
#endif

#ifndef HAVE_TEXTMODE
#define HAVE_TEXTMODE	0		/* =1 for systems that switch between text and graphics*/
#endif

#ifndef VTSWITCH
#define VTSWITCH		0		/* =1 to include virtual terminal switching code*/
#endif

#ifndef USE_EXPOSURE
#define USE_EXPOSURE	1		/* =1 to repaint from framebuffer on X11 expose event*/
#endif

#ifndef INVERT4BPP
#define INVERT4BPP		0		/* =1 for inverted pixels in 4bpp screen driver*/
#endif

#ifndef INVERT2BPP
#define INVERT2BPP		0		/* =1 for inverted pixels in 2bpp screen driver*/
#endif

#ifndef DYNAMICREGIONS
#define DYNAMICREGIONS	1		/* =1 to use MWCLIPREGIONS*/
#endif

#ifndef MW_FEATURE_TWO_KEYBOARDS
#define MW_FEATURE_TWO_KEYBOARDS 0	/* =1 to include multiple keyboard support */
#endif

#ifndef NOMAIN
#define NOMAIN			0		/* =1 for library without main() entry point*/
#endif

/* obsolete features*/
#define FONTMAPPER	0			/* =1 for Morten's font mapper*/
#define OLD_TEXT	0			/* =1 old text draw in wingdi.c::mwDrawText() & static.c*/

#if USE_ALLOCA
/* alloca() is available, so use it for better performance */
#define ALLOCA(size)	alloca(size)
#define FREEA(pmem)
#else
/* no alloca(), so use malloc()/free() instead */
#define ALLOCA(size)	malloc(size)
#define FREEA(pmem)		free(pmem)
#endif

#define REALLOC(addr,oldsize,newsize) realloc((addr),(newsize))

/* determine compiler capability for handling EPRINTF/DPRINTF macros*/
#if (defined(GCC_VERSION) && (GCC_VERSION >= 2093)) || (defined(__GNUC__) && (((__GNUC__ >= 2) && (__GNUC_MINOR__ >= 95)) || (__GNUC__ > 2)))
#define HAVE_VARARG_MACROS	1
#else
#define HAVE_VARARG_MACROS	0
#endif

/*
 * Control whether printf/fprintf required in server and demo programs and C library.
 * if this is set to =0 in Arch.rules, fprintf/printf will be no-op in all demo programs
 * and in the server GdError will be called, which calls write() if HAVE_FILEIO=1
 */
#ifndef HAVE_FPRINTF
#define HAVE_FPRINTF	1		/* =1 EPRINTF/DPRINTF uses fprintf/printf else GdError*/
#endif

/* see if can use GCC compiler-only macro magic to save space */
#if HAVE_VARARG_MACROS && HAVE_FPRINTF
#define EPRINTF(str, args...)   fprintf(stderr, str, ##args)  /* error output*/
#if DEBUG
#define DPRINTF(str, args...)   fprintf(stderr, str, ##args)  /* debug output*/
#else
#define DPRINTF(str, ...)			/* no debug output*/
#endif

#else	/* must call GdError*/
/* error.c*/
int	GdError(const char *format, ...);
int	GdErrorNull(const char *format, ...);  /* doesn't print msgs */

#define EPRINTF			GdError		/* error output*/
#if DEBUG
# define DPRINTF		GdError		/* debug output*/
#else
# if HAVE_VARARG_MACROS
# define DPRINTF(str, ...)			/* no debug output*/
# else
# define DPRINTF		GdErrorNull	/* no debug output*/
# endif
#endif

#endif /* HAVE_VARARG_MACROS && HAVE_FPRINTF*/

/* Sanity check: VTSWITCH involves a timer. */
#if VTSWITCH && !MW_FEATURE_TIMERS
#error VTSWITCH depends on MW_FEATURE_TIMERS - disable VTSWITCH in config or enable MW_FEATURE_TIMERS in Arch.rules
#endif

/* no assert() in MSDOS or ELKS...*/
#if MSDOS | ELKS | PSP
#undef assert
#define assert(x)
#endif

#if RTEMS
  /* RTEMS requires rtems_main()*/
  int rtems_main(int, char **);
  #define main	rtems_main
#endif
