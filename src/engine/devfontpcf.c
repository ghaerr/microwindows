/* 
 * PCF Reader for Microwindows 
 * This implements a quick and dirty .PCF font parser so that we can take advantage of 
 * dynamically loading XFree86 style fonts.  

 * Copyright 2001, Century Embedded Technologies
 * Written by:  Jordan Crouse
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_PCFGZ_SUPPORT
#include <zlib.h>
#endif

#include "device.h"

/* The user hase the option including ZLIB and being able to    */
/* directly read compressed .pcf files, or to omit it and save  */
/* space.  The following defines make life much easier          */

#ifdef HAVE_PCFGZ_SUPPORT

#define FILEP gzFile

#define FOPEN(path, mode)           gzopen(path, mode)
#define FREAD(file, buffer, size)   gzread(file, buffer, size)
#define FSEEK(file, offset, whence) gzseek(file, offset, whence)
#define FCLOSE(file)                gzclose(file)

#else
#define FILEP  (FILE *)

#define FOPEN(path, mode)           fopen(path, mode)
#define FREAD(file, buffer, size)   fread(buffer, size, 1, file)
#define FSEEK(file, offset, whence) fseek(file, offset, whence)
#define FCLOSE(file)                fclose(file)
#endif
 

/* Handling routines for MWPCFFONT */
/* These are similar to MWCOREFONT, because they are essentially the same thing */

static void pcf_unloadfont(PMWFONT font);

extern MWBOOL	gen_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
extern void	gen_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
extern void	gen_gettextbits(PMWFONT pfont, int ch, MWIMAGEBITS *retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);

extern void	corefont_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, int flags);

static MWFONTPROCS pcf_fontprocs = {
  MWTF_ASCII,		/* routines expect ascii*/
  gen_getfontinfo,
  gen_gettextsize,
  gen_gettextbits,
  pcf_unloadfont,
  corefont_drawtext,
  NULL,			/* setfontsize*/
  NULL,			/* setfontrotation*/
  NULL,			/* setfontattr*/
};


/* Various definitions from the Free86 PCF code */

#define PCF_PROPERTIES (1 << 0)
#define PCF_ACCELERATORS (1 << 1)
#define PCF_METRICS (1 << 2)
#define PCF_BITMAPS (1 << 3)
#define PCF_INK_METRICS (1 << 4)
#define PCF_BDF_ENCODINGS (1 << 5)
#define PCF_SWIDTHS (1 << 6)
#define PCF_GLYPH_NAMES (1 << 7)
#define PCF_BDF_ACCELERATORS (1 << 8)

#define PCF_FORMAT_MASK 0xFFFFFF00
#define PCF_DEFAULT_FORMAT 0x00000000

/* A few structures that define the various fields within the file */

struct toc_entry {
  int type;
  int format;
  int size;
  int offset;
};

struct prop_entry {
  unsigned int name;  
  unsigned char is_string;
  unsigned int value;
};

struct string_table {
  unsigned char *name;
  unsigned char *value;
};

struct metric_entry {
  short leftBearing;
  short rightBearing;
  short width;
  short ascent;
  short descent;
  short attributes;
};

/* This is used to quickly reverse the bits in a field */

static unsigned char _reverse_byte[0x100] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

/* Reverse and swap a short */

static void word_reverse_swap(unsigned char *buf) {
  
  unsigned char *ch = buf;
  unsigned char t;

  int count = 2;

  for(; --count >= 0; ch++) *ch = _reverse_byte[*ch];
  t = buf[0]; buf[0] = buf[1]; buf[1] = t;
}


/* These are maintained statically for ease */

static struct toc_entry *toc;
static unsigned long toc_size;

/* Get the offset of the given field */

static int pcf_get_offset(int item) {
  
  int i;
  for(i = 0; i < toc_size; i++) 
    if (item == toc[i].type) return(toc[i].offset);
  
  return(-1);
}

/* Read the properties from the file */

static int pcf_readprops(FILEP file, struct prop_entry **prop, struct string_table **strings) {

  int offset;

  int format;
  int num_props;
  unsigned long ssize;
  int i;

  struct string_table *s;
  struct prop_entry *p;

  unsigned char *string_buffer, *spos;

  if ((offset = pcf_get_offset(PCF_PROPERTIES)) == -1) return(-1);
  FSEEK(file, offset, SEEK_SET);

  FREAD(file, &format, sizeof(format));
  FREAD(file, &num_props, sizeof(num_props));

  p = *prop = (struct prop_entry *) malloc(num_props * sizeof(struct prop_entry));

  for(i = 0; i < num_props; i++) {
    FREAD(file, &p[i].name, sizeof(p[i].name));
    FREAD(file, &p[i].is_string, sizeof(p[i].is_string));
    FREAD(file, &p[i].value, sizeof(p[i].value));
  }

  /* Pad to 32 bit multiples */
  if (num_props & 3) FSEEK(file, 4 - (num_props & 3), SEEK_CUR);

  FREAD(file, &ssize, sizeof(ssize));
  
  /* Read the entire set of strings into memory */

  spos = string_buffer = (unsigned char *) alloca(ssize);
  FREAD(file, string_buffer, ssize);
  
  /* Allocate the group of strings */

  s = *strings = (struct string_table *) malloc(num_props * sizeof(struct string_table));
  
  for(i = 0; i < num_props; i++) {
    s[i].name = (unsigned char *) strdup(spos);
    spos += strlen(s[i].name) + 1;

    if (p[i].is_string) {
      s[i].value = (unsigned char *) strdup(spos);
      spos += strlen(s[i].value) + 1;
    }
    else s[i].value = 0;
  }

  return(num_props);
}

/* Read the actual bitmaps into memory */

static int pcf_readbitmaps(FILEP file, unsigned char **bits, int *bits_size, 
		    unsigned long **offsets) {

  unsigned long *o;
  unsigned char *b;
  
  unsigned long bmsize[4];
  
  unsigned int format, num_glyphs, offset;
  unsigned int pad;

  if ((offset = pcf_get_offset(PCF_BITMAPS)) == -1) return(-1);
  FSEEK(file, offset, SEEK_SET);
  
  FREAD(file, &format, sizeof(format));
  FREAD(file, &num_glyphs, sizeof(num_glyphs));
  
  o = *offsets = (unsigned long *) malloc(num_glyphs * sizeof(unsigned long));
  FREAD(file, o, sizeof(unsigned long) * num_glyphs);

  FREAD(file, bmsize, sizeof(bmsize));
  
  pad = format & (3 << 0);
  *bits_size = bmsize[pad] ? bmsize[pad] : 1;

  b = *bits = (unsigned char *) malloc(*bits_size);
  FREAD(file, b, *bits_size);
    
  return(num_glyphs);
}

/* This reads the metrics of the file */

static int pcf_readmetrics(FILE *file, struct metric_entry **metrics) {

  int offset;
  int format;
  
  if ((offset = pcf_get_offset(PCF_METRICS)) == -1) return(-1);
  FSEEK(file, offset, SEEK_SET);
  
  FREAD(file, &format, sizeof(format));
  
  if ((format & PCF_FORMAT_MASK) == PCF_DEFAULT_FORMAT) {
    unsigned long size;
    struct metric_entry *m;

    FREAD(file, &size, sizeof(size));  /* 32 bits - Number of metrics */

    m = *metrics = (struct metric_entry *) malloc(size * sizeof(struct metric_entry));
    
    FREAD(file, m, sizeof(struct metric_entry) * size);
    return(size);
  }
  else {
    unsigned short size;
    int i;

    struct metric_entry *m;

    FREAD(file, &size, sizeof(size));  /* 16 bits - Number of metrics */

    m = *metrics = (struct metric_entry *) malloc(size * sizeof(struct metric_entry));
    
    for(i = 0; i < size; i++) {
      unsigned char val;
      FREAD(file, &val, sizeof(val)); m[i].leftBearing = val - 0x80;
      FREAD(file, &val, sizeof(val)); m[i].rightBearing = val - 0x80;
      FREAD(file, &val, sizeof(val)); m[i].width = val - 0x80;
      FREAD(file, &val, sizeof(val)); m[i].ascent = val - 0x80;
      FREAD(file, &val, sizeof(val)); m[i].descent = val - 0x80;
    }

    return(size);
  }
}

static int pcf_read_firstchar(FILE *file) {

  int offset;

  struct encode_entry {
    unsigned short firstcol;
    unsigned short lastcol;
    unsigned short firstrow;
    unsigned short lastrow;
    unsigned short def;
    unsigned short nencode;
  } encode;

  if ((offset = pcf_get_offset(PCF_BDF_ENCODINGS)) == -1) return(-1);
  FSEEK(file, offset, SEEK_SET);

  FREAD(file, &encode, sizeof(encode));
	 
  return(encode.firstrow);
}

/* It should come in reversed */

#define PCF_VERSION "\01fcp"

static int pcf_read_toc(FILE *file, struct toc_entry **toc, 
			unsigned long *size) {

  char version[5];

  FSEEK(file, 0, SEEK_SET);
  FREAD(file, &version, 4);  /* 32 bits - version */
  version[4] = 0;

  /* Verify the version */
  if (strcmp(version, PCF_VERSION)) {
    printf("Bad .PCF file\n");
    printf("Got %2.2x %2.2x %2.2x %2.2x and I expected %2.2x %2.2x %2.2x %2.2x\n",
	   version[0], version[1], version[2], version[3], 'p', 'c', 'f', '\0');
    return(-1);
  }

  FREAD(file, size, sizeof(*size));  
  *toc = (struct toc_entry *) calloc(sizeof(struct toc_entry), *size);
  
  if (!*toc) return(-1);

  /* Read in the entire table of contents */
  FREAD(file, *toc, sizeof(struct  toc_entry) * *size);
  return(0);
}

PMWCOREFONT pcf_createfont(char *name) {

  FILE *file = 0;
  MWCOREFONT *pf = 0;

  int i;
  int count;
  int bsize;

  int err = 0;

  /* Various structures that we will go out and search */
  struct metric_entry *metrics = 0;
  MWIMAGEBITS *output;

  unsigned char *glyphs = 0;
  unsigned long *glyphs_offsets = 0;
  
  int max_width = 0, max_descent = 0, max_ascent = 0, max_height = 0;
  int glyph_count;

  /* Check the filename */

  if (!(pf = (MWCOREFONT *) malloc(sizeof(MWCOREFONT)))) return(0);

  pf->fontprocs = &pcf_fontprocs;
  pf->fontsize = pf->fontrotation = pf->fontattr = 0;
 
  if (!(pf->cfont = (PMWCFONT) malloc(sizeof(MWCFONT)))) {
    err = -1; goto leave_func; 
  }
  
  /* open the file */

  if (!(file = FOPEN(name, "r"))) { 
    err = -1; goto leave_func; 
  }
  
  /* Read the table of contents */
  if (pcf_read_toc(file, &toc, &toc_size) == -1) {
    err = -1; goto leave_func; 
  }
  
  /* Now, read in the bitmaps */

  glyph_count = pcf_readbitmaps(file, &glyphs, 
				&bsize, &glyphs_offsets);

  if (glyph_count == -1) { 
    err = -1; goto leave_func; 
  }
  
  pf->cfont->firstchar = pcf_read_firstchar(file);
  if (pf->cfont->firstchar == -1) { 
    err = -1; goto leave_func; 
  }

  /* Read in the metrics */

  count = pcf_readmetrics(file, &metrics);
  
  /* Calculate the various values */

  for(i = 0; i < count; i++) {
    if (metrics[i].width > max_width) {
      max_width = metrics[i].width;
    }

    if (metrics[i].descent > max_descent) {
      max_descent = metrics[i].descent;
    }
    
    if (metrics[i].ascent > max_ascent) {
      max_ascent = metrics[i].ascent;
    }
  }

  max_height = max_ascent + max_descent;

  pf->cfont->maxwidth = max_width;
  pf->cfont->height = max_height;
  pf->cfont->ascent = max_ascent;
  pf->cfont->size = glyph_count;
  
  /* Allocate enough room to hold all of the files and the offsets */
  pf->cfont->bits = (MWIMAGEBITS *) malloc((max_height * sizeof(MWIMAGEBITS)) * glyph_count);
  pf->cfont->offset = (unsigned short *) malloc(glyph_count * sizeof(unsigned short));
  pf->cfont->width = (unsigned char *) malloc(glyph_count * sizeof(unsigned char));

  output = (MWIMAGEBITS *) pf->cfont->bits;

  /* Ok, so now we need to convert each char */

  for(i = 0; i < glyph_count; i++) {

    int h;
    unsigned char *cptr = glyphs + glyphs_offsets[i];
    unsigned long *ptr = (unsigned long *) cptr;

    pf->cfont->offset[i] = (unsigned short) 
      ((unsigned short *) output - (unsigned short *) pf->cfont->bits);

    pf->cfont->width[i] = (unsigned char) metrics[i].width;

    for(h = max_ascent; h > 0; h--) {
      if (h > metrics[i].ascent) 
	*output++ = 0x0000;
      else {
	unsigned short val = (unsigned short) *ptr;
	word_reverse_swap((unsigned char *) &val);
	*output++ = val;
	ptr++;
      }
    }

    for(h = max_descent; h > 0; h--) {
      if (h > metrics[i].descent) *output++ = 0;
      else {
	unsigned short val = (unsigned short) *ptr;
	word_reverse_swap((unsigned char *) &val);
	*output++ = val;
	ptr++;
      }
    }
  }

 leave_func:

  if(metrics) free(metrics);
  if(glyphs) free(glyphs);
  if(glyphs_offsets) free(glyphs_offsets);

  if (toc) free(toc);  
  toc = 0; toc_size = 0;
  
  FCLOSE(file);

  if (err == 0 && pf) return(pf);

  if (pf->cfont) {
    if (pf->cfont->bits) free(pf->cfont->bits);
    if (pf->cfont->offset) free(pf->cfont->offset);
    if (pf->cfont->width) free(pf->cfont->width);
    
    free(pf->cfont);
  }
  
  free(pf);
  return(0);
}

void pcf_unloadfont(PMWFONT font) {
  
  PMWCOREFONT pf = (PMWCOREFONT) font;

  if (pf->cfont) {
    if (pf->cfont->bits) free(pf->cfont->bits);
    if (pf->cfont->offset) free(pf->cfont->offset);
    if (pf->cfont->width) free(pf->cfont->width);
    
    free(pf->cfont);
  }
  
  free(font);
}
