#ifndef _DEVICE_H
#define _DEVICE_H
/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2003, 2005, 2007, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 Koninklijke Philips Electronics
 *
 * Engine-level Screen, Mouse and Keyboard device driver API's and types
 *
 * Contents of this file are not for general export
 */
#define MWINCLUDECOLORS			/* bring in color conversion macros*/
#include "mwtypes.h"			/* public export typedefs*/
#include "mwsystem.h"

/* Changeable limits and options*/
#define UNIFORMPALETTE	1	/* =1 for 256 entry uniform palette (required for palette alpha blending)*/
#define POLYREGIONS		1		/* =1 includes polygon regions*/
#define ANIMATEPALETTE	0		/* =1 animated palette test*/
#define FASTJPEG		0		/* =1 for temp quick jpeg 8bpp*/
#define FT_MINAA_HEIGHT	0		/* min height for FT antialias with win32 plogfont*/
#define TRANSLATE_ESCAPE_SEQUENCES  1		/* =1 to parse fnkeys w/tty driver*/

/* the fontmapper is obsolete*/
#define FONTMAPPER	0			/* =1 for Morten's font mapper*/

/* the following defines are unset in Arch.rules based on ARCH= setting*/
#ifndef HAVE_SELECT
#define HAVE_SELECT		1		/* =1 has select system call*/
#endif

#ifndef HAVE_SIGNAL
#define HAVE_SIGNAL		1		/* =1 has signal system call*/
#endif

#ifndef HAVE_MMAP
#define HAVE_MMAP       1       /* =1 has mmap system call*/
#endif

/* control whether printf/fprintf required in server and demo programs and C library*/
/* if this is set to =0 in Arch.rules, fprintf/printf will be no-op in all demo programs,*/
/* and in the server GdError will be called, which calls write() if HAVE_FILEIO=Y in config*/
#ifndef HAVE_FPRINTF
#define HAVE_FPRINTF	1		/* =1 EPRINTF/DPRINTF uses fprintf/printf, otherwise GdError or no-op*/
#endif

#ifndef HAVE_FLOAT
#define HAVE_FLOAT		1		/* =1 incl float, GdArcAngle*/
#endif

/* the following enable Microwindows features, also unset in Arch.rules*/
#ifndef MW_FEATURE_IMAGES
#define MW_FEATURE_IMAGES 1		/* =1 to enable GdLoadImage/GdDrawImage etc*/
#endif

#ifndef MW_FEATURE_TIMERS
#define MW_FEATURE_TIMERS 1		/* =1 to include MWTIMER support */
#endif

#ifndef MW_FEATURE_TWO_KEYBOARDS
#define MW_FEATURE_TWO_KEYBOARDS 0	/* =1 to include multiple keybioard support */
#endif

#ifndef DYNAMICREGIONS
#define DYNAMICREGIONS	1		/* =1 to use MWCLIPREGIONS*/
#endif

/* determine compiler capability for handling EPRINTF/DPRINTF macros*/
#if (defined(GCC_VERSION) && (GCC_VERSION >= 2093)) || (defined(__GNUC__) && (((__GNUC__ >= 2) && (__GNUC_MINOR__ >= 95)) || (__GNUC__ > 2)))
#define HAVE_VARARG_MACROS	1
#else
#define HAVE_VARARG_MACROS	0
#endif

/* see if can use GCC compiler-only macro magic to save space */
#if HAVE_VARARG_MACROS && HAVE_FPRINTF
#include <stdio.h>    				/* For stderr */
#define EPRINTF(str, args...)   fprintf(stderr, str, ##args)  /* error output*/
#if DEBUG
#define DPRINTF(str, args...)   fprintf(stderr, str, ##args)  /* debug output*/
#else
#define DPRINTF(str, ...)			/* no debug output*/
#endif

#else	/* must call GdError*/

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

typedef void (*MWBLITFUNC)(PSD, PMWBLITPARMS);		/* proto for blitter functions*/

/* screen subdriver entry points: one required for each draw function*/
typedef struct {
	int	 	 (*Init)(PSD psd);
	void 	 (*DrawPixel)(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c);
	MWPIXELVAL (*ReadPixel)(PSD psd, MWCOORD x, MWCOORD y);
	void 	 (*DrawHorzLine)(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
	void	 (*DrawVertLine)(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
	void	 (*FillRect)(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
	/* fallback blit - used only for 1,2,4bpp drivers*/
	void	(*BlitFallback)(PSD destpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
							PSD srcpsd,MWCOORD srcx,MWCOORD srcy,int op);
	/* endian neutral hw pixel format blits*/
	MWBLITFUNC FrameBlit;
	MWBLITFUNC FrameStretchBlit;
	/* fast conversion blits for text and images*/
	MWBLITFUNC BlitCopyMaskMonoByteMSB;				/* ft non-alias*/
	MWBLITFUNC BlitCopyMaskMonoByteLSB;				/* t1 non-alias*/
	MWBLITFUNC BlitCopyMaskMonoWordMSB;				/* core/pcf non-alias*/
	MWBLITFUNC BlitBlendMaskAlphaByte;				/* ft2/t1 antialias*/
	MWBLITFUNC BlitCopyRGBA8888;					/* GdArea RGBA MWPF_RGB image copy*/
	MWBLITFUNC BlitSrcOverRGBA8888;					/* png RGBA image w/alpha*/
	MWBLITFUNC BlitCopyRGB888;						/* png RGB image no alpha*/
	MWBLITFUNC BlitStretchRGBA8888;					/* conversion stretch blit for RGBA src*/
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
	int	planes;			/* # planes*/
	int	bpp;			/* # bpp*/
	int	linelen;		/* line length in bytes for bpp 1,2,4,8*/
						/* line length in pixels for bpp 16, 18, 24, 32*/
	int	pitch;			/* row length in bytes*/
	int	size;			/* size of memory allocated*/
	int32_t	ncolors;	/* # screen colors*/
	int	pixtype;		/* format of pixel value*/
	int data_format;	/* MWIF_ image data format*/
	int	flags;			/* device flags*/
	void *	addr;		/* address of memory allocated (memdc or fb)*/

	/* driver entry points*/
	PMWCOREFONT builtin_fonts;
	PSD		(*Open)(PSD psd);
	void	(*Close)(PSD psd);
	void	(*SetPalette)(PSD psd,int first,int count,MWPALENTRY *pal);
	void	(*GetScreenInfo)(PSD psd,PMWSCREENINFO psi);
	PSD		(*AllocateMemGC)(PSD psd);
	MWBOOL	(*MapMemGC)(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
			int data_format,int linelen,int pitch,int size,void *addr);
	void	(*FreeMemGC)(PSD mempsd);
	void	(*SetPortrait)(PSD psd,int portraitmode);
	void	(*Update)(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
	void	(*PreSelect)(PSD psd);
	int	portrait;	 /* screen portrait mode*/
	PSUBDRIVER orgsubdriver; /* original subdriver for portrait modes*/
	PSUBDRIVER left_subdriver;
	PSUBDRIVER right_subdriver;
	PSUBDRIVER down_subdriver;
	/* SUBDRIVER functions*/
	void	(*DrawPixel)(PSD psd,MWCOORD x,MWCOORD y,MWPIXELVAL c);
	MWPIXELVAL (*ReadPixel)(PSD psd,MWCOORD x,MWCOORD y);
	void	(*DrawHorzLine)(PSD psd,MWCOORD x1,MWCOORD x2,MWCOORD y, MWPIXELVAL c);
	void	(*DrawVertLine)(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2, MWPIXELVAL c);
	void	(*FillRect)(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2, MWPIXELVAL c);
	/* fallback blit - used only for 1,2,4bpp drivers*/
	void	(*BlitFallback)(PSD destpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
							PSD srcpsd,MWCOORD srcx,MWCOORD srcy,int op);
	/* endian neutral hw pixel format blits*/
	MWBLITFUNC FrameBlit;
	MWBLITFUNC FrameStretchBlit;
	/* fast conversion blits for text and images*/
	MWBLITFUNC BlitCopyMaskMonoByteMSB;				/* ft non-alias*/
	MWBLITFUNC BlitCopyMaskMonoByteLSB;				/* t1 non-alias*/
	MWBLITFUNC BlitCopyMaskMonoWordMSB;				/* core/pcf non-alias*/
	MWBLITFUNC BlitBlendMaskAlphaByte;				/* ft2/t1 antialias*/
	MWBLITFUNC BlitCopyRGBA8888;					/* GdArea RGBA MWPF_RGB image copy*/
	MWBLITFUNC BlitSrcOverRGBA8888;					/* png RGBA image w/alpha*/
	MWBLITFUNC BlitCopyRGB888;						/* png RGB image no alpha*/
	MWBLITFUNC BlitStretchRGBA8888;					/* conversion stretch blit for RGBA src*/
} SCREENDEVICE;

/* PSD flags*/
#define	PSF_SCREEN			0x0001	/* screen device*/
#define PSF_MEMORY			0x0002	/* memory device*/
#define PSF_ADDRMALLOC		0x0010	/* psd->addr was malloc'd*/
#define PSF_ADDRSHAREDMEM	0x0020	/* psd->addr is shared memory*/

/* Interface to Mouse Device Driver*/
typedef struct _mousedevice {
	int		(*Open)(struct _mousedevice *);
	void	(*Close)(void);
	int		(*GetButtonInfo)(void);
	void	(*GetDefaultAccel)(int *pscale,int *pthresh);
	int		(*Read)(MWCOORD *dx,MWCOORD *dy,MWCOORD *dz,int *bp);
	int		(*Poll)(void);	/* not required if have select()*/
	int     flags;			/* raw, normal, transform flags*/
} MOUSEDEVICE;

#define MOUSE_NORMAL		0x0000	/* mouse in normal mode*/
#define MOUSE_RAW			0x0001	/* mouse in raw mode*/
#define MOUSE_TRANSFORM		0x0002	/* perform transform*/

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

#define	MAX_CLIPRECTS 	200			/* max clip rects (obsolete)*/

/* GdMakePaletteConversionTable bLoadType types*/
#define LOADPALETTE	1	/* load image palette into system palette*/
#define MERGEPALETTE	2	/* merge image palette into system palette*/

/* entry points*/

/* devdraw.c*/
PSD		GdOpenScreen(void);
void	GdCloseScreen(PSD psd);
int		GdSetPortraitMode(PSD psd, int portraitmode);
int		GdSetMode(int mode);
MWBOOL	GdSetUseBackground(MWBOOL flag);
MWPIXELVAL GdSetForegroundPixelVal(PSD psd, MWPIXELVAL fg);
MWPIXELVAL GdSetBackgroundPixelVal(PSD psd, MWPIXELVAL bg);
MWPIXELVAL GdSetForegroundColor(PSD psd, MWCOLORVAL fg);
MWPIXELVAL GdSetBackgroundColor(PSD psd, MWCOLORVAL bg);
void	GdResetPalette(void);
void	GdSetPalette(PSD psd,int first, int count, MWPALENTRY *palette);
int		GdGetPalette(PSD psd,int first, int count, MWPALENTRY *palette);
MWCOLORVAL GdGetColorRGB(PSD psd, MWPIXELVAL pixel);
MWPIXELVAL GdFindColor(PSD psd, MWCOLORVAL c);
MWPIXELVAL GdFindNearestColor(MWPALENTRY *pal, int size, MWCOLORVAL cr);
int		GdCaptureScreen(PSD psd, char *pathname);	/* debug only*/
void	GdPrintBitmap(PMWBLITPARMS gc, int SSZ);	/* debug only*/
void	GdGetScreenInfo(PSD psd,PMWSCREENINFO psi);
void	GdPoint(PSD psd,MWCOORD x, MWCOORD y);
void	GdLine(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2, MWBOOL bDrawLastPoint);
void	GdRect(PSD psd,MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
void	GdFillRect(PSD psd,MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
MWBOOL	GdColorInPalette(MWCOLORVAL cr,MWPALENTRY *palette,int palsize);
void	GdMakePaletteConversionTable(PSD psd,MWPALENTRY *palette,int palsize,
		MWPIXELVALHW *convtable,int fLoadType);
void	GdDrawImage(PSD psd,MWCOORD x, MWCOORD y, PMWIMAGEHDR pimage);
void	GdBitmap(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,const MWIMAGEBITS *imagebits);
void	GdBitmapByPoint(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,
			const MWIMAGEBITS *imagebits, int clipresult);
void	GdPoly(PSD psd,int count, MWPOINT *points);
void	GdFillPoly(PSD psd,int count, MWPOINT *points);
void	GdReadArea(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height,MWPIXELVALHW *pixels);
void	GdArea(PSD psd,MWCOORD x,MWCOORD y,MWCOORD width,MWCOORD height, void *pixels, int pixtype);
void	GdTranslateArea(MWCOORD width, MWCOORD height, void *in, int inpixtype,
			MWCOORD inpitch, void *out, int outpixtype, int outpitch);
void	drawpoint(PSD psd, MWCOORD x, MWCOORD y);
void	drawrow(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y);
extern SCREENDEVICE scrdev;
extern MWPIXELVAL gr_foreground;		/* current foreground color */
extern MWPIXELVAL gr_background;		/* current background color */
extern MWBOOL 	  gr_usebg;			/* TRUE if background drawn in pixmaps */
extern MWCOLORVAL gr_foreground_rgb;/* current fg color in 0xAARRGGBB format*/
extern MWCOLORVAL gr_background_rgb;

/* drivers/fb.c*/
int	GdCalcMemGCAlloc(PSD psd, unsigned int width, unsigned int height,
		int planes, int bpp, int *size, int *linelen, int *pitch);

/* devblit.c*/
MWBLITFUNC GdFindConvBlit(PSD psd, int data_format, int op);
void	GdConversionBlit(PSD psd, PMWBLITPARMS parms);
void	GdConvBlitInternal(PSD psd, PMWBLITPARMS gc, MWBLITFUNC convblit);
void	GdBlit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD width, MWCOORD height,
			PSD srcpsd,MWCOORD srcx,MWCOORD srcy,int rop);
void	GdStretchBlit(PSD dstpsd, MWCOORD dx1, MWCOORD dy1, MWCOORD dx2,
			MWCOORD dy2, PSD srcpsd, MWCOORD sx1, MWCOORD sy1, MWCOORD sx2, MWCOORD sy2, int rop);

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
int		GdAddFont(char *fndry, char *family, char *fontname, PMWLOGFONT lf, unsigned int flags);
PMWFONT GdCreateFont(PSD psd, const char *name, MWCOORD height, MWCOORD width,
			const PMWLOGFONT plogfont);
MWCOORD	GdSetFontSize(PMWFONT pfont, MWCOORD height, MWCOORD width);
void 	GdGetFontList(MWFONTLIST ***list, int *num);
void 	GdFreeFontList(MWFONTLIST ***list, int num);
int		GdSetFontRotation(PMWFONT pfont, int tenthdegrees);
int		GdSetFontAttr(PMWFONT pfont, int setflags, int clrflags);
void	GdDestroyFont(PMWFONT pfont);
MWBOOL	GdGetFontInfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
int		GdConvertEncoding(const void *istr, MWTEXTFLAGS iflags, int cc, void *ostr, MWTEXTFLAGS oflags);
void	GdGetTextSize(PMWFONT pfont, const void *str, int cc, MWCOORD *pwidth,
			MWCOORD *pheight, MWCOORD *pbase, MWTEXTFLAGS flags);
int		GdGetTextSizeEx(PMWFONT pfont, const void *str, int cc, int nMaxExtent,
			int *lpnFit, int *alpDx, MWCOORD *pwidth,
			MWCOORD *pheight, MWCOORD *pbase, MWTEXTFLAGS flags);	
void	GdText(PSD psd,PMWFONT pfont, MWCOORD x,MWCOORD y,const void *str,int count,MWTEXTFLAGS flags);
PMWFONT	GdCreateFontFromBuffer(PSD psd, const unsigned char *buffer,
			unsigned length, const char *format, MWCOORD height, MWCOORD width);
PMWFONT	GdDuplicateFont(PSD psd, PMWFONT psrcfont, MWCOORD height, MWCOORD width);


/* both devclip1.c and devclip2.c */
MWBOOL	GdClipPoint(PSD psd,MWCOORD x,MWCOORD y);
int		GdClipArea(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2);
extern MWCOORD clipminx, clipminy, clipmaxx, clipmaxy;

/* devclip1.c only*/
void 	GdSetClipRects(PSD psd,int count,MWCLIPRECT *table);

/* devclip2.c only*/
void	GdSetClipRegion(PSD psd, MWCLIPREGION *reg);
void	GdPrintClipRects(PMWBLITPARMS gc);

/* devrgn.c - multi-rectangle region entry points*/
MWBOOL GdPtInRegion(MWCLIPREGION *rgn, MWCOORD x, MWCOORD y);
int    GdRectInRegion(MWCLIPREGION *rgn, const MWRECT *rect);
MWBOOL GdEqualRegion(MWCLIPREGION *r1, MWCLIPREGION *r2);
MWBOOL GdEmptyRegion(MWCLIPREGION *rgn);
MWCLIPREGION *GdAllocRegion(void);
MWCLIPREGION *GdAllocRectRegion(MWCOORD left,MWCOORD top,MWCOORD right,MWCOORD bottom);
MWCLIPREGION *GdAllocRectRegionIndirect(MWRECT *prc);
void GdSetRectRegion(MWCLIPREGION *rgn, MWCOORD left, MWCOORD top, MWCOORD right, MWCOORD bottom);
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
MWCLIPREGION *GdAllocBitmapRegion(MWIMAGEBITS *bitmap, MWCOORD width, MWCOORD height);

/* devrgn2.c*/
MWCLIPREGION *GdAllocPolygonRegion(MWPOINT *points, int count, int mode);
MWCLIPREGION *GdAllocPolyPolygonRegion(MWPOINT *points, int *count, int nbpolygons, int mode);

/* devmouse.c*/
int	GdOpenMouse(void);
void	GdCloseMouse(void);
void	GdGetButtonInfo(int *buttons);
void	GdRestrictMouse(MWCOORD newminx,MWCOORD newminy,MWCOORD newmaxx, MWCOORD newmaxy);
void	GdSetAccelMouse(int newthresh, int newscale);
void	GdMoveMouse(MWCOORD newx, MWCOORD newy);
int		GdReadMouse(MWCOORD *px, MWCOORD *py, int *pb);
void	GdMoveCursor(MWCOORD x, MWCOORD y);
MWBOOL	GdGetCursorPos(MWCOORD *px, MWCOORD *py);
void	GdSetCursor(PMWCURSOR pcursor);
int 	GdShowCursor(PSD psd);
int 	GdHideCursor(PSD psd);
void	GdCheckCursor(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2);
void 	GdFixCursor(PSD psd);
void    GdSetTransform(MWTRANSFORM *);

extern MOUSEDEVICE mousedev;

/* devkbd.c*/
int  	GdOpenKeyboard(void);
void 	GdCloseKeyboard(void);
void 	GdGetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
int  	GdReadKeyboard(MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
extern KBDDEVICE kbddev;

#ifdef MW_FEATURE_TWO_KEYBOARDS
int  	GdOpenKeyboard2(void);
extern KBDDEVICE kbddev2;
#endif

/* devimage.c */
#if MW_FEATURE_IMAGES
int		GdLoadImageFromBuffer(PSD psd, void *buffer, int size, int flags);
void	GdDrawImageFromBuffer(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width,
			MWCOORD height, void *buffer, int size, int flags);
void	GdDrawImageFromFile(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width,
			MWCOORD height, char *path, int flags);
void	GdDrawImagePartToFit(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
			MWCOORD sx, MWCOORD sy, MWCOORD swidth, MWCOORD sheight, int id);
int		GdLoadImageFromFile(PSD psd, char *path, int flags);
void	GdFreeImage(int id);
MWBOOL	GdGetImageInfo(int id, PMWIMAGEINFO pii);
void	GdStretchImage(PMWIMAGEHDR src, MWCLIPRECT *srcrect, PMWIMAGEHDR dst, MWCLIPRECT *dstrect);
void	GdComputeImagePitch(int bpp, int width, int *pitch, int *bytesperpixel);

/* Buffered input functions to replace stdio functions*/
typedef struct {  /* structure for reading images from buffer   */
	unsigned char *start;	/* The pointer to the beginning of the buffer */
	unsigned long offset;	/* The current offset within the buffer       */
	unsigned long size;	/* The total size of the buffer               */
} buffer_t;
void	GdImageBufferInit(buffer_t *buffer, void *startdata, int size);
void	GdImageBufferSeekTo(buffer_t *buffer, unsigned long offset);
int		GdImageBufferRead(buffer_t *buffer, void *dest, unsigned long size);
int		GdImageBufferGetChar(buffer_t *buffer);
char *	GdImageBufferGetString(buffer_t *buffer, char *dest, unsigned int size);
int		GdImageBufferEOF(buffer_t *buffer);

/* individual decoders*/
#ifdef HAVE_BMP_SUPPORT
int	GdDecodeBMP(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#ifdef HAVE_JPEG_SUPPORT
int	GdDecodeJPEG(buffer_t *src, PMWIMAGEHDR pimage, PSD psd, MWBOOL fast_grayscale);
#endif
#ifdef HAVE_PNG_SUPPORT
int	GdDecodePNG(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#ifdef HAVE_GIF_SUPPORT
int	GdDecodeGIF(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#ifdef HAVE_PNM_SUPPORT
int	GdDecodePNM(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#ifdef HAVE_XPM_SUPPORT
int	GdDecodeXPM(buffer_t *src, PMWIMAGEHDR pimage, PSD psd);
#endif
#ifdef HAVE_TIFF_SUPPORT
int	GdDecodeTIFF(char *path, PMWIMAGEHDR pimage);
#endif
#endif /* MW_FEATURE_IMAGES */

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

/* devstipple.c */
void	GdSetDash(uint32_t *mask, int *count);
void	GdSetStippleBitmap(MWIMAGEBITS *stipple, MWCOORD width, MWCOORD height);
void	GdSetTSOffset(int xoff, int yoff);
int	GdSetFillMode(int mode);
void	GdSetTilePixmap(PSD src, MWCOORD width, MWCOORD height);
void	ts_drawpoint(PSD psd, MWCOORD x, MWCOORD y);
void	ts_drawrow(PSD psd, MWCOORD x1, MWCOORD x2,  MWCOORD y);
void	ts_fillrect(PSD psd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h);
void	set_ts_origin(int x, int y);

#if MW_FEATURE_TIMERS
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

#endif /* MW_FEATURE_TIMERS */

/* error.c*/
int	GdError(const char *format, ...);
int	GdErrorNull(const char *format, ...);  /* doesn't print msgs */

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

#endif /*_DEVICE_H*/
