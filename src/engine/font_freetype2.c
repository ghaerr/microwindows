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

/*
 * Note on FreeType versions:
 * 2.0.9 - worked (not tested recently)
 * 2.1.0 - not tested
 * 2.1.1 - worked (switch CONFIG_OPTION_USE_CMAPS off if your fonts are
 *         rejected as invalid).
 * 2.1.2 - avoid this release.  Rotations were the wrong way.  There is
 *         no way to test for this and correct it, because this version of
 *         the library mis-reports the version number as 2.1.1.
 * 2.1.3 - works.
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
 * Microwindows font objects, this can also save memory (since
 * there is a single cache with a fixed size shared across
 * the system, but the memory use without caching is
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


/* **************************************************************************/
/* FreeType 2.x                                                             */
/* **************************************************************************/

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

/* Checking FreeType version numbers */

/**
 * Change a major.minor.patch version number into a single number, which
 * is much simpler to compare.
 *
 * If the paramaters are compile-time constants, then the result is also
 * a compile-time constant (and so can be used in #if statements).
 *
 * @param major Version number part
 * @param minor Version number part
 * @param patch Version number part
 * @return Single version number
 */
#define SIMPLIFY_VERSION_NUMBER(major,minor,patch) \
	((major)*0x01000000UL + (minor)*0x01000UL + (patch)*0x01UL)

/**
 * The FreeType version number, in simplified format.
 */
#define FREETYPE_VERSION_NUMBER_SIMPLE \
	SIMPLIFY_VERSION_NUMBER(FREETYPE_MAJOR,FREETYPE_MINOR,FREETYPE_PATCH)

/**
 * TRUE if the FreeType version number is equal to or greater than the
 * specified version.
 *
 * If the paramaters are compile-time constants, then the result is also
 * a compile-time constant (and so can be used in #if statements).
 *
 * @param major Version number part
 * @param minor Version number part
 * @param patch Version number part
 * @return Single version number
 */
#define HAVE_FREETYPE_VERSION_AFTER_OR_EQUAL(major,minor,patch) \
	(FREETYPE_VERSION_NUMBER_SIMPLE >= SIMPLIFY_VERSION_NUMBER(major,minor,patch))


#ifndef FREETYPE_FONT_DIR
/**
 * The default Freetype font directory.
 */
#define FREETYPE_FONT_DIR "/usr/local/microwin/fonts"
#endif

/**
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
#if HAVE_FREETYPE_VERSION_AFTER_OR_EQUAL(2,1,3)
	FTC_ImageTypeRec imagedesc;
#else
	FTC_ImageDesc imagedesc;
#endif
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
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
		MWCOORD *pbase);
static void freetype2_destroyfont(PMWFONT pfont);
static void freetype2_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, MWTEXTFLAGS flags);
static void freetype2_setfontsize(PMWFONT pfont, MWCOORD fontsize);
static void freetype2_setfontrotation(PMWFONT pfont, int tenthdegrees);
static void freetype2_setfontattr(PMWFONT pfont, int setflags, int clrflags);
static PMWFONT freetype2_duplicate(PMWFONT psrcfont, MWCOORD fontsize);


/**
 * The virtual method table for FreeType 2 fonts (i.e. class MWFREETYPE2FONT).
 */
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
	freetype2_duplicate,
};


static PMWFREETYPE2FONT
freetype2_createfont_internal(freetype2_fontdata * faceid,
			      char *filename, MWCOORD height);


/**
 * The freetype library instance - a singleton.
 */
static FT_Library freetype2_library = NULL;

#if HAVE_FREETYPE_2_CACHE
/**
 * The Freetype 2 cache subsystem requires a 1-1 mapping
 * between "void *" pointers and actual font files.  The
 * only almost-sane way to do this is to have a linked list
 * of font file names, and use pointers to the entries
 * in that array.  Font names are added to the list the
 * first time the font is used, and never removed.
 */
static freetype2_fontdata *freetype2_fonts = NULL;

/**
 * The FreeType 2 cache.
 */
static FTC_Manager freetype2_cache_manager;

/**
 * The FreeType 2 cache for glyph bitmaps.
 */
static FTC_SBitCache freetype2_cache_sbit;

#if HAVE_FREETYPE_2_CMAP_CACHE
/**
 * The FreeType 2 cache for charater->glyph mappings.
 */
static FTC_CMapCache freetype2_cache_cmap;
#endif


#if HAVE_FREETYPE_2_CMAP_CACHE
/**
 * Look up a glyph index from a character code.
 * There are two implementations of this macro, which one is used
 * depends on the setting of HAVE_FREETYPE_2_CMAP_CACHE
 *
 * @param pf_   The Microwindows font
 * @param face_ The equivalent FreeType 2 font
 * @param ch_   The character to look up
 * @return      The glyph index.
 */
#define LOOKUP_CHAR(pf_,face_,ch_) \
	(FTC_CMapCache_Lookup(freetype2_cache_cmap, \
	&((pf_)->cmapdesc), \
	(ch_)))

#else
#define LOOKUP_CHAR(pf_,face_,ch_) \
	(FT_Get_Char_Index((face_), (ch_)))
#endif

/**
 * The FreeType 2 sbit cache does not support bitmaps >256x256.
 * For fonts up to and including the following height, use the cache.
 * For larger fonts, do it the slow way.
 *
 * Note: Should probably detect this using the maximum ascent and
 *       maximum descent values.  FIXME.
 */
#define FREETYPE2_CACHE_FONT_SIZE_LIMIT 100

/**
 * Test if a font can use the FreeType 2 cache.
 *
 * To use the cache, there must be no rotation, and the font must be
 * small enough for the bitmaps to be supported (i.e. it must be under
 * FREETYPE2_CACHE_FONT_SIZE_LIMIT).
 */
#define CAN_USE_FT2_CACHE(pf_) \
    (((pf_)->fontrotation == 0) && \
     ((pf_)->fontsize < FREETYPE2_CACHE_FONT_SIZE_LIMIT))


/**
 * Called from the FreeType 2 cache to load a font file.
 *
 * @param face_id The font ID.  This is a pointer to a freetype2_fontdata structure.
 * @param library the FreeType library instance.
 * @param request_data FIXME
 * @param aface FIXME
 */
static FT_Error
freetype2_face_requester(FTC_FaceID face_id,
			 FT_Library library,
			 FT_Pointer request_data, FT_Face * aface)
{
	freetype2_fontdata *fontdata = (freetype2_fontdata *) face_id;	// simple typecast

	assert(fontdata);

	if (fontdata->isBuffer) {
		unsigned char * buffer = fontdata->data.buffer.data;
		unsigned length = fontdata->data.buffer.length;
		/* DPRINTF("Font magic = '%c%c%c%c', len = %u @ freetype2_face_requester\n", 
		   (char)buffer[0], (char)buffer[1],
		   (char)buffer[2], (char)buffer[3],
		   length); */
		assert(buffer);
		return FT_New_Memory_Face(library, buffer, length, 0, aface);
	} else {
		char * filename = fontdata->data.filename;
		/* DPRINTF("Loading font from file '%s' @ freetype2_face_requester\n", 
		   filename); */
		assert(filename);
		return FT_New_Face(library, filename, 0, aface);
	}
}
#endif


/**
 * Initialize the FreeType 2 driver.  If successful, this is a one-time
 * operation. Subsequent calls will do nothing, successfully.
 *
 * @param psd Unused.
 * @return 0 on error, nonzero on success.
 */
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

/**
 * Create a font from a disk file.
 *
 * @param name The font file name or path.  If no directory is specified,
 *             freetype2_font_dir will be prepended.  If no extension is
 *             specified, ".ttf" will be added.
 * @param height The height of the font, in pixels.
 * @param attr The font attributes - a bitmask.
 * @return The new font, or NULL on error.
 */
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
	faceid = freetype2_fonts;
	while ( (faceid != NULL) && (0 != strcmpi(faceid->data.filename, fontname)) )
	{
		faceid = faceid->next;
	}
	if (faceid == NULL) {
		/* Not found in list, so add it. */
		DPRINTF("Nano-X-Freetype2: Adding new font: %s\n", fontname);
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

/**
 * Create a font from a memory buffer.
 *
 * @param buffer The font data.  This will be copied by this function.
 * @param length The length of the font data.
 * @param height The height of the font, in pixels.
 * @return The new font, or NULL on error.
 */
PMWFREETYPE2FONT
freetype2_createfontfrombuffer(const unsigned char *buffer,
			       unsigned length, MWCOORD height)
{
	PMWFREETYPE2FONT pf;
	freetype2_fontdata *faceid = NULL;
	unsigned char *buffercopy;

	assert(buffer);

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

	/*DPRINTF("Font magic = '%c%c%c%c', len = %u @ freetype2_createfontfrombuffer\n", 
	   (char)buffercopy[0], (char)buffercopy[1],
	   (char)buffercopy[2], (char)buffercopy[3],
	   length); */

	pf = freetype2_createfont_internal(faceid, NULL, height);
	if (!pf) {
		free(faceid);
	}

	return pf;
}

/**
 * Finish loading a font.  This is used for both disk-based and
 * memory-based fonts.
 *
 * Allocates the actual PMWFREETYPE2FONT structure, fills it in, and
 * actually loads the font using FreeType 2 to check the font is valid.
 *
 * @param faceid   Information on how to load the font.
 * @param filename The filename, or NULL if loaded from memory.
 * @param height   The font height in pixels.
 * @return The new font, or NULL on error.
 *
 * @internal
 */
static PMWFREETYPE2FONT
freetype2_createfont_internal(freetype2_fontdata * faceid,
			      char *filename, MWCOORD height)
{
	PMWFREETYPE2FONT pf;
#if HAVE_FREETYPE_2_CACHE
	FT_Face face;
	FT_Size size;
#endif
	FT_Error error;

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
#if HAVE_FREETYPE_VERSION_AFTER_OR_EQUAL(2,1,3)
	pf->imagedesc.flags = 0;	/* Will be set by GdSetFontAttr */
#else
	pf->imagedesc.type = 0;	/* Will be set by GdSetFontAttr */
#endif
#if HAVE_FREETYPE_2_CMAP_CACHE
	pf->cmapdesc.face_id = faceid;
	pf->cmapdesc.type = FTC_CMAP_BY_ENCODING;
	pf->cmapdesc.u.encoding = ft_encoding_unicode;
#endif
#else
	/* Load face */
	if (filename) {
		error = FT_New_Face(freetype2_library, filename, 0, &pf->face);
		if (error != FT_Err_Ok) {
			EPRINTF("Nano-X-Freetype2: Can't load font from file \"%s\" - %lx\n",
			        filename, (unsigned long) error);
			goto out;
		}
		/*DPRINTF("Nano-X-Freetype2: Loading font from file \"%s\"\n", filename); */
	} else {
		error = FT_New_Memory_Face(freetype2_library,
		    buffer, length, 0, &pf->face);
		if (error != FT_Err_Ok) {
			EPRINTF("Nano-X-Freetype2: Can't load font from memory - %lx\n",
			    (unsigned long) error);
			goto out;
		}
		/*DPRINTF("Nano-X-Freetype2: Loading font from memory\n"); */
	}

	error = FT_Select_Charmap(pf->face, ft_encoding_unicode);
	if (error != FT_Err_Ok) {
		EPRINTF("freetype2_createfont: no unicode map table - %lx\n",
		    (unsigned long) error);
		goto out;
	}
#endif

	GdSetFontSize((PMWFONT) pf, height);
	GdSetFontRotation((PMWFONT) pf, 0);
	GdSetFontAttr((PMWFONT) pf, 0, 0);

#if HAVE_FREETYPE_2_CACHE
	/* Check that the font file exists and is valid */
	/*DPRINTF("freetype2_createfont_internal(): testing\n");*/
	error = FTC_Manager_Lookup_Size(freetype2_cache_manager,
	    &(pf->imagedesc.font), &face, &size);
	if (error != FT_Err_Ok) {
		EPRINTF("Nano-X-Freetype2: Freetype 2 error %lx trying to load font.\n",
		    (unsigned long)error);
		free(pf);
		return NULL;
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

/**
 * Frees a font.
 *
 * @param pfont The font to free.  Must not be NULL.
 */
static void
freetype2_destroyfont(PMWFONT pfont)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

	assert(pf);

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


/**
 * Duplicates a font.  Makes a new font that has the same face, attributes
 * and rotation as the source font.  The height can be copied or a new
 * height can be specified.
 *
 * If the old font is a memory font, then this function does not copy the
 * actual buffer of font data, it merely adds another reference to it.
 * The reference count ensures that the buffer of font data is freed
 * correctly when the last font using it is freed.
 *
 * @param psrcfont The font to copy.
 * @param height   The height of the new font, in pixels, or 0 to copy from
 *                 the source font.
 * @return A new font, or NULL on error.
 */
static PMWFONT
freetype2_duplicate(PMWFONT psrcfont, MWCOORD height)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) psrcfont;
	PMWFREETYPE2FONT pnewf;

	assert(pf);

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


/**
 * Set the size of a font.  Caller must update pfont->fontsize.
 *
 * @param pfont    The font to update.
 * @param fontsize The new height in pixels.
 */
static void
freetype2_setfontsize(PMWFONT pfont, MWCOORD fontsize)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;
	MWCOORD pixel_height;
	MWCOORD pixel_width;

	assert(pf);

	pf->fontsize = fontsize;

	/* allow create font with height=0*/
	if (!fontsize)
		return;

	/* In future, set these differently to support different aspect ratios. */
	pixel_height = fontsize;
	pixel_width = fontsize;

#if HAVE_FREETYPE_2_CACHE
	pf->imagedesc.font.pix_width  = pixel_width;
	pf->imagedesc.font.pix_height = pixel_height;
#else
	/* We want real pixel sizes ... not points ... */
	FT_Set_Pixel_Sizes(pf->face, pixel_width, pixel_height);
#endif
}


/**
 * Set the rotation of a font.  Caller must update pfont->fontrotation.
 *
 * @param pfont        The font to update.
 * @param tenthdegrees The new rotation in tenths of degrees.
 */
static void
freetype2_setfontrotation(PMWFONT pfont, int tenthdegrees)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

	assert(pf);

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


/**
 * Set the attributes of a font.  Caller must update pfont->fontattr
 * before calling this function.
 *
 * @param pfont    The font to update.
 * @param setflags Bits being set.  Overrides clrflags.
 * @param clrflags Bits being cleared.
 */
static void
freetype2_setfontattr(PMWFONT pfont, int setflags, int clrflags)
{
#if HAVE_FREETYPE_2_CACHE
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

	assert(pf);

#if HAVE_FREETYPE_VERSION_AFTER_OR_EQUAL(2,1,3)
	pf->imagedesc.flags = FT_LOAD_DEFAULT;
	if (!(pf->fontattr & MWTF_ANTIALIAS))
	{
		pf->imagedesc.flags |= (FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO);
	}
#else
	pf->imagedesc.type = ((pf->fontattr & MWTF_ANTIALIAS)
			      ? ftc_image_grays : ftc_image_mono);
#endif
#else
	/* No cache.  Nothing to do, just check paramater is valid. */
	assert(pfont);
#endif
}


#define ROUND_26_6_TO_INT(valuetoround) (((valuetoround) + 63) >> 6)

/**
 * Get the advance width, ascent and descent of a character.
 * Complicated by the need to use the cache if possible, and to handle rotated text.
 *
 * The face and pf arguments must refer to the same font.
 *
 * Any of the output paramater pointers may be NULL if you only care
 * about some of the values.
 *
 * @param pf          The font to use
 * @param face        A FreeType2 face generated from pf
 * @param glyph_index The glyph to measure
 * @param padvance    [out] advance width.
 * @param pascent     [out] character ascent.
 * @param pdescent    [out] character descent.
 * @return            FreeType error code (0 on success).
 */
static FT_Error
freetype2_get_glyph_size(PMWFREETYPE2FONT pf,
                         FT_Face face,
                         int glyph_index,
                         int *padvance,
                         int *pascent,
                         int *pdescent)
{
	FT_Error error;

	assert (pf);
	assert (face);
	
#if HAVE_FREETYPE_2_CACHE
	if (CAN_USE_FT2_CACHE(pf))
	{
		FTC_SBit sbit;

		error = FTC_SBitCache_Lookup(freetype2_cache_sbit,
					     &(pf->imagedesc),
					     glyph_index, &sbit, NULL);
		if (error)
			return error;
		
		/*DPRINTF("sbit->top = %d, sbit->height = %d\n", sbit->top, sbit->height);*/
		
		if (padvance)
			*padvance = sbit->xadvance;
		if (pascent)
			*pascent = sbit->top;
		if (pdescent)
			*pdescent = sbit->height - sbit->top;
		return 0;
	}
	else
#endif
	{
		error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		if (error)
			return error;

		if (padvance)
			*padvance = ROUND_26_6_TO_INT(face->glyph->advance.x);
		if (pascent || pdescent)
		{
			FT_Glyph glyph;
			FT_BBox bbox;
			
			error = FT_Get_Glyph(face->glyph, &glyph);
			if (error)
				return error;
			
			FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &bbox);
			
			FT_Done_Glyph(glyph);
			
			if (pascent)
				*pascent = bbox.yMax;
			if (pdescent)
				*pdescent = -bbox.yMin;
		}
		
		return 0;
	}
}


/**
 * Get the advance width, ascent and descent of a character.
 * Complicated by the need to use the cache if possible, and to handle rotated text.
 *
 * The face and pf arguments must refer to the same font.
 *
 * Any of the output paramater pointers may be NULL if you only care
 * about some of the values.
 *
 * @param pf       The font to use
 * @param face     A FreeType2 face generated from pf
 * @param ch       The character to measure
 * @param padvance [out] advance width.
 * @param pascent  [out] character ascent.
 * @param pdescent [out] character descent.
 * @return         FreeType error code (0 on success).
 */
static FT_Error
freetype2_get_char_size(PMWFREETYPE2FONT pf,
                        FT_Face face,
                        int ch,
                        int *padvance,
                        int *pascent,
                        int *pdescent)
{
	int glyph = LOOKUP_CHAR(pf, face, ch);
	return freetype2_get_glyph_size(pf, face, glyph, padvance, pascent, pdescent);
}


/**
 * Return information about a specified font.
 *
 * @param pfont     The font to query
 * @param pfontinfo The destination for the font metrics information.
 * @return TRUE on success, FALSE on error.
 */
static MWBOOL
freetype2_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;
	FT_Face face;
	FT_Size size;
	FT_BBox *bbox;
	FT_Size_Metrics *metrics;
	int ch;
	int leading;
#if HAVE_FREETYPE_2_CACHE
	FT_Error error;
#endif
#if MW_FEATURE_DO_NOT_TRUST_FONT_ASCENT_AND_DESCENT
	int font_ascent;
	int font_descent;
#endif

	assert(pf);
	assert(pfontinfo);

#if HAVE_FREETYPE_2_CACHE
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
	pfontinfo->maxwidth = ROUND_26_6_TO_INT(metrics->max_advance);

	pfontinfo->maxascent =
		ROUND_26_6_TO_INT(FT_MulFix(bbox->yMax, metrics->y_scale));
	pfontinfo->maxdescent =
		ROUND_26_6_TO_INT(FT_MulFix(-bbox->yMin, metrics->y_scale));

	pfontinfo->fixed =
		((face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0);

	pfontinfo->baseline = ROUND_26_6_TO_INT(metrics->ascender);
	pfontinfo->descent = ROUND_26_6_TO_INT(abs(metrics->descender));
	pfontinfo->height = pfontinfo->baseline + pfontinfo->descent;
	leading = ROUND_26_6_TO_INT(metrics->height - (metrics->ascender + abs(metrics->descender)));
	pfontinfo->linespacing = pfontinfo->height + leading;

	//DPRINTF("Nano-X-Freetype2: Font metrics:"
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

	for (ch = 0; ch < 256; ch++) {
		int advance;
		if (freetype2_get_char_size(pf, face, ch, &advance, NULL, NULL))
		{
			/* Error - assume default */
			pfontinfo->widths[ch] = pfontinfo->maxwidth;
		}
		else
		{
			/* OK, found the value. */
			pfontinfo->widths[ch] = advance;
		}
		//EPRINTF("pfontinfo->widths[%d]=%d\n", i, pfontinfo->widths[i]);
	}

	return TRUE;
}


/**
 * Draws text onto a screen or pixmap.
 *
 * @param pfont The font to use.
 * @param psd   The device to draw on.
 * @param ax    The destination X co-ordinate.
 * @param ay    The destination Y co-ordinate.
 * @param text  The string to display, in 16-bit Unicode form.
 * @param cc    The number of characters (not bytes) in text.
 * @param flags Flags.
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
	int blit_op;

	assert(pf);
	assert(psd);
	assert(text);

	//DPRINTF("Nano-X-Freetype2: freetype2_drawtext(x=%d, y=%d) called\n", ax, ay);

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

	if (pf->fontattr & MWTF_ANTIALIAS) {
		blit_op = PSDOP_ALPHACOL;
	} else {
		blit_op = PSDOP_BITMAP_BYTES_MSB_FIRST;
	}

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

#if 1
	if ((pf->fontrotation != 0)
#if HAVE_FREETYPE_2_CACHE
	    || (!CAN_USE_FT2_CACHE(pf))	/* Cache does not support bitmaps >256x256 */
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

		/*DPRINTF("Nano-X-Freetype2: freetype2_drawtext() using SLOW routine\n"); */
		pos.x = 0;
		for (i = 0; i < cc; i++) {
			curchar = LOOKUP_CHAR(pf, face, str[i]);

			if (use_kerning && last_glyph_code && curchar) {
				FT_Get_Kerning(face, last_glyph_code, curchar,
					       ft_kerning_default,
					       &kerning_delta);

				//DPRINTF("Nano-X-Freetype2: freetype2_drawtext(): kerning_delta.x=%d, /64=%d\n",
				//        (int)kerning_delta.x, (int)kerning_delta.x/64);
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

			//DPRINTF("Nano-X-Freetype2: freetype2_drawtext(): glyph->advance.x=%d, >>16=%d\n", (int)glyph->advance.x, (int)glyph->advance.x>>16);

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

				//DPRINTF("Nano-X-Freetype2: freetype2_draw_bitmap_%s(ax=%d, ay=%d, gl->l=%d, gl->t=%d)\n",
				//        ((pf->fontattr & MWTF_ANTIALIAS) ? "alpha" : "mono"), ax, ay, bitmapglyph->left, bitmapglyph->top);

				if ((blit_instructions.dstw > 0)
				    && (blit_instructions.dsth > 0)) {
					GdDrawAreaInternal(psd, &blit_instructions, blit_op);
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
			curchar = LOOKUP_CHAR(pf, face, str[i]);

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
			if (error)
				continue;

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
				GdDrawAreaInternal(psd, &blit_instructions, blit_op);
			}

		}
	}
#endif

	GdFixCursor(psd);

// FIXME
//        if (pf->fontattr & MWTF_UNDERLINE)
//                GdLine(psd, startx, starty, x, y, FALSE);
}

/**
 * Measures text, which can be rotated.  This is slower than the
 * non-rotated version.
 *
 * @param pf    The font to use.
 * @param text  The string to measure, in 16-bit Unicode form.
 * @param cc    The number of characters (not bytes) in text.
 * @param pwidth  [out] the width in pixels
 * @param pheight [out] the height in pixels
 * @param pbase   [out] the base in pixels
 *
 * @internal
 */
static void
freetype2_gettextsize_rotated(PMWFREETYPE2FONT pf,
                              const void *text, int cc,
                              MWCOORD * pwidth, MWCOORD * pheight,
                              MWCOORD * pbase)
{
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
		curchar = LOOKUP_CHAR(pf, face, str[i]);

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

		//DPRINTF("Nano-X-Freetype2: freetype2_gettextsize(): glyph '%c' at %d,%d, advance=%d\n",
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

			//DPRINTF("Nano-X-Freetype2: freetype2_gettextsize(): glyph cbox (%d,%d)-(%d,%d)\n",
			//        (glyph_bbox.xMin/*>>6*/), (glyph_bbox.yMin/*>>6*/),
			//        (glyph_bbox.xMax/*>>6*/), (glyph_bbox.yMax/*>>6*/));

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

		//DPRINTF("Nano-X-Freetype2: freetype2_gettextsize(): total cbox (%d,%d)-(%d,%d)\n",
		//        (bbox.xMin/*>>6*/), (bbox.yMin/*>>6*/), (bbox.xMax/*>>6*/), (bbox.yMax/*>>6*/));
	}

	*pwidth = bbox.xMax /*>> 6 */ ;
	*pheight = (bbox.yMax - bbox.yMin) /*>> 6 */ ;
	*pbase = -(bbox.yMin /*>> 6 */ );

	//DPRINTF("Nano-X-Freetype2: freetype2_gettextsize(): numchars = %d, w = %d, h = %d, base = %d\n",
	//        cc, *pwidth, *pheight, *pbase);
}


/**
 * Measures text, which must not be rotated.  This is faster than the
 * rotated version.  This uses the cache system, if available, so
 * the caller must check that they CAN_USE_FT2_CACHE().
 *
 * @param pf         The font to use.
 * @param text       The string to measure, in 16-bit Unicode form.
 * @param char_count The number of characters (not bytes) in text.
 * @param pwidth     [out] the width in pixels
 * @param pheight    [out] the height in pixels
 * @param pbase      [out] the base in pixels
 *
 * @internal
 */
static void
freetype2_gettextsize_fast(PMWFREETYPE2FONT pf,
                           const void *text, int char_count,
                           MWCOORD * pwidth, MWCOORD * pheight,
                           MWCOORD * pbase)
{
	FT_Face face;
	FT_Size size;
	const unsigned short *str = text;
	int char_index;
	int total_advance;
	int max_ascent;
	int max_descent;
	int advance;
	int ascent;
	int descent;
	FT_Error error;
	FT_Vector kerning_delta;
	int use_kerning;
	int cur_glyph_code;
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
	total_advance = 0;
	max_ascent  = 0;
	max_descent = 0;

	for (char_index = 0; char_index < char_count; char_index++) {
		cur_glyph_code = LOOKUP_CHAR(pf, face, str[char_index]);

		if (use_kerning && last_glyph_code && cur_glyph_code) {
			FT_Get_Kerning(face, last_glyph_code, cur_glyph_code,
					   ft_kerning_default,
					   &kerning_delta);

			/*EPRINTF("Nano-X-Freetype2: freetype2_gettextsize(): %d + kerning %d (delta was %d unscaled).\n",
			   (int)ax, (int)(kerning_delta.x >> 6), (int)kerning_delta.x); */
			total_advance += (kerning_delta.x >> 6);
		}
		last_glyph_code = cur_glyph_code;

		error = freetype2_get_glyph_size(pf, face, cur_glyph_code, &advance, &ascent, &descent);
		if (error)
			continue;

		total_advance += advance;
		if (max_ascent < ascent)
			max_ascent = ascent;
		if (max_descent < descent)
			max_descent = descent;
	}

	*pwidth = total_advance;
	*pheight = max_ascent + max_descent;
	*pbase = max_ascent;

	/*EPRINTF("Nano-X-Freetype2: freetype2_gettextsize(): numchars = %d, w = %d, h = %d, base = %d\n",
	   cc, *pwidth, *pheight, *pbase); */
}


/**
 * Measures text.
 *
 * @param pfont The font to use.
 * @param text  The string to measure, in 16-bit Unicode form.
 * @param cc    The number of characters (not bytes) in text.
 * @param flags   Flags specifying the encoding of text.
 * @param pwidth  [out] the width in pixels
 * @param pheight [out] the height in pixels
 * @param pbase   [out] the base in pixels
 */
static void
freetype2_gettextsize(PMWFONT pfont, const void *text, int cc,MWTEXTFLAGS flags,
		      MWCOORD * pwidth, MWCOORD * pheight, MWCOORD * pbase)
{
	PMWFREETYPE2FONT pf = (PMWFREETYPE2FONT) pfont;

	assert(pf);
	assert(text);
	assert(pwidth);
	assert(pheight);
	assert(pbase);

	if ((pf->fontrotation != 0)
#if HAVE_FREETYPE_2_CACHE
	    || (!CAN_USE_FT2_CACHE(pf))
#endif
		) {
		/* Use slow routine for rotated text */
		/* EPRINTF("Nano-X-Freetype2: freetype2_gettextsize() using SLOW routine\n"); */
		freetype2_gettextsize_rotated(pf, text, cc, pwidth,
					      pheight, pbase);
	} else {
		freetype2_gettextsize_fast(pf, text, cc, pwidth,
					      pheight, pbase);
	}
}


#if 0 /* FIXME not yet used */
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

	assert(filename);

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
#endif
