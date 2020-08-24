/* intl.h */
/*
 * Copyright (c) 2004 G.Brugnoni <gabriele.brugnoni@dveprojects.com>
 *
 * International support for Microwindows
 */

/* Map len for utf8 */
extern const char utf8_len_map[256];

/* This macro predict the number of bytes needed by the UTF-8 start character ch */
#define UTF8_NBYTE(ch)	(utf8_len_map[((unsigned char)(ch))])

/* This macro consider the current TextCoding */
#define MW_CHRNBYTE(ch) ((mwTextCoding == MWTF_UTF8)? UTF8_NBYTE(ch): 1)

/*
 *  Do shaping and joining of some international charactes, and return a new UC16 string
 */
unsigned short *doCharShape_UC16(const unsigned short *text, int len,
			int *pNewLen, unsigned long *pAttrib);

/*
 *  Do shaping and joining of some international charactes, and return a new UTF-8 string
 */
char *		doCharShape_UTF8(const char *text, int len, int *pNewLen,
			unsigned long *pAttrib );

/* International Properties returned in attrib field */
#define TEXTIP_STANDARD		0x0001	/* text has some standard (ascii) characters */
#define TEXTIP_EXTENDED		0x0002	/* text has some international characters (nonascii) */
#define TEXTIP_SHAPED		0x0004	/* text needs (or has been changed for) shape/joining */
#define TEXTIP_RTOL		0x0008	/* text has all characters that needs
					  (or has been changed for) right to
					  left (bidi) rendering */

char *		doCharBidi_UTF8(const char *text, int len, int *v2lPos,
			char *pDirection, unsigned long *pAttrib);

unsigned short *doCharBidi_UC16(const unsigned short *text, int len,
			int *v2lPos, char *pDirection, unsigned long *pAttrib);
