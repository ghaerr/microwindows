/*
 *
 * Screen Driver using DJGPP & VESA  Library by Georg Potthast
 *
 */

#include <stdio.h>
#include "device.h"  
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

#include "djvesa.h"

static PSD  vesa_open(PSD psd);
static void vesa_close(PSD psd);
static void vesa_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void vesa_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void vesa_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 800
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 600
#endif

#ifndef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE MWPF_TRUECOLOR565 //16
#endif

SUBDRIVER subdriver;

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	vesa_open,
	vesa_close,
	vesa_setpalette,       
	vesa_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,               /*gen_setportrait*/
	vesa_update,		/* Update*/
	NULL				/* PreSelect*/
};

/*
**	Open graphics
*/
static PSD
vesa_open(PSD psd)
{
	PSUBDRIVER subdriver;

	int vwidth,vheight,vbpp;
    int vbemode;
   
	//vwidth = 1024; //SCREEN_WIDTH;
	//vheight = 786; //SCREEN_HEIGHT;
    //vbpp=32;

	vwidth = SCREEN_WIDTH;
	vheight = SCREEN_HEIGHT;
	if(SCREEN_PIXTYPE == MWPF_TRUECOLOR8888) {
		vbpp=32;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR888) {
		vbpp=24;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR565)  {
		vbpp=16;
	} else {
		vbpp=8; //palette
	}

   //find VESA mode for these parameters
   vbemode = set_vesa_mode(vwidth,vheight,vbpp);
   if (vbemode == -1) {return NULL;} //no valid mode found

   //read in the mode_info structure for the selected VESA mode
   get_mode_info(vbemode);

   psd->xres = psd->xvirtres = mode_info.XResolution;
	psd->yres = psd->yvirtres = mode_info.YResolution;
	psd->planes = 1;
	psd->bpp = mode_info.BitsPerPixel;
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
vesa_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);
	done_VBE_mode();
}

/*
**	Get Screen Info
*/
static void
vesa_getscreeninfo(PSD psd,PMWSCREENINFO psi)
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
vesa_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{ /* this function is untested */ 
	set_vpalette_entry(first,count, &pal[0]);  
}
/*
**	Blit
*/
static void
vesa_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

/*
typedef unsigned char *		ADDR8;
typedef unsigned short *	ADDR16;
typedef uint32_t *			ADDR32;
*/

	MWCOORD x,y;
	MWPIXELVAL c;

/* got to read from psd->addr and write with putpixel()*/

	//if (!((width == 1) || (height == 1))) return;
	//printf("U: %d %d %d %d ",destx,desty,width,height);

if (psd->pixtype == MWPF_TRUECOLOR332)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
        //use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				c = addr[x];
				putpixel(destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		}
	}
else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*2; x++) {
				MWPIXELVAL c = ((unsigned short *)addr)[x]; //((ADDR16)addr)[x];
				putpixel(destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		}
	}
else if (psd->pixtype == MWPF_TRUECOLOR888)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		unsigned int extra = psd->pitch - width * 3;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*3; x++) {
				MWPIXELVAL c = RGB2PIXEL888(addr[2], addr[1], addr[0]);
				putpixel(destx+x, desty+y, c); 
				addr += 3;
			}
			addr += extra; 
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		}
	}
else if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)) & (psd->bpp != 8))
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx <<2);
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x++) {				
				MWPIXELVAL c = ((unsigned long *)addr)[x]; //MWPIXELVAL=uint32_t
				putpixel(destx+x, desty+y, c);
			}
			addr += psd->pitch;     
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		} 
	} 
else /* MWPF_PALETTE*/ 
	{ 
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		//printf (" psd-addr %d addr %d desty:%d destx:%d width:%d height:%d\n",(int)psd->addr,(int)addr,desty,destx,width,height);
		
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				MWPIXELVAL c = addr[x];
				putpixel(destx+x, desty+y, c);
			}
			addr += psd->pitch;
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		}
	}

}

