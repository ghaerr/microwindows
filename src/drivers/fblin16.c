/*
 * Copyright (c) 1999, 2000, 2001 Greg Haerr <greg@censoft.com>
 *
 * 16bpp Linear Video Driver for Microwindows
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>
#include <stdlib.h>

/* We want to do string copying fast, so inline assembly if possible */
#ifndef __OPTIMIZE__
#define __OPTIMIZE__
#endif
#include <string.h>

#include "device.h"
#include "fb.h"

#define USE_DRAWAREA	 0	/* =1 to implement temp removed DrawArea code*/
#define USE_16BIT_ACCESS 0	/* =1 to force 16 bit display access*/

#if USE_16BIT_ACCESS
#define memcpy(d,s,nbytes)	memcpy16(d,s,(nbytes)>>1)
#define memmove(d,s,nbytes)	memcpy16(d,s,(nbytes)>>1)
static void
memcpy16(unsigned short *dst, unsigned short *src, int nwords)
{
	while (--nwords >= 0)
		*dst++ = *src++;
}
#endif

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear16_init(PSD psd)
{
	if (!psd->size) {
		psd->size = psd->yres * psd->linelen;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32*/
		psd->linelen /= 2;
	}
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear16_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	if(gr_mode == MWMODE_COPY)
		addr[x + y * psd->linelen] = c;
	else
		applyOp(gr_mode, c, &addr[x + y * psd->linelen], ADDR16);
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear16_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return addr[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear16_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += x1 + y * psd->linelen;
	if(gr_mode == MWMODE_COPY) {
		/* FIXME: memsetw(dst, c, x2-x1+1)*/
		while(x1++ <= x2)
			*addr++ = c;
	} else {
		while (x1++ <= x2) {
			applyOp(gr_mode, c, addr, ADDR16);
			++addr;
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear16_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR16	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	addr += x + y1 * linelen;
	if(gr_mode == MWMODE_COPY) {
		while(y1++ <= y2) {
			*addr = c;
			addr += linelen;
		}
	} else {
		while (y1++ <= y2) {
			applyOp(gr_mode, c, addr, ADDR16);
			addr += linelen;
		}
	}
	DRAWOFF;
}

/* srccopy bitblt*/
static void
linear16_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR16	dst = dstpsd->addr;
	ADDR16	src = srcpsd->addr;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
#if ALPHABLEND
	unsigned int alpha;
#endif

	assert (dst != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (src != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	dst += dstx + dsty * dlinelen;
	src += srcx + srcy * slinelen;

#if ALPHABLEND
	if((op & MWROP_EXTENSION) != MWROP_BLENDCONSTANT)
		goto stdblit;
	alpha = op & 0xff;

	if (dstpsd->pixtype == MWPF_TRUECOLOR565) {
		while(--h >= 0) {
			for(i=0; i<w; ++i) {
				unsigned int s = *src++;
				unsigned int d = *dst;
				unsigned int t = d & 0xf800;
				unsigned int m1, m2, m3;
				m1 = (((((s & 0xf800) - t)*alpha)>>8) & 0xf800) + t;
				t = d & 0x07e0;
				m2 = (((((s & 0x07e0) - t)*alpha)>>8) & 0x07e0) + t;
				t = d & 0x001f;
				m3 = (((((s & 0x001f) - t)*alpha)>>8) & 0x001f) + t;
				*dst++ = m1 | m2 | m3;
			}
			dst += dlinelen - w;
			src += slinelen - w;
		}
	} else {
		/* 5/5/5 format*/
		while(--h >= 0) {
			for(i=0; i<w; ++i) {
				unsigned int s = *src++;
				unsigned int d = *dst;
				unsigned int t = d & 0x7c00;
				unsigned int m1, m2, m3;
				m1 = (((((s & 0x7c00) - t)*alpha)>>8) & 0x7c00) + t;
				t = d & 0x03e0;
				m2 = (((((s & 0x03e0) - t)*alpha)>>8) & 0x03e0) + t;
				t = d & 0x001f;
				m3 = (((((s & 0x001f) - t)*alpha)>>8) & 0x001f) + t;
				*dst++ = m1 | m2 | m3;
			}
			dst += dlinelen - w;
			src += slinelen - w;
		}
	}
	DRAWOFF;
	return;
stdblit:
#endif

	if (op == MWROP_COPY) {
		/* copy from bottom up if dst in src rectangle*/
		/* memmove is used to handle x case*/
		if (srcy < dsty) {
			src += (h-1) * slinelen;
			dst += (h-1) * dlinelen;
			slinelen *= -1;
			dlinelen *= -1;
		}
		while (--h >= 0) {
			/* a _fast_ memcpy is a _must_ in this routine*/
			memmove(dst, src, w<<1);
			dst += dlinelen;
			src += slinelen;
		}
	} else {
		while (--h >= 0) {
			for (i=0; i<w; i++) {
				applyOp(MWROP_TO_MODE(op), *src, dst, ADDR16);
				++src;
				++dst;
			}
			dst += dlinelen - w;
			src += slinelen - w;
		}
	}
	DRAWOFF;
}

/* VERY experimental globals for debugging stretchblit off-by-some bug*/
extern int g_row_inc, g_col_inc;

/* srccopy stretchblt*/
static void
linear16_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, long op)
{
	ADDR16	dst;
	ADDR16	src;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	i, ymax;
	int	row_pos, row_inc;
	int	col_pos, col_inc;
	unsigned short pixel = 0;

	assert (dstpsd->addr != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (dstw > 0);
	assert (dsth > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (srcw > 0);
	assert (srch > 0);
	assert (dstx+dstw <= dstpsd->xres);
	assert (dsty+dsth <= dstpsd->yres);
	assert (srcx+srcw <= srcpsd->xres);
	assert (srcy+srch <= srcpsd->yres);

	DRAWON;
	row_pos = 0x10000;
if (g_row_inc) row_inc = g_row_inc; else
	row_inc = (srch << 16) / dsth;

	/* stretch blit using integer ratio between src/dst height/width*/
	for (ymax = dsty+dsth; dsty<ymax; ++dsty) {

		/* find source y position*/
		while (row_pos >= 0x10000L) {
			++srcy;
			row_pos -= 0x10000L;
		}

		dst = (ADDR16)dstpsd->addr + dstx + dsty*dlinelen;
		src = (ADDR16)srcpsd->addr + srcx + (srcy-1)*slinelen;

		/* copy a row of pixels*/
		col_pos = 0x10000;
if (g_col_inc) col_inc = g_col_inc; else
		col_inc = (srcw << 16) / dstw;
		for (i=0; i<dstw; ++i) {
			/* get source x pixel*/
			while (col_pos >= 0x10000L) {
				pixel = *src++;
				col_pos -= 0x10000L;
			}
			*dst++ = pixel;
			col_pos += col_inc;
		}

		row_pos += row_inc;
	}
	DRAWOFF;
}

#if USE_DRAWAREA
/* temporarily removed DrawArea entry point code*/
static void init_alpha_lookup(unsigned short **low, unsigned short **high)
{
        unsigned short a, x, *lo, *hi;
        unsigned short r, g, b;
        unsigned short idx;

        lo = *low = malloc(32*256*2);
        hi = *high = malloc(32*256*2);

	if ( hi == 0 || lo == 0 )
		exit(17);

        for ( a=0; a < 32; a++ )
                for ( x=0; x < 256; x++ ) {
                        idx = (a << 8) | x;
                        /* High byte */
                        r = (x >> 3) * a / 31;
                        g = ((x << 3) & 0x38) * a / 31;
                        hi[idx] = (r << 11) | (g << 5);
                        /* Low byte */
                        b = (x & 0x1f) * a / 31;
                        g = ((x >> 5) & 0x7) * a / 31;
                        lo[idx] = (g << 5) | b;
                }
}

static void init_wordmask_lookup(unsigned short **byte2wordmask)
{
	unsigned short *maskp, *b2wm;
	int t, x, u;

	b2wm = *byte2wordmask = malloc(256*8*2);
	if ( b2wm == 0 )
		exit(17);
	for ( t=0; t < 256; t++ ) {
		maskp = b2wm + 8 * t;
		x = t;
		for ( u=1; u < 256; u <<= 1 )
			if ( x & u )
				*maskp++ = 0xffff;
			else
				*maskp++ = 0x0000;
	}
}

/* psd->DrawArea operation PSDOP_PIXMAP_COPYALL which takes a
 * pixmap, each line is byte aligned, and copies it to the
 * screen using fg_color and bg_color to replace a 1 and 0 in
 * the pixmap.  This pixmap is ordered the wrong way around;
 * it has the leftmost pixel (on the screen) in LSB (Bit 0)
 * of the bytes.
 *
 * The reason why this non-intuitive bit ordering is used is
 * to match the bit ordering used in the T1lib font rendering
 * library.
 *
 * Variables used in the gc:
 *       dstx, dsty, dsth, dstw   Destination rectangle
 *       srcx, srcy               Source rectangle
 *       src_linelen              Linesize in bytes of source
 *       pixels                   Pixmap data
 *       fg_color		  Color of a '1' bit
 *       bg_color                 Color of a '0' bit
 */

static void pixmap_copyall(PSD psd, driver_gc_t *gc)
{
	int first_byte, last_byte;
	int hard_prefix, hard_postfix;
	unsigned short prefixbits, postfixbits, *maskp;
	unsigned short xor_color, m;
	unsigned short prefix_mask = 0, prefix_last = 0;
	unsigned short postfix_mask = 0, postfix_last = 0;
	int size_main, t, y;
	unsigned int advance_src, advance_dst;
	ADDR8 src;
	ADDR16 dst;

	static unsigned short *byte2wordmask = 0;

	prefixbits = gc->srcx & 7;
	postfixbits = (gc->srcx + gc->dstw - 1) & 7;
	first_byte = gc->srcx >> 3;
	last_byte = (gc->srcx + gc->dstw - 1) >> 3;

	src = ((ADDR8)gc->pixels) + gc->src_linelen * gc->srcy + first_byte;
	dst = ((ADDR16)psd->addr) + psd->linelen * gc->dsty + gc->dstx;
	xor_color = gc->fg_color ^ gc->bg_color;

	if ( first_byte != last_byte ) {
		if ( prefixbits == 0 ) {
			/* All bits of first byte used */
			hard_prefix = 0;
			size_main = last_byte - first_byte;
		} else {
			/* Needs to do a few odd bits first */
			hard_prefix = 1;
			size_main = last_byte - first_byte - 1;
			prefix_mask = 1 << prefixbits;
			prefix_last = 256;
		}
		if ( postfixbits != 7 ) {
			/* Last byte in source contains a few odd bits */
			hard_postfix = 1;
			postfix_mask = 1;
			postfix_last = 2 << postfixbits;
		} else {
			/* Last byte in source is used completely */
			hard_postfix = 0;
			size_main++;
		}
	} else {
		/* Very narrow pixmap, fits in single first byte */
		hard_prefix = 1;
		hard_postfix = 0;
		size_main = 0;
		prefix_mask = 1 << prefixbits;
		prefix_last = 1 << (prefixbits + gc->dstw);
	}

	advance_src = gc->src_linelen - last_byte + first_byte - 1;
	advance_dst = psd->linelen - gc->dstw;

	if ( byte2wordmask == 0 )
		init_wordmask_lookup(&byte2wordmask);

	DRAWON;
	for ( y=0; y < gc->dsth; y++ ) {

		/* Do pixels of partial first byte */
		if ( hard_prefix ) {
			for ( m=prefix_mask; m < prefix_last; m <<= 1 ) {
				if ( m & *src )
					*dst++ = gc->fg_color;
				else
					*dst++ = gc->bg_color;
			}
			src++;
		}

		/* Do all pixles of main part one byte at a time */
		for ( t=0; t < size_main; t++ ) {	
			maskp = byte2wordmask + 8 * (*src++);

			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
			*dst++ = gc->bg_color ^ (*maskp++ & xor_color);
		}

		/* Do last few bits of line */
		if ( hard_postfix ) {
			for ( m=postfix_mask; m < postfix_last; m <<= 1 ) {
				if ( *src & m )
					*dst++ = gc->fg_color;
				else
					*dst++ = gc->bg_color;
			}
			src++;
		}

		src += advance_src;
		dst += advance_dst;
	}
	DRAWOFF;
}


static unsigned short *low2scale = 0, *high2scale = 0;

static void drawarea_alphamap(PSD psd, driver_gc_t *gc)
{
	ADDR8 src, dst, alpha;
	unsigned short psl, psh, pd;
	unsigned char as, ad;
	int x, y;

	if ( low2scale == 0 )
		init_alpha_lookup(&low2scale,&high2scale);

	src = (ADDR8)(((ADDR16)gc->pixels) + gc->srcx +
		      gc->src_linelen * gc->srcy);
	dst = (ADDR8)(((ADDR16)psd->addr) +
		      psd->linelen * gc->dsty + gc->dstx);
	alpha = ((ADDR8)gc->misc) + gc->src_linelen * gc->srcy + gc->srcx;

	DRAWON;
	for ( y=0; y < gc->dsth; y++ ) {
		for ( x=0; x < gc->dstw; x++ ) {
			as = (*alpha++) >> 3;
			ad = 31 - as;
			psl = low2scale[(as<<8)|*src++];
			psh = high2scale[(as<<8)|*src++];
			pd = low2scale[(ad<<8)|dst[0]] +
				high2scale[(ad<<8)|dst[1]];
			*((unsigned short *)dst)++ = psl + psh + pd;
		}
	}
	DRAWOFF;
}

static void drawarea_alphacol(PSD psd, driver_gc_t *gc)
{
	ADDR8 dst, alpha;
	unsigned short col_low, col_high, psl, psh, pd;
	unsigned char as, ad;
	int x, y;

	if ( low2scale == 0 )
		init_alpha_lookup(&low2scale,&high2scale);

	dst = (ADDR8)(((ADDR16)psd->addr) +
		      psd->linelen * gc->dsty + gc->dstx);
	alpha = ((ADDR8)gc->misc) + gc->src_linelen * gc->srcy + gc->srcx;
	col_low = gc->bg_color & 0xff;
	col_high = ( gc->bg_color >> 8 ) & 0xff;

	DRAWON;
	for ( y=0; y < gc->dsth; y++ ) {
		for ( x=0; x < gc->dstw; x++ ) {
			as = (*alpha++) >> 3;
			if ( as ) {
				if ( (ad = 31 - as) ) {
					psl = low2scale[(as<<8)|col_low];
					psh = high2scale[(as<<8)|col_high];
					pd = low2scale[(ad<<8)|dst[0]] +
						high2scale[(ad<<8)|dst[1]];
					*((unsigned short *)dst)++ = psl + psh + pd;
				} else {
					*((unsigned short *)dst)++ = gc->bg_color;
				}
			}
		}
	}
	DRAWOFF;
}

static void linear16_drawarea(PSD psd, driver_gc_t *gc, int op)
{
	ADDR16	src16, dst, rsrc, rdst;
	int linesize, x, y;
	unsigned short pcol;

	assert(psd->addr != 0);
	/*assert(gc->dstw <= gc->srcw);*/
	assert(gc->dstx >= 0 && gc->dstx+gc->dstw <= psd->xres);
	/*assert(gc->dsty >= 0 && gc->dsty+gc->dsth <= psd->yres);*/
	/*assert(gc->srcx >= 0 && gc->srcx+gc->dstw <= gc->srcw);*/
	assert(gc->srcy >= 0);

#if 0
	op = GD_AREA_COPY;
	printf("DrawArea op=%d x=%d y=%d\n",op,gc->x,gc->y);
#endif

	if ( op == PSDOP_COPY )
		op = gc->gr_usebg ? PSDOP_COPYALL : PSDOP_COPYTRANS;

	switch ( op ) {
		case PSDOP_COPYALL:
			linesize = 2 * gc->dstw;
			src16 = ((ADDR16)gc->pixels) + gc->srcx +
				gc->src_linelen * gc->srcy;
			dst = ((ADDR16)psd->addr) + gc->dstx + 
				psd->linelen * gc->dsty;

			DRAWON;
			for ( y=1; y < gc->dsth; y++ ) {
				memcpy(dst,src16,linesize);
				src16 += gc->src_linelen;
				dst += psd->linelen;
			}
			memcpy(dst,src16,linesize);	/* To be seriously ANSI */
			DRAWOFF;
			break;

		case PSDOP_COPYTRANS:
			src16 = ((ADDR16)gc->pixels) + gc->srcx +
				gc->src_linelen * gc->srcy;
                        dst = ((ADDR16)psd->addr) + gc->dstx +
				psd->linelen * gc->dsty;

                        DRAWON;
                        for ( y=0; y < gc->dsth; y++ ) {
				rdst = dst;
				rsrc = src16;
				for ( x=0; x < gc->dstw; x++ ) {
					pcol = *rsrc++;
					if ( pcol == gc->bg_color )
						rdst++;
					else
						*rdst++ = pcol;
				}
				dst += psd->linelen;
				src16 += gc->src_linelen;
			}
			DRAWOFF;
			break;

		case PSDOP_ALPHAMAP:
			drawarea_alphamap(psd,gc);
			break;

		case PSDOP_ALPHACOL:
			drawarea_alphacol(psd,gc);
			break;

		case PSDOP_PIXMAP_COPYALL:
			pixmap_copyall(psd,gc);
			break;

	}
}
#endif /* USE_DRAWAREA*/

SUBDRIVER fblinear16 = {
	linear16_init,
	linear16_drawpixel,
	linear16_readpixel,
	linear16_drawhorzline,
	linear16_drawvertline,
	gen_fillrect,
	linear16_blit,
#if USE_DRAWAREA
	linear16_drawarea,
#else
	NULL,
#endif
	linear16_stretchblit
};
