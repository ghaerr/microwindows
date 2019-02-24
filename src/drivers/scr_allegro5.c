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

#if ANDROID
#define ALLEGRO_UNSTABLE
#endif

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro.h>

#if ANDROID
#include <allegro5/allegro_android.h>
#endif

/* specific driver entry points*/
static PSD  allegro_open(PSD psd);
static void allegro_close(PSD psd);
static void allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void allegro_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);
static int  allegro_preselect(PSD psd);
static int allegro_pollevents(void);

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
 * The Allegro display is zoomed times the size of the requested screen size,
 * and then the regular sized screen bitmap is scaled up when copied to the display.
 * Using 2.0 zoom improves the readability of the display tremendously for Android and OSX Retina.
 */
#ifndef ALLEGRO_ZOOM
#define ALLEGRO_ZOOM	1.0			/* normally set in config file*/
#endif
float allegro_zoom = ALLEGRO_ZOOM;

ALLEGRO_EVENT_QUEUE *allegro_kbdqueue;
ALLEGRO_EVENT_QUEUE *allegro_mouqueue;
ALLEGRO_EVENT_QUEUE *allegro_scrqueue;
ALLEGRO_DISPLAY *allegro_display;
static ALLEGRO_BITMAP *scrmem;
static MWCOORD upminX, upminY, upmaxX, upmaxY;	/* aggregate update region bounding rect*/
static MWPALENTRY palette[256];

static int
init_allegro(void)
{
	if(!al_init())
	{
		EPRINTF("Error!, Allegro has failed to initialize.\n");
		return 0;
	}

	al_set_new_display_flags(ALLEGRO_WINDOWED);
	al_set_new_window_title("Microwindows Allegro");
	//al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
	//al_set_new_display_option(ALLEGRO_COLOR_SIZE,32,ALLEGRO_SUGGEST);
	//al_set_new_display_option(ALLEGRO_CAN_DRAW_INTO_BITMAP,1,ALLEGRO_REQUIRE);

	allegro_display = al_create_display(SCREEN_WIDTH*allegro_zoom, SCREEN_HEIGHT*allegro_zoom);
	scrmem = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);

	if(allegro_display == NULL)
	{
		EPRINTF("Error!, Failed to create the display.");
		return 0;
	}
	al_set_system_mouse_cursor(allegro_display,ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);

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
	al_register_event_source(allegro_scrqueue, al_get_display_event_source(allegro_display));

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

	al_hide_mouse_cursor(allegro_display);		/* turn off allegro cursor*/

	return 1;
}

/* return nonzero if event available*/
static int
allegro_pollevents(void)
{
	ALLEGRO_EVENT event;

	if (al_peek_next_event(allegro_mouqueue, &event))
		return 1;

	if (al_peek_next_event(allegro_kbdqueue, &event))
		return 1;

	if (al_peek_next_event(allegro_scrqueue, &event))
		return 1;

	return 0;
}

/*
 *	Open graphics
 */
static PSD
allegro_open(PSD psd)
{
	/* init psd and allocate framebuffer*/
	int flags = PSF_SCREEN | PSF_ADDRMALLOC | PSF_DELAYUPDATE | PSF_CANTBLOCK;

	if (!gen_initpsd(psd, MWPIXEL_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, flags))
		return NULL;

	/* init Allegro subsystem*/
	if (!init_allegro())
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
    al_destroy_display(allegro_display);
    al_destroy_event_queue(allegro_kbdqueue);
    al_destroy_event_queue(allegro_mouqueue);
    al_destroy_event_queue(allegro_scrqueue);
	free(psd->addr);		/* free framebuffer memory */
}

/* set palette*/
static void
allegro_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	int i;

	if (count > 256)
		count = 256;

	/* just save the palette for use in allegro_draw*/
	for (i=0; i < count; i++)
		palette[i] = *pal++;
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

#if MWPIXEL_FORMAT == MWPF_TRUECOLORARGB
	unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
	for (y = 0; y < height; y++) {
		for (x = 0; x < (width<<2); x += 4)
			al_draw_pixel(destx+(x>>2), desty+y,
				al_map_rgba(addr[x+2],addr[x+1],addr[x], 255));
		addr += psd->pitch;
	}
#elif MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
	unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
	for (y = 0; y < height; y++) {
		for (x = 0; x < (width<<2); x += 4)
			al_draw_pixel(destx+(x>>2), desty+y,
				al_map_rgba(addr[x],addr[x+1],addr[x+2], 255));
		addr += psd->pitch;
	}
#elif MWPIXEL_FORMAT == MWPF_PALETTE
	unsigned char *addr = psd->addr + desty * psd->pitch + destx;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			unsigned char c = addr[x];
			al_draw_pixel(destx+x, desty+y,
				al_map_rgba(palette[c].r, palette[c].g, palette[c].b, 255));
		}
		addr += psd->pitch;
	}
#endif
}

/* update allegro screen bitmap, returns # pending events*/
static int
allegro_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region to allegro lib*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX >= 0 || upmaxY >= 0)) {
		allegro_draw(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);

		/* reset update region*/
		upminX = upminY = MAX_MWCOORD;
		upmaxX = upmaxY = MIN_MWCOORD;
	}

	if(al_is_bitmap_locked(scrmem))
	{
		al_unlock_bitmap(scrmem);
		al_set_target_bitmap(al_get_backbuffer(allegro_display));
		al_draw_scaled_rotated_bitmap(scrmem, 0, 0, 0, 0, allegro_zoom, allegro_zoom, 0, 0);
		al_flip_display();

#if 0 // MACOSX test
		/* experimental fix for dual mouse cursor on OSX*/
		al_show_mouse_cursor(allegro_display);		/* turn on allegro cursor*/
		al_hide_mouse_cursor(allegro_display);		/* turn off allegro cursor*/
#endif
	}

	/* return nonzero if event available*/
	return allegro_pollevents();
}

/* framebuffer updated, calculate update rectangle*/
static void
allegro_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = MWMIN(x, upminX);
			upminY = MWMIN(y, upminY);
			upmaxX = MWMAX(upmaxX, x+width-1);
			upmaxY = MWMAX(upmaxY, y+height-1);
	} else
		allegro_draw(psd, x, y, width, height);
}
