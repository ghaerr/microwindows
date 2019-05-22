/*
 * Copyright (c) 2000, 2001, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * Image decode routine for PNG files
 *
 * Decode PNG images into 8 or 24 bpp with alpha if present.
 *
 * 2007-Nov-15 - Vladimir Ananiev (vovan888 at gmail com)
 *		alpha channel, gamma correction added - ripped from pngm2pnm.c
 */
#include <stdlib.h>
#include "uni_std.h"
#include "device.h"
#include "../drivers/genmem.h"

#if MW_FEATURE_IMAGES && HAVE_PNG_SUPPORT
#include <png.h>

/* png_jmpbuf() macro is not defined prior to libpng-1.0.6*/
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr)	((png_ptr)->jmpbuf)
#endif

/* This is a quick user defined function to read from the buffer instead of from the file pointer
*/
static void
png_read_buffer(png_structp pstruct, png_bytep pointer, png_size_t size)
{
#if (PNG_LIBPNG_VER >= 10510)
    png_voidp ptr =  png_get_io_ptr(pstruct);
    GdImageBufferRead(ptr, pointer, size);
#else
    //GdImageBufferRead(pstruct->io_ptr, pointer, size);
    GdImageBufferRead(png_get_io_ptr(pstruct), pointer, size);
#endif
}

PSD
GdDecodePNG(buffer_t * src)
{
	unsigned char hdr[8], **rows;
	png_structp state;
	png_infop pnginfo;
	png_uint_32 width, height;
	int bit_depth, color_type, i;
	double file_gamma;
	int channels, data_format;
	PSD pmd;

	GdImageBufferSeekTo(src, 0UL);

	if(GdImageBufferRead(src, hdr, 8) != 8)
		return NULL;

	if(png_sig_cmp(hdr, 0, 8))
		return NULL;

	if(!(state = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		goto nomem;

	if(!(pnginfo = png_create_info_struct(state))) {
		png_destroy_read_struct(&state, NULL, NULL);
		goto nomem;
	}

	if(setjmp(png_jmpbuf(state))) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		return NULL;
	}

	/* Set up the input function */
	png_set_read_fn(state, src, png_read_buffer);
	/* png_init_io(state, src); */

	png_set_sig_bytes(state, 8);

	png_read_info(state, pnginfo);
	png_get_IHDR(state, pnginfo, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	/* set-up the transformations */
	/* transform paletted images into full-color rgb */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	    png_set_expand (state);

	/* expand images to bit-depth 8 (only applicable for grayscale images) */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	    png_set_expand (state);

	/* transform transparency maps into full alpha-channel */
	if (png_get_valid (state, pnginfo, PNG_INFO_tRNS))
	    png_set_expand (state);

	/* downgrade 16-bit images to 8 bit */
	if (bit_depth == 16)
	    png_set_strip_16 (state);

	/* Handle transparency... */
	if (png_get_valid(state, pnginfo, PNG_INFO_tRNS))
	    png_set_tRNS_to_alpha(state);

	/* transform grayscale images into full-color */
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	    png_set_gray_to_rgb (state);

	/* only if file has a file gamma, we do a correction */
	if (png_get_gAMA (state, pnginfo, &file_gamma))
	    png_set_gamma (state, (double) 2.2, file_gamma);

	/* all transformations have been registered; now update pnginfo data,
	 * get rowbytes and channels, and allocate image memory */

	png_read_update_info (state, pnginfo);

	/* get the new color-type and bit-depth (after expansion/stripping) */
	png_get_IHDR (state, pnginfo, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	/* calculate new number of channels and store alpha-presence */
	if (color_type == PNG_COLOR_TYPE_RGB)
	    channels = 3;
	else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	    channels = 4;
//	else if (color_type == PNG_COLOR_TYPE_GRAY)
//	    channels = 1;
//	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
//	    channels = 2;
	else {
	 	/* GdDrawImage currently only supports 32bpp alpha channel*/
		DPRINTF("GdDecodePNG: Gray image type not supported: %d\n", color_type);
		return NULL;
	}

	/* set image data format*/
	data_format = (channels == 4)? MWIF_RGBA8888: MWIF_RGB888;

	//pimage->pitch = width * channels * (bit_depth / 8);
	//bpp = channels * 8;
	pmd = GdCreatePixmap(&scrdev, width, height, data_format, NULL, 0);
	if (!pmd) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		goto nomem;
    }
//DPRINTF("png %dbpp\n", channels*8);

    if(!(rows = malloc(height * sizeof(unsigned char *)))) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		goto nomem;
    }
	for(i = 0; i < height; i++)
		rows[i] = ((unsigned char *)pmd->addr) + i * pmd->pitch;

	png_read_image(state, rows);
	png_read_end(state, NULL);
	free(rows);
	png_destroy_read_struct(&state, &pnginfo, NULL);

	return pmd;

nomem:
	EPRINTF("GdDecodePNG: Out of memory\n");
	return NULL;
}
#endif /* MW_FEATURE_IMAGES && HAVE_PNG_SUPPORT*/
