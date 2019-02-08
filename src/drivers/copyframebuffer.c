/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Fast framebuffer copy routine TEMPLATE - used with scr_fbe.c when TESTDRIVER=1
 */
#include "device.h"
#include "genmem.h"

/* copy Microwindows framebuffer pixels to another framebuffer, same pixel format*/
void
copy_framebuffer(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	unsigned char *dstpixels, unsigned int dstpitch)
{
	unsigned int x, y;
	unsigned int srcpitch = psd->pitch;

//printf("copy_framebuffer %d,%d %d,%d\n", destx, desty, w, h);

	/* Use optimized loops for most common framebuffer modes */
	/* NOTE: ASSUMES DEST FRAMEBUFFER IN SAME FORMAT AS MWPIXEL_FORMAT!!!*/
#if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
	{
		unsigned char *src =  psd->addr + desty * srcpitch + destx;
		unsigned char *dstl = dstpixels + desty * dstpitch + destx;
		for (y = 0; y < h; y++) {
			unsigned char *dst = dstl;
			for (x = 0; x < w; x++) {
				//MWPIXELVAL c = src[x];
				//unsigned long pixel = PIXELVAL_to_pixel(c);
				dst[x] = src[x];
			}
			src += srcpitch;
			dstl += dstpitch;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
	{
		unsigned char *src =  psd->addr + desty * srcpitch + (destx << 1);
		unsigned char *dstl = dstpixels + desty * dstpitch + (destx << 1);
		for (y = 0; y < h; y++) {
			uint16_t *dst = (uint16_t *)dstl;
			for (x = 0; x < w; x++) {
				//MWPIXELVAL c = ((ADDR16)src)[x];
				//unsigned long pixel = PIXELVAL_to_pixel(c);
				dst[x] = ((uint16_t *)src)[x];
			}
			src += srcpitch;
			dstl += dstpitch;
		}
	}
#elif MWPIXEL_FORMAT == MWPF_TRUECOLORRGB
	{
		unsigned char *src =  psd->addr + desty * srcpitch + (destx * 3);
		unsigned char *dstl = dstpixels + desty * dstpitch + (destx * 3);
		unsigned int srcextra = srcpitch - (w * 3);
		for (y = 0; y < h; y++) {
			unsigned char *dst = dstl;
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = RGB2PIXEL888(src[2], src[1], src[0]);
				//unsigned long pixel = PIXELVAL_to_pixel(c);
				// FIXME little-endian only
				*dst++ = (unsigned char)c;			// B
				*dst++ = (unsigned char)c >> 8;		// G
				*dst++ = (unsigned char)c >> 16;	// R
				src += 3;
			}
			src += srcextra;
			dstl += dstpitch;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLORARGB) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)
	{
		// our framebuffer is MWPF_TRUECOLORARGB which is BGRA byte order, and so are SDL screenbits
		unsigned char *src =  psd->addr + desty * srcpitch + (destx << 2);
		unsigned char *dstl = dstpixels + desty * dstpitch + (destx << 2);
		for (y = 0; y < h; y++) {
			uint32_t *dst = (uint32_t *)dstl;
			for (x = 0; x < w; x++) {
				//MWPIXELVAL c = ((ADDR32)src)[x];
				//unsigned long pixel = PIXELVAL_to_pixel(c);
				dst[x] = ((uint32_t *)src)[x];
			}
			src += srcpitch;
			dstl += dstpitch;
		}
	}
#else /* MWPF_PALETTE*/
	{
		unsigned char *src = psd->addr + desty * srcpitch + destx;
		unsigned char *dstl = dstpixels + desty * dstpitch + destx;
		for (y = 0; y < h; y++) {
			unsigned char *dst = dstl;
			for (x = 0; x < w; x++) {
				//MWPIXELVAL c = src[x];
				//unsigned long pixel = PIXELVAL_to_pixel(c);
				dst[x] = src[x];
			}
			src += srcpitch;
			dstl += dstpitch;
		}
	}
#endif
}
