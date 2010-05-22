/*
 * Convert BDF files to C source and/or Rockbox .fnt file format
 *
 * Copyright (c) 2002, 2005 by Greg Haerr <greg@censoft.com>
 *
 * What fun it is converting font data...
 *
 * 01/09/10 fix copyright notice sscanf
 * 09/17/02	Version 1.0
 */
#include <stdio.h>
#include <stdint.h>			/* for uint32_t*/
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* BEGIN font.h*/
/* loadable font magic and version #*/
#define VERSION		"RB11"

/* MWIMAGEBITS helper macros*/
#define MWIMAGE_WORDS(x)	(((x)+15)/16)	/* image size in words*/
#define MWIMAGE_BYTES(x)	(MWIMAGE_WORDS(x)*sizeof(MWIMAGEBITS))
#define	MWIMAGE_BITSPERIMAGE (sizeof(MWIMAGEBITS) * 8)
#define	MWIMAGE_BITVALUE(n)	((MWIMAGEBITS) (((MWIMAGEBITS) 1) << (n)))
#define	MWIMAGE_FIRSTBIT	(MWIMAGE_BITVALUE(MWIMAGE_BITSPERIMAGE - 1))
#define	MWIMAGE_TESTBIT(m)	((m) & MWIMAGE_FIRSTBIT)
#define	MWIMAGE_SHIFTBIT(m)	((MWIMAGEBITS) ((m) << 1))

typedef unsigned short	MWIMAGEBITS;	/* bitmap image unit size*/

/* builtin C-based proportional/fixed font structure */
/* based on The Microwindows Project http://microwindows.org */
typedef struct {
	char *		name;		/* font name*/
	int		maxwidth;	/* max width in pixels*/
	int 		height;		/* height in pixels*/
	int		ascent;		/* ascent (baseline) height*/
	int		firstchar;	/* first character in bitmap*/
	int		size;		/* font size in glyphs*/
	/*const*/ MWIMAGEBITS   *bits;	/* 16-bit right-padded bitmap data*/
	/*const*/ uint32_t *offset;/* offsets into bitmap data*/
	/*const*/ unsigned char *width;	/* character widths or NULL if fixed*/
	int		defaultchar;	/* default char (not glyph index)*/
	int32_t		bits_size;	/* # words of MWIMAGEBITS bits*/

	/* unused by runtime system, read in by convbdf*/
	char *		facename;	/* facename of font*/
	char *		copyright;	/* copyright info for loadable fonts*/
	int		pixel_size;
	int		descent;
	int		fbbw, fbbh, fbbx, fbby;
} MWCFONT, *PMWCFONT;
/* END font.h*/

#define isprefix(buf,str)	(!strncmp(buf, str, strlen(str)))
#define	strequal(s1,s2)		(!strcmp(s1, s2))

#define EXTRA	300		/* # bytes extra allocation for buggy .bdf files*/

int gen_c = 0;
int gen_fnt = 0;
int gen_map = 1;
int start_char = 0;
int limit_char = 65535;
int oflag = 0;
char outfile[256];

void        usage(void);
void        getopts(int *pac, char ***pav);
int         convbdf(char *path);

void        free_font(PMWCFONT pf);
PMWCFONT    bdf_read_font(char *path);
int         bdf_read_header(FILE *fp, PMWCFONT pf);
int         bdf_read_bitmaps(FILE *fp, PMWCFONT pf);
char *      bdf_getline(FILE *fp, char *buf, int len);
MWIMAGEBITS bdf_hexval(unsigned char *buf, int ndx1, int ndx2);

int         gen_c_source(PMWCFONT pf, char *path);
int         gen_fnt_file(PMWCFONT pf, char *path);

void
usage(void)
{
	char help[] = {
	"Usage: convbdf [options] [input-files]\n"
	"       convbdf [options] [-o output-file] [single-input-file]\n"
	"Options:\n"
	"    -c     Convert .bdf to .c source file\n"
	"    -f     Convert .bdf to .fnt font file\n"
	"    -s N   Start output at character encodings >= N\n"
	"    -l N   Limit output to character encodings <= N\n"
	"    -n     Don't generate bitmaps as comments in .c file\n"
	};

	fprintf(stderr, "%s", help);
}

/* parse command line options*/
void
getopts(int *pac, char ***pav)
{
	char *p;
	char **av;
	int ac;

	ac = *pac;
	av = *pav;
	while (ac > 0 && av[0][0] == '-') {
		p = &av[0][1]; 
		while( *p)
			switch(*p++) {
		case ' ':			/* multiple -args on av[]*/
			while( *p && *p == ' ')
				p++;
			if( *p++ != '-')	/* next option must have dash*/
				p = "";
			break;			/* proceed to next option*/
		case 'c':			/* generate .c output*/
			gen_c = 1;
			break;
		case 'f':			/* generate .fnt output*/
			gen_fnt = 1;
			break;
		case 'n':			/* don't gen bitmap comments*/
			gen_map = 0;
			break;
		case 'o':			/* set output file*/
			oflag = 1;
			if (*p) {
				strcpy(outfile, p);
				while (*p && *p != ' ')
					p++;
			} else {
				av++; ac--;
				if (ac > 0)
					strcpy(outfile, av[0]);
			}
			break;
		case 'l':			/* set encoding limit*/
			if (*p) {
				limit_char = atoi(p);
				while (*p && *p != ' ')
					p++;
			} else {
				av++; ac--;
				if (ac > 0)
					limit_char = atoi(av[0]);
			}
			break;
		case 's':			/* set encoding start*/
			if (*p) {
				start_char = atoi(p);
				while (*p && *p != ' ')
					p++;
			} else {
				av++; ac--;
				if (ac > 0)
					start_char = atoi(av[0]);
			}
			break;
		default:
			fprintf(stderr, "Unknown option ignored: %c\r\n", *(p-1));
		}
		++av; --ac;
	}
	*pac = ac;
	*pav = av;
}

/* remove directory prefix and file suffix from full path*/
char *
base_name(char *path)
{
	char *p, *b;
	static char base[256];

	/* remove prepended path and extension*/
	b = path;
	for (p=path; *p; ++p) {
		if (*p == '/')
			b = p + 1;
	}
	strcpy(base, b);
	for (p=base; *p; ++p) {
		if (*p == '.') {
			*p = 0;
			break;
		}
	}
	return base;
}

int
convbdf(char *path)
{
	PMWCFONT pf;
	int ret = 0;

	pf = bdf_read_font(path);
	if (!pf)
		exit(1);

	if (gen_c) {
		if (!oflag) {
			strcpy(outfile, base_name(path));
			strcat(outfile, ".c");
		}
		ret |= gen_c_source(pf, outfile);
	}

	if (gen_fnt) {
		if (!oflag) {
			strcpy(outfile, base_name(path));
			strcat(outfile, ".fnt");
		}
		ret |= gen_fnt_file(pf, outfile);
	}

	free_font(pf);
	return ret;
}

int
main(int ac, char **av)
{
	int ret = 0;

	++av; --ac;		/* skip av[0]*/
	getopts(&ac, &av);	/* read command line options*/

	if (ac < 1 || (!gen_c && !gen_fnt)) {
		usage();
		exit(1);
	}
	if (oflag) {
		if (ac > 1 || (gen_c && gen_fnt)) {
			usage();
			exit(1);
		}
	}

	while (ac > 0) {
		ret |= convbdf(av[0]);
		++av; --ac;
	}

	exit(ret);
}

/* free font structure*/
void
free_font(PMWCFONT pf)
{
	if (!pf)
		return;
	if (pf->name)
		free(pf->name);
	if (pf->facename)
		free(pf->facename);
	if (pf->bits)
		free(pf->bits);
	if (pf->offset)
		free(pf->offset);
	if (pf->width)
		free(pf->width);
	free(pf);
}

/* build incore structure from .bdf file*/
PMWCFONT
bdf_read_font(char *path)
{
	FILE *fp;
	PMWCFONT pf;

	fp = fopen(path, "rb");
	if (!fp) {
		fprintf(stderr, "Error opening file: %s\n", path);
		return NULL;
	}

	pf = (PMWCFONT)calloc(1, sizeof(MWCFONT));
	if (!pf)
		goto errout;
	
	pf->name = strdup(base_name(path));

	if (!bdf_read_header(fp, pf)) {
		fprintf(stderr, "Error reading font header\n");
		goto errout;
	}

	if (!bdf_read_bitmaps(fp, pf)) {
		fprintf(stderr, "Error reading font bitmaps\n");
		goto errout;
	}

	fclose(fp);
	return pf;

errout:
	fclose(fp);
	free_font(pf);
	return NULL;
}

/* read bdf font header information, return 0 on error*/
int
bdf_read_header(FILE *fp, PMWCFONT pf)
{
	int encoding;
	int nchars, maxwidth;
	int firstchar = 65535;
	int lastchar = -1;
	char buf[256];
	char facename[256];
	char copyright[256];

	/* set certain values to errors for later error checking*/
	pf->defaultchar = -1;
	pf->ascent = -1;
	pf->descent = -1;

	for (;;) {
		if (!bdf_getline(fp, buf, sizeof(buf))) {
			fprintf(stderr, "Error: EOF on file\n");
			return 0;
		}
		if (isprefix(buf, "FONT ")) {		/* not required*/
			if (sscanf(buf, "FONT %[^\n]", facename) != 1) {
				fprintf(stderr, "Error: bad 'FONT'\n");
				return 0;
			}
			pf->facename = strdup(facename);
			continue;
		}
		if (isprefix(buf, "COPYRIGHT ")) {	/* not required*/
/*			if (sscanf(buf, "COPYRIGHT \"%[^\"]", copyright) != 1) {*/
            if (sscanf(buf, "COPYRIGHT \"%s\"", copyright) != 1) {
				fprintf(stderr, "Error: bad 'COPYRIGHT'\n");
				return 0;
			}
			pf->copyright = strdup(copyright);
			continue;
		}
		if (isprefix(buf, "DEFAULT_CHAR ")) {	/* not required*/
			if (sscanf(buf, "DEFAULT_CHAR %d", &pf->defaultchar) != 1) {
				fprintf(stderr, "Error: bad 'DEFAULT_CHAR'\n");
				return 0;
			}
		}
		if (isprefix(buf, "FONT_DESCENT ")) {
			if (sscanf(buf, "FONT_DESCENT %d", &pf->descent) != 1) {
				fprintf(stderr, "Error: bad 'FONT_DESCENT'\n");
				return 0;
			}
			continue;
		}
		if (isprefix(buf, "FONT_ASCENT ")) {
			if (sscanf(buf, "FONT_ASCENT %d", &pf->ascent) != 1) {
				fprintf(stderr, "Error: bad 'FONT_ASCENT'\n");
				return 0;
			}
			continue;
		}
		if (isprefix(buf, "FONTBOUNDINGBOX ")) {
			if (sscanf(buf, "FONTBOUNDINGBOX %d %d %d %d",
			    &pf->fbbw, &pf->fbbh, &pf->fbbx, &pf->fbby) != 4) {
				fprintf(stderr, "Error: bad 'FONTBOUNDINGBOX'\n");
				return 0;
			}
			continue;
		}
		if (isprefix(buf, "CHARS ")) {
			if (sscanf(buf, "CHARS %d", &nchars) != 1) {
				fprintf(stderr, "Error: bad 'CHARS'\n");
				return 0;
			}
			continue;
		}

		/*
		 * Reading ENCODING is necessary to get firstchar/lastchar
		 * which is needed to pre-calculate our offset and widths
		 * array sizes.
		 */
		if (isprefix(buf, "ENCODING ")) {
			if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
				fprintf(stderr, "Error: bad 'ENCODING'\n");
				return 0;
			}
			if (encoding >= 0 && encoding <= limit_char && encoding >= start_char) {
				if (firstchar > encoding)
					firstchar = encoding;
				if (lastchar < encoding)
					lastchar = encoding;
			}
			continue;
		}
		if (strequal(buf, "ENDFONT"))
			break;
	}

	/* calc font height*/
	if (pf->ascent < 0 || pf->descent < 0 || firstchar < 0) {
		fprintf(stderr, "Error: Invalid BDF file, requires FONT_ASCENT/FONT_DESCENT/ENCODING\n");
		return 0;
	}
	pf->height = pf->ascent + pf->descent;

	/* calc default char*/
	if (pf->defaultchar < 0 || pf->defaultchar < firstchar ||
	    pf->defaultchar > limit_char)
		pf->defaultchar = firstchar;

	/* calc font size (offset/width entries)*/
	pf->firstchar = firstchar;
	pf->size = lastchar - firstchar + 1;
	
	/* use the font boundingbox to get initial maxwidth*/
	/*maxwidth = pf->fbbw - pf->fbbx;*/
	maxwidth = pf->fbbw;

	/* initially use font maxwidth * height for bits allocation*/
	pf->bits_size = nchars * MWIMAGE_WORDS(maxwidth) * pf->height;

	/* allocate bits, offset, and width arrays*/
	pf->bits = (MWIMAGEBITS *)malloc(pf->bits_size * sizeof(MWIMAGEBITS) + EXTRA);
	pf->offset = (uint32_t *)malloc(pf->size * sizeof(uint32_t));
	pf->width = (unsigned char *)malloc(pf->size * sizeof(unsigned char));
	
	if (!pf->bits || !pf->offset || !pf->width) {
		fprintf(stderr, "Error: no memory for font load\n");
		return 0;
	}

	return 1;
}

/* read bdf font bitmaps, return 0 on error*/
int
bdf_read_bitmaps(FILE *fp, PMWCFONT pf)
{
	int32_t ofs = 0;
	int maxwidth = 0;
	int i, k, encoding, width;
	int bbw, bbh, bbx, bby;
	int proportional = 0;
	int encodetable = 0;
	int32_t l;
	char buf[256];

	/* reset file pointer*/
	fseek(fp, 0L, SEEK_SET);

	/* initially mark offsets as not used*/
	for (i=0; i<pf->size; ++i)
		pf->offset[i] = -1;

	for (;;) {
		if (!bdf_getline(fp, buf, sizeof(buf))) {
			fprintf(stderr, "Error: EOF on file\n");
			return 0;
		}
		if (isprefix(buf, "STARTCHAR")) {
			encoding = width = bbw = bbh = bbx = bby = -1;
			continue;
		}
		if (isprefix(buf, "ENCODING ")) {
			if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
				fprintf(stderr, "Error: bad 'ENCODING'\n");
				return 0;
			}
			if (encoding < start_char || encoding > limit_char)
				encoding = -1;
			continue;
		}
		if (isprefix(buf, "DWIDTH ")) {
			if (sscanf(buf, "DWIDTH %d", &width) != 1) {
				fprintf(stderr, "Error: bad 'DWIDTH'\n");
				return 0;
			}
			/* use font boundingbox width if DWIDTH <= 0*/
			if (width <= 0)
				width = pf->fbbw - pf->fbbx;
			continue;
		}
		if (isprefix(buf, "BBX ")) {
			if (sscanf(buf, "BBX %d %d %d %d", &bbw, &bbh, &bbx, &bby) != 4) {
				fprintf(stderr, "Error: bad 'BBX'\n");
				return 0;
			}
			continue;
		}
		if (strequal(buf, "BITMAP")) {
			MWIMAGEBITS *ch_bitmap = pf->bits + ofs;
			int ch_words;

			if (encoding < 0)
				continue;

			/* set bits offset in encode map*/
			if (pf->offset[encoding-pf->firstchar] != (uint32_t)-1) {
				fprintf(stderr, "Error: duplicate encoding for character %d (0x%02x), ignoring duplicate\n",
					encoding, encoding);
				continue;
			}
			pf->offset[encoding-pf->firstchar] = ofs;

			/* calc char width*/
			if (bbx < 0) {
				width -= bbx;
				/*if (width > maxwidth)
					width = maxwidth;*/
				bbx = 0;
			}
			if (width > maxwidth)
				maxwidth = width;
			pf->width[encoding-pf->firstchar] = width;

			/* clear bitmap*/
			memset(ch_bitmap, 0, MWIMAGE_BYTES(width) * pf->height);

			ch_words = MWIMAGE_WORDS(width);
#define BM(row,col)	(*(ch_bitmap + ((row)*ch_words) + (col)))
#define MWIMAGE_NIBBLES	(MWIMAGE_BITSPERIMAGE/4)

			/* read bitmaps*/
			for (i=0; ; ++i) {
				int hexnibbles;

				if (!bdf_getline(fp, buf, sizeof(buf))) {
					fprintf(stderr, "Error: EOF reading BITMAP data\n");
					return 0;
				}
				if (isprefix(buf, "ENDCHAR"))
					break;

				hexnibbles = strlen(buf);
				for (k=0; k<ch_words; ++k) {
					int ndx = k * MWIMAGE_NIBBLES;
					int padnibbles = hexnibbles - ndx;
					MWIMAGEBITS value;
					
					if (padnibbles <= 0)
						break;
					if (padnibbles >= MWIMAGE_NIBBLES)
						padnibbles = 0;

					value = bdf_hexval((unsigned char *)buf,
						ndx, ndx+MWIMAGE_NIBBLES-1-padnibbles);
					value <<= padnibbles * MWIMAGE_NIBBLES;

					BM(pf->height - pf->descent - bby - bbh + i, k) |=
						value >> bbx;
					/* handle overflow into next image word*/
					if (bbx) {
						BM(pf->height - pf->descent - bby - bbh + i, k+1) =
							value << (MWIMAGE_BITSPERIMAGE - bbx);
					}
				}
			}

			ofs += MWIMAGE_WORDS(width) * pf->height;

			continue;
		}
		if (strequal(buf, "ENDFONT"))
			break;
	}

	/* set max width*/
	pf->maxwidth = maxwidth;

	/* change unused offset/width values to default char values*/
	for (i=0; i<pf->size; ++i) {
		int defchar = pf->defaultchar - pf->firstchar;

		if (pf->offset[i] == (uint32_t)-1) {
			pf->offset[i] = pf->offset[defchar];
			pf->width[i] = pf->width[defchar];
		}
	}

	/* determine whether font doesn't require encode table*/
	l = 0;
	for (i=0; i<pf->size; ++i) {
		if (pf->offset[i] != l) {
			encodetable = 1;
			break;
		}
		l += MWIMAGE_WORDS(pf->width[i]) * pf->height;
	}
	if (!encodetable) {
		free(pf->offset);
		pf->offset = NULL;
	}

	/* determine whether font is fixed-width*/
	for (i=0; i<pf->size; ++i) {
		if (pf->width[i] != maxwidth) {
			proportional = 1;
			break;
		}
	}
	if (!proportional) {
		free(pf->width);
		pf->width = NULL;
	}

	/* reallocate bits array to actual bits used*/
	if (ofs < pf->bits_size) {
		pf->bits = realloc(pf->bits, ofs * sizeof(MWIMAGEBITS));
		pf->bits_size = ofs;
	} else if (ofs > pf->bits_size) {
		fprintf(stderr, "Warning: DWIDTH spec > max FONTBOUNDINGBOX\n");
		if (ofs > pf->bits_size+EXTRA) {
			fprintf(stderr, "Error: Not enough bits initially allocated\n");
			return 0;
		}
		pf->bits_size = ofs;
	}

	return 1;
}

/* read the next non-comment line, returns buf or NULL if EOF*/
char *
bdf_getline(FILE *fp, char *buf, int len)
{
	int c;
	char *b;

	for (;;) {
		b = buf;
		while ((c = getc(fp)) != EOF) {
			if (c == '\r')
				continue;
			if (c == '\n')
				break;
			if (b - buf >= (len - 1))
				break;
			*b++ = c;
		}
		*b = '\0';
		if (c == EOF && b == buf)
			return NULL;
		if (b != buf && !isprefix(buf, "COMMENT"))
			break;
	}
	return buf;
}

/* return hex value of portion of buffer*/
MWIMAGEBITS
bdf_hexval(unsigned char *buf, int ndx1, int ndx2)
{
	MWIMAGEBITS val = 0;
	int i, c;

	for (i=ndx1; i<=ndx2; ++i) {
		c = buf[i];
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else c = 0;
		val = (val << 4) | c;
	}
	return val;
}

/* generate C source from in-core font*/
int
gen_c_source(PMWCFONT pf, char *path)
{
	FILE *ofp;
	int i;
	int did_defaultchar = 0;
	int did_syncmsg = 0;
	time_t t = time(0);
	MWIMAGEBITS *ofs = pf->bits;
	char buf[256];
	char obuf[256];
	char hdr1[] = {
		"/* Generated by convbdf on %s. */\n"
		"#include \"device.h\"\n"
		"\n"
		"/* Font information:\n"
		"   name: %s\n"
		"   facename: %s\n"
		"   w x h: %dx%d\n"
		"   size: %d\n"
		"   ascent: %d\n"
		"   descent: %d\n"
		"   first char: %d (0x%02x)\n"
		"   last char: %d (0x%02x)\n"
		"   default char: %d (0x%02x)\n"
		"   proportional: %s\n"
		"   %s\n"
		"*/\n"
		"\n"
		"/* Font character bitmap data. */\n"
		"static const MWIMAGEBITS _%s_bits[] = {\n"
	};

	ofp = fopen(path, "w");
	if (!ofp) {
		fprintf(stderr, "Can't create %s\n", path);
		return 1;
	}
	fprintf(stderr, "Generating %s\n", path);

	strcpy(buf, ctime(&t));
	buf[strlen(buf)-1] = 0;

	fprintf(ofp, hdr1, buf, 
		pf->name,
		pf->facename? pf->facename: "",
		pf->maxwidth, pf->height,
		pf->size,
		pf->ascent, pf->descent,
		pf->firstchar, pf->firstchar,
		pf->firstchar+pf->size-1, pf->firstchar+pf->size-1,
		pf->defaultchar, pf->defaultchar,
		pf->width? "yes": "no",
		pf->copyright? pf->copyright: "",
		pf->name);

	/* generate bitmaps*/
	for (i=0; i<pf->size; ++i) {
		int x;
		int bitcount = 0;
 		int width = pf->width ? pf->width[i] : pf->maxwidth;
		int height = pf->height;
		MWIMAGEBITS *bits = pf->bits + (pf->offset? pf->offset[i]: (height * i));
		MWIMAGEBITS bitvalue;

		/*
		 * Generate bitmap bits only if not this index isn't
		 * the default character in encode map, or the default
		 * character hasn't been generated yet.
		 */
		if (pf->offset && (pf->offset[i] == pf->offset[pf->defaultchar-pf->firstchar])) {
			if (did_defaultchar)
				continue;
			did_defaultchar = 1;
		}

		fprintf(ofp, "\n/* Character %d (0x%02x):\n   width %d",
			i+pf->firstchar, i+pf->firstchar, width);

		if (gen_map) {
			fprintf(ofp, "\n   +");
			for (x=0; x<width; ++x) fprintf(ofp, "-");
			fprintf(ofp, "+\n");

			x = 0;
			while (height > 0) {
				if (x == 0) fprintf(ofp, "   |");

				if (bitcount <= 0) {
					bitcount = MWIMAGE_BITSPERIMAGE;
					bitvalue = *bits++;
				}

				fprintf(ofp, MWIMAGE_TESTBIT(bitvalue)? "*": " ");

				bitvalue = MWIMAGE_SHIFTBIT(bitvalue);
				--bitcount;
				if (++x == width) {
					fprintf(ofp, "|\n");
					--height;
					x = 0;
					bitcount = 0;
				}
			}
			fprintf(ofp, "   +");
			for (x=0; x<width; ++x) fprintf(ofp, "-");
			fprintf(ofp, "+ */\n");
		} else
			fprintf(ofp, " */\n");

		bits = pf->bits + (pf->offset? pf->offset[i]: (pf->height * i));
		for (x=MWIMAGE_WORDS(width)*pf->height; x>0; --x) {
			fprintf(ofp, "0x%04x,\n", *bits);
			if (!did_syncmsg && *bits++ != *ofs++) {
				fprintf(stderr, "Warning: found encoding values in non-sorted order (not an error).\n");
				did_syncmsg = 1;
			}
		}	
	}
	fprintf(ofp, 	"};\n\n");

	if (pf->offset) {
		/* output offset table*/
		fprintf(ofp, "/* Character->glyph mapping. */\n"
			"static const uint32_t _%s_offset[] = {\n",
			pf->name);

		for (i=0; i<pf->size; ++i)
			fprintf(ofp, "  %u,\t/* (0x%02x) */\n", pf->offset[i], i+pf->firstchar);
		fprintf(ofp, "};\n\n");
	}

	/* output width table for proportional fonts*/
	if (pf->width) {
		fprintf(ofp, 	"/* Character width data. */\n"
			"static const unsigned char _%s_width[] = {\n",
			pf->name);

		for (i=0; i<pf->size; ++i)
			fprintf(ofp, "  %d,\t/* (0x%02x) */\n", pf->width[i], i+pf->firstchar);
		fprintf(ofp, "};\n\n");
	}

	/* output MWCFONT struct*/
	if (pf->offset)
		sprintf(obuf, "_%s_offset,", pf->name);
	else sprintf(obuf, "0,  /* no encode table*/");
	if (pf->width)
		sprintf(buf, "_%s_width,", pf->name);
	else sprintf(buf, "0,  /* fixed width*/");
	fprintf(ofp, 	"/* Exported structure definition. */\n"
		"const MWCFONT font_%s = {\n"
		"  \"%s\",\n"
		"  %d,\n"
		"  %d,\n"
		"  %d,\n"
		"  %d,\n"
		"  %d,\n"
		"  _%s_bits,\n"
		"  %s\n"
		"  %s\n"
		"  %d,\n"
		"  sizeof(_%s_bits)/sizeof(MWIMAGEBITS),\n"
		"};\n",
		pf->name, pf->name,
		pf->maxwidth, pf->height,
		pf->ascent,
		pf->firstchar,
		pf->size,
		pf->name,
		obuf,
		buf,
		pf->defaultchar,
		pf->name);

	return 0;
}

static int
WRITEBYTE(FILE *fp, unsigned char c)
{
	return putc(c, fp) != EOF;
}

static int
WRITESHORT(FILE *fp, unsigned short s)
{
	putc(s, fp);
	return putc(s>>8, fp) != EOF;
}

static int
WRITELONG(FILE *fp, uint32_t l)
{
	putc(l, fp);
	putc(l>>8, fp);
	putc(l>>16, fp);
	return putc(l>>24, fp) != EOF;
}

static int
WRITESTR(FILE *fp, char *str, int count)
{
	return fwrite(str, 1, count, fp) == count;
}

static int
WRITESTRPAD(FILE *fp, char *str, int totlen)
{
	int ret;
	
	while (str && *str && totlen > 0)
		if (*str) {
			ret = putc(*str++, fp);
			--totlen;
		}
	while (--totlen >= 0)
		ret = putc(' ', fp);
	return ret;
}

/* generate .fnt format file from in-core font*/
int
gen_fnt_file(PMWCFONT pf, char *path)
{
	FILE *ofp;
	int i;

	ofp = fopen(path, "wb");
	if (!ofp) {
		fprintf(stderr, "Can't create %s\n", path);
		return 1;
	}
	fprintf(stderr, "Generating %s\n", path);

	/* write magic and version #*/
	WRITESTR(ofp, VERSION, 4);

	/* internal font name*/
	WRITESTRPAD(ofp, pf->name, 64);

	/* copyright*/
	WRITESTRPAD(ofp, pf->copyright, 256);

	/* font info*/
	WRITESHORT(ofp, pf->maxwidth);
	WRITESHORT(ofp, pf->height);
	WRITESHORT(ofp, pf->ascent);
	WRITESHORT(ofp, 0);
	WRITELONG(ofp, pf->firstchar);
	WRITELONG(ofp, pf->defaultchar);
	WRITELONG(ofp, pf->size);

	/* variable font data sizes*/
	WRITELONG(ofp, pf->bits_size);		  /* # words of MWIMAGEBITS*/
	WRITELONG(ofp, pf->offset? pf->size: 0);  /* # longs of offset*/
	WRITELONG(ofp, pf->width? pf->size: 0);	  /* # bytes of width*/

	/* variable font data*/
	for (i=0; i<pf->bits_size; ++i)
		WRITESHORT(ofp, pf->bits[i]);
        if (ftell(ofp) & 2)
		WRITESHORT(ofp, 0);		/* pad to 32-bit boundary*/

	if (pf->offset)
		for (i=0; i<pf->size; ++i)
			WRITELONG(ofp, pf->offset[i]);

	if (pf->width)
		for (i=0; i<pf->size; ++i)
			WRITEBYTE(ofp, pf->width[i]);

	fclose(ofp);
	return 0;
}
