/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * Image decode routine for PNM, PBM, PGM and PPM files
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

#if MW_FEATURE_IMAGES && defined(HAVE_PNM_SUPPORT)

enum {
	PNM_TYPE_NOTPNM,
	PNM_TYPE_PBM,
	PNM_TYPE_PGM,
	PNM_TYPE_PPM
};

int
GdDecodePNM(buffer_t *src, PMWIMAGEHDR pimage)
{
	char buf[256], *p;
	int type = PNM_TYPE_NOTPNM, binary = 0, gothdrs = 0, scale = 0;
	int ch, x = 0, y = 0, i, n, mask, col1, col2, col3;

	GdImageBufferSeekTo(src, 0UL);

	if(!GdImageBufferGetString(src,buf, 4))
		return 0;

	if(!strcmp("P1\n", buf)) type = PNM_TYPE_PBM;
	else if(!strcmp("P2\n", buf)) type = PNM_TYPE_PGM;
	else if(!strcmp("P3\n", buf)) type = PNM_TYPE_PPM;
	else if(!strcmp("P4\n", buf)) {
		type = PNM_TYPE_PBM;
		binary = 1;
	}
	else if(!strcmp("P5\n", buf)) {
		type = PNM_TYPE_PGM;
		binary = 1;
	}
	else if(!strcmp("P6\n", buf)) {
		type = PNM_TYPE_PPM;
		binary = 1;
	}

	if(type == PNM_TYPE_NOTPNM)
		return 0;

	n = 0;
	while((p = GdImageBufferGetString(src, buf, 256))) {
		if(*buf == '#') continue;
		if(type == PNM_TYPE_PBM) {
			if(sscanf(buf, "%i %i", &pimage->width,
					&pimage->height) == 2) {
				pimage->bpp = 1;
				gothdrs = 1;
				if(!(pimage->palette = malloc(
						sizeof(MWPALENTRY) * 2))) {
					EPRINTF("Out of memory\n");
					return 2;
				}
				pimage->palsize = 2;
				pimage->palette[0].r = 0xff;
				pimage->palette[0].g = 0xff;
				pimage->palette[0].b = 0xff;
				pimage->palette[1].r = 0;
				pimage->palette[1].g = 0;
				pimage->palette[1].b = 0;
			}
			break;
		}
		if((type == PNM_TYPE_PGM) || (type == PNM_TYPE_PPM)) {
			if(!n++) {
				if(sscanf(buf, "%i %i", &pimage->width,
					&pimage->height) != 2) break;
			} else {
				if(sscanf(buf, "%i", &i) != 1) break;
				pimage->bpp = 24;
				if(i > 255) {
					EPRINTF("GdDecodePNM: PPM files must be "
						"24bpp\n");
					return 2;
				}
				for(scale = 7, n = 2; scale; scale--, n *= 2)
					if(i < n) break;
				gothdrs = 1;
				break;
			}
		}
	}

	if(!gothdrs) {
		EPRINTF("GdDecodePNM: bad image headers\n");
		if(pimage->palette)
			free(pimage->palette);
		return 2;
	}

	pimage->planes = 1;
	GdComputeImagePitch(pimage->bpp, pimage->width, &pimage->pitch,
						&pimage->bytesperpixel);
	pimage->compression = MWIMAGE_RGB;
	if(!(pimage->imagebits = malloc(pimage->pitch * pimage->height))) {
		EPRINTF("GdDecodePNM: couldn't allocate memory for image\n");
		if(pimage->palette)
			free(pimage->palette);
		return 2;
	}

	p = pimage->imagebits;

	if(type == PNM_TYPE_PBM) {
		if(binary) {
			x = 0;
			y = 0;
			while((ch = GdImageBufferGetChar(src)) != EOF) {
				for(i = 0; i < 8; i++) {
					mask = 0x80 >> i;
					if(ch & mask) *p |= mask;
					else *p &= ~mask;
					if(++x == pimage->width) {
						if(++y == pimage->height)
							return 1;
						p = pimage->imagebits - 1 +
							(y * pimage->pitch);
						x = 0;
						break;
					}
				}
				p++;
			}
		} else {
			n = 0;
			while((ch = GdImageBufferGetChar(src)) != EOF) {
				if(isspace(ch)) continue;
				mask = 0x80 >> n;
				if(ch == '1') *p |= mask;
				else if(ch == '0') *p &= ~mask;
				else goto baddata;
				if(++n == 8) {
					n = 0;
					p++;
				}
				if(++x == pimage->width) {
					if(++y == pimage->height)
						return 1;
					p = pimage->imagebits +
						(y * pimage->pitch);
					n = 0;
					x = 0;
				}
			}
		}
	} else {
		while(1) {
			if(type == PNM_TYPE_PGM) {
				if(binary) {
					if((ch = GdImageBufferGetChar(src)) == EOF)
						goto baddata;
				} else {
				  /*if(fscanf(fp, "%i", &ch) != 1)*/
						goto baddata;
				}
				*p++ = ch << scale;
				*p++ = ch << scale;
				*p++ = ch << scale;
			} else {
				if(binary) {
					if(((col1 = GdImageBufferGetChar(src)) == EOF) ||
					 	((col2 = GdImageBufferGetChar(src)) == EOF) ||
					 	((col3 = GdImageBufferGetChar(src)) == EOF))
						goto baddata;
				} else {
				  /*if(fscanf(fp, "%i %i %i", &col1, &col2, &col3) != 3)*/
						goto baddata;
				}
				*p++ = col1 << scale;
				*p++ = col2 << scale;
				*p++ = col3 << scale;
			}
			if(++x == pimage->width) {
				if(++y == pimage->height) return 1;
				p = pimage->imagebits + (y * pimage->pitch);
				x = 0;
			}
		}
	}

baddata:
	EPRINTF("GdDecodePNM: bad image data\n");
	free(pimage->imagebits);
	if(pimage->palette)
		free(pimage->palette);
	return 2;
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_PNM_SUPPORT)*/
