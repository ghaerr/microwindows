/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2000 Morten Rolland
 *
 * Device-independent font selection routines
 */
/*#define NDEBUG*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>		/* toupper*/
#include <string.h>

#include "device.h"
#if (UNIX | DOS_DJGPP)
#define strcmpi	strcasecmp
#endif

#if FONTMAPPER

/* entry points*/
int select_font(const PMWLOGFONT plogfont, char *physname);

/*
 * The following structure, defines and variables are used to administrate
 * a set of fonts that are scanned when a font is requested.  The set of
 * fonts can be configured with the GdClearFontList and GdAddFont functions.
 */

struct available_font {
	MWLOGFONT	lf;
	char *foundry;
	char *family;
	char *fontname;
	int fontclass, alias;
	struct available_font *next;
};

static struct available_font *all_fonts = 0;

/*
 * Stupid little function to perform a comparison of two strings,
 * returning one of STREQ_EXACT, STREQ_CASE or STREQ_NOMATCH.
 */
#define STREQ_EXACT	0
#define STREQ_CASE	1
#define STREQ_NOMATCH	2

static int streq(const char *a, const char *b)
{
	int rc;

	for ( rc = STREQ_EXACT;  *a != 0; a++, b++ ) {
		if ( *a == *b )
			continue;
		if ( toupper((unsigned char)*a) != toupper((unsigned char)*b) )
			return STREQ_NOMATCH;
		rc = STREQ_CASE;
	}
	if ( *b != 0 )
		return STREQ_NOMATCH;
	return rc;
}

/*
 * Decode the "foundry" information into a MWLF_CLASS_ value denoting
 * which rendering system should be used to render the font.
 */
static int decode_font_class(const char *class)
{
	if ( class == NULL )
		return 0;
	if ( !strcmp("T1",class) )
		return MWLF_CLASS_T1LIB;
	if ( !strcmp("FT",class) )
		return MWLF_CLASS_FREETYPE;
	if ( !strcmp("MWF",class) )
		return MWLF_CLASS_BUILTIN;
	return 0;
}

/*
 * Try to find a font that matches the foundry and font name
 * requested.
 */
void test_font_naming(const char *foundry, const char *fontname,
		      struct available_font *af,
		      struct available_font **best, int *goodness)
{
	int penalty = 0;

	if ( foundry != 0 ) {
		if ( af->foundry != 0 ) {
			switch ( streq(foundry,af->foundry) ) {
				case STREQ_EXACT:
					break;
				case STREQ_CASE:
					penalty += 2;
					break;
				default:
					penalty += 8;
					break;
			}
		} else {
			penalty += 4;
		}
	}

	switch ( streq(fontname,af->fontname) ) {
		case STREQ_EXACT:
			break;
		case STREQ_CASE:
			penalty += 1;
			break;
		default:
			penalty += 16;
			break;
	}

	if ( *goodness < 0 || penalty < *goodness ) {
		*best = af;
		*goodness = penalty;
	}
}


/*
 * The 'test_font_goodness' function attempts to find how suitable a font is
 * compared to a desired one.  The desired font is specified by:
 *
 *   foundry (can be NULL), family and the MWLOGFONT
 *   structure (can be NULL).
 *
 * If any fonts are configured, one will always be returned from here,
 * but the goodness may be very bad.  The goodness (or similarity)
 * factor is computed by adding up:
 */

#define GOODNESS_NO_FONTNAME		1000	/* No matching font-name found*/
#define GOODNESS_WEIGHT_DIFF		1	/* 0-900 for weight difference*/

#define GOODNESS_NO_FOUNDRY		100	/* No foundry */

#define GOODNESS_FOUNDRY_UNKNOWN	16	/* Foundry unknown */
#define GOODNESS_CASE_FAMILY		8
#define GOODNESS_CASE_FONTNAME		4
#define GOODNESS_CASE_FOUNDRY		2
#define GOODNESS_EXACT_FAMILY		1	/* Matched font family */

#define GOODNESS_EXACT_FOUNDRY		0	/* Clean match */
#define GOODNESS_EXACT_FONTNAME		0	/* Clean match */

#define GOODNESS_FAMILY_UNKNOWN		3000
#define GOODNESS_NOMATCH_FAMILY		10000

#define GOODNESS_EXACT_FONTFAMILY	0
#define GOODNESS_CASE_FONTFAMILY	32
#define GOODNESS_CLOSE_SLANT		128
#define GOODNESS_BAD_SLANT		256
#define GOODNESS_WRONG_SPACING		2048

#define GOODNESS_WRONG_SERIFSTYLE	1024

#define GOODNESS_WANTED_SMALLCAPS_NOT_AVAILABLE		4096
#define GOODNESS_ONLY_SMALLCAPS_AVAILABLE		8192


static int find_foundry_penalty(const struct available_font *have,
			 const char *foundry)
{
	if ( foundry == 0 )
		return 0;

	if ( have->foundry == 0 )
		return GOODNESS_FOUNDRY_UNKNOWN;

	switch ( streq(foundry,have->foundry) ) {
		case STREQ_EXACT:
			return GOODNESS_EXACT_FOUNDRY;
			break;
		case STREQ_CASE:
			return GOODNESS_CASE_FOUNDRY;
			break;
		default:
			return GOODNESS_NO_FOUNDRY;
			break;
	}
}

static int find_family_penalty(const struct available_font *have,
			       const char *family)
{
	if ( family == 0 )
		return 0;

	if ( have->family == 0 )
		return GOODNESS_FAMILY_UNKNOWN;

	switch ( streq(family,have->family) ) {
		case STREQ_EXACT:
			return GOODNESS_EXACT_FAMILY;
			break;
		case STREQ_CASE:
			return GOODNESS_CASE_FAMILY;
			break;
		default:
			return GOODNESS_NOMATCH_FAMILY;
			break;
	}
}

static int find_fontname_penalty(const struct available_font *have,
				 const char *fontname)
{
	switch ( streq(have->fontname,fontname) ) {
		case STREQ_EXACT:
			return GOODNESS_EXACT_FONTNAME;
			break;
		case STREQ_CASE:
			return GOODNESS_CASE_FONTNAME;
			break;
		default:
			break;
	}

	/* Test Fontname against font family name */
	if ( have->family != 0 ) {
		switch ( streq(have->family,fontname) ) {
			case STREQ_EXACT:
				return GOODNESS_EXACT_FONTFAMILY;
				break;
			case STREQ_CASE:
				return GOODNESS_CASE_FONTFAMILY;
				break;
			default:
				/* No suitable fontname found */
				break;
		}
	}
	return GOODNESS_NO_FONTNAME;
}

static int find_weight_penalty(PMWLOGFONT want, PMWLOGFONT have)
{
	int weight_diff;

	weight_diff = want->lfWeight - have->lfWeight;
	if ( weight_diff < 0 )
		weight_diff = -weight_diff;
	return weight_diff * GOODNESS_WEIGHT_DIFF;
}

static int find_slant_penalty(PMWLOGFONT want, PMWLOGFONT have)
{
	/* See if slant is acceptable */

	if ( !want->lfItalic && !want->lfRoman && !want->lfOblique ) {
		/* Try to default to Romans if not specified */
		if ( have->lfItalic || have->lfOblique )
			return GOODNESS_CLOSE_SLANT;
		return 0;
	}

	if ( want->lfItalic && have->lfItalic )
		return 0;

	if ( want->lfRoman && have->lfRoman )
		return 0;

	if ( want->lfOblique && have->lfOblique )
		return 0;

	/* No perfect match for the slant, try "closest" one */

	if ( want->lfItalic && have->lfOblique )
		return GOODNESS_CLOSE_SLANT;

	if ( want->lfOblique && have->lfItalic )
		return GOODNESS_CLOSE_SLANT;

	return GOODNESS_BAD_SLANT;
}

static int find_spacing_penalty(PMWLOGFONT want, PMWLOGFONT have)
{
	if ( want->lfProportional && have->lfProportional )
		return 0;
	if ( want->lfMonospace && have->lfMonospace )
		return 0;
	if ( want->lfMonospace || want->lfProportional )
		return GOODNESS_WRONG_SPACING;

	return 0;	/* No special desires */
}

static int find_serif_penalty(PMWLOGFONT want, PMWLOGFONT have)
{
	if ( !want->lfSerif && !want->lfSansSerif )
		return 0;
	if ( want->lfSerif && have->lfSerif )
		return 0;
	if ( want->lfSansSerif && have->lfSansSerif )
		return 0;

	return GOODNESS_WRONG_SERIFSTYLE;
}

static int find_smallcaps_penalty(PMWLOGFONT want, PMWLOGFONT have)
{
	if ( !want->lfSmallCaps && !have->lfSmallCaps )
		return 0;
	if ( want->lfSmallCaps && have->lfSmallCaps )
		return 0;
	if ( want->lfSmallCaps )
		return GOODNESS_WANTED_SMALLCAPS_NOT_AVAILABLE;
	return GOODNESS_ONLY_SMALLCAPS_AVAILABLE;
}
		

static void test_font_goodness(const char *foundry, const char *family,
			       const char *fontname, const PMWLOGFONT lf,
			       struct available_font *af,
			       struct available_font **best, int *goodness)
{
	int penalty = 0;

	penalty += find_foundry_penalty(af,foundry);
	penalty += find_family_penalty(af,family);

	/* Test Fontname, but only if there is no family */
	if ( family == 0 )
		penalty += find_fontname_penalty(af,fontname);

	if ( lf != 0 ) {
		/* Check logical font attributes */
		penalty += find_weight_penalty(lf,&af->lf);
		penalty += find_slant_penalty(lf,&af->lf);
		penalty += find_spacing_penalty(lf,&af->lf);
		penalty += find_serif_penalty(lf,&af->lf);
		penalty += find_smallcaps_penalty(lf,&af->lf);
	}

	/* See if this font is better than the previous one */
	if ( *goodness < 0 || penalty < *goodness ) {
		/* Yes, this font is better; change to it */
		*best = af;
		*goodness = penalty;
	}
}

static struct available_font *find_suitable_font(const char *foundry,
					       const char *fontname,
					       const PMWLOGFONT plogfont)
{
	struct available_font *af, *best;
	char *family;
	int goodness;

	/*
	 * Try to find a font that matches the name specified as the
	 * desired font (and foundry if possible).  If we find a
	 * suitable font, we will use the family name when trying to
	 * find a font that matches the MWLOGFONT attributes.  This
	 * makes it possible to ask for an Italic version of
	 * "Times Roman" and the expected thing will happen (get the
	 * font "Times Italic").
	 */
	goodness = -1;
	for ( af = best = all_fonts; af != 0; af = af->next ) {
		test_font_naming(foundry,fontname,af,&best,&goodness);
	}
	family = 0;
	if ( goodness != -1 ) {
		/* A font with a name that kind of matched was found,
		 * make a note of its family.  If it has no family, we
		 * can't make any use of the 
		 */
		family = best->family;
	}

	/*
	 * Try to find the closest font that matches the font family
	 * we have established.  If no family was found above, all
	 * fonts will be considered.
	 */
	goodness = -1;
	for ( af = best = all_fonts; af != 0; af = af->next ) {
		test_font_goodness(foundry,family,fontname,plogfont,af,
				   &best,&goodness);
	}
	
	return best;
}


int select_font(const PMWLOGFONT plogfont, char *physname)
{
	struct available_font *font;
	char fndry[128], *foundry;
	const char *fontname;
	char *tmp;
	int t, comma;
	int fontclass = 0;
	int found_font = 0;

	fontname = plogfont->lfFaceName;

	for ( t=0; t < 20; t++ ) {
		/* Only follow 20 aliases deep, assume failure if more ... */

		/* Find foundry for the current font */
		foundry = NULL;
		if ( (tmp = index(fontname,',')) != NULL ) {
			/*
			 * We have a font name like T1,c0934345 or
			 * Adobe,Times (e.g. it includes foundry or
			 * rendering method).  Separate them here.
			 */
			comma = tmp - fontname;
			tmp++;
			strncpy(fndry,fontname,comma);
			fndry[comma] = '\0';
			foundry = fndry;
			fontname = tmp;
		}

		fontclass = decode_font_class(foundry);

		if ( plogfont == NULL && fontclass == 0 )
			fontclass = MWLF_CLASS_BUILTIN;

		if ( fontclass ) {
			/* The font is a "physical" font, use it directly */
			strcpy(physname,fontname);
			return fontclass;
		}

		if ( found_font ) {
			/* Oops, should not get here, unless a font definition
			 * resulted in a non-existent font, e.g. the fontclass
			 * is unknown.
			 */
			goto default_font;
		}

		font = find_suitable_font(foundry,fontname,plogfont);

		if ( font == NULL ) {
			goto default_font;
		}

		if ( !font->alias )
			found_font = 1;

		fontname = font->lf.lfFaceName;
	}

 default_font:

	strcpy(physname, MWFONT_SYSTEM_VAR);
	return MWLF_CLASS_BUILTIN;
}

/* This function can be used to clear the existing, possibly default list
 * of fonts on the system.  This is typically done before reading a
 * configuration file that defines the available fonts.
 */
void
GdClearFontList(void)
{
	struct available_font *font, *next_font;

	font = all_fonts;
	while ( font != 0 ) {
		next_font = font->next;
		if ( font->foundry != 0 )
			free(font->foundry);
		if ( font->family != 0 )
			free(font->family);
		free(font->fontname);
		free(font);
		font = next_font;
	}

	all_fonts = 0;
}

/* This function will add a font to the list of available fonts.
 * The physical name is the name as used by the underlying font
 * rendering engine.
 */
int
GdAddFont(char *fndry, char *fmly, char *fontname, PMWLOGFONT lf,
	unsigned int flags)
{
	struct available_font *font, *walk;
	int fontclass = 0;
	char *physname = lf->lfFaceName;

	if ( !strncmp(physname,"T1,",3) ) {
#ifdef HAVE_T1LIB_SUPPORT
		/* Can handle Type 1 fonts */
		physname += 3;
		fontclass = MWLF_CLASS_T1LIB;
		goto do_font;
#else
		/* Can't handle Type 1 fonts */
		return -1;
#endif
	}

	if ( !strncmp(physname,"FT,",3) ) {
#if HAVE_FREETYPE_SUPPORT
		/* Can handle FreeType fonts */
		physname += 3;
		fontclass = MWLF_CLASS_FREETYPE;
		goto do_font;
#else
		/* Can't handle Type 1 fonts */
		return -1;
#endif
	}

	if ( !strncmp(physname,"MWF,",4) ) {
		/* This is a Microwindows built in font */
		physname += 4;
		fontclass = MWLF_CLASS_BUILTIN;
		goto do_font;
	}

	/* Only aliases does not need to use T1, FT or MWF description */
	if ( !(flags & MWLF_FLAGS_ALIAS) )
		return -1;

 do_font:

	font = malloc(sizeof(*font));
	if ( font == 0 )
		return -1;

	font->foundry = 0;
	if ( strcmp("-",fndry) ) {
		font->foundry = strdup(fndry);
		if ( font->foundry == 0 ) {
			free(font);
			return -1;
		}
	}

	font->family = 0;
	if ( strcmp("-",fmly) ) {
		font->family = strdup(fmly);
		if ( font->family == 0 ) {
			free(font->foundry);
			free(font);
			return -1;
		}
	}

	font->fontname = strdup(fontname);
	if ( font->fontname == 0 ) {
		free(font->foundry);
		free(font->family);
		free(font);
		return -1;
	}

	memcpy(&font->lf,lf,sizeof(*lf));

	printf("Adding font: '%s' '%s' '%s' '%s'\n",font->foundry,
	       font->family,font->fontname,font->lf.lfFaceName);

	font->next = 0;
	font->alias = (flags & MWLF_FLAGS_ALIAS) ? 1 : 0;
	font->fontclass = fontclass;

	/* Stupid append at end of list code */
	if ( all_fonts == 0 ) {
		all_fonts = font;
	} else {
		for ( walk = all_fonts; walk->next != 0; walk = walk->next )
			;
		walk->next = font;
	}

	return 0;
}

/*
 * These functions are used to set attributes in a logical font
 * structure, called through a table of function pointers.
 */
static void font_set_light(PMWLOGFONT lf)
	{ lf->lfWeight = MWLF_WEIGHT_LIGHT; }
static void font_set_regular(PMWLOGFONT lf)
	{ lf->lfWeight = MWLF_WEIGHT_REGULAR; }
static void font_set_medium(PMWLOGFONT lf)
	{ lf->lfWeight = MWLF_WEIGHT_MEDIUM; }
static void font_set_demibold(PMWLOGFONT lf)
	{ lf->lfWeight = MWLF_WEIGHT_DEMIBOLD; }
static void font_set_bold(PMWLOGFONT lf)
	{ lf->lfWeight = MWLF_WEIGHT_BOLD; }
static void font_set_black(PMWLOGFONT lf)
	{ lf->lfWeight = MWLF_WEIGHT_BLACK; }

static void font_set_italic(PMWLOGFONT lf) { lf->lfItalic = 1; }
static void font_set_roman(PMWLOGFONT lf) { lf->lfRoman = 1; }
static void font_set_oblique(PMWLOGFONT lf) { lf->lfOblique = 1; }

static void font_set_normal(PMWLOGFONT lf)
	{ lf->lfPitch = MWLF_PITCH_NORMAL; }
static void font_set_semicondensed(PMWLOGFONT lf)
	{ lf->lfPitch = MWLF_PITCH_SEMICONDENSED; }
static void font_set_condensed(PMWLOGFONT lf)
	{ lf->lfPitch = MWLF_PITCH_CONDENSED; }

static void font_set_serif(PMWLOGFONT lf) { lf->lfSerif = 1; }
static void font_set_sansserif(PMWLOGFONT lf) { lf->lfSansSerif = 1; }
static void font_set_monospace(PMWLOGFONT lf) { lf->lfMonospace = 1; }
static void font_set_proportional(PMWLOGFONT lf) { lf->lfProportional = 1; }

int config_font(char *file, int line, int argc, char *argv[])
{
	unsigned int flags = 0;
	MWLOGFONT lf;
	char tmp[512];
	char *p, *q, *fndry, *family, *fontname;
	int size, t;

	static struct {
		char *name;
		void (*function)(PMWLOGFONT lf);
	} attrs[] = {
		/* Weight */
		{ "Light",		font_set_light },
		{ "Regular",		font_set_regular },
		{ "Medium",		font_set_medium },
		{ "DemiBold",		font_set_demibold },
		{ "Demibold",		font_set_demibold },
		{ "Bold",		font_set_bold },
		{ "Black",		font_set_black },

		/* Slant */
		{ "Italic",		font_set_italic },
		{ "Italics",		font_set_italic },
		{ "Roman",		font_set_roman },
		{ "Oblique",		font_set_oblique },

		/* Width */
		{ "Normal",		font_set_normal },
		{ "Semicondensed",	font_set_semicondensed },
		{ "Condensed",		font_set_condensed },

		/* Class */
		{ "Serif",		font_set_serif },
		{ "Sans-serif",		font_set_sansserif },
		{ "Monospace",		font_set_monospace },
		{ "Proportional",	font_set_proportional },

		{ 0, 0 }
	};

	MWLF_Clear(&lf);

	if ( argc != 6 ) {
		fprintf(stderr,"Bad font description %s:%d\n",file,line);
		return 1;
	}

	if ( !strcmp("alias",argv[1]) ) {
		flags |= MWLF_FLAGS_ALIAS;
		fndry = "-";
	} else {
		fndry = argv[1];
	}

	family = argv[2];
	fontname = argv[3];
	strcpy(lf.lfFaceName,argv[5]);
	p = argv[4];

	while ( *p != '\0' ) {
		/* Parse attributes */
		q = strchr(p,',');
		if ( q != 0 ) {
			size = q - p;
			strncpy(tmp,p,size);
			tmp[size] = '\0';
			p = q + 1;
		} else {
			strcpy(tmp,p);
			p += strlen(tmp);
		}

		for ( t = 0; attrs[t].name != 0; t++ ) {
			if ( !strcmp(attrs[t].name,tmp) ) {
				attrs[t].function(&lf);
				goto next;
			}
		}

		fprintf(stderr,"No such font attribute '%s' in %s:%d\n",
			tmp,file,line);
		return 1;

	next: ;
	}

	GdAddFont(fndry,family,fontname,&lf,flags);

	return 0;
}

/*
 * Handle a single configuration line entery.  Arguments as for
 * function 'main(int argc, char **argv)' -- argv[0] is name of
 * original configuration file.  Return negative value for error,
 * zero for OK.
 */

int config_line(char *file, int line, int argc, char *argv[])
{
	if ( !argc )
		return 0;	/* Empty line */

	if ( argv[0][0] == '#' )
		return 0;	/* Comment line */

	if ( !strcmp("font", argv[0]) )
		return config_font(file,line,argc,argv);

	if ( !strcmp("clear-fonts", argv[0]) ) {
		GdClearFontList();
		return 0;
	}

	return -1;
}


/*
 * Read (one of) the configuration files.
 */
#define MAXCONFIGLINESIZE	1024
#define MAXCONFIGELEMENTS	  64

int read_configfile(char *file)
{
	FILE *cf;
	char buffer[MAXCONFIGLINESIZE+1];
	char *args[MAXCONFIGELEMENTS+1];
	unsigned char *p;
	int argc, s, rc, t, line;

	if ( (cf = fopen(file,"r")) == 0 ) {
		fprintf(stderr,"Unable to read config file '%s'\n",file);
		return -1;
	}

	line = 0;
	while ( !feof(cf) ) {
		if ( fgets(buffer,1000,cf) == 0 )
			break;
		line++;
		s = strlen(buffer) - 1;
		while ( s >= 0 && buffer[s] == '\n' )
			buffer[s--] = '\0';
		p = (unsigned char *)buffer;
		argc = 0;
		for ( t=0; t < MAXCONFIGELEMENTS; t++ ) {
			while ( *p != '\0' && isspace(*p) )
				p++;
			if ( *p == '\"' ) {
				/* Quoted string */
				p++;
				args[t] = p;
				argc++;
				while ( *p != '\0' && *p != '\"' )
					p++;
				if ( *p == '\0' ) {
					fprintf(stderr,"Unbalanced quotes in %s:%d\n",
						file,line);
					break;
				}
				*p++ = '\0';
			} else {
				if ( *p == '\0' )
					break;
				args[t] = p;
				argc++;
				while ( *p != '\0' && !isspace(*p) )
					p++;
				*p++ = '\0';
			}
		}
#if 0
		{
			int t;
			for ( t=0; t < argc; t++ )
				printf("#%d: '%s'\n",t,args[t]);
		}
#endif
		rc = config_line(file, line, argc, args);
		if ( rc < 0 )
			return rc;
	}

	fclose(cf);

	return 0;
}
#endif /* FONTMAPPER*/
