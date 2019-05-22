/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * Image decode routine for XPM files
 *
 * Contributed by:
 */
#include <stdlib.h>
#include <string.h>
#include "uni_std.h"
#include "device.h"
#include "../drivers/genmem.h"

#if MW_FEATURE_IMAGES && HAVE_XPM_SUPPORT

struct xpm_cmap {
	int 			palette_entry;
	uint32_t 		color;
	struct xpm_cmap *next;
	char 			mapstr[3];
};

/* This will parse the string into a color value of format 0xAARRGGBB*/
static uint32_t
XPM_parse_color(char *color)
{
	if (color[0] != '#') {
		if (!strcmp(color, "None"))
			return MWNOCOLOR;	/* Transparent - same as 0xAABBGGRR MWCOLORVAL for same*/

		return 0;				/* If its an X color, then we bail */
	} else {
		/* This is ugly! */
		char *sptr = color + 1;
		char rstr[5], gstr[5], bstr[5];
		uint32_t r, g, b;

		switch (strlen(sptr)) {
		case 6:
			return (255L << 24) | strtol(sptr, NULL, 16);

		case 9:	/* RRRGGGBBB */
			strncpy(rstr, sptr, 3);
			strncpy(gstr, sptr + 3, 3);
			strncpy(bstr, sptr + 6, 3);

			rstr[3] = 0;
			gstr[3] = 0;
			bstr[3] = 0;

			r = strtol(rstr, NULL, 16) >> 4;
			g = strtol(gstr, NULL, 16) >> 4;
			b = strtol(bstr, NULL, 16) >> 4;
			return (uint32_t)(255L << 24 | r << 16 | g << 8 | b);

		case 12:
			strncpy(rstr, sptr, 4);
			strncpy(gstr, sptr + 4, 4);
			strncpy(bstr, sptr + 8, 4);

			rstr[4] = 0;
			gstr[4] = 0;
			bstr[4] = 0;

			r = strtol(rstr, NULL, 16) >> 8;
			g = strtol(gstr, NULL, 16) >> 8;
			b = strtol(bstr, NULL, 16) >> 8;

			return (uint32_t)(255L << 24 | (r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF));
		}
	}

	return 0;
}

/* A series of status indicators that let us know whats going on */
#define LOAD_HEADER 1
#define LOAD_COLORS 2
#define LOAD_PALETTE 3
#define LOAD_PIXELS 4
#define LOAD_DONE 5

/* The magic that "should" indicate an XPM (does it really?) */
#define XPM_MAGIC "/* XPM */"

PSD
GdDecodeXPM(buffer_t * src)
{
	struct xpm_cmap *colorheap = NULL;	/* A "heap" of color structs */
	unsigned char *imageptr = NULL;
	unsigned char *imageline = NULL;
	char *c;
	int a;
	int col, row = 0, colors = 0, cpp = 0;
	int in_color = 0;
	int read_xline = 0;
	int status = LOAD_HEADER;
	PSD pmd = NULL;
	int data_format = MWIF_PAL8, palsize;
	struct xpm_cmap *colormap[256];	/* A quick hash of 256 spots for colors */
	char xline[1024];
	char dline[1024];

	/* Start over at the beginning with the file */
	GdImageBufferSeekTo(src, 0L);

	/* get first line*/
	GdImageBufferGetString(src, xline, sizeof(xline));
	xline[strlen(xline) - 1] = 0;

	/* Check the magic */
	if (strncmp(xline, XPM_MAGIC, sizeof(XPM_MAGIC)))
		return NULL;

	for (a = 0; a < 256; a++)
		colormap[a] = NULL;

	while (!GdImageBufferEOF(src)) {
		/* Get the next line from the file */
		GdImageBufferGetString(src, xline, sizeof(xline));
		xline[strlen(xline) - 1] = 0;

		/* Check it out */
		if (xline[0] == '/' && xline[1] == '*')	/* Comment */
			continue;

		if (xline[0] != '\"')
			continue;

		/* remove the quotes from the line */
		for (c = xline + 1, a = 0; *c != '\"' && *c != 0; c++, a++)
			dline[a] = *c;
		dline[a] = 0;

		/* Is it the header? */
		if (status == LOAD_HEADER) {
			char *sptr = dline;
			col = strtol(sptr, &sptr, 10);
			row = strtol(sptr, &sptr, 10);
			colors = strtol(sptr, &sptr, 10);
			cpp = strtol(sptr, NULL, 10);

			/* create 8bpp palette image if colors <= 256, else 32bpp RGBA*/
			if (colors <= 256) {
				data_format = MWIF_PAL8;
				palsize = colors;
			} else {
				data_format = MWIF_RGBA8888;
				palsize = 0;
			}

			pmd = GdCreatePixmap(&scrdev, col, row, data_format, NULL, palsize);
			if (!pmd)
				return NULL;
DPRINTF("xpm %dbpp\n", pmd->bpp);

			imageline = imageptr = pmd->addr;

			/* Allocate enough room for all the colors */
			colorheap = (struct xpm_cmap *) malloc(colors * sizeof(struct xpm_cmap));
			if (!colorheap) {
				EPRINTF("GdDecodeXPM: No mem for palette\n");
				goto out;
			}

			status = LOAD_COLORS;
			in_color = 0;
			continue;
		}

		/* Are we in load colors? */
		if (status == LOAD_COLORS) {
			struct xpm_cmap *n;
			char tstr[5];
			char cstr[256];
			int m;
			char *p;

			c = dline;

			/* Go at at least 1 character, and then count until we have two spaces in a row */
			strncpy(tstr, c, cpp);

			c += cpp;
			for (; *c == '\t' || *c == ' '; c++)
				continue;	/* Skip over whitespace */

			/* We assume that a 'c' follows.  What if it doesn't? */
			c += 2;

			tstr[cpp] = 0;

			/* Now we put it into the array for easy lookup   */
			/* We base it off the first charater, even though */
			/* there may be up to 4                           */
			m = (unsigned char)tstr[0];

			if (colormap[m]) {
				n = colormap[m];

				while (n->next)
					n = n->next;
				n->next = &colorheap[in_color];
				n = n->next;
			} else {
				colormap[m] = &colorheap[in_color];
				n = colormap[m];
			}

			n->next = 0;

			/* Record the string */
			strncpy(n->mapstr, tstr, cpp);
			n->mapstr[cpp] = 0;

			/* Now record the palette entry */
			n->palette_entry = in_color;

			/* get color string*/
			for (p=cstr; *c && *c != '"'; )
				*p++ = *c++;
			*p = 0;

			/* convert to 0xAARRGGB value*/
			n->color = XPM_parse_color(cstr);

			if (data_format == MWIF_PAL8) {
				if (n->color == MWNOCOLOR)
					pmd->transcolor = in_color; /* set transcolor to palette index*/

				pmd->palette[in_color].r = (unsigned char)(n->color >> 16);
				pmd->palette[in_color].g = (unsigned char)(n->color >> 8);
				pmd->palette[in_color].b = (unsigned char)n->color;
			}

			if (++in_color == colors) {
				read_xline = 0;
				status = LOAD_PIXELS;
			}

			continue;
		}

		if (status == LOAD_PIXELS) {
			uint32_t dwordcolor = 0;
			char pxlstr[5];

			c = dline;

			while (*c) {
				unsigned char z = 0;

				if (cpp == 1) {
					z = *c;

					if (!colormap[z]) {
						EPRINTF("GdDecodeXPM: No color entry for (%c)\n", z);
						goto out;
					}

					if (data_format == MWIF_PAL8)
						dwordcolor = colormap[z]->palette_entry;
					else
						dwordcolor = colormap[z]->color;

					c++;
				} else {
					struct xpm_cmap *n;

					/* We grab the largest possible, and then compare */
					strncpy(pxlstr, c, cpp);
					z = pxlstr[0];

					if (!colormap[z]) {
						EPRINTF("GdDecodeXPM: No color entry for (%s)\n", pxlstr);
						goto out;
					}

					n = colormap[z];
					while (n) {
						if (!strncmp (n->mapstr, pxlstr, cpp))
							break;
						n = n->next;
					}

					if (!n) {
						EPRINTF("GdDecodeXPM: No color found for (%s)\n", pxlstr);
						goto out;
					}

					if (data_format == MWIF_PAL8)
						dwordcolor = n->palette_entry;
					else
						dwordcolor = n->color;
					c += cpp;
				}

				if (data_format == MWIF_PAL8)
					*imageptr++ = (unsigned char)dwordcolor;
				else {
					imageptr[0] = (unsigned char)(dwordcolor >> 16);	// R
					imageptr[1] = (unsigned char)(dwordcolor >>  8);	// G
					imageptr[2] = (unsigned char) dwordcolor;			// B
					imageptr[3] = (unsigned char)(dwordcolor >> 24);	// A
					imageptr += 4;
				}
			}

			/* Pad to the end of the line */
			while (imageptr < imageline + pmd->pitch)
				*imageptr++ = 0;
			imageline = imageptr;

			read_xline++;

			if (read_xline == row)
				status = LOAD_DONE;

			continue;
		}
	}

out:
	if (colorheap)
		free(colorheap);
	if (status != LOAD_DONE) {
		GdFreePixmap(pmd);
		return NULL;
	}
	return pmd;
}
#endif /* MW_FEATURE_IMAGES && HAVE_XPM_SUPPORT*/
