/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * T1lib Adobe type1 routines contributed by Vidar Hokstad
 * Freetype TrueType routines contributed by Martin Jolicoeur
 * Han Zi Ku routines contributed by Tanghao and Jauming
 *
 * Device-independent font and text drawing routines
 *
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string.h>

#include "device.h"
#if (UNIX | DOS_DJGPP)
#define strcmpi	strcasecmp
#endif

#if HAVE_T1LIB_SUPPORT
#include <t1lib.h>
#define T1LIB_USE_AA_HIGH

typedef struct {
	PMWFONTPROCS	fontprocs;	/* common hdr*/
	MWCOORD		fontsize;
	int		fontrotation;
	int		fontattr;		

	int		fontid;		/* t1lib stuff*/
} MWT1LIBFONT, *PMWT1LIBFONT;

static int  t1lib_init(PSD psd);
static PMWT1LIBFONT t1lib_createfont(const char *name, MWCOORD height,int attr);
static void t1lib_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, int flags);
static MWBOOL t1lib_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void t1lib_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
static void t1lib_destroyfont(PMWFONT pfont);

/* handling routines for MWT1LIBFONT*/
static MWFONTPROCS t1lib_procs = {
	MWTF_ASCII,			/* routines expect ascii*/
	t1lib_getfontinfo,
	t1lib_gettextsize,
	NULL,				/* gettextbits*/
	t1lib_destroyfont,
	t1lib_drawtext,
	NULL,				/* setfontsize*/
	NULL,				/* setfontrotation*/
	NULL,				/* setfontattr*/
};
#endif

#ifdef T1LIB_USE_AA_HIGH
typedef unsigned long OUTPIXELVAL;
#else
typedef MWPIXELVAL OUTPIXELVAL;
#endif

#if HAVE_FREETYPE_SUPPORT
#include <freetype/freetype.h>
#include <freetype/ftxkern.h>
#include <freetype/ftnameid.h>
#include <freetype/ftxcmap.h>
#include <freetype/ftxwidth.h>
#include <math.h>

/************/

typedef struct xTT_Glyph_Cache_ {

  TT_Error  error;
  TT_Glyph  glyph;
  TT_UShort index;
  TT_UShort flags;

  struct xTT_Glyph_Cache_ * l;
  struct xTT_Glyph_Cache_ * r;

} xTT_Glyph_Cache;

typedef struct xTT_Instance_Object_ {

  TT_Face     face;
  TT_Instance instance;

  xTT_Glyph_Cache * head;

} xTT_Instance_Object;

typedef struct xTT_Instance_ {

  xTT_Instance_Object * instance;

} xTT_Instance;

typedef struct xTT_Outline_Object_ {

  TT_Outline outline;
  TT_BBox    bbox;

} xTT_Outline_Object;

typedef struct xTT_Outline_ {

  xTT_Outline_Object * outline;

} xTT_Outline;

typedef struct xTT_Glyph_Object_ {

  TT_Glyph * glyph;

  xTT_Outline_Object outline;

} xTT_Glyph_Object;

typedef struct xTT_Glyph_ {

  xTT_Glyph_Object * glyph;

} xTT_Glyph;

TT_Error
xTT_Glyph_Cache_Find ( xTT_Instance instance,
		       xTT_Glyph glyph,
		       TT_UShort glyphIndex,
		       TT_UShort loadFlags )
{
  xTT_Glyph_Cache  * node;
  xTT_Glyph_Cache ** inode = &instance.instance->head;

  int miss = 0;

  while ( 1 )
  {
    if (*inode == 0) 
    {
      miss = 1;

      node = *inode = calloc(1,sizeof(**inode));

      if (node == 0)
	return TT_Err_Out_Of_Memory;

      node->error = TT_New_Glyph(instance.instance->face,&(node->glyph));

      if (node->error == 0)
	node->error = TT_Load_Glyph(instance.instance->instance,
				    node->glyph,
				    (node->index = glyphIndex),
				    (node->flags = loadFlags));

      if (node->error != 0)
	TT_Done_Glyph(node->glyph);
    } 
    else 
    {
      node = *inode;
    }

    if (glyphIndex < node->index)
      inode = &node->l;
    else if (glyphIndex > node->index)
      inode = &node->r;
    else if (loadFlags < node->flags)
      inode = &node->l;
    else if (loadFlags > node->flags)
      inode = &node->r;
    else
    {
      static int count [] = { 0, 0 };
      ++count[miss];
      printf("\r(%s | hit %d | miss %d)",__TIME__,count[0],count[1]);
      glyph.glyph->glyph = &node->glyph;
      return node->error;
    }
  }
}

void
xTT_Glyph_Cache_Free ( xTT_Glyph_Cache ** pnode )
{
  xTT_Glyph_Cache * node;

  if (pnode == 0)
    return;

  node = *pnode;

  if (node == 0)
    return;

  if (node->l)
    xTT_Glyph_Cache_Free(&node->l);

  if (node->r)
    xTT_Glyph_Cache_Free(&node->r);

  TT_Done_Glyph(node->glyph);
  free(node);

  *pnode = 0;
}

TT_Error
xTT_New_Instance ( TT_Face face,
		   xTT_Instance * instance )
{
  instance->instance = calloc(1,sizeof(*instance->instance));
  if (instance->instance == 0)
    return TT_Err_Out_Of_Memory;

  instance->instance->face = face;
  instance->instance->head = 0;

  return TT_New_Instance(face,&instance->instance->instance);
}

TT_Error
xTT_Done_Instance ( xTT_Instance instance )
{
  TT_Error error;

  xTT_Glyph_Cache_Free(&instance.instance->head);
  error = TT_Done_Instance(instance.instance->instance);

  free(instance.instance);

  return error;
}

TT_Error
xTT_Get_Instance_Metrics ( xTT_Instance instance,
			   TT_Instance_Metrics * imetrics )
{
  return TT_Get_Instance_Metrics(instance.instance->instance,imetrics);
}

TT_Error
xTT_Set_Instance_CharSize ( xTT_Instance instance,
			    TT_F26Dot6 charsize )
{
  xTT_Glyph_Cache_Free(&instance.instance->head);
  return TT_Set_Instance_CharSize(instance.instance->instance,charsize);
}

TT_Error
xTT_Set_Instance_PixelSizes ( xTT_Instance instance,
			      TT_UShort pixelWidth,
			      TT_UShort pixelHeight,
			      TT_F26Dot6 pointSize )
{
  xTT_Glyph_Cache_Free(&instance.instance->head);
  return TT_Set_Instance_PixelSizes(instance.instance->instance,
				    pixelWidth,
				    pixelHeight,
				    pointSize);
}

TT_Error
xTT_Set_Instance_Transform_Flags ( xTT_Instance instance, 
				   TT_Bool rotated,
				   TT_Bool stretched )
{
  xTT_Glyph_Cache_Free(&instance.instance->head);
  return TT_Set_Instance_Resolutions(instance.instance->instance,
				     rotated,
				     stretched);
}

TT_Error
xTT_Set_Instance_Resolutions ( xTT_Instance instance,
			       TT_UShort xResolution,
			       TT_UShort yResolution )
{
  xTT_Glyph_Cache_Free(&instance.instance->head);
  return TT_Set_Instance_Resolutions(instance.instance->instance,
				     xResolution,
				     yResolution);
}

TT_Error
xTT_New_Glyph ( TT_Face face,
		xTT_Glyph * glyph )
{
  glyph->glyph = calloc(1,sizeof(*glyph->glyph));
  if (glyph->glyph == 0)
    return TT_Err_Out_Of_Memory;
  return 0;
}

TT_Error
xTT_Done_Glyph ( xTT_Glyph glyph )
{
  free(glyph.glyph);
  return 0;
}

TT_Error
xTT_Load_Glyph ( xTT_Instance instance,
		 xTT_Glyph glyph,
		 TT_UShort glyphIndex,
		 TT_UShort loadFlags )
{
  TT_Error error;
  error = xTT_Glyph_Cache_Find(instance,glyph,glyphIndex,loadFlags);
  TT_Get_Glyph_Outline(*glyph.glyph->glyph,
		       &glyph.glyph->outline.outline);
  TT_Get_Outline_BBox(&glyph.glyph->outline.outline,
		      &glyph.glyph->outline.bbox);
  return error;
}

TT_Error
xTT_Get_Glyph_Metrics ( xTT_Glyph glyph,
			TT_Glyph_Metrics * metrics )
{
  return TT_Get_Glyph_Metrics(*glyph.glyph->glyph,metrics);
}

TT_Error
xTT_Get_Glyph_Outline ( xTT_Glyph glyph,
			xTT_Outline * outline )
{
  outline->outline = &glyph.glyph->outline;
  return 0;
}

TT_Error
xTT_Get_Outline_BBox ( xTT_Outline * outline,
		       TT_BBox * bbox )
{
  *bbox = outline->outline->bbox;
  return 0;
}

void
xTT_Transform_Outline ( xTT_Outline * outline,
			TT_Matrix * matrix )
{
  TT_Transform_Outline(&outline->outline->outline,matrix);
  TT_Get_Outline_BBox(&outline->outline->outline,
		      &outline->outline->bbox);
}

TT_Error
xTT_Get_Glyph_Bitmap ( xTT_Glyph glyph,
		       TT_Raster_Map * bitmap,
		       TT_F26Dot6 xOffset,
		       TT_F26Dot6 yOffset )
{
  return TT_Get_Glyph_Bitmap(*glyph.glyph->glyph,
			     bitmap,
			     xOffset,
			     yOffset);
}

TT_Error
xTT_Get_Glyph_Pixmap ( xTT_Glyph glyph,
		       TT_Raster_Map * pixmap,
		       TT_F26Dot6 xOffset,
		       TT_F26Dot6 yOffset )
{
  return TT_Get_Glyph_Pixmap(*glyph.glyph->glyph,
			     pixmap,
			     xOffset,
			     yOffset);
}

#ifndef xTT_ALIAS
#define xTT_ALIAS 1
#endif

#if xTT_ALIAS

#define TT_Instance                     xTT_Instance
#define TT_Outline                      xTT_Outline
#define TT_Glyph                        xTT_Glyph

#define TT_New_Instance                 xTT_New_Instance
#define TT_Done_Instance                xTT_Done_Instance

#define TT_Get_Instance_Metrics         xTT_Get_Instance_Metrics
#define TT_Set_Instance_CharSize        xTT_Set_Instance_CharSize
#define TT_Set_Instance_PixelSizes      xTT_Set_Instance_PixelSizes
#define TT_Set_Instance_Transform_Flags xTT_Set_Instance_Transform_Flags
#define TT_Set_Instance_Resolutions     xTT_Set_Instance_Resolutions

#define TT_New_Glyph                    xTT_New_Glyph
#define TT_Done_Glyph                   xTT_Done_Glyph

#define TT_Load_Glyph                   xTT_Load_Glyph
#define TT_Get_Glyph_Metrics            xTT_Get_Glyph_Metrics
#define TT_Get_Glyph_Outline            xTT_Get_Glyph_Outline
#define TT_Get_Glyph_Bitmap             xTT_Get_Glyph_Bitmap
#define TT_Get_Glyph_Pixmap             xTT_Get_Glyph_Pixmap

#define TT_Get_Outline_BBox             xTT_Get_Outline_BBox
#define TT_Transform_Outline            xTT_Transform_Outline

#endif

/************/

#ifndef FREETYPE_FONT_DIR
#define FREETYPE_FONT_DIR "/usr/local/microwin/fonts"
#endif

#if TT_FREETYPE_MAJOR != 1 | TT_FREETYPE_MINOR < 3
#error "You must link with freetype lib version 1.3.x +, and not freetype 2. \
Download it at http://www.freetype.org or http://microwindows.org"
#endif

#ifndef MWFREETYPEFONT_CACHE
#define MWFREETYPEFONT_CACHE 1
#endif

#if MWFREETYPEFONT_CACHE

#ifndef MWFREETYPEFONT_CACHEBITMAP
#define MWFREETYPEFONT_CACHEBITMAP 1
#endif

typedef struct mwfreetypefontcache {
  unsigned curchar;
  void * buffer;
#if MWFREETYPEFONT_CACHEBITMAP
  MWPIXELVAL fg;
  MWPIXELVAL bg;
  MWBOOL usebg;
  void * bitmap;
#endif
  struct mwfreetypefontcache * l;
  struct mwfreetypefontcache * r;
} MWFREETYPEFONTCACHE;

#endif

typedef struct {

	PMWFONTPROCS	fontprocs;	/* common hdr*/
	MWCOORD		fontsize;
	int		fontrotation;
	int		fontattr;		

	TT_Face 	face;		/* freetype stuff*/
	TT_Instance	instance;
	TT_CharMap 	char_map;
	TT_Kerning 	directory;
	TT_Matrix 	matrix;
	TT_Glyph 	glyph;
	MWBOOL 		can_kern;
	short 		last_glyph_code;
	short 		last_pen_pos;

#if MWFREETYPEFONT_CACHE
        MWFREETYPEFONTCACHE * glyph_cache;
#endif

} MWFREETYPEFONT, *PMWFREETYPEFONT;

static int  freetype_init(PSD psd);
static PMWFREETYPEFONT freetype_createfont(const char *name, MWCOORD height,
		int attr);
static MWBOOL freetype_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void freetype_gettextsize(PMWFONT pfont, const void *text,
		int cc, MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
static void freetype_destroyfont(PMWFONT pfont);
static void freetype_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, int flags);
static void freetype_setfontsize(PMWFONT pfont, MWCOORD fontsize);
static void freetype_setfontrotation(PMWFONT pfont, int tenthdegrees);
		
/* handling routines for MWFREETYPEFONT*/
static MWFONTPROCS freetype_procs = {
	MWTF_UC16,			/* routines expect unicode 16*/
	freetype_getfontinfo,
	freetype_gettextsize,
	NULL,				/* gettextbits*/
	freetype_destroyfont,
	freetype_drawtext,
	freetype_setfontsize,
	freetype_setfontrotation,
	NULL,				/* setfontattr*/
};

static TT_Engine 	engine;		/* THE ONLY freetype engine */
#endif /* HAVE_FREETYPE_SUPPORT*/

#if HAVE_HZK_SUPPORT
/*
 * 12x12 and 16x16 ascii and chinese fonts
 * Big5 and GB2312 encodings supported
 */
#define MAX_PATH	256
typedef struct {
	int	width;
	int	height;
	int	size;
	unsigned long use_count;
	char *	pFont;
	char	file[MAX_PATH + 1];
} HZKFONT;

static int use_big5=1;
static HZKFONT CFont[2];	/* font cache*/
static HZKFONT AFont[2];	/* font cache*/

/*jmt: moved inside MWHZKFONT*/
static int afont_width = 8;
static int cfont_width = 16;
static int font_height = 16;
static char *afont_address;
static char *cfont_address;

typedef struct {
	PMWFONTPROCS	fontprocs;	/* common hdr*/
	MWCOORD		fontsize;
	int		fontrotation;
	int		fontattr;		

	HZKFONT 	CFont;		/* hzkfont stuff */
	HZKFONT 	AFont;
	int 		afont_width;
	int 		cfont_width;
	int 		font_height;
	char 		*afont_address;
	char 		*cfont_address;
} MWHZKFONT, *PMWHZKFONT;

static int  hzk_init(PSD psd);
static PMWHZKFONT hzk_createfont(const char *name, MWCOORD height,int fontattr);
static MWBOOL hzk_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void hzk_gettextsize(PMWFONT pfont, const void *text,
		int cc, MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
/*static void hzk_gettextbits(PMWFONT pfont, int ch, IMAGEBITS *retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);*/
static void hzk_destroyfont(PMWFONT pfont);
static void hzk_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, int flags);
static void hzk_setfontsize(PMWFONT pfont, MWCOORD fontsize);
/*static void hzk_setfontrotation(PMWFONT pfont, int tenthdegrees);*/
		
/* handling routines for MWHZKFONT*/
static MWFONTPROCS hzk_procs = {
	MWTF_ASCII,			/* routines expect ASCII*/
	hzk_getfontinfo,
	hzk_gettextsize,
	NULL,				/* hzk_gettextbits*/
	hzk_destroyfont,
	hzk_drawtext,
	hzk_setfontsize,
	NULL, 				/* setfontrotation*/
	NULL,				/* setfontattr*/
};

static int
UC16_to_GB(const unsigned char *uc16, int cc, unsigned char *ascii);
#endif /* HAVE_HZK_SUPPORT*/

static PMWFONT	gr_pfont;            	/* current font*/

/* temp extern decls*/
extern MWPIXELVAL gr_foreground;
extern MWPIXELVAL gr_background;
extern MWBOOL gr_usebg;

static int  utf8_to_utf16(const unsigned char *utf8, int cc,
		unsigned short *unicode16);

#if FONTMAPPER
/* entry point for font selection*/
int select_font(const PMWLOGFONT plogfont, char *physname);
#endif

/*
 * Set the font for future calls.
 */
PMWFONT
GdSetFont(PMWFONT pfont)
{
	PMWFONT	oldfont = gr_pfont;
	gr_pfont = pfont;
	return oldfont;
}

/*
 * Select a font, based on various parameters.
 * If plogfont is specified, name and height parms are ignored
 * and instead used from MWLOGFONT.
 * 
 * If height is 0, return builtin font from passed name.
 * Otherwise find builtin font best match based on height.
 */
PMWFONT
GdCreateFont(PSD psd, const char *name, MWCOORD height,
	const PMWLOGFONT plogfont)
{
	int 		i;
	int		fontht;
	int		fontno;
 	int		fontclass;
	int		fontattr = 0;
	PMWFONT		pfont;
	PMWCOREFONT	pf = psd->builtin_fonts;
	MWFONTINFO	fontinfo;
	MWSCREENINFO 	scrinfo;
 	char		fontname[128];

	GdGetScreenInfo(psd, &scrinfo);

	/* if plogfont not specified, use name and height*/
	if (!plogfont) {
		if (!name)
			name = MWFONT_SYSTEM_VAR;
		strcpy(fontname, name);
		fontclass = MWLF_CLASS_ANY;
	} else {
#if FONTMAPPER
		/* otherwise, use MWLOGFONT name and height*/
 		fontclass = select_font(plogfont, fontname);
#else
		if (!name)
			name = MWFONT_SYSTEM_VAR;
		strcpy(fontname, name);
		fontclass = MWLF_CLASS_ANY;
#endif
		height = plogfont->lfHeight;
		if (plogfont->lfUnderline)
			fontattr = MWTF_UNDERLINE;
	}
	height = abs(height);
 
 	if (!fontclass)
 		goto first;
 
	/* use builtin screen fonts, FONT_xxx, if height is 0 */
 	if (height == 0 || fontclass == MWLF_CLASS_ANY ||
	    fontclass == MWLF_CLASS_BUILTIN) {
  		for(i = 0; i < scrinfo.fonts; ++i) {
 			if(!strcmpi(pf[i].name, fontname)) {
  				pf[i].fontsize = pf[i].cfont->height;
				pf[i].fontattr = fontattr;
  				return (PMWFONT)&pf[i];
  			}
  		}
 
		/* return first builtin font*/
		if (height == 0 || fontclass == MWLF_CLASS_BUILTIN)
			goto first;
  	}

#if HAVE_HZK_SUPPORT
        /* Make sure the library is initialized */
	if (hzk_init(psd)) {
		pfont = (PMWFONT)hzk_createfont(name, height, fontattr);
		if(pfont)		
			return pfont;
		printf("hzk_createfont: %s not found\n", name);
	}
#endif

#if HAVE_FREETYPE_SUPPORT
 	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_FREETYPE) {
		if (freetype_init(psd)) {
		/* auto antialias for height > 14 for kaffe*/
			if (plogfont && plogfont->lfHeight > 14 &&
				plogfont->lfQuality)
					fontattr |= MWTF_ANTIALIAS;

			pfont = (PMWFONT)freetype_createfont(fontname, height,
					fontattr);
			if(pfont) {
				/* temp kaffe kluge*/
				pfont->fontattr |= FS_FREETYPE;
				return pfont;
			}
 			DPRINTF("freetype_createfont: %s,%d not found\n",
				fontname, height);
		}
  	}
#endif

#if HAVE_T1LIB_SUPPORT
	if (fontclass == MWLF_CLASS_ANY || fontclass == MWLF_CLASS_T1LIB) {
		if (t1lib_init(psd)) {
			pfont = (PMWFONT)t1lib_createfont(fontname, height,
					fontattr);
			if(pfont)
				return pfont;
			DPRINTF("t1lib_createfont: %s,%d not found\n",
				fontname, height);
		}
  	}
#endif

	/* find builtin font closest in height*/
	if(height != 0) {
		fontno = 0;
		height = abs(height);
		fontht = MAX_MWCOORD;
		for(i = 0; i < scrinfo.fonts; ++i) {
			pfont = (PMWFONT)&pf[i];
			GdGetFontInfo(pfont, &fontinfo);
			if(fontht > abs(height-fontinfo.height)) { 
				fontno = i;
				fontht = abs(height-fontinfo.height);
			}
		}
		pf[fontno].fontsize = pf[fontno].cfont->height;
		pf[fontno].fontattr = fontattr;
		return (PMWFONT)&pf[fontno];
	}

first:
	/* Return first builtin font*/
	pf->fontsize = pf->cfont->height;
	pf->fontattr = fontattr;
	return (PMWFONT)&pf[0];
}

/* Set the font size for the passed font*/
MWCOORD
GdSetFontSize(PMWFONT pfont, MWCOORD fontsize)
{
	MWCOORD	oldfontsize = pfont->fontsize;

	pfont->fontsize = fontsize;

	if (pfont->fontprocs->SetFontSize)
	    pfont->fontprocs->SetFontSize(pfont, fontsize);

	return oldfontsize;
}

/* Set the font rotation angle in tenths of degrees for the passed font*/
int
GdSetFontRotation(PMWFONT pfont, int tenthdegrees)
{
	MWCOORD	oldrotation = pfont->fontrotation;

	pfont->fontrotation = tenthdegrees;

	if (pfont->fontprocs->SetFontRotation)
	    pfont->fontprocs->SetFontRotation(pfont, tenthdegrees);
	
	return oldrotation;
}

/*
 * Set/reset font attributes (MWTF_KERNING, MWTF_ANTIALIAS)
 * for the passed font.
 */
int
GdSetFontAttr(PMWFONT pfont, int setflags, int clrflags)
{
	MWCOORD	oldattr = pfont->fontattr;

	pfont->fontattr &= ~clrflags;
	pfont->fontattr |= setflags;

	if (pfont->fontprocs->SetFontAttr)
	    pfont->fontprocs->SetFontAttr(pfont, setflags, clrflags);
	
	return oldattr;
}

/* Unload and deallocate font*/
void
GdDestroyFont(PMWFONT pfont)
{
	if (pfont->fontprocs->DestroyFont)
		pfont->fontprocs->DestroyFont(pfont);
}

/* Return information about a specified font*/
MWBOOL
GdGetFontInfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	if(!pfont || !pfont->fontprocs->GetFontInfo)
		return FALSE;

	return pfont->fontprocs->GetFontInfo(pfont, pfontinfo);
}

/*
 * Convert from one encoding to another
 * Input cc and returned cc is character count, not bytes
 * Return < 0 on error or can't translate
 */
int
GdConvertEncoding(const void *istr, int iflags, int cc, void *ostr, int oflags)
{
	const unsigned char 	*istr8;
	const unsigned short 	*istr16;
	const unsigned long	*istr32;
	unsigned char 		*ostr8;
	unsigned short 		*ostr16;
	unsigned long		*ostr32;
	unsigned int		ch;
	int			icc;
	unsigned short		buf16[512];

	iflags &= MWTF_PACKMASK;
	oflags &= MWTF_PACKMASK;

	/* allow -1 for len with ascii*/
	if(cc == -1 && (iflags == MWTF_ASCII))
		cc = strlen((char *)istr);

	/* first check for utf8 input encoding*/
	if(iflags == MWTF_UTF8) {
		/* we've only got uc16 now so convert to uc16...*/
		cc = utf8_to_utf16((unsigned char *)istr, cc,
			oflags==MWTF_UC16?(unsigned short*) ostr: buf16);

		if(oflags == MWTF_UC16 || cc < 0)
			return cc;

		/* will decode again to requested format (probably ascii)*/
		iflags = MWTF_UC16;
		istr = buf16;
	}

#if HAVE_HZK_SUPPORT
	if(iflags == MWTF_UC16 && oflags == MWTF_ASCII) {
		/* only support uc16 convert to ascii now...*/
		cc = UC16_to_GB( istr, cc, ostr);
		return cc;
	}
#endif

	icc = cc;
	istr8 = istr;
	istr16 = istr;
	istr32 = istr;
	ostr8 = ostr;
	ostr16 = ostr;
	ostr32 = ostr;

	/* Convert between formats.  Note that there's no error
	 * checking here yet.
	 */
	while(--icc >= 0) {
		switch(iflags) {
		default:
			ch = *istr8++;
			break;
		case MWTF_UC16:
			ch = *istr16++;
			break;
		case MWTF_UC32:
			ch = *istr32++;
		}
		switch(oflags) {
		default:
			*ostr8++ = (unsigned char)ch;
			break;
		case MWTF_UC16:
			*ostr16++ = (unsigned short)ch;
			break;
		case MWTF_UC32:
			*ostr32++ = ch;
		}
	}
	return cc;
}

/* Get the width and height of passed text string in the passed font*/
void
GdGetTextSize(PMWFONT pfont, const void *str, int cc, MWCOORD *pwidth,
	MWCOORD *pheight, MWCOORD *pbase, int flags)
{
	const void *	text;
	unsigned long	buf[256];
	int		defencoding = pfont->fontprocs->encoding;

	/* convert encoding if required*/
	if((flags & MWTF_PACKMASK) != defencoding) {
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK;
		flags |= defencoding;
		text = buf;
	} else text = str;

	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

	if(cc <= 0 || !pfont->fontprocs->GetTextSize) {
		*pwidth = *pheight = *pbase = 0;
		return;
	}

	/* calc height and width of string*/
	pfont->fontprocs->GetTextSize(pfont, text, cc, pwidth, pheight, pbase);
}

/* Draw a text string at a specifed coordinates in the foreground color
 * (and possibly the background color), applying clipping if necessary.
 * The background color is only drawn if the gr_usebg flag is set.
 * Use the current font.
 */
void
GdText(PSD psd, MWCOORD x, MWCOORD y, const void *str, int cc, int flags)
{
	const void *	text;
	unsigned long	buf[256];
	int		defencoding = gr_pfont->fontprocs->encoding;

	/* convert encoding if required*/
	if((flags & MWTF_PACKMASK) != defencoding) {
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK;
		flags |= defencoding;
		text = buf;
	} else text = str;

	if(cc == -1 && (flags & MWTF_PACKMASK) == MWTF_ASCII)
		cc = strlen((char *)str);

	if(cc <= 0 || !gr_pfont->fontprocs->DrawText)
		return;

	/* draw text string*/
	gr_pfont->fontprocs->DrawText(gr_pfont, psd, x, y, text, cc, flags);
}

/*
 * Draw ascii text using COREFONT type font.
 */
void
corefont_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
	const void *text, int cc, int flags)
{
	const unsigned char *str = text;
	MWCOORD		width;			/* width of text area */
	MWCOORD 	height;			/* height of text area */
	MWCOORD		base;			/* baseline of text*/
	MWCOORD		startx, starty;
						/* bitmap for characters */
	MWIMAGEBITS bitmap[MAX_CHAR_HEIGHT*MAX_CHAR_WIDTH/MWIMAGE_BITSPERIMAGE];

	pfont->fontprocs->GetTextSize(pfont, str, cc, &width, &height, &base);
	
	if(flags & MWTF_BASELINE)
		y -= base;
	else if(flags & MWTF_BOTTOM)
		y -= (height - 1);
	startx = x;
	starty = y + base;

	switch (GdClipArea(psd, x, y, x + width - 1, y + height - 1)) {
	case CLIP_VISIBLE:
		/*
		 * For size considerations, there's no low-level text
		 * draw, so we've got to draw all text
		 * with per-point clipping for the time being
		if (gr_usebg)
			psd->FillRect(psd, x, y, x + width - 1, y + height - 1,
				gr_background);
		psd->DrawText(psd, x, y, str, cc, gr_foreground, pfont);
		GdFixCursor(psd);
		return;
		*/
		break;

	case CLIP_INVISIBLE:
		return;
	}

	/* Get the bitmap for each character individually, and then display
	 * them using clipping for each one.
	 */
	while (--cc >= 0 && x < psd->xvirtres) {
		unsigned int ch = *str++;
#if HAVE_BIG5_SUPPORT
		/* chinese big5 decoding*/
		if (ch >= 0xA1 && ch <= 0xF9 && cc >= 1 &&
			((*str >= 0x40 && *str <= 0x7E) ||
			 (*str >= 0xA1 && *str <= 0xFE)) ) {
				ch = (ch << 8) | *str++;
				--cc;
		}
#endif
#if HAVE_GB2312_SUPPORT
		/* chinese gb2312 decoding*/
		if (ch >= 0xA1 && ch < 0xF8 && cc >= 1 &&
			*str >= 0xA1 && *str < 0xFF) {
				ch = (ch << 8) | *str++;
				--cc;
		}
#endif
		pfont->fontprocs->GetTextBits(pfont, ch, bitmap, &width,
			&height, &base);

		/* note: change to bitmap*/
		GdBitmap(psd, x, y, width, height, bitmap);
		x += width;
	}

	if (pfont->fontattr & MWTF_UNDERLINE)
		GdLine(psd, startx, starty, x, starty, FALSE);

	GdFixCursor(psd);
}

#if HAVE_T1LIB_SUPPORT | HAVE_FREETYPE_SUPPORT
/*
 * Produce blend table from src and dst based on passed alpha table
 * Used because we don't quite yet have GdArea with alphablending,
 * so we pre-blend fg/bg colors for fade effect.
 */
static void
alphablend(PSD psd, OUTPIXELVAL *out, MWPIXELVAL src, MWPIXELVAL dst,
	unsigned char *alpha, int count)
{
	unsigned int	a, d;
	unsigned char	r, g, b;
	MWCOLORVAL	palsrc, paldst;
	extern MWPALENTRY gr_palette[256];

	while (--count >= 0) {
	    a = *alpha++;

#define BITS(pixel,shift,mask)	(((pixel)>>shift)&(mask))
	    if(a == 0)
		*out++ = dst;
	    else if(a == 255)
		*out++ = src;
	    else 
		switch(psd->pixtype) {
	        case MWPF_TRUECOLOR0888:
	        case MWPF_TRUECOLOR888:
		    d = BITS(dst, 16, 0xff);
		    r = (unsigned char)(((BITS(src, 16, 0xff) - d)*a)>>8) + d;
		    d = BITS(dst, 8, 0xff);
		    g = (unsigned char)(((BITS(src, 8, 0xff) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0xff);
		    b = (unsigned char)(((BITS(src, 0, 0xff) - d)*a)>>8) + d;
		    *out++ = (r << 16) | (g << 8) | b;
		    break;

	        case MWPF_TRUECOLOR565:
		    d = BITS(dst, 11, 0x1f);
		    r = (unsigned char)(((BITS(src, 11, 0x1f) - d)*a)>>8) + d;
		    d = BITS(dst, 5, 0x3f);
		    g = (unsigned char)(((BITS(src, 5, 0x3f) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0x1f);
		    b = (unsigned char)(((BITS(src, 0, 0x1f) - d)*a)>>8) + d;
		    *out++ = (r << 11) | (g << 5) | b;
		    break;

	        case MWPF_TRUECOLOR555:
		    d = BITS(dst, 10, 0x1f);
		    r = (unsigned char)(((BITS(src, 10, 0x1f) - d)*a)>>8) + d;
		    d = BITS(dst, 5, 0x1f);
		    g = (unsigned char)(((BITS(src, 5, 0x1f) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0x1f);
		    b = (unsigned char)(((BITS(src, 0, 0x1f) - d)*a)>>8) + d;
		    *out++ = (r << 10) | (g << 5) | b;
		    break;

	        case MWPF_TRUECOLOR332:
		    d = BITS(dst, 5, 0x07);
		    r = (unsigned char)(((BITS(src, 5, 0x07) - d)*a)>>8) + d;
		    d = BITS(dst, 2, 0x07);
		    g = (unsigned char)(((BITS(src, 2, 0x07) - d)*a)>>8) + d;
		    d = BITS(dst, 0, 0x03);
		    b = (unsigned char)(((BITS(src, 0, 0x03) - d)*a)>>8) + d;
		    *out++ = (r << 5) | (g << 2) | b;
		    break;

	        case MWPF_PALETTE:
		    /* reverse lookup palette entry for blend ;-)*/
		    palsrc = GETPALENTRY(gr_palette, src);
		    paldst = GETPALENTRY(gr_palette, dst);
		    d = REDVALUE(paldst);
		    r = (unsigned char)(((REDVALUE(palsrc) - d)*a)>>8) + d;
		    d = GREENVALUE(paldst);
		    g = (unsigned char)(((GREENVALUE(palsrc) - d)*a)>>8) + d;
		    d = BLUEVALUE(paldst);
		    b = (unsigned char)(((BLUEVALUE(palsrc) - d)*a)>>8) + d;
		    *out++ = GdFindNearestColor(gr_palette, (int)psd->ncolors,
				MWRGB(r, g, b));
		    break;
	  	}
	}
}
#endif /*HAVE_T1LIB_SUPPORT | HAVE_FREETYPE_SUPPORT*/

#if HAVE_T1LIB_SUPPORT
/* contributed by Vidar Hokstad*/

static int
t1lib_init(PSD psd)
{
	static int inited = 0;

	if (inited)
		return 1;

	T1_SetBitmapPad(8);
	if (!T1_InitLib(0))
		return 0;
#ifdef T1LIB_USE_AA_HIGH	 
	T1_AASetLevel(T1_AA_HIGH);
#else
	T1_AASetLevel(T1_AA_LOW);
#endif	 
#if 0
	/* kluge: this is required if 16bpp drawarea driver is used*/
	if(psd->bpp == 16)
		T1_AASetBitsPerPixel(16);
	else
#endif
		T1_AASetBitsPerPixel(sizeof(MWPIXELVAL)*8);

	inited = 1;
	return 1;
}

static PMWT1LIBFONT
t1lib_createfont(const char *name, MWCOORD height, int attr)
{
	PMWT1LIBFONT	pf;
	int		id;
	char *		p;
	char		buf[256];

	/* match name against t1 font id's from t1 font database*/
	for(id=0; id<T1_Get_no_fonts(); ++id) {
		strncpy(buf, T1_GetFontFileName(id), sizeof(buf));

		/* remove extension*/
		for(p=buf; *p; ++p) {
			if(*p == '.') {
				*p = 0;
				break;
			}
		}

		if(!strcmpi(name, buf)) {
			/* allocate font structure*/
			pf = (PMWT1LIBFONT)calloc(sizeof(MWT1LIBFONT), 1);
			if (!pf)
				return NULL;
			pf->fontprocs = &t1lib_procs;
			GdSetFontSize((PMWFONT)pf, height);
			GdSetFontRotation((PMWFONT)pf, 0);
			GdSetFontAttr((PMWFONT)pf, attr, 0);
			pf->fontid = id;
			return pf;
		}
	}
	return NULL;
}

/*
 * Draw ascii text string using T1LIB type font
 */
static void
t1lib_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
	const void *text, int cc, int flags)
{
	PMWT1LIBFONT	pf = (PMWT1LIBFONT)pfont;
	const unsigned char *str = text;
   	MWCOORD		width;			/* width of text area */
	MWCOORD 	height;			/* height of text area */
	MWCOORD		underliney;
        GLYPH * g; /* T1lib glyph structure. Memory handling by T1lib */
#ifdef T1LIB_USE_AA_HIGH   
        OUTPIXELVAL	gvals[17];

        /* Blending array for antialiasing. The steeper the values increase
	 * near the end, the sharper the characters look, but also more jagged
	 */
        static unsigned char blend[17] = {
	   0x00, 0x00, 0x04, 0x0c, 0x10, 0x14, 0x18, 0x20,
	   0x30, 0x38, 0x40, 0x50, 0x70, 0x80, 0xa0, 0xc0, 0xff
	};
#else   
        OUTPIXELVAL	gvals[5];
        static unsigned char blend[5] = { 0x00, 0x44, 0x88, 0xcc, 0xff };
#endif   

        /* Check if we should throw out some fonts */

        if (pf->fontattr&MWTF_ANTIALIAS) {
#ifdef T1LIB_USE_AA_HIGH      
	   alphablend(psd, gvals, gr_foreground, gr_background, blend, 17);
           T1_AAHSetGrayValues(gvals);
#else      
	   alphablend(psd, gvals, gr_foreground, gr_background, blend, 5);
           T1_AASetGrayValues(gvals[0],gvals[1],gvals[2],gvals[3],gvals[4]);
#endif
	   g = T1_AASetString(pf->fontid,(char *)str,cc,0,
		(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0,
		pf->fontsize * 1.0, 0);

	   if (g && g->bits) {
	      /*MWPIXELVAL save = gr_background;*/
	      width = g->metrics.rightSideBearing - g->metrics.leftSideBearing;
	      height = g->metrics.ascent - g->metrics.descent;

	      if(flags & MWTF_BASELINE)
		y -= g->metrics.ascent;
	      else if(flags & MWTF_BOTTOM)
		y -= (height - 1);
	      underliney = y + g->metrics.ascent;

	      /* FIXME: Looks damn ugly if usebg is false.
	       * Will be handled when using alphablending in GdArea...
	       */
	      /* clipping handled in GdArea*/
	      /*FIXME kluge for transparency*/
	      /*gr_background = gr_foreground + 1;*/
	      /*gr_usebg = 0;*/
	      GdArea(psd,x,y, width, height, g->bits, MWPF_PIXELVAL);
	      /*gr_background = save;*/

	      if (pf->fontattr & MWTF_UNDERLINE)
		   GdLine(psd, x, underliney, x+width, underliney, FALSE);

	   }
	} else {
	   /* Do non-aa drawing */
	   g = T1_SetString(pf->fontid,(char *)str,cc,0,
			(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0,
			pf->fontsize * 1.0, 0);

	   if (g && g->bits) {
	      unsigned char * b;
	      int xoff;
	      int maxy;
	      int xmod;
	      
	      /* I'm sure this sorry excuse for a bitmap rendering routine can
	       * be optimized quite a bit ;)
	       */
	      width = g->metrics.rightSideBearing - g->metrics.leftSideBearing;
	      height = g->metrics.ascent - g->metrics.descent;

	      if(flags & MWTF_BASELINE)
		y -= g->metrics.ascent;
	      else if(flags & MWTF_BOTTOM)
		y -= (height - 1);
	      underliney = y + g->metrics.ascent;
	      
	      b = g->bits;
	      maxy = y + height;
	      
/*	      if ((x + width) > psd->xvirtres) {
		 xmod = (x + width - psd->xvirtres + 7) >> 3;
		 width = width + x + width - psd->xvirtres;
	      } else xmod = 0;
*/
	      xmod = 0;
	      while (y < maxy) {
		 unsigned char data;
		 xoff = 0;
		 while (xoff < width ) {
		    if (!(xoff % 8)) {
		       data = *b;
		       b++;
		    }
		    
		    if (GdClipPoint(psd, x+xoff,y)) {
		       if (gr_usebg) {
	 		  psd->DrawPixel(psd,x+xoff,y,
			      data & (1 << (xoff % 8)) ?
			            gr_foreground : gr_background);
		       } else if (data & (1 << (xoff % 8))) {
			  psd->DrawPixel(psd,x+xoff,y, gr_foreground);
		       }
		    }
		    xoff++;
		 }
		 b += xmod;
		 y++;
	      }
	      if (pf->fontattr & MWTF_UNDERLINE)
		   GdLine(psd, x, underliney, x+xoff, underliney, FALSE);
	   }
        }

   if (g && g->bits) {
	   /* Save some memory */
	   free(g->bits);
           g->bits = 0; /* Make sure T1lib doesnt try to free it again */
   }

   GdFixCursor(psd);
}

static MWBOOL
t1lib_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	int	i;
	MWCOORD	width, height, baseline;

	/* FIXME, guess all sizes*/
	GdGetTextSize(pfont, "A", 1, &width, &height, &baseline, MWTF_ASCII);
	pfontinfo->height = height;
	pfontinfo->maxwidth = width;
	pfontinfo->baseline = 0;
	pfontinfo->firstchar = 32;
	pfontinfo->lastchar = 255;
	pfontinfo->fixed = TRUE;
	for(i=0; i<256; ++i)
		pfontinfo->widths[i] = width;
	return TRUE;
}

/* Get the width and height of passed text string in the current font*/
static void
t1lib_gettextsize(PMWFONT pfont, const void *text, int cc,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWT1LIBFONT		pf = (PMWT1LIBFONT)pfont;
	const unsigned char *	str = text;
	GLYPH *			g;

	g = T1_SetString(pf->fontid, (char *)str, cc, 0,
			(pf->fontattr&MWTF_KERNING)? T1_KERNING: 0, pf->fontsize * 1.0, 0);
	*pwidth = g->metrics.rightSideBearing - g->metrics.leftSideBearing;
	*pheight = g->metrics.ascent - g->metrics.descent;
	if(g && g->bits) {
		free(g->bits);
		g->bits = 0;
	}
#if 0
	BBox 			b;

	/* FIXME. Something is *VERY* wrong here */
	b = T1_GetStringBBox(pf->fontid, str, cc, 0, (pf->fontattr&MWTF_KERNING)?T1_KERNING:0);

	DPRINTF("b.urx = %d, b.llx = %d\n",b.urx, b.llx);
	DPRINTF("b.ury = %d, b.lly = %d\n",b.ury, b.lly);
	*pwidth = (b.urx - b.llx);
	*pheight = (b.lly - b.ury);
#endif
}

static void
t1lib_destroyfont(PMWFONT pfont)
{
	PMWT1LIBFONT	pf = (PMWT1LIBFONT)pfont;

	T1_DeleteAllSizes(pf->fontid);
	free(pf);
}

#endif /* HAVE_T1LIB_SUPPORT*/

#if HAVE_FREETYPE_SUPPORT
static OUTPIXELVAL gray_palette[5];

static int
freetype_init(PSD psd)
{
	static int inited = 0;

	if (inited)
		return 1;
	
	/* Init freetype library */
	if (TT_Init_FreeType (&engine) != TT_Err_Ok) {
		return 0;
	}

	/* Init kerning extension */
	if (TT_Init_Kerning_Extension (engine) != TT_Err_Ok)
		return 0;

	inited = 1;
	return 1;
}

static PMWFREETYPEFONT
freetype_createfont(const char *name, MWCOORD height, int attr)
{
	PMWFREETYPEFONT 	pf;
	unsigned short 		i, n;
	unsigned short 		platform, encoding;
	TT_Face_Properties 	properties;
	char *			p;
	char			fontname[128];

	/* check for pathname prefix*/
	if (strchr(name, '/') != NULL)
		strcpy(fontname, name);
	else {
		strcpy(fontname, FREETYPE_FONT_DIR);
		strcat(fontname, "/");
		strcat(fontname, name);
	}

	/* check for extension*/
	if ((p = strrchr(fontname, '.')) == NULL ||
	    strcmp(p, ".ttf") != 0) {
		strcat(fontname, ".ttf");
	}

	/* allocate font structure*/
	pf = (PMWFREETYPEFONT)calloc(sizeof(MWFREETYPEFONT), 1);
	if (!pf)
		return NULL;
	pf->fontprocs = &freetype_procs;

	/* Load face */
	if (TT_Open_Face (engine, fontname, &pf->face) != TT_Err_Ok)
		goto out;

	/* Load first kerning table */
	pf->can_kern = TRUE;
	if (TT_Load_Kerning_Table (pf->face, 0) != TT_Err_Ok)
		pf->can_kern = FALSE;
	else {
		if (TT_Get_Kerning_Directory (pf->face, &pf->directory)
		    != TT_Err_Ok)
			pf->can_kern = FALSE;
		else {
			/* Support only version 0 kerning table ... */
			if ((pf->directory.version != 0) ||
				(pf->directory.nTables <= 0) ||
				(pf->directory.tables->loaded != 1) ||
				(pf->directory.tables->version != 0) ||
				(pf->directory.tables->t.kern0.nPairs <= 0))
					pf->can_kern = FALSE;
		}
	}

	/* get face properties and allocate preload arrays */
	TT_Get_Face_Properties (pf->face, &properties);

#if 0
	/*
	 * Use header information for ascent and descent
	 * to compute scaled ascent/descent for current font height.
	 */
	h = properties.os2->sTypoAscender - properties.os2->sTypoDescender
		+ properties.os2->sTypoLineGap;
	ascent = properties.os2->sTypoAscender
		+ properties.os2->sTypoLineGap/2;
	pf->ascent = (ascent * height + h/2) / h;
	pf->descent = height - pf->ascent;
#endif
	/* Create a glyph container */
	if (TT_New_Glyph (pf->face, &pf->glyph) != TT_Err_Ok)
		goto out;

	/* create instance */
	if (TT_New_Instance (pf->face, &pf->instance) != TT_Err_Ok)
		goto out;

	/* Set the instance resolution */
	if (TT_Set_Instance_Resolutions (pf->instance, 96, 96) != TT_Err_Ok)
		goto out;

	/* Look for a Unicode charmap: Windows flavor of Apple flavor only */
	n = properties.num_CharMaps;

	for (i = 0; i < n; i++) {
		TT_Get_CharMap_ID (pf->face, i, &platform, &encoding);
		if (((platform == TT_PLATFORM_MICROSOFT) &&
			(encoding == TT_MS_ID_UNICODE_CS)) ||
				((platform == TT_PLATFORM_APPLE_UNICODE) &&
			 		(encoding == TT_APPLE_ID_DEFAULT)))
		{
			TT_Get_CharMap (pf->face, i, &pf->char_map);
			i = n + 1;
		}
	}
	if (i == n) {
		DPRINTF("freetype_createfont: no unicode map table\n");
		goto out;
	}
	
	GdSetFontSize((PMWFONT)pf, height);
	GdSetFontRotation((PMWFONT)pf, 0);
	GdSetFontAttr((PMWFONT)pf, attr, 0);

	return pf;

out:
	free(pf);
	return NULL;
}

static int
compute_kernval(PMWFREETYPEFONT pf, short current_glyph_code)
{
	int 		i = 0;
	int 		kernval;
	int 		nPairs = pf->directory.tables->t.kern0.nPairs;
	TT_Kern_0_Pair *pair = pf->directory.tables->t.kern0.pairs;

	if (pf->last_glyph_code != -1) {
		while ((pair->left != pf->last_glyph_code)
			&& (pair->right != current_glyph_code))
		{
			pair++;
			i++;
			if (i == nPairs)
			break;
		}

		if (i == nPairs)
			kernval = 0;
		else
			/* We round the value (hence the +32) */
			kernval = (pair->value + 32) & -64;
	} else
		kernval = 0;

	return kernval;
}

static TT_UShort
Get_Glyph_Width(PMWFREETYPEFONT pf, TT_UShort glyph_index)
{
	TT_Glyph_Metrics metrics;

    	if (TT_Load_Glyph ( pf->instance, pf->glyph,
		TT_Char_Index (pf->char_map,glyph_index),
		TTLOAD_SCALE_GLYPH|TTLOAD_HINT_GLYPH) != TT_Err_Ok)
    	{
		/* Try to load default glyph: index 0 */
		if (TT_Load_Glyph ( pf->instance, pf->glyph, 0,
			TTLOAD_SCALE_GLYPH|TTLOAD_HINT_GLYPH) != TT_Err_Ok)
		    return 0;
	}

	TT_Get_Glyph_Metrics (pf->glyph, &metrics);
	return((metrics.advance & 0xFFFFFFC0) >> 6);
}

static MWFREETYPEFONTCACHE *
find_cache_glyph ( PMWFREETYPEFONT pf, unsigned curchar )
{
  MWFREETYPEFONTCACHE ** inode = &pf->glyph_cache;

  while (1)
    {
      if (*inode == 0)
	{
	  *inode = calloc(sizeof(MWFREETYPEFONTCACHE),1);
	  (*inode)->curchar = curchar;
	}

      if (curchar < (*inode)->curchar)
	inode = &(*inode)->l;
      else if (curchar > (*inode)->curchar)
	inode = &(*inode)->r;
      else
	return *inode;
    }
}

static void
free_cache_glyph ( MWFREETYPEFONTCACHE * node )
{
  if (node->l != 0)
    free_cache_glyph(node->l);

  if (node->r != 0)
    free_cache_glyph(node->r);

  free(node);
}

/* Render a single glyph*/
static void
drawchar(PMWFREETYPEFONT pf, PSD psd, unsigned curchar, int x_offset,
	int y_offset)
{
	TT_F26Dot6 	xmin, ymin, xmax, ymax, x, y, z;
	unsigned char 	*src, *srcptr;
	MWPIXELVAL 	*dst, *dstptr;
	MWPIXELVAL 	*bitmap;
	int 		size, width, height;
	TT_Outline 	outline;
	TT_BBox 	bbox;
	TT_Raster_Map 	Raster;
	TT_Error 	error;
	/*MWPIXELVAL 	save;*/
        MWFREETYPEFONTCACHE * cache;

	/* we begin by grid-fitting the bounding box */
	TT_Get_Glyph_Outline (pf->glyph, &outline);
	TT_Get_Outline_BBox (&outline, &bbox);

	xmin = (bbox.xMin & -64) >> 6;
	ymin = (bbox.yMin & -64) >> 6;
	xmax = ((bbox.xMax + 63) & -64) >> 6;
	ymax = ((bbox.yMax + 63) & -64) >> 6;
	width = xmax - xmin;
	height = ymax - ymin;
	size = width * height;

	/* now re-allocate the raster bitmap */
	Raster.rows = height;
	Raster.width = width;

	if (pf->fontattr&MWTF_ANTIALIAS)
		Raster.cols = (Raster.width + 3) & -4;	/* pad to 32-bits */
	else
		Raster.cols = (Raster.width + 7) & -8;	/* pad to 64-bits ??? */

        cache = find_cache_glyph(pf,curchar);
        /* SDR: if cache is 0 here, something really really bad happened */

	Raster.flow = TT_Flow_Up;
	Raster.size = Raster.rows * Raster.cols;
	Raster.bitmap = cache->buffer;

	if (Raster.bitmap == 0)
	  {
	    Raster.bitmap = malloc (Raster.size);

	    memset (Raster.bitmap, 0, Raster.size);

	    /* now render the glyph in the small pixmap */

	    /* IMPORTANT NOTE: the offset parameters passed to the function     */
	    /* TT_Get_Glyph_Bitmap() must be integer pixel values, i.e.,        */
	    /* multiples of 64.  HINTING WILL BE RUINED IF THIS ISN'T THE CASE! */
	    /* This is why we _did_ grid-fit the bounding box, especially xmin  */
	    /* and ymin.                                                        */

	    if (!(pf->fontattr&MWTF_ANTIALIAS))
	      error = TT_Get_Glyph_Bitmap (pf->glyph, &Raster,
					   -xmin * 64, -ymin * 64);
	    else
	      error = TT_Get_Glyph_Pixmap (pf->glyph, &Raster,
					   -xmin * 64, -ymin * 64);

	    if (error) {
	      free (Raster.bitmap);
	      return;
	    }

            cache->buffer = Raster.bitmap;
	  }

#if MWFREETYPEFONT_CACHEBITMAP
	if ((memcmp(&gr_foreground,&cache->fg,sizeof(gr_foreground)) != 0) || 
	    (memcmp(&gr_background,&cache->bg,sizeof(gr_background)) != 0) || 
	    (gr_usebg != cache->usebg))
	  {
	    if (cache->bitmap)
	      {
		free(cache->bitmap);
		cache->bitmap = 0;
	      }
	  }

	bitmap = cache->bitmap;
#else
	bitmap = 0;
#endif

	if (bitmap == 0)
	  {
	    bitmap = malloc (size * sizeof (MWPIXELVAL));
	    memset (bitmap, 0, size * sizeof (MWPIXELVAL));

	    src = (char *) Raster.bitmap;
	    dst = bitmap + (size - width);

	    for (y = ymin; y < ymax; y++) 
	      {
		srcptr = src;
		dstptr = dst;

		for (x = xmin; x < xmax; x++) 
		  {
		    if (pf->fontattr&MWTF_ANTIALIAS)
		      *dstptr++ = gray_palette[(int) *srcptr];
		    else
		      {
		        for ( z=0;
			      z <= ((xmax-x-1) < 7 ? (xmax-x-1) : 7);
			      z++ )
			    *dstptr++ = ((*srcptr << z) & 0x80) ? gr_foreground : gr_background;
			x += 7;
		      }

		    srcptr++;
		  }

		src += Raster.cols;
		dst -= width;
	      }
#if MWFREETYPEFONT_CACHEBITMAP
	    cache->fg = gr_foreground;
	    cache->bg = gr_background;
	    cache->usebg = gr_usebg;
	    cache->bitmap = bitmap;
#endif
	  }

	/* Now draw the bitmap ... */
	GdArea(psd, x_offset + xmin, y_offset - (ymin + height), width, height,
		bitmap, MWPF_PIXELVAL);

#if !MWFREETYPEFONT_CACHEBITMAP
	free(bitmap);
#endif
}

/*
 * Draw unicode 16 text string using FREETYPE type font
 */
static void
freetype_drawtext(PMWFONT pfont, PSD psd, MWCOORD ax, MWCOORD ay,
	const void *text, int cc, int flags)
{
	PMWFREETYPEFONT	pf = (PMWFREETYPEFONT)pfont;
	const unsigned short *	str = text;
	TT_F26Dot6 	x = ax, y = ay;
	TT_Pos 		vec_x, vec_y;
	int 		i;
	TT_F26Dot6	startx, starty;
	TT_Outline 	outline;
	TT_UShort 	curchar;
	TT_Glyph_Metrics metrics;
	TT_Face_Properties properties;
	TT_Instance_Metrics imetrics;
	TT_F26Dot6 ascent, descent;
	static unsigned char blend[5] = { 0x00, 0x44, 0x88, 0xcc, 0xff };
	static unsigned char virtual_palette[5] = { 0, 1, 2, 3, 4 };

	pf->last_glyph_code = -1;		/* reset kerning*/
	pf->last_pen_pos = -32767;

	/* 
	 * Compute instance ascent & descent values
	 * in fractional units (1/64th pixel)
	 */
	TT_Get_Face_Properties (pf->face, &properties);
	TT_Get_Instance_Metrics(pf->instance, &imetrics);	
  
	ascent = ((properties.horizontal->Ascender * imetrics.y_scale)/0x10000);
	descent = ((properties.horizontal->Descender*imetrics.y_scale)/0x10000);

	/* 
	 * Offset the starting point if necessary,
	 * FreeType always aligns at baseline
	 */
	if (flags&MWTF_BOTTOM) {
		vec_x = 0;
		vec_y = descent;
		TT_Transform_Vector(&vec_x, &vec_y,&pf->matrix);
		x -= vec_x / 64;
		y += vec_y / 64;
	} else if (flags&MWTF_TOP) {
		vec_x = 0;
		vec_y = ascent;
		TT_Transform_Vector(&vec_x, &vec_y,&pf->matrix);
		x -= vec_x / 64;
		y += vec_y / 64;
	}

	/* Set the "graylevels" */
	if (pf->fontattr&MWTF_ANTIALIAS) {
		TT_Set_Raster_Gray_Palette (engine, virtual_palette);

		alphablend(psd, gray_palette, gr_foreground, gr_background,
			blend, 5);
	}

	startx = x;
	starty = y;
	for (i = 0; i < cc; i++) {
		curchar = TT_Char_Index (pf->char_map, str[i]);

		if (TT_Load_Glyph (pf->instance, pf->glyph, curchar,
			TTLOAD_DEFAULT) != TT_Err_Ok)
				continue;

		if (pf->fontrotation) {
			TT_Get_Glyph_Outline (pf->glyph, &outline);
			TT_Transform_Outline (&outline, &pf->matrix);
		}

		TT_Get_Glyph_Metrics (pf->glyph, &metrics);

		if ((pf->fontattr&MWTF_KERNING) && pf->can_kern) {
			if (pf->fontrotation) {
				vec_x = compute_kernval(pf, curchar);
				vec_y = 0;
				TT_Transform_Vector(&vec_x, &vec_y,&pf->matrix);

				x += vec_x / 64;
				y -= vec_y / 64;
			} else
				x += compute_kernval(pf, curchar) / 64;
		}
			
		drawchar(pf, psd, curchar, x, y);

		if (pf->fontrotation) {
			vec_x = metrics.advance;
			vec_y = 0;
			TT_Transform_Vector (&vec_x, &vec_y, &pf->matrix);

			x += vec_x / 64;
			y -= vec_y / 64;
		} else {
			x += metrics.advance / 64;

			/* Kerning point syndrome avoidance */
			if (pf->last_pen_pos > x)
				x = pf->last_pen_pos;
			pf->last_pen_pos = x;
		}

		pf->last_glyph_code = curchar;
	}

	if (pf->fontattr & MWTF_UNDERLINE)
		GdLine(psd, startx, starty, x, y, FALSE);
}

/*
 * Return information about a specified font.
 */
static MWBOOL
freetype_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	int	i;
	PMWFREETYPEFONT	pf = (PMWFREETYPEFONT)pfont;
	TT_Face_Properties 	properties;
	TT_Instance_Metrics imetrics;
	TT_UShort last_glyph_index;

	TT_Get_Face_Properties (pf->face, &properties);
	TT_Get_Instance_Metrics(pf->instance, &imetrics);

	/* Fill up the fields */
	pfontinfo->height = (((properties.horizontal->Ascender * \
	                        imetrics.y_scale)/ 0x10000) >> 6) -
	                    (((properties.horizontal->Descender * \
	                        imetrics.y_scale)/ 0x10000) >> 6);
	pfontinfo->maxwidth = ((properties.horizontal->xMax_Extent * \
	                        imetrics.x_scale)/ 0x10000) >> 6;
	pfontinfo->baseline = ((properties.horizontal->Ascender * \
	                        imetrics.y_scale)/ 0x10000) >> 6;
	pfontinfo->firstchar = TT_CharMap_First(pf->char_map, NULL);
	pfontinfo->lastchar = TT_CharMap_Last(pf->char_map, NULL);
	pfontinfo->fixed = properties.postscript->isFixedPitch;
		
	last_glyph_index = properties.num_Glyphs > 255 ? 255: properties.num_Glyphs-1;

	/* Doesn't work ... don't know why ....*/
#if 0
	if (TT_Get_Face_Widths( pf->face, 0,
				last_glyph_index, widths, NULL ) != TT_Err_Ok) {
	  return TRUE;
	}

	for(i=0; i<=last_glyph_index; i++)
	  DPRINTF("widths[%d]: %d\n", i, widths[i]);
#endif

	/* Get glyphs widths */
	for(i=0; i<=last_glyph_index; i++)
	  pfontinfo->widths[i] = Get_Glyph_Width(pf, i);

#if 0
	DPRINTF("x_ppem: %d\ny_ppem: %d\nx_scale: %d\ny_scale: %d\n\
    x_resolution: %d\ny_resolution: %d\n",
    imetrics.x_ppem, imetrics.y_ppem, imetrics.x_scale, \
    imetrics.y_scale, imetrics.x_resolution, imetrics.y_resolution);

    DPRINTF("Ascender: %d\nDescender: %d\nxMax_Extent: %d\n\
    Mac Style Say Italic?: %d\nMac Style Say Bold?: %d\n\
    sTypoAscender: %d\nsTypoDescender: %d\nusWinAscent: %d\n\
    usWinDescent: %d\nusFirstCharIndex: %d\nusLastCharIndex: %d\n\
    OS2 Say Italic?: %d\nOS2 Say Bold?: %d\nOS2 Say monospaced?: %d\n\
    Postscript Say monospaced?: %d\n",\
    properties.horizontal->Ascender,
    properties.horizontal->Descender,
    properties.horizontal->xMax_Extent,
    (properties.header->Mac_Style & 0x2)?1:0,
    (properties.header->Mac_Style & 0x1)?1:0,
    properties.os2->sTypoAscender,
    properties.os2->sTypoDescender,
    properties.os2->usWinAscent,
    properties.os2->usWinDescent,
    properties.os2->usFirstCharIndex,
    properties.os2->usLastCharIndex,
    (properties.os2->fsSelection & 0x1)?1:0,
    (properties.os2->fsSelection & 0x20)?1:0,
    properties.postscript->isFixedPitch,
    (properties.os2->panose[3] == 9)?1:0);
#endif	
			
	return TRUE;
}

static void
freetype_gettextsize(PMWFONT pfont, const void *text, int cc,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWFREETYPEFONT	pf = (PMWFREETYPEFONT)pfont;
	const unsigned short *	str = text;
	TT_F26Dot6 	x = 0;
	int 		i;
	TT_UShort 	curchar;
	TT_Glyph_Metrics metrics;
	TT_Face_Properties 	properties;
	TT_Instance_Metrics imetrics;

	TT_Get_Face_Properties (pf->face, &properties);
	TT_Get_Instance_Metrics(pf->instance, &imetrics);
	
	pf->last_glyph_code = -1;		/* reset kerning*/
	pf->last_pen_pos = -32767;
	
	for (i = 0; i < cc; i++) {
		curchar = TT_Char_Index (pf->char_map, str[i]);

		if (TT_Load_Glyph (pf->instance, pf->glyph, curchar,
			TTLOAD_SCALE_GLYPH|TTLOAD_HINT_GLYPH) != TT_Err_Ok)
				continue;

		TT_Get_Glyph_Metrics (pf->glyph, &metrics);

		if ((pf->fontattr&MWTF_KERNING) && pf->can_kern) {
				x += compute_kernval(pf, curchar) / 64;
		}
		
		x += metrics.advance / 64;

		/* Kerning point syndrome avoidance */
		if (pf->last_pen_pos > x)
			x = pf->last_pen_pos;
		pf->last_pen_pos = x;

		pf->last_glyph_code = curchar;
	}

	*pwidth = x;
	*pheight =  (((properties.horizontal->Ascender * 
	                imetrics.y_scale)/ 0x10000) >> 6) -
	            (((properties.horizontal->Descender * 
	            imetrics.y_scale)/ 0x10000) >> 6);
    	/* FIXME: is it what's required ?? */
	*pbase = (((-properties.horizontal->Descender) * 
	            imetrics.y_scale)/ 0x10000) >> 6;
}

static void
freetype_destroyfont(PMWFONT pfont)
{
	PMWFREETYPEFONT	pf = (PMWFREETYPEFONT)pfont;

	TT_Close_Face(pf->face);

	/*----------*/

	if (pf->glyph_cache != 0)
	  free_cache_glyph(pf->glyph_cache);

	/*----------*/

	free(pf);
}

static void
freetype_setfontsize(PMWFONT pfont, MWCOORD fontsize)
{
	PMWFREETYPEFONT	pf = (PMWFREETYPEFONT)pfont;
	
	pf->fontsize = fontsize;
	
	/* We want real pixel sizes ... not points ...*/
	TT_Set_Instance_PixelSizes( pf->instance, pf->fontsize,
                                pf->fontsize, pf->fontsize * 64 );
#if 0
	/* set charsize (convert to points for freetype)*/
	TT_Set_Instance_CharSize (pf->instance,
		((pf->fontsize * 72 + 96/2) / 96) * 64);
#endif
}

static void
freetype_setfontrotation(PMWFONT pfont, int tenthdegrees)
{
	PMWFREETYPEFONT	pf = (PMWFREETYPEFONT)pfont;
	float 		angle;
	
	pf->fontrotation = tenthdegrees;

	/* Build the rotation matrix with the given angle */
	TT_Set_Instance_Transform_Flags (pf->instance, TRUE, FALSE);

	angle = pf->fontrotation * M_PI / 1800;
	pf->matrix.yy = (TT_Fixed) (cos (angle) * (1 << 16));
	pf->matrix.yx = (TT_Fixed) (sin (angle) * (1 << 16));
	pf->matrix.xx = pf->matrix.yy;
	pf->matrix.xy = -pf->matrix.yx;
}

#endif /* HAVE_FREETYPE_SUPPORT*/

/* UTF-8 to UTF-16 conversion.  Surrogates are handeled properly, e.g.
 * a single 4-byte UTF-8 character is encoded into a surrogate pair.
 * On the other hand, if the UTF-8 string contains surrogate values, this
 * is considered an error and returned as such.
 *
 * The destination array must be able to hold as many Unicode-16 characters
 * as there are ASCII characters in the UTF-8 string.  This in case all UTF-8
 * characters are ASCII characters.  No more will be needed.
 *
 * Copyright (c) 2000 Morten Rolland, Screen Media
 */
static int
utf8_to_utf16(const unsigned char *utf8, int cc, unsigned short *unicode16)
{
	int count = 0;
	unsigned char c0, c1;
	unsigned long scalar;

	while(--cc >= 0) {
		c0 = *utf8++;
		/*DPRINTF("Trying: %02x\n",c0);*/

		if ( c0 < 0x80 ) {
			/* Plain ASCII character, simple translation :-) */
			*unicode16++ = c0;
			count++;
			continue;
		}

		if ( (c0 & 0xc0) == 0x80 )
			/* Illegal; starts with 10xxxxxx */
			return -1;

		/* c0 must be 11xxxxxx if we get here => at least 2 bytes */
		scalar = c0;
		if(--cc < 0)
			return -1;
		c1 = *utf8++;
		/*DPRINTF("c1=%02x\n",c1);*/
		if ( (c1 & 0xc0) != 0x80 )
			/* Bad byte */
			return -1;
		scalar <<= 6;
		scalar |= (c1 & 0x3f);

		if ( !(c0 & 0x20) ) {
			/* Two bytes UTF-8 */
			if ( scalar < 0x80 )
				return -1;	/* Overlong encoding */
			*unicode16++ = scalar & 0x7ff;
			count++;
			continue;
		}

		/* c0 must be 111xxxxx if we get here => at least 3 bytes */
		if(--cc < 0)
			return -1;
		c1 = *utf8++;
		/*DPRINTF("c1=%02x\n",c1);*/
		if ( (c1 & 0xc0) != 0x80 )
			/* Bad byte */
			return -1;
		scalar <<= 6;
		scalar |= (c1 & 0x3f);

		if ( !(c0 & 0x10) ) {
			/*DPRINTF("####\n");*/
			/* Three bytes UTF-8 */
			if ( scalar < 0x800 )
				return -1;	/* Overlong encoding */
			if ( scalar >= 0xd800 && scalar < 0xe000 )
				return -1;	/* UTF-16 high/low halfs */
			*unicode16++ = scalar & 0xffff;
			count++;
			continue;
		}

		/* c0 must be 1111xxxx if we get here => at least 4 bytes */
		c1 = *utf8++;
		if(--cc < 0)
			return -1;
		/*DPRINTF("c1=%02x\n",c1);*/
		if ( (c1 & 0xc0) != 0x80 )
			/* Bad byte */
			return -1;
		scalar <<= 6;
		scalar |= (c1 & 0x3f);

		if ( !(c0 & 0x08) ) {
			/* Four bytes UTF-8, needs encoding as surrogates */
			if ( scalar < 0x10000 )
				return -1;	/* Overlong encoding */
			scalar -= 0x10000;
			*unicode16++ = ((scalar >> 10) & 0x3ff) + 0xd800;
			*unicode16++ = (scalar & 0x3ff) + 0xdc00;
			count += 2;
			continue;
		}

		return -1;	/* No support for more than four byte UTF-8 */
	}
	return count;
}

#if HAVE_HZK_SUPPORT

/* UniCode-16 (MWTF_UC16) to GB(MWTF_ASCII) Chinese Characters conversion.
 * a single 2-byte UC16 character is encoded into a surrogate pair.
 * return -1 ,if error;
 * The destination array must be able to hold as many
 * as there are Unicode-16 characters.
 *
 * Copyright (c) 2000 Tang Hao (TownHall)(tang_hao@263.net).
 */
static int
UC16_to_GB(const unsigned char *uc16, int cc, unsigned char *ascii)
{
	FILE* fp;
	char buffer[256];
	unsigned char *uc16p;
	int i=0,j=0, k;
	unsigned char *filebuffer;
	unsigned short *uc16pp,*table;
	unsigned short uc16px;
	int length=31504;

	if (use_big5)
		length=54840;

    	uc16p=(unsigned char *) uc16;
	uc16pp=(unsigned short *) uc16;

	strcpy(buffer,HZK_FONT_DIR);
	if (use_big5)
    		strcat(buffer,"/BG2UBG.KU");
	else
    		strcat(buffer,"/UGB2GB.KU");
	if(!(fp = fopen(buffer, "rb"))) 
	{
   	  	 fprintf (stderr, "Error.\nThe %s file can not be found!\n",buffer);
   		 return -1;
    	}

	filebuffer= (unsigned char *)malloc ( length);

	if(fread(filebuffer, sizeof(char),length, fp) < length) {
	   	  fprintf (stderr, "Error in reading ugb2gb.ku file!\n");
	   	  fclose(fp);
 	     	  return -1;
	}
    	fclose(fp);

	if (use_big5)
	{
		table=(unsigned short *)filebuffer;
		while(1)
		{
			if(j>=cc)
			{
				ascii[i]=0;
				break;
			}
			uc16px=*uc16pp;
			if((uc16px)<=0x00ff)
			{
				ascii[i]=(char)(*uc16pp);
				i++;
			}
			else
			{
				ascii[i]=0xa1; ascii[i+1]=0x40;
				for (k=0; k<13710; k++)
				{
					if (*(table+(k*2+1))==(uc16px))
					{
						ascii[i]=(char)((*(table+(k*2)) & 0xff00) >> 8);
						ascii[i+1]=(char)(*(table+(k*2)) & 0x00ff);
						break;
					}
				}
				i+=2;
			}
			uc16pp++; j++;
		}
	}
	else
	{
	while(1)
	{
		if(j>=cc)
		{
			ascii[i]=0;
			break;
		}
		if((*((uc16p)+j)==0)&&(*((uc16p)+j+1)==0))
		{
			ascii[i]=0;
			break;
		}
		else
		{
			if(*((uc16p)+j+1)==0)
			{
				ascii[i]=*((uc16p)+j);
				i++;
				j+=2;
			}
			else
			{
			/*	 to find the place of unicode charater .¶þ·Ö·¨Æ¥Åä*/
            		{
				int p1=0,p2=length-4,p;
				unsigned int c1,c2,c,d;
				c1=((unsigned int )filebuffer[p1])*0x100+(filebuffer[p1+1]);
                		c2=((unsigned int )filebuffer[p2])*0x100+(filebuffer[p2+1]);
				d=((unsigned int )*((uc16p)+j))*0x100+*((uc16p)+j+1);
                		if(c1==d)
				{
					ascii[i]=filebuffer[p1+2];
					ascii[i+1]=filebuffer[p1+3];
					goto findit;
 	            		}
                		if(c2==d)
				{
					ascii[i]=filebuffer[p2+2];
					ascii[i+1]=filebuffer[p2+3];
					goto findit;
                		}
				while(1)
				{
					p=(((p2-p1)/2+p1)>>2)<<2;
					c=((unsigned int )filebuffer[p])*0x100+(filebuffer[p+1]);
					if(d==c)	/*find it*/
					{
						ascii[i]=filebuffer[p+2];
						ascii[i+1]=filebuffer[p+3];
						break;
          	   	   		}
					else if(p2<=p1+4) /*can't find.*/
					{
						ascii[i]='.';/*((uc16p)+j);*/
						ascii[i+1]='.';/*((uc16p)+j+1);*/
						break;
					}
					else if(d<c)
					{
						p2=p;
						c2=c;										
                  			}
					else
					{
						p1=p;
						c1=c;														
					}
				}
	            	}
			findit:
  			i+=2;
			j+=2;
			}
		}
	}
	}
	free(filebuffer);

	return i;
}

/************************** functions definition ******************************/

static int hzk_id( PMWHZKFONT pf )
{
	switch(pf->font_height)
	{
	case 12:
		return 0;
	case 16: default:
		return 1;
	}
}

/* This function get Chinese font info from etc file.*/
static MWBOOL GetCFontInfo( PMWHZKFONT pf )
{
	int charset;

	if (use_big5)
		charset=(13094+408);
	else
		charset=8178;

    	CFont[hzk_id(pf)].width = pf->cfont_width;
    	pf->CFont.width = pf->cfont_width;

    	CFont[hzk_id(pf)].height = pf->font_height;
    	pf->CFont.height = pf->font_height;

    	CFont[hzk_id(pf)].size = ((pf->CFont.width + 7) / 8) *
		pf->CFont.height * charset;
    	pf->CFont.size = ((pf->CFont.width + 7) / 8) * pf->CFont.height * charset;

    	if(pf->CFont.size < charset * 8)
        	return FALSE;

	strcpy(CFont[hzk_id(pf)].file,HZK_FONT_DIR);
	strcpy(pf->CFont.file,HZK_FONT_DIR);

	if(pf->font_height==16)
	{
		strcat(CFont[hzk_id(pf)].file,"/hzk16");
		strcat(pf->CFont.file,"/hzk16");
	}
	else
	{
		strcat(CFont[hzk_id(pf)].file,"/hzk12");
		strcat(pf->CFont.file,"/hzk12");
	}

    	if (use_big5)
    	{
		CFont[hzk_id(pf)].file[strlen(pf->CFont.file)-3]+=use_big5;
		pf->CFont.file[strlen(pf->CFont.file)-3]+=use_big5;
    	}

    	return TRUE;
}

/* This function get ASCII font info from etc file.*/
static MWBOOL GetAFontInfo( PMWHZKFONT pf )
{
    	AFont[hzk_id(pf)].width = pf->afont_width;
    	pf->AFont.width = pf->afont_width;

    	AFont[hzk_id(pf)].height = pf->font_height;
    	pf->AFont.height = pf->font_height;

    	AFont[hzk_id(pf)].size = ((pf->AFont.width + 7) / 8) *
		pf->AFont.height * 255;
    	pf->AFont.size = ((pf->AFont.width + 7) / 8) * pf->AFont.height * 255;
    
	if(pf->AFont.size < 255 * 8)
        	return FALSE;

	strcpy(AFont[hzk_id(pf)].file,HZK_FONT_DIR);
	strcpy(pf->AFont.file,HZK_FONT_DIR);
	
	if(pf->font_height==16)
	{
	    	strcat(AFont[hzk_id(pf)].file,"/asc16");
	    	strcat(pf->AFont.file,"/asc16");
	}
  	else
	{
	    	strcat(AFont[hzk_id(pf)].file,"/asc12");
	    	strcat(pf->AFont.file,"/asc12");
	}
    	return TRUE;
}

/* This function load system font into memory.*/
static MWBOOL LoadFont( PMWHZKFONT pf )
{
    	FILE* fp;

	if(!GetCFontInfo(pf))
	{
		fprintf (stderr, "Get Chinese HZK font info failure!\n");
		return FALSE;
	}
    	if(CFont[hzk_id(pf)].pFont == NULL)/*check font cache*/
	{
		

 	   	/* Allocate system memory for Chinese font.*/
 		if( !(CFont[hzk_id(pf)].pFont = (char *)malloc(pf->CFont.size)) )
 		{
	 	       	fprintf (stderr, "Allocate memory for Chinese HZK font failure.\n");
		        return FALSE;
	 	}
 	
		/* Open font file and read information to the system memory.*/
 		fprintf (stderr, "Loading Chinese HZK font from file(%s)..." ,pf->CFont.file);
		if(!(fp = fopen(CFont[hzk_id(pf)].file, "rb"))) 
		{
   		  	fprintf (stderr, "Error.\nThe Chinese HZK font file can not be found!\n");
   	    	 	return FALSE;
    		}
	    	if(fread(CFont[hzk_id(pf)].pFont, sizeof(char), pf->CFont.size, fp) < pf->CFont.size) 
		{
	      	  	fprintf (stderr, "Error in reading Chinese HZK font file!\n");
	 	     	fclose(fp);
 	       		return FALSE;
		}

		fclose(fp);

		CFont[hzk_id(pf)].use_count=0;

		fprintf (stderr, "done.\n" );

	}
	cfont_address = CFont[hzk_id(pf)].pFont;
	pf->cfont_address = CFont[hzk_id(pf)].pFont;
	pf->CFont.pFont = CFont[hzk_id(pf)].pFont;

	CFont[hzk_id(pf)].use_count++;

	if(!GetAFontInfo(pf))
	{
	       fprintf (stderr, "Get ASCII HZK font info failure!\n");
	       return FALSE;
	}
    	if(AFont[hzk_id(pf)].pFont == NULL)/*check font cache*/
	{
		
 		
 		/* Allocate system memory for ASCII font.*/
 		if( !(AFont[hzk_id(pf)].pFont = (char *)malloc(pf->AFont.size)) )
 		{
 		       	fprintf (stderr, "Allocate memory for ASCII HZK font failure.\n");
 		       	free(CFont[hzk_id(pf)].pFont);
 		       	CFont[hzk_id(pf)].pFont = NULL;
			return FALSE;
 		}
 	
	 	/* Load ASCII font information to the near memory.*/
 		fprintf (stderr, "Loading ASCII HZK font..." );
 		if(!(fp = fopen(AFont[hzk_id(pf)].file, "rb"))) 
		{
 		       	fprintf (stderr, "Error.\nThe ASCII HZK font file can not be found!\n");
 		       	return FALSE;
 		}
	 	if(fread(AFont[hzk_id(pf)].pFont, sizeof(char), pf->AFont.size, fp) < pf->AFont.size) 
		{
 		       	fprintf (stderr, "Error in reading ASCII HZK font file!\n");
 		       	fclose(fp);
 		       	return FALSE;
	 	}
 	
 		fclose(fp);
 	
		AFont[hzk_id(pf)].use_count=0;

		fprintf (stderr, "done.\n" );

  	}
	afont_address = AFont[hzk_id(pf)].pFont;
	pf->afont_address = AFont[hzk_id(pf)].pFont;
	pf->AFont.pFont = AFont[hzk_id(pf)].pFont;

	AFont[hzk_id(pf)].use_count++;

  	return TRUE;
}

/* This function unload system font from memory.*/
static void UnloadFont( PMWHZKFONT pf )
{
	CFont[hzk_id(pf)].use_count--;
	AFont[hzk_id(pf)].use_count--;

	if (!CFont[hzk_id(pf)].use_count)
	{	
	    	free(pf->CFont.pFont);
	    	free(pf->AFont.pFont);

	    	CFont[hzk_id(pf)].pFont = NULL;
	    	AFont[hzk_id(pf)].pFont = NULL;
	}
}

static int
hzk_init(PSD psd)
{
	/* FIXME: *.KU file should be opened and
	 * read in here...*/
	return 1;
}

static PMWHZKFONT
hzk_createfont(const char *name, MWCOORD height, int attr)
{
	PMWHZKFONT	pf;

	if(strcmp(name,"HZKFONT")!=0 && strcmp(name,"HZXFONT")!=0)
		return FALSE;

printf("hzk_createfont(%s,%d)\n",name,height);

	use_big5=name[2]-'K';

	/* allocate font structure*/
	pf = (PMWHZKFONT)calloc(sizeof(MWHZKFONT), 1);
	if (!pf)
		return NULL;
	pf->fontprocs = &hzk_procs;

	pf->fontsize=height;
	/*GdSetFontSize((PMWFONT)pf, height);*/
	GdSetFontRotation((PMWFONT)pf, 0);
	GdSetFontAttr((PMWFONT)pf, attr, 0);

	if(height==12)
	{		
		afont_width = 6;
		cfont_width = 12;
		font_height = 12;

		pf->afont_width = 6;
		pf->cfont_width = 12;
		pf->font_height = 12;
	}
	else 	
	{		
		afont_width = 8;
		cfont_width = 16;
		font_height = 16;

		pf->afont_width = 8;
		pf->cfont_width = 16;
		pf->font_height = 16;
	}

    	/* Load the font library to the system memory.*/
	if(!LoadFont(pf))
  	      	return FALSE;

	return pf;
}

int IsBig5(int i)
{
	if ((i>=0xa140 && i<=0xa3bf) || /* a140-a3bf(!a3e0) */
	    (i>=0xa440 && i<=0xc67e) || /* a440-c67e        */
	    (i>=0xc6a1 && i<=0xc8d3) || /* c6a1-c8d3(!c8fe) */
	    (i>=0xc940 && i<=0xf9fe))   /* c940-f9fe        */
		return 1;
	else
		return 0;
}

/*
 * following several function is used in hzk_drawtext
 */

static int getnextchar(char* s, unsigned char* cc)
{
    	if( s[0] == '\0') return 0;

    	cc[0] = (unsigned char)(*s);
    	cc[1] = (unsigned char)(*(s + 1));

    	if (use_big5)
    	{
		if( IsBig5( (int) ( (cc[0] << 8) + cc[1]) ) )
			return 1;
    	}
    	else
	{
    		if( ((unsigned char)cc[0] > 0xa0) &&
		    ((unsigned char)cc[1] > 0xa0) )
        		return 1;
	}

    	cc[1] = '\0';

    	return 1;
}

static void
expandcchar(PMWHZKFONT pf, int bg, int fg, unsigned char* c, MWPIXELVAL* bitmap)
{
	int i=0;
    	int c1, c2, seq;
	int x,y;
    	unsigned char *font;
    	int b = 0;		/* keep gcc happy with b = 0 - MW */

	int pixelsize;
	pixelsize=sizeof(MWPIXELVAL);

   	c1 = c[0];
    	c2 = c[1];
	if (use_big5)
	{
		seq=0;
		/*ladd=loby-(if(loby<127)?64:98)*/
		c2-=(c2<127?64:98);   

		/*hadd=(hiby-164)*157*/
		if (c1>=0xa4)/*standard font*/
		{
			seq=(((c1-164)*157)+c2);
			if (seq>=5809) seq-=408;
		}

		/*hadd=(hiby-161)*157*/
		if (c1<=0xa3)/*special font*/
			seq=(((c1-161)*157)+c2)+13094;
	}
	else
    		seq=((c1 - 161)*94 + c2 - 161); 

	font = pf->cfont_address + ((seq) *
		  (pf->font_height * ((pf->cfont_width + 7) / 8)));

     	for (y = 0; y < pf->font_height; y++)
        	for (x = 0; x < pf->cfont_width; x++) 
		{
            		if (x % 8 == 0)
                		b = *font++;

            		if (b & (128 >> (x % 8)))   /* pixel */
			  	bitmap[i++]=fg;
			else
				bitmap[i++]=bg;
		}		
}

static void expandchar(PMWHZKFONT pf, int bg, int fg, int c, MWPIXELVAL* bitmap)
{
	int i=0;
	int x,y;
    	unsigned char *font;
	int pixelsize;
    	int b = 0;		/* keep gcc happy with b = 0 - MW */

	pixelsize=sizeof(MWPIXELVAL);

    	font = pf->afont_address + c * (pf->font_height *
		((pf->afont_width + 7) / 8));

  	for (y = 0; y < pf->font_height; y++)
		for (x = 0; x < pf->afont_width; x++) 
		{
	    		if (x % 8 == 0)
				b = *font++;
	    		if (b & (128 >> (x % 8)))	/* pixel */
				bitmap[i++]=fg;
			else
				bitmap[i++]=bg;
  		}
}

/*
 * Draw ASCII text string using HZK type font
 */
static void
hzk_drawtext(PMWFONT pfont, PSD psd, MWCOORD ax, MWCOORD ay,
	const void *text, int cc, int flags)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

    	unsigned char c[2];
	MWPIXELVAL *bitmap;
    	unsigned char s1[3];
 	char *s,*sbegin;

	s=(char *)text;

	if(cc==1)
	{
		s1[0]=*((unsigned char*)text);
		s1[1]=0x0;
		s1[2]=0x0;
		s=s1;
    	}

	sbegin=s;
    	bitmap = (MWPIXELVAL *)ALLOCA(pf->cfont_width * pf->font_height *
			sizeof(MWPIXELVAL));

    	while( getnextchar(s, c) )
	{
              	if( c[1] != '\0') 
		{
                	expandcchar(pf, gr_background,gr_foreground,
                            c, bitmap);
			/* Now draw the bitmap ... */
			
			if (flags&MWTF_TOP)
				GdArea(psd,ax, ay, pf->cfont_width,
					pf->font_height, bitmap, MWPF_PIXELVAL);
			else
				GdArea(psd,ax, ay-pf->font_height+2,
					pf->cfont_width, pf->font_height,
					bitmap, MWPF_PIXELVAL);

                	s += 2;
                	ax += pf->cfont_width;
            	}
            	else 
		{
                	expandchar(pf, gr_background,gr_foreground,
                           c[0], bitmap);
			/* Now draw the bitmap ... */

			if (flags&MWTF_TOP) 
				GdArea(psd,ax, ay, pf->afont_width,
					pf->font_height, bitmap, MWPF_PIXELVAL);
			else
				GdArea(psd,ax, ay-pf->font_height+2,
					pf->afont_width, pf->font_height,
					bitmap, MWPF_PIXELVAL);

                	s += 1;
                	ax += pf->afont_width;
            	}
						
		if(s>=sbegin+cc)break;
    	}

	FREEA(bitmap);
}

/*
 * Return information about a specified font.
 */
static MWBOOL
hzk_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

	int i;

	pfontinfo->height = pf->font_height;
	pfontinfo->maxwidth = pf->cfont_width;
	pfontinfo->baseline = pf->font_height-2;
	pfontinfo->firstchar = 0;
	pfontinfo->lastchar = 0;
	pfontinfo->fixed = TRUE;
		
	for(i=0; i<=256; i++)
		pfontinfo->widths[i] = pf->afont_width;

	return TRUE;
}

static void
hzk_gettextsize(PMWFONT pfont, const void *text, int cc,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

   	unsigned char c[2];
 	char *s,*sbegin;
    	unsigned char s1[3];

	int ax=0;



	s=(char *)text;
	if(cc==0)
	{
		*pwidth = 0;
		*pheight = pf->font_height;
		*pbase = pf->font_height-2;

	}
	if(cc==1)
	{
		s1[0]=*((unsigned char*)text);
		s1[1]=0x0;
		s1[2]=0x0;
		s=s1;
    	}
	sbegin=s;
    	while( getnextchar(s, c) )
	{
		if( c[1] != '\0') 
		{
           		s += 2;
           		ax += pf->cfont_width;
        	}
        	else 
		{
           		s += 1;
           		ax += pf->afont_width;
        	}
		if(s>=sbegin+cc)break;
		/*	fprintf(stderr,"s=%x,sbegin=%x,cc=%x\n",s,sbegin,cc);*/

    	}
/*	fprintf(stderr,"ax=%d,\n",ax);*/

	*pwidth = ax;
	*pheight = pf->font_height;
	*pbase = pf->font_height-2;
}

static void
hzk_destroyfont(PMWFONT pfont)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

	UnloadFont(pf);
	free(pf);
}

static void
hzk_setfontsize(PMWFONT pfont, MWCOORD fontsize)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

	/*jmt: hzk_setfontsize not supported*/
	/*jmt: & pf->fontsize can't be changed*/
	/*jmt: because of hzk_id() :p*/
	pf->fontsize=pf->font_height;
}
#endif /* HAVE_HZK_SUPPORT*/

/* FIXME: this routine should work for all font renderers...*/
int
GdGetTextSizeEx(PMWFONT pfont, const void *str, int cc,int nMaxExtent,
	int* lpnFit, int* alpDx,MWCOORD *pwidth,MWCOORD *pheight,
	MWCOORD *pbase, int flags)
{
#ifdef HAVE_FREETYPE_SUPPORT
	unsigned short	buf[256];
	unsigned short* text;
	PMWFREETYPEFONT	pf = (PMWFREETYPEFONT)pfont;
	int		defencoding = pf->fontprocs->encoding;
	int 		x = 0;
	int 		i;
	TT_UShort 	curchar;
	TT_Glyph_Metrics metrics;
	TT_Face_Properties 	properties;
	TT_Instance_Metrics imetrics;

	if ((cc<0)||(!str))
	{
		*pwidth = *pheight = *pbase = 0;
		return 0;
	}
	/* convert encoding if required*/
	if((flags & MWTF_PACKMASK) != defencoding) 
	{
		cc = GdConvertEncoding(str, flags, cc, buf, defencoding);
		flags &= ~MWTF_PACKMASK;
		flags |= defencoding;
		text=buf;
	} else text =(unsigned short*)str;
	if(cc <= 0)
	{
		*pwidth = *pheight = *pbase = 0;
		return 0;
	}

	TT_Get_Face_Properties (pf->face, &properties);
	TT_Get_Instance_Metrics(pf->instance, &imetrics);
	
	pf->last_glyph_code = -1;		/* reset kerning*/
	pf->last_pen_pos = -32767;
	if (lpnFit)
	     *lpnFit=-1;
	for (i = 0; i < cc; i++) 
	{
		curchar = TT_Char_Index (pf->char_map,text[i]);
		
		if (TT_Load_Glyph (pf->instance, pf->glyph, curchar,
			TTLOAD_SCALE_GLYPH|TTLOAD_HINT_GLYPH) != TT_Err_Ok)			
		{		
		     printf("Unable to load glyph with index=%d\n",curchar);    
			return 0;
		}
		TT_Get_Glyph_Metrics (pf->glyph, &metrics);
		if ((pf->fontattr&MWTF_KERNING) && pf->can_kern)
		{
			x += compute_kernval(pf, curchar) / 64;
		}
		x += metrics.advance / 64;
     		if((lpnFit)&&(alpDx))
		{
			if (x<=nMaxExtent)
			     alpDx[i]=x;
			else
			     if (*lpnFit==-1)
					(*lpnFit)=i;               		     
		}
		/* Kerning point syndrome avoidance */
		if (pf->last_pen_pos > x)
			x = pf->last_pen_pos;
		pf->last_pen_pos = x;
		pf->last_glyph_code = curchar;
	}
     	if ((lpnFit)&&(*lpnFit==-1))
    		*lpnFit=cc;
	*pwidth = x;
	*pheight = (((properties.horizontal->Ascender * 
	                imetrics.y_scale)/ 0x10000) >> 6) -
			    (((properties.horizontal->Descender * 
			    imetrics.y_scale)/ 0x10000) >> 6);
	/*FIXME: is it what's required ??*/
	if (pbase)
		*pbase = (((-properties.horizontal->Descender) * 
			    imetrics.y_scale)/ 0x10000) >> 6;
	return 1;
#else /* HAVE_FREETYPE_SUPPORT*/
	*pwidth = *pheight = *pbase = 0;
	return 0;
#endif
}
