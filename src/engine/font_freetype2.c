/*
 * Freetype 2 driver for Microwindows
 *
 * Originally written by Koninklijke Philips Electronics N.V.
 *
 * (Loosly) Based on the FreeType 1.x driver, font_freetype.c.
 *
 * Portions contributed by Koninklijke Philips Electronics N.V.
 * These portions are Copyright 2002 Koninklijke Philips Electronics
 * N.V.  All Rights Reserved.  These portions are licensed under the
 * terms of the Mozilla Public License, version 1.1, or, at your
 * option, the GNU General Public License version 2.0.  Please see
 * the file "ChangeLog" for documentation regarding these
 * contributions.
 */

/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string.h>

#include <dirent.h>

#include "device.h"

#include "devfont.h"

#if (UNIX | DOS_DJGPP)
#define strcmpi strcasecmp
#endif

/* temp extern decls*/
extern MWPIXELVAL gr_foreground;
extern MWPIXELVAL gr_background;
extern MWBOOL gr_usebg;

/*
 * Enable the Freetype 2 font cache.  Only applicable if
 * FreeType 2 itself is enabled.
 *
 * It is STRONGLY recommended that you turn this option on,
 * as to will give a HUGE speed boost.  If you are using many
 * MicroWindows font objects, this can also save memory.
 * (There is a single cache shared across the system, and it
 * has a fixed memory usage.  Without caching, memory use is
 * proportional to the number of MWFREETYPE2FONT objects).
 *
 * FIXME: This option should be in the config file.
 */
#define HAVE_FREETYPE_2_CACHE 1


/*
 * Enable the Freetype 2 character map cache.  Only applicable if
 * FreeType 2 itself is enabled, and HAVE_FREETYPE_2_CACHE is also
 * enabled.
 *
 * It is recommended that you turn this option on if you are
 * using FreeType 2.1.1 or later, as it should give a small
 * speed boost.  (With earlier releases, this should work but
 * might actually slow down rendering slightly - the cache was
 * much slower before FreeType 2.1.1.)
 *
 * FIXME: This option should be in the config file.
 */
#define HAVE_FREETYPE_2_CMAP_CACHE 0


/****************************************************************************/
/* FreeType 2.x                                                             */
/****************************************************************************/

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRIGONOMETRY_H
#include FT_GLYPH_H
#if HAVE_FREETYPE_2_CACHE
# include FT_CACHE_H
# include FT_CACHE_SMALL_BITMAPS_H
# if HAVE_FREETYPE_2_CMAP_CACHE
#  include FT_CACHE_CHARMAP_H
# endif
#endif

/*
 * The default Freetype font directory.
 */
#ifndef FREETYPE_FONT_DIR
#define FREETYPE_FONT_DIR "/usr/local/microwin/fonts"
#endif

/*
 * The Freetype 2 font directory.
 */
char *freetype2_font_dir;


typedef struct freetype2_fontdata_ freetype2_fontdata;

struct freetype2_fontdata_
{
	int isBuffer;
	union
	{
		char *filename;

		struct
		{
			unsigned char *data;
			unsigned length;
		}
		buffer;
	}
	data;
	int refcount;		/* Currently only used for buffers, not files */
#if HAVE_FREETYPE_2_CACHE
	freetype2_fontdata *next;
#endif
};

struct MWFREETYPE2FONT_STRUCT
{
	PMWFONTPROCS fontprocs;	/* common hdr */
	MWCOORD fontsize;
	int fontrotation;
	int fontattr;

	/* freetype stuff */

	char *filename;		/* NULL if buffered */
	freetype2_fontdata *faceid;	/* only used if HAVE_FREETYPE_2_CACHE or buffered. */
#if HAVE_FREETYPE_2_CACHE
	FTC_ImageDesc imagedesc;
#if HAVE_FREETYPE_2_CMAP_CACHE
	FTC_CMapDescRec cmapdesc;
#endif
#else
	FT_Face face;
#endif
	FT_Matrix matrix;

};

static MWBOOL freetype2_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void freetype2_gettextsize(PMWFONT pfont, const void *text, int cc,
				  MWCOORD * pwidth, MWCOORD * pheight,
				  MWCOORD * pbase);
static void freetype2_destroyfont(PMWFONT pfont);
static void freetype2_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
			       const void *text, int cc, MWTEXTFLAGS flags);
static void freetype2_setfontsize(PMWFONT pfont, MWCOORD fontsize);
static void freetype2_setfontrotation(PMWFONT pfont, int tenthdegrees);
static void freetype2_setfontattr(PMWFONT pfont, int setflags, int clrflags);
#if 0 /* FIXME Nano-X doesn't have an API for this feature */
static PMWFONT freetype2_duplicate(PMWFONT psrcfont, MWCOORD fontsize);
#endif

/* handling routines for MWFREETYPE2FONT*/
static MWFONTPROCS freetype2_procs = {
	MWTF_UC16,		/* routines expect unicode 16 */
	freetype2_getfontinfo,
	freetype2_gettextsize,
	NULL,			/* gettextbits */
	freetype2_destroyfont,
	freetype2_drawtext,
	freetype2_setfontsize,
	freetype2_setfontrotation,
	freetype2_setfontattr,
};

static FT_Library freetype2_library = NULL;	/* THE ONLY freetype library instance */


/****************************************************************************/
/* FreeType 2.x                                                             */
/****************************************************************************/

#if 0				/* Old fake anti-aliasing */
/* 16 colors to use for alpha blending, from
 * freetype2_gray_palette[0]   = background to
 * freetype2_gray_palette[15] = foreground.
 */
static OUTPIXELVAL freetype2_gray_palette[16];


static unsigned char freetype2_alpha_levels[16] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};
#endif /* Old fake anti-aliasing */


static PMWFREETYPE2FONT
freetype2_createfont_internal(freetype2_fontdata * faceid,
			      char *filename, MWCOORD height);

#if HAVE_FREETYPE_2_CACHE
/*
 * The Freetype 2 cache subsystem requires a 1-1 mapping
 * between "void *" pointers and actual font files.  The
 * only almost-sane way to do this is to have a linked list
 * of font file names, and use pointers to the entries
 * in that array.  Font names are added to the list the
 * first time the font is used, and never removed.
 */
static freetype2_fontdata *freetype2_fonts = NULL;

static FTC_Manager freetype2_cache_manager;
static FTC_SBitCache freetype2_cache_sbit;
#if HAVE_FREETYPE_2_CMAP_CACHE
static FTC_CMapCache freetype2_cache_cmap;
#endif

static FT_Error
freetype2_face_requester(FTC_FaceID face_id,
			 FT_Library library,
			 FT_Pointer request_data, FT_Face * aface)
{
	freetype2_fontdata *fontdata = (freetype2_fontdata *) face_id;	// simple typecast

	if (fontdata->isBuffer) {
		/*char * buffer = fontdata->data.buffer.data;
		   printf ("Font magic = '%c%c%c%c', len = %u @ freetype2_face_requester\n", 
		   (char)buffer[0], (char)buffer[1],
		   (char)buffer[2], (char)buffer[3],
		   (unsigned)fontdata->data.buffer.length); */
		return FT_New_Memory_Face(library, fontdata->data.buffer.data,
					  fontdata->data.buffer.length, 0,
					  aface);
	} else {
		/*printf ("Loading font from file '%s' @ freetype2_face_requester\n", 
		   fontdata->data.filename); */
		return FT_New_Face(library, fontdata->data.filename, 0,
				   aface);
	}
}
#endif


/* Initialization */
int
freetype2_init(PSD psd)
{
	FT_Error err;

	if (freetype2_library != NULL) {
		return 1;
	}

	if ((freetype2_font_dir = getenv("MWFONTS")) == NULL) {
		freetype2_font_dir = FREETYPE_FONT_DIR;
	}

	/* Init freetype library */
	err = FT_Init_FreeType(&freetype2_library);

	if (err != FT_Err_Ok) {
		freetype2_library = NULL;
		EPRINTF("Error 0x%x initializing FreeType 2\n", err);
		return 0;
	}
#if HAVE_FREETYPE_2_CACHE
	/* initialize cache manager */
	err = FTC_Manager_New(freetype2_library, 3,	/* Faces */
			      5,	/* Sizes */
			      512 * 1024,	/* Bytes - 512K */
			      &freetype2_face_requester, NULL,
			      &freetype2_cache_manager);
	if (err != FT_Err_Ok) {
		EPRINTF("Error 0x%x initializing FreeType 2 cache system\n",
			err);

		freetype2_cache_manager = NULL;
		FT_Done_FreeType(freetype2_library);
		freetype2_library = NULL;
		return 0;
	}

	err = FTC_SBitCache_New(freetype2_cache_manager,
				&freetype2_cache_sbit);
	if (err != FT_Err_Ok) {
		EPRINTF("Error 0x%x initializing FreeType 2 sbit cache system\n", err);

		freetype2_cache_sbit = NULL;
		FTC_Manager_Done(freetype2_cache_manager);
		freetype2_cache_manager = NULL;
		FT_Done_FreeType(freetype2_library);
		freetype2_library = NULL;
		return 0;
	}
#if HAVE_FREETYPE_2_CMAP_CACHE
	err = FTC_CMapCache_New(freetype2_cache_manager,
				&freetype2_cache_cmap);
	if (err != FT_Err_Ok) {
		EPRINTF("Error 0x%x initializing FreeType 2 cmap cache system\n", err);

		freetype2_cache_cmap = NULL;
		// FIXME: Should we free the SBit cache here?
		freetype2_cache_sbit = NULL;
		FTC_Manager_Done(freetype2_cache_manager);
		freetype2_cache_manager = NULL;
		FT_Done_FreeType(freetype2_library);
		freetype2_library = NULL;
		return 0;
	}
#endif
#endif

	return 1;
}

PMWFREETYPE2FONT
freetype2_createfont(const char *name, MWCOORD height, int attr)
{
	PMWFREETYPE2FONT pf;
	char *p;
	char *fontname;
	freetype2_fontdata *faceid = NULL;
#if HAVE_FREETYPE_2_CACHE
	int first_time = 0;
#endif

	/* Initialization */
	if (freetype2_library == NULL) {
		/* Init freetype library */
		if (!freetype2_init(NULL)) {
			return NULL;
		}
	}

	fontname = malloc(6 + strlen(name) + strlen(freetype2_font_dir));
	if (fontname == NULL)
		return NULL;

	/* check for pathname prefix */
	if (strchr(name, '/') != NULL) {
		strcpy(fontname, name);
	} else {
		strcpy(fontname, freetype2_font_dir);
		strcat(fontname, "/");
		strcat(fontname, name);
	}

	/* check for extension */
	if ((p = strrchr(fontname, '.')) == NULL ||
	    ((strcmpi(p, ".ttf") != 0) && (strcmpi(p, ".pfr") != 0))) {
		strcat(fontname, ".ttf");
	}
#if HAVE_FREETYPE_2_CACHE
	for (faceid = freetype2_fonts; (faceid != NULL)
	     && (0 != strcmpi(faceid->data.filename, fontname));
	     faceid = faceid->next) {
		/* No-op (loop condition and step do everything we need) */
	}
	if (faceid == NULL) {
		/* Not found in list, so add it. */
		printf("JGF-Nano-X: Adding new font: %s\n", fontname);
		faceid = (freetype2_fontdata *) calloc(sizeof(*faceid), 1);
		if (faceid == NULL) {
			free(fontname);
			return NULL;
		}
		/* faceid->isBuffer = 0; - implied by calloc */
		faceid->data.filename = fontname;

		/* Add to font list. */
		faceid->next = freetype2_fonts;
		freetype2_fonts = faceid;

		/* Set a special flag.  If we can't load the font,
		 * we want to destroy the faceid.
		 */
		first_time = 1;
	} else {
		free(fontname);
	}
	fontname = faceid->data.filename;
#else
	faceid = NULL;
#endif

	pf = freetype2_createfont_internal(faceid, fontname, height);
	if (!pf) {
#if HAVE_FREETYPE_2_CACHE
		if (first_time) {
			assert(freetype2_fonts == faceid);
			freetype2_fonts = faceid->next;
			free(fontname);
			free(faceid);
		}
#else
		free(fontname);
#endif
		return NULL;
	}

	GdSetFontAttr((PMWFONT) pf, attr, 0);

	return pf;
}

#if 0 /* FIXME Nano-X doesn't have an API for this feature */
PMWFREETYPE2FONT
freetype2_createfontfrombuffer(unsigned char *buffer,
			       unsigned length, MWCOORD height)
{
	PMWFREETYPE2FONT pf;
	freetype2_fontdata *faceid = NULL;
	unsigned char *buffercopy;

	/* Initialization */
	if (freetype2_library == NULL) {
		/* Init freetype library */
		if (!freetype2_init(NULL)) {
			return NULL;
		}
	}

	faceid = (freetype2_fontdata *) calloc(sizeof(freetype2_fontdata), 1);
	if (!faceid)
		return NULL;

	buffercopy = (unsigned char *) malloc(length);
	if (!buffercopy) {
		free(faceid);
		return NULL;
	}
	memcpy(buffercopy, buffer, length);

	faceid->isBuffer = TRUE;
	faceid->data.buffer.length = length;
	faceid->data.buffer.data = buffercopy;
	faceid->refcount = 1;

	/*printf ("Font magic = '%c%c%c%c', len = %u @ freetype2_createfontfrombuffer\n", 
	   (char)buffercopy[0], (char)buffercopy[1],
	   (char)buffercopy[2], (char)buffercopy[3],
	   length); */

	pf = freetype2_createfont_internal(faceid, NULL, height);
	if (!pf) {
		free(faceid);
	}

	return pf;
}
#endif

static PMWFREETYPE2FONT
freetype2_createfont_internal(freetype2_fontdata * faceid,
			      char *filename, MWCOORD height)
{
	PMWFREETYPE2FONT pf;

	/* allocate font structure */
	pf = (PMWFREETYPE2FONT) calloc(sizeof(MWFREETYPE2FONT), 1);
	if (!pf) {
		return NULL;
	}
	pf->fontprocs = &freetype2_procs;

	pf->faceid = faceid;
	pf->filename = filename;
#if HAVE_FREETYPE_2_CACHE
	pf->imagedesc.font.face_id = faceid;
	pf->imagedesc.font.pix_width = 0;	/* Will be set by GdSetFontSize */
	pf->imagedesc.font.pix_height = 0;	/* Will be set by GdSetFontSize */
	pf->imagedesc.type = 0;	/* Will be set by GdSetFontAttr */
#if HAVE_FREETYPE_2_CMAP_CACHE
	pf->cmapdesc.face_id = faceid;
	pf->cmapdesc.type = FTC_CMAP_BY_ENCODING;
	pf->cmapdesc.u.encoding = ft_encoding_unicode;
#endif
#else
	/* Load face */
	if (filename) {
		if (FT_New_Face(freetype2_library, filename, 0, &pf->face) !=
		    FT_Err_Ok) {
			EPRINTF("JGF-Nano-X: Can't load font from file \"%s\"\n", filename);
			goto out;
		}
		/*printf("JGF-Nano-X: Loading font from file \"%s\"\n", filename); */
	} else {
		if (FT_New_Memory_Face
		    (freetype2_library, buffer, length, 0,
		     &pf->face) != FT_Err_Ok) {
			EPRINTF("JGF-Nano-X: Can't load font from memory\n");
			goto out;
		}
		/*printf("JGF-Nano-X: Loading font from memory\n"); */
	}

	if (FT_Select_Charmap(pf->face, ft_encoding_unicode) != FT_Err_Ok) {
		EPRINTF("freetype2_createfont: no unicode map table\n");
		goto out;
	}
#endif

	GdSetFontSize((PMWFONT) pf, height);
	GdSetFontRotation((PMWFONT) pf, 0);
	GdSetFontAttr((PMWFONT) pf, 0, 0);

#if HAVE_FREETYPE_2_CACHE
	{
		FT_Face face;
		FT_Size size;
		if (FTC_Manager_Lookup_Size
		    (freetype2_cache_manager, &(pf->imagedesc.font), &face,
		     &size)) {
			/*DPRINTF("Freetype 2 error trying to load font.\n"); */
			free(pf);
			return NULL;
		}
	}
#endif

	return pf;

#if ! HAVE_FREETYPE_2_CACHE
      out:
	if (pf->face != NULL) {
		FT_Done_Face(pf->face);
	}
	free(pf);
	return NULL;
#endif
}

static void
freetype2_destroyfont(PMWFONT pfont)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

#if ! HAVE_FREETYPE_2_CACHE
	FT_Done_Face(pf->face);

	if (pf->filename)
		free(pf->filename);
#endif

	if (pf->faceid && pf->faceid->isBuffer) {
		if (--pf->faceid->refcount <= 0) {
#if HAVE_FREETYPE_2_CACHE
			FTC_Manager_Reset(freetype2_cache_manager);
#endif
			free(pf->faceid->data.buffer.data);
			free(pf->faceid);
		}
	}

	free(pf);
}


#if 0 /* FIXME Nano-X doesn't have an API for this feature */
static PMWFONT
freetype2_duplicate(PMWFONT psrcfont, MWCOORD height)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) psrcfont;
	PMWFREETYPE2FONT pnewf;

	if (height == 0) {
		height = pf->fontsize;
	}

	if (pf->filename) {
		/* Font from disk file */
		char *filename;
#if HAVE_FREETYPE_2_CACHE
		/* Stored in faceid, which is effectively static.  No need
		 * to copy. */
		filename = pf->filename;
#else
		/* Dynamically allocated, must copy. */
		filename = malloc(1 + strlen(pf->filename));
		if (!filename)
			return NULL;
		strcpy(filename, pf->filename);
#endif
		pnewf = freetype2_createfont_internal(pf->faceid, filename,
						      height);
		if (!pnewf) {
			free(filename);
			return NULL;
		}
	} else {
		pf->faceid->refcount++;

		pnewf = freetype2_createfont_internal(pf->faceid, NULL,
						      height);
		if (!pnewf) {
			pf->faceid->refcount--;
			return NULL;
		}
	}

	GdSetFontAttr((PMWFONT) pnewf, pf->fontattr, 0);

	if (pf->fontrotation)
		GdSetFontRotation((PMWFONT) pnewf, pf->fontrotation);

	return (PMWFONT) pnewf;
}
#endif

static void
freetype2_setfontsize(PMWFONT pfont, MWCOORD fontsize)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

	pf->fontsize = fontsize;

#if HAVE_FREETYPE_2_CACHE
	pf->imagedesc.font.pix_height = fontsize;
	pf->imagedesc.font.pix_width = fontsize;
#else
	/* We want real pixel sizes ... not points ... */
	FT_Set_Pixel_Sizes(pf->face, 0, fontsize);
#endif
}


static void
freetype2_setfontrotation(PMWFONT pfont, int tenthdegrees)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

	/* Normalize so that 0 <= tenthdegrees < 3600 */
	if ((tenthdegrees < 0) || (tenthdegrees >= 3600)) {
		tenthdegrees = tenthdegrees % 3600;
		if (tenthdegrees < 0) {
			tenthdegrees += 3600;
		}
	}

	pf->fontrotation = tenthdegrees;

	if (pf->fontrotation == 0) {
		/* Identity transform */
		pf->matrix.yy = (FT_Fixed) (1 << 16);
		pf->matrix.xx = (FT_Fixed) (1 << 16);
		pf->matrix.yx = (FT_Fixed) 0;
		pf->matrix.xy = (FT_Fixed) 0;
	} else {
		FT_Angle angle = (tenthdegrees << 16) / 10;
		FT_Vector sincosvec;
		FT_Vector_Unit(&sincosvec, angle);
		pf->matrix.yy = sincosvec.y;
		pf->matrix.xx = sincosvec.y;
		pf->matrix.yx = sincosvec.x;
		pf->matrix.xy = -sincosvec.x;
	}
}


static void
freetype2_setfontattr(PMWFONT pfont, int setflags, int clrflags)
{
#if HAVE_FREETYPE_2_CACHE
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

	pf->imagedesc.type = ((pf->fontattr & MWTF_ANTIALIAS)
			      ? ftc_image_grays : ftc_image_mono);
#endif
}


/*
 * Return information about a specified font.
 */
static MWBOOL
freetype2_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;
	FT_Face face;
	FT_Size size;
	FT_BBox *bbox;
	FT_Size_Metrics *metrics;
	int i;

#if HAVE_FREETYPE_2_CACHE
	FT_Error error;

	error = FTC_Manager_Lookup_Size(freetype2_cache_manager,
					&(pf->imagedesc.font), &face, &size);
	if (error) {
		EPRINTF("Freetype 2 error 0x%x getting font info.\n", error);
		return FALSE;
	}
#else
	face = pf->face;
	size = face->size;
#endif

	bbox = &(face->bbox);
	metrics = &(size->metrics);

	if (!FT_IS_SCALABLE(face)) {
		/* FIXME */
		return FALSE;
	}

	/* Fill up the fields */
	pfontinfo->maxwidth = (metrics->max_advance + 63) >> 6;
	pfontinfo->baseline = (metrics->ascender + 63) >> 6;
	pfontinfo->height = pfontinfo->baseline + ((abs(metrics->descender) + 63) >> 6);

	pfontinfo->fixed =
		((face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0);

	//printf("JGF-Nano-X: Font metrics:"
	//    "\n    maxwidth = %3d"
	//    "\n    baseline = %3d        max_ascent   = %3d"
	//    "\n    descent  = %3d        max_descent  = %3d"
	//    "\n    height   = %3d        line_spacing = %3d"
	//    "\n\n",
	//    pfontinfo->maxwidth,
	//    pfontinfo->baseline, pfontinfo->max_ascent,
	//    pfontinfo->descent,  pfontinfo->max_descent,
	//    pfontinfo->height,   pfontinfo->line_spacing
	//    );

	/* FIXME: Following are hacks... */
	pfontinfo->firstchar = 0;
	pfontinfo->lastchar = 0xFFFFU;

#if HAVE_FREETYPE_2_CACHE
	if ((pf->fontrotation != 0) || (pf->fontsize > 200)) {
		/* Cache does not support bitmaps >256x256 */
#endif
		for (i = 0; i < 256; i++) {
			/* FIXME: Should use an image cache, if caching is
			 * enabled.
			 */
			pfontinfo->widths[i] =
				(FT_Load_Char(face, i, FT_LOAD_DEFAULT)
				 ? pfontinfo->maxwidth
				 : ((face->glyph->advance.x +
				     ((1 << 6) - 1)) >> 6));
			//printf("pfontinfo->widths[%d]=%d\n", i, pfontinfo->widths[i]);
		}
#if HAVE_FREETYPE_2_CACHE
	} else {
		int curchar;
		FTC_SBit sbit;

		/* Get bitmaps from cache. */
		for (i = 0; i < 256; i++) {
#if HAVE_FREETYPE_2_CMAP_CACHE
			curchar = FTC_CMapCache_Lookup(freetype2_cache_cmap,
						       &(pf->cmapdesc), i);
#else
			curchar = FT_Get_Char_Index(face, i);
#endif
			error = FTC_SBitCache_Lookup(freetype2_cache_sbit,
						     &(pf->imagedesc),
						     curchar, &sbit, NULL);
			pfontinfo->widths[i] =
				error ? pfontinfo->maxwidth : sbit->xadvance;
		}
	}
#endif
	return TRUE;
}



#if 1
/* Optimized! */
#define freetype2_draw_bitmap_mono(psd,blit_instructions) \
	GdDrawAreaInternal((psd), (blit_instructions), PSDOP_BITMAP_BYTES_MSB_FIRST)

#else

/*
 * Render a simple bitmap.
 *
 * pitch is specified in bytes, and may be negative to
 * indicate that the bitmap is upside down.
 */
static void
freetype2_draw_bitmap_mono(PSD psd, driver_gc_t * blit_instructions)
{
#if 1
	GdDrawAreaInternal(psd, blit_instructions,
			   PSDOP_BITMAP_BYTES_MSB_FIRST);

#elif 0
	int x, y, z, zmax;
	unsigned char v;
	MWPIXELVAL *dst;
	MWPIXELVAL *bitmap;
	int end_of_row_skip;
	unsigned char *src = blit_instructions->pixels;
	int width = blit_instructions->dstw;
	int height = blit_instructions->dsth;
	int pitch = blit_instructions->src_linelen;
	int x_offset = blit_instructions->dstx;
	int y_offset = blit_instructions->dsty;

	bitmap = malloc(width * height * sizeof(MWPIXELVAL));
	if (bitmap == 0) {
		return;
	}

	if (pitch < 0) {
		src = src + (-pitch) * (height - 1);
		y_offset -= height;
	}
	end_of_row_skip = pitch - ((width + 7) >> 3);

	dst = bitmap;

	for (y = 0; y < height; y++) {
		for (x = width; x > 0; x -= 8) {
			v = *src++;
			zmax = (x > 8 ? 8 : x);
			for (z = 0; z < zmax; z++) {
				*dst++ = (v & 0x80) ? gr_foreground :
					gr_background;
				v <<= 1;
			}
		}
		src += end_of_row_skip;
	}

	/* FIXME: GdArea problem if fg == bg */

	/* Now draw the bitmap ... */
	GdArea(psd, x_offset, y_offset, width, height, bitmap, MWPF_PIXELVAL);

	free(bitmap);

#else /* 0 */
	unsigned char *dst;
	unsigned char *imagebits;
	int width_bytes;
	int destpitch;
	int x;
	int y;
	unsigned char *src = blit_instructions->pixels;
	int width = blit_instructions->dstw;
	int height = blit_instructions->dsth;
	int pitch = blit_instructions->src_linelen;
	int x_offset = blit_instructions->dstx;
	int y_offset = blit_instructions->dsty;

	width_bytes = ((width + 7) >> 3);

	/* destpitch is measured in bytes. */
	destpitch =
		(width_bytes +
		 (sizeof(MWIMAGEBITS) - 1)) & ~(sizeof(MWIMAGEBITS) - 1);

	printf("JGF-Nano-X: freetype2_draw_bitmap_mono(): h=%d, w=%d, w_bytes=%d, srcpitch=%d, destpitch=%d\n", height, width, width_bytes, srcpitch, destpitch);

	imagebits = (unsigned char *) malloc(destpitch * height);
	if (imagebits == 0) {
		EPRINTF("Out of memory in freetype2_draw_bitmap_mono()\n");
		return;
	}

	if (pitch < 0) {
		src = src + (-pitch) * (height - 1);
		y_offset -= height;
	}
	dst = imagebits;

	for (y = 0; y < height; y++) {
		for (x = width; x > 0; x -= 8) {
#if 0
			static char s[9];
			s[0] = ((*src & (1 << 7)) ? '1' : '0');
			s[1] = ((*src & (1 << 6)) ? '1' : '0');
			s[2] = ((*src & (1 << 5)) ? '1' : '0');
			s[3] = ((*src & (1 << 4)) ? '1' : '0');
			s[4] = ((*src & (1 << 3)) ? '1' : '0');
			s[5] = ((*src & (1 << 2)) ? '1' : '0');
			s[6] = ((*src & (1 << 1)) ? '1' : '0');
			s[7] = ((*src & (1)) ? '1' : '0');
			s[8] = '\0';
			printf("JGF-Nano-X: freetype2_draw_bitmap_mono(): (%2d,%2d) = %02x  %s\n", x, y, *src, s);
#endif
			*dst++ = *src++;
		}
		src += pitch - width_bytes;
		dst += destpitch - width_bytes;
	}

	printf("JGF-Nano-X: freetype2_draw_bitmap_mono() calling GdBitmap(psd=0x%x, x=%d, y=%d, w=%d, h=%d, bits=0x%x)\n", (int) psd, x_offset, y_offset, width, height, (int) imagebits);
	GdBitmap(psd, x_offset, y_offset, width, height,
		 (MWIMAGEBITS *) imagebits);

	free(imagebits);

#endif
}
#endif /* End of #if 0 around freetype2_draw_bitmap_mono */


/*
 * Draw unicode 16 text string using FREETYPE2 type font
 */
static void
freetype2_drawtext(PMWFONT pfont, PSD psd, MWCOORD ax, MWCOORD ay,
		   const void *text, int cc, MWTEXTFLAGS flags)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;
	FT_Face face;
	FT_Size size;
	const unsigned short *str = text;
	FT_Vector pos;
	int i;
	FT_Glyph glyph;
	FT_Error error;
	FT_Vector kerning_delta;
	int curchar;
	int use_kerning;
	int last_glyph_code = 0;	/* Used for kerning */
	driver_gc_t blit_instructions;

	//printf("JGF-Nano-X: freetype2_drawtext(x=%d, y=%d) called\n", ax, ay);

#if HAVE_FREETYPE_2_CACHE
	error = FTC_Manager_Lookup_Size(freetype2_cache_manager,
					&(pf->imagedesc.font), &face, &size);
	if (error) {
		EPRINTF("Freetype 2 error 0x%x getting font for drawtext.\n",
			error);
		return;
	}
#else
	face = pf->face;
	size = face->size;
#endif

	use_kerning = ((pf->fontattr & MWTF_KERNING) && FT_HAS_KERNING(face));

	/* Initialize the parts of blit_instructions we won't change */
	blit_instructions.fg_color = gr_foreground;
	blit_instructions.bg_color = gr_background;
	blit_instructions.gr_usebg = gr_usebg;
	blit_instructions.srcx = 0;
	blit_instructions.srcy = 0;
	blit_instructions.dst_linelen = 0;	/* Unused. */

	/*
	 * Offset the starting point if necessary,
	 * FreeType always aligns at baseline
	 */
	if (flags & MWTF_BOTTOM) {
		pos.y = (abs(size->metrics.descender) + 63) & ~63;	/* descent */
	} else if (flags & MWTF_TOP) {
		pos.y = -(size->metrics.ascender + 63) & ~63;	/* -ascent */
	} else {
		pos.y = 0;
	}

#if 0				/* Old fake anti-aliasing */
	/* Set the "graylevels" */
	if (pf->fontattr & MWTF_ANTIALIAS) {
		alphablend(psd, freetype2_gray_palette, gr_foreground,
			   gr_background, freetype2_alpha_levels, 16);
	}
#endif

#if 1
	if ((pf->fontrotation != 0)
#if HAVE_FREETYPE_2_CACHE
	    || (pf->fontsize > 200)	/* Cache does not support bitmaps >256x256 */
#endif
		) {
		/* Use slow routine for rotated text */
#endif
		FT_BitmapGlyph bitmapglyph;
		FT_Bitmap *bitmap;
		FT_Render_Mode render_mode =
			(pf->
			 fontattr & MWTF_ANTIALIAS) ? ft_render_mode_normal :
			ft_render_mode_mono;

		/*printf("JGF-Nano-X: freetype2_drawtext() using SLOW routine\n"); */
		pos.x = 0;
		for (i = 0; i < cc; i++) {
#if HAVE_FREETYPE_2_CMAP_CACHE
			curchar = FTC_CMapCache_Lookup(freetype2_cache_cmap,
						       &(pf->cmapdesc),
						       str[i]);
#else
			curchar = FT_Get_Char_Index(face, str[i]);
#endif

			if (use_kerning && last_glyph_code && curchar) {
				FT_Get_Kerning(face, last_glyph_code, curchar,
					       ft_kerning_default,
					       &kerning_delta);

				//printf("JGF-Nano-X: freetype2_drawtext(): kerning_delta.x=%d, /64=%d\n",
				//       (int)kerning_delta.x, (int)kerning_delta.x/64);
				pos.x += kerning_delta.x & (~63);
			}
			last_glyph_code = curchar;

			/* FIXME: Should use an image cache to optimize
			 * rendering of rotated text.
			 */

			error = FT_Load_Glyph(face, curchar, FT_LOAD_DEFAULT);
			if (error)
				continue;

			error = FT_Get_Glyph(face->glyph, &glyph);
			if (error)
				continue;

			// translate the glyph image now..
			FT_Glyph_Transform(glyph, 0, &pos);

			//printf("JGF-Nano-X: freetype2_drawtext(): glyph->advance.x=%d, >>16=%d\n", (int)glyph->advance.x, (int)glyph->advance.x>>16);

			pos.x += (glyph->advance.x >> 10) & (~63);

			if (pf->fontrotation) {
				// rotate the glyph image now..
				FT_Glyph_Transform(glyph, &pf->matrix, 0);
			}
			// convert glyph image to bitmap
			//
			error = FT_Glyph_To_Bitmap(&glyph, render_mode, 0,	// no additional translation
						   1);	// do not destroy copy in "image"
			if (!error) {
				bitmapglyph = (FT_BitmapGlyph) glyph;
				bitmap = &(bitmapglyph->bitmap);

				blit_instructions.dstx =
					ax + bitmapglyph->left;
				blit_instructions.dsty =
					ay - bitmapglyph->top;
				blit_instructions.dstw = bitmap->width;
				blit_instructions.dsth = bitmap->rows;
				blit_instructions.src_linelen = bitmap->pitch;
				blit_instructions.pixels = bitmap->buffer;
				blit_instructions.misc = bitmap->buffer;

				//printf("JGF-Nano-X: freetype2_draw_bitmap_%s(ax=%d, ay=%d, gl->l=%d, gl->t=%d)\n",
				//        ((pf->fontattr & MWTF_ANTIALIAS) ? "alpha" : "mono"), ax, ay, bitmapglyph->left, bitmapglyph->top);

				if ((blit_instructions.dstw > 0)
				    && (blit_instructions.dsth > 0)) {
					if (pf->fontattr & MWTF_ANTIALIAS) {
						GdDrawAreaInternal(psd,
								   &blit_instructions,
								   PSDOP_ALPHACOL);
					} else {
						freetype2_draw_bitmap_mono
							(psd,
							 &blit_instructions);
					}
				}

				FT_Done_Glyph(glyph);
			}
		}
#if 1
	} else {
		/* No rotation - optimized loop */
#if HAVE_FREETYPE_2_CACHE
		FTC_SBit sbit;
#else
		FT_Bitmap *bitmap;
#endif

		ay += (pos.y >> 6);

		for (i = 0; i < cc; i++) {
#if HAVE_FREETYPE_2_CMAP_CACHE
			curchar = FTC_CMapCache_Lookup(freetype2_cache_cmap,
						       &(pf->cmapdesc),
						       str[i]);
#else
			curchar = FT_Get_Char_Index(face, str[i]);
#endif

			if (use_kerning && last_glyph_code && curchar) {
				FT_Get_Kerning(face, last_glyph_code, curchar,
					       ft_kerning_default,
					       &kerning_delta);

				ax += (kerning_delta.x >> 6);
			}
			last_glyph_code = curchar;

#if HAVE_FREETYPE_2_CACHE
			error = FTC_SBitCache_Lookup(freetype2_cache_sbit,
						     &(pf->imagedesc),
						     curchar, &sbit, NULL);

			blit_instructions.dstx = ax + sbit->left;
			blit_instructions.dsty = ay - sbit->top;
			blit_instructions.dstw = sbit->width;
			blit_instructions.dsth = sbit->height;
			blit_instructions.src_linelen = sbit->pitch;
			blit_instructions.pixels = sbit->buffer;
			blit_instructions.misc = sbit->buffer;

			ax += sbit->xadvance;
#else
			error = FT_Load_Glyph(face, curchar,
					      (pf->fontattr & MWTF_ANTIALIAS)
					      ? (FT_LOAD_RENDER |
						 FT_LOAD_MONOCHROME) :
					      FT_LOAD_RENDER);
			if (error)
				continue;

			bitmap = &(face->glyph->bitmap);

			blit_instructions.dstx =
				ax + face->glyph->bitmap_left;
			blit_instructions.dsty = ay - face->glyph->bitmap_top;
			blit_instructions.dstw = bitmap->width;
			blit_instructions.dsth = bitmap->rows;
			blit_instructions.src_linelen = bitmap->pitch;
			blit_instructions.pixels = bitmap->buffer;
			blit_instructions.misc = bitmap->buffer;

			/* Wierdness: After FT_Load_Glyph, face->glyph->advance.x is in 26.6.
			 * After a translation with FT_Glyph_Transform, it is in 16.16.
			 * Must be a FreeType 2.0.8 bug.
			 */
			ax += (face->glyph->advance.x >> 6);
#endif

			if ((blit_instructions.dstw > 0)
			    && (blit_instructions.dsth > 0)) {
				if (pf->fontattr & MWTF_ANTIALIAS) {
					GdDrawAreaInternal(psd,
							   &blit_instructions,
							   PSDOP_ALPHACOL);
				} else {
					freetype2_draw_bitmap_mono(psd,
								   &blit_instructions);
				}
			}

		}
	}
#endif

	GdFixCursor(psd);

// FIXME
//        if (pf->fontattr & MWTF_UNDERLINE)
//                GdLine(psd, startx, starty, x, y, FALSE);
}

static void
freetype2_gettextsize_rotated(PMWFONT pfont, const void *text, int cc,
			      MWCOORD * pwidth, MWCOORD * pheight,
			      MWCOORD * pbase)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;
	FT_Face face;
	FT_Size size;
	const unsigned short *str = text;
	FT_Vector pos;
	int i;
	FT_Glyph glyph;
	FT_Error error;
	FT_Vector kerning_delta;
	int curchar;

	int use_kerning;
	int last_glyph_code = 0;	/* Used for kerning */

	FT_BBox bbox;
	FT_BBox glyph_bbox;

#if HAVE_FREETYPE_2_CACHE
	error = FTC_Manager_Lookup_Size(freetype2_cache_manager,
					&(pf->imagedesc.font), &face, &size);
	if (error) {
		EPRINTF("Freetype 2 error 0x%x getting font info.\n", error);
		*pwidth = 0;
		*pheight = 0;
		*pbase = 0;
		return;
	}
#else
	face = pf->face;
	size = face->size;
#endif

	use_kerning = ((pf->fontattr & MWTF_KERNING) && FT_HAS_KERNING(face));

	bbox.xMin = 0;
	bbox.yMin = 0;
	bbox.xMax = 0;
	bbox.yMax = 0;

	/*
	 * Starting point
	 */
	pos.x = 0;
	pos.y = 0;

	for (i = 0; i < cc; i++) {
#if HAVE_FREETYPE_2_CMAP_CACHE
		curchar = FTC_CMapCache_Lookup(freetype2_cache_cmap,
					       &(pf->cmapdesc), str[i]);
#else
		curchar = FT_Get_Char_Index(face, str[i]);
#endif

		if (use_kerning && last_glyph_code && curchar) {
			FT_Get_Kerning(face, last_glyph_code, curchar,
				       ft_kerning_default, &kerning_delta);

			pos.x += kerning_delta.x & (~63);
		}
		last_glyph_code = curchar;

		/* FIXME: Should use an image cache to optimize
		 * rendering of rotated text.
		 */
		error = FT_Load_Glyph(face, curchar, FT_LOAD_DEFAULT);
		if (error)
			continue;

		error = FT_Get_Glyph(face->glyph, &glyph);
		if (error)
			continue;

		//printf("JGF-Nano-X: freetype2_gettextsize(): glyph '%c' at %d,%d, advance=%d\n",
		//       str[i], (pos.x>>6), (pos.y>>6), (glyph->advance.x >> 16));

		// translate the glyph image now..
		FT_Glyph_Transform(glyph, 0, &pos);

		pos.x += (glyph->advance.x >> 10) & (~63);

		if (pf->fontrotation) {
			// rotate the glyph image now..
			FT_Glyph_Transform(glyph, &pf->matrix, 0);
		}

		if (i == 0) {
			FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &bbox);
		} else {
			FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels,
					  &glyph_bbox);

			//printf("JGF-Nano-X: freetype2_gettextsize(): glyph cbox (%d,%d)-(%d,%d)\n",
			//       (glyph_bbox.xMin/*>>6*/), (glyph_bbox.yMin/*>>6*/),
			//       (glyph_bbox.xMax/*>>6*/), (glyph_bbox.yMax/*>>6*/));

			if (glyph_bbox.xMin < bbox.xMin)
				bbox.xMin = glyph_bbox.xMin;

			if (glyph_bbox.yMin < bbox.yMin)
				bbox.yMin = glyph_bbox.yMin;

			if (glyph_bbox.xMax > bbox.xMax)
				bbox.xMax = glyph_bbox.xMax;

			if (glyph_bbox.yMax > bbox.yMax)
				bbox.yMax = glyph_bbox.yMax;
		}
		FT_Done_Glyph(glyph);

		//printf("JGF-Nano-X: freetype2_gettextsize(): total cbox (%d,%d)-(%d,%d)\n",
		//       (bbox.xMin/*>>6*/), (bbox.yMin/*>>6*/), (bbox.xMax/*>>6*/), (bbox.yMax/*>>6*/));
	}

	*pwidth = bbox.xMax /*>> 6 */ ;
	*pheight = (bbox.yMax - bbox.yMin) /*>> 6 */ ;
	*pbase = -(bbox.yMin /*>> 6 */ );

	//printf("JGF-Nano-X: freetype2_gettextsize(): numchars = %d, w = %d, h = %d, base = %d\n",
	//       cc, *pwidth, *pheight, *pbase);
}


static void
freetype2_gettextsize(PMWFONT pfont, const void *text, int cc,
		      MWCOORD * pwidth, MWCOORD * pheight, MWCOORD * pbase)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;
	if ((pf->fontrotation != 0)
#if HAVE_FREETYPE_2_CACHE
	    || (pf->fontsize > 200)	/* Cache does not support bitmaps >256x256 */
#endif
		) {
		/* Use slow routine for rotated text */
		/* printf("JGF-Nano-X: freetype2_gettextsize() using SLOW routine\n"); */
		freetype2_gettextsize_rotated(pfont, text, cc, pwidth,
					      pheight, pbase);
		return;
	} else {
		FT_Face face;
		FT_Size size;
		const unsigned short *str = text;
		int i;
		int ax;
		int yMin;
		int yMax;
#if HAVE_FREETYPE_2_CACHE
		FTC_SBit sbit;
#else
		FT_BBox glyph_bbox;
#endif
		FT_Error error;
		FT_Vector kerning_delta;
		int curchar;
		int use_kerning;
		int last_glyph_code = 0;	/* Used for kerning */

#if HAVE_FREETYPE_2_CACHE
		error = FTC_Manager_Lookup_Size(freetype2_cache_manager,
						&(pf->imagedesc.font), &face,
						&size);
		if (error) {
			EPRINTF("Freetype 2 error 0x%x getting font info.\n",
				error);
			*pwidth = 0;
			*pheight = 0;
			*pbase = 0;
			return;
		}
#else
		face = pf->face;
		size = face->size;
#endif

		use_kerning = ((pf->fontattr & MWTF_KERNING)
			       && FT_HAS_KERNING(face));


		/*
		 * Starting point
		 */
		ax = 0;
		yMin = 0;
		yMax = 0;

		for (i = 0; i < cc; i++) {
#if HAVE_FREETYPE_2_CMAP_CACHE
			curchar = FTC_CMapCache_Lookup(freetype2_cache_cmap,
						       &(pf->cmapdesc),
						       str[i]);
#else
			curchar = FT_Get_Char_Index(face, str[i]);
#endif

			if (use_kerning && last_glyph_code && curchar) {
				FT_Get_Kerning(face, last_glyph_code, curchar,
					       ft_kerning_default,
					       &kerning_delta);

				/*printf("JGF-Nano-X: freetype2_gettextsize(): %d + kerning %d (delta was %d unscaled).\n",
				   (int)ax, (int)(kerning_delta.x >> 6), (int)kerning_delta.x); */
				ax += (kerning_delta.x >> 6);
			}
			last_glyph_code = curchar;

#if HAVE_FREETYPE_2_CACHE
			error = FTC_SBitCache_Lookup(freetype2_cache_sbit,
						     &(pf->imagedesc),
						     curchar, &sbit, NULL);

			/*printf("JGF-Nano-X: freetype2_gettextsize(): %d + char(%c #%d) %d.\n",
			   (int)ax, (char)(((str[i] >= 32) && (str[i] < 127)) ? str[i] : '?'), (int)str[i], (int)sbit->xadvance); */
			ax += sbit->xadvance;

			if (sbit->top < yMin)
				yMin = sbit->top;

			if (sbit->top + sbit->height > yMax)
				yMax = sbit->top + sbit->height;

#else
			error = FT_Load_Glyph(face, curchar, FT_LOAD_DEFAULT);
			if (error)
				continue;

			ax += (glyph->advance.x >> 16);

			FT_Glyph_Get_CBox(face->glyph, ft_glyph_bbox_pixels,
					  &glyph_bbox);

			if (glyph_bbox.yMin < yMin)
				yMin = glyph_bbox.yMin;

			if (glyph_bbox.yMax > yMax)
				yMax = glyph_bbox.yMax;
#endif
		}

		*pwidth = ax;
		*pheight = (yMax - yMin);
		*pbase = -yMin;

		/*printf("JGF-Nano-X: freetype2_gettextsize(): numchars = %d, w = %d, h = %d, base = %d\n",
		   cc, *pwidth, *pheight, *pbase); */
	}
}


/*
 * Get the human-readable name of a font, given it's filename.
 *
 * Returns NULL if font is invalid or does not contain a name.
 * Otherwise, returns a malloc'd ASCII string.
 */
char *
freetype2_get_name(const char *filename)
{
	FT_Face face;
	char *result = NULL;

	/* Load face */
	if (FT_New_Face(freetype2_library, filename, 0, &face) == FT_Err_Ok) {
		result = malloc(strlen(face->family_name) + 1);
		if (result != NULL) {
			strcpy(result, face->family_name);
		}

		FT_Done_Face(face);
	}

	return result;
}
