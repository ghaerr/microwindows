/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Martin Jolicoeur <martinj@visuaide.com>
 * Portions Copyright (c) Independant JPEG group (ijg)
 *
 * Image decode routine for JPEG files
 *
 * JPEG support must be enabled (see README.txt in contrib/jpeg)
 *
 * If FASTJPEG is defined, JPEG images are decoded to
 * a 256 color standardized palette (mwstdpal8). Otherwise,
 * the images are decoded depending on their output
 * components (usually 24bpp).
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

#if MW_FEATURE_IMAGES && defined(HAVE_JPEG_SUPPORT)
#include "jpeglib.h"

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

int
GdDecodeJPEG(buffer_t * src, PMWIMAGEHDR pimage, PSD psd, MWBOOL fast_grayscale)
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
	GdImageBufferSeekTo(src, 0UL);
	if (GdImageBufferRead(src, magic, 2) != 2 || magic[0] != 0xFF || magic[1] != 0xD8)
		return 0;	/* not JPEG image */

	if (GdImageBufferRead(src, magic, 8) != 8
	 || (strncmp((char *)&magic[4], "JFIF", 4) != 0 && strncmp((char *)&magic[4], "Exif", 4) != 0))
		return 0;	/* not JPEG image */

	GdImageBufferSeekTo(src, 0);
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
	GdComputeImagePitch(pimage->bpp, pimage->width, &pimage->pitch,
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
#endif /* MW_FEATURE_IMAGES && defined(HAVE_JPEG_SUPPORT)*/
