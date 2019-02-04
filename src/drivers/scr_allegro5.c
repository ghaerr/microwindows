/*
 * Screen Driver for Allegro/Android
 *	#define ANDROID=1 for Android support on Allegro
 *
 * Adapted to version 0.93 by Georg Potthast
 * Updated to new driver format Greg Haerr 2019
 */
#include <stdio.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro.h>

#if ANDROID
#include <allegro5/allegro_android.h>
#endif

#if MWPIXEL_FORMAT != MWPF_TRUECOLORARGB
#error Allegro driver only supports SCREEN_PIXTYPE=MWPF_TRUECOLORARGB
#endif

/* specific driver entry points*/
static PSD  allegro_open(PSD psd);
static void allegro_close(PSD psd);
static void allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void allegro_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);
static int  allegro_preselect(PSD psd);

#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define max(a,b)            (((a) > (b)) ? (a) : (b))

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
	allegro_update,
	allegro_preselect
};

/*
 * The Allegro display is created 2 times the size of the requested screen size,
 * and then the regular sized screen bitmap is scaled up when copied to the display.
 * This improves the readability of the display tremendously.
 */
float allegro_zoom = 2.0;

ALLEGRO_EVENT_QUEUE *allegro_kbdqueue;
ALLEGRO_EVENT_QUEUE *allegro_mouqueue;
ALLEGRO_EVENT_QUEUE *allegro_scrqueue;
static ALLEGRO_DISPLAY *display;
static ALLEGRO_BITMAP *scrmem;
static MWCOORD upminX, upminY, upmaxX, upmaxY;	/* aggregate update region bounding rect*/

static int
init_allegro(void)
{
	if(!al_init())
	{
		EPRINTF("Error!, Allegro has failed to initialize.\n");
		return 0;
	}

	al_set_new_display_flags(ALLEGRO_WINDOWED);
	//al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
	//al_set_new_display_option(ALLEGRO_COLOR_SIZE,32,ALLEGRO_SUGGEST);
	//al_set_new_display_option(ALLEGRO_CAN_DRAW_INTO_BITMAP,1,ALLEGRO_REQUIRE);

	display = al_create_display(SCREEN_WIDTH*allegro_zoom, SCREEN_HEIGHT*allegro_zoom);
	scrmem = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);

	if(display == NULL)
	{
		EPRINTF("Error!, Failed to create the display.");
		return 0;
	}
	al_set_system_mouse_cursor(display,ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);

	if(!al_install_keyboard())
	{
		EPRINTF("Error!, Failed to install keyboard.\n");
		return 0;
	}

	if(!al_install_mouse())
	{
		EPRINTF("Error!, Failed to install mouse.");
		return 0;
	}

	allegro_kbdqueue = al_create_event_queue();
	if(allegro_kbdqueue == NULL)
	{
		EPRINTF("Error!, Failed to create the keyboard event queue.");
		return 0;
	}
	al_register_event_source(allegro_kbdqueue, al_get_keyboard_event_source());

	allegro_mouqueue = al_create_event_queue();
	if(allegro_mouqueue == NULL)
	{
		EPRINTF("Error!, Failed to create the mouse event queue.");
		return 0;
	}
#if !ANDROID
	/* android mouse input handled below*/
	al_register_event_source(allegro_mouqueue, al_get_mouse_event_source());
#endif

	allegro_scrqueue = al_create_event_queue();
	if(allegro_scrqueue == NULL)
	{
		EPRINTF("Error!, Failed to create the display event queue.");
		return 0;
	}
	al_register_event_source(allegro_scrqueue, al_get_display_event_source(display));

#if ANDROID
	if(!al_install_touch_input())
	{
		EPRINTF("Error!, Failed to install touch_input.");
	}
	else
	{
		al_set_mouse_emulation_mode(ALLEGRO_MOUSE_EMULATION_5_0_x);
		al_register_event_source(allegro_mouqueue,
			al_get_touch_input_mouse_emulation_event_source());
	}

	/* allow using al_open to read (only) files from the apk store, e.g. fonts*/
	al_android_set_apk_file_interface();
#endif

	al_hide_mouse_cursor(display);		/* turn off allegro cursor*/

	return 1;
}

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

	case MWPF_PALETTE:
		psd->bpp = SCREEN_DEPTH;				/* SCREEN_DEPTH in config*/
		break;
	}
	psd->planes = 1;

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* Calculate the correct size and pitch from xres, yres and bpp*/
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp, &psd->size, &psd->pitch);

	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_ADDRMALLOC | PSF_DELAYUPDATE | PSF_CANTBLOCK;
	psd->portrait = MWPORTRAIT_NONE;

	/* select an fb subdriver matching our planes and bpp for backing store*/
	subdriver = select_fb_subdriver(psd);
	psd->orgsubdriver = subdriver;
	if (!subdriver)
		return NULL;

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

	if (!init_allegro())
		return NULL;

	/* allocate framebuffer*/
	if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;

	al_rest(0.2);

	return psd;
}

/*
 *	Close graphics
 */
static void
allegro_close(PSD psd)
{
    al_destroy_display(display);
    al_destroy_event_queue(allegro_kbdqueue);
    al_destroy_event_queue(allegro_mouqueue);
    al_destroy_event_queue(allegro_scrqueue);
	free(psd->addr);		/* free framebuffer memory */
}

/* palette mode not supported*/
static void
allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

/* copy from Microwindows framebuffer to Allegro scrmem bitmap*/
static void
allegro_draw(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	MWCOORD x,y;

	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

	if(!al_is_bitmap_locked(scrmem))
		al_lock_bitmap(scrmem, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
	al_set_target_bitmap(scrmem);

	/* only MWPF_TRUECOLORARGB supported*/
	unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
	for (y = 0; y < height; y++) {
		for (x = 0; x < (width<<2); x += 4)
			al_draw_pixel(destx+(x>>2),desty+y, al_map_rgb(addr[x+2],addr[x+1],addr[x]));
		addr += psd->pitch;
	}
}

/* update allegro screen bitmap, returns # pending events*/
static int
allegro_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region to allegro lib*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX || upmaxY)) {
		allegro_draw(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);

		/* reset update region*/
		upminX = upminY = ~(1 << ((sizeof(int)*8)-1));	// largest positive int
		upmaxX = upmaxY = 0;
	}

	if(al_is_bitmap_locked(scrmem))
	{
		al_unlock_bitmap(scrmem);
		al_set_target_bitmap(al_get_backbuffer(display));
		al_draw_scaled_rotated_bitmap(scrmem, 0, 0, 0, 0, allegro_zoom, allegro_zoom, 0, 0);
		al_flip_display();
	}

	/* return nonzero if event available*/
	return 1;
}

/* framebuffer updated, calculate update rectangle*/
static void
allegro_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = min(x, upminX);
			upminY = min(y, upminY);
			upmaxX = max(upmaxX, x+width-1);
			upmaxY = max(upmaxY, y+height-1);
	} else
		allegro_draw(psd, x, y, width, height);
}
