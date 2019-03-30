/*
 * Copyright (c) 1999,2000,2001,2003,2005,2007,2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Device-independent mid level blit routines.
 *
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 */

/* find a conversion blit based on data format and blit op*/
/* used by GdBitmap, GdArea and GdDrawImage*/
MWBLITFUNC
GdFindConvBlit(PSD psd, int data_format, int op)
{
	MWBLITFUNC convblit = NULL;

	//convblit = psd->BlitCopyMaskMonoWordMSB;	/* conv mono word MSBFirst*/
	/* determine which blit to use*/
	switch (data_format) {
	case MWIF_ALPHABYTE:			/* ft2 alias, t1lib alias*/
		//convblit = psd->BlitBlendMaskAlphaByte;		/* conv 8bpp alpha with fg/bg*/
		break;

	case MWIF_MONOBYTEMSB:			/* ft2 non-alias*/
		//convblit = psd->BlitCopyMaskMonoByteMSB;	/* conv mono byte MSBFirst*/
		break;

	case MWIF_MONOWORDMSB:			/* core mwcfont, pcf*/
		convblit = psd->BlitCopyMaskMonoWordMSB;	/* conv mono word MSBFirst*/
		break;

	case MWIF_MONOBYTELSB:			/* t1lib non-alias*/
		//convblit = psd->BlitCopyMaskMonoByteLSB;	/* conv mono byte LSBFirst*/
		break;

	case MWIF_RGBA8888:				/* png 32bpp w/alpha, GdArea MWPF_RGB/MWPF_TRUECOLORABGR*/
		//if (op == MWROP_SRC_OVER) {
			//convblit = psd->BlitSrcOverRGBA8888;	/* image, src 32bpp w/alpha - srcover*/
			//break;
		//}
		/* assume copy*/
		//convblit = psd->BlitCopyRGBA8888;			/* GdArea MWPF_RGB*/
		break;

	case MWIF_BGRA8888:				/* GdArea MWPF_TRUECOLOR8888*/
		/* assume copy*/
		//if (psd->data_format == MWIF_BGRA8888)
			//convblit = convblit_copy_8888_8888;		/* 32bpp to 32bpp copy*/
		//else if (psd->data_format == MWIF_BGR888)	/* GdArea MWPF_PIXELVAL conversion*/
			//convblit = convblit_copy_bgra8888_bgr888; /* 32bpp BGRX to 24bpp BGR copy*/
		break;

	case MWIF_RGB888:				/* png 24bpp no alpha*/
		//convblit = psd->BlitCopyRGB888;				/* image, src 24bpp - copy*/
		break;

	case MWIF_BGR888:				/* GdArea MWPF_TRUECOLOR888*/
		//if (psd->data_format == MWIF_BGR888)
			//convblit = convblit_copy_888_888;		/* 24bpp to 24bpp copy*/
		break;

	case MWIF_RGB565:				/* GdArea MWPF_TRUECOLOR565*/
	case MWIF_RGB555:				/* GdArea MWPF_TRUECOLOR555*/
	case MWIF_RGB1555:                              /* GdArea MWPF_TRUECOLOR1555*/
		//if (psd->data_format == data_format)
			//convblit = convblit_copy_16bpp_16bpp;	/* 16bpp to 16bpp copy*/
		break;
	}

	return convblit;
}

/* blit from non-psd source to destination*/
/* used by gen_drawtext, FT2 and T1LIB*/
void
GdConversionBlit(PSD psd, PMWBLITPARMS parms)
{
	/* first find blit based on data format and blit op*/
	MWBLITFUNC convblit = GdFindConvBlit(psd, parms->data_format, parms->op);

	/* call conversion blit routine with clipping*/
	if (convblit) {
		/* setup destination parms*/
		parms->dst_pitch = psd->pitch;
		parms->data_out = psd->addr;
		parms->srcpsd = NULL;		/* used in frameblits only*/

		GdConvBlitInternal(psd, parms, convblit);
		return;
	}
	DPRINTF("GdConversionBlit: No convblit available\n");
}

/* fallback blitter wrapper for older 1,2,4 bpp subdrivers*/
static void
BlitFallback(PSD psd, PMWBLITPARMS gc)
{
	DPRINTF("GdBlit: No frameblit, using psd->BlitFallBack\n");
	psd->BlitFallback(psd, gc->dstx, gc->dsty, gc->width, gc->height,
		gc->srcpsd, gc->srcx, gc->srcy, gc->op);
}

/* find a framebuffer blit based on source data format and blit op*/
/* used by GdBlit*/
static MWBLITFUNC
GdFindFrameBlit(PSD psd, int src_data_format, int op)
{
	/* try conversion blits if possible*/
	switch (src_data_format) {
	case MWIF_RGBA8888:
		if (op == MWROP_SRC_OVER) {
			if (psd->BlitSrcOverRGBA8888)
				return psd->BlitSrcOverRGBA8888;
		}
		if (psd->BlitCopyRGBA8888)
			return psd->BlitCopyRGBA8888;
		break;

	case MWIF_MONOBYTEMSB:
		/* use conversion blit if destination not palette*/
		//FIXME this won't work if this function merged with GdFindConvBlit
		if (psd->BlitCopyMaskMonoByteMSB && psd->bpp >= 8)
			return psd->BlitCopyMaskMonoByteMSB;
		break;
	}

	/* try fallback blit if no frameblit*/
	if (!psd->FrameBlit) {
		if (!psd->BlitFallback)
			return NULL;
		return BlitFallback;		/* wrapper function to reorder parameters*/
	}

	/* BGRA->BGRA is handled properly with frameblit_xxxa in fblin32.c*/

	/* use frameblit*/
	return psd->FrameBlit;
}

/**
 * Copy source rectangle of pixels to destination rectangle quickly
 *
 * @param dstpsd Drawing surface to draw to.
 * @param dstx Destination X co-ordinate.
 * @param dsty Destination Y co-ordinate.
 * @param width Width of rectangle to copy.
 * @param height Height of rectangle to copy.
 * @param srcpsd Drawing surface to copy from.
 * @param srcx Source X co-ordinate.
 * @param srcy Source Y co-ordinate.
 * @param rop Raster operation.
 */
/* Copy from srcpsd to dstpsd. Source and/or dest may be rotated, and/or same psd.*/
void
GdBlit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD width, MWCOORD height,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int rop)
{
	MWBLITFUNC	frameblit;
	MWBLITPARMS parms;

	/* Find appropriate blitter based on source data format and rop*/
	frameblit = GdFindFrameBlit(dstpsd, srcpsd->data_format, rop);
	if (!frameblit) {
		DPRINTF("GdBlit: No frameblit found for op %d\n", rop);
		return;
	}
	
	/* clip blit rectangle to source screen/bitmap size*/
	/* we must do this because there isn't any source clipping setup*/
	if (srcx < 0) {
		width += srcx;
		dstx -= srcx;
		srcx = 0;
	}
	if (srcy < 0) {
		height += srcy;
		dsty -= srcy;
		srcy = 0;
	}
	if (srcx + width > srcpsd->xvirtres)
		width = srcpsd->xvirtres - srcx;
	if (srcy + height > srcpsd->yvirtres)
		height = srcpsd->yvirtres - srcy;

	parms.op = rop;
	parms.data_format = dstpsd->data_format;
	parms.width = width;
	parms.height = height;
	parms.dstx = dstx;
	parms.dsty = dsty;
	parms.srcx = srcx;
	parms.srcy = srcy;
	parms.src_pitch = srcpsd->pitch;

	parms.fg_colorval = gr_foreground_rgb;	/* for mask convblit*/
	parms.bg_colorval = gr_background_rgb;
	parms.fg_pixelval = gr_foreground;		/* for palette mask convblit*/
	parms.bg_pixelval = gr_background;
	parms.usebg = gr_usebg;

	parms.data = srcpsd->addr;
	parms.dst_pitch = dstpsd->pitch;		/* usually set in GdConversionBlit*/
	parms.data_out = dstpsd->addr;
	parms.srcpsd = srcpsd;					/* for GdCheckCursor/GdFixCursor*/
	parms.src_xvirtres = srcpsd->xvirtres;	/* used in frameblit for src rotation*/
	parms.src_yvirtres = srcpsd->yvirtres;

	GdConvBlitInternal(dstpsd, &parms, frameblit);
}

/* call conversion blit with clipping and cursor fix*/
void
GdConvBlitInternal(PSD psd, PMWBLITPARMS gc, MWBLITFUNC convblit)
{
	MWCOORD dstx = gc->dstx;
	MWCOORD dsty = gc->dsty;
	MWCOORD width = gc->width;
	MWCOORD height = gc->height;
	MWCOORD srcx = gc->srcx;
	MWCOORD srcy = gc->srcy;
	int count, clipresult, checksrc;
	MWCLIPRECT *prc;
	extern MWCLIPRECT cliprects[];
	extern int clipcount;

	/* check clipping region*/
	clipresult = GdClipArea(psd, dstx, dsty, dstx + width - 1, dsty + height - 1);
	if (clipresult == CLIP_INVISIBLE)
		return;

	/* check cursor in src region of both screen devices*/
	GdCheckCursor(psd, srcx, srcy, srcx + width - 1, srcy + height - 1);
	if ((checksrc = (gc->srcpsd != NULL && gc->srcpsd != psd)) != 0)
		GdCheckCursor(gc->srcpsd, srcx, srcy, srcx + width - 1, srcy + height - 1);

	if (clipresult == CLIP_VISIBLE) {
		convblit(psd, gc);
		GdFixCursor(psd);
		if (checksrc)
			GdFixCursor(gc->srcpsd);
		return;
	} else	/* partially clipped, check cursor in dst region once*/
		GdCheckCursor(psd, dstx, dsty, dstx + width - 1, dsty + height - 1);

	/* we'll traverse visible region and draw*/
	prc = cliprects;
	count = clipcount;

	while (count-- > 0) {
		MWCOORD rx1, rx2, ry1, ry2, rw, rh;
		rx1 = prc->x;		/* old clip-code*/
		ry1 = prc->y;
		rx2 = prc->x + prc->width;
		ry2 = prc->y + prc->height;

		/* Check if this rect intersects with the one we draw */
		if (rx1 < dstx) rx1 = dstx;
		if (ry1 < dsty) ry1 = dsty;
		if (rx2 > dstx + width) rx2 = dstx + width;
		if (ry2 > dsty + height) ry2 = dsty + height;

		rw = rx2 - rx1;
		rh = ry2 - ry1;

		if (rw > 0 && rh > 0) {
			gc->dstx = rx1;
			gc->dsty = ry1;
			gc->width = rw;
			gc->height = rh;
			gc->srcx = srcx + rx1 - dstx;
			gc->srcy = srcy + ry1 - dsty;
			convblit(psd, gc);
		}
		prc++;
	}
	GdFixCursor(psd);
	if (checksrc)
		GdFixCursor(gc->srcpsd);

	/* Reset everything, in case the caller re-uses it. */
	gc->dstx = dstx;
	gc->dsty = dsty;
	gc->width = width;
	gc->height = height;
	gc->srcx = srcx;
	gc->srcy = srcy;
}
