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

#define COPY	0		/* mode parm*/
#define SRCOVER	1

static unsigned int  muldiv255(unsigned int a,unsigned int b)
{
	return ((((a)+1)*(b))>>8);		/* very fast, 92% accurate*/
}

/* framebuffer pixel format blit - must handle backwards copy, nonstd rotation code*/
static void frameblit_blit(PSD psd, PMWBLITPARMS gc,
	int SSZ, int SR, int SG, int SB, int SA,
	int DSZ, int DR, int DG, int DB, int DA, int PORTRAIT)
{
	int op = gc->op;
	int src_pitch, dst_pitch;
	int ssz, dsz;
	int width, height, tmp;
	unsigned char *src, *dst;
	/* handle Frame->Frame or Pixmap->Frame (FIXME still need Frame->Portrait)*/
	//if (psd->portrait == gc->srcpsd->portrait)
	src_pitch = gc->src_pitch;
	dsz = DSZ;					/* dst: next pixel over*/
	dst_pitch = gc->dst_pitch;	/* dst: next line down*/
	//else CONVBLIT_ROTATE_COORDS(psd, gc)

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
		/* fast copy implementation, almost identical to default case below*/
		while (--height >= 0)
		{
			register unsigned char *d = dst;
			register unsigned char *s = src;
			int w = width;

			while (--w >= 0)
			{
				*(ADDR32)d = *(ADDR32)s;
				d += dsz;
				s += ssz;
			}
			src += src_pitch;
			dst += dst_pitch;
		}
		break;

	case MWROP_SRC_OVER:
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
		while (--height >= 0)
		{
			register unsigned char *s = src;
			register unsigned char *d = dst;

			APPLYOP_SRC_PTR(op, width, s, d, ssz, dsz);
			src += src_pitch;
			dst += dst_pitch;
		}
	}
	DRAWOFF;

	/* update screen bits if driver requires it*/
	//if (psd->portrait == gc->srcpsd->portrait)
		if (psd->Update)
			psd->Update(psd, gc->dstx, gc->dsty, gc->width, gc->height);
	//else CONVBLIT_UPDATE(psd, gc)
}

/* framebuffer pixel format copy blit - 32bpp*/
void frameblit_xxxa8888(PSD psd, PMWBLITPARMS gc)
{
	/* NOTE: src_over works for alpha in fourth byte only (RGBA and BGRA)*/
	//frameblit_blit(psd, gc, 4, R,G,B,A, 4, R,G,B,A, psd->portrait);
	  frameblit_blit(psd, gc, 4, 0,1,2,3, 4, 0,1,2,3, psd->portrait);
}
