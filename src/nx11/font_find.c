/*
 * Portable Graphics Library - font device independent routines
 * Copyright (c) 2010 by Greg Haerr
 *
 * 5/12/2010 g haerr
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uni_std.h"
#include "nxlib.h"
#include "X11/Xatom.h"

/* fontlist.c*/
extern char *FONT_DIR_LIST[]; 				/* pcf/truetype/type1 font dir list*/
extern nxStaticFontList staticFontList[];	/* static font dir list*/

/* font device-independent routines - font_find.c*/
char *font_findfont(char *name, int height, int width, int *return_height);
int font_findalias(int index, const char *fontspec, char *alias);
char **font_enumfonts(char *pattern, int maxnames, int *count_return, int chkalias);
void font_freefontnames(char **fontlist);

static char **findXLFDfont(char *pattern, int maxnames, int *count, int chkalias);
static FILE * _nxOpenFontDir(char *str);
static void _nxSetDefaultFontDir(void);
static void _nxSetFontDir(char **directories, int ndirs);
static void _nxFreeFontDir(char ***list);

/* nxlib font.c*/
static char **_nxfontlist = NULL;
static int _nxfontcount = 0;

static FILE *
_nxOpenFontDir(char *str)
{
	char path[256];

	sprintf(path, "%s/fonts.dir", str);
	return fopen(path, "r");
}


static FILE *
openfontalias(char *str)
{
	char path[256];

	sprintf(path, "%s/fonts.alias", str);
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		sprintf(path, "%s/fonts.ali", str); //try this for short file name support
		f = fopen(path, "r");
	}
	return f;
}


static void
_nxSetDefaultFontDir(void)
{
	int ndirs;

	/* count number of directories in structure*/
	for (ndirs = 0; FONT_DIR_LIST[ndirs]; ndirs++)
		continue;

	_nxSetFontDir(FONT_DIR_LIST, ndirs);
}

/* make a copy of passed dirlist and store in global vars*/
static void
_nxSetFontDir(char **directories, int ndirs)
{
	int i;

	_nxFreeFontDir(&_nxfontlist);

	_nxfontlist = (char **)calloc(ndirs+1, sizeof(char *));
	for (i = 0; i < ndirs; i++)
		_nxfontlist[i] = strdup(directories[i]);

	_nxfontcount = ndirs;
}

static void
_nxFreeFontDir(char ***addrlist)
{
	char **list = *addrlist;
	int i;

	if (list) {
		for (i = 0; list[i]; i++)
			free(list[i]);
		free(list);
	}

	/* possibly zero globals*/
	if (addrlist == &_nxfontlist) {
		_nxfontlist = 0;
		_nxfontcount = 0;
	}
}

/* nxlib ListFonts.c*/
struct _list {
	char **list;
	int alloc;
	int used;
	struct _list *next;
};

static struct _list *g_fontlist = NULL;

static struct _list *
_createFontList(void)
{
	struct _list *ptr;

	if (!g_fontlist)
		ptr = g_fontlist =
			(struct _list *)calloc(sizeof(struct _list), 1);
	else {
		struct _list *t;

		for (t = g_fontlist; t->next; t = t->next)
			continue;
		ptr = t->next =
			(struct _list *)calloc(sizeof(struct _list), 1);
	}
	return ptr;
}

static int
_addFontToList(struct _list *list, char *font)
{
	if (list->alloc == 0) {
		list->list = malloc(5 * sizeof(char *));
		list->alloc = 5;
	} else if (list->used == list->alloc) {
		list->list = realloc(list->list, (list->alloc + 5) * sizeof(char *));
		list->alloc += 5;
	}
	list->list[list->used++] = strdup(font);

	return list->used;
}

static char **
_getFontList(struct _list *list, int *size)
{
	if (!list->list) {
		*size = 0;
		return NULL;
	}

	if (list->alloc != list->used)
		list->list = realloc(list->list, (list->used) * sizeof(char *));

	*size = list->used;
	return list->list;
}

void
font_freefontnames(char **fontlist)
{
	struct _list *ptr = g_fontlist;
	struct _list *prev = NULL;

	if (!fontlist)
		return;

	while (ptr) {
		if (ptr->list == fontlist) {
			int i;
			for (i = 0; i < ptr->used; i++)
				free(ptr->list[i]);

			free(ptr->list);

			if (ptr == g_fontlist)
				g_fontlist = ptr->next;
			else
				prev->next = ptr->next;

			return;
		}
		prev = ptr;
		ptr = ptr->next;
	}
}

static int
dashcount(char *name)
{
	int	ndashes = 0;

	while (*name)
		if (*name++ == '-')
			++ndashes;
	return ndashes;
}

static int
patternmatch(char *pat, int patdashes, char *string, int stringdashes)
{
	int c, t;

	if (stringdashes < patdashes)
		return 0;

	for (;;) {
	    switch (c = *pat++) {
	    case '*':
			if (!(c = *pat++))
				return 1;

			if (c == '-') {
				patdashes--;
				for (;;) {
					while ((t = *string++) != '-')
						if (!t)
							return 0;
					stringdashes--;
					if (patternmatch(pat, patdashes, string, stringdashes))
						return 1;
					if (stringdashes == patdashes)
						return 0;
				}
			} else {
				for (;;) {
					while ((t = *string++) != c) {
						if (!t)
							return 0;
						if (t == '-') {
							if (stringdashes-- < patdashes)
								return 0;
						}
					}
					if (patternmatch(pat, patdashes, string, stringdashes))
						return 1;
				}
			}
			break;
	    case '?':
			if (*string++ == '-')
				stringdashes--;
			break;
	    case '\0':
			return (*string == '\0');
	    case '-':
			if (*string++ == '-') {
				patdashes--;
				stringdashes--;
				break;
			}
			return 0;
	    default:
			if (c == *string++)
				break;
			return 0;
	    }
	}
}

static int
match(char *pat, char *string)
{
	return patternmatch(pat, dashcount(pat), string, dashcount(string));
}

/*
 * Search font directory fonts.dir files and return list of XLFD's that match wildchars.
 * fonts.alias is not used with this function.
 */
static void
findfont_wildcard(char *pattern, int maxnames, struct _list *fontlist)
{
	int f, i;

	DPRINTF("findfont_wildcard: '%s' maxnames %d\n", pattern, maxnames);
	/* loop through each font dir and read fonts.dir*/
	for (f = 0; f < _nxfontcount; f++) {
		FILE *fontdir = _nxOpenFontDir(_nxfontlist[f]);
		int fcount;
		char buffer[128];

		DPRINTF("nxopenfontdir %s\n", _nxfontlist[f]);
		if (!fontdir)
			continue;

		/* get fonts.dir linecount*/
		if (!fgets(buffer, sizeof(buffer), fontdir) || !(fcount = atoi(buffer))) {
			fclose(fontdir);
			continue;
		}

		/* read XLFD and add to list if matches wildcard pattern*/
		for (i = 0; i < fcount; i++) {
			char *xlfd;

			if (!(fgets(buffer, sizeof(buffer), fontdir)))
				continue;
			buffer[strlen(buffer) - 1] = '\0';

			/* XLFD is second field*/
			xlfd = strchr(buffer, ' ');
			if (!xlfd)
				continue;
			*xlfd++ = '\0';

			/* if XLFD matches pattern, add to fontlist*/
			if (match(pattern, xlfd)) {
				DPRINTF("enumfont add: %s\n", xlfd);
				if (_addFontToList(fontlist, xlfd) == maxnames)
					break;
			}
		}
		fclose(fontdir);
	}

#if ANDROID // FIXME broken, segfaults in strlen code below
	//Android has no fonts dir so emulated this here - so far no fontlist was found above
	if (maxnames == 1) {
	  _addFontToList(fontlist, pattern); //FLTK just checks again if selected size is supported
	} else {
	  pattern[strlen(pattern) - 1] = '\0'; //cut off last *
	  char * pch;
	  pch = strstr (pattern,"-*-courier-");
	  if (pch) {
	    strcat(pattern,"0-0-0-0-m-0-iso8859-1"); //m= monospaced
	  } else {
	    strcat(pattern,"0-0-0-0-p-0-iso8859-1"); //size=0 means all sizes available
	  }
	  _addFontToList(fontlist, pattern);
	}
#endif

#if HAVE_STATICFONTS
	for (i = 0; staticFontList[i].file; i++) {
		if (match(pattern, staticFontList[i].xlfd))
			if (_addFontToList(fontlist, staticFontList[i].xlfd) == maxnames)
				break;
	}
#endif
}


/*
 * Compare two strings just like strcmp, but preserve decimal integer
 * sorting order, i.e. "2" < "10" or "iso8859-2" < "iso8859-10" <
 * "iso10646-1". Strings are sorted as if sequences of digits were
 * prefixed by a length indicator (i.e., does not ignore leading zeroes).
 *
 * Markus Kuhn <Markus.Kuhn@cl.cam.ac.uk>
 */
#define isdigit(c) ('0' <= (c) && (c) <= '9')
static int
strcmpn(unsigned char *s1, unsigned char *s2)
{
	int digits, predigits = 0;
	unsigned char *ss1, *ss2;

	while (1) {
		if (*s1 == 0 && *s2 == 0)
			return 0;
		digits = isdigit(*s1) && isdigit(*s2);
		if (digits && !predigits) {
			ss1 = s1;
			ss2 = s2;
			while (isdigit(*ss1) && isdigit(*ss2))
				ss1++, ss2++;
			if (!isdigit(*ss1) && isdigit(*ss2))
				return -1;
			if (isdigit(*ss1) && !isdigit(*ss2))
				return 1;
		}
		if (*s1 < *s2)
			return -1;
		if (*s1 > *s2)
			return 1;
		predigits = digits;
		s1++, s2++;
	}
}

static int
comparefunc(const void* a, const void* b)
{
	unsigned char *aa = *(unsigned char **)a;
	unsigned char *bb = *(unsigned char **)b;

	return strcmpn(aa, bb);
}

/* enumerate fonts matching passed XLFD pattern*/
char **font_enumfonts(char *pattern, int maxnames, int *count_return, int chkalias)
{
	char **fontlist;
	int count;

	DPRINTF("font_enumfonts: '%s' maxnames %d\n", pattern, maxnames);
	fontlist = findXLFDfont(pattern, maxnames, &count, chkalias);
	*count_return = count;

	/* sort the return, helps for lack of locale info at end of XLFD*/
	if (fontlist)
		qsort((char *)fontlist, count, sizeof(char *), comparefunc);

	DPRINTF("font_enumfonts: return count %d\n", count);
	return fontlist;
}

/* nxlib LoadFont.c*/
static int
prefix(const char *prestr, char *allstr)
{
	while (*prestr)
		if (*prestr++ != *allstr++)
			return 0;
	if (*allstr && *allstr != '.')
		return 0;
	return 1;
}

static int
any(int c, const char *str)
{
	while (*str)
		if (*str++ == c)
			return 1;
	return 0;
}

/* check if passed fontspec is aliased in fonts.alias file*/
int
font_findalias(int index, const char *fontspec, char *alias)
{
	FILE *aliasfile;
	int match = 0;
	char *p;
	char buffer[256];

	/* loop through each line in the fonts.alias file for match*/
	aliasfile = openfontalias(_nxfontlist[index]);
	if (aliasfile) {
		for (;;) {
			if (!fgets(buffer, sizeof(buffer), aliasfile))
				break;
			buffer[strlen(buffer) - 1] = '\0';

			/* ignore blank and ! comments*/
			if (buffer[0] == '\0' || buffer[0] == '!')
				continue;

			/* fontname is first space separated field*/
			/* check for tab first as filename may have spaces*/
			p = strchr(buffer, '\t');
			if (!p)
				p = strchr(buffer, ' ');
			if (!p)
				continue;
			*p = '\0';

			/* check exact match*/
			if (strcmp(fontspec, buffer) == 0) {

				/* alias is second space separated field*/
				do ++p; while (*p == ' ' || *p == '\t');

				strcpy(alias, p);
				DPRINTF("font_findalias: replacing %s with %s\n", fontspec, alias);
				match = 1;
				break;
			}
		}
		fclose(aliasfile);
	}

	return match;
}

/* return height component of XLFD: ...--height-...*/
static int
xlfdheight(const char *xlfd)
{
	int dashcount = 0;
	int height = 0;

	while (*xlfd && dashcount < 8) {
		if (*xlfd++ == '-') {
			if (++dashcount == 7) {
				while (*xlfd >= '0' && *xlfd <= '9')
					height = height * 10 + (*xlfd++ - '0');
				break;
			}
		}
	}
	return height;
}

/*
 * Search font directory fonts.dir files and return full font pathname matching
 * fontspec, no wildcards allowed.
 *
 * If fonts.dir does not exist, fontspec tried directly as a filename.
 *
 * If fontspec is XLFD:
 * Each fonts.dir line is checked for exact match.
 * If no match is found, then -0- is substituted for passed XLFD
 * height and match attempted (will return only -0- scaleable font in this case).
 * Height is updated based on matched XLFD height.
 *
 * If fontspec is not XLFD and has no '/' path chars, then it is assumed to
 * be a prefix of a font filename (excluding .gz or extension) and each
 * fonts.dir line is checked for a match.
 * Height is not returned in this case.
 *
 * If no match is found, NULL is returned.
 *
 * This routine calls strdup() for return value, caller must free after use.
 */ 
static char *
findfont_nowildcard(const char *fontspec, int *height)
{
	int i, f;
	char buffer[128];
	char path[256];

	if (!_nxfontcount)
		_nxSetDefaultFontDir();

	DPRINTF("findfont_nowildcard: '%s'\n", fontspec);

	/* set zero return height for cases where we don't have info*/
	*height = 0;

	/* first check for whole path specfied.  If so, return as is*/
	if (fontspec[0] == '/')
		return strdup(fontspec);

	/* loop through each font dir and read fonts.dir*/
	for (f = 0; f < _nxfontcount; f++) {
		FILE *fontdir = _nxOpenFontDir(_nxfontlist[f]);
		int fcount;

		/*
		 * If no fonts.dir file, check fontspec as filename.
		 * This allows .ttf files to be found in typical font directory
		 * installations for non-X11/XLFD fonts.
		 */
		if (!fontdir) {
			sprintf(path, "%s/%s", _nxfontlist[f], fontspec);
			if (access(path, F_OK) == 0) {
				DPRINTF("findfont_nowild: partial path match %s = %s\n", fontspec, path);
				return strdup(path);
			}

			/* no match, continue with next font directory and start all over*/
			continue;
		}

		/* get fonts.dir linecount*/
		if (!fgets(buffer, sizeof(buffer), fontdir) || !(fcount = atoi(buffer))) {
			fclose(fontdir);
			continue;
		}

		if (fontspec[0] == '-') {
			/* Fontspec is XLFD: loop throuch each line in the fonts.dir file for XLFD match*/
			for (i = 0; i < fcount; i++) {
				char *xlfd;

				/* font filename is first field*/
				if (!fgets(buffer, sizeof(buffer), fontdir))
					continue;
				buffer[strlen(buffer) - 1] = '\0';

				/* XLFD is second field*/
				xlfd = strchr(buffer, ' ');
				if (!xlfd)
					continue;
				*xlfd++ = '\0';

				/* if XLFD matches fontspec, return full font pathname*/
				if (strcmp(fontspec, xlfd) == 0) {

					/* return full font pathname and height*/
					sprintf(path, "%s/%s", _nxfontlist[f], buffer);
					*height = xlfdheight(xlfd);
					DPRINTF("findfont_nowild: exact XLFD match %s %s = %s (%d)\n",
						fontspec, xlfd, path, *height);
					fclose(fontdir);
					return strdup(path);
				} else {
					int j;
					int dashcount = 0;
					int len;

					/* no exact match, check for match with -0- height*/
					if (xlfdheight(xlfd) != 0)
						continue;		/* not scaleable, try next fonts.dir line*/

					/* otherwise check for scaleable font match spec
				 	 * with passed pixel size, that is:
				 	 *     match XLFD  "...normal--0-0-0-0-0-..."
				 	 * with passed     "...normal--12-0-0-0-0-..."
				 	 * for height 12.
				 	 */
					len = MWMIN(strlen(xlfd), strlen(fontspec));

					/* match before and after height at '--0-' in XLFD string*/
					for (j = 0; j < len && dashcount < 8; j++) {
						if (xlfd[j] == '-')
							dashcount++;
						if (xlfd[j] != fontspec[j]) {
							if (dashcount == 7 && xlfd[j] == '0') {
								int st = j;

								/* pass over passed height*/
								while (fontspec[j] >= '0' && fontspec[j] <= '9')
									j++;

								/* and check that rest of XLFD line matches*/
								if (strcmp(&fontspec[j], &xlfd[st+1]) == 0) {

									/* match - return full font pathname and height*/
									sprintf(path, "%s/%s", _nxfontlist[f], buffer);
									*height = xlfdheight(fontspec);
									DPRINTF("findfont_nowild: XLFD -0- match %s %s = '%s' height %d\n", fontspec, xlfd, path, *height);
									fclose(fontdir);
									return strdup(path);
								}
							}
							break;
						}
					}
				}
			}
		} else {	/* fontspec[0] != '-'*/
			/*
		 	 * Fontspec is not XLFD.  Loop through each fonts.dir line and look
		 	 * for fontspec being a prefix of the font filename.
		 	 */
			for (i = 0; i < fcount; i++) {
				char *p;

				/* font filename is first field*/
				if (!fgets(buffer, sizeof(buffer), fontdir))
					continue;
				buffer[strlen(buffer) - 1] = '\0';
				p = strchr(buffer, ' ');
				if (!p)
					continue;
				*p = '\0';

				/* prefix allows font.pcf to match font.pcf.gz for example*/
				if (prefix(fontspec, buffer)) {
					char path[256];

					/* return full font pathname*/
					sprintf(path, "%s/%s", _nxfontlist[f], buffer);
					DPRINTF("findfont_nowild: non-XLFD prefix match %s %s = '%s'\n",
						fontspec, buffer, path);
					fclose(fontdir);
					return strdup(path);
				}
			}
		}

		if (fontdir)
			fclose(fontdir);
	}

#if HAVE_STATICFONTS
		/* repeat code from above but search static font list for match*/
		if (fontspec[0] == '-') {
			/* Fontspec is XLFD: loop throuch each line staticFontList for XLFD match*/
			for (i=0; staticFontList[i].file; i++) {
				char *xlfd = staticFontList[i].xlfd;

				/* if XLFD matches fontspec, return full font pathname*/
				if (strcmp(fontspec, xlfd) == 0) {

					*height = xlfdheight(xlfd);
					DPRINTF("findfont_nowild: exact XLFD match %s %s = %s (%d)\n",
						xlfd, fontspec, staticFontList[i].file, *height);
					return strdup(staticFontList[i].file);
				} else {
					int j;
					int dashcount = 0;
					int len;

					/* no exact match, check for match with -0- height*/
					if (xlfdheight(xlfd) != 0)
						continue;		/* not scaleable, try next fonts.dir line*/

					/* otherwise check for scaleable font match spec
				 	 * with passed pixel size, that is:
				 	 *     match XLFD  "...normal--0-0-0-0-0-..."
				 	 * with passed     "...normal--12-0-0-0-0-..."
				 	 * for height 12.
				 	 */
					len = MWMIN(strlen(xlfd), strlen(fontspec));

					/* match before and after height at '--0-' in XLFD string*/
					for (j = 0; j < len && dashcount < 8; j++) {
						if (xlfd[j] == '-')
							dashcount++;
						if (xlfd[j] != fontspec[j]) {
							if (dashcount == 7 && xlfd[j] == '0') {
								int st = j;

								/* pass over passed height*/
								while (fontspec[j] >= '0' && fontspec[j] <= '9')
									j++;

								/* and check that rest of XLFD line matches*/
								if (strcmp(&fontspec[j], &xlfd[st+1]) == 0) {

									/* match - return full font pathname and height*/
									*height = xlfdheight(fontspec);
									DPRINTF("findfont_nowild: exact XLFD match %s %s = %s (%d)\n", xlfd, fontspec, staticFontList[i].file, *height);
									return strdup(staticFontList[i].file);
								}
							}
							break;
						}
					}
				}
			}
		}
#endif /* HAVE_STATICFONTS*/

	/* check for partial path specfied.  If so, return as is*/
	if (any('/', fontspec)) {
		DPRINTF("findfont_nowild: partial path assumed, returning %s\n", fontspec);
		return strdup(fontspec);
	}

		DPRINTF("findfont_nowild: fail\n");
	return NULL;
}

/*
 * Search font directory fonts.dir files and return list of XLFD's that match with wildchars,
 * or return exact one that matches without wildchars.
 * For each font dir, fonts.alias is checked for matching pattern and replaced
 */
static char **
findXLFDfont(char *pattern, int maxnames, int *count, int chkalias)
{
	struct _list *fontlist = _createFontList();
	int f;

	if (!_nxfontlist)
		_nxSetDefaultFontDir();

	/* loop through each font dir and read fonts.alias*/
	if (chkalias) {
		for (f = 0; f < _nxfontcount; f++) {
			char alias[256];

			/* rewrite pattern if aliased and start over*/
			//FIXME infinite recursion!
			if (font_findalias(f, pattern, alias))
				return findXLFDfont(alias, maxnames, count, chkalias);
		}
	}

	if (pattern[0] == '-' && (any('*', pattern) || any('?', pattern))) {
		findfont_wildcard(pattern, maxnames, fontlist);
	} else {
		char *fontfile;
		int dummy_height;

		/* check for no wildcard match*/
		fontfile = findfont_nowildcard(pattern, &dummy_height);

		/* special case handling for 'fixed' - return 'fixed'*/
		if (!fontfile && !strcmp(pattern, "fixed")) {
			DPRINTF("XListFont: forcing add 'fixed' to list\n");
			fontfile = strdup("fixed");		/* faked to force add pattern to list*/
		}

		if(fontfile) { 			/* there's a perfect match */
			free(fontfile); 	/* free because of strdup */
			_addFontToList(fontlist, pattern);
		}
	}
	return _getFontList(fontlist, count);
}

#if HAVE_STATICFONTS
/* search static font list for matching font name*/
int
font_findstaticfont(char *fontname, unsigned char** data, int* size) {
	int i;

	for (i = 0; staticFontList[i].file; i++) {
		if (!strcmp(staticFontList[i].file,fontname)) {
			if (staticFontList[i].data) {
				*data = staticFontList[i].data;
				*size = staticFontList[i].data_size;
				return 1;
			}
		}
	}
	return 0;
}
#endif

/*
 * Return font full pathname based on XLFD or filename spec.
 * For each font dir, fonts.alias is checked for matching fontspec and replaced
 *
 * If an XLFD is specified with wildcards, all XLFD that match are examined
 * and a font returned whose height is closest but <= to passed height returned.
 *
 * If wildcards were not specified, and a match is not found, then scaleable XLFD's
 * matching '-0-' for height are checked also.
 *
 * If an XLFD was not specified, a partial filename is assumed.
 * If fonts.dir does not exist, directory is checked directly for font existence.
 * If the path seperator '/' is included, no further modifications are done.
 * Otherwise, each fonts.dir line is checked for prefixes (without extension)
 * of the filename for a match.
 *
 * If no font is loaded, NULL is returned.
 */
char *
font_findfont(char *name, int height, int width, int *return_height)
{
	char *	   fontpath = NULL;
	char **	   fontlist = NULL;
	int f;

	DPRINTF("findfont: start %s h/w %d,%d\n", name, height, width);
	if (!_nxfontcount)
		_nxSetDefaultFontDir();

	/* loop through each font dir and read fonts.alias*/
	for (f = 0; f < _nxfontcount; f++)
	{
		char alias[256];

		/* rewrite fontspec if aliased and start over*/
		//FIXME infinite recursion!
		if (font_findalias(f, name, alias))
			return font_findfont(alias, height, width, return_height);
	}

	/* check if XLFD wildcard specified and enumerate XLFD fonts if so*/
	if (name[0] == '-' && (any('*', name) || any('?', name))) {
		int count;

		/* enumerate XLFD fonts matching spec*/
		fontlist = font_enumfonts(name, 10000, &count, 0);
		if (fontlist) {
			int i, okindex = 0;
			for (i = 0; i < count; ++i) {
				/* find tallest font less than or equal to requested pixel height*/
				int h = xlfdheight(fontlist[i]);

				if (h <= height)
					okindex = i;
			}

			/* return pathname of XLFD font spec*/
			fontpath = findfont_nowildcard(fontlist[okindex], return_height);
			DPRINTF("findfont: fini wild %s %d %d = '%s' height %d\n",
				fontlist[okindex], okindex, count, fontpath, *return_height);
		}
	} else {
		/*
		 * Wildcard not specified, search fonts.alias and fonts.dir files.
		 * Fontspec is either XLFD or filename prefix.
		 *
		 * If XLFD, find exact match or match with -0- height.
		 * If not XLFD search for filename prefix.
		 */
		fontpath = findfont_nowildcard(name, return_height);

		/* set height info to reasonable value for aliased non-XLFD cases*/
		if (*return_height == 0)
			*return_height = (height? height: 13);

		DPRINTF("findfont: FINI nowild %s = %s height %d\n", name, fontpath, *return_height);
	}

	/* free temporaries*/
	if (fontlist)
		font_freefontnames(fontlist);

	return fontpath;
}

/* non-portable NXLIB/X11 implementation follows*/

#if ANDROID
/* 
 * convert helvetica to Roboto, times to Georgia and use Courier for all
 * disregarding italic and bold
 */
Font
android_create_font_alias(const char *name)
{
  	GR_FONT_ID font = 0;
	int height;
	char * pch;
	char fontnamestring[120];

	height=xlfdheight(name); //read height from XLFD description received

        pch = strstr (name,"-helvetica-medium-r-normal");
	if (pch) {
	  sprintf(fontnamestring,"Roboto-Regular");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-helvetica-medium-o-normal");
	if (pch) {
	  sprintf(fontnamestring,"Roboto-Italic");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-helvetica-bold-r-normal");
	if (pch) {
	  sprintf(fontnamestring,"Roboto-Bold");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-helvetica-bold-o-normal");
	if (pch) {
	  sprintf(fontnamestring,"Roboto-BoldItalic");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}

	pch = strstr (name,"-times-medium-r-normal");
	if (pch) {
	  sprintf(fontnamestring,"Georgia-Regular");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-times-medium-o-normal");
	if (pch) {
	  sprintf(fontnamestring,"Georgia-Italic");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-times-bold-r-normal");
	if (pch) {
	  sprintf(fontnamestring,"Georgia-Bold");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-times-bold-o-normal");
	if (pch) {
	  sprintf(fontnamestring,"Georgia-BoldItalic");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
//android has just one courier font
	pch = strstr (name,"-courier-medium-r-normal");
	if (pch) {
	  sprintf(fontnamestring,"courier");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-courier-medium-o-normal");
	if (pch) {
	  sprintf(fontnamestring,"courier");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-courier-bold-r-normal");
	if (pch) {
	  sprintf(fontnamestring,"courier");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
        pch = strstr (name,"-courier-bold-o-normal");
	if (pch) {
	  sprintf(fontnamestring,"courier");
	  font = GrCreateFontEx(fontnamestring, height, height, NULL);
	}
	
	GrSetFontAttr(font, GR_TFKERNING | GR_TFANTIALIAS, 0);

	DPRINTF("android_create_font_alias(xlfd name:'%s') = aliased fontnamestring used: '%s' height %d font-id: [%d]\n", name, fontnamestring, height, font);

  return font;
}
#endif /* ANDROID*/

/* old LoadFont.c*/
Font
XLoadFont(Display * dpy, _Xconst char *name)
{
 	GR_FONT_ID font = 0;
	int height;
	char *fontname;

#if ANDROID
        return android_create_font_alias(name);
#endif

	DPRINTF("XLoadFont('%s')\n", name);
	/* first try to find XLFD or fontname from X11/fonts.dir and fonts.alias files*/
	fontname = font_findfont((char *)name, 0, 0, &height);
	if(fontname) {
#if HAVE_STATICFONTS
		int size;
		unsigned char *data;

		/* check static font list*/
		if (font_staticGetBufferInfo(fontname, &data, &size))
			font = GrCreateFontFromBuffer(data, size, "TTF", height, height);
		else
#endif
 		font = GrCreateFontEx(fontname, height, height, NULL);
	} else if (!strcmp(name, "fixed")) {
		DPRINTF("XLoadFont: mapping 'fixed' to builtin SystemFixed\n");
		/* special-case handle 'fixed' and map to builtin system fixed font*/
		fontname = strdup(MWFONT_SYSTEM_FIXED);
		height = 13;
		font = GrCreateFontEx(fontname, 0, 0, NULL); /* height zero to force builtin lookup by name*/
	}
    GrSetFontAttr(font, GR_TFKERNING | GR_TFANTIALIAS, 0);
	DPRINTF("XLoadFont('%s') = '%s' height %d [%d]\n", name, fontname, height, font);
	if (fontname)
		Xfree(fontname);
	return font;
}

/* old ListFonts.c*/
char **
XListFonts(Display * display, _Xconst char *pattern, int maxnames, int *actual_count_return)
{
	return font_enumfonts((char *)pattern, maxnames, actual_count_return, 1);
}

int
XFreeFontNames(char **list)
{
	font_freefontnames(list);
	return 1;
}

/* old SetPath.c*/
static char **
_nxCopyFontDir(int *count)
{
	int i;
	char **list;

	if (!_nxfontcount)
		_nxSetDefaultFontDir();

	list = (char **)Xcalloc(_nxfontcount+1, sizeof(char *));

	for (i = 0; i < _nxfontcount; i++)
		list[i] = strdup(_nxfontlist[i]);
	*count = _nxfontcount;

	return list;
}

char **
XGetFontPath(Display *display, int *npaths_return)
{
	return _nxCopyFontDir(npaths_return);
}

int
XSetFontPath(Display *display, char **directories, int ndirs)
{
	_nxSetFontDir(directories, ndirs);
	return 1;
}

int
XFreeFontPath(char **list)
{
	_nxFreeFontDir(&list);
	return 1;
}

/* Stub out XCreateFontSet*/
XFontSet
XCreateFontSet(Display *display, _Xconst char *base_font_name_list, 
	char ***missing_charset_list_return, int *missing_charset_count_return,
	char **def_string_return)
{
	*missing_charset_list_return = NULL;
	*missing_charset_count_return = 0;
	return NULL;
}

/* Stub out XGetFontProperty*/
Bool
XGetFontProperty(XFontStruct * font, Atom atom, unsigned long *value_return)
{
	DPRINTF("XGetFontProperty called\n");
	switch (atom) {
	case XA_FONT:			/* 18*/
	case XA_UNDERLINE_POSITION:	/* 51*/
	case XA_UNDERLINE_THICKNESS:	/* 52*/
		break;
	default:
		DPRINTF("XGetFontProperty: Unknown FontProperty Atom %d\n", (int)atom);
	}
	return 0;
}
