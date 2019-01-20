/*
 *
 *  Screen NULL driver by Georg Potthast 2018
 * 
 *  For testing the Makefile for a new platform before writing a special screen driver
 * 
 *  will output to stdout when pixels for the screen are received, no graphics screen
 *
 */
#include <stdio.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

static PSD  NUL_open(PSD psd);
static void NUL_close(PSD psd);
static void NUL_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void NUL_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void NUL_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 768
#endif

#ifndef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE MWPF_TRUECOLOR8888
#endif

SUBDRIVER subdriver;

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	NUL_open,
	NUL_close,
	NUL_setpalette,       
	NUL_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,  		/*gen_setportrait */
	NUL_update,	/* Update*/
	NULL		/* PreSelect*/
};

/*
**	Open graphics
*/
static PSD
NUL_open(PSD psd)
{
	PSUBDRIVER subdriver;
	int avwidth,avheight,avbpp;
	
	avwidth = SCREEN_WIDTH;
	avheight = SCREEN_HEIGHT;
	if(SCREEN_PIXTYPE == MWPF_TRUECOLOR8888) {
		avbpp=32;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR888) {
		avbpp=24;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR565)  {
		avbpp=16;
	} else {
		avbpp=8; //palette
	}

	psd->xres = psd->xvirtres = avwidth; //GrScreenX();
	psd->yres = psd->yvirtres = avheight; //GrScreenY();
	psd->planes = 1;
	psd->bpp = avbpp; //md_info->bpp;
	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_ADDRMALLOC;
	/* Calculate the correct size and linelen here */
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp,
		&psd->size, &psd->pitch);

    if(psd->bpp == 32) {
		psd->pixtype = MWPF_TRUECOLOR8888;	
	} else if(psd->bpp == 16) {
		psd->pixtype = MWPF_TRUECOLOR565; 
	} else if(psd->bpp == 24)  {
		psd->pixtype = MWPF_TRUECOLOR888;
	} else {
		psd->pixtype = MWPF_PALETTE;
	}
		  
  psd->portrait = MWPORTRAIT_NONE;
  psd->data_format = set_data_format(psd);

  /*
   * set and initialize subdriver into screen driver
   * psd->size is calculated by subdriver init
   */
  subdriver = select_fb_subdriver(psd);
  
  psd->orgsubdriver = subdriver;

  set_subdriver(psd, subdriver);
  if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;
  return psd;
}

/*
**	Close graphics
*/
static void
NUL_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);
}

/*
**	Get Screen Info
*/
static void
NUL_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);

	psi->fbdriver = FALSE;	/* not running fb driver, no direct map */

	if(scrdev.yvirtres > 600) {
		/* SVGA 1024x768*/
		psi->xdpcm = 42;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 42;	/* assumes screen height of 18 cm*/        
	} else if(scrdev.yvirtres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(scrdev.yvirtres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}

/*
**	Set Palette
*/
static void
NUL_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

/*
**	Blit
*/
static void
NUL_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
    static uint32_t counter;
    int printwidth; int i;
    unsigned char *addr; 
    
    if (psd->pixtype == MWPF_TRUECOLOR332) {
		addr = psd->addr + desty * psd->pitch + destx;}
    else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555)) {	
		addr = psd->addr + desty * psd->pitch + (destx << 1);}
    else if (psd->pixtype == MWPF_TRUECOLOR888) {
		addr = psd->addr + desty * psd->pitch + destx * 3; }	
    else if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR))) {
		addr = psd->addr + desty * psd->pitch + (destx << 2);
    }
				
    /* print data to stdout */
    counter++; //count calls to NUL_update()
    fprintf(stdout,"Receiving pixels -  call #:%u, xpos:%d, ypos:%d, width:%d, height:%d, ",counter,destx,desty,width,height); 
    if (counter > 4000000000) { counter=0; } /* wrap around */
    printwidth=width*4; /* four bytes per pixel max */    
    if (printwidth > 32) printwidth = 32; /*print 8 pixels max - zero based */
    printf("Pixel bytes: ");
    for (i=0;i<printwidth;i++){
	if ( ((i % 4) == 0) & (i != 0) ) printf("-");
	printf("%02X",addr[i]);	
    }
    printf("\n");
    fflush(stdout);
}
