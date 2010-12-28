/*
 * Device-independent low level convblit routines - framebuffer blits
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

/* Rotate src and dst coords, swap w/h - used with Frame->Frame blit.
 * For framebuffer blits, the orientation is the same,
 * so the blit move itself doesn't change.  Here, we
 * transform the source and dest x,y,w,h coordinates to get to the
 * the "top left" of the bitmap psd->addr. These rotations are
 * slightly different than the rotations required for the
 * convblits where we actually copy memory differently
 * depending on the orientation.
 */
#define FRAMEBLIT_ROTATE_COORDS(psd, gc) \
{ \
	ssz = SSZ; \
	src_pitch = gc->src_pitch; \
	dsz = DSZ;					/* dst: next pixel over*/ \
	dst_pitch = gc->dst_pitch;	/* dst: next line down*/ \
	switch (PORTRAIT) { \
	case LEFT: \
		/* rotate left: X = Y, Y = xmax - X - W, W = H*/ \
		tmp = gc->dsty; \
		gc->dsty = psd->xvirtres - gc->dstx - gc->width; \
		gc->dstx = tmp; \
\
		tmp = gc->srcy; \
		gc->srcy = gc->src_xvirtres - gc->srcx - gc->width; \
		gc->srcx = tmp; \
\
		tmp = gc->width; \
		gc->width = gc->height; \
		gc->height = tmp; \
		break; \
\
	case RIGHT: \
		/* rotate right: X = ymax - Y - H, Y = X, W = H*/ \
		tmp = gc->dstx; \
		gc->dstx = psd->yvirtres - gc->dsty - gc->height; \
		gc->dsty = tmp; \
\
		tmp = gc->srcx; \
		gc->srcx = gc->src_yvirtres - gc->srcy - gc->height; \
		gc->srcy = tmp; \
\
		tmp = gc->width; \
		gc->width = gc->height; \
		gc->height = tmp; \
		break; \
\
	case DOWN: \
		/* rotate down: X = xmax - X - W, Y = ymax - Y - H*/ \
		gc->dstx = psd->xvirtres - gc->dstx - gc->width; \
		gc->dsty = psd->yvirtres - gc->dsty - gc->height; \
\
		gc->srcx = gc->src_xvirtres - gc->srcx - gc->width; \
		gc->srcy = gc->src_yvirtres - gc->srcy - gc->height; \
		break; \
	} \
}

/*
 * Update screen bits if driver requires it. 
 * For framebuffer blits, we started with the "top left"
 * corner, so no additional rotations are required.
 */
#define FRAMEBLIT_UPDATE(psd, gc) \
{ \
	if (psd->Update) \
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height); \
}

/* Rotate src and dst coords - used with Frame->Frame StretchBlit.
 * Although this is a framebuffer format blit and the
 * orientation between psd's is the same, we use the
 * convblit coordinate rotation code, but rotate both
 * source and dest coordinates.  This is required because
 * the src/dst ratio is computed in the upper level code
 * and the passed stretchblit step parms can't be rotated
 * without recalcing the original data.
 */
#define STRETCH_FRAMEBLIT_ROTATE_COORDS(psd, gc) \
{ \
	switch (PORTRAIT) { \
	case NONE: \
	default: \
		dsz = DSZ;					/* src/dst: next pixel over*/ \
		ssz = SSZ; \
		dst_pitch = gc->dst_pitch;	/* src/dst: next line down*/ \
		src_pitch = gc->src_pitch; \
		break; \
\
	case LEFT: \
		/* change src/dst top left to lower left for left portrait*/ \
		/* rotate left: X -> Y, Y -> maxx - X*/ \
		tmp = gc->dsty; \
		gc->dsty = psd->xvirtres - gc->dstx - 1; \
		gc->dstx = tmp; \
\
		tmp = gc->srcy; \
		gc->srcy = gc->src_xvirtres - gc->srcx - 1; \
		gc->srcx = tmp; \
\
		dsz = -gc->dst_pitch;		/* src/dst: next row up*/ \
		ssz = -gc->src_pitch; \
		dst_pitch = DSZ;			/* src/dst: next pixel right*/ \
		src_pitch = SSZ; \
		break; \
\
	case RIGHT: \
		/* change src/dst top left to upper right for right portrait*/ \
 		/* Rotate right: X -> maxy - y - h, Y -> X, W -> H, H -> W*/ \
		tmp = gc->dstx; \
		gc->dstx = psd->yvirtres - gc->dsty - 1; \
		gc->dsty = tmp; \
\
		tmp = gc->srcx; \
		gc->srcx = gc->src_yvirtres - gc->srcy - 1; \
		gc->srcy = tmp; \
\
		dsz = gc->dst_pitch;		/* src/dst: next pixel down*/ \
		ssz = gc->src_pitch; \
		dst_pitch = -DSZ;			/* src/dst: next pixel left*/ \
		src_pitch = -SSZ; \
		break; \
\
	case DOWN: \
		/* change src/dst top left to lower right for down portrait*/ \
 		/* Rotate down: X -> maxx - x - w, Y -> maxy - y - h*/ \
		gc->dstx = psd->xvirtres - gc->dstx - 1; \
		gc->dsty = psd->yvirtres - gc->dsty - 1; \
\
		gc->srcx = gc->src_xvirtres - gc->srcx - 1; \
		gc->srcy = gc->src_yvirtres - gc->srcy - 1; \
\
		dsz = -DSZ;					/* src/dst: next pixel left*/ \
		ssz = -DSZ; \
		dst_pitch = -gc->dst_pitch;	/* src/dst: next pixel up*/ \
		src_pitch = -gc->src_pitch; \
		break; \
	} \
}

/* Rotate dst coords - Used with Pixmap->Frame or Pixmap->Pixmap.
 * For convblits, the source data is always non-portrait.
 * Here, we rotate just the dest coordinates so as to start
 * in the rotated "0,0" position.  Each image row is then
 * copied from the src and rotated dest positions.
 */
#define CONVBLIT_ROTATE_COORDS(psd, gc) \
{ \
	ssz = SSZ; \
	src_pitch = gc->src_pitch; \
	/* switch could be optimized out with constant PORTRAIT paramater*/ \
	switch (PORTRAIT) { \
	case NONE: \
	default: \
		dsz = DSZ;					/* dst: next pixel over*/ \
		dst_pitch = gc->dst_pitch;	/* dst: next line down*/ \
		break; \
\
	case LEFT: \
		/* change dst top left to lower left for left portrait*/ \
		/* rotate left: X -> Y, Y -> maxx - X*/ \
		tmp = gc->dsty; \
		gc->dsty = psd->xvirtres - gc->dstx - 1; \
		gc->dstx = tmp; \
\
		dsz = -gc->dst_pitch;		/* dst: next row up*/ \
		dst_pitch = DSZ;			/* dst: next pixel right*/ \
		break; \
\
	case RIGHT: \
		/* change dst top left to upper right for right portrait*/ \
 		/* Rotate right: X -> maxy - y - h, Y -> X, W -> H, H -> W*/ \
		tmp = gc->dstx; \
		gc->dstx = psd->yvirtres - gc->dsty - 1; \
		gc->dsty = tmp; \
\
		dsz = gc->dst_pitch;		/* dst: next pixel down*/ \
		dst_pitch = -DSZ;			/* dst: next pixel left*/ \
		break; \
\
	case DOWN: \
		/* change dst top left to lower right for down portrait*/ \
 		/* Rotate down: X -> maxx - x - w, Y -> maxy - y - h*/ \
		gc->dstx = psd->xvirtres - gc->dstx - 1; \
		gc->dsty = psd->yvirtres - gc->dsty - 1; \
\
		dsz = -DSZ;					/* dst: next pixel left*/ \
		dst_pitch = -gc->dst_pitch;	/* dst: next pixel up*/ \
		break; \
	} \
}

/*
 * Update screen bits if driver requires it. 
 * For convblits, we "unrotate" back to the top
 * left corner of device space.
 */
#define CONVBLIT_UPDATE(psd, gc) \
{ \
	/* switch could be optimized out with constant PORTRAIT paramater*/ \
	switch (PORTRAIT) { \
	case NONE: \
		psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height); \
		break; \
\
	case LEFT: \
		/* adjust x,y,w,h to physical top left and w/h*/ \
		psd->Update(psd, gc->dstx, gc->dsty - gc->width + 1, gc->height, gc->width); \
		break; \
\
	case RIGHT: \
		/* adjust x,y,w,h to physical top left and w/h*/ \
		psd->Update(psd, gc->dstx - gc->height + 1, gc->dsty, gc->height, gc->width); \
		break; \
\
	case DOWN: \
		/* adjust x,y,w,h to physical top left and w/h*/ \
		psd->Update(psd, gc->dstx - gc->width + 1, gc->dsty - gc->height + 1, gc->width, gc->height); \
		break; \
	} \
}


/* framebuffer pixel format blit - must handle backwards copy, nonstd rotation code*/
static inline void ALWAYS_INLINE frameblit_blit(PSD psd, PMWBLITPARMS gc,
	int SSZ, int SR, int SG, int SB, int SA,
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)
{
	int op = gc->op;
	int src_pitch, dst_pitch;
	int ssz, dsz;
	int width, height, tmp;
	unsigned char *src, *dst;

	/* handle Frame->Frame or Pixmap->Frame (FIXME still need Frame->Portrait)*/
	if (psd->portrait == gc->srcpsd->portrait)
		FRAMEBLIT_ROTATE_COORDS(psd, gc)
	else CONVBLIT_ROTATE_COORDS(psd, gc)

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

	/* src_over supported for 32bpp framebuffer only*/
	if (op == MWROP_SRC_OVER && psd->bpp != 32)
		op = MWROP_COPY;

	/*
	 * NOTE: The default implementation uses APPLYOP() which forces a
	 * switch() within the inner loop to select the rop code.  
	 * A fast implementation of MWROP_COPY is provided for speed.
	 * Any other rop can be sped up by including a case for it, and
	 * calling APPLYOP with a constant op parameter.
	 *
	 * The SRC_OVER case must be handled seperately, as APPLYOP doesn't
	 * handle it, along with the FIXME other compositing Porter-Duff ops.
	 */
	DRAWON;
	switch (op) {
	case MWROP_COPY:
//printf("blit copy\n");
		/* fast copy implementation, almost identical to default case below*/
		while (--height >= 0)
		{
			register unsigned char *d = dst;
			register unsigned char *s = src;
			int w = width;

			while (--w >= 0)
			{
				/* inline implementation will optimize out switch statement*/
				switch (DSZ) {
				case 4:
					*(ADDR32)d = *(ADDR32)s;
					break;
				case 3:
					d[DR] = s[SR];
					d[DG] = s[SG];
					d[DB] = s[SB];
					break;
				case 2:
					*(ADDR16)d = *(ADDR16)s;
					break;
				case 1:
					*(ADDR8)d = *(ADDR8)s;
					break;
				}
				d += dsz;
				s += ssz;
			}
			src += src_pitch;
			dst += dst_pitch;
		}
		break;

	case MWROP_SRC_OVER:
//printf("blit src_over\n");
		/* src_over only supported on 32bpp framebuffer*/
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;
			int w = width;

			while (--w >= 0)
			{
				unsigned int alpha;
				if ((alpha = s[SA]) == 255)				/* copy source*/
				{
					d[DR] = s[SR];
					d[DG] = s[SG];
					d[DB] = s[SB];
					d[DA] = s[SA];
				}
				else if (alpha != 0)					/* blend source w/dest*/
				{
 					/* d += muldiv255(a, s - d)*/
					d[DR] += muldiv255(alpha, s[SR] - d[DR]);
					d[DG] += muldiv255(alpha, s[SG] - d[DG]);
					d[DB] += muldiv255(alpha, s[SB] - d[DB]);

 					/* d += muldiv255(a, 255 - d)*/
					d[DA] += muldiv255(alpha, 255 - d[DA]);
				}
				d += dsz;
				s += ssz;
			}
			src += src_pitch;
			dst += dst_pitch;
		}
		break;

	case MWROP_BLENDCONSTANT:
		/* only supported on 24/32bpp framebuffer*/
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;
			int w = width;

			while (--w >= 0)
			{
				unsigned int alpha = 150;	/* blend src/dst with constant alpha*/
				if (DSZ == 2) {
					unsigned short val = ((unsigned short *)s)[0];
					unsigned short sr = REDMASK(val);
					unsigned short sg = GREENMASK(val);
					unsigned short sb = BLUEMASK(val);
					alpha = 255 - alpha + 1; /* flip alpha then add 1 (see muldiv255)*/

					/* d = muldiv255(255-a, d - s) + s*/
					((unsigned short *)d)[0] =
						muldiv255_16bpp(((unsigned short *)d)[0], sr, sg, sb, alpha);
				}
				else
				{
 					/* d += muldiv255(a, s - d)*/
					d[DR] += muldiv255(alpha, s[SR] - d[DR]);
					d[DG] += muldiv255(alpha, s[SG] - d[DG]);
					d[DB] += muldiv255(alpha, s[SB] - d[DB]);

 					/* d += muldiv255(a, 255 - d)*/
					if (DA >= 0)
						d[DA] += muldiv255(alpha, 255 - d[DA]);
				}
				d += dsz;
				s += ssz;
			}
			src += src_pitch;
			dst += dst_pitch;
		}
		break;

	default:
//printf("blit op %d\n", op);
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;

			/* inline implementation will optimize out switch statement*/
			switch (DSZ) {
			case 4:
				APPLYOP(op, width, *(ADDR32), s, *(ADDR32), d, ssz, dsz);
				break;
			case 3:
				{
					int w = width;
					while (--w >= 0)
					{
						APPLYOP(op, 3, *(ADDR8), s, *(ADDR8), d, 1, 1);
						s += ssz - 3;
						d += dsz - 3;
					}
				}
				break;
			case 2:
				APPLYOP(op, width, *(ADDR16), s, *(ADDR16), d, ssz, dsz);
				break;
			case 1:
				APPLYOP(op, width, *(ADDR8), s, *(ADDR8), d, ssz, dsz);
				break;
			}
			src += src_pitch;
			dst += dst_pitch;
		}
	}
	DRAWOFF;

	/* update screen bits if driver requires it*/
	if (!psd->Update)
		return;
	if (psd->portrait == gc->srcpsd->portrait)
		FRAMEBLIT_UPDATE(psd, gc)
	else CONVBLIT_UPDATE(psd, gc)
}

/* framebuffer pixel format copy blit - 32bpp*/
void frameblit_xxxa8888(PSD psd, PMWBLITPARMS gc)
{
	/* NOTE: src_over works for alpha in fourth byte only (RGBA and BGRA)*/
	frameblit_blit(psd, gc, 4, R,G,B,A, 4, R,G,B,A, psd->portrait);
}

/* framebuffer pixel format copy blit - 24bpp*/
void frameblit_24bpp(PSD psd, PMWBLITPARMS gc)
{
	frameblit_blit(psd, gc, 3, B,G,R,-1, 3, B,G,R,-1, psd->portrait);
}

/* framebuffer pixel format copy blit - 16bpp*/
void frameblit_16bpp(PSD psd, PMWBLITPARMS gc)
{
	frameblit_blit(psd, gc, 2, 0,0,0,0, 2, 0,0,0,1, psd->portrait);
}

/* framebuffer pixel format copy blit - 8bpp*/
void frameblit_8bpp(PSD psd, PMWBLITPARMS gc)
{
	frameblit_blit(psd, gc, 1, 0,0,0,0, 1, 0,0,0,0, psd->portrait);
}

/* framebuffer pixel format stretch blit - src/dst rotation code, no backwards copy*/
static inline void ALWAYS_INLINE frameblit_stretchblit(PSD psd, PMWBLITPARMS gc,
	int SSZ, int SR, int SG, int SB, int SA,
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)
{
	int src_x_step = gc->src_x_step; 		/* normal steps in source image*/
	int src_y_step = gc->src_y_step; 
	int src_x_step_one = gc->src_x_step_one; /* 1-unit steps in source image*/
	int src_y_step_one = gc->src_y_step_one;
	int err_x_step = gc->err_x_step; 		/* 1-unit error steps in source image*/
	int err_y_step = gc->err_y_step;
	int err_y = gc->err_y; 					/* source coordinate error tracking*/
	int err_x_start = gc->err_x;
	int dst_y_step;							/* normal steps in dest image*/
	int x_denominator = gc->x_denominator;
	int y_denominator = gc->y_denominator;
	int op = gc->op;
	int width = gc->width;
	int height = gc->height;
	int ssz, dsz;							/* inner loop step*/
	int src_pitch, dst_pitch;				/* outer loop step*/
	int tmp;
	unsigned char * src;			/* source image ptr*/
	unsigned char * dst;			/* dest image ptr*/

	CONVBLIT_ROTATE_COORDS(psd, gc)

	/* adjust step values based on bytes per pixel and pitch*/
	src_x_step 		*= ssz;
	src_x_step_one  *= ssz;
	src_y_step 		*= src_pitch;
	src_y_step_one  *= src_pitch;
	dst_y_step 		 = dst_pitch;

	src = ((unsigned char *)gc->data)     + gc->srcy * gc->src_pitch + gc->srcx * SSZ;
	dst = ((unsigned char *)gc->data_out) + gc->dsty * gc->dst_pitch + gc->dstx * DSZ;

	/*
	 * NOTE: The default implementation uses APPLYOP() which forces a
	 * switch() within the inner loop to select the rop code.  
	 * A fast implementation of MWROP_COPY is provided for speed.
	 * Both MWROP_COPY and MWROP_SRCOVER are enhanced to convert
	 * from 32/24/16bpp src to downsized destinations.
	 *
	 * Any other rop can be sped up by including a case for it, and
	 * calling APPLYOP with a constant op parameter.  The MWROP_XOR_FGBG
	 * example shows this.  A specialized very fast example of
	 * MWROP_CLEAR is also included.
	 *
	 * The SRC_OVER case must be handled seperately, as APPLYOP doesn't
	 * handle it, along with the other compositing Porter-Duff ops. FIXME
	 */
	DRAWON;
	switch (op) {
	case MWROP_COPY:
//printf("sblit copy\n");
		/* fast copy implementation, converts from different DSZ/SSZ sizes*/
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;
			int err_x = err_x_start;
			int w = width;

			while (--w >= 0)
			{
				/* inline implementation will optimize out if statements*/
				if (DSZ == 1)
					*(ADDR8)d = *(ADDR8)s;
				else if (DSZ == 2)
				{
					if (SSZ == 2)
						((unsigned short *)d)[0] = ((unsigned short *)s)[0];
					else
						((unsigned short *)d)[0] = RGB2PIXEL(s[SR], s[SG], s[SB]);
				}
				else	/* DSZ 3,4*/
				{
					d[DR] = s[SR];
					d[DG] = s[SG];
					d[DB] = s[SB];
					if (DA >= 0)
						d[DA] = s[SA];
				}
				d += dsz;
				s += src_x_step;

				err_x += err_x_step;
				if (err_x >= 0) {
					s += src_x_step_one;
					err_x -= x_denominator;
				}
			}
			dst += dst_y_step;
			src += src_y_step;

			err_y += err_y_step;
			if (err_y >= 0) {
				src += src_y_step_one;
				err_y -= y_denominator;
			}
		}
		break;

	case MWROP_SRC_OVER:
//printf("sblit src_over\n");
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;
			int err_x = err_x_start;
			int w = width;

			while (--w >= 0)
			{
				unsigned int alpha;

				if ((alpha = s[SA]) == 255)				/* copy source*/
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
						d[DR] = s[SR];
						d[DG] = s[SG];
						d[DB] = s[SB];
						if (DA >= 0)
							d[DA] = s[SA];
					}
				}
				else if (alpha != 0)					/* blend source w/dest*/
				{
					if (DSZ == 2) {
						unsigned short sr = RED2PIXEL(s[SR]);
						unsigned short sg = GREEN2PIXEL(s[SG]);
						unsigned short sb = BLUE2PIXEL(s[SB]);
						alpha = 255 - alpha + 1; /* flip alpha then add 1 (see muldiv255)*/

						/* d = muldiv255(255-a, d - s) + s*/
						((unsigned short *)d)[0] =
							muldiv255_16bpp(((unsigned short *)d)[0], sr, sg, sb, alpha);
					}
					else
					{
 						/* d += muldiv255(a, s - d)*/
						d[DR] += muldiv255(alpha, s[SR] - d[DR]);
						d[DG] += muldiv255(alpha, s[SG] - d[DG]);
						d[DB] += muldiv255(alpha, s[SB] - d[DB]);

 						/* d += muldiv255(a, 255 - d)*/
						if (DA >= 0)
							d[DA] += muldiv255(alpha, 255 - d[DA]);
					}
				}
				d += dsz;
				s += src_x_step;

				err_x += err_x_step;
				if (err_x >= 0) {
					s += src_x_step_one;
					err_x -= x_denominator;
				}
			}
			dst += dst_y_step;
			src += src_y_step;

			err_y += err_y_step;
			if (err_y >= 0) {
				src += src_y_step_one;
				err_y -= y_denominator;
			}
		}
		break;
#if EXAMPLE
	/* sample fast implementation for MWROP_CLEAR rop*/
	case MWROP_CLEAR:
		while (--height >= 0)
		{
			unsigned char *d = dst;
			int w = width;

			while (--w >= 0)
			{
				/* inline implementation will optimize out switch statement*/
				switch (DSZ) {
				case 4:
					*(ADDR32)d = 0;
					break;
				case 3:
					d[2] = d[1] = d[0] = 0;
					break;
				case 2:
					*(ADDR16)d = 0;
					break;
				case 1:
					*(ADDR8)d = 0;
					break;
				}
				d += dsz;
			}
			dst += dst_y_step;
		}
		break;

	/* sample fast implementation for MWROP_XOR_FGBG rop*/
	case MWROP_XOR_FGBG:
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;
			int err_x = err_x_start;
			int w = width;

			while (--w >= 0)
			{
				/* inline implementation will optimize out switch statement and APPLYOP switch*/
				switch (DSZ) {
				case 4:
					APPLYOP(MWROP_XOR_FGBG, 1, *(ADDR32), s, *(ADDR32), d, 0, 0);
					break;
				case 3:
					APPLYOP(op, 3, *(ADDR8), s, *(ADDR8), d, 1, 1);
					s -= 3;		// required for APPLYOP FIXME
					d -= 3;
					break;
				case 2:
					APPLYOP(MWROP_XOR_FGBG, 1, *(ADDR16), s, *(ADDR16), d, 0, 0);
					break;
				case 1:
					APPLYOP(MWROP_XOR_FGBG, 1, *(ADDR8), s, *(ADDR8), d, 0, 0);
					break;
				}
				d += dsz;
				s += src_x_step;

				err_x += err_x_step;
				if (err_x >= 0) {
					s += src_x_step_one;
					err_x -= x_denominator;
				}
			}
			dst += dst_y_step;
			src += src_y_step;

			err_y += err_y_step;
			if (err_y >= 0) {
				src += src_y_step_one;
				err_y -= y_denominator;
			}
		}
		break;
#endif /* EXAMPLE*/

	default:
//printf("sblit op %d\n", op);
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;
			int err_x = err_x_start;
			int w = width;

			while (--w >= 0)
			{
				/* inline implementation will optimize out switch statement*/
				switch (DSZ) {
				case 4:
					APPLYOP(op, 1, *(ADDR32), s, *(ADDR32), d, 0, 0);
					break;
				case 3:
					APPLYOP(op, 3, *(ADDR8), s, *(ADDR8), d, 1, 1);
					s -= 3;		// required for APPLYOP FIXME
					d -= 3;
					break;
				case 2:
					APPLYOP(op, 1, *(ADDR16), s, *(ADDR16), d, 0, 0);
					break;
				case 1:
					APPLYOP(op, 1, *(ADDR8), s, *(ADDR8), d, 0, 0);
					break;
				}
				d += dsz;
				s += src_x_step;

				err_x += err_x_step;
				if (err_x >= 0) {
					s += src_x_step_one;
					err_x -= x_denominator;
				}
			}
			dst += dst_y_step;
			src += src_y_step;

			err_y += err_y_step;
			if (err_y >= 0) {
				src += src_y_step_one;
				err_y -= y_denominator;
			}
		}
		break;
	}
	DRAWOFF;

	/* update screen bits if driver requires it*/
	if (!psd->Update)
		return;
	CONVBLIT_UPDATE(psd, gc)
}

/* framebuffer pixel format stretch blit - 32bpp with alpha in 4th byte*/
void
frameblit_stretch_xxxa8888(PSD psd, PMWBLITPARMS gc)
{
	/* NOTE: src_copy works for alpha in fourth byte only (RGBA and BGRA)*/
	frameblit_stretchblit(psd, gc, 4, R,G,B,A, 4, R,G,B,A, psd->portrait);
}

/* framebuffer pixel format stretch blit - 24bpp*/
void
frameblit_stretch_24bpp(PSD psd, PMWBLITPARMS gc)
{
	frameblit_stretchblit(psd, gc, 3, R,G,B,-1, 3, R,G,B,-1, psd->portrait);
}

/* framebuffer pixel format stretch blit - 16bpp*/
void
frameblit_stretch_16bpp(PSD psd, PMWBLITPARMS gc)
{
	frameblit_stretchblit(psd, gc, 2, 0,0,0,-1, 2, 0,0,0,-1, psd->portrait);
}

/* framebuffer pixel format stretch blit - 8bpp*/
void
frameblit_stretch_8bpp(PSD psd, PMWBLITPARMS gc)
{
	frameblit_stretchblit(psd, gc, 1, 0,0,0,-1, 1, 0,0,0,-1, psd->portrait);
}

/* framebuffer pixel format stretch blit - RGBA to BGRA*/
void
frameblit_stretch_rgba8888_bgra8888(PSD psd, PMWBLITPARMS gc)
{
	/* works for src_over and copy only*/
	frameblit_stretchblit(psd, gc, 4, R,G,B,A, 4, B,G,R,A, psd->portrait);
}

/* framebuffer pixel format stretch blit - RGBA to 24bpp BGR*/
void
frameblit_stretch_rgba8888_bgr888(PSD psd, PMWBLITPARMS gc)
{
	/* works for src_over and copy only*/
	frameblit_stretchblit(psd, gc, 4, R,G,B,A, 3, B,G,R,-1, psd->portrait);
}

/* framebuffer pixel format stretch blit - RGBA to 16bpp*/
void
frameblit_stretch_rgba8888_16bpp(PSD psd, PMWBLITPARMS gc)
{
	/* works for src_over and copy only*/
	frameblit_stretchblit(psd, gc, 4, R,G,B,A, 2, 0,0,0,-1, psd->portrait);
}
