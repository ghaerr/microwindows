/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Martin Jolicoeur <martinj@visuaide.com>
 * Portions Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * Image load/cache/resize/display routines
 *
 * GIF, BMP, JPEG, PPM, PGM, PBM, PNG, XPM and TIFF formats are supported.
 *
 * Instead of working with a file, we work with a buffer (either
 * provided by the user or through mmap).  This improves speed,
 * and provides a mechanism by which the client can send image
 * data directly to the engine.
 *
 * WARNING: GIF decoder routine is licensed under LGPL only!
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
#include "../drivers/genmem.h"
#if HAVE_MMAP
#include <sys/mman.h>
#endif

#if MW_FEATURE_IMAGES /* whole file */

/*
 * Image decoding and display
 * NOTE: This routine and APIs will change in subsequent releases.
 *
 * Decodes and loads a graphics file, then resizes to width/height,
 * then displays image at x, y
 * If width/height == -1, don't resize, use image size.
 * Clipping is not currently supported, just stretch/shrink to fit.
 *
 */
static PSD GdDecodeImage(buffer_t *src, char *path, int flags);

/*
 * Buffered input functions to replace stdio functions
 */
void
GdImageBufferInit(buffer_t *buffer, void *startdata, int size)
{
	buffer->start = startdata;
	buffer->size = size;
	buffer->offset = 0;
}

void
GdImageBufferSeekTo(buffer_t *buffer, unsigned long offset)
{
	if (offset < buffer->size)
		buffer->offset = offset;
}
   
int
GdImageBufferRead(buffer_t *buffer, void *dest, unsigned long size)
{
	unsigned long copysize;

	if (buffer->offset == buffer->size)
		return 0;	/* EOF*/

	if (buffer->offset + size > buffer->size) 
		copysize = buffer->size - buffer->offset;
	else copysize = size;

	memcpy(dest, buffer->start + buffer->offset, copysize);

	buffer->offset += copysize;
	return copysize;
}
 
int
GdImageBufferGetChar(buffer_t *buffer)
{
	if (buffer->offset == buffer->size) 
		return EOF;
	return buffer->start[buffer->offset++];
}
 
char *
GdImageBufferGetString(buffer_t *buffer, char *dest, unsigned int size)
{
	int i,o;
	unsigned int copysize = size - 1;

	if (buffer->offset == buffer->size) 
		return 0;

	if (buffer->offset + copysize > buffer->size) 
		copysize = buffer->size - buffer->offset;

	for(o=0, i=buffer->offset; i < buffer->offset + copysize; i++, o++) {
		if ((dest[o] = buffer->start[i]) == '\n')
			break;
	}

	buffer->offset = i + 1;
	dest[o + 1] = 0;

	return dest;
}
 
int
GdImageBufferEOF(buffer_t *buffer)
{
	return (buffer->offset == buffer->size);
}
 

/**
 * Load an image from a memory buffer.
 *
 * @param psd Screen device.
 * @param buffer The buffer containing the image data.
 * @param size The size of the buffer.
 * @param flags If nonzero, JPEG images will be loaded as grayscale.  Yuck!
 */
PSD
GdLoadImageFromBuffer(void *buffer, int size, int flags)
{
	buffer_t src;

	GdImageBufferInit(&src, buffer, size);
	return GdDecodeImage(&src, NULL, flags);
}

/**
 * Draw an image from a memory buffer.
 *
 * @param psd Drawing surface.
 * @param x X destination co-ordinate.
 * @param y Y destination co-ordinate.
 * @param width If >=0, the image will be scaled to this width.
 * If <0, the image will not be scaled horiziontally.
 * @param height If >=0, the image will be scaled to this height.
 * If <0, the image will not be scaled vertically.
 * @param buffer The buffer containing the image data.
 * @param size The size of the buffer.
 * @param flags If nonzero, JPEG images will be loaded as grayscale.  Yuck!
 */
void
GdDrawImageFromBuffer(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width,
	MWCOORD height, void *buffer, int size, int flags)
{
	PSD		 pmd;
	buffer_t src;

	GdImageBufferInit(&src, buffer, size);
	pmd = GdDecodeImage(&src, NULL, flags);

	if (pmd) {
		GdDrawImagePartToFit(psd, x, y, width, height, 0, 0, 0, 0, pmd);
		pmd->FreeMemGC(pmd);
	}
}

#if HAVE_FILEIO
/**
 * Draw an image from a file.
 *
 * @param psd Drawing surface.
 * @param x X destination co-ordinate.
 * @param y Y destination co-ordinate.
 * @param width If >=0, the image will be scaled to this width.
 * If <0, the image will not be scaled horiziontally.
 * @param height If >=0, the image will be scaled to this height.
 * If <0, the image will not be scaled vertically.
 * @param path The file containing the image data.
 * @param flags If nonzero, JPEG images will be loaded as grayscale.  Yuck!
 */
void
GdDrawImageFromFile(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	char *path, int flags)
{
	PSD	pmd;

	pmd = GdLoadImageFromFile(path, flags);
	if (pmd) {
		GdDrawImagePartToFit(psd, x, y, width, height, 0, 0, 0, 0, pmd);
		pmd->FreeMemGC(pmd);
	}
}

/**
 * Load an image from a file.
 *
 * @param path The file containing the image data.
 * @param flags If nonzero, JPEG images will be loaded as grayscale.  Yuck!
 */
PSD
GdLoadImageFromFile(char *path, int flags)
{
	int fd;
	PSD	pmd;
	void *buffer = 0;
	buffer_t src;
	struct stat s;
  
	fd = open(path, O_RDONLY);
	if (fd < 0 || fstat(fd, &s) < 0) {
		EPRINTF("GdLoadImageFromFile: can't open image: %s\n", path);
		return 0;
	}

#if HAVE_MMAP
	buffer = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!buffer) {
		EPRINTF("GdLoadImageFromFile: Couldn't map image %s\n", path);
		close(fd);
		return 0;
	}
#else
	buffer = malloc(s.st_size);
	if (!buffer) {
		EPRINTF("GdLoadImageFromFile: Couldn't malloc image %s\n", path);
		close(fd);
		return 0;
	}

	if (read(fd, buffer, s.st_size) != s.st_size) {
		EPRINTF("GdLoadImageFromFile: Couldn't load image %s\n", path);
		close(fd);
		return 0;
	}
#endif

	GdImageBufferInit(&src, buffer, s.st_size);
	pmd = GdDecodeImage(&src, path, flags);

#if HAVE_MMAP
	munmap(buffer, s.st_size);
#else
	free(buffer);
#endif
	close(fd);
	return pmd;
}
#endif /* HAVE_FILEIO*/

/*
 * GdDecodeImage:
 * @src: The image data.
 * @flags: If nonzero, JPEG images will be loaded as grayscale.  Yuck!
 *
 * Load an image into a pixmap.
 */
static PSD
GdDecodeImage(buffer_t *src, char *path, int flags)
{
	PSD			pmd;

#if HAVE_TIFF_SUPPORT
	/* must be first... no buffer support yet*/
	if (path && (pmd = GdDecodeTIFF(path)) != NULL)
		return pmd;
#endif
#if HAVE_BMP_SUPPORT
	if ((pmd = GdDecodeBMP(src, TRUE)) != NULL)
		return pmd;
#endif
#if HAVE_GIF_SUPPORT
	if ((pmd = GdDecodeGIF(src)) != NULL)
		return pmd;
#endif
#if HAVE_JPEG_SUPPORT
	if ((pmd = GdDecodeJPEG(src, flags)) != NULL)
		return pmd;
#endif
#if HAVE_PNG_SUPPORT
	if ((pmd = GdDecodePNG(src)) != NULL)
		return pmd;
#endif
#if HAVE_PNM_SUPPORT
	if ((pmd = GdDecodePNM(src)) != NULL)
		return pmd;
#endif
#if HAVE_XPM_SUPPORT
	if ((pmd = GdDecodeXPM(src)) != NULL)
		return pmd;
#endif
	EPRINTF("GdLoadImageFromFile: Image load error\n");
	return NULL;
}

/**
 * Draw whole or part of the image, stretching to fit destination.
 *
 * @param psd Drawing surface.
 * @param x X destination co-ordinate.
 * @param y Y destination co-ordinate.
 * @param width If >=0, the image will be scaled to this width.
 * If <0, the image will not be scaled horiziontally.
 * @param height If >=0, the image will be scaled to this height.
 * If <0, the image will not be scaled vertically.
 * @param sx source X co-ordinate.
 * @param sy source Y co-ordinate.
 * @param swidth source width.  If 0, draw whole image.
 * @param sheight source height.
 * @param id Image to draw.
 */
void
GdDrawImagePartToFit(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	MWCOORD sx, MWCOORD sy, MWCOORD swidth, MWCOORD sheight, PSD pmd)
{
	PSD			pmd2;
	MWCLIPRECT	rcDst,rcSrc;

	if (height < 0)
		height = pmd->yvirtres;
	if (width < 0)
		width = pmd->xvirtres;

	/* no need to stretch if width/height the same and no source offsets*/
	if (height == pmd->yvirtres && width == pmd->xvirtres && swidth == 0) {
		GdDrawImage(psd, x, y, (PMWIMAGEHDR)pmd);	// FIXME casting MWIMAGEHDR
		return;
	}

#if OLDWAY
	/* create similar image, different width/height, no palette*/
	pmd2 = GdCreatePixmap(&scrdev, width, height, pmd->data_format, NULL, 0);
	if (!pmd2) {
		EPRINTF("GdDrawImagePartToFit: no memory\n");
		return;
	}
	pmd2->transcolor = pmd->transcolor;

	/* fake up palette*/
	pmd2->palsize = pmd->palsize;
	pmd2->palette = pmd->palette;

	rcDst.x = 0;
	rcDst.y = 0;
	rcDst.width = width;
	rcDst.height = height;

	/* src rect, not used if swidth == 0*/
	rcSrc.x = sx;
	rcSrc.y = sy;
	rcSrc.width = swidth;
	rcSrc.height = sheight;

	/* Stretch full source to destination rectangle*/
	// FIXME casting MWIMAGEHDR below
	GdStretchImage((PMWIMAGEHDR)pmd, (swidth == 0)? NULL: &rcSrc, (PMWIMAGEHDR)pmd2, &rcDst);
	GdDrawImage(psd, x, y, (PMWIMAGEHDR)pmd2);

	/* undo faked up palette before free*/
	pmd2->palsize = 0;
	pmd2->palette = NULL;
	GdFreePixmap(pmd2);
#else
	GdStretchBlit(psd, x, y, width, height, pmd, sx, sy, sx+swidth, sy+sheight, MWROP_COPY);
#endif
}

/**
 * Get information about an image or pixmap.
 *
 * @param id Pixmap ID
 * @param pii Destination for image information.
 * @return TRUE on success, FALSE on error.
 */
MWBOOL
GdGetImageInfo(PSD pmd, PMWIMAGEINFO pii)
{
	int		i;

	if (!pmd) {
		memset(pii, 0, sizeof(*pii));
		return FALSE;
	}

	pii->width = pmd->xvirtres;
	pii->height = pmd->yvirtres;
	pii->planes = pmd->planes;
	pii->bpp = pmd->bpp;
	pii->data_format = pmd->data_format;
	pii->pitch = pmd->pitch;
	pii->bytesperpixel = pmd->bytesperpixel;
	pii->palsize = pmd->palsize;
	if (pmd->palsize) {
		if (pmd->palette) {
			for (i=0; i<pmd->palsize; ++i)
				pii->palette[i] = pmd->palette[i];
		} else {
			/* FIXME handle jpeg's without palette*/
			GdGetPalette(&scrdev, 0, pmd->palsize, pii->palette);
		}
	}
	return TRUE;
}
#endif /* MW_FEATURE_IMAGES - whole file */
