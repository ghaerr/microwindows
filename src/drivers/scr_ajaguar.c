/*
 *
 *  Screen Atari Jaguar driver by Jean-Paul Mari 2019
 * 
 *  For testing the Makefile for a new platform before writing a special screen driver
 * 
 *  will output to stdout when pixels for the screen are received, no graphics screen
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"
#include "JAGUAR.H"

static PSD  AJAGUAR_open(PSD psd);
static void AJAGUAR_close(PSD psd);
static void AJAGUAR_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void AJAGUAR_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void AJAGUAR_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 320
#endif
#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 240
#endif
#ifndef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE MWPF_PALETTE
#endif
#define SCREEN_AJAGUAR

SUBDRIVER subdriver;
#ifdef SCREEN_AJAGUAR
extern unsigned char ajag_screen[];
#endif

SCREENDEVICE scrdev __attribute__ ((section (".mwjagdata"))) = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	AJAGUAR_open,
	AJAGUAR_close,
	AJAGUAR_setpalette,       
	AJAGUAR_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,  		/*gen_setportrait */
	AJAGUAR_update,	/* Update*/
	NULL		/* PreSelect*/
};

/*
**	Open graphics
*/
static PSD AJAGUAR_open(PSD psd)
{
	PSUBDRIVER subdriver;
	int avwidth,avheight,avbpp;
	
	avwidth = SCREEN_WIDTH;
	avheight = SCREEN_HEIGHT;

#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR8888)
	if (SCREEN_PIXTYPE == MWPF_TRUECOLOR8888)
	{
		avbpp=32;
	}
	else
#endif
	{
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR888)
		if (SCREEN_PIXTYPE == MWPF_TRUECOLOR888)
		{
			avbpp = 24;
		}
		else
#endif
		{
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR565)
			if (SCREEN_PIXTYPE == MWPF_TRUECOLOR565)
			{
				avbpp = 16;
			}
			else
#endif
			{
				avbpp = 8; //palette
			}
		}
	}

	psd->xres = psd->xvirtres = avwidth; //GrScreenX();
	psd->yres = psd->yvirtres = avheight; //GrScreenY();
	psd->planes = 1;
	psd->bpp = avbpp; //md_info->bpp;
	psd->ncolors = (psd->bpp >= 24) ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_ADDRMALLOC;
	/* Calculate the correct size and linelen here */
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp, &psd->size, &psd->pitch);

#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR8888)
    if (psd->bpp == 32)
	{
		psd->pixtype = MWPF_TRUECOLOR8888;	
	}
	else
#endif
	{
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR565)
		if (psd->bpp == 16)
		{
			psd->pixtype = MWPF_TRUECOLOR565;
		}
		else
#endif
		{
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR888)
			if (psd->bpp == 24)
			{
				psd->pixtype = MWPF_TRUECOLOR888;
			}
			else
#endif
			{
				psd->pixtype = MWPF_PALETTE;
			}
		}
	}
		  
	psd->portrait = MWPORTRAIT_NONE;
	psd->data_format = set_data_format(psd);

	/* set and initialize subdriver into screen driver, psd->size is calculated by subdriver init */
	subdriver = select_fb_subdriver(psd);
	psd->orgsubdriver = subdriver;
	set_subdriver(psd, subdriver);

	/* set palette information */
#if (MWPIXEL_FORMAT == MWPF_PALETTE)
	psd->palsize = 256 * sizeof(MWPALENTRY);
	if (!(psd->palette = malloc(psd->palsize)))
	{
		return NULL;
	}
	else
#endif
	{
		/* framebuffer memory allocation */
#if !HAVE_BLITTER_SUPPORT
		if (!(psd->addr = malloc(psd->size)))
#else
		if (!(psd->addr = ((unsigned long)malloc(psd->size) & ~7)))
#endif
		{
			return NULL;
		}
	}

	return psd;
}

/*
**	Close graphics
*/
static void AJAGUAR_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);
	/* free palette */
	free(psd->palette);
}

/*
**	Get Screen Info
*/
static void AJAGUAR_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);

#if SCREEN_HEIGHT == 240
	/* 320x240 */
	psi->xdpcm = 13;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 13;	/* assumes screen height of 18 cm*/
#else
	if (scrdev.yvirtres > 600)
	{
		/* SVGA 1024x768*/
		psi->xdpcm = 42;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 42;	/* assumes screen height of 18 cm*/        
	}
	else
	{
		if (scrdev.yvirtres > 480)
		{
			/* SVGA 800x600*/
			psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
			psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
		}
		else
		{
			if (scrdev.yvirtres > 350)
			{
				/* VGA 640x480*/
				psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
				psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
			}
			else
			{
				/* EGA 640x350*/
				psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
				psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
			}
		}
	}
#endif
}

/*
**	Set Palette
Atari Jaguar palette encoding:
Bits 0-5  : Green
Bits 6-10 : Blue
Bits 11-15: Red
*/
static void AJAGUAR_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
#ifdef SCREEN_AJAGUAR
	if (psd->pixtype == MWPF_PALETTE)
	{
		unsigned short int *Ptr = (unsigned short int *)(CLUT + (first * 2));			// 0xF00400
		pal += first;

		while (count--)
		{
#if 0
			unsigned short int r = (pal->r > 0x1f) ? (pal->r >> 3) : pal->r;
			unsigned short int b = (pal->b > 0x1f) ? (pal->b >> 3) : pal->b;
			unsigned short int g = (pal->g > 0x3f) ? (pal->g >> 2) : pal->g;

			*Ptr++ = ((r << 11) | (b << 6) | g);
#elif 0
			if ((pal->r < 0x20) && (pal->b < 0x20) && (pal->g < 0x40))
			{
				*Ptr++ = ((pal->r << 11) | (pal->b << 6) | pal->g);
			}
			else
			{
				*Ptr++ = ((pal->r >> 3) << 11) | ((pal->b >> 3) << 6) | (pal->g >> 2);
			}
#else
			*Ptr++ = ((pal->r >> 3) << 11) | ((pal->b >> 3) << 6) | (pal->g >> 2);
#endif
			pal++;
		}
	}
#endif
}

/*
**	Blit
*/
static void AJAGUAR_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
#if 0
    static uint32_t counter;
#endif
#if !HAVE_BLITTER_SUPPORT
    unsigned char *addr;
#ifdef SCREEN_AJAGUAR
	unsigned char *jaddr;
	int ly, lx;
#endif
#endif

    if ((psd->pixtype == MWPF_TRUECOLOR332) || (psd->pixtype == MWPF_PALETTE))
	{
#if !HAVE_BLITTER_SUPPORT
		addr = psd->addr + desty * psd->pitch + destx;
#ifdef SCREEN_AJAGUAR
		jaddr = &ajag_screen[desty * psd->pitch + destx];

		for (ly = 0; ly < height; ly++)
		{
			for (lx = 0; lx < width; lx++)
			{
				*jaddr++ = *addr++;
			}

			addr += psd->xres - width;
			jaddr += psd->xres - width;
		}
#endif
#else
		while (!(*B_CMD & 1));
		*A1_BASE = &ajag_screen;											// [(desty * psd->pitch) + destx];
		*A2_BASE = psd->addr;												// +((desty * psd->pitch) + destx);
		*A1_PIXEL = *A2_PIXEL = (desty << 16) | destx;
		*A1_FLAGS = *A2_FLAGS = (PIXEL8 | XADDPIX | WID320 | PITCH1);
		//*A1_STEP = *A2_STEP = (1 << 16) | (short int)-320;
		*B_COUNT = (height << 16) | width;
		*B_CMD = LFU_REPLACE | SRCEN;
#endif
	}
#if 0
	else
	{
		if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
		{
			addr = psd->addr + desty * psd->pitch + (destx << 1);
		}
		else
		{
			if (psd->pixtype == MWPF_TRUECOLOR888)
			{
				addr = psd->addr + desty * psd->pitch + destx * 3;
			}
			else
			{
				if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)))
				{
					addr = psd->addr + desty * psd->pitch + (destx << 2);
				}
			}
		}
	}
#endif

#if 0
    /* print data to stdout */
    counter++; //count calls to AJAGUAR_update()
    //fprintf(stdout,"Receiving pixels -  call #:%u, xpos:%d, ypos:%d, width:%d, height:%d, ",counter,destx,desty,width,height); 
    if (counter > 4000000000)
	{
		counter=0;
	} /* wrap around */
    int printwidth=width*4; /* four bytes per pixel max */    
	if (printwidth > 32)
	{
		printwidth = 32; /*print 8 pixels max - zero based */
	}
    //printf("Pixel bytes: ");
    for (int i=0;i<printwidth;i++)
	{
		if (((i % 4) == 0) & (i != 0))
		{
			//printf("-");
		}
		//printf("%02X",addr[i]);	
    }

    //printf("\n");
    //fflush(stdout);
#endif
}
