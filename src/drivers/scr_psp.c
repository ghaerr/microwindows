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

static void
fb_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	fb_open,
	fb_close,
	fb_setpalette,
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	NULL,				/* Update*/
	NULL				/* PreSelect*/
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
    psd->ncolors = 1 << 24;
    psd->pitch = 512 * 4;
	psd->size = psd->yres * psd->pitch;
    psd->flags = PSF_SCREEN;
    psd->pixtype = MWPF_TRUECOLORABGR;
	psd->data_format = MWIF_RGBA8888;

    subdriver = select_fb_subdriver(psd);
    if (!subdriver) {
        EPRINTF("No driver for screen\n");
		return NULL;
    }
    set_subdriver(psd, subdriver);

    psd->addr = (void *)(0x40000000 | (unsigned int)sceGeEdramGetAddr());
    return psd;	/* success*/
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
}
