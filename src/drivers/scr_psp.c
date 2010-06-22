/*
 * scr_psp.c
 *
 * Microwindows screen driver for the Sony PSP by Jim Paris
 *
 * Also some generic PSP stuff here, because I'm lazy
 */
#include <pspkernel.h>
#include <pspge.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <string.h>

/* Define the module info section */
PSP_MODULE_INFO("MWINTEST", 0, 1, 1);

/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void gen_getscreeninfo(PSD psd,PMWSCREENINFO psi);

static void
stub_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
//    pspDebugScreenPrintf("no palette support, oh shit!\n");
}

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	fb_open,
	fb_close,
	gen_getscreeninfo,
	stub_setpalette,
	NULL,			/* DrawPixel subdriver*/
	NULL,			/* ReadPixel subdriver*/
	NULL,			/* DrawHorzLine subdriver*/
	NULL,			/* DrawVertLine subdriver*/
	NULL,			/* FillRect subdriver*/
	gen_fonts,
	NULL,			/* Blit subdriver*/
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	0,				/* int portrait */
	NULL,			/* orgsubdriver */
	NULL 			/* StretchBlitEx subdriver*/
};

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
    PSUBDRIVER subdriver;

    pspDebugScreenInit();

    psd->xres = psd->xvirtres = 480;
    psd->yres = psd->yvirtres = 272;
    psd->planes = 1;
    psd->bpp = 32;
    psd->ncolors = 1 << psd->bpp;
    psd->size = 0;
    psd->linelen = 512 * 4;
    psd->flags = PSF_SCREEN;
    psd->pixtype = MWPF_TRUECOLORABGR;

    subdriver = select_fb_subdriver(psd);
    if (!subdriver) {
        EPRINTF("No driver for screen\n");
		return NULL;
    }

    if(!set_subdriver(psd, subdriver, TRUE)) {
        EPRINTF("Driver initialize failed\n");
		return NULL;
    }

    psd->addr = (void *)(0x40000000 | (unsigned int)sceGeEdramGetAddr());
    return psd;	/* success*/
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
}

static void
gen_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
    psi->rows = psd->yvirtres;
    psi->cols = psd->xvirtres;
    psi->planes = psd->planes;
    psi->bpp = psd->bpp;
	psi->data_format = psd->data_format;
    psi->ncolors = psd->ncolors;
    psi->pixtype = psd->pixtype;
    psi->fonts = NUMBER_FONTS;
    psi->ydpcm = 120;
    psi->xdpcm = 120;
}
