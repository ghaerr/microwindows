/*
 * Copyright (c) 2000, 2001, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * Image decode routine for PNG files
 *
 * Currently for simplicity we get the PNG library to convert the file to
 * 24 bit RGB format with no alpha channel information even if we could
 * potentially store the image more efficiently by taking note of the image
 * type and depth and acting accordingly. Similarly, > 8 bits per channel,
 * gamma correction, etc. are not supported.
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
	int bit_depth, colourtype, i;

	GdImageBufferSeekTo(src, 0UL);

	if(GdImageBufferRead(src, hdr, 8) != 8)
		return 0;

	if(png_sig_cmp(hdr, 0, 8))
		return 0;

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
	GdComputeImagePitch(pimage->bpp, pimage->width, &pimage->pitch,
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
	EPRINTF("GdDecodePNG: Out of memory\n");
	return 2;
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_PNG_SUPPORT)*/
