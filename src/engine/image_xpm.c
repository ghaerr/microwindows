/*
 * Copyright (c) 2000, 2001, 2003, 2005 Greg Haerr <greg@censoft.com>
 *
 * Image decode routine for XPM files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "device.h"
#include "swap.h"

#if MW_FEATURE_IMAGES && defined(HAVE_XPM_SUPPORT)

struct xpm_cmap {
	char mapstr[3];
	long palette_entry;
	long color;
	struct xpm_cmap *next;
};


/* This will parse the string into a color value of some sort */
static long
XPM_parse_color(char *color)
{
	if (color[0] != '#') {
		if (!strcmp(color, "None"))
			return MWNOCOLOR;	/* Transparent */
		else
			return 0;	/* If its an X color, then we bail */
	} else {
		/* This is ugly! */
		char *sptr = color + 1;
		char rstr[5], gstr[5], bstr[5];
		long r, g, b;

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
			return (long)(255L << 24 | r << 16 | g << 8 | b);

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

			return (long)(255L << 24 |
				(r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF));
		}
	}

	return 0;
}

/* A series of status indicators that let us know whats going on */
/* It could be an enum if you want */

#define LOAD_HEADER 1
#define LOAD_COLORS 2
#define LOAD_PALETTE 3
#define LOAD_PIXELS 4
#define LOAD_DONE 5

/* The magic that "should" indicate an XPM (does it really?) */
#define XPM_MAGIC "/* XPM */"

int
GdDecodeXPM(buffer_t * src, PMWIMAGEHDR pimage, PSD psd)
{
	struct xpm_cmap *colorheap = 0;	/* A "heap" of color structs */
	unsigned char *imageptr = 0;
	char *c;
	int a;
	int col, row, colors, cpp;
	int in_color = 0;
	int read_xline = 0;
	int status = LOAD_HEADER;
	MWSCREENINFO sinfo;
	struct xpm_cmap *colormap[256];	/* A quick hash of 256 spots for colors */
	char xline[1024];
	char dline[1024];

	/* Very first thing, get the screen info */
	GdGetScreenInfo(psd, &sinfo);

	for (a = 0; a < 256; a++)
		colormap[a] = 0;

	pimage->imagebits = NULL;
	pimage->palette = NULL;

	/* Start over at the beginning with the file */
	GdImageBufferSeekTo(src, 0UL);
	GdImageBufferGetString(src, xline, sizeof(xline));

	/* Chop the EOL */
	xline[strlen(xline) - 1] = 0;

	/* Check the magic */
	if (strncmp(xline, XPM_MAGIC, sizeof(XPM_MAGIC)))
		return 0;

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
			sscanf(dline, "%i %i %i %i", &col, &row, &colors, &cpp);

			pimage->width = col;
			pimage->height = row;
			pimage->planes = 1;

			if (sinfo.bpp <= 8) {
				pimage->bpp = sinfo.bpp;
				pimage->compression = 0;
			} else {
				pimage->bpp = 32;
				pimage->compression = MWIMAGE_BGR;
			}

			pimage->palsize = colors;
			GdComputeImagePitch(pimage->bpp, col, &pimage->pitch,
					    &pimage->bytesperpixel);

			pimage->imagebits = malloc(pimage->pitch * pimage->height);
			imageptr = (unsigned char *) pimage->imagebits;

			/* Allocate enough room for all the colors */
			colorheap = (struct xpm_cmap *) malloc(colors *
							   sizeof(struct xpm_cmap));

			/* Allocate the palette space (if required) */
			if (sinfo.bpp <= 8)
				pimage->palette = malloc(256 * sizeof(MWPALENTRY));

			if (!colorheap) {
				EPRINTF("GdDecodeXPM: No mem for palette\n");
				return -1;
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
			unsigned char m;

			c = dline;

			/* Go at at least 1 charater, and then count until we have
			   two spaces in a row */
			strncpy(tstr, c, cpp);

			c += cpp;
			for (; *c == '\t' || *c == ' '; c++)
				continue;	/* Skip over whitespace */

			/* FIXME: We assume that a 'c' follows.  What if it doesn't? */
			c += 2;

			tstr[cpp] = 0;

			/* Now we put it into the array for easy lookup   */
			/* We base it off the first charater, even though */
			/* there may be up to 4                           */
			m = tstr[0];

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
			n->palette_entry = (long) in_color;

			/* This is the color */
			sscanf(c, "%65535s", cstr);

			/* Turn it into a real value */
			n->color = XPM_parse_color(cstr);

			/* If we are in palette mode, then we need to */
			/* load the palette (duh..) */

			if (sinfo.bpp <= 8) {
				if (n->color == MWNOCOLOR) {
					/* set transcolor to palette index*/
					pimage->transcolor = in_color;
				}

				pimage->palette[in_color].r =
					(n->color >> 16) & 0xFF;
				pimage->palette[in_color].g =
					(n->color >> 8) & 0xFF;
				pimage->palette[in_color].b = n->color & 0xFF;
			}

			if (++in_color == colors) {
				read_xline = 0;
				status = LOAD_PIXELS;
			}

			continue;
		}

		if (status == LOAD_PIXELS) {
			int bytecount = 0;
			int bitcount = 0;
			long dwordcolor = 0;
			int i;
			char pxlstr[5];

			c = dline;

			while (*c) {
				unsigned char z = 0;

				if (cpp == 1) {
					z = *c;

					if (!colormap[z]) {
						EPRINTF("GdDecodeXPM: No color entry for (%c)\n", z);
						return -1;
					}

					if (sinfo.bpp <= 8)
						dwordcolor = (long)colormap[z]->palette_entry;
					else
						dwordcolor = colormap[z]->color;

					c++;
				} else {
					struct xpm_cmap *n;

					/* We grab the largest possible, and then compare */
					strncpy(pxlstr, c, cpp);
					z = pxlstr[0];

					if (!colormap[z]) {
						EPRINTF("GdDecodeXPM: No color entry for (%s)\n",
							pxlstr);
						return -1;
					}

					n = colormap[z];
					while (n) {
						if (!strncmp (n->mapstr, pxlstr, cpp))
							break;
						n = n->next;
					}

					if (!n) {
						EPRINTF("GdDecodeXPM: No color found for (%s)\n",
							pxlstr);
						return -1;
					}

					if (sinfo.bpp <= 8)
						dwordcolor = (long)n->palette_entry;
					else
						dwordcolor = n->color;
					c += cpp;
				}

				/* 
				 * This ugly thing is needed to ensure that we
				 * work well in all modes.
				 */
				switch (sinfo.bpp) {
				case 2:
					if (bitcount == 0)
						imageptr[0] = 0;

					imageptr[0] |= (dwordcolor & 0x3) << (4 - bitcount);
					bitcount++;

					if (bitcount == 4) {
						imageptr++;
						bytecount += pimage->bytesperpixel;
						bitcount = 0;
					}

					break;

				case 4:
					if (bitcount == 0)
						imageptr[0] = 0;

					imageptr[0] |= (dwordcolor & 0xF) << (2 - bitcount);
					bitcount++;

					if (bitcount == 2) {
						imageptr++;
						bytecount += pimage->bytesperpixel;
						bitcount = 0;
					}

					break;

				case 8:
				case 16:
				case 24:
				case 32:

					for (i = 0; i < pimage->bytesperpixel; i++)
						imageptr[i] = (dwordcolor >> (8 * i)) & 0xFF;

					imageptr += pimage->bytesperpixel;
					bytecount += pimage->bytesperpixel;
					break;

#ifdef NOTUSED
				case 8:
					imageptr[0] = (unsigned char) (dwordcolor & 0xFF);
					imageptr += pimage->bytesperpixel;
					bytecount += pimage->bytesperpixel;
					break;

				case 16:
				case 24:
				case 32:
					imageptr[0] = (unsigned char) (dwordcolor >> 24) & 0xFF;
					imageptr[1] = (unsigned char) (dwordcolor >> 16) & 0xFF;
					imageptr[2] = (unsigned char) (dwordcolor >> 8) & 0xFF;
					imageptr[3] = (unsigned char) (dwordcolor & 0xFF);
					imageptr += pimage->bytesperpixel;
					bytecount += pimage->bytesperpixel;
					break;
#endif
				}
			}

			/* Pad to the end of the line */
			if (bytecount < pimage->pitch)
				for (i = 0; i < (pimage->pitch - bytecount);
				     i++)
					*imageptr++ = 0x00;

			read_xline++;

			if (read_xline == row)
				status = LOAD_DONE;

			continue;
		}
	}

	free(colorheap);

	if (status != LOAD_DONE)
		return -1;
	return 1;
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_XPM_SUPPORT) */
