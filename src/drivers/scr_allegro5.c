/*
 * Screen Driver for Allegro/Android
 *
 *  adapted to version 0.93 by Georg Potthast
 *
 */
#include <stdio.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro.h>

/* specific driver entry points*/
static PSD  allegro_open(PSD psd);
static void allegro_close(PSD psd);
static void allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void allegro_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

SUBDRIVER subdriver;

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	allegro_open,
	allegro_close,
	allegro_setpalette,       
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	allegro_update,		/* Update*/
	NULL				/* PreSelect*/
};

extern ALLEGRO_DISPLAY *display;
ALLEGRO_BITMAP *display_bitmap, *scrmem;
ALLEGRO_LOCKED_REGION *locked_region;
int lock_flags = ALLEGRO_LOCK_READWRITE;
int zoomfactor=1;

/*
 *	Open graphics
 */
static PSD
allegro_open(PSD psd)
{
	PSUBDRIVER subdriver;

	psd->pixtype = MWPIXEL_FORMAT;				/* SCREEN_PIXTYPE in config*/
	psd->xres = psd->xvirtres = SCREEN_WIDTH;	/* SCREEN_WIDTH in config*/
	psd->yres = psd->yvirtres = SCREEN_HEIGHT;	/* SCREEN_HEIGHT in config*/

	/* use pixel format to set bpp*/
	psd->pixtype = MWPIXEL_FORMAT;
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

#if MWPIXEL_FORMAT == MWPF_PALETTE
	case MWPF_PALETTE:
		psd->bpp = SCREEN_DEPTH;				/* SCREEN_DEPTH in config*/
		break;
#endif
	}
	psd->planes = 1;

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* Calculate the correct size and pitch from xres, yres and bpp*/
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp, &psd->size, &psd->pitch);

	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);
	psd->flags = PSF_SCREEN;
	psd->portrait = MWPORTRAIT_NONE;

	/* select an fb subdriver matching our planes and bpp for backing store*/
	subdriver = select_fb_subdriver(psd);
	psd->orgsubdriver = subdriver;
	if (!subdriver)
		return NULL;

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

	/* allocate framebuffer*/
	if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;
	psd->flags |= PSF_ADDRMALLOC;

#if ANDROID
	scrmem = al_create_bitmap(al_get_bitmap_width(al_get_backbuffer(display)),
		al_get_bitmap_height(al_get_backbuffer(display)));
	al_set_target_bitmap(scrmem);
	al_clear_to_color(al_map_rgb_f(0x0, 0, 0)); //black background
	//calc zoom factor being a multiple of 0.2 (f.e. 6) for nice looking zoom - assume letter orientation of Android device
	zoomfactor=al_get_bitmap_width(al_get_backbuffer(display))/SCREEN_WIDTH; //2575/640 =4
#endif

	return psd;
}

/*
 *	Close graphics
 */
static void
allegro_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);
}

/*
 *	Set Palette
 */
static void
allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

/*
 *	Update Allegro screen
 */
static void
allegro_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	MWCOORD x,y;
	MWPIXELVAL c;

	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

#if 0000 // non of these are implemented and will be fixed in the driver rewrite
	if (psd->pixtype == MWPF_TRUECOLOR332)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				c = addr[x];
				//GrPlot(destx+x, desty+y, c); 
				////_putpixel(screen,destx+x,desty+y,(int)c);
			}
			addr += psd->pitch;
		}
	}
	else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	
	        unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*2; x++) {
				MWPIXELVAL c = ((unsigned short *)addr)[x]; 
				//GrPlot(destx+x, desty+y, c); 
				////_putpixel16(screen,destx+x,desty+y,(int)c);
			}
			addr += psd->pitch;
		}
	}
	else if (psd->pixtype == MWPF_TRUECOLORRGB)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		unsigned int extra = psd->pitch - width * 3;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*3; x++) {
				MWPIXELVAL c = RGB2PIXEL888(addr[2], addr[1], addr[0]);
				//GrPlot(destx+x, desty+y, c);
				////_putpixel24(screen,destx+x,desty+y,(int)c);
				addr += 3;
			}
			addr += extra;
		}
	}
	else if ((MWPIXEL_FORMAT == MWPF_TRUECOLORARGB) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR))
#endif


	{
#if ANDROID
	if(!al_is_bitmap_locked(scrmem))
		al_lock_bitmap(scrmem, ALLEGRO_PIXEL_FORMAT_ANY, 0);
	al_set_target_bitmap(scrmem);
#else
	display_bitmap = al_get_backbuffer(display);
	al_set_target_bitmap(display_bitmap);
	al_set_target_backbuffer(display);

	if(!al_is_bitmap_locked(display_bitmap))
		locked_region = al_lock_bitmap(display_bitmap, ALLEGRO_PIXEL_FORMAT_RGBA_8888, lock_flags);
	//if(!al_is_bitmap_locked(display_bitmap))
	//	locked_region = al_lock_bitmap(display_bitmap, ALLEGRO_PIXEL_FORMAT_ANY, lock_flags);
#endif

/*
psd->addr = fixed pointer to start of entire internal microwindows pixel buffer
destx = logical pixel start horizontal axsis, desty = logical pixel start vertical axsis
width = logical line width, height = number of lines
addr = physical start of pixel block to render passed by microwindows
addr += psd->pitch = physical start of next line of pixel block to render
width*4 (=width <<2) = physical line length of pixel block to render
*/  
	    unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x = x+4) { 			
				//do not read outside the screen buffer memory area or crash
				if ((addr+y+x*4)>(psd->addr+psd->size)) return;
				//the android display is created as default by Allegro as 565 (16 bit)
				//therefore al_draw_pixel to do conversion from 32bit
				 al_draw_pixel(destx+(x/4),desty+y,
				 	al_map_rgb((unsigned char)addr[x+2],(unsigned char)addr[x+1],(unsigned char)addr[x]));				
			}
			addr += psd->pitch;  
		}
	}
}
