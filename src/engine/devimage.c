/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Martin Jolicoeur <martinj@visuaide.com>
 * Portions Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 * Portions Copyright (c) Independant JPEG group (ijg)
 *
 * Image load/cache/resize/display routines
 *
 * If FASTJPEG is defined, JPEG images are decoded to
 * a 256 color standardized palette (mwstdpal8). Otherwise,
 * the images are decoded depending on their output
 * components (usually 24bpp).
 *
 * GIF, BMP, JPEG, PPM, PGM, PBM, PNG, XPM and TIFF formats are supported.
 * JHC:  Instead of working with a file, we work with a buffer
 *       (either provided by the user or through mmap).  This
 *	 improves speed, and provides a mechanism by which the
 *	 client can send image data directly to the engine 
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

typedef struct {  /* structure for reading images from buffer   */
	unsigned char *start;	/* The pointer to the beginning of the buffer */
	unsigned long offset;	/* The current offset within the buffer       */
	unsigned long size;	/* The total size of the buffer               */
} buffer_t;
 

static void ComputePitch(int bpp, int width, int *pitch, int *bytesperpixel);
#if defined(HAVE_BMP_SUPPORT)
static int  LoadBMP(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#if defined(HAVE_JPEG_SUPPORT)
static int  LoadJPEG(buffer_t *src, PMWIMAGEHDR pimage, PSD psd,
		MWBOOL fast_grayscale);
#endif
#if defined(HAVE_PNG_SUPPORT)
static int  LoadPNG(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#if defined(HAVE_GIF_SUPPORT)
static int  LoadGIF(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#if defined(HAVE_PNM_SUPPORT)
static int LoadPNM(buffer_t *src, PMWIMAGEHDR pimage);
#endif
#if defined(HAVE_XPM_SUPPORT)
static int LoadXPM(buffer_t *src, PMWIMAGEHDR pimage, PSD psd);
#endif
#if defined(HAVE_TIFF_SUPPORT)
static int LoadTIFF(char *path, PMWIMAGEHDR pimage);
#endif

/*
 * Buffered input functions to replace stdio functions
 */
static void
binit(buffer_t *buffer, void *startdata, int size)
{
	buffer->start = startdata;
	buffer->size = size;
	buffer->offset = 0;
}

static long
bseek(buffer_t *buffer, long offset, int whence)
{
	long new;

	switch(whence) {
	case SEEK_SET:
		if (offset >= buffer->size || offset < 0)
			return -1L;
		buffer->offset = offset;
		break;

	case SEEK_CUR:
		new = buffer->offset + offset;
		if (new >= buffer->size || new < 0)
			return -1L;
		buffer->offset = new;
		break;

	case SEEK_END:
		new = buffer->size - 1 + offset;
		if (new >= buffer->size || new < 0)
			return -1L;
		buffer->offset = new;
		break;

	default:
		return -1L;
	}
	return buffer->offset;
}
   
static int
bread(buffer_t *buffer, void *dest, unsigned long size)
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
 
static int
bgetc(buffer_t *buffer)
{
	if (buffer->offset == buffer->size) 
		return EOF;
	return buffer->start[buffer->offset++];
}
 
static char *
bgets(buffer_t *buffer, char *dest, unsigned int size)
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
 
static int
beof(buffer_t *buffer)
{
	return (buffer->offset == buffer->size);
}
 
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

	binit(&src, buffer, size);
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

	binit(&src, buffer, size);
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
#endif /* defined(HAVE_FILEIO) */

#if defined(HAVE_FILEIO)
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
    return(0);
  }
  
#if HAVE_MMAP
  buffer = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (!buffer) {
    EPRINTF("GdLoadImageFromFile: Couldn't map image %s\n", path);
    close(fd);
    return(0);
  }
#else
  buffer = malloc(s.st_size);
  if (!buffer) {
     EPRINTF("GdLoadImageFromFile: Couldn't load image %s\n", path);
     close(fd);
     return(0);
  }
  
  if (read(fd, buffer, s.st_size) != s.st_size) {
    EPRINTF("GdLoadImageFromFile: Couldn't load image %s\n", path);
    close(fd);
    return(0);
  }
#endif

  binit(&src, buffer, s.st_size);
  id = GdDecodeImage(psd, &src, path, flags);
  
#if HAVE_MMAP
  munmap(buffer, s.st_size);
#else
  free(buffer);
#endif

  close(fd);
  return(id);
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
		loadOK = LoadTIFF(path, pimage);
#endif
#if defined(HAVE_BMP_SUPPORT)
	if (loadOK == 0) 
		loadOK = LoadBMP(src, pimage);
#endif
#if defined(HAVE_GIF_SUPPORT)
	if (loadOK == 0) 
		loadOK = LoadGIF(src, pimage);
#endif
#if defined(HAVE_JPEG_SUPPORT)
	if (loadOK == 0) 
		loadOK = LoadJPEG(src, pimage, psd, flags);
#endif
#if defined(HAVE_PNG_SUPPORT)
	if (loadOK == 0) 
		loadOK = LoadPNG(src, pimage);
#endif
#if defined(HAVE_PNM_SUPPORT)
	if(loadOK == 0)
		loadOK = LoadPNM(src, pimage);
#endif
#if defined(HAVE_XPM_SUPPORT)
	if (loadOK == 0) 
		loadOK = LoadXPM(src, pimage, psd);
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
		ComputePitch(pimage->bpp, width, &image2.pitch,
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
static void
ComputePitch(int bpp, int width, int *pitch, int *bytesperpixel)
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

/*
 * StretchImage - Resize an image
 *
 * Major portions from SDL Simple DirectMedia Layer by Sam Lantinga
 * Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga <slouken@devolution.com>
 * This a stretch blit implementation based on ideas given to me by
 *  Tomasz Cejner - thanks! :)
 */
/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define DEFINE_COPY_ROW(name, type)					\
static void name(type *src, int src_w, type *dst, int dst_w)		\
{									\
	int i;								\
	int pos, inc;							\
	type pixel = 0;							\
									\
	pos = 0x10000;							\
	inc = (src_w << 16) / dst_w;					\
	for ( i=dst_w; i>0; --i ) {					\
		while ( pos >= 0x10000L ) {				\
			pixel = *src++;					\
			pos -= 0x10000L;				\
		}							\
		*dst++ = pixel;						\
		pos += inc;						\
	}								\
}

DEFINE_COPY_ROW(copy_row1, unsigned char)
DEFINE_COPY_ROW(copy_row2, unsigned short)
DEFINE_COPY_ROW(copy_row4, unsigned long)

static void copy_row3(unsigned char *src, int src_w, unsigned char *dst,
	int dst_w)
{
	int i;
	int pos, inc;
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	pos = 0x10000;
	inc = (src_w << 16) / dst_w;
	for ( i=dst_w; i>0; --i ) {
		while ( pos >= 0x10000L ) {
			b = *src++;
			g = *src++;
			r = *src++;
			pos -= 0x10000L;
		}
		*dst++ = b;
		*dst++ = g;
		*dst++ = r;
		pos += inc;
	}
}

/**
 * Perform a stretch blit between two image structs of the same format.
 *
 * @param src Source image.
 * @param srcrect Source rectangle.
 * @param dst Destination image.
 * @param dstrect Destination rectangle.
 */
void
GdStretchImage(PMWIMAGEHDR src, MWCLIPRECT *srcrect, PMWIMAGEHDR dst,
	MWCLIPRECT *dstrect)
{
	int pos, inc;
	int bytesperpixel;
	int dst_maxrow;
	int src_row, dst_row;
	MWUCHAR *srcp = 0;
	MWUCHAR *dstp;
	MWCLIPRECT full_src;
	MWCLIPRECT full_dst;

	if ( src->bytesperpixel != dst->bytesperpixel ) {
		EPRINTF("GdStretchImage: bytesperpixel mismatch\n");
		return;
	}

	/* Verify the blit rectangles */
	if ( srcrect ) {
		if ( (srcrect->x < 0) || (srcrect->y < 0) ||
		     ((srcrect->x+srcrect->width) > src->width) ||
		     ((srcrect->y+srcrect->height) > src->height) ) {
			EPRINTF("GdStretchImage: invalid source rect\n");
			return;
		}
	} else {
		full_src.x = 0;
		full_src.y = 0;
		full_src.width = src->width;
		full_src.height = src->height;
		srcrect = &full_src;
	}
	if ( dstrect ) {
		/* if stretching to nothing, return*/
		if (!dstrect->width || !dstrect->height)
			return;
		if ( (dstrect->x < 0) || (dstrect->y < 0) ||
		     ((dstrect->x+dstrect->width) > dst->width) ||
		     ((dstrect->y+dstrect->height) > dst->height) ) {
			EPRINTF("GdStretchImage: invalid dest rect\n");
			return;
		}
	} else {
		full_dst.x = 0;
		full_dst.y = 0;
		full_dst.width = dst->width;
		full_dst.height = dst->height;
		dstrect = &full_dst;
	}

	/* Set up the data... */
	pos = 0x10000;
	inc = (srcrect->height << 16) / dstrect->height;
	src_row = srcrect->y;
	dst_row = dstrect->y;
	bytesperpixel = dst->bytesperpixel;

	/* Perform the stretch blit */
	for ( dst_maxrow = dst_row+dstrect->height; dst_row<dst_maxrow;
								++dst_row ) {
		dstp = (MWUCHAR *)dst->imagebits + (dst_row*dst->pitch)
				    + (dstrect->x*bytesperpixel);
		while ( pos >= 0x10000L ) {
			srcp = (MWUCHAR *)src->imagebits + (src_row*src->pitch)
				    + (srcrect->x*bytesperpixel);
			++src_row;
			pos -= 0x10000L;
		}

		switch (bytesperpixel) {
		case 1:
			copy_row1(srcp, srcrect->width, dstp, dstrect->width);
			break;
		case 2:
			copy_row2((unsigned short *)srcp, srcrect->width,
				(unsigned short *)dstp, dstrect->width);
			break;
		case 3:
			copy_row3(srcp, srcrect->width, dstp, dstrect->width);
			break;
		case 4:
			copy_row4((unsigned long *)srcp, srcrect->width,
				(unsigned long *)dstp, dstrect->width);
			break;
		}

		pos += inc;
	}
}

#if defined(HAVE_JPEG_SUPPORT)
#include "jpeglib.h"
/*
 * JPEG decompression routine
 *
 * JPEG support must be enabled (see README.txt in contrib/jpeg)
 *
 * SOME FINE POINTS: (from libjpeg)
 * In the below code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.doc for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.doc for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 */
static buffer_t *inptr;

static void
init_source(j_decompress_ptr cinfo)
{
	cinfo->src->next_input_byte = inptr->start;
	cinfo->src->bytes_in_buffer = inptr->size;
}

static void
fill_input_buffer(j_decompress_ptr cinfo)
{
	return;
}

static void
skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	if (num_bytes >= inptr->size)
		return;
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
}

static boolean
resync_to_restart(j_decompress_ptr cinfo, int desired)
{
	return jpeg_resync_to_restart(cinfo, desired);
}

static void
term_source(j_decompress_ptr cinfo)
{
	return;
}

static int
LoadJPEG(buffer_t * src, PMWIMAGEHDR pimage, PSD psd, MWBOOL fast_grayscale)
{
	int i;
	int ret = 2;		/* image load error */
	unsigned char magic[8];
	struct jpeg_source_mgr smgr;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
#if FASTJPEG
	extern MWPALENTRY mwstdpal8[256];
#else
	MWPALENTRY palette[256];
#endif

	/* first determine if JPEG file since decoder will error if not */
	bseek(src, 0, SEEK_SET);
	if (!bread(src, magic, 2))
		return 0;
	if (magic[0] != 0xFF || magic[1] != 0xD8)
		return 0;	/* not JPEG image */

	bread(src, magic, 8);
	if (strncmp(magic+4, "JFIF", 4) != 0)
		return 0;	/* not JPEG image */

	bread(src, 0, SEEK_SET);
	pimage->imagebits = NULL;
	pimage->palette = NULL;

	/* Step 1: allocate and initialize JPEG decompression object */
	/* We set up the normal JPEG error routines. */
	cinfo.err = jpeg_std_error(&jerr);

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2:  Setup the source manager */
	smgr.init_source = (void *) init_source;
	smgr.fill_input_buffer = (void *) fill_input_buffer;
	smgr.skip_input_data = (void *) skip_input_data;
	smgr.resync_to_restart = (void *) resync_to_restart;
	smgr.term_source = (void *) term_source;
	cinfo.src = &smgr;
	inptr = src;

	/* Step 2: specify data source (eg, a file) */
	/* jpeg_stdio_src (&cinfo, fp); */

	/* Step 3: read file parameters with jpeg_read_header() */
	jpeg_read_header(&cinfo, TRUE);
	/* Step 4: set parameters for decompression */
	cinfo.out_color_space = fast_grayscale? JCS_GRAYSCALE: JCS_RGB;
	cinfo.quantize_colors = FALSE;

#if FASTJPEG
	goto fastjpeg;
#endif
	if (!fast_grayscale) {
		if (psd->pixtype == MWPF_PALETTE) {
fastjpeg:
			cinfo.quantize_colors = TRUE;
#if FASTJPEG
			cinfo.actual_number_of_colors = 256;
#else
			/* Get system palette */
			cinfo.actual_number_of_colors = 
				GdGetPalette(psd, 0, psd->ncolors, palette);
#endif
	
			/* Allocate jpeg colormap space */
			cinfo.colormap = (*cinfo.mem->alloc_sarray)
				((j_common_ptr) &cinfo, JPOOL_IMAGE,
			       	(JDIMENSION)cinfo.actual_number_of_colors,
				(JDIMENSION)3);

			/* Set colormap from system palette */
			for(i = 0; i < cinfo.actual_number_of_colors; ++i) {
#if FASTJPEG
				cinfo.colormap[0][i] = mwstdpal8[i].r;
				cinfo.colormap[1][i] = mwstdpal8[i].g;
				cinfo.colormap[2][i] = mwstdpal8[i].b;
#else
				cinfo.colormap[0][i] = palette[i].r;
				cinfo.colormap[1][i] = palette[i].g;
				cinfo.colormap[2][i] = palette[i].b;
#endif
			}
		}
	} else {
		/* Grayscale output asked */
		cinfo.quantize_colors = TRUE;
		cinfo.out_color_space = JCS_GRAYSCALE;
		cinfo.desired_number_of_colors = 256;
	}
	jpeg_calc_output_dimensions(&cinfo);

	pimage->width = cinfo.output_width;
	pimage->height = cinfo.output_height;
	pimage->planes = 1;
#if FASTJPEG
	pimage->bpp = 8;
#else
	pimage->bpp = (fast_grayscale || psd->pixtype == MWPF_PALETTE)?
		8: cinfo.output_components*8;
#endif
	ComputePitch(pimage->bpp, pimage->width, &pimage->pitch,
		&pimage->bytesperpixel);
	pimage->compression = MWIMAGE_RGB;	/* RGB not BGR order*/
	pimage->palsize = (pimage->bpp == 8)? 256: 0;
	pimage->imagebits = malloc(pimage->pitch * pimage->height);
	if(!pimage->imagebits)
		goto err;
	pimage->palette = NULL;

	if(pimage->bpp <= 8) {
		pimage->palette = malloc(256*sizeof(MWPALENTRY));
		if(!pimage->palette)
			goto err;
		if (fast_grayscale) {
			for (i=0; i<256; ++i) {
				MWPALENTRY pe;
				/* FIXME could use static palette here*/
				pe.r = pe.g = pe.b = i;
				pimage->palette[i] = pe;
			}
		} else {
#if FASTJPEG
			/* FASTJPEG case only, normal uses hw palette*/
			for (i=0; i<256; ++i)
				pimage->palette[i] = mwstdpal8[i];
#endif
		}
	}

	/* Step 5: Start decompressor */
	jpeg_start_decompress (&cinfo);

	/* Step 6: while (scan lines remain to be read) */
	while(cinfo.output_scanline < cinfo.output_height) {
		JSAMPROW rowptr[1];
		rowptr[0] = (JSAMPROW)(pimage->imagebits +
			cinfo.output_scanline * pimage->pitch);
		jpeg_read_scanlines (&cinfo, rowptr, 1);
	}
	ret = 1;

err:
	/* Step 7: Finish decompression */
	jpeg_finish_decompress (&cinfo);

	/* Step 8: Release JPEG decompression object */
	jpeg_destroy_decompress (&cinfo);

	/* May want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */
	return ret;
}
#endif /* defined(HAVE_JPEG_SUPPORT)*/

#if defined(HAVE_PNG_SUPPORT)
#include <png.h>
/* png_jmpbuf() macro is not defined prior to libpng-1.0.6*/
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr)	((png_ptr)->jmpbuf)
#endif
/*
 * Load a PNG file.
 * Currently for simplicity we get the PNG library to convert the file to
 * 24 bit RGB format with no alpha channel information even if we could
 * potentially store the image more efficiently by taking note of the image
 * type and depth and acting accordingly. Similarly, > 8 bits per channel,
 * gamma correction, etc. are not supported.
 */

/* This is a quick user defined function to read from the buffer instead of from the file pointer */
static void
png_read_buffer(png_structp pstruct, png_bytep pointer, png_size_t size)
{
	bread(pstruct->io_ptr, pointer, size);
}

static int
LoadPNG(buffer_t * src, PMWIMAGEHDR pimage)
{
	unsigned char hdr[8], **rows;
	png_structp state;
	png_infop pnginfo;
	png_uint_32 width, height;
	int bit_depth, colourtype, i;

	bseek(src, 0L, SEEK_SET);

	if(bread(src, hdr, 8) != 8) return 0;

	if(png_sig_cmp(hdr, 0, 8)) return 0;

	if(!(state = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, 
						NULL, NULL))) goto nomem;

	if(!(pnginfo = png_create_info_struct(state))) {
		png_destroy_read_struct(&state, NULL, NULL);
		goto nomem;
	}

	if(setjmp(png_jmpbuf(state))) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		return 2;
	}

	/* Set up the input function */
	png_set_read_fn(state, src, png_read_buffer);
	/* png_init_io(state, src); */

	png_set_sig_bytes(state, 8);
	png_read_info(state, pnginfo);
	png_get_IHDR(state, pnginfo, &width, &height, &bit_depth, &colourtype,
							NULL, NULL, NULL);

	pimage->width = width;
	pimage->height = height;
	pimage->bpp = 24;
	pimage->planes = 1;
	ComputePitch(pimage->bpp, pimage->width, &pimage->pitch,
						&pimage->bytesperpixel);
	pimage->compression = MWIMAGE_RGB;
        if(!(pimage->imagebits = malloc(pimage->pitch * pimage->height))) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		goto nomem;
        }
        if(!(rows = malloc(pimage->height * sizeof(unsigned char *)))) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		goto nomem;
        }
	for(i = 0; i < pimage->height; i++)
		rows[i] = pimage->imagebits + (i * pimage->pitch);

	png_set_expand(state);
	if(bit_depth == 16)
		png_set_strip_16(state);
	if(colourtype & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(state);
	if(colourtype == PNG_COLOR_TYPE_GRAY ||
			colourtype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(state);

	png_read_image(state, rows);

	png_read_end(state, NULL);
	free(rows);
	png_destroy_read_struct(&state, &pnginfo, NULL);

	return 1;

nomem:
	EPRINTF("LoadPNG: Out of memory\n");
	return 2;
}
#endif /* defined(HAVE_PNG_SUPPORT)*/

#if defined(HAVE_BMP_SUPPORT)
/* BMP stuff*/
#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;
typedef long		LONG;

typedef struct {
	/* BITMAPFILEHEADER*/
	BYTE	bfType[2];
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BMPFILEHEAD;

#define FILEHEADSIZE 14

/* windows style*/
typedef struct {
	/* BITMAPINFOHEADER*/
	DWORD	BiSize;
	DWORD	BiWidth;
	DWORD	BiHeight;
	WORD	BiPlanes;
	WORD	BiBitCount;
	DWORD	BiCompression;
	DWORD	BiSizeImage;
	DWORD	BiXpelsPerMeter;
	DWORD	BiYpelsPerMeter;
	DWORD	BiClrUsed;
	DWORD	BiClrImportant;
} BMPINFOHEAD;

#define INFOHEADSIZE 40

/* os/2 style*/
typedef struct {
	/* BITMAPCOREHEADER*/
	DWORD	bcSize;
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} BMPCOREHEAD;

#define COREHEADSIZE 12

static int	DecodeRLE8(MWUCHAR *buf, buffer_t *src);
static int	DecodeRLE4(MWUCHAR *buf, buffer_t *src);
static void	put4(int b);

/*
 * BMP decoding routine
 */

/* Changed by JHC to allow a buffer instead of a filename */

static int
LoadBMP(buffer_t *src, PMWIMAGEHDR pimage)
{
	int		h, i, compression;
	int		headsize;
	MWUCHAR		*imagebits;
	BMPFILEHEAD	bmpf;
	BMPINFOHEAD	bmpi;
	BMPCOREHEAD	bmpc;
	MWUCHAR 	headbuffer[INFOHEADSIZE];

	bseek(src, 0, SEEK_SET);

	pimage->imagebits = NULL;
	pimage->palette = NULL;

	/* read BMP file header*/
	if (bread(src, &headbuffer, FILEHEADSIZE) != FILEHEADSIZE)
	  return(0);

	bmpf.bfType[0] = headbuffer[0];
	bmpf.bfType[1] = headbuffer[1];

	/* Is it really a bmp file ? */
	if (*(WORD*)&bmpf.bfType[0] != wswap(0x4D42)) /* 'BM' */
		return 0;	/* not bmp image*/

	/*bmpf.bfSize = dwswap(dwread(&headbuffer[2]));*/
	bmpf.bfOffBits = dwswap(dwread(&headbuffer[10]));

	/* Read remaining header size */
	if (bread(src,&headsize,sizeof(DWORD)) != sizeof(DWORD))
		return 0;	/* not bmp image*/
	headsize = dwswap(headsize);

	/* might be windows or os/2 header */
	if(headsize == COREHEADSIZE) {

		/* read os/2 header */
		if(bread(src, &headbuffer, COREHEADSIZE-sizeof(DWORD)) !=
			COREHEADSIZE-sizeof(DWORD))
				return 0;	/* not bmp image*/

		/* Get data */
		bmpc.bcWidth = wswap(*(WORD*)&headbuffer[0]);
		bmpc.bcHeight = wswap(*(WORD*)&headbuffer[2]);
		bmpc.bcPlanes = wswap(*(WORD*)&headbuffer[4]);
		bmpc.bcBitCount = wswap(*(WORD*)&headbuffer[6]);
		
		pimage->width = (int)bmpc.bcWidth;
		pimage->height = (int)bmpc.bcHeight;
		pimage->bpp = bmpc.bcBitCount;
		if (pimage->bpp <= 8)
			pimage->palsize = 1 << pimage->bpp;
		else pimage->palsize = 0;
		compression = BI_RGB;
	} else {
		/* read windows header */
		if(bread(src, &headbuffer, INFOHEADSIZE-sizeof(DWORD)) !=
			INFOHEADSIZE-sizeof(DWORD))
				return 0;	/* not bmp image*/

		/* Get data */
		bmpi.BiWidth = dwswap(*(DWORD*)&headbuffer[0]);
		bmpi.BiHeight = dwswap(*(DWORD*)&headbuffer[4]);
		bmpi.BiPlanes = wswap(*(WORD*)&headbuffer[8]);
		bmpi.BiBitCount = wswap(*(WORD*)&headbuffer[10]);
		bmpi.BiCompression = dwswap(*(DWORD*)&headbuffer[12]);
		bmpi.BiSizeImage = dwswap(*(DWORD*)&headbuffer[16]);
		bmpi.BiXpelsPerMeter = dwswap(*(DWORD*)&headbuffer[20]);
		bmpi.BiYpelsPerMeter = dwswap(*(DWORD*)&headbuffer[24]);
		bmpi.BiClrUsed = dwswap(*(DWORD*)&headbuffer[28]);
		bmpi.BiClrImportant = dwswap(*(DWORD*)&headbuffer[32]);

		pimage->width = (int)bmpi.BiWidth;
		pimage->height = (int)bmpi.BiHeight;
		pimage->bpp = bmpi.BiBitCount;
		pimage->palsize = (int)bmpi.BiClrUsed;
		if (pimage->palsize > 256)
			pimage->palsize = 0;
		else if(pimage->palsize == 0 && pimage->bpp <= 8)
			pimage->palsize = 1 << pimage->bpp;
		compression = bmpi.BiCompression;
	}
	pimage->compression = MWIMAGE_BGR;	/* right side up, BGR order*/
	pimage->planes = 1;

	/* currently only 1, 4, 8 and 24 bpp bitmaps*/
	if(pimage->bpp > 8 && pimage->bpp != 24) {
		EPRINTF("LoadBMP: image bpp not 1, 4, 8 or 24\n");
		return 2;	/* image loading error*/
	}

	/* compute byte line size and bytes per pixel*/
	ComputePitch(pimage->bpp, pimage->width, &pimage->pitch,
		&pimage->bytesperpixel);

	/* Allocate image */
	if( (pimage->imagebits = malloc(pimage->pitch*pimage->height)) == NULL)
		goto err;
	if( (pimage->palette = malloc(256*sizeof(MWPALENTRY))) == NULL)
		goto err;

	/* get colormap*/
	if(pimage->bpp <= 8) {
		for(i=0; i<pimage->palsize; i++) {
			pimage->palette[i].b = bgetc(src);
			pimage->palette[i].g = bgetc(src);
			pimage->palette[i].r = bgetc(src);
			if(headsize != COREHEADSIZE)
				bgetc(src);
		}
	}

	/* decode image data*/
	bseek(src, bmpf.bfOffBits, SEEK_SET);

	h = pimage->height;
	/* For every row ... */
	while (--h >= 0) {
		/* turn image rightside up*/
		imagebits = pimage->imagebits + h*pimage->pitch;

		/* Get row data from file */
		if(compression == BI_RLE8) {
			if(!DecodeRLE8(imagebits, src))
				break;
		} else if(compression == BI_RLE4) {
			if(!DecodeRLE4(imagebits, src))
				break;
		} else {
			if(bread(src, imagebits, pimage->pitch) !=
				pimage->pitch)
					goto err;
		}
	}
	return 1;		/* bmp image ok*/
	
err:
	EPRINTF("LoadBMP: image loading error\n");
	if(pimage->imagebits)
		free(pimage->imagebits);
	if(pimage->palette)
		free(pimage->palette);
	return 2;		/* bmp image error*/
}

/*
 * Decode one line of RLE8, return 0 when done with all bitmap data
 */
static int
DecodeRLE8(MWUCHAR *buf, buffer_t *src)
{
	int		c, n;
	MWUCHAR *	p = buf;

	for( ;;) {
	  switch( n = bgetc(src)) {
	  case EOF:
	    return( 0);
	  case 0:			/* 0 = escape*/
	    switch( n = bgetc(src)) {
	    case 0: 	/* 0 0 = end of current scan line*/
	      return( 1);
	    case 1:		/* 0 1 = end of data*/
	      return( 1);
	    case 2:		/* 0 2 xx yy delta mode NOT SUPPORTED*/
	      (void)bgetc(src);
	      (void)bgetc(src);
	      continue;
	    default:	/* 0 3..255 xx nn uncompressed data*/
	      for( c=0; c<n; c++)
		*p++ = bgetc(src);
	      if( n & 1)
		(void)bgetc(src);
	      continue;
	    }
	  default:
	    c = bgetc(src);
	    while( n--)
	      *p++ = c;
	    continue;
	  }
	}
}

/*
 * Decode one line of RLE4, return 0 when done with all bitmap data
 */
static MWUCHAR *p;
static int	once;

static void
put4(int b)
{
	static int	last;

	last = (last << 4) | b;
	if( ++once == 2) {
		*p++ = last;
		once = 0;
	}
}
	
static int
DecodeRLE4(MWUCHAR *buf, buffer_t *src)
{
	int		c, n, c1, c2;

	p = buf;
	once = 0;
	c1 = 0;

	for( ;;) {
	  switch( n = bgetc(src)) {
	  case EOF:
	    return( 0);
	  case 0:			/* 0 = escape*/
	    switch( n = bgetc(src)) {
	    case 0: 	/* 0 0 = end of current scan line*/
	      if( once)
		put4( 0);
	      return( 1);
	    case 1:		/* 0 1 = end of data*/
	      if( once)
		put4( 0);
	      return( 1);
	    case 2:		/* 0 2 xx yy delta mode NOT SUPPORTED*/
	      (void)bgetc(src);
	      (void)bgetc(src);
	      continue;
	    default:	/* 0 3..255 xx nn uncompressed data*/
	      c2 = (n+3) & ~3;
	      for( c=0; c<c2; c++) {
		if( (c & 1) == 0)
		  c1 = bgetc(src);
		if( c < n)
		  put4( (c1 >> 4) & 0x0f);
		c1 <<= 4;
	      }
	      continue;
	    }
	  default:
	    c = bgetc(src);
	    c1 = (c >> 4) & 0x0f;
	    c2 = c & 0x0f;
	    for( c=0; c<n; c++)
	      put4( (c&1)? c2: c1);
	    continue;
	  }
	}
}
#endif /* defined(HAVE_BMP_SUPPORT)*/

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

#if defined(HAVE_GIF_SUPPORT)
/* Code for GIF decoding has been adapted from XPaint:                   */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993 David Koblas.			       | */
/* | Copyright 1996 Torsten Martinsen.				       | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.	       | */
/* +-------------------------------------------------------------------+ */
/* Portions Copyright (C) 1999  Sam Lantinga*/
/* Adapted for use in SDL by Sam Lantinga -- 7/20/98 */
/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* GIF stuff*/
/*
 * GIF decoding routine
 */
#define	MAXCOLORMAPSIZE		256
#define	MAX_LWZ_BITS		12
#define INTERLACE		0x40
#define LOCALCOLORMAP		0x80

#define CM_RED		0
#define CM_GREEN	1
#define CM_BLUE		2

#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))
#define	ReadOK(src,buffer,len)	bread(src, buffer, len)
#define LM_to_uint(a,b)		(((b)<<8)|(a))

struct {
    unsigned int Width;
    unsigned int Height;
    unsigned char ColorMap[3][MAXCOLORMAPSIZE];
    unsigned int BitPixel;
    unsigned int ColorResolution;
    unsigned int Background;
    unsigned int AspectRatio;
    int GrayScale;
} GifScreen;

static struct {
    int transparent;
    int delayTime;
    int inputFlag;
    int disposal;
} Gif89;

static int ReadColorMap(buffer_t *src, int number,
		unsigned char buffer[3][MAXCOLORMAPSIZE], int *flag);
static int DoExtension(buffer_t *src, int label);
static int GetDataBlock(buffer_t *src, unsigned char *buf);
static int GetCode(buffer_t *src, int code_size, int flag);
static int LWZReadByte(buffer_t *src, int flag, int input_code_size);
static int ReadImage(buffer_t *src, PMWIMAGEHDR pimage, int len, int height, int,
		unsigned char cmap[3][MAXCOLORMAPSIZE],
		int gray, int interlace, int ignore);

static int
LoadGIF(buffer_t *src, PMWIMAGEHDR pimage)
{
    unsigned char buf[16];
    unsigned char c;
    unsigned char localColorMap[3][MAXCOLORMAPSIZE];
    int grayScale;
    int useGlobalColormap;
    int bitPixel;
    int imageCount = 0;
    char version[4];
    int imageNumber = 1;
    int ok = 0;

    bseek(src, 0, SEEK_SET);

    pimage->imagebits = NULL;
    pimage->palette = NULL;

    if (!ReadOK(src, buf, 6))
        return 0;		/* not gif image*/
    if (strncmp((char *) buf, "GIF", 3) != 0)
        return 0;
    strncpy(version, (char *) buf + 3, 3);
    version[3] = '\0';

    if (strcmp(version, "87a") != 0 && strcmp(version, "89a") != 0) {
	EPRINTF("LoadGIF: GIF version number not 87a or 89a\n");
        return 2;		/* image loading error*/
    }
    Gif89.transparent = -1;
    Gif89.delayTime = -1;
    Gif89.inputFlag = -1;
    Gif89.disposal = 0;

    if (!ReadOK(src, buf, 7)) {
	EPRINTF("LoadGIF: bad screen descriptor\n");
        return 2;		/* image loading error*/
    }
    GifScreen.Width = LM_to_uint(buf[0], buf[1]);
    GifScreen.Height = LM_to_uint(buf[2], buf[3]);
    GifScreen.BitPixel = 2 << (buf[4] & 0x07);
    GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
    GifScreen.Background = buf[5];
    GifScreen.AspectRatio = buf[6];

    if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
	if (ReadColorMap(src, GifScreen.BitPixel, GifScreen.ColorMap,
			 &GifScreen.GrayScale)) {
	    EPRINTF("LoadGIF: bad global colormap\n");
            return 2;		/* image loading error*/
	}
    }

    do {
	if (!ReadOK(src, &c, 1)) {
	    EPRINTF("LoadGIF: EOF on image data\n");
            goto done;
	}
	if (c == ';') {		/* GIF terminator */
	    if (imageCount < imageNumber) {
		EPRINTF("LoadGIF: no image %d of %d\n", imageNumber,imageCount);
                goto done;
	    }
	}
	if (c == '!') {		/* Extension */
	    if (!ReadOK(src, &c, 1)) {
		EPRINTF("LoadGIF: EOF on extension function code\n");
                goto done;
	    }
	    DoExtension(src, c);
	    continue;
	}
	if (c != ',') {		/* Not a valid start character */
	    continue;
	}
	++imageCount;

	if (!ReadOK(src, buf, 9)) {
	    EPRINTF("LoadGIF: bad image size\n");
            goto done;
	}
	useGlobalColormap = !BitSet(buf[8], LOCALCOLORMAP);

	bitPixel = 1 << ((buf[8] & 0x07) + 1);

	if (!useGlobalColormap) {
	    if (ReadColorMap(src, bitPixel, localColorMap, &grayScale)) {
		EPRINTF("LoadGIF: bad local colormap\n");
                goto done;
	    }
	    ok = ReadImage(src, pimage, LM_to_uint(buf[4], buf[5]),
			      LM_to_uint(buf[6], buf[7]),
			      bitPixel, localColorMap, grayScale,
			      BitSet(buf[8], INTERLACE),
			      imageCount != imageNumber);
	} else {
	    ok = ReadImage(src, pimage, LM_to_uint(buf[4], buf[5]),
			      LM_to_uint(buf[6], buf[7]),
			      GifScreen.BitPixel, GifScreen.ColorMap,
			      GifScreen.GrayScale, BitSet(buf[8], INTERLACE),
			      imageCount != imageNumber);
	}
    } while (ok == 0);

    /* set transparent color, if any*/
    pimage->transcolor = Gif89.transparent;

    if (ok)
	    return 1;		/* image load ok*/

done:
    if (pimage->imagebits)
	    free(pimage->imagebits);
    if (pimage->palette)
	    free(pimage->palette);
    return 2;			/* image load error*/
}

static int
ReadColorMap(buffer_t *src, int number, unsigned char buffer[3][MAXCOLORMAPSIZE],
    int *gray)
{
    int i;
    unsigned char rgb[3];
    int flag;

    flag = TRUE;

    for (i = 0; i < number; ++i) {
	if (!ReadOK(src, rgb, sizeof(rgb)))
	    return 1;
	buffer[CM_RED][i] = rgb[0];
	buffer[CM_GREEN][i] = rgb[1];
	buffer[CM_BLUE][i] = rgb[2];
	flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

#if 0
    if (flag)
	*gray = (number == 2) ? PBM_TYPE : PGM_TYPE;
    else
	*gray = PPM_TYPE;
#else
    *gray = 0;
#endif

    return FALSE;
}

static int
DoExtension(buffer_t *src, int label)
{
    static unsigned char buf[256];

    switch (label) {
    case 0x01:			/* Plain Text Extension */
	break;
    case 0xff:			/* Application Extension */
	break;
    case 0xfe:			/* Comment Extension */
	while (GetDataBlock(src, (unsigned char *) buf) != 0);
	return FALSE;
    case 0xf9:			/* Graphic Control Extension */
	GetDataBlock(src, (unsigned char *) buf);
	Gif89.disposal = (buf[0] >> 2) & 0x7;
	Gif89.inputFlag = (buf[0] >> 1) & 0x1;
	Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
	if ((buf[0] & 0x1) != 0)
	    Gif89.transparent = buf[3];

	while (GetDataBlock(src, (unsigned char *) buf) != 0);
	return FALSE;
    default:
	break;
    }

    while (GetDataBlock(src, (unsigned char *) buf) != 0);

    return FALSE;
}

static int ZeroDataBlock = FALSE;

static int
GetDataBlock(buffer_t *src, unsigned char *buf)
{
    unsigned char count;

    if (!ReadOK(src, &count, 1))
	return -1;
    ZeroDataBlock = count == 0;

    if ((count != 0) && (!ReadOK(src, buf, count)))
	return -1;
    return count;
}

static int
GetCode(buffer_t *src, int code_size, int flag)
{
    static unsigned char buf[280];
    static int curbit, lastbit, done, last_byte;
    int i, j, ret;
    unsigned char count;

    if (flag) {
	curbit = 0;
	lastbit = 0;
	done = FALSE;
	return 0;
    }
    if ((curbit + code_size) >= lastbit) {
	if (done) {
	    if (curbit >= lastbit)
		EPRINTF("LoadGIF: bad decode\n");
	    return -1;
	}
	buf[0] = buf[last_byte - 2];
	buf[1] = buf[last_byte - 1];

	if ((count = GetDataBlock(src, &buf[2])) == 0)
	    done = TRUE;

	last_byte = 2 + count;
	curbit = (curbit - lastbit) + 16;
	lastbit = (2 + count) * 8;
    }
    ret = 0;
    for (i = curbit, j = 0; j < code_size; ++i, ++j)
	ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

    curbit += code_size;

    return ret;
}

static int
LWZReadByte(buffer_t *src, int flag, int input_code_size)
{
    int code, incode;
    register int i;
    static int fresh = FALSE;
    static int code_size, set_code_size;
    static int max_code, max_code_size;
    static int firstcode, oldcode;
    static int clear_code, end_code;
    static int table[2][(1 << MAX_LWZ_BITS)];
    static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;

    if (flag) {
	set_code_size = input_code_size;
	code_size = set_code_size + 1;
	clear_code = 1 << set_code_size;
	end_code = clear_code + 1;
	max_code_size = 2 * clear_code;
	max_code = clear_code + 2;

	GetCode(src, 0, TRUE);

	fresh = TRUE;

	for (i = 0; i < clear_code; ++i) {
	    table[0][i] = 0;
	    table[1][i] = i;
	}
	for (; i < (1 << MAX_LWZ_BITS); ++i)
	    table[0][i] = table[1][0] = 0;

	sp = stack;

	return 0;
    } else if (fresh) {
	fresh = FALSE;
	do {
	    firstcode = oldcode = GetCode(src, code_size, FALSE);
	} while (firstcode == clear_code);
	return firstcode;
    }
    if (sp > stack)
	return *--sp;

    while ((code = GetCode(src, code_size, FALSE)) >= 0) {
	if (code == clear_code) {
	    for (i = 0; i < clear_code; ++i) {
		table[0][i] = 0;
		table[1][i] = i;
	    }
	    for (; i < (1 << MAX_LWZ_BITS); ++i)
		table[0][i] = table[1][i] = 0;
	    code_size = set_code_size + 1;
	    max_code_size = 2 * clear_code;
	    max_code = clear_code + 2;
	    sp = stack;
	    firstcode = oldcode = GetCode(src, code_size, FALSE);
	    return firstcode;
	} else if (code == end_code) {
	    int count;
	    unsigned char buf[260];

	    if (ZeroDataBlock)
		return -2;

	    while ((count = GetDataBlock(src, buf)) > 0);

	    if (count != 0) {
		/*
		 * EPRINTF("missing EOD in data stream (common occurence)");
		 */
	    }
	    return -2;
	}
	incode = code;

	if (code >= max_code) {
	    *sp++ = firstcode;
	    code = oldcode;
	}
	while (code >= clear_code) {
	    *sp++ = table[1][code];
	    if (code == table[0][code])
		EPRINTF("LoadGIF: circular table entry\n");
	    code = table[0][code];
	}

	*sp++ = firstcode = table[1][code];

	if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
	    table[0][code] = oldcode;
	    table[1][code] = firstcode;
	    ++max_code;
	    if ((max_code >= max_code_size) &&
		(max_code_size < (1 << MAX_LWZ_BITS))) {
		max_code_size *= 2;
		++code_size;
	    }
	}
	oldcode = incode;

	if (sp > stack)
	    return *--sp;
    }
    return code;
}

static int
ReadImage(buffer_t* src, PMWIMAGEHDR pimage, int len, int height, int cmapSize,
	  unsigned char cmap[3][MAXCOLORMAPSIZE],
	  int gray, int interlace, int ignore)
{
    unsigned char c;
    int i, v;
    int xpos = 0, ypos = 0, pass = 0;

    /*
     *	Initialize the compression routines
     */
    if (!ReadOK(src, &c, 1)) {
	EPRINTF("LoadGIF: EOF on image data\n");
	return 0;
    }
    if (LWZReadByte(src, TRUE, c) < 0) {
	EPRINTF("LoadGIF: error reading image\n");
	return 0;
    }

    /*
     *	If this is an "uninteresting picture" ignore it.
     */
    if (ignore) {
	while (LWZReadByte(src, FALSE, c) >= 0);
	return 0;
    }
    /*image = ImageNewCmap(len, height, cmapSize);*/
    pimage->width = len;
    pimage->height = height;
    pimage->planes = 1;
    pimage->bpp = 8;
    ComputePitch(8, len, &pimage->pitch, &pimage->bytesperpixel);
    pimage->compression = 0;
    pimage->palsize = cmapSize;
    pimage->palette = malloc(256*sizeof(MWPALENTRY));
    pimage->imagebits = malloc(height*pimage->pitch);
    if(!pimage->imagebits || !pimage->palette)
	    return 0;

    for (i = 0; i < cmapSize; i++) {
	/*ImageSetCmap(image, i, cmap[CM_RED][i],
		     cmap[CM_GREEN][i], cmap[CM_BLUE][i]);*/
	pimage->palette[i].r = cmap[CM_RED][i];
	pimage->palette[i].g = cmap[CM_GREEN][i];
	pimage->palette[i].b = cmap[CM_BLUE][i];
    }

    while ((v = LWZReadByte(src, FALSE, c)) >= 0) {
	pimage->imagebits[ypos * pimage->pitch + xpos] = v;

	++xpos;
	if (xpos == len) {
	    xpos = 0;
	    if (interlace) {
		switch (pass) {
		case 0:
		case 1:
		    ypos += 8;
		    break;
		case 2:
		    ypos += 4;
		    break;
		case 3:
		    ypos += 2;
		    break;
		}

		if (ypos >= height) {
		    ++pass;
		    switch (pass) {
		    case 1:
			ypos = 4;
			break;
		    case 2:
			ypos = 2;
			break;
		    case 3:
			ypos = 1;
			break;
		    default:
			goto fini;
		    }
		}
	    } else {
		++ypos;
	    }
	}
	if (ypos >= height)
	    break;
    }

fini:
    return 1;
}
#endif /* defined(HAVE_GIF_SUPPORT)*/

#if defined(HAVE_PNM_SUPPORT)
enum {
	PNM_TYPE_NOTPNM,
	PNM_TYPE_PBM,
	PNM_TYPE_PGM,
	PNM_TYPE_PPM
};
static int LoadPNM(buffer_t *src, PMWIMAGEHDR pimage)
{
	char buf[256], *p;
	int type = PNM_TYPE_NOTPNM, binary = 0, gothdrs = 0, scale = 0;
	int ch, x = 0, y = 0, i, n, mask, col1, col2, col3;

	bseek(src, 0L, SEEK_SET);

	if(!bgets(src,buf, 4)) return 0;

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

	if(type == PNM_TYPE_NOTPNM) return 0;

	n = 0;
	while((p = bgets(src, buf, 256))) {
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
					EPRINTF("LoadPNM: PPM files must be "
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
		EPRINTF("LoadPNM: bad image headers\n");
		if(pimage->palette) free(pimage->palette);
		return 2;
	}

	pimage->planes = 1;
	ComputePitch(pimage->bpp, pimage->width, &pimage->pitch,
						&pimage->bytesperpixel);
	pimage->compression = MWIMAGE_RGB;
	if(!(pimage->imagebits = malloc(pimage->pitch * pimage->height))) {
		EPRINTF("LoadPNM: couldn't allocate memory for image\n");
		if(pimage->palette) free(pimage->palette);
		return 2;
	}

	p = pimage->imagebits;

	if(type == PNM_TYPE_PBM) {
		if(binary) {
			x = 0;
			y = 0;
			while((ch = bgetc(src)) != EOF) {
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
			while((ch = bgetc(src)) != EOF) {
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
					if((ch = bgetc(src)) == EOF)
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
					if(((col1 = bgetc(src)) == EOF) ||
					 	((col2 = bgetc(src)) == EOF) ||
					 	((col3 = bgetc(src)) == EOF))
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
	EPRINTF("LoadPNM: bad image data\n");
	free(pimage->imagebits);
	if(pimage->palette) free(pimage->palette);
	return 2;
}
#endif /* defined(HAVE_PNM_SUPPORT) */

#if defined(HAVE_XPM_SUPPORT)
struct xpm_cmap {
  char mapstr[3];
  long palette_entry;
  long color;
  struct xpm_cmap *next;
};

 
static long XPM_parse_color(char *color)
{
  /* This will parse the string into a color value of some sort */

  if (color[0] != '#')
    {
      if (!strcmp(color, "None"))
	return(-1); /* Transparent */
      else
	return(0); /* If its an X color, then we bail */
    }
  else
    {
      /* This is ugly! */

      char *sptr = color + 1;
      char rstr[5], gstr[5], bstr[5];
      long r,g,b;

      switch(strlen(sptr))
	{
	case 6:
	  return(strtol(sptr, NULL, 16));

	case 9: /* RRRGGGBBB */
	  strncpy(rstr, sptr, 3);
	  strncpy(gstr, sptr + 3, 3);
	  strncpy(bstr, sptr + 6, 3);

	  rstr[3] = 0;
	  gstr[3] = 0;
	  bstr[3] = 0;

	  r = strtol(rstr, NULL, 16) >> 4;
	  g = strtol(gstr, NULL, 16) >> 4;
	  b = strtol(bstr, NULL, 16) >> 4;

	  return( (long) ( r << 16 | g << 8 | b));

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
	  
	  return( (long) ( (r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF)));
	}
    }

  return(0);
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
#define XPM_TRANSCOLOR 0x01000000

static int LoadXPM(buffer_t *src, PMWIMAGEHDR pimage, PSD psd) 
{
  struct xpm_cmap *colorheap = 0;  /* A "heap" of color structs */
  struct xpm_cmap *colormap[256];  /* A quick hash of 256 spots for colors */
  
  unsigned char *imageptr = 0;

  MWSCREENINFO sinfo;

  char xline[300];
  char dline[300];

  char *c;
  int a;

  int col, row, colors, cpp;
  int in_color = 0;
  int read_xline = 0;

  int status = LOAD_HEADER;

  /* Very first thing, get the screen info */
  GdGetScreenInfo(psd, &sinfo);

  for(a = 0; a < 256; a++)
    colormap[a] = 0;

  pimage->imagebits = NULL;
  pimage->palette = NULL;

  /* Start over at the beginning with the file */
  bseek(src, 0, SEEK_SET);
 
  bgets(src, xline, 300);
 
  /* Chop the EOL */
  xline[strlen(xline) - 1] = 0;

  /* Check the magic */
  if (strncmp(xline, XPM_MAGIC, sizeof(XPM_MAGIC))) return(0);

  while(!beof(src))
    {
      /* Get the next line from the file */
      bgets(src,xline, 300);
      xline[strlen(xline) - 1] = 0;

      /* Check it out */
      if (xline[0] == '/' && xline[1] == '*') /* Comment */
	continue;

      if (xline[0] != '\"')
	continue;

      /* remove the quotes from the line */
      for(c = xline + 1, a = 0; *c != '\"' && *c != 0; c++, a++)
	dline[a] = *c;

      dline[a] = 0;

      /* Is it the header? */
      if (status == LOAD_HEADER)
	{
	  sscanf(dline, "%i %i %i %i", &col, &row, &colors, &cpp);

	  pimage->width = col;
	  pimage->height = row;
	  pimage->planes = 1;

	  if (sinfo.bpp <= 8)
	    {
	      pimage->bpp = sinfo.bpp;
	      pimage->compression = 0;
	      pimage->transcolor = -1;
	    }
	  else
	    {
	      pimage->bpp = 32;
	      pimage->transcolor = XPM_TRANSCOLOR;
	      pimage->compression = MWIMAGE_BGR;	  
	    }

	  pimage->palsize = colors;

	  ComputePitch(pimage->bpp, col, &pimage->pitch, &pimage->bytesperpixel);

	  pimage->imagebits = malloc(pimage->pitch * pimage->height); 
	  imageptr = (unsigned char *) pimage->imagebits;

	  /* Allocate enough room for all the colors */
	  colorheap = (struct xpm_cmap *) malloc(colors * sizeof(struct xpm_cmap));

	  /* Allocate the palette space (if required) */

	  if (sinfo.bpp <= 8)
	      pimage->palette = malloc(256*sizeof(MWPALENTRY));

	  if (!colorheap)
	    {
	      EPRINTF("Couldn't allocate any memory for the colors\n");
	      return(0);
	    }

	  status = LOAD_COLORS;
	  in_color = 0;
	  continue;
	}

      /* Are we in load colors? */
      if (status == LOAD_COLORS)
	{
	  struct xpm_cmap *n;

	  char tstr[5];
	  char cstr[256];

	  unsigned char m;

	  c = dline;

	  /* Go at at least 1 charater, and then count until we have
	     two spaces in a row */

	  strncpy(tstr, c, cpp);

	  c += cpp;	  
	  for(; *c == '\t' || *c == ' '; c++); /* Skip over whitespace */

	  /* FIXME: We assume that a 'c' follows.  What if it doesn't? */
	  c +=2;
	  
	  tstr[cpp] = 0;

	  /* Now we put it into the array for easy lookup   */
	  /* We base it off the first charater, even though */
	  /* there may be up to 4                           */

	  m = tstr[0];
	  
	  if (colormap[m])
	    {
	      n = colormap[m];
	      
	      while(n->next) n = n->next;
	      n->next = &colorheap[in_color];
	      n = n->next;
	    }
	  else
	    {
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

	  if (sinfo.bpp <= 8)
	    {
	      if (n->color == -1)
		{
		  pimage->transcolor = in_color;
		  n->color = -1;
		}
		
	      pimage->palette[in_color].r = (n->color >> 16) & 0xFF;
	      pimage->palette[in_color].g = (n->color >> 8) & 0xFF;
	      pimage->palette[in_color].b = n->color & 0xFF;  
	    }
	  else
	    {
	      if (n->color == -1) {
		n->color = XPM_TRANSCOLOR;
	      }
	    }

	  if (++in_color == colors)
	    {
	      read_xline = 0;
	      status = LOAD_PIXELS;
	    }

	  continue;	 
	}
      
      if (status == LOAD_PIXELS)
      {
	int bytecount = 0;
	int bitcount = 0;
	long dwordcolor = 0;
	int i;
	char pxlstr[3];

	c = dline;
	
	while(*c)
	  {
	    unsigned char z = 0;

	    if (cpp == 1)
	      {		
		z = *c;
		
		if (!colormap[z])
		  {
		    EPRINTF("No color entry for (%c)\n", z);
		    return(0);
		  }

		if (sinfo.bpp <= 8)
		  dwordcolor = (long) colormap[z]->palette_entry;
		else
		  dwordcolor = colormap[z]->color;	

		c++;
	      }
	    else
	      {
		struct xpm_cmap *n; 

		/* We grab the largest possible, and then compare */

		strncpy(pxlstr, c, cpp);
		z = pxlstr[0];

		if (!colormap[z])
		  {
		    EPRINTF("No color entry for (%s)\n", pxlstr);
		    return(0);
		  }
			
		n = colormap[z];

		while(n)
		  {
		    if (!strncmp(n->mapstr, pxlstr, cpp))
		      break;

		    n = n->next;
		  }

		if (!n)
		  {
		    EPRINTF("No color found for (%s)\n", pxlstr);	    
		    return(0);
		  }

		if (sinfo.bpp <= 8)
		  dwordcolor = (long) n->palette_entry;
		else
		  dwordcolor = n->color;
		c += cpp;
	      }

	    /* 
	     * This ugly thing is needed to ensure that we
	     * work well in all modes.
	     */
	    switch(sinfo.bpp)
	      {
	      case 2:
		if (bitcount == 0)
		  imageptr[0] = 0;
		
		imageptr[0] |= (dwordcolor & 0x3) << (4 - bitcount);
		bitcount++;
		
		if (bitcount == 4)
		  {
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
		
		if (bitcount == 2)
		  {
		    imageptr++;
		    bytecount += pimage->bytesperpixel;
		    bitcount = 0;
		  }
		
		break;

	      case 8:
	      case 16:
	      case 24:
	      case 32:
	      
		for(i = 0; i < pimage->bytesperpixel; i++)
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
	  for(i = 0; i < (pimage->pitch - bytecount); i++)
	    *imageptr++ = 0x00;

	read_xline++;
	
	if (read_xline == row)
	  status = LOAD_DONE;

	continue;
      }
    }

  free(colorheap);

  if (status != LOAD_DONE)
    return(-1);
  return(1);
}
#endif /* defined(HAVE_XPM_SUPPORT)*/

#if defined(HAVE_TIFF_SUPPORT)
#include <tiffio.h>

static int
LoadTIFF(char *path, PMWIMAGEHDR pimage)
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
	pimage->compression = MWIMAGE_RGB | MWIMAGE_UPSIDEDOWN;

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
	EPRINTF("LoadTIFF: image loading error\n");
	if (tif)
		TIFFClose(tif);
	if(pimage->imagebits)
		free(pimage->imagebits);
	if(pimage->palette)
		free(pimage->palette);
	return 2;		/* image error*/
}
#endif /* defined(HAVE_TIFF_SUPPORT)*/

#endif /* MW_FEATURE_IMAGES - whole file */
