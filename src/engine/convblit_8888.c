/*
 * Device-independent low level convblit routines - 32, 24, 16bpp RGBA/RGB input
 *
 * Copyright (c) 2010 Greg Haerr <greg@censoft.com>
 *
 * This file will need to be modified when adding a new hardware framebuffer
 * image format.
 *
 * Currently, 32bpp BGRA, 32bpp RGBA, 24bpp BGR, and 16bpp RGB565/555 are defined.
 *
 * These routines do no range checking, clipping, or cursor
 * overwriting checks, but instead draw directly to the
 * data_out memory buffer specified in the passed BLITPARMS struct.
 */
#include "device.h"
#include "convblit.h"
#include "../drivers/fb.h"		// DRAWON macro

/* for convenience in specifying inline parms*/
#define R		0		/* RGBA parms*/
#define G		1
#define B		2
#define A		3

#define NONE	MWPORTRAIT_NONE
#define LEFT	MWPORTRAIT_LEFT
#define RIGHT	MWPORTRAIT_RIGHT
#define DOWN	MWPORTRAIT_DOWN

#define COPY	0		/* mode parm*/
#define SRCOVER	1

/*
 * Conversion blit for COPY or SRCOVER from RGBA or RGB input to
 * 32, 24 or 16bpp output, and rotate according to portrait specified.
 *
 * The gcc inline mechanism can compile this function with the
 * result of no switch and few if statements, as most use constant comparisons,
 * and will be optimized out.  Thus, the inner loops run very fast!
 * This is also true for the copy vs srcover.  When COPY is specified,
 * no blending code will be included.
 */
static inline void convblit_8888(PSD psd, PMWBLITPARMS gc, int mode,
	int SSZ, int SR, int SG, int SB, int SA,
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)
{
	unsigned char *src, *dst;
	int dsz, dst_pitch;
	int height, newx, newy;
	int src_pitch = gc->src_pitch;

	/* compiler can optimize out switch statement and most else to constants*/
	switch (PORTRAIT) {
	case NONE:
		dsz = DSZ;					/* dst: next pixel over*/
		dst_pitch = gc->dst_pitch;	/* dst: next line down*/
		break;

	case LEFT:
		/* change dst top left to lower left for left portrait*/
		/* rotate left: X -> Y, Y -> maxx - X*/
		newx = gc->dsty;
		gc->dsty = psd->xvirtres - gc->dstx - 1;
		gc->dstx = newx;

		dsz = -gc->dst_pitch;		/* dst: next row up*/
		dst_pitch = DSZ;			/* dst: next pixel right*/
		break;

	case RIGHT:
		/* change dst top left to upper right for right portrait*/
 		/* Rotate right: X -> maxy - y - h, Y -> X, W -> H, H -> W*/
		newy = gc->dstx;
		gc->dstx = psd->yvirtres - gc->dsty - 1;
		gc->dsty = newy;

		dsz = gc->dst_pitch;		/* dst: next pixel down*/
		dst_pitch = -DSZ;			/* dst: next pixel left*/
		break;

	case DOWN:
		/* change dst top left to lower right for down portrait*/
 		/* Rotate down: X -> maxx - x - w, Y -> maxy - y - h*/
		gc->dstx = psd->xvirtres - gc->dstx - 1;
		gc->dsty = psd->yvirtres - gc->dsty - 1;

		dsz = -DSZ;					/* dst: next pixel left*/
		dst_pitch = -gc->dst_pitch;	/* dst: next pixel up*/
		break;
	}

	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + gc->srcx * SSZ;
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + gc->dstx * DSZ;

	DRAWON;
	height = gc->height;
	while (--height >= 0)
	{
		register unsigned char *d = dst;
		register unsigned char *s = src;
		unsigned int alpha;
		int w = gc->width;

		while (--w >= 0)
		{
			/* inline implementation will optimize out all but two compares in inner loop*/
			if (mode == COPY || (alpha = s[SA]) == 255)		/* copy source*/
			{
				if (DSZ == 2)
				{
					if (SSZ == 2)
						((unsigned short *)d)[0] = ((unsigned short *)s)[0];
					else
						((unsigned short *)d)[0] = RGB2PIXEL(s[SR], s[SG], s[SB]);
				}
				else
				{
					if (DA >= 0)			/* compiler will optimize out completely*/
						d[DA] = (SA >= 0)? s[SA]: 255;

					d[DR] = s[SR];
					d[DG] = s[SG];
					d[DB] = s[SB];
				}
			}
			else if (alpha != 0)							/* blend source w/dest*/
			{
				if (DSZ == 2) {
					unsigned short sr = RED2PIXEL(s[SR]);
					unsigned short sg = GREEN2PIXEL(s[SG]);
					unsigned short sb = BLUE2PIXEL(s[SB]);
					alpha = 255 - alpha + 1; /* flip alpha then add 1 (see muldiv255)*/

					/* d = muldiv255(255-a, d - s) + s*/
					((unsigned short *)d)[0] = muldiv255_16bpp(((unsigned short *)d)[0], sr, sg, sb, alpha);
				}
				else
				{
 					/* d += muldiv255(a, 255 - d)*/
					if (DA >= 0)
						d[DA] += muldiv255(alpha, 255 - d[DA]);

 					/* d += muldiv255(a, s - d)*/
					d[DR] += muldiv255(alpha, s[SR] - d[DR]);
					d[DG] += muldiv255(alpha, s[SG] - d[DG]);
					d[DB] += muldiv255(alpha, s[SB] - d[DB]);
				}
			}
			d += dsz;
			s += SSZ;				/* src: next pixel right*/
		}
		src += src_pitch;			/* src: next line down*/
		dst += dst_pitch;
	}
	DRAWOFF;

	/* update screen bits if driver requires it*/
	if (!psd->Update)
		return;

	switch (PORTRAIT) {		/* switch will be optimized out*/
	case NONE:
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
		break;

	case LEFT:
		/* adjust x,y,w,h to physical top left and w/h*/
		psd->Update(psd, gc->dstx, gc->dsty - gc->width + 1, gc->height, gc->width);
		break;

	case RIGHT:
		/* adjust x,y,w,h to physical top left and w/h*/
		psd->Update(psd, gc->dstx - gc->height + 1, gc->dsty, gc->height, gc->width);
		break;

	case DOWN:
		/* adjust x,y,w,h to physical top left and w/h*/
		psd->Update(psd, gc->dstx - gc->width + 1, gc->dsty - gc->height + 1, gc->width, gc->height);
		break;
	}
}

/*---------- 32bpp BGRA output ----------*/

#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
/* Conversion blit srcover 32bpp RGBA image to 32bpp RGBA image*/
void convblit_srcover_rgba8888_rgba8888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, SRCOVER, 4, R,G,B,A, 4, R,G,B,A, psd->portrait);
}

/* Conversion blit copy 32bpp RGBA image to 32bpp RGBA image*/
void convblit_copy_rgba8888_rgba8888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 4, R,G,B,A, 4, R,G,B,A, psd->portrait);
}

/* Conversion blit copy 24bpp RGB image to 32bpp RGBA image*/
void convblit_copy_rgb888_rgba8888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 3, R,G,B,-1, 4, R,G,B,A, psd->portrait);
}

#else /* MWPF_TRUECOLOR8888*/

void convblit_srcover_rgba8888_bgra8888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, SRCOVER, 4, R,G,B,A, 4, B,G,R,A, psd->portrait);
}

/* Conversion blit copy 32bpp RGBA image to 32bpp BGRA image*/
void convblit_copy_rgba8888_bgra8888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 4, R,G,B,A, 4, B,G,R,A, psd->portrait);
}

/* Conversion blit copy 24bpp RGB image to 32bpp BGRA image*/
void convblit_copy_rgb888_bgra8888(PSD psd, PMWBLITPARMS gc)
{
	// -1 forces 255 alpha in destination
	convblit_8888(psd, gc, COPY, 3, R,G,B,-1, 4, B,G,R,A, psd->portrait);
}
#endif

/* Copy 32bpp XXXX image to 32bpp XXXX image*/
void convblit_copy_8888_8888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 4, R,G,B,A, 4, R,G,B,A, psd->portrait);
}

/*---------- 24bpp BGR output ----------*/

/* Conversion blit srcover 32bpp RGBA image to 24bpp BGR image*/
void convblit_srcover_rgba8888_bgr888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, SRCOVER, 4, R,G,B,A, 3, B,G,R,-1, psd->portrait);
}

/* Conversion blit copy 32bpp RGBA image to 24bpp BGR image*/
void convblit_copy_rgba8888_bgr888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 4, R,G,B,A, 3, B,G,R,-1, psd->portrait);
}

/* Conversion blit copy 24bpp RGB image to 24bpp BGR image*/
void convblit_copy_rgb888_bgr888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 3, R,G,B,-1, 3, B,G,R,-1, psd->portrait);
}

/* Copy 24bpp XXX image to 24bpp XXX image*/
void convblit_copy_888_888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 3, R,G,B,-1, 3, R,G,B,-1, psd->portrait);
}

/* Copy 32bpp BGRA image to 24bpp BGR image (GdArea MWPF_PIXELVAL)*/
void convblit_copy_bgra8888_bgr888(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 4, B,G,R,A, 3, B,G,R,-1, psd->portrait);
}

/*---------- 16bpp BGR output ----------*/

/* Conversion blit srcover 32bpp RGBA image to 16bpp image*/
void convblit_srcover_rgba8888_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, SRCOVER, 4, R,G,B,A, 2, 0,0,0,-1, psd->portrait);
}

/* Conversion blit copy 32bpp RGBA image to 16bpp image*/
void convblit_copy_rgba8888_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 4, R,G,B,A, 2, 0,0,0,-1, psd->portrait);
}

/* Conversion blit copy 24bpp RGB image to 16bpp image*/
void convblit_copy_rgb888_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 3, R,G,B,-1, 2, 0,0,0,-1, psd->portrait);
}

/* Copy 16bpp image to 16bpp image*/
void convblit_copy_16bpp_16bpp(PSD psd, PMWBLITPARMS gc)
{
	convblit_8888(psd, gc, COPY, 2, 0,0,0,-1, 2, 0,0,0,-1, psd->portrait);
}

/* framebuffer pixel format blit - must handle backwards copy, different rotation code*/
static inline void frameblit_8888(PSD psd, PMWBLITPARMS gc,
	int SSZ, int SR, int SG, int SB, int SA,
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)
{
	unsigned char *src, *dst;
	int width, height, tmp;
	int src_pitch = gc->src_pitch;
	int dst_pitch = gc->dst_pitch;	/* dst: next line down*/
	int ssz = SSZ;
	int dsz = DSZ;					/* dst: next pixel over*/

	/*
	 * For psd -> psd blits, the orientation between psd's is always
	 * the same, so the blit move itself doesn't change. Here,
	 * we just transform the x,y,w,h coordinates to get to the
	 * the "top left" of the bitmap psd->addr. These rotations
	 * slightly different than the rotations required for the
	 * convblits where we actually copy memory differently
	 * depending on the orientation.
	 */
	switch (PORTRAIT) {
	case LEFT:
		// X = Y
		// Y = xmax - X - W
		// W = H
		// H = W
		tmp = gc->dsty;
		gc->dsty = psd->xvirtres - gc->dstx - gc->width;
		gc->dstx = tmp;

		tmp = gc->srcy;
		gc->srcy = gc->srcpsd->xvirtres - gc->srcx - gc->width;
		gc->srcx = tmp;

		tmp = gc->width;
		gc->width = gc->height;
		gc->height = tmp;
		break;

	case RIGHT:
		// X = ymax - Y - H
		// Y = X
		// W = H
		// H = W
		tmp = gc->dstx;
		gc->dstx = psd->yvirtres - gc->dsty - gc->height;
		gc->dsty = tmp;

		tmp = gc->srcx;
		gc->srcx = gc->srcpsd->yvirtres - gc->srcy - gc->height;
		gc->srcy = tmp;

		tmp = gc->width;
		gc->width = gc->height;
		gc->height = tmp;
		break;

	case DOWN:
		// X = xmax - X - W
		// Y = ymax - Y - H
		gc->dstx = psd->xvirtres - gc->dstx - gc->width;
		gc->dsty = psd->yvirtres - gc->dsty - gc->height;

		gc->srcx = gc->srcpsd->xvirtres - gc->srcx - gc->width;
		gc->srcy = gc->srcpsd->yvirtres - gc->srcy - gc->height;
		break;
	}

	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + gc->srcx * SSZ;
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + gc->dstx * DSZ;

	width = gc->width;
	height = gc->height;

	/* check for backwards copy if dst in src rect, in same psd*/
	if (gc->data == gc->data_out)
	{
		if (gc->srcy < gc->dsty)
		{
			/* copy from bottom upwards*/
			src += (height - 1) * gc->src_pitch;
			dst += (height - 1) * gc->dst_pitch;
			src_pitch = -src_pitch;
			dst_pitch = -dst_pitch;
		}
		if (gc->srcx < gc->dstx)
		{
			/* copy from right to left*/
			src += (width - 1) * SSZ;
			dst += (width - 1) * DSZ;
			ssz = -ssz;
			dsz = -dsz;
		}
	}

	DRAWON;
	while (--height >= 0)
	{
		register unsigned char *d = dst;
		register unsigned char *s = src;
		int w = width;

		while (--w >= 0)
		{
			/* inline implementation will optimize out all compares in inner loop*/
			if (DSZ == 2)
			{
				if (SSZ == 2)
					((unsigned short *)d)[0] = ((unsigned short *)s)[0];
				else
					((unsigned short *)d)[0] = RGB2PIXEL(s[SR], s[SG], s[SB]);
			}
			else
			{
				if (DA >= 0)
					d[DA] = (SA >= 0)? s[SA]: 255;
				d[DR] = s[SR];
				d[DG] = s[SG];
				d[DB] = s[SB];
			}
			d += dsz;
			s += ssz;
		}
		src += src_pitch;
		dst += dst_pitch;
	}
	DRAWOFF;

	/* update screen bits if driver requires it*/
	if (psd->Update)
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
}

/* framebuffer pixel format copy blit - 32bpp*/
void frameblit_copy_8888_8888(PSD psd, PMWBLITPARMS gc)
{
	frameblit_8888(psd, gc, 4, R,G,B,A, 4, R,G,B,A, psd->portrait);
}

/* framebuffer pixel format copy blit - 24bpp*/
void frameblit_copy_888_888(PSD psd, PMWBLITPARMS gc)
{
	frameblit_8888(psd, gc, 3, R,G,B,-1, 3, R,G,B,-1, psd->portrait);
}

/* framebuffer pixel format copy blit - 16bpp*/
void frameblit_copy_16bpp_16bpp(PSD psd, PMWBLITPARMS gc)
{
	frameblit_8888(psd, gc, 2, 0,0,0,-1, 2, 0,0,0,-1, psd->portrait);
}
