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

#if MW_FEATURE_IMAGES && defined(HAVE_PNG_SUPPORT)
#include <png.h>

/* png_jmpbuf() macro is not defined prior to libpng-1.0.6*/
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr)	((png_ptr)->jmpbuf)
#endif

/* This is a quick user defined function to read from the buffer instead of from the file pointer */
static void
png_read_buffer(png_structp pstruct, png_bytep pointer, png_size_t size)
{
	GdImageBufferRead(pstruct->io_ptr, pointer, size);
}

int
GdDecodePNG(buffer_t * src, PMWIMAGEHDR pimage)
{
	unsigned char hdr[8], **rows;
	png_structp state;
	png_infop pnginfo;
	png_uint_32 width, height;
	int bit_depth, color_type, i;
	double file_gamma;
	int channels, alpha_present;

	GdImageBufferSeekTo(src, 0UL);

	if(GdImageBufferRead(src, hdr, 8) != 8)
		return 0;

	if(png_sig_cmp(hdr, 0, 8))
		return 0;

	if(!(state = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		goto nomem;

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
	png_get_IHDR(state, pnginfo, &width, &height, &bit_depth, &color_type,
		NULL, NULL, NULL);

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
	png_get_IHDR (state, pnginfo, &width, &height, &bit_depth, &color_type,
	    NULL, NULL, NULL);

	/* calculate new number of channels and store alpha-presence */
	if (color_type == PNG_COLOR_TYPE_GRAY)
	    channels = 1;
	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	    channels = 2;
	else if (color_type == PNG_COLOR_TYPE_RGB)
	    channels = 3;
	else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	    channels = 4;
	else
	    channels = 0; /* should never happen */
	/* 
	 * FIXME note that GdDrawImage currently only supports 32bpp alpha channel.
	 * Gray 8bpp w/alpha will be faked as 16bpp truecolor to avoid crashing.
	 */
	//alpha_present = (channels - 1) % 2;
	alpha_present = (channels == 4);	/* force only COLOR_TYPE_RGB w/alpha*/
	
	pimage->width = width;
	pimage->height = height;
	pimage->palsize = 0;
	pimage->planes = 1;
	pimage->pitch = width * channels * (bit_depth / 8);
	pimage->bpp = channels * 8;
	pimage->bytesperpixel = channels;

	if (alpha_present)
		pimage->compression = MWIMAGE_RGB | MWIMAGE_ALPHA_CHANNEL;
	else
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

	png_read_image(state, rows);
	png_read_end(state, NULL);
	free(rows);
	png_destroy_read_struct(&state, &pnginfo, NULL);

	return 1;

nomem:
	EPRINTF("GdDecodePNG: Out of memory\n");
	return 2;
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_PNG_SUPPORT)*/
