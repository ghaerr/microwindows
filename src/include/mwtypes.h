#ifndef _MWTYPES_H
#define _MWTYPES_H
/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Exported Microwindows engine typedefs and defines
 */
#define MWPACKED	__attribute__ ((aligned(1), packed))

/* builtin font std names*/
#define MWFONT_SYSTEM_VAR	"System"	/* winFreeSansSerif 11x13 (ansi)*/
#define MWFONT_SYSTEM_FIXED	"SystemFixed"	/* X6x13 (should be ansi)*/
#define MWFONT_GUI_VAR		"System"	/* deprecated (was "Helvetica")*/
#define MWFONT_OEM_FIXED	"SystemFixed"	/* deprecated (was "Terminal")*/

/* Text/GetTextSize encoding flags*/
#define MWTF_ASCII	0x00000000L	/* 8 bit packing, ascii*/
#define MWTF_UTF8	0x00000001L	/* 8 bit packing, utf8*/
#define MWTF_UC16	0x00000002L	/* 16 bit packing, unicode 16*/
#define MWTF_UC32	0x00000004L	/* 32 bit packing, unicode 32*/
#define MWTF_XCHAR2B	0x00000008L	/* 16 bit packing, X11 big endian PCF*/
#define MWTF_PACKMASK	0x0000000FL	/* packing bits mask*/

/* asian double-byte encodings*/
#define MWTF_DBCS_BIG5	0x00000100L	/* chinese big5*/
#define MWTF_DBCS_EUCCN	0x00000200L	/* chinese EUCCN (gb2312+0x80)*/
#define MWTF_DBCS_EUCKR	0x00000300L	/* korean EUCKR (ksc5601+0x80)*/
#define MWTF_DBCS_EUCJP	0x00000400L	/* japanese EUCJP*/
#define MWTF_DBCS_JIS	0x00000500L	/* japanese JISX0213*/
#define MWTF_DBCSMASK	0x00000700L	/* DBCS encodings bitmask*/

/* Text alignment flags*/
#define MWTF_TOP	0x01000000L	/* align on top*/
#define MWTF_BASELINE	0x02000000L	/* align on baseline*/
#define MWTF_BOTTOM	0x04000000L	/* align on bottom*/

/* SetFontAttr flags (no intersect with MWTF_ above)*/
#define MWTF_KERNING	0x0001		/* font kerning*/
#define MWTF_ANTIALIAS	0x0002		/* antialiased output*/
#define MWTF_UNDERLINE	0x0004		/* draw underline*/
#define MWTF_FREETYPE	0x1000		/* FIXME: remove*/

/* Drawing modes*/
#define	MWMODE_COPY		0	/* src*/
#define	MWMODE_XOR		1	/* src ^ dst*/
#define	MWMODE_OR		2	/* src | dst*/
#define	MWMODE_AND		3	/* src & dst*/
#define	MWMODE_CLEAR		4	/* 0*/
#define	MWMODE_SETTO1		5	/* 11111111*/ /* obsolete name, will be MWMODE_SET*/
#define	MWMODE_EQUIV		6	/* ~(src ^ dst)*/
#define	MWMODE_NOR		7	/* ~(src | dst)*/
#define	MWMODE_NAND		8	/* ~(src & dst)*/
#define	MWMODE_INVERT		9	/* ~dst*/
#define	MWMODE_COPYINVERTED	10	/* ~src*/
#define	MWMODE_ORINVERTED	11	/* ~src | dst*/
#define	MWMODE_ANDINVERTED	12	/* ~src & dst*/
#define MWMODE_ORREVERSE	13	/* src | ~dst*/
#define	MWMODE_ANDREVERSE	14	/* src & ~dst*/
#define	MWMODE_NOOP		15	/* dst*/
#define	MWMODE_XOR_FGBG		16	/* src ^ background ^ dst (This is the Java XOR mode) */

#define MWMODE_SIMPLE_MAX 16	/* Last "simple" (non-alpha) mode */

/*
 * Porter-Duff rules for alpha compositing.
 *
 * Only SRC, CLEAR, and SRC_OVER are commonly used.
 * The rest are very uncommon, although a full Java implementation
 * would require them.  (ATOP and XOR were introduced in JDK 1.4)
 *
 * Note that MWMODE_PORTERDUFF_XOR is *very* different from MWMODE_XOR.
 */
/* #define	MWMODE_CLEAR - already correctly defined */
#define	MWMODE_SRC	MWMODE_COPY
#define	MWMODE_DST	MWMODE_NOOP
#define	MWMODE_SRC_OVER	17
#define	MWMODE_DST_OVER	18
#define	MWMODE_SRC_IN	19
#define	MWMODE_DST_IN	20
#define	MWMODE_SRC_OUT	21
#define	MWMODE_DST_OUT	22
#define	MWMODE_SRC_ATOP	23
#define	MWMODE_DST_ATOP	24
#define	MWMODE_PORTERDUFF_XOR	25

#define	MWMODE_MAX		25


/* Line modes */
#define MWLINE_SOLID      0
#define MWLINE_ONOFF_DASH 1

/* FUTURE: MWLINE_DOUBLE_DASH */

/* Fill mode  */
#define MWFILL_SOLID          0  
#define MWFILL_STIPPLE        1  
#define MWFILL_OPAQUE_STIPPLE 2  
#define MWFILL_TILE           3

/* Mouse button bits*/
#define MWBUTTON_L	04
#define MWBUTTON_M	02
#define MWBUTTON_R	01

/* Color defines*/
#define MWARGB(a,r,g,b)	((MWCOLORVAL)(((unsigned char)(r)|\
				(((unsigned)(unsigned char)(g))<<8))|\
				(((unsigned long)(unsigned char)(b))<<16)|\
				(((unsigned long)(unsigned char)(a))<<24)))
#define MWRGB(r,g,b)	MWARGB(255,(r),(g),(b))		/* rgb full alpha*/
#define MW0RGB(r,g,b)	MWARGB(0,(r),(g),(b))		/* rgb no alpha*/


/* convert an MWROP to drawing mode MWMODE value*/
#define MWROP_TO_MODE(op)	((op) >> 24)

/* convert an MWMODE to blitting mode MWROP value*/
#define MWMODE_TO_ROP(op)	(((long)(op)) << 24)


/* 
 * ROP blitter opcodes (extensions < 0x20000000 are reserved
 * for MWMODE_xxx blit ops, although currently some of these
 * are unused).
 */
#define MWROP_EXTENSION		0xff000000L	/* rop extension bits*/

/* copy src -> dst except for transparent color in src*/
#define MWROP_SRCTRANSCOPY	0x21000000L

/* alpha blend src -> dst with constant alpha, alpha value in low 8 bits*/
#define MWROP_BLENDCONSTANT	0x22000000L

/* alpha blend fg/bg color -> dst with src as alpha channel*/
#define MWROP_BLENDFGBG		0x23000000L

/* alpha blend src -> dst with separate per pixel alpha channel*/
#define MWROP_BLENDCHANNEL	0x24000000L

/* stretch src -> dst*/
#define MWROP_STRETCH		0x25000000L

/* Use the MWMODE value in the graphics context
 * to choose the appropriate MWROP value.
 * (This is only valid in calls to the Nano-X API,
 * it is not valid for the lower level blitters) 
 */
#define MWROP_USE_GC_MODE	0xff000000L

/* blits rops based on src/dst binary operations*/
#define MWROP_COPY		MWMODE_TO_ROP(MWMODE_COPY)
#define	MWROP_XOR		MWMODE_TO_ROP(MWMODE_XOR)
#define	MWROP_OR		MWMODE_TO_ROP(MWMODE_OR)
#define MWROP_AND		MWMODE_TO_ROP(MWMODE_AND)
#define	MWROP_CLEAR		MWMODE_TO_ROP(MWMODE_CLEAR)
#define	MWROP_SET		MWMODE_TO_ROP(MWMODE_SETTO1)
#define	MWROP_EQUIV		MWMODE_TO_ROP(MWMODE_EQUIV)
#define	MWROP_NOR		MWMODE_TO_ROP(MWMODE_NOR)
#define	MWROP_NAND		MWMODE_TO_ROP(MWMODE_NAND)
#define	MWROP_INVERT		MWMODE_TO_ROP(MWMODE_INVERT)
#define	MWROP_COPYINVERTED	MWMODE_TO_ROP(MWMODE_COPYINVERTED)
#define	MWROP_ORINVERTED	MWMODE_TO_ROP(MWMODE_ORINVERTED)
#define	MWROP_ANDINVERTED	MWMODE_TO_ROP(MWMODE_ANDINVERTED)
#define MWROP_ORREVERSE		MWMODE_TO_ROP(MWMODE_ORREVERSE)
#define	MWROP_ANDREVERSE	MWMODE_TO_ROP(MWMODE_ANDREVERSE)
#define	MWROP_NOOP		MWMODE_TO_ROP(MWMODE_NOOP)
#define	MWROP_XOR_FGBG		MWMODE_TO_ROP(MWMODE_XOR_FGBG)

/*
 * Porter-Duff rules for alpha compositing.
 *
 * Only SRC, CLEAR, and SRC_OVER are commonly used.
 * The rest are very uncommon, although a full Java implementation
 * would require them.
 *
 * Note that MWMODE_PORTERDUFF_XOR is very different from MWMODE_XOR.
 */
/* #define	MWMODE_CLEAR - already correctly defined */
#define	MWROP_SRC	MWMODE_TO_ROP(MWMODE_SRC)
#define	MWROP_DST	MWMODE_TO_ROP(MWMODE_DST)
#define	MWROP_SRC_OVER	MWMODE_TO_ROP(MWMODE_SRC_OVER)
#define	MWROP_DST_OVER	MWMODE_TO_ROP(MWMODE_DST_OVER)
#define	MWROP_SRC_IN	MWMODE_TO_ROP(MWMODE_SRC_IN)
#define	MWROP_DST_IN	MWMODE_TO_ROP(MWMODE_DST_IN)
#define	MWROP_SRC_OUT	MWMODE_TO_ROP(MWMODE_SRC_OUT)
#define	MWROP_DST_OUT	MWMODE_TO_ROP(MWMODE_DST_OUT)
#define	MWROP_SRC_ATOP	MWMODE_TO_ROP(MWMODE_SRC_ATOP)
#define	MWROP_DST_ATOP	MWMODE_TO_ROP(MWMODE_DST_ATOP)
#define	MWROP_PORTERDUFF_XOR	MWMODE_TO_ROP(MWMODE_PORTERDUFF_XOR)

#define MWROP_SRCCOPY		MWROP_COPY	/* obsolete*/
#define MWROP_SRCAND		MWROP_AND	/* obsolete*/
#define MWROP_SRCINVERT		MWROP_XOR	/* obsolete*/
#define MWROP_BLACKNESS     	MWROP_CLEAR	/* obsolete*/

/* 
 * Pixel formats
 * Note the two pseudo pixel formats are never returned by display drivers,
 * but rather used as a data structure type in GrArea.  The other
 * types are both returned by display drivers and used as pixel packing
 * specifiers.
 */
#define MWPF_RGB	   0	/* pseudo, convert from packed 32 bit RGB*/
#define MWPF_PIXELVAL	   1	/* pseudo, no convert from packed PIXELVAL*/
#define MWPF_PALETTE	   2	/* pixel is packed 8 bits 1, 4 or 8 pal index*/
#define MWPF_TRUECOLOR0888 3	/* pixel is packed 32 bits 8/8/8 truecolor*/
#define MWPF_TRUECOLOR888  4	/* pixel is packed 24 bits 8/8/8 truecolor*/
#define MWPF_TRUECOLOR565  5	/* pixel is packed 16 bits 5/6/5 truecolor*/
#define MWPF_TRUECOLOR555  6	/* pixel is packed 16 bits 5/5/5 truecolor*/
#define MWPF_TRUECOLOR332  7	/* pixel is packed 8 bits 3/3/2 truecolor*/
#define MWPF_TRUECOLOR8888 8	/* pixel is packed 32 bits 8/8/8/8 truecolor with alpha */

/*
 * MWPIXELVAL definition: changes based on target system
 * Set using -DMWPIXEL_FORMAT=MWPF_XXX
 *
 * For the Nano-X server, it is important to use the correct MWPF_* value
 * for the MWPIXEL_FORMAT macro in order to match the hardware,
 * while the Nano-X clients that includes this file can get away with
 * a default pixel format of 24-bit color as the client will either:
 *    1) Use the MWPF_PIXELVAL native format when calling GrReadArea, in
 *       which case we have to have enough spare room to hold 32-bit
 *       pixlevalues (hence the default MWPF_TRUECOLOR0888 format), or
 *    2) Will use some other PF_* format, in which case the application
 *       is well aware of which pixel-format it uses and can avoid the
 *       device specific RGB2PIXEL and use RGB2PIXEL565 etc. instead,
 *       and specifiy the pixel fomar as MWPF_TRUECOLOR565 etc. when
 *       calling the GrArea function(s).
 */
#ifndef MWPIXEL_FORMAT
#define MWPIXEL_FORMAT	MWPF_TRUECOLOR0888
#endif

#if defined(__AS386_16__)
/* Force 8 bit palettized display for ELKS*/
#undef MWPIXEL_FORMAT
#define MWPIXEL_FORMAT	MWPF_PALETTE
#endif

#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
typedef unsigned short MWPIXELVAL;
#else
  #if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
  typedef unsigned char MWPIXELVAL;
  #else
    #if MWPIXEL_FORMAT == MWPF_PALETTE
    typedef unsigned char MWPIXELVAL;
    #else
      typedef unsigned long MWPIXELVAL;
    #endif
  #endif
#endif

/* portrait modes*/
#define MWPORTRAIT_NONE		0x00	/* hw framebuffer, no rotation*/
#define MWPORTRAIT_LEFT		0x01	/* rotate left*/
#define	MWPORTRAIT_RIGHT	0x02	/* rotate right*/
#define MWPORTRAIT_DOWN		0x04	/* upside down*/

/*
 * Type definitions
 */
typedef int		MWCOORD;	/* device coordinates*/
typedef int		MWBOOL;		/* boolean value*/
typedef unsigned char	MWUCHAR;	/* unsigned char*/
typedef unsigned long	MWCOLORVAL;	/* device-independent color value*/
typedef unsigned short	MWIMAGEBITS;	/* bitmap image unit size*/
typedef unsigned long	MWTIMEOUT;	/* timeout value */
typedef unsigned long	MWTEXTFLAGS;	/* MWTF_ text flag*/

#define MWCOORD_MAX	0x7fff		/* maximum coordinate value*/
#define MWCOORD_MIN	(-MWCOORD_MAX)	/* minimum coordinate value*/

/* MWIMAGEBITS macros*/
#define MWIMAGE_WORDS(x)	(((x)+15)/16)
#define MWIMAGE_BYTES(x)	(MWIMAGE_WORDS(x)*sizeof(MWIMAGEBITS))
/* size of image in words*/
#define	MWIMAGE_SIZE(width, height)  	\
	((height) * (((width) + MWIMAGE_BITSPERIMAGE - 1) / MWIMAGE_BITSPERIMAGE))
#define	MWIMAGE_BITSPERIMAGE	(sizeof(MWIMAGEBITS) * 8)
#define	MWIMAGE_BITVALUE(n)	((MWIMAGEBITS) (((MWIMAGEBITS) 1) << (n)))
#define	MWIMAGE_FIRSTBIT	(MWIMAGE_BITVALUE(MWIMAGE_BITSPERIMAGE - 1))
#define	MWIMAGE_NEXTBIT(m)	((MWIMAGEBITS) ((m) >> 1))
#define	MWIMAGE_TESTBIT(m)	((m) & MWIMAGE_FIRSTBIT)  /* use with shiftbit*/
#define	MWIMAGE_SHIFTBIT(m)	((MWIMAGEBITS) ((m) << 1))  /* for testbit*/

/* dbl linked list data structure*/
typedef struct _mwlist {		/* LIST must be first decl in struct*/
	struct _mwlist *next;		/* next item*/
	struct _mwlist *prev;		/* previous item*/
} MWLIST, *PMWLIST;

/* dbl linked list head data structure*/
typedef struct _mwlisthead {
	struct _mwlist *head;		/* first item*/
	struct _mwlist *tail;		/* last item*/
} MWLISTHEAD, *PMWLISTHEAD;

/* Keyboard state modifiers*/
typedef unsigned int	MWKEYMOD;

/* GetScreenInfo structure*/
typedef struct {
	MWCOORD  rows;		/* number of rows on screen */
	MWCOORD  cols;		/* number of columns on screen */
	int 	 xdpcm;		/* dots/centimeter in x direction */
	int 	 ydpcm;		/* dots/centimeter in y direction */
	int	 planes;	/* hw # planes*/
	int	 bpp;		/* hw bpp*/
	long	 ncolors;	/* hw number of colors supported*/
	int 	 fonts;		/* number of built-in fonts */
	int 	 buttons;	/* buttons which are implemented */
	MWKEYMOD modifiers;	/* modifiers which are implemented */
	int	 pixtype;	/* format of pixel value*/
	int	 portrait;	/* current portrait mode*/
	MWBOOL	 fbdriver;	/* true if running mwin fb screen driver*/
	unsigned long rmask;	/* red mask bits in pixel*/
	unsigned long gmask;	/* green mask bits in pixel*/
	unsigned long bmask;	/* blue mask bits in pixel*/
	MWCOORD	 xpos;		/* current x mouse position*/
	MWCOORD	 ypos;		/* current y mouse position*/

/* items below are get/set by the window manager and not used internally*/
	int	vs_width;	/* virtual screen width/height*/
	int	vs_height;
	int	ws_width;	/* workspace width/height*/
	int	ws_height;
} MWSCREENINFO, *PMWSCREENINFO;

/* client side window framebuffer info*/
typedef struct {
	unsigned char *	physpixels;	/* address of real framebuffer*/
	/* note winpixels is only correct in non-portrait modes*/
	unsigned char *	winpixels;	/* address of 0,0 this window in fb*/
	int	pixtype;	/* MWPF_ pixel type*/
	int	bpp;		/* bits per pixel*/
	int	bytespp;	/* bytes per pixel*/
	int	pitch;		/* bytes per scan line for window (=fb pitch)*/
	MWCOORD	x, y;		/* absolute window coordinates*/
	int	portrait_mode;	/* current portrait mode*/
	MWCOORD	xres;		/* real framebuffer resolution*/
	MWCOORD	yres;
	MWCOORD	xvirtres;	/* virtual framebuffer resolution*/
	MWCOORD	yvirtres;
} MWWINDOWFBINFO;

/**
 * Structure returned by GetFontInfo.
 *
 * All sizes are in pixels.
 *
 * Some of the sizes are limits for "most characters".  With built-in bitmap
 * fonts, "most characters" means "all characters".  Otherwise, the
 * definition of "most characters" depends on the person who designed the
 * font.  Typically it is the alphanumeric characters, and it may or may not
 * include accented characters.
 */
typedef struct {
	/**
	 * Maximum advance width of any character.
	 */
	int maxwidth;

	/**
	 * Height of "most characters" in the font. This does not include any
	 * leading (blank space between lines of text).
	 * Always equal to (baseline+descent).
	 */
	int height;

	/**
	 * The ascent (height above the baseline) of "most characters" in
	 * the font.
	 *
	 * Note: This member variable should be called "ascent", to be
	 * consistent with FreeType 2, and also to be internally consistent
	 * with the "descent" member.  It has not been renamed because that
	 * would break backwards compatibility.  FIXME
	 */
	int baseline;

	/**
	 * The descent (height below the baseline) of "most characters" in
	 * the font.
	 *
	 * Should be a POSITIVE number.
	 */
	int descent;

	/**
	 * Maximum height of any character above the baseline.
	 */
	int maxascent;

	/**
	 * Maximum height of any character below the baseline.
	 *
	 * Should be a POSITIVE number.
	 */
	int maxdescent;

	/**
	 * The distance between the baselines of two consecutive lines of text.
	 * This is usually height plus some font-specific "leading" value.
	 */
	int linespacing;

	/**
	 * First character in the font.
	 */
	int firstchar;

	/**
	 * Last character in the font.
	 */
	int lastchar;

	/**
	 * True (nonzero) if font is fixed width.  In that case, maxwidth
	 * gives the width for every character in the font.
	 */
	MWBOOL fixed;

	/**
	 * Table of character advance widths for characters 0-255.
	 * Note that fonts can contain characters with codes >255 - in that
	 * case this table contains the advance widths for some but not all
	 * characters.  Also note that if the font contains kerning
	 * information, the advance width of the string "AV" may differ from
	 * the sum of the advance widths for the characters 'A' and 'V'.
	 */
	MWUCHAR widths[256];
} MWFONTINFO, *PMWFONTINFO;


/* GetFontList structure */
typedef struct {
	char *ttname;		/* TrueType name, eg "Times New Roman Bold" */
	char *mwname;		/* microwin name, eg "timesb" */
} MWFONTLIST, *PMWFONTLIST;

/* logical font descriptor*/

/* font classes - used to specify a particular renderer*/
#define MWLF_CLASS_ANY		0	/* any font*/
#define MWLF_CLASS_BUILTIN	1	/* builtin fonts*/
#define MWLF_CLASS_FNT		2	/* FNT native fonts*/
#define MWLF_CLASS_PCF		3	/* X11 PCF/PCF.GZ fonts*/
#define MWLF_CLASS_FREETYPE	4	/* FreeType 1 or 2 fonts in TT format*/
#define MWLF_CLASS_T1LIB	5	/* T1LIB outlined Adobe Type 1 fonts*/
#define MWLF_CLASS_MGL		6	/* MGL (EUCJP) fonts*/
#define MWLF_CLASS_HZK		7	/* chinese HZK fonts*/

#define MWLF_FACESIZE		64	/* max facename size*/

/* font type selection - lfOutPrecision*/
#define MWLF_TYPE_DEFAULT	0	/* any font*/
#define MWLF_TYPE_SCALED	4	/* outlined font (tt or adobe)*/
#define MWLF_TYPE_RASTER	5	/* raster only*/
#define MWLF_TYPE_TRUETYPE	7	/* truetype only*/
#define MWLF_TYPE_ADOBE		10	/* adobe type 1 only*/

/* font weights - lfWeight*/
#define MWLF_WEIGHT_DEFAULT	0	/* any weight*/
#define MWLF_WEIGHT_THIN	100	/* thin*/
#define MWLF_WEIGHT_EXTRALIGHT	200
#define MWLF_WEIGHT_LIGHT	300	/* light */
#define MWLF_WEIGHT_NORMAL	400	/* regular*/
#define MWLF_WEIGHT_REGULAR	400
#define MWLF_WEIGHT_MEDIUM	500	/* medium */
#define MWLF_WEIGHT_DEMIBOLD	600
#define MWLF_WEIGHT_BOLD	700	/* bold*/
#define MWLF_WEIGTH_EXTRABOLD	800
#define MWLF_WEIGHT_BLACK	900	/* black */

/* font charset - lfCharSet*/
#define MWLF_CHARSET_ANSI	0	/* win32 ansi*/
#define MWLF_CHARSET_DEFAULT	1	/* any charset*/
#define MWLF_CHARSET_UNICODE	254	/* unicode*/
#define MWLF_CHARSET_OEM	255	/* local hw*/

/* font pitch - lfPitch */
#define MWLF_PITCH_DEFAULT		0	/* any pitch */
#define MWLF_PITCH_ULTRACONDENSED	10
#define MWLF_PITCH_EXTRACONDENSED	20
#define MWLF_PITCH_CONDENSED		30
#define MWLF_PITCH_SEMICONDENSED	40
#define MWLF_PITCH_NORMAL		50
#define MWLF_PITCH_SEMIEXPANDED		60
#define MWLF_PITCH_EXPANDED		70
#define MWLF_PITCH_EXTRAEXPANDED	80
#define MWLF_PITCH_ULTRAEXPANDED	90

/* flags for the GdAddFont function */
#define MWLF_FLAGS_ALIAS	1

/* windows-compatible MWLOGFONT structure*/
typedef struct {
	long	lfHeight;		/* desired height in pixels*/
	long	lfWidth;		/* desired width in pixels or 0*/
	long	lfEscapement;		/* rotation in tenths of degree*/
	long	lfOrientation;		/* not used*/
	long	lfWeight;		/* font weight*/
	MWUCHAR	lfItalic;		/* =1 for italic */
	MWUCHAR	lfUnderline;		/* =1 for underline */
	MWUCHAR	lfStrikeOut;		/* not used*/
	MWUCHAR	lfCharSet;		/* font character set*/
	MWUCHAR	lfOutPrecision;		/* font type selection*/
	MWUCHAR	lfClipPrecision;	/* not used*/
	MWUCHAR	lfQuality;		/* not used*/
	MWUCHAR lfPitchAndFamily;	/* not used*/
	/* end of windows-compatibility*/

	MWUCHAR lfClass;		/* font class (renderer) */

	/* following only used by FONTMAPPER when enabled*/
	MWUCHAR	lfPitch;		/* font pitch */
	MWUCHAR	lfRoman;		/* =1 for Roman letters (upright) */
	MWUCHAR	lfSerif;		/* =1 for Serifed font */
	MWUCHAR	lfSansSerif;		/* =1 for Sans-serif font */
	MWUCHAR	lfModern;		/* =1 for Modern font */
	MWUCHAR	lfMonospace;		/* =1 for Monospaced font */
	MWUCHAR	lfProportional;		/* =1 for Proportional font */
	MWUCHAR	lfOblique;		/* =1 for Oblique (kind of Italic) */
	MWUCHAR	lfSmallCaps;		/* =1 for small caps */

	/* render-dependent full path or facename here*/
	char	lfFaceName[MWLF_FACESIZE];/* font name, may be aliased*/

} MWLOGFONT, *PMWLOGFONT;

/*
 * Macros to initialize the MWLOGFONT structure to the most common defaults
 * needed by application programs and the nano-X server program.
 */

#define MWLF_Clear(lf)					\
	do {						\
		(lf)->lfHeight = 0;			\
		(lf)->lfWidth = 0;			\
		(lf)->lfEscapement = 0;			\
		(lf)->lfOrientation = 0;		\
		(lf)->lfWeight = MWLF_WEIGHT_REGULAR;	\
		(lf)->lfPitch = 0;			\
		(lf)->lfClass = MWLF_CLASS_ANY;		\
		(lf)->lfItalic = 0;			\
		(lf)->lfOblique = 0;			\
		(lf)->lfRoman = 0;			\
		(lf)->lfSerif = 0;			\
		(lf)->lfSansSerif = 0;			\
		(lf)->lfModern = 0;			\
		(lf)->lfMonospace = 0;			\
		(lf)->lfProportional = 0;		\
		(lf)->lfSmallCaps = 0;			\
		(lf)->lfUnderline = 0;			\
		(lf)->lfStrikeOut = 0;			\
		(lf)->lfCharSet = 0;			\
		(lf)->lfOutPrecision = 0;		\
		(lf)->lfClipPrecision = 0;		\
		(lf)->lfQuality = 0;			\
		(lf)->lfPitchAndFamily = 0;		\
		(lf)->lfFaceName[0] = '\0';		\
	} while (0)

#define MWLF_SetBold(lf)				\
	do {						\
		(lf)->lfWeight = MWLF_WEIGHT_BOLD;	\
	} while (0)

#define MWLF_SetRegular(lf)				\
	do {						\
		(lf)->lfWeight = MWLF_WEIGHT_REGULAR;	\
	} while (0)

#define MWLF_SetItalics(lf)				\
	do {						\
		(lf)->lfItalic = 1;			\
		(lf)->lfOblique = 0;			\
		(lf)->lfRoman = 0;			\
	} while (0)

#define MWLF_SetRoman(lf)				\
	do {						\
		(lf)->lfItalic = 0;			\
		(lf)->lfOblique = 0;			\
		(lf)->lfRoman = 1;			\
	} while (0)

/*
 * Rectangle and point structures.
 * These structures are "inherited" in wingdi.h for
 * the Win32 RECT and POINT structures, so they must match
 * Microsoft's definition.
 */

/* MWPOINT used in GdPoly, GdFillPoly*/
typedef struct {
	MWCOORD x;
	MWCOORD y;
} MWPOINT;

/* MWRECT used in region routines*/
typedef struct {
	MWCOORD	left;
	MWCOORD	top;
	MWCOORD	right;
	MWCOORD	bottom;
} MWRECT;

/* dynamically allocated multi-rectangle clipping region*/
typedef struct {
	int	size;		/* malloc'd # of rectangles*/
	int	numRects;	/* # rectangles in use*/
	int	type; 		/* region type*/
	MWRECT *rects;		/* rectangle array*/
	MWRECT	extents;	/* bounding box of region*/
} MWCLIPREGION;

/* region types */
#define MWREGION_ERROR		0
#define MWREGION_NULL		1
#define MWREGION_SIMPLE		2
#define MWREGION_COMPLEX	3

/* GdRectInRegion return codes*/
#define MWRECT_OUT	0	/* rectangle not in region*/
#define MWRECT_ALLIN	1	/* rectangle all in region*/
#define MWRECT_PARTIN	2	/* rectangle partly in region*/

/* GdAllocPolyRegion types*/
#define MWPOLY_EVENODD		1
#define MWPOLY_WINDING		2

typedef struct {
	MWCOORD		width;
	MWCOORD		height;
	MWIMAGEBITS *	bitmap;
} MWSTIPPLE;

/* In-core color palette structure*/
typedef struct {
	MWUCHAR	r;
	MWUCHAR	g;
	MWUCHAR	b;
	MWUCHAR _padding;
} MWPALENTRY;

/* In-core mono and color image structure*/
#define MWIMAGE_UPSIDEDOWN	01	/* compression flag: upside down image*/
#define MWIMAGE_BGR		00	/* compression flag: BGR byte order*/
#define MWIMAGE_RGB		02	/* compression flag: RGB not BGR bytes*/
#define MWIMAGE_ALPHA_CHANNEL   04	/* compression flag: 32-bit w/alpha */

typedef struct {
	int		width;		/* image width in pixels*/
	int		height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel (1, 4 or 8)*/
	int		pitch;		/* bytes per line*/
	int		bytesperpixel;	/* bytes per pixel*/
	int		compression;	/* compression algorithm*/
	int		palsize;	/* palette size*/
	long		transcolor;	/* transparent color or -1 if none*/
	MWPALENTRY *	palette;	/* palette*/
	MWUCHAR *	imagebits;	/* image bits (dword right aligned)*/
} MWIMAGEHDR, *PMWIMAGEHDR;

/* image information structure - returned by GdGetImageInfo*/
typedef struct {
	int		id;		/* image id*/
	int		width;		/* image width in pixels*/
	int		height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel (1, 4 or 8)*/
	int		pitch;		/* bytes per line*/
	int		bytesperpixel;	/* bytes per pixel*/
	int		compression;	/* compression algorithm*/
	int		palsize;	/* palette size*/
	MWPALENTRY 	palette[256];	/* palette*/
} MWIMAGEINFO, *PMWIMAGEINFO;

#define	MWMAX_CURSOR_SIZE	32		/* maximum cursor x and y size*/
#define	MWMAX_CURSOR_BUFLEN	MWIMAGE_SIZE(MWMAX_CURSOR_SIZE,MWMAX_CURSOR_SIZE)

/* In-core software cursor structure*/
typedef struct {
	int		width;			/* cursor width in pixels*/
	int		height;			/* cursor height in pixels*/
	MWCOORD		hotx;			/* relative x pos of hot spot*/
	MWCOORD		hoty;			/* relative y pos of hot spot*/
	MWCOLORVAL	fgcolor;		/* foreground color*/
	MWCOLORVAL	bgcolor;		/* background color*/
	MWIMAGEBITS	image[MWMAX_CURSOR_SIZE*2];/* cursor image bits*/
	MWIMAGEBITS	mask[MWMAX_CURSOR_SIZE*2];/* cursor mask bits*/
} MWCURSOR, *PMWCURSOR;

/** touchscreen device transform coefficients for GdSetTransform*/
typedef struct {
	int	a, b, c;	/* xpos = (a*jitx + b*jity + c)/denom */
	int	d, e, f;	/* ypos = (d*jitx + e*jity + f)/denom */
	int	s;		/* denom*/
} MWTRANSFORM;

typedef struct _mwfont *	PMWFONT;

/* outline and filled arc and pie types*/
#define MWARC		0x0001	/* arc*/
#define MWOUTLINE	0x0002
#define MWARCOUTLINE	0x0003	/* arc + outline*/
#define MWPIE		0x0004	/* pie (filled)*/
#define MWELLIPSE	0x0008	/* ellipse outline*/
#define MWELLIPSEFILL	0x0010	/* ellipse filled*/

#ifdef MWINCLUDECOLORS
/*
 * Common colors - note any color including these may not be
 * available on palettized systems, and the system will
 * then use the nearest color already in the system palette,
 * or allocate a new color entry.
 * These colors are the first 16 entries in the std palette,
 * and are written to the system palette if writable.
 */
#define BLACK		MWRGB( 0  , 0  , 0   )
#define BLUE		MWRGB( 0  , 0  , 128 )
#define GREEN		MWRGB( 0  , 128, 0   )
#define CYAN		MWRGB( 0  , 128, 128 )
#define RED		MWRGB( 128, 0  , 0   )
#define MAGENTA		MWRGB( 128, 0  , 128 )
#define BROWN		MWRGB( 128, 64 , 0   )
#define LTGRAY		MWRGB( 192, 192, 192 )
#define GRAY		MWRGB( 128, 128, 128 )
#define LTBLUE		MWRGB( 0  , 0  , 255 )
#define LTGREEN		MWRGB( 0  , 255, 0   )
#define LTCYAN		MWRGB( 0  , 255, 255 )
#define LTRED		MWRGB( 255, 0  , 0   )
#define LTMAGENTA	MWRGB( 255, 0  , 255 )
#define YELLOW		MWRGB( 255, 255, 0   )
#define WHITE		MWRGB( 255, 255, 255 )

/* other common colors*/
#define DKGRAY		MWRGB( 32,  32,  32)
#endif /* MWINCLUDECOLORS*/

/* Keyboard values*/
typedef unsigned short	MWKEY;
typedef unsigned short	MWSCANCODE;

#define MWKEY_UNKNOWN		0
/* Following special control keysyms are mapped to ASCII*/
#define MWKEY_BACKSPACE		8
#define MWKEY_TAB		9
#define MWKEY_ENTER		13
#define MWKEY_ESCAPE		27
/* Keysyms from 32-126 are mapped to ASCII*/

#define MWKEY_NONASCII_MASK	0xFF00
/* Following keysyms are mapped to private use portion of Unicode-16*/
/* arrows + home/end pad*/
#define MWKEY_FIRST		0xF800
#define MWKEY_LEFT		0xF800
#define MWKEY_RIGHT		0xF801
#define MWKEY_UP		0xF802
#define MWKEY_DOWN		0xF803
#define MWKEY_INSERT		0xF804
#define MWKEY_DELETE		0xF805
#define MWKEY_HOME		0xF806
#define MWKEY_END		0xF807
#define MWKEY_PAGEUP		0xF808
#define MWKEY_PAGEDOWN		0xF809

/* Numeric keypad*/
#define MWKEY_KP0		0xF80A
#define MWKEY_KP1		0xF80B
#define MWKEY_KP2		0xF80C
#define MWKEY_KP3		0xF80D
#define MWKEY_KP4		0xF80E
#define MWKEY_KP5		0xF80F
#define MWKEY_KP6		0xF810
#define MWKEY_KP7		0xF811
#define MWKEY_KP8		0xF812
#define MWKEY_KP9		0xF813
#define MWKEY_KP_PERIOD		0xF814
#define MWKEY_KP_DIVIDE		0xF815
#define MWKEY_KP_MULTIPLY	0xF816
#define MWKEY_KP_MINUS		0xF817
#define MWKEY_KP_PLUS		0xF818
#define MWKEY_KP_ENTER		0xF819
#define MWKEY_KP_EQUALS		0xF81A

/* Function keys */
#define MWKEY_F1		0xF81B
#define MWKEY_F2		0xF81C
#define MWKEY_F3		0xF81D
#define MWKEY_F4		0xF81E
#define MWKEY_F5		0xF81F
#define MWKEY_F6		0xF820
#define MWKEY_F7		0xF821
#define MWKEY_F8		0xF822
#define MWKEY_F9		0xF823
#define MWKEY_F10		0xF824
#define MWKEY_F11		0xF825
#define MWKEY_F12		0xF827

/* Key state modifier keys*/
#define MWKEY_NUMLOCK		0xF828
#define MWKEY_CAPSLOCK		0xF829
#define MWKEY_SCROLLOCK		0xF82A
#define MWKEY_LSHIFT		0xF82B
#define MWKEY_RSHIFT		0xF82C
#define MWKEY_LCTRL		0xF82D
#define MWKEY_RCTRL		0xF82E
#define MWKEY_LALT		0xF82F
#define MWKEY_RALT		0xF830
#define MWKEY_LMETA		0xF831
#define MWKEY_RMETA		0xF832
#define MWKEY_ALTGR		0xF833

/* Misc function keys*/
#define MWKEY_PRINT		0xF834
#define MWKEY_SYSREQ		0xF835
#define MWKEY_PAUSE		0xF836
#define MWKEY_BREAK		0xF837
#define MWKEY_QUIT		0xF838	/* virtual key*/
#define MWKEY_MENU		0xF839	/* virtual key*/
#define MWKEY_REDRAW		0xF83A	/* virtual key*/

/* Handheld function keys*/
/* #define MWKEY_RECORD		0xF840 -- Replaced by HAVi code */
/* #define MWKEY_PLAY		0xF841 -- Replaced by HAVi code */
#define MWKEY_CONTRAST		0xF842
#define MWKEY_BRIGHTNESS	0xF843
#define MWKEY_SELECTUP		0xF844
#define MWKEY_SELECTDOWN	0xF845
#define MWKEY_ACCEPT		0xF846
#define MWKEY_CANCEL		0xF847
#define MWKEY_APP1		0xF848
#define MWKEY_APP2		0xF849
#define MWKEY_APP3              0xF84A
#define MWKEY_APP4              0xF84B
#define MWKEY_SUSPEND           0xF84C
#define MWKEY_END_NORMAL	0xF84D	/* insert additional keys before this*/

/*
 * The following keys are useful for remote controls on consumer
 * electronics devices (e.g. TVs, videos, DVD players, cable
 * boxes, satellite boxes, digital terrestrial recievers, ...)
 *
 * The codes are taken from the HAVi specification:
 *   HAVi Level 2 User Interface version 1.1, May 15th 2001
 * They are listed in section 8.7.
 *
 * For more information see http://www.havi.org/
 */

/* MWKEY code for first HAVi key */
#define MWKEY_HAVI_KEY_BASE   (MWKEY_END_NORMAL+1)

/* HAVi code for first HAVi key */
#define MWKEY_HAVI_CODE_FIRST  403

/* HAVi code for last HAVi key */
#define MWKEY_HAVI_CODE_LAST   460

/* HRcEvent.VK_... code to MWKEY_... code */
#define MWKEY_FROM_HAVI_CODE(h) ((h) + (MWKEY_HAVI_KEY_BASE - MWKEY_HAVI_CODE_FIRST))

/* MWKEY_... code to HRcEvent.VK_... code */
#define MWKEY_TO_HAVI_CODE(m)   ((m) - (MWKEY_HAVI_KEY_BASE - MWKEY_HAVI_CODE_FIRST))

/* Can an MWKEY_... code be converted into a HRcEvent.VK_... code? */
#define MWKEY_IS_HAVI_CODE(m)   (  (unsigned)((m) - MWKEY_HAVI_KEY_BASE) \
               <= (unsigned)(MWKEY_HAVI_CODE_LAST - MWKEY_HAVI_CODE_FIRST) )


#define MWKEY_COLORED_KEY_0         MWKEY_FROM_HAVI_CODE(403)
#define MWKEY_COLORED_KEY_1         MWKEY_FROM_HAVI_CODE(404)
#define MWKEY_COLORED_KEY_2         MWKEY_FROM_HAVI_CODE(405)
#define MWKEY_COLORED_KEY_3         MWKEY_FROM_HAVI_CODE(406)
#define MWKEY_COLORED_KEY_4         MWKEY_FROM_HAVI_CODE(407)
#define MWKEY_COLORED_KEY_5         MWKEY_FROM_HAVI_CODE(408)
#define MWKEY_POWER                 MWKEY_FROM_HAVI_CODE(409)
#define MWKEY_DIMMER                MWKEY_FROM_HAVI_CODE(410)
#define MWKEY_WINK                  MWKEY_FROM_HAVI_CODE(411)
#define MWKEY_REWIND                MWKEY_FROM_HAVI_CODE(412)
#define MWKEY_STOP                  MWKEY_FROM_HAVI_CODE(413)
#define MWKEY_EJECT_TOGGLE          MWKEY_FROM_HAVI_CODE(414)
#define MWKEY_PLAY                  MWKEY_FROM_HAVI_CODE(415)
#define MWKEY_RECORD                MWKEY_FROM_HAVI_CODE(416)
#define MWKEY_FAST_FWD              MWKEY_FROM_HAVI_CODE(417)
#define MWKEY_PLAY_SPEED_UP         MWKEY_FROM_HAVI_CODE(418)
#define MWKEY_PLAY_SPEED_DOWN       MWKEY_FROM_HAVI_CODE(419)
#define MWKEY_PLAY_SPEED_RESET      MWKEY_FROM_HAVI_CODE(420)
#define MWKEY_RECORD_SPEED_NEXT     MWKEY_FROM_HAVI_CODE(421)
#define MWKEY_GO_TO_START           MWKEY_FROM_HAVI_CODE(422)
#define MWKEY_GO_TO_END             MWKEY_FROM_HAVI_CODE(423)
#define MWKEY_TRACK_PREV            MWKEY_FROM_HAVI_CODE(424)
#define MWKEY_TRACK_NEXT            MWKEY_FROM_HAVI_CODE(425)
#define MWKEY_RANDOM_TOGGLE         MWKEY_FROM_HAVI_CODE(426)
#define MWKEY_CHANNEL_UP            MWKEY_FROM_HAVI_CODE(427)
#define MWKEY_CHANNEL_DOWN          MWKEY_FROM_HAVI_CODE(428)
#define MWKEY_STORE_FAVORITE_0      MWKEY_FROM_HAVI_CODE(429)
#define MWKEY_STORE_FAVORITE_1      MWKEY_FROM_HAVI_CODE(430)
#define MWKEY_STORE_FAVORITE_2      MWKEY_FROM_HAVI_CODE(431)
#define MWKEY_STORE_FAVORITE_3      MWKEY_FROM_HAVI_CODE(432)
#define MWKEY_RECALL_FAVORITE_0     MWKEY_FROM_HAVI_CODE(433)
#define MWKEY_RECALL_FAVORITE_1     MWKEY_FROM_HAVI_CODE(434)
#define MWKEY_RECALL_FAVORITE_2     MWKEY_FROM_HAVI_CODE(435)
#define MWKEY_RECALL_FAVORITE_3     MWKEY_FROM_HAVI_CODE(436)
#define MWKEY_CLEAR_FAVORITE_0      MWKEY_FROM_HAVI_CODE(437)
#define MWKEY_CLEAR_FAVORITE_1      MWKEY_FROM_HAVI_CODE(438)
#define MWKEY_CLEAR_FAVORITE_2      MWKEY_FROM_HAVI_CODE(439)
#define MWKEY_CLEAR_FAVORITE_3      MWKEY_FROM_HAVI_CODE(440)
#define MWKEY_SCAN_CHANNELS_TOGGLE  MWKEY_FROM_HAVI_CODE(441)
#define MWKEY_PINP_TOGGLE           MWKEY_FROM_HAVI_CODE(442)
#define MWKEY_SPLIT_SCREEN_TOGGLE   MWKEY_FROM_HAVI_CODE(443)
#define MWKEY_DISPLAY_SWAP          MWKEY_FROM_HAVI_CODE(444)
#define MWKEY_SCREEN_MODE_NEXT      MWKEY_FROM_HAVI_CODE(445)
#define MWKEY_VIDEO_MODE_NEXT       MWKEY_FROM_HAVI_CODE(446)
#define MWKEY_VOLUME_UP             MWKEY_FROM_HAVI_CODE(447)
#define MWKEY_VOLUME_DOWN           MWKEY_FROM_HAVI_CODE(448)
#define MWKEY_MUTE                  MWKEY_FROM_HAVI_CODE(449)
#define MWKEY_SURROUND_MODE_NEXT    MWKEY_FROM_HAVI_CODE(450)
#define MWKEY_BALANCE_RIGHT         MWKEY_FROM_HAVI_CODE(451)
#define MWKEY_BALANCE_LEFT          MWKEY_FROM_HAVI_CODE(452)
#define MWKEY_FADER_FRONT           MWKEY_FROM_HAVI_CODE(453)
#define MWKEY_FADER_REAR            MWKEY_FROM_HAVI_CODE(454)
#define MWKEY_BASS_BOOST_UP         MWKEY_FROM_HAVI_CODE(455)
#define MWKEY_BASS_BOOST_DOWN       MWKEY_FROM_HAVI_CODE(456)
#define MWKEY_INFO                  MWKEY_FROM_HAVI_CODE(457)
#define MWKEY_GUIDE                 MWKEY_FROM_HAVI_CODE(458)
#define MWKEY_TELETEXT              MWKEY_FROM_HAVI_CODE(459)
#define MWKEY_SUBTITLE              MWKEY_FROM_HAVI_CODE(460)

#define MWKEY_LAST                  MWKEY_SUBTITLE

/* Keyboard state modifiers*/
#define MWKMOD_NONE  		0x0000
#define MWKMOD_LSHIFT		0x0001
#define MWKMOD_RSHIFT		0x0002
#define MWKMOD_LCTRL 		0x0040
#define MWKMOD_RCTRL 		0x0080
#define MWKMOD_LALT  		0x0100
#define MWKMOD_RALT  		0x0200
#define MWKMOD_LMETA 		0x0400		/* Windows key*/
#define MWKMOD_RMETA 		0x0800		/* Windows key*/
#define MWKMOD_NUM   		0x1000
#define MWKMOD_CAPS  		0x2000
#define MWKMOD_ALTGR 		0x4000
#define MWKMOD_SCR		0x8000

#define MWKMOD_CTRL	(MWKMOD_LCTRL|MWKMOD_RCTRL)
#define MWKMOD_SHIFT	(MWKMOD_LSHIFT|MWKMOD_RSHIFT)
#define MWKMOD_ALT	(MWKMOD_LALT|MWKMOD_RALT)
#define MWKMOD_META	(MWKMOD_LMETA|MWKMOD_RMETA)

#define MWKINFO_LED_MASK	(1 << 0)
#define MWKINFO_LED_MODE_MASK	(1 << 1)

/* Keyboard info values */
#define MWKINFO_LED_CAP		(1 << 0)
#define MWKINFO_LED_NUM		(1 << 1)
#define MWKINFO_LED_SCR		(1 << 2)

#define MWKINFO_LED_MODE_ON	(1 << 3)
#define MWKINFO_LED_MODE_OFF	(1 << 4)

typedef struct {
	int led;
	int led_mode;
} MWKBINFO, *PMWKBINFO;
#endif /* _MWTYPES_H*/
