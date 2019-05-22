/*
 * Copyright (c) 2000, 2001, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * Image decode routine for PNM, PBM, PGM and PPM files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uni_std.h"
#include <ctype.h>
#include "device.h"
#include "../drivers/genmem.h"

#if MW_FEATURE_IMAGES && HAVE_PNM_SUPPORT

enum {
	PNM_TYPE_NOTPNM,
	PNM_TYPE_PBM,
	PNM_TYPE_PGM,
	PNM_TYPE_PPM
};

PSD
GdDecodePNM(buffer_t *src)
{
	unsigned char *p;
	int type = PNM_TYPE_NOTPNM, binary = 0, gothdrs = 0, scale = 0;
	int ch, x = 0, y = 0, i, n, mask, col1, col2, col3;
	PSD pmd;
	int width = 0, height = 0, data_format, palsize = 0;
	char buf[256];

	GdImageBufferSeekTo(src, 0UL);

	if(!GdImageBufferGetString(src,buf, 4))
		return NULL;

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
		return NULL;

	n = 0;
	while((p = (unsigned char *)GdImageBufferGetString(src, buf, sizeof(buf)))) {
		if(buf[0] == '#')
			continue;
		if(type == PNM_TYPE_PBM) {
			char *sptr = buf;

			width = strtol(sptr, &sptr, 10);
			height = strtol(sptr, &sptr, 10);
			data_format = MWIF_PAL1;
			palsize = 2;
			gothdrs = 1;
			break;
		}
		if(type == PNM_TYPE_PGM || type == PNM_TYPE_PPM) {
			if(!n++) {
				char *sptr = buf;

				width = strtol(sptr, &sptr, 10);
				height = strtol(sptr, NULL, 10);
			} else {
				char *sptr = buf;

				i = strtol(sptr, NULL, 10);
				data_format = MWIF_RGB888;
				if(i > 255) {
					EPRINTF("GdDecodePNM: PPM files must be 24bpp\n");
					return NULL;
				}
				for(scale = 7, n = 2; scale; scale--, n *= 2)
					if(i < n)
						break;
				gothdrs = 1;
				break;
			}
		}
	}

	if(!gothdrs) {
		EPRINTF("GdDecodePNM: bad image headers\n");
		return NULL;
	}

	pmd = GdCreatePixmap(&scrdev, width, height, data_format, NULL, palsize);
	if (!pmd)
		return NULL;

	if (pmd->palette) {
		pmd->palette[0].r = 0xff;
		pmd->palette[0].g = 0xff;
		pmd->palette[0].b = 0xff;
		pmd->palette[1].r = 0;
		pmd->palette[1].g = 0;
		pmd->palette[1].b = 0;
	}
	p = pmd->addr;

	if(type == PNM_TYPE_PBM) {
		if(binary) {
			x = 0;
			y = 0;
			while((ch = GdImageBufferGetChar(src)) != EOF) {
				for(i = 0; i < 8; i++) {
					mask = 0x80 >> i;
					if(ch & mask)
						*p |= mask;
					else *p &= ~mask;
					if(++x == width) {
						if(++y == height)
							return pmd;
						p = pmd->addr - 1 + (y * pmd->pitch);
						x = 0;
						break;
					}
				}
				p++;
			}
		} else {
			n = 0;
			while((ch = GdImageBufferGetChar(src)) != EOF) {
				if(isspace(ch))
					continue;
				mask = 0x80 >> n;
				if(ch == '1')
					*p |= mask;
				else if(ch == '0')
					*p &= ~mask;
				else goto baddata;
				if(++n == 8) {
					n = 0;
					p++;
				}
				if(++x == width) {
					if(++y == height)
						return pmd;
					p = pmd->addr + (y * pmd->pitch);
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
			if(++x == width) {
				if(++y == height)
					return pmd;
				p = pmd->addr + (y * pmd->pitch);
				x = 0;
			}
		}
	}

baddata:
	EPRINTF("GdDecodePNM: bad image data\n");
	GdFreePixmap(pmd);
	return NULL;
}
#endif /* MW_FEATURE_IMAGES && HAVE_PNM_SUPPORT*/
