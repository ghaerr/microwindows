/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2003, 2005, 2010, 2011 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Exported Microwindows engine typedefs and defines
 */

#define MWPACKED
#define PACKEDDATA			/* FIXME for MSVC #pragma pack(1) equiv*/
#define ALWAYS_INLINE

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
#define MWTF_BOLD		0x0008		/* draw bold glyph (not present on all renderers)*/

#define MWTF_CMAP_DEFAULT 0x0010	/* use default (unicode) charset in truetype font (not required)*/
#define MWTF_CMAP_0		  0x0020	/* use charmap 0 in truetype font*/
#define MWTF_CMAP_1       0x0040	/* use charmap 1 in truetype font*/

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
#define MWIF_HASALPHA		0x00000080L
#define MWIF_BYTEDATA		0x00000100L
#define MWIF_LEWORDDATA		0x00000200L		/* 16-bit little endian format (retrofit format)*/
//#define MWIF_BEQUADDATA	0x00000400L		/* 32-bit big endian format*/
#define MWIF_MSBFIRST		0x00000800L		/* highest bit displayed leftmost*/
#define MWIF_LSBFIRST		0x00001000L		/* lowest bit displayed leftmost*/
#define MWIF_MONOBYTEMSB	(MWIF_1BPP | MWIF_MONO | MWIF_BYTEDATA | MWIF_MSBFIRST)
#define MWIF_MONOBYTELSB	(MWIF_1BPP | MWIF_MONO | MWIF_BYTEDATA | MWIF_LSBFIRST)
#define MWIF_MONOWORDMSB	(MWIF_1BPP | MWIF_MONO | MWIF_LEWORDDATA | MWIF_MSBFIRST)
//#define MWIF_MONOQUADMSB	(MWIF_1BPP | MWIF_MONO | MWIF_BEQUADDATA | MWIF_MSBFIRST)
#define MWIF_ALPHABYTE		(MWIF_8BPP | MWIF_HASALPHA| MWIF_BYTEDATA)

/* framebuffer and image data formats - yet unsupported formats commented out*/
#define MWIF_BGRA8888	(0x00010000L|MWIF_HASALPHA) /* 32bpp BGRA image byte order (old TRUECOLORARGB)*/
#define MWIF_RGBA8888	(0x00020000L|MWIF_HASALPHA)	/* 32bpp RGBA image byte order (old TRUECOLORABGR)*/
//#define MWIF_ARGB8888	(0x00030000L|MWIF_HASALPHA)	/* 32bpp ARGB image byte order (new)*/
//#define MWIF_ABGR8888	(0x00040000L|MWIF_HASALPHA)	/* 32bpp ABGR image byte order (new)*/
//#define MWIF_BGRX8888	(0x00050000L|MWIF_HASALPHA)	/* 32bpp BGRX image order no alpha (new)*/
#define MWIF_BGR888		 0x00060000L		/* 24bpp BGR image byte order  (old TRUECOLOR888)*/
#define MWIF_RGB888		 0x00070000L		/* 24bpp RGB image byte order  (png no alpha)*/
#define MWIF_RGB565		 0x00080000L		/* 16bpp 5/6/5 RGB packed l.endian (old TRUECOLOR565)*/
//#define MWIF_RGB565_BE 0x00090000L		/* 16bpp 5/6/5 RGB packed b.endian (new)*/
#define MWIF_RGB555		 0x000A0000L		/* 16bpp 5/5/5 RGB packed l.endian (old TRUECOLOR555)*/
//#define MWIF_RGB555_BE 0x000B0000L		/* 16bpp 5/5/5 RGB packed b.endian (new)*/
#define MWIF_RGB1555	 0x000C0000L		        /* 16bpp 1/5/5/5 NDS color format */
//#define MWIF_BGR555_BE 0x000D0000L		/* 16bpp 5/5/5 BGR packed b.endian (new)*/
#define MWIF_RGB332		 0x000E0000L		/*  8bpp 3/3/2 RGB packed (old TRUECOLOR332)*/
#define MWIF_BGR233		 0x000F0000L		/*  8bpp 2/3/3 BGR packed (old TRUECOLOR233)*/
#define MWIF_PAL1		 MWIF_MONOBYTEMSB	/*  1bpp palette (old MWPF_PALETTE)*/
#define MWIF_PAL2		 0x00200000L		/*  2bpp palette (old MWPF_PALETTE)*/
#define MWIF_PAL4		 0x00400000L		/*  4bpp palette (old MWPF_PALETTE)*/
#define MWIF_PAL8		 0x00800000L		/*  8bpp palette (old MWPF_PALETTE)*/
//#define MWIF_PALETTE	 0x00F00000L		/* requires palette*/

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
#define	MWROP_OR			2	/* src | dst (PAINT)*/
#define	MWROP_AND			3	/* src & dst (MASK)*/
#define	MWROP_CLEAR			4	/* 0*/
#define	MWROP_SET			5	/* ~0*/
#define	MWROP_EQUIV			6	/* ~(src ^ dst)*/
#define	MWROP_NOR			7	/* ~(src | dst)*/
#define	MWROP_NAND			8	/* ~(src & dst)*/
#define	MWROP_INVERT		9	/* ~dst*/
#define	MWROP_COPYINVERTED	10	/* ~src*/
#define	MWROP_ORINVERTED	11	/* ~src | dst*/
#define	MWROP_ANDINVERTED	12	/* ~src & dst (SUBTRACT)*/
#define MWROP_ORREVERSE		13	/* src | ~dst*/
#define	MWROP_ANDREVERSE	14	/* src & ~dst*/
#define	MWROP_NOOP			15	/* dst*/
#define	MWROP_XOR_FGBG		16	/* src ^ background ^ dst (Java XOR mode)*/
#define MWROP_SIMPLE_MAX 	16	/* last non-compositing rop*/

/* Porter-Duff compositing operations.  Only SRC, CLEAR and SRC_OVER are commonly used*/
#define	MWROP_SRC			MWROP_COPY
#define	MWROP_DST			MWROP_NOOP
#define	MWROP_SRC_OVER		17	/* dst = alphablend src,dst*/
#define	MWROP_DST_OVER		18
#define	MWROP_SRC_IN		19
#define	MWROP_DST_IN		20
#define	MWROP_SRC_OUT		21
#define	MWROP_DST_OUT		22
#define	MWROP_SRC_ATOP		23
#define	MWROP_DST_ATOP		24
#define	MWROP_PORTERDUFF_XOR 25
#define MWROP_SRCTRANSCOPY	26	/* copy src -> dst except for transparent color in src*/
#define	MWROP_MAX			26	/* last non-blit rop*/

/* blit ROP modes in addtion to MWROP_xxx */
#define MWROP_BLENDCONSTANT		32	/* alpha blend src -> dst with constant alpha*/
#define MWROP_BLENDFGBG			33	/* alpha blend fg/bg color -> dst with src alpha channel*/
#define MWROP_USE_GC_MODE		255 /* use current GC mode for ROP.  Nano-X CopyArea only*/

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
#define MWPF_TRUECOLORRGB  4	/* pixel is packed 24 bits R/G/B RGB truecolor*/
#define MWPF_TRUECOLOR888  4	/* deprecated*/
#define MWPF_TRUECOLOR565  5	/* pixel is packed 16 bits 5/6/5 RGB truecolor*/
#define MWPF_TRUECOLOR555  6	/* pixel is packed 16 bits 5/5/5 RGB truecolor*/
#define MWPF_TRUECOLOR332  7	/* pixel is packed  8 bits 3/3/2 RGB truecolor*/
#define MWPF_TRUECOLORARGB 8	/* pixel is packed 32 bits A/R/G/B ARGB truecolor with alpha */
#define MWPF_TRUECOLOR8888 8	/* deprecated*/
#define MWPF_TRUECOLOR0888 8	/* deprecated*/
#define MWPF_TRUECOLOR233  9	/* pixel is packed  8 bits 2/3/3 BGR truecolor*/
#define MWPF_HWPIXELVAL   10	/* pseudo, no convert, pixels are in hw format*/
#define MWPF_TRUECOLORABGR 11	/* pixel is packed 32 bits A/B/G/R ABGR truecolor with alpha */
#define MWPF_TRUECOLOR1555 12   /* pixel is packed 16 bits 1/5/5/5 NDS truecolor */

/*
 * MWPIXELVALHW definition: changes based on target system
 * Set using -DMWPIXEL_FORMAT=MWPF_XXX
 *
 * For the Nano-X server, it is important to use the correct MWPF_* value
 * for the MWPIXEL_FORMAT macro in order to match the hardware,
 * while the Nano-X clients that includes this file can get away with
 * a default pixel format of 32-bit color as the client will either:
 *    1) Use the MWPF_PIXELVAL native format when calling GrReadArea, in
 *       which case we have to have enough spare room to hold 32-bit
 *       pixel values (hence the default MWPF_TRUECOLORARGB format), or
 *    2) Will use some other PF_* format, in which case the application
 *       is well aware of which pixel-format it uses and can avoid the
 *       device specific RGB2PIXEL and use RGB2PIXEL565 etc. instead,
 *       and specifiy the pixel format as MWPF_TRUECOLOR565 etc. when
 *       calling the GrArea function(s).
 */

typedef uint32_t MWPIXELVALHW;	/* correct for MWPF_TRUECOLORARGB*/

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
typedef uint32_t		MWPIXELVAL;	/* pixel value parameter type, not for packing*/
typedef unsigned short	MWIMAGEBITS;/* bitmap image unit size*/
typedef uint32_t		MWTIMEOUT;	/* timeout value */
typedef uint32_t		MWTEXTFLAGS;/* MWTF_ text flag*/

/* max char height/width must be >= 16 and a multiple of sizeof(MWIMAGEBITS)*/
#define MAX_CHAR_HEIGHT	128			/* maximum text bitmap height*/
#define MAX_CHAR_WIDTH	128			/* maximum text bitmap width*/
#define MIN_MWCOORD ((MWCOORD) (1 << ((sizeof(MWCOORD)*8)-1))) /* min coordinate value*/
#define MAX_MWCOORD ((MWCOORD) ~(MIN_MWCOORD))	/* max coordinate value*/

#define TRUE			1
#define FALSE			0

int	MWMIN(int a,int b);
int	MWMAX(int a,int b);
int MWABS(int x);
int MWIMAGE_SIZE(int width, int height);

/* MWIMAGEBITS macros*/
#define	MWIMAGE_BITSPERIMAGE	16
#define	MWIMAGE_FIRSTBIT		0x8000
//#define MWIMAGE_WORDS(x)	(((x)+15)/16)
//#define MWIMAGE_BYTES(x)	(MWIMAGE_WORDS(x)*sizeof(MWIMAGEBITS))
/* size of image in words*/
//#define	MWIMAGE_SIZE(width, height)		((height) * (((width) + MWIMAGE_BITSPERIMAGE - 1) / MWIMAGE_BITSPERIMAGE))
//#define	MWIMAGE_BITVALUE(n)	((MWIMAGEBITS) (((MWIMAGEBITS) 1) << (n)))
//#define	MWIMAGE_NEXTBIT(m)	((MWIMAGEBITS) ((m) >> 1))
//#define	MWIMAGE_TESTBIT(m)	((m) & MWIMAGE_FIRSTBIT)  /* use with shiftbit*/
//#define	MWIMAGE_SHIFTBIT(m)	((MWIMAGEBITS) ((m) << 1))  /* for testbit*/

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
	int		data_format;/* MWIF_ image data format*/
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
	uint32_t amask;		/* alpha mask bits in pixel*/
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
	unsigned int pitch;	/* bytes per scan line for window (=fb pitch)*/
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

/* In-core color palette structure*/
typedef struct {
	MWUCHAR	r;
	MWUCHAR	g;
	MWUCHAR	b;
	MWUCHAR _padding;
} MWPALENTRY;

/* GdConversionBlit parameter structure*/
typedef struct {
	int			op;				/* MWROP operation requested*/
	int			data_format;	/* MWIF_ image data format*/
	MWCOORD		width, height;	/* width and height for src and dest*/
	MWCOORD		dstx, dsty;		/* dest x, y*/
	MWCOORD		srcx, srcy;		/* source x, y*/
	unsigned int src_pitch;		/* source row length in bytes*/
	MWCOLORVAL	fg_colorval;	/* fg color, MWCOLORVAL 0xAARRGGBB format*/
	MWCOLORVAL	bg_colorval;
	uint32_t	fg_pixelval;	/* fg color, hw pixel format*/
	uint32_t	bg_pixelval;
	MWBOOL		usebg;			/* set =1 to draw background*/
	void *		data;			/* input image data GdConversionBlit*/

	/* these items filled in by GdConversionBlit*/
	void *		data_out;		/* output image from conversion blits subroutines*/
	unsigned int dst_pitch;		/* dest row length in bytes*/

	/* used by GdBlit and GdStretchBlit for GdCheckCursor and fallback blit*/
	PSD			srcpsd;			/* source psd for psd->psd blits*/

	/* used by frameblits only*/
	MWCOORD		src_xvirtres;	/* srcpsd->x/y virtres, used in frameblit for src coord rotation*/
	MWCOORD		src_yvirtres;

	/* used in stretch blits only*/
	int			src_x_step;		/* normal steps in source image*/
	int			src_y_step;
	int			src_x_step_one;	/* 1-unit steps in source image*/
	int			src_y_step_one;
	int			err_x_step;		/* 1-unit error steps in source image*/
	int			err_y_step;
	int			err_y;			/* source coordinate error tracking*/
	int			err_x;
	int			x_denominator;	/* denominator fraction*/
	int			y_denominator;

	/* used in palette conversions only*/
	MWPALENTRY *palette;		/* palette for image*/
	uint32_t	transcolor;		/* transparent color in image*/
} MWBLITPARMS, *PMWBLITPARMS;

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

/* image structure - if changed, convbmp.c and client.c::GrDrawImageBits needs updating*/
typedef struct {
	/* shared header with SCREENDEVICE*/
	int		flags;		/* PSF_IMAGEHDR*/
	int		width;		/* image width in pixels*/
	int		height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel*/
	int		data_format;/* MWIF_ image data format*/
	unsigned int pitch;	/* bytes per line*/
	MWUCHAR *imagebits;	/* image bits (dword padded)*/
	int		palsize;	/* palette size*/
	MWPALENTRY *palette;/* palette*/
	uint32_t transcolor;/* transparent color or MWNOCOLOR if none*/
	/* end of shared header*/
} MWIMAGEHDR, *PMWIMAGEHDR;

/* image information structure - returned by GdGetImageInfo*/
typedef struct {
	int		id;			/* image id*/
	int 	width;		/* image width in pixels*/
	int 	height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel (1, 4 or 8)*/
	int		data_format;/* MWIF image data format*/
	unsigned int pitch;	/* bytes per line*/
	uint32_t transcolor;/* transparent color or MWNOCOLOR if none*/
	int		palsize;	/* palette size*/
	MWPALENTRY 	palette[256];	/* palette*/
} MWIMAGEINFO, *PMWIMAGEINFO;

#define	MWMAX_CURSOR_SIZE	32		/* maximum cursor x and y size*/
#define	MWMAX_CURSOR_BUFLEN	128

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
MWCOLORVAL MWARGB(int a,int r,int g,int b);
MWCOLORVAL MWRGB(int r,int g,int b);

/* no color, used for transparency, should not be 0, -1 or any MWRGB color*/
#define MWNOCOLOR	0x01000000L			/* MWRGBA(1, 0, 0, 0)*/

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
/* single byte color macros for 24/32bpp*/

/* extract MWCOLORVAL (0xAABBGGRR) values*/
//#define REDVALUE(rgb)	((rgb) & 0xff)
//#define GREENVALUE(rgb) (((rgb) >> 8) & 0xff)
//#define BLUEVALUE(rgb)	(((rgb) >> 16) & 0xff)
//#define ALPHAVALUE(rgb)	(((rgb) >> 24) & 0xff)

/* MWIF_BGRA8888*/
#define RMASKBGRA	0x00ff0000
#define GMASKBGRA	0x0000ff00
#define BMASKBGRA	0x000000ff
#define AMASKBGRA	0xff000000

/*
 * Conversion from MWCOLORVAL to MWPIXELVAL
 */
/* create 32 bit 8/8/8/8 format pixel from ABGR colorval (0xAABBGGRR)*/
/* In this format, alpha is preserved. */
//#define COLOR2PIXEL8888(c)	\
	((((c) & 0xff) << 16) | ((c) & 0xff00ff00ul) | (((c) & 0xff0000) >> 16))

/*
 * Conversion from MWPIXELVAL to MWCOLORVAL
 */
/* create ABGR colorval (0xAABBGGRR) from 8/8/8/8 ARGB (0xAARRGGBB) format pixel*/
//#define PIXEL8888TOCOLORVAL(p)	\
	((((p) & 0xff0000ul) >> 16) | ((p) & 0xff00ff00ul) | (((p) & 0xffu) << 16) | 0xff000000ul)

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

/* Mouse button bits, compatible with Windows*/
#define MWBUTTON_L		  0x01		/* left button*/
#define MWBUTTON_R		  0x02		/* right button*/
#define MWBUTTON_M		  0x10		/* middle*/
#define MWBUTTON_SCROLLUP 0x20		/* wheel up*/
#define MWBUTTON_SCROLLDN 0x40		/* wheel down*/

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

#define MWKEY_LAST          MWKEY_END_NORMAL

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
