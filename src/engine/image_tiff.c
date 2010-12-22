/*
 * Copyright (c) 2000, 2001, 2003, 2010 Greg Haerr <greg@censoft.com>
 *
 * Image decode routine for TIFF files
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
#include "convblit.h"
#include "swap.h"

#if MW_FEATURE_IMAGES && HAVE_TIFF_SUPPORT
#include <tiffio.h>

/*
 * Conversion blit flip y direction 32bpp (upside-down)
 */
void convblit_flipy_8888(PMWBLITPARMS gc)
{
	unsigned char *dst;
	unsigned char *src = ((unsigned char *)gc->data) + gc->srcy * gc->src_pitch + gc->srcx * 4;
	int height = gc->height;

	/* flip y coordinate*/
	gc->dsty = height - gc->dsty - 1;
	dst = ((unsigned char *)gc->data_out)            + gc->dsty * gc->dst_pitch + gc->dstx * 4;

	while (--height >= 0)
	{
		register unsigned char *d = dst;
		register unsigned char *s = src;
		int w = gc->width;

		while (--w >= 0)
		{
			d[0] = s[0];
			d[1] = s[1];
			d[2] = s[2];
			d[3] = s[3];

			d += 4;
			s += 4;
		}
		src += gc->src_pitch;
		dst -= gc->dst_pitch;
	}
}

int
GdDecodeTIFF(char *path, PMWIMAGEHDR pimage)
{
	TIFF 	*tif;
	int		w, h;
	MWBLITPARMS parms;
	static TIFFErrorHandler prev_handler = NULL;

	if (!prev_handler)
		prev_handler = TIFFSetErrorHandler(NULL);

	tif = TIFFOpen(path, "r");
	if (!tif)
		return 0;

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	pimage->width = w;
	pimage->height = h;
	pimage->bpp = 32;
	pimage->pitch = w * 4;
	pimage->bytesperpixel = 4;
	pimage->planes = 1;
	pimage->palsize = 0;
	pimage->palette = NULL;
	pimage->data_format = MWIF_RGBA8888;	/* 32bpp RGBA image*/

	/* Allocate image */
	if ((parms.data = malloc(h * pimage->pitch)) == NULL ||
	    (pimage->imagebits = malloc(h * pimage->pitch)) == NULL)
			goto err;

	TIFFReadRGBAImage(tif, pimage->width, pimage->height, (uint32 *)parms.data, 0);

	/* use conversion blit to flip upside down image*/
	parms.dstx = parms.dsty = parms.srcx = parms.srcy = 0;
	parms.width = pimage->width;
	parms.height = pimage->height;
	parms.src_pitch = parms.dst_pitch = pimage->pitch;
	parms.data_out = pimage->imagebits;
	convblit_flipy_8888(&parms);
	free(parms.data);

	TIFFClose(tif);
	return 1;

err:
	EPRINTF("GdDecodeTIFF: image loading error\n");
	if (tif)
		TIFFClose(tif);
	if(parms.data)
		free(parms.data);
	if(pimage->imagebits)
		free(pimage->imagebits);
	if(pimage->palette)
		free(pimage->palette);
	return 2;		/* image error*/
}
#endif /* MW_FEATURE_IMAGES && HAVE_TIFF_SUPPORT*/
