/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
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
 * WARNING: GIF decoder and stretchimage routine are licensed under GPL only!
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
#if HAVE_MMAP
#include <sys/mman.h>
#endif

#if MW_FEATURE_IMAGES /* whole file */

/* cached image list*/
typedef struct {
	MWLIST		link;		/* link list*/
	int		id;		/* image id*/
	PMWIMAGEHDR	pimage;		/* image data*/
	PSD		psd;		/* FIXME shouldn't need this*/
} IMAGEITEM, *PIMAGEITEM;

static MWLISTHEAD imagehead;		/* global image list*/
static int nextimageid = 1;

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
static int GdDecodeImage(PSD psd, buffer_t *src, char *path, int flags);

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
int
GdLoadImageFromBuffer(PSD psd, void *buffer, int size, int flags)
{
	buffer_t src;

	GdImageBufferInit(&src, buffer, size);
	return GdDecodeImage(psd, &src, NULL, flags);
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
	int id;
	buffer_t src;

	GdImageBufferInit(&src, buffer, size);
	id = GdDecodeImage(psd, &src, NULL, flags);

	if (id) {
		GdDrawImageToFit(psd, x, y, width, height, id);
		GdFreeImage(id);
	}
}

#if defined(HAVE_FILEIO)
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
GdDrawImageFromFile(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width,
	MWCOORD height, char *path, int flags)
{
	int	id;

	id = GdLoadImageFromFile(psd, path, flags);
	if (id) {
		GdDrawImageToFit(psd, x, y, width, height, id);
		GdFreeImage(id);
	}
}

/**
 * Load an image from a file.
 *
 * @param psd Drawing surface.
 * @param path The file containing the image data.
 * @param flags If nonzero, JPEG images will be loaded as grayscale.  Yuck!
 */
int
GdLoadImageFromFile(PSD psd, char *path, int flags)
{
	int fd, id;
	struct stat s;
	void *buffer = 0;
	buffer_t src;
  
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
	id = GdDecodeImage(psd, &src, path, flags);

#if HAVE_MMAP
	munmap(buffer, s.st_size);
#else
	free(buffer);
#endif
	close(fd);
	return id;
}
#endif /* defined(HAVE_FILEIO) */

/*
 * GdDecodeImage:
 * @psd: Drawing surface.
 * @src: The image data.
 * @flags: If nonzero, JPEG images will be loaded as grayscale.  Yuck!
 *
 * Load an image.
 */
static int
GdDecodeImage(PSD psd, buffer_t * src, char *path, int flags)
{
        int         loadOK = 0;
        PMWIMAGEHDR pimage;
        PIMAGEITEM  pItem;

	/* allocate image struct*/
	pimage = (PMWIMAGEHDR)malloc(sizeof(MWIMAGEHDR));
	if(!pimage) {
		return 0;
	}
	pimage->imagebits = NULL;
	pimage->palette = NULL;
	pimage->transcolor = -1L;

#if defined(HAVE_TIFF_SUPPORT)
	/* must be first... no buffer support yet*/
	if (path)
		loadOK = GdDecodeTIFF(path, pimage);
#endif
#if defined(HAVE_BMP_SUPPORT)
	if (loadOK == 0) 
		loadOK = GdDecodeBMP(src, pimage);
#endif
#if defined(HAVE_GIF_SUPPORT)
	if (loadOK == 0) 
		loadOK = GdDecodeGIF(src, pimage);
#endif
#if defined(HAVE_JPEG_SUPPORT)
	if (loadOK == 0) 
		loadOK = GdDecodeJPEG(src, pimage, psd, flags);
#endif
#if defined(HAVE_PNG_SUPPORT)
	if (loadOK == 0) 
		loadOK = GdDecodePNG(src, pimage);
#endif
#if defined(HAVE_PNM_SUPPORT)
	if(loadOK == 0)
		loadOK = GdDecodePNM(src, pimage);
#endif
#if defined(HAVE_XPM_SUPPORT)
	if (loadOK == 0) 
		loadOK = GdDecodeXPM(src, pimage, psd);
#endif

	if (loadOK == 0) {
		EPRINTF("GdLoadImageFromFile: unknown image type\n");
		goto err;		/* image loading error*/
	}
	if (loadOK != 1)
		goto err;		/* image loading error*/

	/* allocate id*/
	pItem = GdItemNew(IMAGEITEM);
	if (!pItem)
		goto err;
	pItem->id = nextimageid++;
	pItem->pimage = pimage;
	pItem->psd = psd;
	GdListAdd(&imagehead, &pItem->link);

	return pItem->id;

err:
	free(pimage);
	return 0;			/* image loading error*/
}

static PIMAGEITEM
findimage(int id)
{
	PMWLIST		p;
	PIMAGEITEM	pimagelist;

	for (p=imagehead.head; p; p=p->next) {
		pimagelist = GdItemAddr(p, IMAGEITEM, link);
		if (pimagelist->id == id)
			return pimagelist;
	}
	return NULL;
}

/**
 * Draw an image.
 *
 * @param psd Drawing surface.
 * @param x X destination co-ordinate.
 * @param y Y destination co-ordinate.
 * @param width If >=0, the image will be scaled to this width.
 * If <0, the image will not be scaled horiziontally.
 * @param height If >=0, the image will be scaled to this height.
 * If <0, the image will not be scaled vertically.
 * @param id Image to draw.
 */
void
GdDrawImageToFit(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height,
	int id)
{
	PIMAGEITEM	pItem;
	PMWIMAGEHDR	pimage;

	pItem = findimage(id);
	if (!pItem)
		return;
	pimage = pItem->pimage;

	/*
	 * Display image, possibly stretch/shrink to resize
	 */
	if (height < 0)
		height = pimage->height;
	if (width < 0)
		width = pimage->width;

	if (height != pimage->height || width != pimage->width) {
		MWCLIPRECT	rcDst;
		MWIMAGEHDR	image2;

		/* create similar image, different width/height*/

		image2.width = width;
		image2.height = height;
		image2.planes = pimage->planes;
		image2.bpp = pimage->bpp;
		GdComputeImagePitch(pimage->bpp, width, &image2.pitch,
			&image2.bytesperpixel);
		image2.compression = pimage->compression;
		image2.palsize = pimage->palsize;
		image2.palette = pimage->palette;	/* already allocated*/
		image2.transcolor = pimage->transcolor;
		if( (image2.imagebits = malloc(image2.pitch*height)) == NULL) {
			EPRINTF("GdDrawImageToFit: no memory\n");
			return;
		}

		rcDst.x = 0;
		rcDst.y = 0;
		rcDst.width = width;
		rcDst.height = height;

		/* Stretch full source to destination rectangle*/
		GdStretchImage(pimage, NULL, &image2, &rcDst);
		GdDrawImage(psd, x, y, &image2);
		free(image2.imagebits);
	} else
		GdDrawImage(psd, x, y, pimage);
}

/**
 * Destroy an image.
 *
 * @param id Image to free.
 */
void
GdFreeImage(int id)
{
	PIMAGEITEM	pItem;
	PMWIMAGEHDR	pimage;

	pItem = findimage(id);
	if (pItem) {
		GdListRemove(&imagehead, &pItem->link);
		pimage = pItem->pimage;

		/* delete image bits*/
		if(pimage->imagebits)
			free(pimage->imagebits);
		if(pimage->palette)
			free(pimage->palette);

		free(pimage);
		GdItemFree(pItem);
	}
}

/**
 * Get information about an image.
 *
 * @param id Image to query.
 * @param pii Destination for image information.
 * @return TRUE on success, FALSE on error.
 */
MWBOOL
GdGetImageInfo(int id, PMWIMAGEINFO pii)
{
	PMWIMAGEHDR	pimage;
	PIMAGEITEM	pItem;
	int		i;

	pItem = findimage(id);
	if (!pItem) {
		memset(pii, 0, sizeof(*pii));
		return FALSE;
	}
	pimage = pItem->pimage;
	pii->id = id;
	pii->width = pimage->width;
	pii->height = pimage->height;
	pii->planes = pimage->planes;
	pii->bpp = pimage->bpp;
	pii->pitch = pimage->pitch;
	pii->bytesperpixel = pimage->bytesperpixel;
	pii->compression = pimage->compression;
	pii->palsize = pimage->palsize;
	if (pimage->palsize) {
		if (pimage->palette) {
			for (i=0; i<pimage->palsize; ++i)
				pii->palette[i] = pimage->palette[i];
		} else {
			/* FIXME handle jpeg's without palette*/
			GdGetPalette(pItem->psd, 0, pimage->palsize,
				pii->palette);
		}
	}
	return TRUE;
}

#define PIX2BYTES(n)	(((n)+7)/8)
/*
 * compute image line size and bytes per pixel
 * from bits per pixel and width
 */
void
GdComputeImagePitch(int bpp, int width, int *pitch, int *bytesperpixel)
{
	int	linesize;
	int	bytespp = 1;

	if(bpp == 1)
		linesize = PIX2BYTES(width);
	else if(bpp <= 4)
		linesize = PIX2BYTES(width<<2);
	else if(bpp <= 8)
		linesize = width;
	else if(bpp <= 16) {
		linesize = width * 2;
		bytespp = 2;
	} else if(bpp <= 24) {
		linesize = width * 3;
		bytespp = 3;
	} else {
		linesize = width * 4;
		bytespp = 4;
	}

	/* rows are DWORD right aligned*/
	*pitch = (linesize + 3) & ~3;
	*bytesperpixel = bytespp;
}

#if 0
void print_image(PMWIMAGEHDR image)
{
	int i;

	DPRINTF("Image:\n\n");
	DPRINTF("height: %d\n", image->height);
	DPRINTF("width: %d\n", image->width);
	DPRINTF("planes: %d\n", image->planes);
	DPRINTF("bpp: %d\n", image->bpp);
	DPRINTF("compression: %d\n", image->compression);
	DPRINTF("palsize: %d\n", image->palsize);

	for (i=0;i<image->palsize;i++)
		DPRINTF("palette: %d, %d, %d\n", image->palette[i].r,
			image->palette[i].g, image->palette[i].b);

	for(i=0;i<(image->width*image->height);i++)
		DPRINTF("imagebits: %d\n", image->imagebits[i]);
}
#endif

#endif /* MW_FEATURE_IMAGES - whole file */
