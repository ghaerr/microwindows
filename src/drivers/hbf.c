/*
 * Copyright 1993,1994,1995 by Ross Paterson
 *
 * - You can use this code for any purpose and without fee, except that
 *   you may distribute only verbatim copies of it.  Please send me any
 *   bug fixes, ports or improvements for inclusion in future versions.
 *
 * - The code is supplied with no warranty of any kind.
 *
 * Ross Paterson <rap@doc.ic.ac.uk>
 * 17 October 1995
 *
 * The following people have supplied bug fixes:
 *	Simon Chow	<khsc@synoptics.com>
 *	Fung Fung Lee	<lee@simd.stanford.edu>
 *	Man-Chi Pong	<mcpong@cs.ust.hk>
 *	Steven Simpson	<simpson@math.psu.edu>
 *	Charles Wang	<charles.wang@infores.com>
 *	Werner Lemberg	<a7621gac@awiuni11.edvz.univie.ac.at>
 */

/*
 * Two C interfaces to HBF files.
 *
 * The multiple interfaces make this code rather messy; I intend
 * to clean it up as experience is gained on what is really needed.
 *
 * There are also two modes of operation:
 * - the default is to read each bitmap from its file as demanded
 * - if IN_MEMORY is defined, the whole bitmap file is held in memory.
 *   In this case, if running under Unix, the bitmap files may be gzipped
 *   (but the filename used in the HBF file should be the name of the
 *   file before it was gzipped).
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "hbf.h"
#include "mwconfig.h"	/* for EPRINTF*/

//#define IN_MEMORY

#if UNIX
#define unix
#endif

#ifdef __MSDOS__
#define msdos
#endif

/*
 * if the linker complains about an unresolved identifier '_strdup',
 * uncomment the following definition.
 */
/* #define NO_STRDUP */

#ifdef	__STDC__
#	define	_(x)	x
#else
#	define	_(x)	()
#endif

#define	reg	register

typedef	int	bool;
#define	TRUE	1
#define	FALSE	0

#define	Bit(n)	(1<<(7 - (n)))

/*
 * Messy file system issues
 */

#ifdef unix
#define	PATH_DELIMITER ':'
#define	RelativeFileName(fn)	((fn)[0] != '/')
#define	LocalFileName(fn)	(strchr(fn, '/') == NULL)
#endif /* unix */
#ifdef msdos
#define	PATH_DELIMITER ';'
#define	HasDrive(fn)	(isalpha((fn)[0]) && (fn)[1] == ':')
#define	RelativeFileName(fn)	(! HasDrive(fn) && (fn)[0] != '\\')
#define	LocalFileName(fn)	(! HasDrive(fn) && strchr(fn, '\\') == NULL)
#define	READ_BINARY	"rb"
#endif /* msdos */
#ifdef vms
#define	PATH_DELIMITER ','
#define	RelativeFileName(fn)	(strchr(fn, ':') == NULL && ((fn)[0] != '[' || (fn)[1] == '.' || (fn)[1] == '-'))
#define	LocalFileName(fn)	(strchr(fn, ':') == NULL && strchr(fn, ']') == NULL)
#endif

#ifndef	RelativeFileName
#define	RelativeFileName(fn)	FALSE
#endif

#ifndef	LocalFileName
#define	LocalFileName(fn)	FALSE
#endif

#ifndef READ_BINARY
#define	READ_BINARY	"r"
#endif

#define	MAX_FILENAME	1024

/*
 *	Internal structures
 */

typedef	unsigned char	byte;

#define PROPERTY	struct _PROPERTY
#define BM_FILE		struct _BM_FILE
#define B2_RANGE	struct _B2_RANGE
#define CODE_RANGE	struct _CODE_RANGE

PROPERTY {
	char		*prop_name;
	char		*prop_value;
	PROPERTY	*prop_next;
};

BM_FILE {
	char	*bmf_name;
#ifdef IN_MEMORY
	byte	*bmf_contents;
#else
	FILE	*bmf_file;
#endif
	long	bmf_size;
	BM_FILE	*bmf_next;
};

B2_RANGE {
	byte		b2r_start;
	byte		b2r_finish;
	B2_RANGE	*b2r_next;
};

typedef	unsigned short	CHAR;
typedef	unsigned int	CHAR_INDEX;	/* character index in file */
#define	BAD_CHAR_INDEX	0xffff

CODE_RANGE {
	CHAR		code_start;
	CHAR		code_finish;
	BM_FILE		*code_bm_file;
	long		code_offset;
	CHAR_INDEX	code_pos;
	bool		code_transposed;
	bool		code_inverted;
	CODE_RANGE	*code_next;
};

/*
 *	Extended internal version of HBF
 */

typedef struct {
	/* fields corresponding to the definition */
	HBF		public;
	/* plus internal stuff */
	char		*filename;
	byte		*bitmap_buffer;
	unsigned int	b2_size;	/* number of legal byte-2's */
	PROPERTY	*property;
	B2_RANGE	*byte_2_range;
	CODE_RANGE	*code_range;
	BM_FILE		*bm_file;
} HBF_STRUCT;

#define	FirstByte(code)		((code)>>8)
#define	SecondByte(code)	((code)&0xff)
#define	MakeCode(byte1,byte2)	(((byte1)<<8)|(byte2))

/* size of a bitmap in the file (may be affected by transposition) */
#define	FileBitmapSize(hbfFile,cp) \
		((cp)->code_transposed ? \
			(hbfBitmapBBox(hbfFile)->hbf_height + 7)/8 * \
				hbfBitmapBBox(hbfFile)->hbf_width : \
			HBF_BitmapSize(hbfFile))

#define	NEW(type)	((type *)malloc((unsigned)(sizeof(type))))

#define	QUOTE '"'

#define MAXLINE	1024

extern	char	*strdup		_((const char *s));

static	void	add_b2r _((B2_RANGE **last_b2r, int start, int finish));
static	bool	add_code_range _((HBF_STRUCT *hbf, const char *line));
static	void	add_property _((HBF_STRUCT *hbf, const char *lp));
static	CHAR_INDEX	b2_pos _((HBF_STRUCT *hbf, HBF_CHAR code));
static	int	b2_size _((B2_RANGE *b2r));
static	void	clear_bbox _((HBF_BBOX *bbox));
static	void	clear_record _((HBF_STRUCT *hbf));
static	char	*concat _((const char *dir, int dirlen, const char *stem));
static	char	*expand_filename _((const char *name, const char *filename));
static	const	byte *get_bitmap
		_((HBF_STRUCT *hbf, HBF_CHAR code, byte *buffer));
static	byte	*local_buffer _((HBF_STRUCT *hbf));
static	void	invert _((byte *buffer, unsigned length));
#ifdef IN_MEMORY
static	bool	read_bitmap_file _((BM_FILE *bmf, FILE *f));
static	bool	copy_transposed
		_((HBF *hbf, byte *bitmap, const byte *source));
#else
static	bool	get_transposed _((HBF *hbf, FILE *f, byte *bitmap));
#endif
static	bool	match _((const char *lp, const char *sp));
static	bool	parse_file _((FILE *f, HBF_STRUCT *hbf));
static	FILE	*path_open
		_((const char *path, const char *filename, char **fullp));
static	bool	real_open _((const char *filename, HBF_STRUCT *hbf));

/* Error reporting */
int	hbfDebug = 1;	/* set this for error reporting */

#ifdef	__STDC__
#include <stdarg.h>

static void
eprintf(const char *fmt, ...)
{
	if (hbfDebug) {
		va_list	args;
		char buf[512];

		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		EPRINTF("HBF: %s\n", buf);
		va_end(args);
	}
}
#else /* ! __STDC__ */
/* poor man's variable-length argument list */
static void
eprintf(fmt, x1, x2, x3, x4, x5, x6, x7, x8, x9)
	const	char	*fmt;
	int	x1, x2, x3, x4, x5, x6, x7, x8, x9;
{
	if (hbfDebug) {
		(void)EPRINTF("HBF: ");
		(void)EPRINTF(fmt, x1, x2, x3, x4, x5, x6, x7, x8, x9);
		(void)EPRINTF("\n");
	}
}
#endif /* __STDC__ */

static void
clear_bbox(bbox)
	HBF_BBOX *bbox;
{
	bbox->hbf_width = bbox->hbf_height = 0;
	bbox->hbf_xDisplacement = bbox->hbf_yDisplacement = 0;
}

static void
clear_record(hbf)
	HBF_STRUCT *hbf;
{
	clear_bbox(&(hbf->public.hbf_bitmap_bbox));
	clear_bbox(&(hbf->public.hbf_font_bbox));
	hbf->property = NULL;
	hbf->filename = NULL;
	hbf->bitmap_buffer = NULL;
	hbf->byte_2_range = NULL;
	hbf->code_range = NULL;
	hbf->bm_file = NULL;
}

/*
 *	Byte-2 ranges
 */

static void
add_b2r(last_b2r, start, finish)
reg	B2_RANGE **last_b2r;
	int	start;
	int	finish;
{
reg	B2_RANGE *b2r;

	b2r = NEW(B2_RANGE);
	while (*last_b2r != NULL && (*last_b2r)->b2r_start < start)
		last_b2r = &((*last_b2r)->b2r_next);
	b2r->b2r_next = *last_b2r;
	b2r->b2r_start = start;
	b2r->b2r_finish = finish;
	*last_b2r = b2r;
}

static CHAR_INDEX
b2_pos(hbf, code)
	HBF_STRUCT	*hbf;
	HBF_CHAR	code;
{
reg	B2_RANGE *b2r;
reg	unsigned c;
reg	CHAR_INDEX	pos;

	c = SecondByte(code);
	pos = 0;
	for (b2r = hbf->byte_2_range; b2r != NULL; b2r = b2r->b2r_next)
		if (b2r->b2r_start <= c && c <= b2r->b2r_finish)
			return pos + c - b2r->b2r_start;
		else
			pos += b2r->b2r_finish - b2r->b2r_start + 1;
	return BAD_CHAR_INDEX;
}

static int
b2_size(b2r)
reg	B2_RANGE *b2r;
{
reg	int	size;

	size = 0;
	for ( ; b2r != NULL; b2r = b2r->b2r_next)
		size += b2r->b2r_finish - b2r->b2r_start + 1;
	return size;
}

/* map a position to a character code */
static long
code_of(hbf, pos)
	HBF_STRUCT	*hbf;
	long		pos;
{
	long	code;
	int	residue;
reg	B2_RANGE *b2r;

	code = pos / hbf->b2_size * 256;
	residue = pos % hbf->b2_size;
	for (b2r = hbf->byte_2_range; b2r != NULL; b2r = b2r->b2r_next)
		if (b2r->b2r_start + residue <= b2r->b2r_finish)
			return code + b2r->b2r_start + residue;
		else
			residue -= b2r->b2r_finish - b2r->b2r_start + 1;
	/* should never get here */
	return 0L;
}

/*
 *	String stuff
 */

static bool
match(lp, sp)
reg	const	char	*lp;
reg	const	char	*sp;
{
	while (*lp == *sp && *sp != '\0') {
		lp++;
		sp++;
	}
	return (*lp == '\0' || isspace(*lp)) && *sp == '\0';
}

#ifdef NO_STRDUP
char *
strdup(s)
	const	char	*s;
{
	char	*new_s;

	new_s = malloc((unsigned)strlen(s) + 1);
	strcpy(new_s, s);
	return new_s;
}
#endif

/*
 *	Properties
 */

static void
add_property(hbf, lp)
reg	HBF_STRUCT	*hbf;
reg	const char	*lp;
{
reg	PROPERTY	*prop;
	char	tmp[MAXLINE];
reg	char	*tp;

	prop = NEW(PROPERTY);

	tp = tmp;
	while (*lp != '\0' && ! isspace(*lp))
		*tp++ = *lp++;
	*tp = '\0';
	prop->prop_name = strdup(tmp);

	while (*lp != '\0' && isspace(*lp))
		lp++;

	tp = tmp;
	if (*lp == QUOTE) {
		lp++;
		while (*lp != '\0' && ! (*lp == QUOTE && *++lp != QUOTE))
			*tp++ = *lp++;
	}
	else
		for (;;) {
			while (*lp != '\0' && ! isspace(*lp))
				*tp++ = *lp++;
			while (*lp != '\0' && isspace(*lp))
				lp++;
			if (*lp == '\0')
				break;
			*tp++ = ' ';
		}
	*tp = '\0';
	prop->prop_value = strdup(tmp);

	prop->prop_next = hbf->property;
	hbf->property = prop;
}

const char *
hbfProperty(hbfFile, propName)
	HBF		*hbfFile;
	const	char	*propName;
{
reg	HBF_STRUCT	*hbf;
reg	PROPERTY	*prop;

	hbf = (HBF_STRUCT *)hbfFile;
	for (prop = hbf->property; prop != NULL; prop = prop->prop_next)
		if (strcmp(prop->prop_name, propName) == 0)
			return prop->prop_value;
	return NULL;
}

/*
 *	Compatability routines
 */

const char *
HBF_GetProperty(handle, propertyName)
	HBF		*handle;
	const	char	*propertyName;
{
	return hbfProperty(handle, propertyName);
}

int
HBF_GetFontBoundingBox(handle, width, height, xDisplacement, yDisplacement)
	HBF_Handle	handle;
	unsigned int	*width;
	unsigned int	*height;
	int		*xDisplacement;
	int		*yDisplacement;
{
	if (width != NULL)
		*width = hbfFontBBox(handle)->hbf_width;
	if (height != NULL)
		*height = hbfFontBBox(handle)->hbf_height;
	if (xDisplacement != NULL)
		*xDisplacement = hbfFontBBox(handle)->hbf_xDisplacement;
	if (yDisplacement != NULL)
		*yDisplacement = hbfFontBBox(handle)->hbf_yDisplacement;
	return 0;
}

int
HBF_GetBitmapBoundingBox(handle, width, height, xDisplacement, yDisplacement)
	HBF_Handle	handle;
	unsigned int	*width;
	unsigned int	*height;
	int		*xDisplacement;
	int		*yDisplacement;
{
	if (width != NULL)
		*width = hbfBitmapBBox(handle)->hbf_width;
	if (height != NULL)
		*height = hbfBitmapBBox(handle)->hbf_height;
	if (xDisplacement != NULL)
		*xDisplacement = hbfBitmapBBox(handle)->hbf_xDisplacement;
	if (yDisplacement != NULL)
		*yDisplacement = hbfBitmapBBox(handle)->hbf_yDisplacement;
	return 0;
}

/*
 * Prepend a directory to a relative filename.
 */
static char *
concat(dir, dirlen, stem)
	const	char	*dir;	/* not necessarily null-terminated */
	int	dirlen;		/* number of significant chars in dir */
	const	char	*stem;	/* relative filename */
{
	char	*fullname;

	if (dirlen == 0)	/* null: current directory */
		return strdup(stem);
#ifdef unix
	fullname = malloc(dirlen + strlen(stem) + 2);
	(void)sprintf(fullname, "%.*s/%s", dirlen, dir, stem);
#else
#ifdef msdos
	fullname = malloc(dirlen + strlen(stem) + 2);
	(void)sprintf(fullname, "%.*s\\%s", dirlen, dir, stem);
#else
#ifdef vms
	if (dir[dirlen-1] == ']' && stem[0] == '[' && stem[1] == '-') {
		dirlen--;
		stem++;
		fullname = malloc(dirlen + strlen(stem) + 2);
		(void)sprintf(fullname, "%.*s.%s", dirlen, dir, stem);
	}
	else {
		if (dir[dirlen-1] == ']' && stem[0] == '[' && stem[1] == '.') {
			dirlen--;
			stem++;
		}
		fullname = malloc(dirlen + strlen(stem) + 1);
		(void)sprintf(fullname, "%.*s%s", dirlen, dir, stem);
	}
#else
	fullname = strdup(stem);
#endif /* vms */
#endif /* msdos */
#endif /* unix */
	return fullname;
}

/*
 *	Bitmap files
 *
 *	If the host operating system has a heirarchical file system and
 *	the bitmap file name is relative, it is relative to the directory
 *	containing the HBF file.
 */
static char *
expand_filename(name, hbf_name)
	const	char	*name;
	const	char	*hbf_name;
{
#ifdef unix
reg	char	*s;
reg	int	size;

	size = name[0] != '/' && (s = strrchr(hbf_name, '/')) != NULL ?
		s - hbf_name + 1 : 0;
	s = malloc((unsigned)size + strlen(name) + 1);
	(void)sprintf(s, "%.*s%s", size, hbf_name, name);
	return s;
#else
#ifdef msdos
reg	char	*s;
reg	int	size;

	size = HasDrive(name) ? 0 :
		name[0] == '\\' ? (HasDrive(hbf_name) ? 2 : 0) :
		(s = strrchr(hbf_name, '\\')) != NULL ?
			s - hbf_name + 1 : 0;
	s = malloc((unsigned)size + strlen(name) + 1);
	(void)sprintf(s, "%.*s%s", size, hbf_name, name);
	return s;
#else
#ifdef vms
reg	char	*s;
reg	const	char	*copyto;
reg	int	size;

	if ((s = strchr(hbf_name, ']')) != NULL && RelativeFileName(name))
		return concat(hbf_name, (s - hbf_name) + 1, name);

	copyto = hbf_name;
	if ((s = strstr(copyto, "::")) != NULL && strstr(name, "::") == NULL)
		copyto = s+2;
	if ((s = strchr(copyto, ':')) != NULL && strchr(name, ':') == NULL)
		copyto = s+1;
	size = copyto - hbf_name;
	s = malloc((unsigned)size + strlen(name) + 1);
	(void)sprintf(s, "%.*s%s", size, hbf_name, name);
	return s;
#else
	return strdup(name);
#endif /* vms */
#endif /* msdos */
#endif /* unix */
}

static BM_FILE *
find_file(hbf, filename)
	HBF_STRUCT *hbf;
	const char *filename;
{
	BM_FILE	**fp;
reg	BM_FILE	*file;
	FILE	*f;
#ifdef IN_MEMORY
#ifdef unix
	bool	from_pipe;
#endif
#endif

	for (fp = &(hbf->bm_file); *fp != NULL; fp = &((*fp)->bmf_next))
		if (strcmp((*fp)->bmf_name, filename) == 0)
			return *fp;

	file = NEW(BM_FILE);
	if (file == NULL) {
		eprintf("out of memory");
		return NULL;
	}
	file->bmf_name = expand_filename(filename, hbf->filename);
	if (file->bmf_name == NULL) {
		free((char *)file);
		return NULL;
	}
	f = fopen(file->bmf_name, READ_BINARY);
#ifdef IN_MEMORY
#ifdef unix
	from_pipe = FALSE;
	if (f == NULL) {
		char	tmp[400];

		sprintf(tmp, "%s.gz", file->bmf_name);
		if ((f = fopen(tmp, "r")) != NULL) {
			fclose(f);
			sprintf(tmp, "gzcat %s.gz", file->bmf_name);
			if ((f = popen(tmp, "r")) != NULL)
				from_pipe = TRUE;
		}
	}
#endif /* unix */
#endif /* IN_MEMORY */
	if (f == NULL) {
		eprintf("can't open bitmap file '%s'", file->bmf_name);
		free(file->bmf_name);
		free((char *)file);
		return NULL;
	}
#ifdef IN_MEMORY
	if (! read_bitmap_file(file, f)) {
		free(file->bmf_name);
		free((char *)file);
		return NULL;
	}
#ifdef unix
	if (from_pipe)
		pclose(f);
	else
		fclose(f);
#else /* ! unix */
	fclose(f);
#endif /* ! unix */
#else /* ! IN_MEMORY */
	file->bmf_file = f;
	fseek(f, 0L, 2);
	file->bmf_size = ftell(f);
#endif /* ! IN_MEMORY */
	file->bmf_next = NULL;
	*fp = file;
	return file;
}

#ifdef IN_MEMORY
#define	GRAIN_SIZE	512

static bool
read_bitmap_file(bmf, f)
	BM_FILE	*bmf;
	FILE	*f;
{
	byte	*contents, *cp;
	long	size;
	int	c;

	size = 0;
	cp = contents = (byte *)malloc((unsigned)GRAIN_SIZE);
	if (contents == NULL) {
		eprintf("not enough space for bitmap file");
		return 0;
	}
	while ((c = getc(f)) != EOF) {
		if (size%GRAIN_SIZE == 0) {
			contents = (byte *)realloc((char *)contents,
					(unsigned)(size + GRAIN_SIZE));
			if (contents == NULL) {
				eprintf("not enough space for bitmap file");
				return 0;
			}
			cp = contents + size;
		}
		*cp++ = c;
		size++;
	}
	bmf->bmf_size = size;
	bmf->bmf_contents = (byte *)realloc((char *)contents, (unsigned)size);
	return TRUE;
}
#endif /* IN_MEMORY */

/*
 *	Code ranges
 */

/* check that a code range fits within its bitmap file */
static bool
too_short(hbf, cp)
	HBF_STRUCT	*hbf;
	CODE_RANGE	*cp;
{
	int	bm_size;
	long	offset, end_offset;
	BM_FILE	*bmf;
	long	start, finish;

	bm_size = FileBitmapSize(&(hbf->public), cp);
	offset = cp->code_offset;
	start = cp->code_start;
	finish = cp->code_finish;
	end_offset = offset + bm_size *
			(hbf->b2_size*(long)FirstByte(finish) +
				b2_pos(hbf, finish) - cp->code_pos + 1);
	bmf = cp->code_bm_file;
	if (end_offset <= bmf->bmf_size)
		return FALSE;
	/* bitmap file is too short: produce a specific error message */
	if (offset > bmf->bmf_size)
		eprintf("bitmap file '%s' is shorter than offset 0x%04lx",
			bmf->bmf_name, offset);
	else if (offset + bm_size > bmf->bmf_size)
		eprintf("bitmap file '%s' too short: no room for any bitmaps at offset 0x%04lx",
			bmf->bmf_name, offset);
	else
		eprintf("bitmap file '%s' is too short - code range appears to be 0x%04lx-0x%04lx",
			bmf->bmf_name,
			start,
			code_of(hbf, cp->code_pos +
					(bmf->bmf_size - offset)/bm_size) - 1);
	return TRUE;
}

static const char *
skip_word(n, s)
	int	n;
	const	char	*s;
{
	for ( ; n > 0; n--) {
		while (*s != '\0' && ! isspace(*s))
			s++;
		while (*s != '\0' && isspace(*s))
			s++;
	}
	return s;
}

/* optional keywords at the end of a CODE_RANGE line */
static void
parse_keywords(cp, s)
	CODE_RANGE *cp;
	const	char	*s;
{
	for (s = skip_word(4, s) ; *s != '\0'; s = skip_word(1, s)) {
		switch (*s) {
		case 's': case 'S': case 't': case 'T':
			/* keyword "sideways" or "transposed" */
			cp->code_transposed = TRUE;
			break;
		case 'i': case 'I':
			/* keyword "inverted" */
			cp->code_inverted = TRUE;
		}
	}
}

static bool
add_code_range(hbf, line)
	HBF_STRUCT	*hbf;
	const char	*line;
{
	CODE_RANGE *cp;
	CODE_RANGE **cpp;
	long	start, finish;
	long	offset;
	char	filename[MAXLINE];
	BM_FILE	*bmf;
	CHAR_INDEX b2pos;

	if (sscanf(line, "HBF_CODE_RANGE %li-%li %s %li",
			   &start, &finish, filename, &offset) != 4) {
		eprintf("syntax error in HBF_CODE_RANGE");
		return FALSE;
	}
	/* code ranges are checked in real_open() */
	if ((bmf = find_file(hbf, filename)) == NULL)
		return FALSE;
	if ((cp = NEW(CODE_RANGE)) == NULL) {
		eprintf("out of memory");
		return FALSE;
	}

	cp->code_start = (CHAR)start;
	cp->code_finish = (CHAR)finish;
	cp->code_bm_file = bmf;
	cp->code_offset = offset;
	cp->code_transposed = cp->code_inverted = FALSE;
	parse_keywords(cp, line);
	/* insert it in order */
	for (cpp = &hbf->code_range;
	     *cpp != NULL && (*cpp)->code_finish < start;
	     cpp = &((*cpp)->code_next))
		;
	if (*cpp != NULL && (*cpp)->code_start <= finish) {
		eprintf("code ranges overlap");
		return FALSE;
	}
	cp->code_next = *cpp;
	*cpp = cp;

	/* set code_pos, and check range */
	if (start > finish) {
		eprintf("illegal code range 0x%04lx-0x%04lx", start, finish);
		return FALSE;
	}
	if ((b2pos = b2_pos(hbf, start)) == BAD_CHAR_INDEX) {
		eprintf("illegal start code 0x%04lx", start);
		return FALSE;
	}
	cp->code_pos = hbf->b2_size*(long)FirstByte(start) + b2pos;
	if ((b2pos = b2_pos(hbf, finish)) == BAD_CHAR_INDEX) {
		eprintf("illegal finish code 0x%04lx", finish);
		return FALSE;
	}
	/* check that the bitmap file has enough bitmaps */
	return ! too_short(hbf, cp);
}

/*
 *	Reading and parsing of an HBF file
 */

/* get line, truncating to len, and trimming trailing spaces */
static bool
get_line(buf, len, f)
	char	*buf;
	int	len;
	FILE	*f;
{
	int	c;
	char	*bp;

	bp = buf;
	for (;;) {
		if ((c = getc(f)) == EOF) {
			eprintf("unexpected end of file");
			return FALSE;
		}
		if (c == '\n' || c == '\r') {
			/* trim trailing space */
			while (bp > buf && isspace(*(bp-1)))
				bp--;
			*bp = '\0';
			return TRUE;
		}
		if (len > 0) {
			*bp++ = c;
			len--;
		}
	}
}

/* get next non-COMMENT line */
static bool
get_text_line(buf, len, f)
	char	*buf;
	int	len;
	FILE	*f;
{
	while (get_line(buf, len, f))
		if (*buf != '\0' && ! match(buf, "COMMENT"))
			return TRUE;
	return FALSE;
}

static bool
get_property(line, keyword, hbf)
	const	char	*line;
	const	char	*keyword;
	HBF_STRUCT	*hbf;
{
	if (! match(line, keyword)) {
		eprintf("%s expected", keyword);
		return FALSE;
	}
	add_property(hbf, line);
	return TRUE;
}

static bool
get_bbox(line, keyword, bbox)
	const	char	*line;
	const	char	*keyword;
	HBF_BBOX	*bbox;
{
	int	w, h, xd, yd;

	if (! match(line, keyword) ||
	    sscanf(line + strlen(keyword), "%i %i %i %i",
			&w, &h, &xd, &yd) != 4) {
		eprintf("%s expected", keyword);
		return FALSE;
	}
	if (w <= 0 || h <= 0) {
		eprintf("illegal %s dimensions %dx%d", keyword, w, h);
		return FALSE;
	}
	bbox->hbf_width = w;
	bbox->hbf_height = h;
	bbox->hbf_xDisplacement = xd;
	bbox->hbf_yDisplacement = yd;
	return TRUE;
}

/*
 *  HBFHeaderFile ::=
 * 	'HBF_START_FONT'		version			EOLN
 * 	'HBF_CODE_SCHEME'		word ...		EOLN
 * 	'FONT'				fontName		EOLN
 * 	'SIZE'				ptsize xres yres	EOLN
 * 	'HBF_BITMAP_BOUNDING_BOX'	w h xd yd		EOLN
 * 	'FONTBOUNDINGBOX'		w h xd yd		EOLN
 * 	X11R5FontPropertySection
 * 	'CHARS'				n			EOLN
 * 	HBFByte2RangeSection
 * 	HBFCodeRangeSection
 * 	'HBF_END_FONT'			EOLN .
 *
 * This implementation allows extra lines before HBF_END_FONT.
 * Anything after HBF_END_FONT is ignored.
 */

static bool
parse_file(f, hbf)
	FILE	*f;
reg	HBF_STRUCT *hbf;
{
	char	line[MAXLINE];
	int	start, finish;

	if (! get_text_line(line, MAXLINE, f) ||
	    ! get_property(line, "HBF_START_FONT", hbf))
		return FALSE;

	if (! get_text_line(line, MAXLINE, f) ||
	    ! get_property(line, "HBF_CODE_SCHEME", hbf))
		return FALSE;

	if (! get_text_line(line, MAXLINE, f) ||
	    ! get_property(line, "FONT", hbf))
		return FALSE;

	if (! get_text_line(line, MAXLINE, f) ||
	    ! get_property(line, "SIZE", hbf))
		return FALSE;

	if (! get_text_line(line, MAXLINE, f) ||
	    ! get_bbox(line, "HBF_BITMAP_BOUNDING_BOX",
			&(hbf->public.hbf_bitmap_bbox)))
		return FALSE;

	if (! get_text_line(line, MAXLINE, f) ||
	    ! get_bbox(line, "FONTBOUNDINGBOX", &(hbf->public.hbf_font_bbox)))
		return FALSE;

	if (! get_text_line(line, MAXLINE, f))
		return FALSE;
	if (match(line, "STARTPROPERTIES")) {
		for (;;) {
			if (! get_text_line(line, MAXLINE, f))
				return FALSE;
			if (match(line, "ENDPROPERTIES"))
				break;
			add_property(hbf, line);
		}
		if (! get_text_line(line, MAXLINE, f))
			return FALSE;
	}

	if (match(line, "CHARS"))
		if (! get_text_line(line, MAXLINE, f))
			return FALSE;

	if (match(line, "HBF_START_BYTE_2_RANGES")) {
		for (;;) {
			if (! get_text_line(line, MAXLINE, f))
				return FALSE;
			if (match(line, "HBF_END_BYTE_2_RANGES"))
				break;
			if (sscanf(line, "HBF_BYTE_2_RANGE %i-%i",
					&start, &finish) != 2) {
				eprintf("HBF_BYTE_2_RANGE expected");
				return FALSE;
			}
			add_b2r(&(hbf->byte_2_range), start, finish);
		}
		if (! get_text_line(line, MAXLINE, f))
			return FALSE;
	}
	else
		add_b2r(&(hbf->byte_2_range), 0, 0xff);
	hbf->b2_size = b2_size(hbf->byte_2_range);

	if (! match(line, "HBF_START_CODE_RANGES")) {
		eprintf("HBF_START_CODE_RANGES expected");
		return FALSE;
	}
	for (;;) {
		if (! get_text_line(line, MAXLINE, f))
			return FALSE;
		if (match(line, "HBF_END_CODE_RANGES"))
			break;
		if (! add_code_range(hbf, line))
			return FALSE;
	}

	for (;;) {
		if (! get_text_line(line, MAXLINE, f))
			return FALSE;
		if (match(line, "HBF_END_FONT"))
			break;
		/* treat extra lines as properties (for private extensions) */
		add_property(hbf, line);
	}

	return TRUE;
}

static FILE *
path_open(path, filename, fullp)
	const	char	*path;
	const	char	*filename;
	char	**fullp;
{
	if (LocalFileName(filename) && path != NULL) {
#ifdef PATH_DELIMITER
		int	len;
		char	*fullname;
		FILE	*f;
		const	char	*p_next;

		len = strlen(filename);
		for (;;) {
			p_next = strchr(path, PATH_DELIMITER);
			if (p_next == NULL)
				p_next = path + strlen(path);
			fullname = concat(path, p_next - path, filename);
			if ((f = fopen(fullname, "r")) != NULL) {
				*fullp = fullname;
				return f;
			}
			free(fullname);
			if (*p_next == '\0')
				break;
			path = p_next + 1;
		}
#endif
		return NULL;
	}
	else {
		*fullp = strdup(filename);
		return fopen(*fullp, "r");
	}
}

static bool
real_open(filename, hbf)
	const	char	*filename;
reg	HBF_STRUCT *hbf;
{
	FILE	*f;

	f = path_open(getenv("HBFPATH"), filename, &(hbf->filename));
	if (f == NULL) {
		eprintf("can't read file '%s'", filename);
		return FALSE;
	}
	if (! parse_file(f, hbf)) {
		fclose(f);
		return FALSE;
	}
	fclose(f);
	return TRUE;
}

HBF *
hbfOpen(filename)
	const	char	*filename;
{
reg	HBF_STRUCT *hbf;

	if ((hbf = NEW(HBF_STRUCT)) == NULL) {
		eprintf("can't allocate HBF structure");
		return NULL;
	}
	clear_record(hbf);
	if (real_open(filename, hbf))
		return &(hbf->public);
	hbfClose(&(hbf->public));
	return NULL;
}

int
HBF_OpenFont(filename, ptrHandleStorage)
	const	char	*filename;
	HBF	**ptrHandleStorage;
{
	return (*ptrHandleStorage = hbfOpen(filename)) == NULL ? -1 : 0;
}

/*
 *	Close files, free everything associated with the HBF.
 */

int
HBF_CloseFont(hbfFile)
	HBF	*hbfFile;
{
reg	HBF_STRUCT	*hbf;
	PROPERTY	*prop_ptr, *prop_next;
	B2_RANGE	*b2r_ptr, *b2r_next;
	CODE_RANGE	*code_ptr, *code_next;
	BM_FILE		*bmf_ptr, *bmf_next;
	int		status;

	status = 0;
	hbf = (HBF_STRUCT *)hbfFile;

	if (hbf->filename != NULL)
		free(hbf->filename);
	if (hbf->bitmap_buffer != NULL)
		free(hbf->bitmap_buffer);

	for (prop_ptr = hbf->property;
	     prop_ptr != NULL;
	     prop_ptr = prop_next) {
		prop_next = prop_ptr->prop_next;
		free(prop_ptr->prop_name);
		free(prop_ptr->prop_value);
		free((char *)prop_ptr);
	}

	for (b2r_ptr = hbf->byte_2_range;
	     b2r_ptr != NULL;
	     b2r_ptr = b2r_next) {
		b2r_next = b2r_ptr->b2r_next;
		free((char *)b2r_ptr);
	}

	for (code_ptr = hbf->code_range;
	     code_ptr != NULL;
	     code_ptr = code_next) {
		code_next = code_ptr->code_next;
		free((char *)code_ptr);
	}

	for (bmf_ptr = hbf->bm_file;
	     bmf_ptr != NULL;
	     bmf_ptr = bmf_next) {
		bmf_next = bmf_ptr->bmf_next;
#ifdef IN_MEMORY
		free((char *)(bmf_ptr->bmf_contents));
#else
		if (bmf_ptr->bmf_file != NULL &&
		    fclose(bmf_ptr->bmf_file) < 0)
			status = -1;
#endif
		free(bmf_ptr->bmf_name);
		free((char *)bmf_ptr);
	}

	free((char *)hbf);

	return status;
}

void
hbfClose(hbfFile)
	HBF	*hbfFile;
{
	(void)HBF_CloseFont(hbfFile);
}

/*
 *	Fetch a bitmap
 */

const byte *
hbfGetBitmap(hbf, code)
	HBF		*hbf;
	HBF_CHAR	code;
{
	return get_bitmap((HBF_STRUCT *)hbf, code, (byte *)NULL);
}

int
HBF_GetBitmap(hbf, code, buffer)
	HBF		*hbf;
	HBF_CHAR	code;
	byte		*buffer;
{
	return get_bitmap((HBF_STRUCT *)hbf, code, buffer) == NULL ? -1 : 0;
}

/*
 * Internal function to fetch a bitmap.
 * If buffer is non-null, it must be used.
 */
static const byte *
get_bitmap(hbf, code, buffer)
reg	HBF_STRUCT	*hbf;
	HBF_CHAR	code;
	byte		*buffer;
{
	CHAR_INDEX	pos, b2pos;
reg	CODE_RANGE	*cp;
	BM_FILE		*bmf;
	int		bm_size;
	long		offset;

	if ((b2pos = b2_pos(hbf, code)) == BAD_CHAR_INDEX)
		return NULL;
	pos = hbf->b2_size*FirstByte(code) + b2pos;
	for (cp = hbf->code_range; cp != NULL; cp = cp->code_next)
		if (cp->code_start <= code && code <= cp->code_finish) {
			bmf = cp->code_bm_file;
			bm_size = FileBitmapSize(&(hbf->public), cp);
			offset = cp->code_offset +
				   (long)(pos - cp->code_pos) * bm_size;
#ifdef IN_MEMORY
			if (buffer == NULL &&
			    ! cp->code_transposed && ! cp->code_inverted)
				return bmf->bmf_contents + offset;
#endif /* IN_MEMORY */
			if (buffer == NULL &&
			    ((buffer = local_buffer(hbf)) == NULL))
				return NULL;
#ifdef IN_MEMORY
			if (cp->code_transposed)
				copy_transposed(&(hbf->public),
						buffer,
						bmf->bmf_contents + offset);
			else
				memcpy((char *)buffer,
				       (char *)(bmf->bmf_contents + offset),
				       bm_size);
#else /* ! IN_MEMORY */
			if (fseek(bmf->bmf_file, offset, 0) != 0) {
				eprintf("seek error on code 0x%04x", code);
				return NULL;
			}
			if (cp->code_transposed ?
			    ! get_transposed(&(hbf->public), bmf->bmf_file,
						buffer) :
			    fread((char *)buffer,
					bm_size, 1, bmf->bmf_file) != 1) {
				eprintf("read error on code 0x%04x", code);
				return NULL;
			}
#endif /* IN_MEMORY */
			if (cp->code_inverted)
				invert(buffer, HBF_BitmapSize(&(hbf->public)));
			return buffer;
		}
	eprintf("code 0x%04x out of range", code);
	return NULL;
}

static byte *
local_buffer(hbf)
	HBF_STRUCT	*hbf;
{
	if (hbf->bitmap_buffer == NULL &&
	    (hbf->bitmap_buffer = (byte *)malloc(HBF_BitmapSize(&(hbf->public)))) == NULL) {
		eprintf("out of memory");
		return NULL;
	}
	return hbf->bitmap_buffer;
}

static void
invert(buffer, length)
	byte	*buffer;
	unsigned int	length;
{
	for ( ; length > 0; length--)
		*buffer++ ^= 0xff;
}

#ifdef IN_MEMORY
static bool
copy_transposed(hbf, bitmap, source)
	HBF	*hbf;
reg	byte	*bitmap;
reg	const	byte	*source;
{
reg	byte	*pos;
reg	byte	*bm_end;
	int	x;
	int	width;
reg	int	row_size;
reg	int	c;
reg	int	imask, omask;

	width = hbfBitmapBBox(hbf)->hbf_width;
	row_size = HBF_RowSize(hbf);
	bm_end = bitmap + HBF_BitmapSize(hbf);
	(void)memset((char *)bitmap, '\0', HBF_BitmapSize(hbf));
	for (x = 0; x < width; x++) {
		pos = bitmap + x/8;
		omask = Bit(x%8);
		/* y = 0 */
		for (;;) {
			c = *source++;
			for (imask = Bit(0); imask != 0; imask >>= 1) {
				/*
				 * At this point,
				 *
				 *	imask == Bit(y%8)
				 *	pos == bitmap + y*row_size + x/8
				 *
				 * We examine bit y of row x of the input,
				 * setting bit x of row y of the output if
				 * required, by applying omask to *pos.
				 */
				if ((c & imask) != 0)
					*pos |= omask;
				/* if (++y > height) goto end_column */
				pos += row_size;
				if (pos >= bm_end)
					goto end_column;
			}
		}
end_column:
		;
	}
	return TRUE;
}
#else /* ! IN_MEMORY */
static bool
get_transposed(hbf, f, bitmap)
	HBF	*hbf;
	FILE	*f;
reg	byte	*bitmap;
{
reg	byte	*pos;
reg	byte	*bm_end;
	int	x;
	int	width;
reg	int	row_size;
reg	int	c;
reg	int	imask, omask;

	width = hbfBitmapBBox(hbf)->hbf_width;
	row_size = HBF_RowSize(hbf);
	bm_end = bitmap + HBF_BitmapSize(hbf);
	(void)memset((char *)bitmap, '\0', HBF_BitmapSize(hbf));
	for (x = 0; x < width; x++) {
		pos = bitmap + x/8;
		omask = Bit(x%8);
		/* y = 0 */
		for (;;) {
			if ((c = getc(f)) == EOF)
				return FALSE;
			for (imask = Bit(0); imask != 0; imask >>= 1) {
				/*
				 * At this point,
				 *
				 *	imask == Bit(y%8)
				 *	pos == bitmap + y*row_size + x/8
				 *
				 * We examine bit y of row x of the input,
				 * setting bit x of row y of the output if
				 * required, by applying omask to *pos.
				 */
				if ((c & imask) != 0)
					*pos |= omask;
				/* if (++y > height) goto end_column */
				pos += row_size;
				if (pos >= bm_end)
					goto end_column;
			}
		}
end_column:
		;
	}
	return TRUE;
}
#endif /* ! IN_MEMORY */

/*
 * Call function on each valid code in ascending order.
 */
void
hbfForEach(hbfFile, func)
reg	HBF	*hbfFile;
reg	void	(*func)_((HBF *sameHbfFile, HBF_CHAR code));
{
	HBF_STRUCT	*hbf;
	CODE_RANGE	*cp;
reg	B2_RANGE	*b2r;
reg	unsigned	byte1, byte2;
reg	unsigned	finish;

	hbf = (HBF_STRUCT *)hbfFile;
	for (cp = hbf->code_range; cp != NULL; cp = cp->code_next) {
		byte1 = FirstByte(cp->code_start);
		byte2 = SecondByte(cp->code_start);
		while (MakeCode(byte1, byte2) <= cp->code_finish) {
			for (b2r = hbf->byte_2_range;
			     b2r != NULL;
			     b2r = b2r->b2r_next) {
				if (byte2 < b2r->b2r_start)
					byte2 = b2r->b2r_start;
				finish = b2r->b2r_finish;
				if (byte1 == FirstByte(cp->code_finish) &&
				    finish > SecondByte(cp->code_finish))
					finish = SecondByte(cp->code_finish);
				while (byte2 <= finish) {
					(*func)(hbfFile,
						MakeCode(byte1, byte2));
					byte2++;
				}
			}
			byte1++;
			byte2 = 0;
		}
	}
}

const char *
hbfFileName(hbf)
	HBF	*hbf;
{
	return ((HBF_STRUCT *)hbf)->filename;
}

long
hbfChars(hbfFile)
	HBF	*hbfFile;
{
	HBF_STRUCT	*hbf;
	CODE_RANGE	*cp;
	long		num_chars;

	hbf = (HBF_STRUCT *)hbfFile;
	num_chars = 0;
	for (cp = hbf->code_range; cp != NULL; cp = cp->code_next)
		num_chars +=
			hbf->b2_size*FirstByte(cp->code_finish) +
			b2_pos(hbf, cp->code_finish) -
			(hbf->b2_size*FirstByte(cp->code_start) +
			b2_pos(hbf, cp->code_start)) + 1;
	return num_chars;
}

/*
 *	Functions also implemented as macros
 */

#ifdef hbfBitmapBBox
#undef hbfBitmapBBox
#endif

HBF_BBOX *
hbfBitmapBBox(hbf)
	HBF	*hbf;
{
	return &(hbf->hbf_bitmap_bbox);
}

#ifdef hbfFontBBox
#undef hbfFontBBox
#endif

HBF_BBOX *
hbfFontBBox(hbf)
	HBF	*hbf;
{
	return &(hbf->hbf_font_bbox);
}
