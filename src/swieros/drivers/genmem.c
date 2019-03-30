/*
 * Copyright (c) 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Screen Driver Utilities
 * 
 * Microwindows memory device routines
 */

/* alloc and initialize a new memory drawing surface (memgc)*/
PSD
GdCreatePixmap(PSD rootpsd, MWCOORD width, MWCOORD height, int format, void *pixels, int palsize)
{
	PSD		pmd;
	int 	bpp, planes, data_format, pixtype;
	unsigned int size, pitch;
   
	if (width <= 0 || height <= 0)
		return NULL;

	bpp = rootpsd->bpp;
	data_format = rootpsd->data_format;
	pixtype = rootpsd->pixtype;
	planes = rootpsd->planes;

	/* check if format supported*/
	switch (format) {
	case 0:			/* default, return framebuffer compatible pixmap*/
		break;
	case 32:		/* match framebuffer format if running 32bpp, else RGBA*/
		if (bpp == 32)
			break;
		/* else fall through - create RGBA8888 pixmap*/
	case MWIF_RGBA8888:
		bpp = 32;
		data_format = format;
		pixtype = MWPF_TRUECOLORABGR;
		break;
	case MWIF_BGRA8888:
		bpp = 32;
		data_format = format;
		pixtype = MWPF_TRUECOLOR8888;
		break;
	/*case MWIF_PAL1:*/				/* MWIF_PAL1 is MWIF_MONOBYTEMSB*/
	case MWIF_MONOBYTEMSB:			/* ft2 non-alias*/
	case MWIF_MONOBYTELSB:			/* t1lib non-alias*/
	case MWIF_MONOWORDMSB:			/* core mwcfont, pcf*/
		bpp = 1;
		data_format = format;
		pixtype = MWPF_PALETTE;
		break;
	case MWIF_PAL2:
		bpp = 2;
		data_format = format;
		pixtype = MWPF_PALETTE;
		break;
	case MWIF_PAL4:
		bpp = 4;
		data_format = format;
		pixtype = MWPF_PALETTE;
		break;
	case MWIF_PAL8:
		bpp = 8;
		data_format = format;
		pixtype = MWPF_PALETTE;
		break;
        case MWIF_RGB1555:
 	        bpp = 16;
		data_format = format;
		pixtype = MWPF_TRUECOLOR1555;
		break;
	case MWIF_RGB555:
		bpp = 16;
		data_format = format;
		pixtype = MWPF_TRUECOLOR555;
		break;
	case MWIF_RGB565:
		bpp = 16;
		data_format = format;
		pixtype = MWPF_TRUECOLOR565;
		break;
	case MWIF_RGB888:
		bpp = 24;
		data_format = format;
		pixtype = MWPF_TRUECOLOR888;
		break;
	default:
		DPRINTF("GdCreatePixmap: unsupported format %08x\n", format);
		return NULL;	/* fail*/
	}

	/*
	 * Allocate offscreen drawing surface.  If screen driver doesn't
	 * support blitting, this will fail.  Use root window screen
	 * device for compatibility.
	 */
	pmd = rootpsd->AllocateMemGC(rootpsd);
	if (!pmd)
		return NULL;

	GdCalcMemGCAlloc(pmd, width, height, planes, bpp, &size, &pitch);

	/* Allocate space for pixel values */
	if (!pixels) {
		pixels = calloc(size, 1);
		pmd->flags |= PSF_ADDRMALLOC;
	}
	if (!pixels) {
err:
		pmd->FreeMemGC(pmd);
		return NULL;
	}
 
	/* allocate palette*/
	if (palsize && (pmd->palette = calloc(palsize*sizeof(MWPALENTRY), 1)) == NULL)
		goto err;
	pmd->palsize = palsize;
 
	pmd->MapMemGC(pmd, width, height, planes, bpp, data_format, pitch, size, pixels);
	pmd->pixtype = pixtype;		/* save pixtype for proper colorval creation*/
	pmd->ncolors = (pmd->bpp >= 24)? (1 << 24): (1 << pmd->bpp);

	return pmd;
}

void
GdFreePixmap(PSD pmd)
{
	pmd->FreeMemGC(pmd);
}

/* allocate a memory offscreen screen device (pixmap)*/
PSD 
gen_allocatememgc(PSD psd)
{
	PSD	mempsd;

	mempsd = malloc(sizeof(SCREENDEVICE));
	if (!mempsd)
		return NULL;

	/* copy passed device get initial values*/
	*mempsd = *psd;

	/* initialize*/
	mempsd->flags = PSF_MEMORY;			/* reset PSF_SCREEN or PSF_ADDRMALLOC flags*/
	mempsd->portrait = MWPORTRAIT_NONE; /* don't rotate offscreen pixmaps*/
	mempsd->addr = NULL;
	mempsd->Update = NULL;				/* no external updates required for mem device*/
	mempsd->palette = NULL;				/* don't copy any palette*/
	mempsd->palsize = 0;
	mempsd->transcolor = MWNOCOLOR;		/* no transparent colors unless set by image loader*/

	return mempsd;
}

/* 
 * Initialize memory device with passed parms,
 * select suitable framebuffer subdriver,
 * and set subdriver in memory device.
 *
 * Pixmaps are always drawn using linear fb drivers
 * in non-portrait mode.
 */
MWBOOL
gen_mapmemgc(PSD mempsd, MWCOORD w, MWCOORD h, int planes, int bpp, int data_format,
	unsigned int pitch, int size, void *addr)
{
	PSUBDRIVER subdriver;

	/* pixmaps are always in non-portrait orientation*/
	mempsd->xres = w;
	mempsd->yres = h;		

	mempsd->xvirtres = w;
	mempsd->yvirtres = h;
	mempsd->planes = planes;
	mempsd->bpp = bpp;
	mempsd->data_format = data_format;
	mempsd->pitch = pitch;
	mempsd->size = size;
	mempsd->addr = addr;

	/* select and init hw compatible framebuffer subdriver for pixmap drawing*/
	subdriver = select_fb_subdriver(mempsd);
	if(!subdriver)
		return 0;

	/* assign portrait subdriver or regular fb driver for pixmap drawing*/
	set_portrait_subdriver(mempsd);

	return 1;
}

void
gen_freememgc(PSD mempsd)
{
//	/* may be an image header, just return*/
//	if (!(mempsd->flags & PSF_MEMORY))
//		return;

	if (mempsd->addr && (mempsd->flags & PSF_ADDRMALLOC))
		free(mempsd->addr);

	if (mempsd->palette)
		free(mempsd->palette);
	free(mempsd);
}

/*
 * Calculate size and pitch of memory gc.
 * If bpp or planes is 0, use passed psd's bpp/planes.
 * Pitch is calculated to be DWORD right aligned for speed.
 */
int
GdCalcMemGCAlloc(PSD psd, int width, int height, int planes, int bpp,
	unsigned int *psize, unsigned *ppitch)
{
	unsigned int pitch;

	if(!planes)
		planes = psd->planes;
	if(!bpp)
		bpp = psd->bpp;
	/* 
	 * swap width and height in left/right portrait modes,
	 * so imagesize is calculated properly
	 */
	if(psd->portrait & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT)) {
		int tmp = width;
		width = height;
		height = tmp;
	}

	/* use 4bpp linear for VGA 4 planes memdc format*/
	if(planes == 4)
		bpp = 4;

	/* compute pitch: bytes per line*/
	switch(bpp) {
	case 1:
		pitch = (width+7)/8;
		break;
	case 2:
		pitch = (width+3)/4;
		break;
	case 4:
		pitch = (width+1)/2;
		break;
	case 8:
		pitch = width;
		break;
	case 16:
		pitch = width * 2;
		break;
	case 18:
	case 24:
		pitch = width * 3;
		break;
	case 32:
		pitch = width * 4;
		break;
	default:
		*ppitch = *psize = 0;
		return 0;
	}

	/* right align pitch to DWORD boundary*/
	pitch = (pitch + 3) & ~3;

	*psize = pitch * height;
	*ppitch = pitch;
	return 1;
}

void
gen_setportrait(PSD psd, int portraitmode)
{
	psd->portrait = portraitmode;

	/* swap x and y in left or right portrait modes*/
	if (portraitmode & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT)) {
		/* swap x, y*/
		psd->xvirtres = psd->yres;
		psd->yvirtres = psd->xres;
	} else {
		/* normal x, y*/
		psd->xvirtres = psd->xres;
		psd->yvirtres = psd->yres;
	}

	/* assign portrait subdriver or original driver*/
	set_portrait_subdriver(psd);
}

/*
 * Set portrait subdriver or original subdriver according
 * to current portrait mode.
 */
void
set_portrait_subdriver(PSD psd)
{
	PSUBDRIVER subdriver;

	switch (psd->portrait) {
	case MWPORTRAIT_NONE:
	default:
		subdriver = psd->orgsubdriver;
		break;
	case MWPORTRAIT_LEFT:
		subdriver = psd->left_subdriver;
		break;
	case MWPORTRAIT_RIGHT:
		subdriver = psd->right_subdriver;
		break;
	case MWPORTRAIT_DOWN:
		subdriver = psd->down_subdriver;
		break;
	}
	set_subdriver(psd, subdriver);
}

void
gen_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	/* temporarily stop updates for speed*/
	void (*Update)(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height) = psd->Update;
	int X1 = x1;
	int Y1 = y1;
	psd->Update = NULL;

	if (psd->portrait & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT))
		while(x1 <= x2)
			psd->DrawVertLine(psd, x1++, y1, y2, c);
	else
		while(y1 <= y2)
			psd->DrawHorzLine(psd, x1, x2, y1++, c);

	/* now redraw once if external update required*/
	if (Update) {
		Update(psd, X1, Y1, x2-X1+1, y2-Y1+1);
		psd->Update = Update;
	}
}

/*
 * Set subdriver entry points in screen device
 */
void
set_subdriver(PSD psd, PSUBDRIVER subdriver)
{
	/* set subdriver entry points in screen driver*/
	psd->DrawPixel 		= subdriver->DrawPixel;
	psd->ReadPixel 		= subdriver->ReadPixel;
	psd->DrawHorzLine 	= subdriver->DrawHorzLine;
	psd->DrawVertLine 	= subdriver->DrawVertLine;
	psd->FillRect	 	= subdriver->FillRect;
	psd->BlitFallback 	= subdriver->BlitFallback;
	psd->FrameBlit 				 = subdriver->FrameBlit;
	psd->FrameStretchBlit 		 = subdriver->FrameStretchBlit;
	psd->BlitCopyMaskMonoByteMSB = subdriver->BlitCopyMaskMonoByteMSB;
	psd->BlitCopyMaskMonoByteLSB = subdriver->BlitCopyMaskMonoByteLSB;
	psd->BlitCopyMaskMonoWordMSB = subdriver->BlitCopyMaskMonoWordMSB;
	psd->BlitBlendMaskAlphaByte  = subdriver->BlitBlendMaskAlphaByte;
	psd->BlitCopyRGBA8888     	 = subdriver->BlitCopyRGBA8888;
	psd->BlitSrcOverRGBA8888     = subdriver->BlitSrcOverRGBA8888;
	psd->BlitCopyRGB888          = subdriver->BlitCopyRGB888;
	psd->BlitStretchRGBA8888     = subdriver->BlitStretchRGBA8888;
}

/* fill in a subdriver struct from passed screen device*/
void
get_subdriver(PSD psd, PSUBDRIVER subdriver)
{
	/* set subdriver entry points in screen driver*/
	subdriver->DrawPixel 		= psd->DrawPixel;
	subdriver->ReadPixel 		= psd->ReadPixel;
	subdriver->DrawHorzLine 	= psd->DrawHorzLine;
	subdriver->DrawVertLine 	= psd->DrawVertLine;
	subdriver->FillRect	 		= psd->FillRect;
	subdriver->BlitFallback 	= psd->BlitFallback;
	subdriver->FrameBlit 			   = psd->FrameBlit;
	subdriver->FrameStretchBlit 	   = psd->FrameStretchBlit;
	subdriver->BlitCopyMaskMonoByteMSB = psd->BlitCopyMaskMonoByteMSB;
	subdriver->BlitCopyMaskMonoByteLSB = psd->BlitCopyMaskMonoByteLSB;
	subdriver->BlitCopyMaskMonoWordMSB = psd->BlitCopyMaskMonoWordMSB;
	subdriver->BlitBlendMaskAlphaByte  = psd->BlitBlendMaskAlphaByte;
	subdriver->BlitCopyRGBA8888        = psd->BlitCopyRGBA8888;
	subdriver->BlitSrcOverRGBA8888     = psd->BlitSrcOverRGBA8888;
	subdriver->BlitCopyRGB888          = psd->BlitCopyRGB888;
	subdriver->BlitStretchRGBA8888     = psd->BlitStretchRGBA8888;
}
