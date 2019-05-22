/*
 * Copyright (c) 2000, 2001, 2003, 2010 Greg Haerr <greg@censoft.com>
 *
 * Image decode routine for TIFF files
 */
#include <stdlib.h>
#include "uni_std.h"
#include "device.h"
#include "convblit.h"
#include "../drivers/genmem.h"

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

PSD
GdDecodeTIFF(char *path)
{
	TIFF 	*tif;
	int		w, h;
	PSD		pmd;
	MWBLITPARMS parms;
	static TIFFErrorHandler prev_handler = NULL;

	if (!prev_handler)
		prev_handler = TIFFSetErrorHandler(NULL);

	tif = TIFFOpen(path, "r");
	if (!tif)
		return NULL;

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

	parms.data = NULL;
	pmd = GdCreatePixmap(&scrdev, w, h, MWIF_RGBA8888, NULL, 0);
	if (!pmd)
		goto err;

	/* Allocate extra image buffer*/
	if ((parms.data = malloc(h * pmd->pitch)) == NULL)
			goto err;

	TIFFReadRGBAImage(tif, w, h, (uint32 *)parms.data, 0);

	/* use conversion blit to flip upside down image*/
	parms.dstx = parms.dsty = parms.srcx = parms.srcy = 0;
	parms.width = w;
	parms.height = h;
	parms.src_pitch = parms.dst_pitch = pmd->pitch;
	parms.data_out = pmd->addr;
	convblit_flipy_8888(&parms);
	free(parms.data);

	TIFFClose(tif);
	return pmd;

err:
	EPRINTF("GdDecodeTIFF: image loading error\n");
	if (tif)
		TIFFClose(tif);
	if(parms.data)
		free(parms.data);
	if (pmd)
		GdFreePixmap(pmd);
	return NULL;		/* image error*/
}
#endif /* MW_FEATURE_IMAGES && HAVE_TIFF_SUPPORT*/
