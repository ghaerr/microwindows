/*
 * Copyright (c) 2000, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Screen Driver Utilities
 * 
 * Microwindows framebuffer selection routines
 * Including this file will drag in all fblinX framebuffer subdrivers
 * When a new framebuffer subdriver is written, it should be referenced
 * here in select_fb_driver.
 */

/*
 * Initialize a PSD struct for screen driver open routine
 * Allocate framebuffer memory if PSF_ADDRMALLOC passed
 */
int
gen_initpsd(PSD psd, int pixtype, MWCOORD xres, MWCOORD yres, int flags)
{
	PSUBDRIVER subdriver;

	psd->pixtype = pixtype;				/* usually SCREEN_PIXTYPE in config*/
	psd->xres = psd->xvirtres = xres;	/* usually SCREEN_WIDTH in config*/
	psd->yres = psd->yvirtres = yres;	/* usually SCREEN_HEIGHT in config*/
	psd->flags = flags;

	/* use pixel format to set bpp*/
	switch (psd->pixtype) {
	case MWPF_TRUECOLORARGB:
	case MWPF_TRUECOLORABGR:
	default:
		psd->bpp = 32;
		break;

	case MWPF_TRUECOLORRGB:
		psd->bpp = 24;
		break;

	case MWPF_TRUECOLOR565:
	case MWPF_TRUECOLOR555:
		psd->bpp = 16;
		break;

	case MWPF_TRUECOLOR332:
		psd->bpp = 8;
		break;

	}
	psd->planes = 1;

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* Calculate the correct size and pitch from xres, yres and bpp*/
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp, &psd->size, &psd->pitch);

	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->portrait = MWPORTRAIT_NONE;

	/* select an fb subdriver matching our planes and bpp for backing store*/
	subdriver = select_fb_subdriver(psd);
	psd->orgsubdriver = subdriver;
	if (!subdriver)
		return 0;

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

	/*
	 * Allocate framebuffer if requested
	 * psd->size is calculated by subdriver init
	 */
	if ((flags & PSF_ADDRMALLOC) && (psd->addr = malloc(psd->size)) == NULL)
		return 0;
	return 1;		/* success*/
}


/* select a framebuffer subdriver based on planes and bpp*/
/* modify this procedure to add a new framebuffer subdriver*/
PSUBDRIVER 
select_fb_subdriver(PSD psd)
{
	PSUBDRIVER *pdriver = NULL;
	extern PSUBDRIVER fblinear32rgba[4];

	pdriver = fblinear32rgba;

	if (!pdriver)
		return NULL;

	/* remember normal and portrait subdrivers*/
	psd->orgsubdriver = pdriver[0];
	psd->left_subdriver = pdriver[1];
	psd->right_subdriver = pdriver[2];
	psd->down_subdriver = pdriver[3];

	/* return driver selected*/
	return pdriver[0];
}

/* set standard data_format from bpp and pixtype*/
int
set_data_format(PSD psd)
{
	int data_format = 0;

	switch(psd->pixtype) {
	case MWPF_TRUECOLOR8888:
		data_format = MWIF_BGRA8888;
		break;
	case MWPF_TRUECOLORABGR:
		data_format = MWIF_RGBA8888;
		break;
	case MWPF_TRUECOLOR888:
		data_format = MWIF_BGR888;
		break;
	case MWPF_TRUECOLOR565:
		data_format = MWIF_RGB565;
		break;
	case MWPF_TRUECOLOR555:
		data_format = MWIF_RGB555;
		break;
	case MWPF_TRUECOLOR1555:
	        data_format = MWIF_RGB1555;
		break;
	case MWPF_TRUECOLOR332:
		data_format = MWIF_RGB332;
		break;
	case MWPF_TRUECOLOR233:
		data_format = MWIF_BGR233;
		break;
	case MWPF_PALETTE:
		switch (psd->bpp) {
		case 8:
			data_format = MWIF_PAL8;
			break;
		case 4:
			data_format = MWIF_PAL4;
			break;
		case 2:
			data_format = MWIF_PAL2;
			break;
		case 1:
			data_format = MWIF_PAL1;
			break;
		}
		break;
	}

	return data_format;
}

/* general routine to return screen info, ok for all fb devices*/
void
gen_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->data_format = psd->data_format;
	psi->ncolors = psd->ncolors;
	psi->fonts = NUMBER_FONTS;
	psi->portrait = psd->portrait;
	psi->fbdriver = TRUE;	/* running fb driver, can direct map*/
	psi->pixtype = psd->pixtype;

	switch (psd->data_format) {
	case MWIF_BGRA8888:
		psi->rmask = RMASKBGRA;
		psi->gmask = GMASKBGRA;
		psi->bmask = BMASKBGRA;
		psi->amask = AMASKBGRA;
		break;
	default:
		psi->amask 	= 0x00;
		psi->rmask 	= 0xff;
		psi->gmask 	= 0xff;
		psi->bmask	= 0xff;
		break;
	}

	if(psd->yvirtres > 600) {	// FIXME update
		/* SVGA 1024x768*/
		psi->xdpcm = 42;
		psi->ydpcm = 42;
	} else if(psd->yvirtres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(psd->yvirtres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
        } else if(psd->yvirtres <= 240) {
		/* half VGA 640x240 */
		psi->xdpcm = 14;        /* assumes screen width of 24 cm*/ 
		psi->ydpcm =  5;
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}
