#ifndef _MWTYPES_H
#define _MWTYPES_H
/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Exported Microwindows engine typedefs and defines
 */
#include <stdint.h> 		/* for uint32_t, int32_t*/
#if __ECOS
#include <ecosmwconfig.h>	/*include the eCos configuration "translation" header */
#endif

/* configurable options*/
#define USE_ALLOCA	1			/* alloca() is available */

#if 0000	/* use if stdint.h missing*/
/* typedef 32 bit types for 16 and 64 bit environments*/
#if __WORDSIZE == 64
	typedef unsigned int	uint32_t;
	typedef int				int32_t;
#else
	typedef unsigned long	uint32_t;
	typedef long			int32_t;
#endif
#endif

/* compiler specific defines*/
#define MWPACKED	__attribute__ ((aligned(1), packed))

/* force byte-packed structures*/
#if defined(GCC_VERSION)
#define PACKEDDATA			__attribute__ ((__packed__))
#else
#define PACKEDDATA			/* FIXME for MSVC #pragma pack(1) equiv*/
#endif

#if USE_ALLOCA
/* alloca() is available, so use it for better performance */
#define ALLOCA(size)	alloca(size)
#define FREEA(pmem)
#else
/* no alloca(), so use malloc()/free() instead */
#define ALLOCA(size)	malloc(size)
#define FREEA(pmem)	free(pmem)
#endif

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

/* SetFontAttr and capabilities flags (not used with MWTF_ above)*/
#define MWTF_KERNING	0x0001		/* font kerning*/
#define MWTF_ANTIALIAS	0x0002		/* antialiased output*/
#define MWTF_UNDERLINE	0x0004		/* draw underline*/

#define MWTF_FREETYPE	0x1000		/* FIXME: remove*/
#define MWTF_SCALEHEIGHT 0x2000		/* font can scale height seperately*/
#define MWTF_SCALEWIDTH	0x4000		/* font can scale width seperately*/

/* Image data formats, used by GdConversionBlit*/

/* bits per pixel*/
#define MWIF_1BPP			0x00000001L
#define MWIF_8BPP			0x00000008L
#define MWIF_15BPP			0x0000000FL
#define MWIF_16BPP			0x00000010L
#define MWIF_24BPP			0x00000018L
#define MWIF_32BPP			0x00000020L
#define MWIF_BPPMASK		0x0000003FL

/* monochrome bitmap formats*/
#define MWIF_MONO			0x00000040L
#define MWIF_ALPHA			0x00000080L
#define MWIF_BYTEDATA		0x00000100L
#define MWIF_WORDDATA		0x00000200L
#define MWIF_DWORDDATA		0x00000400L
#define MWIF_MSBFIRST		0x00000800L
#define MWIF_LSBFIRST		0x00001000L
#define MWIF_MONOBYTEMSB	(MWIF_1BPP | MWIF_MONO | MWIF_BYTEDATA | MWIF_MSBFIRST)
#define MWIF_MONOBYTELSB	(MWIF_1BPP | MWIF_MONO | MWIF_BYTEDATA | MWIF_LSBFIRST)
#define MWIF_MONOWORDMSB	(MWIF_1BPP | MWIF_MONO | MWIF_WORDDATA | MWIF_MSBFIRST)
#define MWIF_ALPHABYTE		(MWIF_8BPP | MWIF_ALPHA| MWIF_BYTEDATA)

/* color formats*/
#define MWIF_BGRA8888		0x00010000L		/* 32bpp BGRA image byte order (old TRUECOLOR8888)*/
#define MWIF_ARGB8888		0x00020000L		/* 32bpp ARGB image byte order (new)*/
#define MWIF_RGBA8888		0x00030000L		/* 32bpp RGBA image byte order (old TRUECOLORABGR)*/
//#define MWIF_ABGR8888		0x00040000L		/* 32bpp ABGR image byte order (new)*/
#define MWIF_BGR888			0x00050000L		/* 24bpp BGR image byte order  (old TRUECOLOR888)*/
//#define MWIF_RGB888		0x00060000L		/* 24bpp RGB image byte order  (new)*/
#define MWIF_RGB565			0x00070000L		/* 16bpp 5/6/5 RGB packed l.endian (old TRUECOLOR565)*/
//#define MWIF_RGB565_BR	0x00080000L		/* 16bpp 5/6/5 RGB packed b.endian (new)*/
#define MWIF_RGB555			0x00090000L		/* 16bpp 5/5/5 RGB packed l.endian (old TRUECOLOR555)*/
//#define MWIF_RGB555_BR	0x000A0000L		/* 16bpp 5/5/5 RGB packed b.endian (new)*/
#define MWIF_BGR555			0x000B0000L		/* 16bpp 5/5/5 BGR packed l.endian (old TRUECOLOR1555)*/
//#define MWIF_BGR555_BR	0x000C0000L		/* 16bpp 5/5/5 BGR packed b.endian (new)*/
#define MWIF_BGR332			0x000D0000L		/*  8bpp 3/3/2 RGB packed (old TRUECOLOR332)*/
#define MWIF_BGR233			0x000E0000L		/*  8bpp 2/3/3 BGR packed (old TRUECOLOR233)*/
#define MWIF_PAL8			0x000F0000L		/*  8bpp palette (old MWPF_PALETTE)*/

/* Line modes */
#define MWLINE_SOLID      0
#define MWLINE_ONOFF_DASH 1
/* FUTURE: MWLINE_DOUBLE_DASH */

/* Fill mode  */
#define MWFILL_SOLID          0  
#define MWFILL_STIPPLE        1  
#define MWFILL_OPAQUE_STIPPLE 2  
#define MWFILL_TILE           3

/* Drawing modes (raster ops)*/
#define	MWROP_COPY			0	/* src*/
#define	MWROP_XOR			1	/* src ^ dst*/
#define	MWROP_OR			2	/* src | dst*/
#define	MWROP_AND			3	/* src & dst*/
#define	MWROP_CLEAR			4	/* 0*/
#define	MWROP_SET			5	/* ~0, was MWROP_SETTO1*/
#define	MWROP_EQUIV			6	/* ~(src ^ dst)*/
#define	MWROP_NOR			7	/* ~(src | dst)*/
#define	MWROP_NAND			8	/* ~(src & dst)*/
#define	MWROP_INVERT		9	/* ~dst*/
#define	MWROP_COPYINVERTED	10	/* ~src*/
#define	MWROP_ORINVERTED	11	/* ~src | dst*/
#define	MWROP_ANDINVERTED	12	/* ~src & dst*/
#define MWROP_ORREVERSE		13	/* src | ~dst*/
#define	MWROP_ANDREVERSE	14	/* src & ~dst*/
#define	MWROP_NOOP			15	/* dst*/
#define	MWROP_XOR_FGBG		16	/* src ^ background ^ dst (Java XOR mode)*/
#define MWROP_SIMPLE_MAX 	16	/* last non-compositing rop*/

/* Porter-Duff compositing operations.  Only SRC, CLEAR and SRC_OVER are commonly used*/
#define	MWROP_SRC			MWROP_COPY
#define	MWROP_DST			MWROP_NOOP
//#define MWROP_CLEAR		MWROP_CLEAR
#define	MWROP_SRC_OVER		17	/* dst = alphablend src,dst*/
#define	MWROP_DST_OVER		18
#define	MWROP_SRC_IN		19
#define	MWROP_DST_IN		20
#define	MWROP_SRC_OUT		21
#define	MWROP_DST_OUT		22
#define	MWROP_SRC_ATOP		23
#define	MWROP_DST_ATOP		24
#define	MWROP_PORTERDUFF_XOR 25
#define MWROP_SRCTRANSCOPY 26
#define	MWROP_MAX			26	/* last non-blit rop*/

/* blit ROP modes in addtion to MWROP_xxx */
#define MWROP_BLENDCONSTANT		32	/* alpha blend src -> dst with constant alpha*/
#define MWROP_BLENDFGBG			33	/* alpha blend fg/bg color -> dst with src alpha channel*/
//#define MWROP_SRCTRANSCOPY	34	/* copy src -> dst except for transparent color in src*/
//#define MWROP_BLENDCHANNEL	35	/* alpha blend src -> dst with separate per pixel alpha chan*/
//#define MWROP_STRETCH			36	/* stretch src -> dst*/



#define MWROP_USE_GC_MODE		255 /* use current GC mode for ROP.  Nano-X CopyArea only*/

#define MWROP_SRCCOPY		MWROP_COPY	/* obsolete*/
//#define MWROP_SRCAND		MWROP_AND	/* obsolete*/
//#define MWROP_SRCINVERT	MWROP_XOR	/* obsolete*/
//#define MWROP_BLACKNESS   MWROP_CLEAR	/* obsolete*/

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
#define MWPF_TRUECOLOR0888 3	/* pixel is packed 32 bits 0/R/G/B 0RGB truecolor zero alpha*/
#define MWPF_TRUECOLOR888  4	/* pixel is packed 24 bits R/G/B RGB truecolor*/
#define MWPF_TRUECOLOR565  5	/* pixel is packed 16 bits 5/6/5 RGB truecolor*/
#define MWPF_TRUECOLOR555  6	/* pixel is packed 16 bits 5/5/5 RGB truecolor*/
#define MWPF_TRUECOLOR332  7	/* pixel is packed  8 bits 3/3/2 RGB truecolor*/
#define MWPF_TRUECOLOR8888 8	/* pixel is packed 32 bits A/R/G/B ARGB truecolor with alpha */
#define MWPF_TRUECOLOR233  9	/* pixel is packed  8 bits 2/3/3 BGR truecolor*/
#define MWPF_HWPIXELVAL   10	/* pseudo, no convert, pixels are in hw format*/
#define MWPF_TRUECOLORABGR 11	/* pixel is packed 32 bits A/B/G/R ABGR truecolor with alpha */

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
 *       and specifiy the pixel format as MWPF_TRUECOLOR565 etc. when
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
  #if (MWPIXEL_FORMAT == MWPF_TRUECOLOR332) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR233)
  typedef unsigned char MWPIXELVAL;
  #else
    #if MWPIXEL_FORMAT == MWPF_PALETTE
    typedef unsigned char MWPIXELVAL;
    #else
      typedef uint32_t MWPIXELVAL;
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
typedef int				MWCOORD;	/* device coordinates*/
typedef int				MWBOOL;		/* boolean value*/
typedef unsigned char	MWUCHAR;	/* unsigned char*/
typedef uint32_t		MWCOLORVAL;	/* device-independent color value (0xAABBGGRR)*/
typedef unsigned short	MWIMAGEBITS;/* bitmap image unit size*/
typedef uint32_t		MWTIMEOUT;	/* timeout value */
typedef uint32_t		MWTEXTFLAGS;/* MWTF_ text flag*/

#define MWCOORD_MAX	0x7fff		/* maximum coordinate value*/
#define MWCOORD_MIN	(-MWCOORD_MAX)	/* minimum coordinate value*/

/* max char height/width must be >= 16 and a multiple of sizeof(MWIMAGEBITS)*/
#define MAX_CHAR_HEIGHT	128			/* maximum text bitmap height*/
#define MAX_CHAR_WIDTH	128			/* maximum text bitmap width*/
#define	MIN_MWCOORD	((MWCOORD) -32768)	/* minimum coordinate value */
#define	MAX_MWCOORD	((MWCOORD) 32767)	/* maximum coordinate value */

#ifndef TRUE
#define TRUE			1
#endif
#ifndef FALSE
#define FALSE			0
#endif

#define	MWMIN(a,b)		((a) < (b) ? (a) : (b))
#define	MWMAX(a,b) 		((a) > (b) ? (a) : (b))
#define MWABS(x)		((x) < 0 ? -(x) : (x))
#define MWCLAMP(x,a,b)	((x) > (b) ? (b) : ((x) < (a) ? (a) : (x)))

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
	MWCOORD	rows;		/* number of rows on screen */
	MWCOORD cols;		/* number of columns on screen */
	int 	xdpcm;		/* dots/centimeter in x direction */
	int 	ydpcm;		/* dots/centimeter in y direction */
	int	 	planes;		/* hw # planes*/
	int	 	bpp;		/* hw bpp*/
	int32_t	ncolors;	/* hw number of colors supported*/
	int 	fonts;		/* number of built-in fonts */
	int 	buttons;	/* buttons which are implemented */
	MWKEYMOD modifiers;	/* modifiers which are implemented */
	int	 	pixtype;	/* format of pixel value*/
	int	 	portrait;	/* current portrait mode*/
	MWBOOL	fbdriver;	/* true if running mwin fb screen driver*/
	uint32_t rmask;		/* red mask bits in pixel*/
	uint32_t gmask;		/* green mask bits in pixel*/
	uint32_t bmask;		/* blue mask bits in pixel*/
	MWCOORD	xpos;		/* current x mouse position*/
	MWCOORD	ypos;		/* current y mouse position*/

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
	int	pixtype;		/* MWPF_ pixel type*/
	int	bpp;			/* bits per pixel*/
	int	bytespp;		/* bytes per pixel*/
	int	pitch;			/* bytes per scan line for window (=fb pitch)*/
	MWCOORD	x, y;		/* absolute window coordinates*/
	int	portrait_mode;	/* current portrait mode*/
	MWCOORD	xres;		/* real framebuffer resolution*/
	MWCOORD	yres;
	MWCOORD	xvirtres;	/* virtual framebuffer resolution*/
	MWCOORD	yvirtres;
} MWWINDOWFBINFO;

/* builtin C-based proportional/fixed font structure*/
typedef struct {
	char *			name;		/* font name*/
	int				maxwidth;	/* max width in pixels*/
	unsigned int	height;		/* height in pixels*/
	int				ascent;		/* ascent (baseline) height*/
	int				firstchar;	/* first character in bitmap*/
	int				size;		/* font size in characters*/
	const MWIMAGEBITS *bits;	/* 16-bit right-padded bitmap data*/
	const uint32_t 	*offset;	/* offsets into bitmap data*/
	const unsigned char *width;	/* character widths or 0 if fixed*/
	int				defaultchar;/* default char (not glyph index)*/
	int32_t			bits_size;	/* # words of MWIMAGEBITS bits*/
} MWCFONT, *PMWCFONT;

/* draw procs associated with fonts.  Strings are [re]packed using defencoding*/
typedef struct _mwscreendevice *PSD;
typedef struct _mwfont *		PMWFONT;
typedef struct _mwfontinfo *	PMWFONTINFO;

typedef struct {
	int		capabilities;		/* flags for font subdriver capabilities*/
	MWTEXTFLAGS	encoding;	/* routines expect this encoding*/
	MWBOOL	(*Init)(PSD psd);
	PMWFONT	(*CreateFont)(const char *name, MWCOORD height, MWCOORD width, int attr);
	MWBOOL	(*GetFontInfo)(PMWFONT pfont, PMWFONTINFO pfontinfo);
	void 	(*GetTextSize)(PMWFONT pfont, const void *text, int cc,
			MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
			MWCOORD *pbase);
	void	(*GetTextBits)(PMWFONT pfont, int ch, const MWIMAGEBITS **retmap,
			MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
	void	(*DestroyFont)(PMWFONT pfont);
	void	(*DrawText)(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
			const void *str, int count, MWTEXTFLAGS flags);
	int     (*SetFontSize)(PMWFONT pfont, MWCOORD height, MWCOORD width);
	void    (*SetFontRotation)(PMWFONT pfont, int tenthdegrees);
	int     (*SetFontAttr)(PMWFONT pfont, int setflags, int clrflags);
	PMWFONT (*Duplicate) (PMWFONT psrcfont, MWCOORD height, MWCOORD width);
} MWFONTPROCS, *PMWFONTPROCS;

/* new multi-renderer font struct*/
typedef struct _mwfont {		/* common hdr for all font structures*/
	PMWFONTPROCS	fontprocs;	/* font-specific rendering routines*/
	MWCOORD			fontsize;	/* font height in pixels*/
	MWCOORD			fontwidth;	/* font width in pixels*/
	int				fontrotation; /* font rotation*/
	int				fontattr;	/* font attributes: kerning/antialias*/
	/* font-specific rendering data here*/
} MWFONT;

/* builtin core font struct*/
typedef struct {
	/* common hdr*/
	PMWFONTPROCS	fontprocs;
	MWCOORD			fontsize;	/* font height in pixels*/
	MWCOORD			fontwidth;	/* font width in pixels*/
	int				fontrotation;
	int				fontattr;
	/* core font specific data*/
	char *		name;			/* Microwindows font name*/
	PMWCFONT	cfont;			/* builtin font data*/
} MWCOREFONT, *PMWCOREFONT;

/* GdConversionBlit parameter structure*/
typedef struct {
	int			op;				/* MWROP operation requested*/
	int			data_format;	/* MWIF_ image data format*/
	MWCOORD		width, height;	/* width and height for src and dest*/
	MWCOORD		dstx, dsty;		/* dest x, y*/
	MWCOORD		srcx, srcy;		/* source x, y*/
	int			src_pitch;		/* source row length in bytes*/
	uint32_t	fg_color;		/* foreground color, hw pixel format*/
	uint32_t	bg_color;
	MWBOOL		usebg;			/* set =1 to draw background*/
	void *		data;			/* input image data GdConversionBlit*/

	/* these items filled in by GdConversionBlit*/
	void *		data_out;		/* output image from conversion blits subroutines*/
	MWCOORD		dst_pitch;		/* dest row length in bytes*/

//	uint32_t	transcolor;		/* trans color for MWROP_SRCTRANSCOPY*/
//	PSD			alphachan;		/* alpha chan for MWROP_BLENDCHANNEL*/
//	void *		misc;			/* alpha channel for PSDOP_ALPHAMAP*/
} MWBLITPARMS, *PMWBLITPARMS;

typedef struct { // DEPRECATED
	int op;
	MWCOORD width, height;
	MWCOORD dstx, dsty;
	MWCOORD srcx, srcy;
	MWPIXELVAL fg_color;
	MWPIXELVAL bg_color;
	int usebg;
	void *data;

	MWCOORD src_linelen;		// must be set in GdConversionBlit
	MWCOORD dst_linelen;		// must be set in GdConversionBlit
} driver_gc_t;

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
typedef struct _mwfontinfo {
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
} MWFONTINFO;


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
	int32_t	lfHeight;		/* desired height in pixels*/
	int32_t	lfWidth;		/* desired width in pixels or 0*/
	int32_t	lfEscapement;	/* rotation in tenths of degree*/
	int32_t	lfOrientation;	/* not used*/
	int32_t	lfWeight;		/* font weight*/
	MWUCHAR	lfItalic;		/* =1 for italic */
	MWUCHAR	lfUnderline;	/* =1 for underline */
	MWUCHAR	lfStrikeOut;	/* not used*/
	MWUCHAR	lfCharSet;		/* font character set*/
	MWUCHAR	lfOutPrecision;	/* font type selection*/
	MWUCHAR	lfClipPrecision;/* not used*/
	MWUCHAR	lfQuality;		/* not used*/
	MWUCHAR lfPitchAndFamily;/* not used*/
	/* end of windows-compatibility*/

	MWUCHAR lfClass;		/* font class (renderer) */

	/* Following only used by (the legacy) FONTMAPPER when enabled.
	 * They are only kept around to stay source and binary
	 * compatible to previous microwindows releases.
	 */
	MWUCHAR	lfPitch;		/* font pitch */
	MWUCHAR	lfRoman;		/* =1 for Roman letters (upright) */
	MWUCHAR	lfSerif;		/* =1 for Serifed font */
	MWUCHAR	lfSansSerif;	/* =1 for Sans-serif font */
	MWUCHAR	lfModern;		/* =1 for Modern font */
	MWUCHAR	lfMonospace;	/* =1 for Monospaced font */
	MWUCHAR	lfProportional;	/* =1 for Proportional font */
	MWUCHAR	lfOblique;		/* =1 for Oblique (kind of Italic) */
	MWUCHAR	lfSmallCaps;	/* =1 for small caps */
	/* End of fontmapper-only variables */

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

/* static clip rectangle: drawing allowed if point within rectangle*/
typedef struct {
	MWCOORD 	x;		/* x coordinate of top left corner */
	MWCOORD 	y;		/* y coordinate of top left corner */
	MWCOORD 	width;		/* width of rectangle */
	MWCOORD 	height;		/* height of rectangle */
} MWCLIPRECT;

/* dynamically allocated multi-rectangle clipping region*/
typedef struct {
	int	size;		/* malloc'd # of rectangles*/
	int	numRects;	/* # rectangles in use*/
	int	type; 		/* region type*/
	MWRECT *rects;		/* rectangle array*/
	MWRECT	extents;	/* bounding box of region*/
} MWCLIPREGION;

typedef struct {
	MWCOORD	width;
	MWCOORD	height;
	PSD	psd;
} MWTILE;

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
#define MWIMAGE_555		0x08	/* compression flag: 5/5/5 format*/

typedef struct {
	int		width;		/* image width in pixels*/
	int		height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel (1, 4 or 8)*/
	int		pitch;		/* bytes per line*/
	int		bytesperpixel;	/* bytes per pixel*/
	int		compression;/* compression algorithm*/
	int		palsize;	/* palette size*/
	int32_t	transcolor;	/* transparent color or -1 if none*/
	MWPALENTRY *palette;/* palette*/
	MWUCHAR *imagebits;	/* image bits (dword right aligned)*/
} MWIMAGEHDR, *PMWIMAGEHDR;

/* image information structure - returned by GdGetImageInfo*/
typedef struct {
	int		id;			/* image id*/
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
	int			width;			/* cursor width in pixels*/
	int			height;			/* cursor height in pixels*/
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
	int	s;			/* denom*/
} MWTRANSFORM;

/* outline and filled arc and pie types*/
#define MWARC		0x0001	/* arc*/
#define MWOUTLINE	0x0002
#define MWARCOUTLINE	0x0003	/* arc + outline*/
#define MWPIE		0x0004	/* pie (filled)*/
#define MWELLIPSE	0x0008	/* ellipse outline*/
#define MWELLIPSEFILL	0x0010	/* ellipse filled*/

/* create MWCOLORVAL (0xAABBGGRR format)*/
#define MWARGB(a,r,g,b)	((MWCOLORVAL)(((unsigned char)(r)|\
				(((uint32_t)(unsigned char)(g))<<8))|\
				(((uint32_t)(unsigned char)(b))<<16)|\
				(((uint32_t)(unsigned char)(a))<<24)))
#define MWRGB(r,g,b)	MWARGB(255,(r),(g),(b))		/* argb 255 alpha*/
#define MW0RGB(r,g,b)	MWARGB(0,(r),(g),(b))		/* rgb 0 alpha*/

/* no color, used for transparency, should not be 0, -1 or any MWRGB color*/
#define MWNOCOLOR	0x01000000L

#ifdef MWINCLUDECOLORS
/*
 * Alpha blending evolution
 *
 * Blending r,g,b pixels w/src alpha
 * unoptimized two mult one div		 	bg = (a*fg+(255-a)*bg)/255
 * optimized one mult one div			bg = (a*(fg-bg))/255 + bg
 * optimized /255 replaced with +1/>>8	bg = (((a+1)*(fg-bg))>>8) + bg
 * optimized +=							bg +=(((a+1)*(fg-bg))>>8)
 * macro +=								bg +=muldiv255(a,fg-bg)
 * macro =								bg  =muldiv255(a,fg-bg) + bg
 * -or-
 * macro = (src/dst reversed)			bg  =muldiv255(255-a,bg-fg) + fg
 *
 * Updating dst alpha from src alpha
 * original routine						d =   (255-d)*a/255 + d
 * rearrange							d =   a*(255-d)/255 + d
 * replace multiply by fast +1>>8		d = (((a+1)*(255-d)) >> 8) + d
 * macro =								d =  muldiv255(a, 255 - d) + d
 * macro +=								d += muldiv255(a, 255 - d)
 * -or- src/dst reversed (method used in 0.91, not quite correct)
 * mathematical correct  routine		d =  (d * (255 - a)/255 + a
 * rearrange							d = ((d * (255 - a + 1)) >> 8) + a
 * alternate (used in v0.91)			d = ((d * (256 - a)) >> 8) + a
 * macro = (to duplicate 0.91 code)		d = muldiv255(255 - a, d) + a
 * correct macro =						d = muldiv255(d, 255 - a) + a
 */
//#define muldiv255(a,b)	(((a)*(b))/255)		/* slow divide, exact*/
#define muldiv255(a,b)		((((a)+1)*(b))>>8)		/* very fast, 92% accurate*/
//#define muldiv255(a,b)	((((a)+((a)>>7))*(b))>>8)	/* fast, 35% accurate*/
#define mulscale(a,b,n)		((((a)+1)*(b))>>(n))	/* very fast, always shift for 16bpp*/

/* palette color definition*/
#define RGBDEF(r,g,b)	{r, g, b}

/* return palette entry as MWCOLORVAL (0xAABBGGRR)*/
#define GETPALENTRY(pal,index) ((MWCOLORVAL)(pal[index].r | (pal[index].g << 8) |\
									    (pal[index].b << 16) | (255 << 24)))

/* extract MWCOLORVAL (0xAABBGGRR) values*/
#define REDVALUE(rgb)	((rgb) & 0xff)
#define GREENVALUE(rgb) (((rgb) >> 8) & 0xff)
#define BLUEVALUE(rgb)	(((rgb) >> 16) & 0xff)
#define ALPHAVALUE(rgb)	(((rgb) >> 24) & 0xff)

/* Truecolor color conversion and extraction macros*/
/*
 * Conversion from 8-bit RGB components to MWPIXELVAL
 */
/* create 32 bit 8/8/8/8 format pixel (0xAARRGGBB) from RGB triplet*/
#define RGB2PIXEL8888(r,g,b)	\
	(0xFF000000UL | ((r) << 16) | ((g) << 8) | (b))

/* create 32 bit 8/8/8/8 format pixel (0xAABBGGRR) from RGB triplet*/
#define RGB2PIXELABGR(r,g,b)	\
	(0xFF000000UL | ((b) << 16) | ((g) << 8) | (r))

/* create 32 bit 8/8/8/8 format pixel (0xAARRGGBB) from ARGB quad*/
#define ARGB2PIXEL8888(a,r,g,b)	\
	(((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

/* create 32 bit 8/8/8/8 format pixel (0xAABBGGRR) from ARGB quad*/
#define ARGB2PIXELABGR(a,r,g,b)	\
	(((a) << 24) | ((b) << 16) | ((g) << 8) | (r))

/* create 24 bit 8/8/8 format pixel (0x00RRGGBB) from RGB triplet*/
#define RGB2PIXEL888(r,g,b)	\
	(((r) << 16) | ((g) << 8) | (b))

/* create 16 bit 5/6/5 format pixel from RGB triplet */
#define RGB2PIXEL565(r,g,b)	\
	((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

/* create 16 bit 5/5/5 format pixel from RGB triplet */
#define RGB2PIXEL555(r,g,b)	\
	((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3))

/* create 16 bit b/g/r 5/5/5 format pixel from RGB triplet */
#define RGB2PIXEL1555(r,g,b)	\
	((((b) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((r) & 0xf8) >> 3) | 0x8000)

/* create 8 bit 3/3/2 format pixel from RGB triplet*/
#define RGB2PIXEL332(r,g,b)	\
	(((r) & 0xe0) | (((g) & 0xe0) >> 3) | (((b) & 0xc0) >> 6))

/* create 8 bit 2/3/3 format pixel from RGB triplet*/
#define RGB2PIXEL233(r,g,b)	\
	((((r) & 0xe0) >> 5) | (((g) & 0xe0) >> 2) | (((b) & 0xc0) >> 0))


/*
 * Conversion from MWCOLORVAL to MWPIXELVAL
 */
/* create 32 bit 8/8/8/8 format pixel from ABGR colorval (0xAABBGGRR)*/
/* In this format, alpha is preserved. */
#define COLOR2PIXEL8888(c)	\
	((((c) & 0xff) << 16) | ((c) & 0xff00ff00ul) | (((c) & 0xff0000) >> 16))

/* create 32 bit 8/8/8/8 format pixel from ABGR colorval (0xAABBGGRR)*/
/* In this format, alpha is preserved. */
#define COLOR2PIXELABGR(c)	\
        (c)

/* create 24 bit 8/8/8 format pixel from 0BGR colorval (0x00BBGGRR)*/
/* In this format, alpha is ignored. */
#define COLOR2PIXEL888(c)	\
	((((c) & 0xff) << 16) | ((c) & 0xff00) | (((c) & 0xff0000) >> 16))

/* create 16 bit 5/6/5 format pixel from 0BGR colorval (0x00BBGGRR)*/
/* In this format, alpha is ignored. */
#define COLOR2PIXEL565(c)	\
	((((c) & 0xf8) << 8) | (((c) & 0xfc00) >> 5) | (((c) & 0xf80000) >> 19))

/* create 16 bit 5/5/5 format pixel from 0BGR colorval (0x00BBGGRR)*/
/* In this format, alpha is ignored. */
#define COLOR2PIXEL555(c)	\
	((((c) & 0xf8) << 7) | (((c) & 0xf800) >> 6) | (((c) & 0xf80000) >> 19))

/* create 16 bit b/g/r 5/5/5 format pixel from 0BGR colorval (0x00BBGGRR)*/
/* In this format, alpha is ignored. */
#define COLOR2PIXEL1555(c)	\
	((((c) & 0xf8) >> 3) | (((c) & 0xf800) >> 6) | (((c) & 0xf80000) >> 9) | 0x8000)

/* create 8 bit 3/3/2 format pixel from 0BGR colorval (0x00BBGGRR)*/
/* In this format, alpha is ignored. */
#define COLOR2PIXEL332(c)	\
	(((c) & 0xe0) | (((c) & 0xe000) >> 11) | (((c) & 0xc00000) >> 22))

/* create 8 bit 2/3/3 format pixel from 0BGR colorval (0x00BBGGRR)*/
/* In this format, alpha is ignored. */
#define COLOR2PIXEL233(c)	\
        ((((c) & 0xC00000) >> 16) | (((c) & 0x00E000) >> 10) | (((c) & 0xE0) >> 5))

/*
 * Conversion from MWPIXELVAL to red, green or blue components
 */
/* return 8/8/8/8 bit a, r, g or b component of 32 bit pixelval*/
#define PIXEL8888ALPHA(pixelval)	(((pixelval) >> 24) & 0xff)
#define PIXEL8888RED(pixelval)  	(((pixelval) >> 16) & 0xff)
#define PIXEL8888GREEN(pixelval)	(((pixelval) >> 8) & 0xff)
#define PIXEL8888BLUE(pixelval) 	((pixelval) & 0xff)

/* return 8/8/8/8 bit a, r, g or b component of 32 bit pixelval*/
#define PIXELABGRALPHA(pixelval)	(((pixelval) >> 24) & 0xff)
#define PIXELABGRBLUE(pixelval)  	(((pixelval) >> 16) & 0xff)
#define PIXELABGRGREEN(pixelval)	(((pixelval) >> 8) & 0xff)
#define PIXELABGRRED(pixelval)		((pixelval) & 0xff)

/* return 8/8/8 bit r, g or b component of 24 bit pixelval*/
#define PIXEL888RED(pixelval)		(((pixelval) >> 16) & 0xff)
#define PIXEL888GREEN(pixelval)		(((pixelval) >> 8) & 0xff)
#define PIXEL888BLUE(pixelval)		((pixelval) & 0xff)

/* return 5/6/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL565RED(pixelval)		(((pixelval) >> 11) & 0x1f)
#define PIXEL565GREEN(pixelval)		(((pixelval) >> 5) & 0x3f)
#define PIXEL565BLUE(pixelval)		((pixelval) & 0x1f)

/* return 5/5/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL555RED(pixelval)		(((pixelval) >> 10) & 0x1f)
#define PIXEL555GREEN(pixelval)		(((pixelval) >> 5) & 0x1f)
#define PIXEL555BLUE(pixelval)		((pixelval) & 0x1f)

/* return b/g/r 5/5/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL1555BLUE(pixelval)		(((pixelval) >> 10) & 0x1f)
#define PIXEL1555GREEN(pixelval)	(((pixelval) >> 5) & 0x1f)
#define PIXEL1555RED(pixelval)		((pixelval) & 0x1f)

/* return 3/3/2 bit r, g or b component of 8 bit pixelval*/
#define PIXEL332RED(pixelval)		(((pixelval) >> 5) & 0x07)
#define PIXEL332GREEN(pixelval)		(((pixelval) >> 2) & 0x07)
#define PIXEL332BLUE(pixelval)		((pixelval) & 0x03)

/* return 2/3/3 bit b, g, r component of 8 bit pixelval*/
#define PIXEL233RED(pixelval)		((pixelval) & 0x07)
#define PIXEL233GREEN(pixelval)		(((pixelval) >> 3) & 0x07)
#define PIXEL233BLUE(pixelval)		(((pixelval) >> 6) & 0x03)

/*
 * Conversion from MWPIXELVAL to normal 8-bit red, green or blue components
 */
/* return 8/8/8/8 bit a, r, g or b component of 32 bit pixelval*/
#define PIXEL8888ALPHA8(pixelval)		(((pixelval) >> 24) & 0xff)
#define PIXEL8888RED8(pixelval)			(((pixelval) >> 16) & 0xff)
#define PIXEL8888GREEN8(pixelval)		(((pixelval) >> 8) & 0xff)
#define PIXEL8888BLUE8(pixelval)		((pixelval) & 0xff)

/* return 8/8/8/8 bit a, r, g or b component of 32 bit pixelval*/
#define PIXELABGRALPHA8(pixelval)		(((pixelval) >> 24) & 0xff)
#define PIXELABGRBLUE8(pixelval)		(((pixelval) >> 16) & 0xff)
#define PIXELABGRGREEN8(pixelval)		(((pixelval) >> 8) & 0xff)
#define PIXELABGRRED8(pixelval)			((pixelval) & 0xff)

/* return 8 bit r, g or b component of 8/8/8 24 bit pixelval*/
#define PIXEL888RED8(pixelval)          (((pixelval) >> 16) & 0xff)
#define PIXEL888GREEN8(pixelval)        (((pixelval) >> 8) & 0xff)
#define PIXEL888BLUE8(pixelval)         ((pixelval) & 0xff)

/* return 8 bit r, g or b component of 5/6/5 16 bit pixelval*/
#define PIXEL565RED8(pixelval)          (((pixelval) >> 8) & 0xf8)
#define PIXEL565GREEN8(pixelval)        (((pixelval) >> 3) & 0xfc)
#define PIXEL565BLUE8(pixelval)         (((pixelval) << 3) & 0xf8)

/* return 8 bit r, g or b component of 5/5/5 16 bit pixelval*/
#define PIXEL555RED8(pixelval)          (((pixelval) >> 7) & 0xf8)
#define PIXEL555GREEN8(pixelval)        (((pixelval) >> 2) & 0xf8)
#define PIXEL555BLUE8(pixelval)         (((pixelval) << 3) & 0xf8)

/* return 8 bit r, g or b component of b/g/r 5/5/5 16 bit pixelval*/
#define PIXEL1555BLUE8(pixelval)		(((pixelval) >> 7) & 0xf8)
#define PIXEL1555GREEN8(pixelval)		(((pixelval) >> 2) & 0xf8)
#define PIXEL1555RED8(pixelval)			(((pixelval) << 3) & 0xf8)

/* return 8 bit r, g or b component of 3/3/2 8 bit pixelval*/
#define PIXEL332RED8(pixelval)          ( (pixelval)       & 0xe0)
#define PIXEL332GREEN8(pixelval)        (((pixelval) << 3) & 0xe0)
#define PIXEL332BLUE8(pixelval)         (((pixelval) << 6) & 0xc0)

/* return 8 bit r, g or b component of 2/3/3 8 bit pixelval*/
#define PIXEL233RED8(pixelval)          (((pixelval) << 5) & 0xe0)
#define PIXEL233GREEN8(pixelval)        (((pixelval) << 2) & 0xe0)
#define PIXEL233BLUE8(pixelval)         ( (pixelval)       & 0xc0)

/*
 * Conversion from MWPIXELVAL to *32-bit* red, green or blue components
 * (i.e. PIXEL888RED32(x) == (PIXEL888RED8(x) << 24).  These macros optimize
 * out the extra shift.)
 */
/* return 32 bit a, r, g or b component of 8/8/8/8 32 bit pixelval*/
#define PIXEL8888ALPHA32(pixelval)        ( ((uint32_t)(pixelval))        & 0xff000000UL)
#define PIXEL8888RED32(pixelval)          ((((uint32_t)(pixelval)) <<  8) & 0xff000000UL)
#define PIXEL8888GREEN32(pixelval)        ((((uint32_t)(pixelval)) << 16) & 0xff000000UL)
#define PIXEL8888BLUE32(pixelval)         ((((uint32_t)(pixelval)) << 24) & 0xff000000UL)

/* return 32 bit a, r, g or b component of 8/8/8/8 32 bit pixelval*/
#define PIXELABGRALPHA32(pixelval)        ( ((uint32_t)(pixelval))        & 0xff000000UL)
#define PIXELABGRBLUE32(pixelval)         ((((uint32_t)(pixelval)) <<  8) & 0xff000000UL)
#define PIXELABGRGREEN32(pixelval)        ((((uint32_t)(pixelval)) << 16) & 0xff000000UL)
#define PIXELABGRRED32(pixelval)          ((((uint32_t)(pixelval)) << 24) & 0xff000000UL)

/* return 32 bit r, g or b component of 8/8/8 24 bit pixelval*/
#define PIXEL888RED32(pixelval)          ((((uint32_t)(pixelval)) <<  8) & 0xff000000UL)
#define PIXEL888GREEN32(pixelval)        ((((uint32_t)(pixelval)) << 16) & 0xff000000UL)
#define PIXEL888BLUE32(pixelval)         ((((uint32_t)(pixelval)) << 24) & 0xff000000UL)

/* return 32 bit r, g or b component of 5/6/5 16 bit pixelval*/
#define PIXEL565RED32(pixelval)          ((((uint32_t)(pixelval)) << 16) & 0xf8000000UL)
#define PIXEL565GREEN32(pixelval)        ((((uint32_t)(pixelval)) << 21) & 0xfc000000UL)
#define PIXEL565BLUE32(pixelval)         ((((uint32_t)(pixelval)) << 27) & 0xf8000000UL)

/* return 32 bit r, g or b component of 5/5/5 16 bit pixelval*/
#define PIXEL555RED32(pixelval)          ((((uint32_t)(pixelval)) << 17) & 0xf8000000UL)
#define PIXEL555GREEN32(pixelval)        ((((uint32_t)(pixelval)) << 22) & 0xf8000000UL)
#define PIXEL555BLUE32(pixelval)         ((((uint32_t)(pixelval)) << 27) & 0xf8000000UL)

/* return 32 bit r, g or b component of 3/3/2 8 bit pixelval*/
#define PIXEL332RED32(pixelval)          ((((uint32_t)(pixelval)) << 24) & 0xe0000000UL)
#define PIXEL332GREEN32(pixelval)        ((((uint32_t)(pixelval)) << 27) & 0xe0000000UL)
#define PIXEL332BLUE32(pixelval)         ((((uint32_t)(pixelval)) << 30) & 0xc0000000UL)

/* return 32 bit r, g or b component of 2/3/3 8 bit pixelval*/
#define PIXEL233RED32(pixelval)          ((((uint32_t)(pixelval)) << 29) & 0xe0000000UL)
#define PIXEL233GREEN32(pixelval)        ((((uint32_t)(pixelval)) << 26) & 0xe0000000UL)
#define PIXEL233BLUE32(pixelval)         ((((uint32_t)(pixelval)) << 24) & 0xc0000000UL)

/*
 * Conversion from MWPIXELVAL to MWCOLORVAL
 */
/* create ABGR colorval (0xAABBGGRR) from 8/8/8/8 ARGB (0xAARRGGBB) format pixel*/
#define PIXEL8888TOCOLORVAL(p)	\
	((((p) & 0xff0000ul) >> 16) | ((p) & 0xff00ff00ul) | (((p) & 0xffu) << 16) | 0xff000000ul)

/* create ABGR colorval (0xAABBGGRR) from 8/8/8/8 ABGR (0xAABBGGRR) format pixel*/
#define PIXELABGRTOCOLORVAL(p)	\
	((p) | 0xff000000ul)

/* create ABGR colorval (0xFFBBGGRR) from 8/8/8 RGB (0x00RRGGBB) format pixel*/
#define PIXEL888TOCOLORVAL(p)	\
	(0xff000000ul | (((p) & 0xff0000ul) >> 16) | ((p) & 0xff00u) | (((p) & 0xffu) << 16) | 0xff000000ul)

/* create ABGR colorval (0xFFBBGGRR) from 5/6/5 format pixel*/
#define PIXEL565TOCOLORVAL(p)	\
	(0xff000000ul | (((p) & 0xf800u) >> 8) | (((p) & 0x07e0u) << 5) | (((p) & 0x1ful) << 19) | 0xff000000ul)

/* create ABGR colorval (0xFFBBGGRR) from 5/5/5 format pixel*/
#define PIXEL555TOCOLORVAL(p)	\
	(0xff000000ul | (((p) & 0x7c00u) >> 7) | (((p) & 0x03e0u) << 6) | (((p) & 0x1ful) << 19) | 0xff000000ul)

/* create ABGR colorval (0xFFBBGGRR) from 3/3/2 format pixel*/
#define PIXEL332TOCOLORVAL(p)	\
	(0xff000000ul | (((p) & 0xe0u)) | (((p) & 0x1cu) << 11) | (((p) & 0x03ul) << 19) | 0xff000000ul)

/* create ABGR colorval (0x00BBGGRR) from 2/3/3 format pixel*/
#define PIXEL233TOCOLORVAL(p)	\
	(((p) & 0x07) | (((p) & 0x38) << 5) | (((p) & 0xC0) << 14))

/*
 * Conversion from ARGB values
 */
/* create 16 bit 5/6/5 format pixel from ARGB value (0xAARRGGBB)*/
#define ARGB2PIXEL565(c) \
	((((c) & 0x0000f8) >> 3) | (((c) & 0x00fc00) >> 5) | (((c) & 0xf80000) >> 8))

/* convert ARGB (0xAARRGGBB) to ABGR colorval (0xAABBGGRR)*/
#define ARGB2COLORVAL(c) \
	(((c) & 0xFF00FF00UL) | (((c) & 0x00FF0000UL) >> 16) | (((c) & 0x000000FFUL) << 16))

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR8888
#define RGB2PIXEL(r,g,b)	RGB2PIXEL8888(r,g,b)
#define COLORVALTOPIXELVAL(c)	COLOR2PIXEL8888(c)
#define PIXELVALTOCOLORVAL(p)	PIXEL8888TOCOLORVAL(p)
#define PIXEL2RED(p)		PIXEL8888RED(p)
#define PIXEL2GREEN(p)		PIXEL8888GREEN(p)
#define PIXEL2BLUE(p)		PIXEL8888BLUE(p)
#endif

#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
#define RGB2PIXEL(r,g,b)	RGB2PIXELABGR(r,g,b)
#define COLORVALTOPIXELVAL(c)	COLOR2PIXELABGR(c)
#define PIXELVALTOCOLORVAL(p)	PIXELABGRTOCOLORVAL(p)
#define PIXEL2RED(p)		PIXELABGRRED(p)
#define PIXEL2GREEN(p)		PIXELABGRGREEN(p)
#define PIXEL2BLUE(p)		PIXELABGRBLUE(p)
#endif

#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR888) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR0888)
#define RGB2PIXEL(r,g,b)	RGB2PIXEL888(r,g,b)
#define COLORVALTOPIXELVAL(c)	COLOR2PIXEL888(c)
#define PIXELVALTOCOLORVAL(p)	PIXEL888TOCOLORVAL(p)
#define PIXEL2RED(p)		PIXEL888RED(p)
#define PIXEL2GREEN(p)		PIXEL888GREEN(p)
#define PIXEL2BLUE(p)		PIXEL888BLUE(p)
#endif

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR565
#define RGB2PIXEL(r,g,b)	RGB2PIXEL565(r,g,b)
#define COLORVALTOPIXELVAL(c)	COLOR2PIXEL565(c)
#define PIXELVALTOCOLORVAL(p)	PIXEL565TOCOLORVAL(p)
#define PIXEL2RED(p)		PIXEL565RED(p)
#define PIXEL2GREEN(p)		PIXEL565GREEN(p)
#define PIXEL2BLUE(p)		PIXEL565BLUE(p)
#endif

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR555
#define RGB2PIXEL(r,g,b)	RGB2PIXEL555(r,g,b)
#define COLORVALTOPIXELVAL(c)	COLOR2PIXEL555(c)
#define PIXELVALTOCOLORVAL(p)	PIXEL555TOCOLORVAL(p)
#define PIXEL2RED(p)		PIXEL555RED(p)
#define PIXEL2GREEN(p)		PIXEL555GREEN(p)
#define PIXEL2BLUE(p)		PIXEL555BLUE(p)
#endif

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
#define RGB2PIXEL(r,g,b)	RGB2PIXEL332(r,g,b)
#define COLORVALTOPIXELVAL(c)	COLOR2PIXEL332(c)
#define PIXELVALTOCOLORVAL(p)	PIXEL332TOCOLORVAL(p)
#define PIXEL2RED(p)		PIXEL332RED(p)
#define PIXEL2GREEN(p)		PIXEL332GREEN(p)
#define PIXEL2BLUE(p)		PIXEL332BLUE(p)
#endif

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR233
#define RGB2PIXEL(r,g,b)	RGB2PIXEL233(r,g,b)
#define COLORVALTOPIXELVAL(c)	COLOR2PIXEL233(c)
#define PIXELVALTOCOLORVAL(p)	PIXEL233TOCOLORVAL(p)
#define PIXEL2RED(p)		PIXEL233RED(p)
#define PIXEL2GREEN(p)		PIXEL233GREEN(p)
#define PIXEL2BLUE(p)		PIXEL233BLUE(p)
#endif

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
#define RED			MWRGB( 128, 0  , 0   )
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

#if 0000
/* colors assumed in first 16 palette entries*/
/* note: don't use palette indices if the palette may
 * be reloaded.  Use the RGB values instead.
 */
#define BLACK		PALINDEX(0)		/*   0,   0,   0*/
#define BLUE		PALINDEX(1)
#define GREEN		PALINDEX(2)
#define CYAN		PALINDEX(3)
#define RED			PALINDEX(4)
#define MAGENTA		PALINDEX(5)
#define BROWN		PALINDEX(6)
#define LTGRAY		PALINDEX(7)		/* 192, 192, 192*/
#define GRAY		PALINDEX(8)		/* 128, 128, 128*/
#define LTBLUE		PALINDEX(9)
#define LTGREEN		PALINDEX(10)
#define LTCYAN		PALINDEX(11)
#define LTRED		PALINDEX(12)
#define LTMAGENTA	PALINDEX(13)
#define YELLOW		PALINDEX(14)
#define WHITE		PALINDEX(15)	/* 255, 255, 255*/
#endif

#endif /* MWINCLUDECOLORS*/

/* Mouse button bits*/
#define MWBUTTON_L	04
#define MWBUTTON_M	02
#define MWBUTTON_R	01

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

#define MWKEY_HAVI_KEY_BASE   (MWKEY_END_NORMAL+1) /* MWKEY code for first HAVi key */
#define MWKEY_HAVI_CODE_FIRST  403			/* HAVi code for first HAVi key */
#define MWKEY_HAVI_CODE_LAST   460			/* HAVi code for last HAVi key */
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
#define MWKMOD_SCR			0x8000

#define MWKMOD_CTRL	(MWKMOD_LCTRL|MWKMOD_RCTRL)
#define MWKMOD_SHIFT	(MWKMOD_LSHIFT|MWKMOD_RSHIFT)
#define MWKMOD_ALT	(MWKMOD_LALT|MWKMOD_RALT)
#define MWKMOD_META	(MWKMOD_LMETA|MWKMOD_RMETA)

typedef struct {
	int led;
	int led_mode;
} MWKBINFO, *PMWKBINFO;

#define MWKINFO_LED_MASK	(1 << 0)
#define MWKINFO_LED_MODE_MASK	(1 << 1)

/* Keyboard info values */
#define MWKINFO_LED_CAP		(1 << 0)
#define MWKINFO_LED_NUM		(1 << 1)
#define MWKINFO_LED_SCR		(1 << 2)
#define MWKINFO_LED_MODE_ON	(1 << 3)
#define MWKINFO_LED_MODE_OFF (1 << 4)

#endif /* _MWTYPES_H*/
