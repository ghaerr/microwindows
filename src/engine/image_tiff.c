/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
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
#include "swap.h"

#if MW_FEATURE_IMAGES && defined(HAVE_TIFF_SUPPORT)
#include <tiffio.h>

int
GdDecodeTIFF(char *path, PMWIMAGEHDR pimage)
{
	TIFF 	*tif;
	int	w, h;
	long	size;

	tif = TIFFOpen(path, "r");
	if (!tif)
		return 0;

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	size = w * h;
	pimage->width = w;
	pimage->height = h;
	pimage->bpp = 32;
	pimage->pitch = w * sizeof(uint32);
	pimage->bytesperpixel = 4;
	pimage->planes = 1;
	pimage->palsize = 0;
	pimage->palette = NULL;

	/* upside down, RGB order (with alpha)*/
	pimage->compression = MWIMAGE_RGB | MWIMAGE_ALPHA_CHANNEL |
		MWIMAGE_UPSIDEDOWN;

	/* Allocate image */
	if ((pimage->imagebits = malloc(size * sizeof(uint32))) == NULL)
		goto err;

	TIFFReadRGBAImage(tif, pimage->width, pimage->height,
		(uint32 *)pimage->imagebits, 0);

#if 0
	{
		/* FIXME alpha channel should be blended with destination*/
		int i;
		uint32	*rgba;
		uint32	rgba_r, rgba_g, rgba_b, rgba_a;
		rgba = (uint32 *)pimage->imagebits;
		for (i = 0; i < size; ++i, ++rgba) {
			if ((rgba_a = TIFFGetA(*rgba) + 1) == 256)
				continue;
			rgba_r = (TIFFGetR(*rgba) * rgba_a)>>8;
			rgba_g = (TIFFGetG(*rgba) * rgba_a)>>8;
			rgba_b = (TIFFGetB(*rgba) * rgba_a)>>8;
			*rgba = 0xff000000|(rgba_b<<16)|(rgba_g<<8)|(rgba_r);
		}
	}
#endif
	TIFFClose(tif);
	return 1;

err:
	EPRINTF("GdDecodeTIFF: image loading error\n");
	if (tif)
		TIFFClose(tif);
	if(pimage->imagebits)
		free(pimage->imagebits);
	if(pimage->palette)
		free(pimage->palette);
	return 2;		/* image error*/
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_TIFF_SUPPORT)*/
