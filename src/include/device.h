#ifndef _DEVICE_H
#define _DEVICE_H
/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Engine-level Screen, Mouse and Keyboard device driver API's and types
 * 
 * Contents of this file are not for general export
 */
#include "mwtypes.h"				/* public export typedefs*/

/* Changeable limits and options*/
#define ALPHABLEND	1			/* =1 to include blending code*/
#define DYNAMICREGIONS	1			/* =1 to use MWCLIPREGIONS*/
#define HAVEFLOAT	1			/* =1 incl float, GdArcAngle*/
#define POLYREGIONS	1			/* =1 includes polygon regions*/
#define ANIMATEPALETTE	0			/* =1 animated palette test*/
#define FONTMAPPER	0			/* =1 for Morten's font mapper*/
#define USE_ALLOCA	1			/* alloca() is available */
#define EPRINTF		GdError			/* error output*/
#define DPRINTF		GdErrorNull		/* debug output*/

/* max char height/width must be >= 16 and a multiple of sizeof(MWIMAGEBITS)*/
#define MAX_CHAR_HEIGHT	128			/* maximum text bitmap height*/
#define MAX_CHAR_WIDTH	128			/* maximum text bitmap width*/
#define	MIN_MWCOORD	((MWCOORD) -32768)	/* minimum coordinate value */
#define	MAX_MWCOORD	((MWCOORD) 32767)	/* maximum coordinate value */
#define	MAX_CLIPRECTS 	200			/* max clip rects (obsolete)*/

typedef struct _mwscreendevice *PSD;

/* builtin C-based proportional/fixed font structure*/
typedef struct {
	char *		name;		/* font name*/
	int		maxwidth;	/* max width in pixels*/
	int		height;		/* height in pixels*/
	int		ascent;		/* ascent (baseline) height*/
	int		firstchar;	/* first character in bitmap*/
	int		size;		/* font size in characters*/
	MWIMAGEBITS *	bits;		/* 16-bit right-padded bitmap data*/
	unsigned short *offset;		/* 256 offsets into bitmap data*/
	unsigned char *	width;		/* 256 character widths or 0 if fixed*/
} MWCFONT, *PMWCFONT;

/* draw procs associated with fonts.  Strings are [re]packed using defencoding*/
typedef struct {
	int	encoding;	/* routines expect this encoding*/
	MWBOOL	(*GetFontInfo)(PMWFONT pfont, PMWFONTINFO pfontinfo);
	void 	(*GetTextSize)(PMWFONT pfont, const void *text, int cc,
			MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
	void	(*GetTextBits)(PMWFONT pfont, int ch, MWIMAGEBITS *retmap,
			MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
	void	(*DestroyFont)(PMWFONT pfont);
	void	(*DrawText)(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
			const void *str, int count, int flags);
	void    (*SetFontSize)(PMWFONT pfont, MWCOORD fontsize);
	void    (*SetFontRotation)(PMWFONT pfont, int tenthdegrees);
	void    (*SetFontAttr)(PMWFONT pfont, int setflags, int clrflags);
} MWFONTPROCS, *PMWFONTPROCS;

/* new multi-renderer font struct*/
typedef struct _mwfont {		/* common hdr for all font structures*/
	PMWFONTPROCS	fontprocs;	/* font-specific rendering routines*/
	MWCOORD		fontsize;	/* font height in pixels*/
	int		fontrotation;	/* font rotation*/
	int		fontattr;	/* font attributes: kerning/antialias*/
	/* font-specific rendering data here*/
} MWFONT;

/* fontattr flags*/
#define FS_FREETYPE      0x0800

/* builtin core font struct*/
typedef struct {
	PMWFONTPROCS	fontprocs;	/* common hdr*/
	MWCOORD		fontsize;
	int		fontrotation;
	int		fontattr;

	char *		name;		/* Microwindows font name*/
	PMWCFONT	cfont;		/* builtin font data*/
} MWCOREFONT, *PMWCOREFONT;

/* This structure is used to pass parameters into the low
 * level device driver functions.
 */
typedef struct {
	MWCOORD dstx, dsty, dstw, dsth, dst_linelen;
	MWCOORD srcx, srcy, src_linelen;
	void *pixels, *misc;
	MWPIXELVAL bg_color, fg_color;
	int gr_usebg;
} driver_gc_t;

/* Operations for the Blitter/Area functions */
#define PSDOP_COPY	0
#define PSDOP_COPYALL	1
#define PSDOP_COPYTRANS 2
#define PSDOP_ALPHAMAP	3
#define PSDOP_ALPHACOL	4
#define PSDOP_PIXMAP_COPYALL	5

/* common blitter parameter structure*/
typedef struct {
	PSD		dstpsd;		/* dst drawable*/
	MWCOORD		dstx, dsty;	/* dst x,y,w,h*/
	MWCOORD		dstw, dsth;
	MWCOORD		srcx, srcy;	/* src x,y*/
	MWCOORD		srcw, srch;	/* src w,h if stretchblit*/
	PSD		srcpsd;		/* src drawable*/
	unsigned long 	rop;		/* raster opcode*/
	PSD		alphachan;	/* alpha chan for MWROP_BLENDCHANNEL*/
	MWPIXELVAL	fgcolor;	/* fg/bg color for MWROP_BLENDFGBG*/
	MWPIXELVAL	bgcolor;
	MWPIXELVAL	transcolor;	/* trans color for MWROP_SRCTRANSCOPY*/
} MWBLITARGS, *PMWBLITARGS;

/* screen subdriver entry points: one required for each draw function*/
/* NOTE: currently used for fb driver only*/
typedef struct {
	int	 (*Init)(PSD psd);
	void 	 (*DrawPixel)(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c);
	MWPIXELVAL (*ReadPixel)(PSD psd, MWCOORD x, MWCOORD y);
	void 	 (*DrawHorzLine)(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y,
			MWPIXELVAL c);
	void	 (*DrawVertLine)(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2,
			MWPIXELVAL c);
	void	 (*FillRect)(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,
			MWCOORD y2,MWPIXELVAL c);
	void	 (*Blit)(PSD destpsd, MWCOORD destx, MWCOORD desty, MWCOORD w,
			MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
	void	 (*DrawArea)(PSD psd, driver_gc_t *gc, int op);
	void	 (*StretchBlit)(PSD destpsd, MWCOORD destx, MWCOORD desty,
			MWCOORD dstw, MWCOORD dsth, PSD srcpsd, MWCOORD srcx,
			MWCOORD srcy, MWCOORD srcw, MWCOORD srch, long op);
} SUBDRIVER, *PSUBDRIVER;

/*
 * Interface to Screen Device Driver
 * This structure is also allocated for memory (offscreen) drawing and blitting.
 */
typedef struct _mwscreendevice {
	MWCOORD	xres;		/* X screen res (real) */
	MWCOORD	yres;		/* Y screen res (real) */
	MWCOORD	xvirtres;	/* X drawing res (will be flipped in portrait mode) */
	MWCOORD	yvirtres;	/* Y drawing res (will be flipped in portrait mode) */
	int	planes;		/* # planes*/
	int	bpp;		/* # bpp*/
	int	linelen;	/* line length in bytes for bpp 1,2,4,8*/
				/* line length in pixels for bpp 16, 24, 32*/
	int	size;		/* size of memory allocated*/
	long	ncolors;	/* # screen colors*/
	int	pixtype;	/* format of pixel value*/
	int	flags;		/* device flags*/
	void *	addr;		/* address of memory allocated (memdc or fb)*/

	PSD	(*Open)(PSD psd);
	void	(*Close)(PSD psd);
	void	(*GetScreenInfo)(PSD psd,PMWSCREENINFO psi);
	void	(*SetPalette)(PSD psd,int first,int count,MWPALENTRY *pal);
	void	(*DrawPixel)(PSD psd,MWCOORD x,MWCOORD y,MWPIXELVAL c);
	MWPIXELVAL (*ReadPixel)(PSD psd,MWCOORD x,MWCOORD y);
	void	(*DrawHorzLine)(PSD psd,MWCOORD x1,MWCOORD x2,MWCOORD y,
			MWPIXELVAL c);
	void	(*DrawVertLine)(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2,
			MWPIXELVAL c);
	void	(*FillRect)(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
			MWPIXELVAL c);
	PMWCOREFONT builtin_fonts;

	/***void (*DrawText)(PSD psd,MWCOORD x,MWCOORD y,const MWUCHAR *str,
			int count, MWPIXELVAL fg, PMWFONT pfont);***/

	void	(*Blit)(PSD destpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,
			MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
	void	(*PreSelect)(PSD psd);
	void	(*DrawArea)(PSD psd, driver_gc_t *gc, int op);
	int	(*SetIOPermissions)(PSD psd);
	PSD	(*AllocateMemGC)(PSD psd);
	MWBOOL	(*MapMemGC)(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
			int linelen,int size,void *addr);
	void	(*FreeMemGC)(PSD mempsd);
	void	(*StretchBlit)(PSD destpsd,MWCOORD destx,MWCOORD desty,
			MWCOORD destw,MWCOORD desth,PSD srcpsd,MWCOORD srcx,
			MWCOORD srcy,MWCOORD srcw,MWCOORD srch,long op);
	void	(*SetPortrait)(PSD psd,int portraitmode);
	int	portrait;	 /* screen portrait mode*/
	PSUBDRIVER orgsubdriver; /* original subdriver for portrait modes*/
} SCREENDEVICE;

/* PSD flags*/
#define	PSF_SCREEN		0x0001	/* screen device*/
#define PSF_MEMORY		0x0002	/* memory device*/
#define PSF_HAVEBLIT		0x0004	/* have bitblit*/
#define PSF_HAVEOP_COPY		0x0008	/* psd->DrawArea can do area copy*/
#define PSF_ADDRMALLOC		0x0010	/* psd->addr was malloc'd*/
#define PSF_ADDRSHAREDMEM	0x0020	/* psd->addr is shared memory*/

/* Interface to Mouse Device Driver*/
typedef struct _mousedevice {
	int	(*Open)(struct _mousedevice *);
	void	(*Close)(void);
	int	(*GetButtonInfo)(void);
	void	(*GetDefaultAccel)(int *pscale,int *pthresh);
	int	(*Read)(MWCOORD *dx,MWCOORD *dy,MWCOORD *dz,int *bp);
	int	(*Poll)(void);		/* not required if have select()*/
} MOUSEDEVICE;

/* Interface to Keyboard Device Driver*/
typedef struct _kbddevice {
	int  (*Open)(struct _kbddevice *pkd);
	void (*Close)(void);
	void (*GetModifierInfo)(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
	int  (*Read)(MWKEY *buf,MWKEYMOD *modifiers,MWSCANCODE *scancode);
	int  (*Poll)(void);		/* not required if have select()*/
} KBDDEVICE;

/* Clip areas*/
#define CLIP_VISIBLE		0
#define CLIP_INVISIBLE		1
#define CLIP_PARTIAL		2

/* static clip rectangle: drawing allowed if point within rectangle*/
typedef struct {
	MWCOORD 	x;		/* x coordinate of top left corner */
	MWCOORD 	y;		/* y coordinate of top left corner */
	MWCOORD 	width;		/* width of rectangle */
	MWCOORD 	height;		/* height of rectangle */
} MWCLIPRECT;

#ifndef TRUE
#define TRUE			1
#endif
#ifndef FALSE
#define FALSE			0
#endif

#define	MWMIN(a,b)		((a) < (b) ? (a) : (b))
#define	MWMAX(a,b) 		((a) > (b) ? (a) : (b))

/* MWIMAGEBITS macros*/
#define	MWIMAGE_SIZE(width, height)  ((height) * (((width) + MWIMAGE_BITSPERIMAGE - 1) / MWIMAGE_BITSPERIMAGE))
#define MWIMAGE_WORDS(x)	(((x)+15)/16)
#define MWIMAGE_BYTES(x)	(((x)+7)/8)
#define	MWIMAGE_BITSPERIMAGE	(sizeof(MWIMAGEBITS) * 8)
#define	MWIMAGE_FIRSTBIT	((MWIMAGEBITS) 0x8000)
#define	MWIMAGE_NEXTBIT(m)	((MWIMAGEBITS) ((m) >> 1))
#define	MWIMAGE_TESTBIT(m)	((m) & MWIMAGE_FIRSTBIT)  /* use with shiftbit*/
#define	MWIMAGE_SHIFTBIT(m)	((MWIMAGEBITS) ((m) << 1))  /* for testbit*/

/* color and palette defines*/
#define RGBDEF(r,g,b)	{r, g, b}

#define GETPALENTRY(pal,index) ((unsigned long)(pal[index].r |\
				(pal[index].g << 8) | (pal[index].b << 16)))
/*#define GETPALENTRY(pal,index) ((*(unsigned long *)&pal[index])&0x00ffffff)*/

#define REDVALUE(rgb)	((rgb) & 0xff)
#define GREENVALUE(rgb) (((rgb) >> 8) & 0xff)
#define BLUEVALUE(rgb)	(((rgb) >> 16) & 0xff)

/* Truecolor color conversion and extraction macros*/
/*
 * Conversion from RGB to MWPIXELVAL
 */
/* create 24 bit 8/8/8 format pixel (0x00RRGGBB) from RGB triplet*/
#define RGB2PIXEL888(r,g,b)	\
	(((r) << 16) | ((g) << 8) | (b))

/* create 16 bit 5/6/5 format pixel from RGB triplet */
#define RGB2PIXEL565(r,g,b)	\
	((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

/* create 16 bit 5/5/5 format pixel from RGB triplet */
#define RGB2PIXEL555(r,g,b)	\
	((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3))

/* create 8 bit 3/3/2 format pixel from RGB triplet*/
#define RGB2PIXEL332(r,g,b)	\
	(((r) & 0xe0) | (((g) & 0xe0) >> 3) | (((b) & 0xc0) >> 6))

/*
 * Conversion from MWCOLORVAL to MWPIXELVAL
 */
/* create 24 bit 8/8/8 format pixel from RGB colorval (0x00BBGGRR)*/
#define COLOR2PIXEL888(c)	\
	((((c) & 0xff) << 16) | ((c) & 0xff00) | (((c) & 0xff0000) >> 16))

/* create 16 bit 5/6/5 format pixel from RGB colorval (0x00BBGGRR)*/
#define COLOR2PIXEL565(c)	\
	((((c) & 0xf8) << 8) | (((c) & 0xfc00) >> 5) | (((c) & 0xf80000) >> 19))

/* create 16 bit 5/5/5 format pixel from RGB colorval (0x00BBGGRR)*/
#define COLOR2PIXEL555(c)	\
	((((c) & 0xf8) << 7) | (((c) & 0xf800) >> 6) | (((c) & 0xf80000) >> 19))

/* create 8 bit 3/3/2 format pixel from RGB colorval (0x00BBGGRR)*/
#define COLOR2PIXEL332(c)	\
	(((c) & 0xe0) | (((c) & 0xe000) >> 11) | (((c) & 0xc00000) >> 22))

/*
 * Conversion from MWPIXELVAL to red, green or blue components
 */
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

/* return 3/3/2 bit r, g or b component of 8 bit pixelval*/
#define PIXEL332RED(pixelval)		(((pixelval) >> 5) & 0x07)
#define PIXEL332GREEN(pixelval)		(((pixelval) >> 2) & 0x07)
#define PIXEL332BLUE(pixelval)		((pixelval) & 0x03)

/*
 * Conversion from MWPIXELVAL to MWCOLORVAL
 */
/* create RGB colorval (0x00BBGGRR) from 8/8/8 format pixel*/
#define PIXEL888TOCOLORVAL(p)	\
	((((p) & 0xff0000) >> 16) | ((p) & 0xff00) | (((p) & 0xff) << 16))

/* create RGB colorval (0x00BBGGRR) from 5/6/5 format pixel*/
#define PIXEL565TOCOLORVAL(p)	\
	((((p) & 0xf800) >> 8) | (((p) & 0x07e0) << 5) | (((p) & 0x1f) << 19))

#define PIXEL555TOCOLORVAL(p)	\
	((((p) & 0x7c00) >> 7) | (((p) & 0x03e0) << 6) | (((p) & 0x1f) << 19))

/* create RGB colorval (0x00BBGGRR) from 3/3/2 format pixel*/
#define PIXEL332TOCOLORVAL(p)	\
	((((p) & 0xe0)) | (((p) & 0x18) << 11) | (((p) & 0x03) << 19))

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

/* Alpha blend two pixels using 8-bit alpha */
/* FIXME this will be quite a bit faster as an inlined function */
#define ALPHAPIXELRED(pixelvalsrc, pixelvaldest, alpha)	\
	(unsigned char)((((PIXEL2RED(pixelvalsrc) - PIXEL2RED(pixelvaldest))\
			  * alpha) >> 8) + PIXEL2RED(pixelvaldest))

#define ALPHAPIXELGREEN(pixelvalsrc, pixelvaldest, alpha)	\
	(unsigned char)((((PIXEL2GREEN(pixelvalsrc)-PIXEL2GREEN(pixelvaldest))\
			  * alpha) >> 8) + PIXEL2GREEN(pixelvaldest))

#define ALPHAPIXELBLUE(pixelvalsrc, pixelvaldest, alpha)	\
	(unsigned char)((((PIXEL2BLUE(pixelvalsrc) - PIXEL2BLUE(pixelvaldest))\
			  * alpha) >> 8) + PIXEL2BLUE(pixelvaldest))

#if 0000
/* colors assumed in first 16 palette entries*/
/* note: don't use palette indices if the palette may
 * be reloaded.  Use the RGB values instead.
 */
#define BLACK		PALINDEX(0)		/*   0,   0,   0*/
#define BLUE		PALINDEX(1)
#define GREEN		PALINDEX(2)
#define CYAN		PALINDEX(3)
#define RED		PALINDEX(4)
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
#define WHITE		PALINDEX(15)		/* 255, 255, 255*/
#endif

/* GdMakePaletteConversionTable bLoadType types*/
#define LOADPALETTE	1	/* load image palette into system palette*/
#define MERGEPALETTE	2	/* merge image palette into system palette*/

/* entry points*/

/* devdraw.c*/
PSD	GdOpenScreen(void);
void	GdCloseScreen(PSD psd);
int	GdSetPortraitMode(PSD psd, int portraitmode);
int	GdSetMode(int mode);
MWBOOL	GdSetUseBackground(MWBOOL flag);
MWPIXELVAL GdSetForeground(MWPIXELVAL fg);
MWPIXELVAL GdSetBackground(MWPIXELVAL bg);
void	GdResetPalette(void);
void	GdSetPalette(PSD psd,int first, int count, MWPALENTRY *palette);
int	GdGetPalette(PSD psd,int first, int count, MWPALENTRY *palette);
MWPIXELVAL GdFindColor(MWCOLORVAL c);
MWPIXELVAL GdFindNearestColor(MWPALENTRY *pal, int size, MWCOLORVAL cr);
int	GdCaptureScreen(char *path);
void	GdGetScreenInfo(PSD psd,PMWSCREENINFO psi);
void	GdPoint(PSD psd,MWCOORD x, MWCOORD y);
void	GdLine(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
		MWBOOL bDrawLastPoint);
void	GdRect(PSD psd,MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
void	GdFillRect(PSD psd,MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
void	GdBitmap(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,
		MWIMAGEBITS *imagebits);
MWBOOL	GdColorInPalette(MWCOLORVAL cr,MWPALENTRY *palette,int palsize);
void	GdMakePaletteConversionTable(PSD psd,MWPALENTRY *palette,int palsize,
		MWPIXELVAL *convtable,int fLoadType);
void	GdDrawImage(PSD psd,MWCOORD x, MWCOORD y, PMWIMAGEHDR pimage);
void	GdPoly(PSD psd,int count, MWPOINT *points);
void	GdFillPoly(PSD psd,int count, MWPOINT *points);
void	GdReadArea(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,
		MWPIXELVAL *pixels);
void	GdArea(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,
		void *pixels, int pixtype);
void	GdTranslateArea(MWCOORD width, MWCOORD height, void *in, int inpixtype,
		MWCOORD inpitch, void *out, int outpixtype, int outpitch);
void	GdCopyArea(PSD psd,MWCOORD srcx,MWCOORD srcy,MWCOORD width,
		MWCOORD height, MWCOORD destx, MWCOORD desty);
void	GdBlit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD width,
		MWCOORD height,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long rop);
void	GdStretchBlit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
		MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy,
		MWCOORD srcw, MWCOORD srch, long rop);
int	GdCalcMemGCAlloc(PSD psd, unsigned int width, unsigned int height,
		int planes, int bpp, int *size, int *linelen);
extern SCREENDEVICE scrdev;

/* devarc.c*/
/* requires float*/
void	GdArcAngle(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD rx, MWCOORD ry,
		MWCOORD angle1, MWCOORD angle2, int type);
/* integer only*/
void	GdArc(PSD psd, MWCOORD x0, MWCOORD y0, MWCOORD rx, MWCOORD ry,
		MWCOORD ax, MWCOORD ay, MWCOORD bx, MWCOORD by, int type);
void	GdEllipse(PSD psd,MWCOORD x, MWCOORD y, MWCOORD rx, MWCOORD ry,
		MWBOOL fill);

/* devfont.c*/
void	GdClearFontList(void);
int	GdAddFont(char *fndry, char *family, char *fontname, PMWLOGFONT lf,
		  unsigned int flags);
PMWFONT	GdSetFont(PMWFONT pfont);
PMWFONT GdCreateFont(PSD psd, const char *name, MWCOORD height,
		const PMWLOGFONT plogfont);
MWCOORD	GdSetFontSize(PMWFONT pfont, MWCOORD fontsize);
int	GdSetFontRotation(PMWFONT pfont, int tenthdegrees);
int	GdSetFontAttr(PMWFONT pfont, int setflags, int clrflags);
void	GdDestroyFont(PMWFONT pfont);
MWBOOL	GdGetFontInfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
int	GdConvertEncoding(const void *istr, int iflags, int cc, void *ostr,
		int oflags);
void	GdGetTextSize(PMWFONT pfont, const void *str, int cc, MWCOORD *pwidth,
		MWCOORD *pheight, MWCOORD *pbase, int flags);
int	GdGetTextSizeEx(PMWFONT pfont, const void *str, int cc,
		int nMaxExtent, int *lpnFit, int *alpDx, MWCOORD *pwidth,
		MWCOORD *pheight, MWCOORD *pbase, int flags);	
void	GdText(PSD psd,MWCOORD x,MWCOORD y,const void *str,int count,int flags);

/* devclip1.c*/
void 	GdSetClipRects(PSD psd,int count,MWCLIPRECT *table);
MWBOOL	GdClipPoint(PSD psd,MWCOORD x,MWCOORD y);
int	GdClipArea(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2);
extern MWCOORD clipminx, clipminy, clipmaxx, clipmaxy;

/* devclip2.c*/
void	GdSetClipRegion(PSD psd, MWCLIPREGION *reg);

/* devrgn.c - multi-rectangle region entry points*/
MWBOOL GdPtInRegion(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y);
int    GdRectInRegion(MWCLIPREGION *rgn, const MWRECT *rect);
MWBOOL GdEqualRegion(MWCLIPREGION *r1, MWCLIPREGION *r2);
MWBOOL GdEmptyRegion(MWCLIPREGION *rgn);
MWCLIPREGION *GdAllocRegion(void);
MWCLIPREGION *GdAllocRectRegion(MWCOORD left,MWCOORD top,MWCOORD right,MWCOORD bottom);
MWCLIPREGION *GdAllocRectRegionIndirect(MWRECT *prc);
void GdSetRectRegion(MWCLIPREGION *rgn, MWCOORD left, MWCOORD top,
		MWCOORD right, MWCOORD bottom);
void GdSetRectRegionIndirect(MWCLIPREGION *rgn, MWRECT *prc);
void GdDestroyRegion(MWCLIPREGION *rgn);
void GdOffsetRegion(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y);
int  GdGetRegionBox(MWCLIPREGION *rgn, MWRECT *prc);
void GdUnionRectWithRegion(const MWRECT *rect, MWCLIPREGION *rgn);
void GdSubtractRectFromRegion(const MWRECT *rect, MWCLIPREGION *rgn);
void GdCopyRegion(MWCLIPREGION *d, MWCLIPREGION *s);
void GdIntersectRegion(MWCLIPREGION *d, MWCLIPREGION *s1, MWCLIPREGION *s2);
void GdUnionRegion(MWCLIPREGION *d, MWCLIPREGION *s1, MWCLIPREGION *s2);
void GdSubtractRegion(MWCLIPREGION *d, MWCLIPREGION *s1, MWCLIPREGION *s2);
void GdXorRegion(MWCLIPREGION *d, MWCLIPREGION *s1, MWCLIPREGION *s2);

/* devrgn2.c*/
MWCLIPREGION *GdAllocPolygonRegion(MWPOINT *points, int count, int mode);
MWCLIPREGION *GdAllocPolyPolygonRegion(MWPOINT *points, int *count,
		int nbpolygons, int mode);

/* devmouse.c*/
int	GdOpenMouse(void);
void	GdCloseMouse(void);
void	GdGetButtonInfo(int *buttons);
void	GdRestrictMouse(MWCOORD newminx,MWCOORD newminy,MWCOORD newmaxx,
		MWCOORD newmaxy);
void	GdSetAccelMouse(int newthresh, int newscale);
void	GdMoveMouse(MWCOORD newx, MWCOORD newy);
int	GdReadMouse(MWCOORD *px, MWCOORD *py, int *pb);
void	GdMoveCursor(MWCOORD x, MWCOORD y);
MWBOOL	GdGetCursorPos(MWCOORD *px, MWCOORD *py);
void	GdSetCursor(PMWCURSOR pcursor);
int 	GdShowCursor(PSD psd);
int 	GdHideCursor(PSD psd);
void	GdCheckCursor(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2);
void 	GdFixCursor(PSD psd);
extern MOUSEDEVICE mousedev;

/* devkbd.c*/
int  	GdOpenKeyboard(void);
void 	GdCloseKeyboard(void);
void 	GdGetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
int  	GdReadKeyboard(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
extern KBDDEVICE kbddev;

/* devimage.c */
void	GdDrawImageFromFile(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width,
		MWCOORD height, char *path, int flags);
int	GdLoadImageFromFile(PSD psd, char *path, int flags);
void	GdDrawImageToFit(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width,
		MWCOORD height, int id);
void	GdFreeImage(int id);
MWBOOL	GdGetImageInfo(int id, PMWIMAGEINFO pii);
void	GdStretchImage(PMWIMAGEHDR src, MWCLIPRECT *srcrect, PMWIMAGEHDR dst,
		MWCLIPRECT *dstrect);

/* devlist.c*/
/* field offset*/
#define MWITEM_OFFSET(type, field)    ((long)&(((type *)0)->field))

void * 	GdItemAlloc(unsigned int size);
void	GdListAdd(PMWLISTHEAD pHead,PMWLIST pItem);
void	GdListInsert(PMWLISTHEAD pHead,PMWLIST pItem);
void	GdListRemove(PMWLISTHEAD pHead,PMWLIST pItem);
#define GdItemNew(type)	((type *)GdItemAlloc(sizeof(type)))
#define GdItemFree(ptr)	free((void *)ptr)

/* return base item address from list ptr*/
#define GdItemAddr(p,type,list)	((type *)((long)p - MWITEM_OFFSET(type,list)))

#if UNIX || DJGPP

#include <sys/time.h>

typedef void (*MWTIMERCB)(void *);

#define  MWTIMER_ONESHOT         0 
#define  MWTIMER_PERIODIC        1

typedef struct mw_timer MWTIMER;
struct mw_timer {
	struct timeval	timeout;
	MWTIMERCB	callback;
	void		*arg;
	MWTIMER		*next;
	MWTIMER		*prev;
    int         type;     /* MWTIMER_ONESHOT or MWTIMER_PERIODIC */
    MWTIMEOUT   period;
};

MWTIMER		*GdAddTimer(MWTIMEOUT timeout, MWTIMERCB callback, void *arg);
MWTIMER         *GdAddPeriodicTimer(MWTIMEOUT timeout, MWTIMERCB callback, void *arg);
void		GdDestroyTimer(MWTIMER *timer);
MWTIMER		*GdFindTimer(void *arg);
MWBOOL		GdGetNextTimeout(struct timeval *tv, MWTIMEOUT timeout);
MWBOOL		GdTimeout(void);

#endif

/* error.c*/
int	GdError(const char *format, ...);
int	GdErrorNull(const char *format, ...);  /* doesn't print msgs */

#ifdef USE_ALLOCA
/* alloca() is available, so use it for better performance */
#define ALLOCA(size)	alloca(size)
#define FREEA(pmem)
#else
/* no alloca(), so use malloc()/free() instead */
#define ALLOCA(size)	malloc(size)
#define FREEA(pmem)	free(pmem)
#endif

/* no assert() in MSDOS or ELKS...*/
#if MSDOS | ELKS
#undef assert
#define assert(x)
#endif

/* RTEMS requires rtems_main()*/
#if __rtems__
#define main	rtems_main
#endif

#if !_MINIX
#ifndef __rtems__
#define HAVESELECT	1	/* has select system call*/
#endif
#endif

#endif /*_DEVICE_H*/
