/* kate: space-indent off; indent-width 8; replace-tabs-save off; replace-tabs off; show-tabs on;  tab-width 8; */
/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver for Linux kernel framebuffers
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 *
 * Modified for eCos by
 *   Alexander Neundorf <neundorf@kde.org>
 *   Gary Thomas <gthomas@redhat.com>
 *   Richard Panton <richard.panton@3glab.org>
 * 
 * Note: modify select_fb_driver() to add new framebuffer subdrivers

 ftok is taken from FreeBSD:

 * Copyright (c) 1994 SigmaSoft, Th. Lockert <tholo@sigmasoft.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _GNU_SOURCE 1
#include <assert.h>

#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

#include "ecos_synth_qvfb.h"

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>

////////// microwindows screen driver code

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette);
static void gen_getscreeninfo(PSD psd,PMWSCREENINFO psi);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	fb_open,
	fb_close,
	gen_getscreeninfo,
	fb_setpalette,
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
	fb_mapmemgc,
	gen_freememgc
};

/* static variables*/
static int status;		/* 0=never inited, 1=once inited, 2=inited. */

/* local functions*/
static void	set_directcolor_palette(PSD psd);

extern void qvfb_run_mwuid(void);


/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
    PSUBDRIVER subdriver;

    char* fb=qvfb_connect();

    diag_printf("******** fb is %p\n", fb);

    if (fb==NULL)
       goto fail;

//    diag_printf("DISPLAY=%s\n", cyg_hal_sys_getenv("DISPLAY"));

    assert(status < 2);

    psd->xres = psd->xvirtres = qvfb_width();
    psd->yres = psd->yvirtres = qvfb_height();

    /* set planes from fb type*/
    psd->planes = 1;   /* FIXME */

    psd->bpp = qvfb_depth();

    psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);

    /* set linelen to byte length, possibly converted later*/
    psd->linelen = qvfb_linestep();
    psd->size = 0;		/* force subdriver init of size*/

    psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
    if (psd->bpp == 16)
        psd->flags |= PSF_HAVEOP_COPY;

    /* set pixel format*/
    switch (psd->bpp) {
    case 32:
        psd->pixtype = MWPF_TRUECOLOR0888;
        break;
    case 16:
        psd->pixtype = MWPF_TRUECOLOR565;
        break;
    default:
        EPRINTF("Unsupported display type: %d\n", psd->bpp);
        goto fail;
    }

    diag_printf("%dx%dx%d linelen %d bpp %d\n", psd->xres,
      psd->yres, psd->ncolors, psd->linelen, psd->bpp);

    /* select a framebuffer subdriver based on planes and bpp*/
    subdriver = select_fb_subdriver(psd);
    if (!subdriver) {
        EPRINTF("No driver for screen\n", psd->bpp);
        goto fail;
    }

    /*
     * set and initialize subdriver into screen driver
     * psd->size is calculated by subdriver init
     */
    if(!set_subdriver(psd, subdriver, TRUE)) {
        EPRINTF("Driver initialize failed\n", psd->bpp);
        goto fail;
    }

    /* mmap framebuffer into this address space*/
    psd->addr = fb;
    if(psd->addr == NULL || psd->addr == (unsigned char *)-1) {
//        EPRINTF("Error mmaping %s: %m\n", env);
        goto fail;
    }

    status = 2;
    return psd;	/* success*/

 fail:
    return NULL;
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
   qvfb_disconnect();
}

/* setup directcolor palette - required for ATI cards*/
static void
set_directcolor_palette(PSD psd)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
}

static int fade = 100;

/* convert Microwindows palette to framebuffer format and set it*/
static void
fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
}

/* get framebuffer palette*/
void
ioctl_getpalette(int start, int len, short *red, short *green, short *blue)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
}

/* set framebuffer palette*/
void
ioctl_setpalette(int start, int len, short *red, short *green, short *blue)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
}

/* experimental palette animation*/
void
setfadelevel(PSD psd, int f)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
}

static void
gen_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
    psi->rows = psd->yvirtres;
    psi->cols = psd->xvirtres;
    psi->planes = psd->planes;
    psi->bpp = psd->bpp;
    psi->ncolors = psd->ncolors;
    psi->pixtype = psd->pixtype;
    psi->fonts = NUMBER_FONTS;

    psi->ydpcm = 42; // 320 / (3 * 2.54)
    psi->xdpcm = 38; //240 / (2.5 * 2.54)
}


