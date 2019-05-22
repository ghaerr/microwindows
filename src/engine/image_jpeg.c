/*
 * Copyright (c) 2000, 2001, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Martin Jolicoeur <martinj@visuaide.com>
 * Portions Copyright (c) Independant JPEG group (ijg)
 *
 * Image decode routine for JPEG files
 *
 * If USE_STD_PALETTE=1 set, JPEG images are decoded to
 * a 256 color standardized palette (mwstdpal8). Otherwise,
 * the images are decoded depending on their output
 * components (usually 24bpp), unless running in palette mode,
 * where a pal8 image is forced.
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
#include <string.h>
#include "uni_std.h"
#include "device.h"
#include "../drivers/genmem.h"

#if MW_FEATURE_IMAGES && HAVE_JPEG_SUPPORT

#define USE_STD_PALETTE		0		/* =1 to decode pal8 to standard 256 color palette*/

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

PSD
GdDecodeJPEG(buffer_t * src, MWBOOL fast_grayscale)
{
	int i;
	unsigned char magic[8];
	PSD pmd = NULL;
	int bpp, data_format, palsize;
	struct jpeg_source_mgr smgr;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
#if USE_STD_PALETTE
	extern MWPALENTRY mwstdpal8[256];
#else
	MWPALENTRY palette[256];
#endif

	/* first determine if JPEG file since decoder will error if not */
	GdImageBufferSeekTo(src, 0UL);
	if (GdImageBufferRead(src, magic, 2) != 2 || magic[0] != 0xFF || magic[1] != 0xD8)
		return NULL;	/* not JPEG image */

	if (GdImageBufferRead(src, magic, 8) != 8 ||
	    (strncmp((char *)&magic[4], "JFIF", 4) != 0 && strncmp((char *)&magic[4], "Exif", 4) != 0))
		return NULL;	/* not JPEG image */

	GdImageBufferSeekTo(src, 0);

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

	if (!fast_grayscale) {
		/* if running in palette mode, force pal8 output*/
		if (scrdev.pixtype == MWPF_PALETTE) {
			cinfo.quantize_colors = TRUE;
#if USE_STD_PALETTE
			cinfo.actual_number_of_colors = 256;
#else
			/* Use current system palette for decode*/
			cinfo.actual_number_of_colors = GdGetPalette(&scrdev, 0, scrdev.ncolors, palette);
#endif
	
			/* Allocate jpeg colormap space */
			cinfo.colormap = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE,
			       	(JDIMENSION)cinfo.actual_number_of_colors, (JDIMENSION)3);

			for(i = 0; i < cinfo.actual_number_of_colors; ++i) {
#if USE_STD_PALETTE
				/* set colormap from standard palette*/
				cinfo.colormap[0][i] = mwstdpal8[i].r;
				cinfo.colormap[1][i] = mwstdpal8[i].g;
				cinfo.colormap[2][i] = mwstdpal8[i].b;
#else
				/* Set colormap from system palette*/
				cinfo.colormap[0][i] = palette[i].r;
				cinfo.colormap[1][i] = palette[i].g;
				cinfo.colormap[2][i] = palette[i].b;
#endif
			}
		}
	} else {
		/* 256 shade grayscale output*/
		cinfo.quantize_colors = TRUE;
		cinfo.out_color_space = JCS_GRAYSCALE;
		cinfo.desired_number_of_colors = 256;
	}
	jpeg_calc_output_dimensions(&cinfo);

	bpp = cinfo.output_components*8;
	switch (bpp) {
	case 24:
		data_format = MWIF_RGB888;
		break;
	case 8:
		data_format = MWIF_PAL8;
		break;
	default:
		EPRINTF("GdDecodeJPEG: can't handled %dbpp image\n", bpp);
		goto err;
	}
	palsize = (bpp == 8)? 256: 0;

	pmd = GdCreatePixmap(&scrdev, cinfo.output_width, cinfo.output_height, data_format, NULL, palsize);
	if (!pmd)
		goto err;
DPRINTF("jpeg bpp %d\n", bpp);

	if(bpp == 8) {
		if (fast_grayscale) {
			/* use 256 shade linear palette*/
			for (i=0; i<256; ++i) {
				MWPALENTRY pe;
				pe.r = pe.g = pe.b = i;
				pe._padding = 0;
				pmd->palette[i] = pe;
			}
		} else {
#if USE_STD_PALETTE
			/* copy standard palette rather than current hw palette*/
			for (i=0; i<256; ++i)
				pmd->palette[i] = mwstdpal8[i];
#else
			/* copy current system palette*/
			for (i=0; i<256; ++i)
					pmd->palette[i] = palette[i];
#endif
		}
	}

	/* Step 5: Start decompressor */
	jpeg_start_decompress (&cinfo);

	/* Step 6: while (scan lines remain to be read) */
	while(cinfo.output_scanline < cinfo.output_height) {
		JSAMPROW rowptr[1];
		rowptr[0] = (JSAMPROW)(pmd->addr + cinfo.output_scanline * pmd->pitch);
		jpeg_read_scanlines (&cinfo, rowptr, 1);
	}

err:
	/* Step 7: Finish decompression */
	jpeg_finish_decompress (&cinfo);

	/* Step 8: Release JPEG decompression object */
	jpeg_destroy_decompress (&cinfo);

	/* May want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */
	return pmd;
}
#endif /* MW_FEATURE_IMAGES && HAVE_JPEG_SUPPORT*/
