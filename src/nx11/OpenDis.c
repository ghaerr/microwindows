#include "nxlib.h"
#include <stdlib.h>

Font _nxCursorFont = None;	/* global because no dpy->cursor_font*/

/* required globals for emulation*/
int _Xdebug = 0;

/* dummy lock stuff*/
int (*_XLockMutex_fn)();
int (*_XUnlockMutex_fn)();
int *_Xglobal_lock;

/* statics for this file*/
static char static_display_name[] = ":0";
static Display *OutOfMemory(Display *dpy);

/* 
 * Connects to a server, Creates a Display object.
 */
Display *
XOpenDisplay(_Xconst char *display)
{
	Display *dpy;
	static int fd = -1;
	GR_SCREEN_INFO sinfo;

/*
 * Connect with Nano-X server and allocate a display structure.
 */
	if (fd < 0 && (fd = GrOpen()) < 0) {
		EPRINTF("nxlib: can't connect to nano-X server\r\n");
		return NULL;
	}

	GrGetScreenInfo(&sinfo);
	if (sinfo.bpp < 8) {
		EPRINTF("nxlib: Unsupported bpp: %d\n", sinfo.bpp);
		GrClose();
		return NULL;
	}

	if ((dpy = (Display *)Xcalloc(1, sizeof(Display))) == NULL)
		return OutOfMemory(dpy);

	/* Initialize as much of the display structure as we can.*/
	dpy->fd			= fd;
	dpy->display_name	= static_display_name;
	dpy->proto_major_version= 11;
	dpy->proto_minor_version= 6;
	dpy->release 		= 0;
	dpy->min_keycode	= 0;
	dpy->max_keycode	= 255;	/* FIXME?*/
	dpy->motion_buffer	= 0;
	dpy->nformats		= 1;
	dpy->nscreens		= 1;
	dpy->byte_order		= LSBFirst;
	dpy->bitmap_unit	= 16;	/* FIXME?*/
	dpy->bitmap_pad		= 32;	/* FIXME?*/
	dpy->bitmap_bit_order   = MSBFirst;
	dpy->max_request_size	= 65532;
	dpy->default_screen	= 0;
	dpy->vendor = "CSET";	/* null terminated, padded to 4 bytes*/
	dpy->qlen		= 0;	/* FIXME, required for QLength macro*/
	dpy->request		= 0;	/* FIXME, required by NextRequest*/
	dpy->last_request_read	= 0;	/* FIXME, required by LastKnownRequestProcessed*/

#if 0 /* Xlib private data, not used in nx11lib*/
	dpy->lock_meaning	= NoSymbol;
	dpy->event_vec[X_Error] = _XUnknownWireEvent;
	dpy->event_vec[X_Reply] = _XUnknownWireEvent;
	dpy->wire_vec[X_Error]  = _XUnknownNativeEvent;
	dpy->wire_vec[X_Reply]  = _XUnknownNativeEvent;
	for (i = KeyPress; i < LASTEvent; i++) {
	    dpy->event_vec[i] 	= _XWireToEvent;
	    dpy->wire_vec[i] 	= NULL;
	}
	for (i = LASTEvent; i < 128; i++) {
	    dpy->event_vec[i] 	= _XUnknownWireEvent;
	    dpy->wire_vec[i] 	= _XUnknownNativeEvent;
	}
	dpy->next_event_serial_num = 1;
	dpy->vnumber = X_PROTOCOL;
	dpy->resource_alloc = _XAllocID;
	dpy->idlist_alloc = _XAllocIDs;
	dpy->last_req = (char *)&_dummy_request;

	endian = 1;
	if (*(char *) &endian)
	    client.byteOrder = '\154'; /* 'l' */
	else
	    client.byteOrder = '\102'; /* 'B' */

	dpy->resource_base	= 0;
	dpy->resource_mask	= 0;

	mask = dpy->resource_mask;
	dpy->resource_shift	= 0;
	while (!(mask & 1)) {
	    dpy->resource_shift++;
	    mask = mask >> 1;
	}
	dpy->resource_max = (dpy->resource_mask >> dpy->resource_shift) - 5;
#endif


/*
 * Z axis Screen format information.
 */
	dpy->pixmap_format = (ScreenFormat *)Xcalloc(1, sizeof(ScreenFormat));
	if (dpy->pixmap_format == NULL)
	        return OutOfMemory (dpy);
	{
	    ScreenFormat *fmt = &dpy->pixmap_format[0];
	    fmt->depth = sinfo.bpp;
	    fmt->bits_per_pixel = sinfo.bpp;
	    fmt->scanline_pad = 32;	/* FIXME?*/
	    //fmt->ext_data = NULL;
	}

/*
 * allocate the Screen structure.
 */
	dpy->screens = (Screen *)Xcalloc(1, sizeof(Screen));
	if (dpy->screens == NULL)
	        return OutOfMemory (dpy);
	{
	    Screen *sp = &dpy->screens[0];

	    sp->display	    = dpy;
	    sp->root 	    = GR_ROOT_WINDOW_ID;
	    sp->width	    = sinfo.cols;
	    sp->height	    = sinfo.rows;
	    sp->white_pixel = 0x00ffffff;
	    sp->black_pixel = 0x00000000;
/* guess at dots per inch...*/
#define DPIX	75
#define DPIY	75
	    sp->mwidth      = (sp->width * 254 + DPIX * 5) / (DPIX * 10);
	    sp->mheight     = (sp->height * 254  + DPIY * 5) / (DPIY * 10);
	    sp->min_maps    = 1;
	    sp->max_maps    = 1;

	    //sp->root_input_mask = u.rp->currentInputMask;
	    //sp->backing_store= u.rp->backingStore;
	    //sp->save_unders = u.rp->saveUnders;
	    //sp->ext_data   = NULL;
		    
/*
 * allocate the depth structure.
 */
	    sp->ndepths	= 1;
	    sp->depths = (Depth *)Xmalloc(sizeof(Depth));
	    if (sp->depths == NULL)
		return OutOfMemory (dpy);
	    {
		Depth *dp = &sp->depths[0];
		dp->depth = sinfo.bpp;
		dp->nvisuals = 1;
		dp->visuals = (Visual *)Xmalloc(sizeof(Visual));
		if (dp->visuals == NULL)
			return OutOfMemory (dpy);
		{
		    Visual *vp = &dp->visuals[0];
		    vp->visualid	= 1;	
		    vp->class		= TrueColor;	//FIXME
		    vp->map_entries	= 256;		//FIXME
		    vp->red_mask	= sinfo.rmask;
		    vp->green_mask	= sinfo.gmask;
		    vp->blue_mask	= sinfo.bmask;
		    vp->ext_data	= NULL;

		    /* 
		     * set bits_per_rgb:
		     * = 8 for 8bpp palette and truecolor
		     * = green (5 or 6) for 16bpp
		     * = 8 for 24 & 32 bpp
		     */
		    if (sinfo.bpp == 16)
				vp->bits_per_rgb = (sinfo.pixtype == MWPF_TRUECOLOR555)? 5: 6;
		    else
				vp->bits_per_rgb = 8; 	/* 8, 24, 32*/
		}
	    }
	    sp->root_depth  = sp->depths[0].depth;
	    sp->root_visual = &sp->depths[0].visuals[0];
	    DPRINTF("nxlib: display bpp %d, bits_per_rgb %d\n",
	    	sp->root_depth, sp->root_visual->bits_per_rgb);

	    /* can't call next function until default visual in place*/
	    sp->cmap = _nxDefaultColormap(dpy);
	}

/*
 * Set up other stuff clients are always going to use.
 */
	{
	    Screen *sp = &dpy->screens[0];
	    XGCValues values;

	    values.foreground = sp->black_pixel;
	    values.background = sp->white_pixel;
	    if ((sp->default_gc = XCreateGC (dpy, sp->root,
		    GCForeground|GCBackground, &values)) == NULL) {
			return OutOfMemory(dpy);
	    }
	}
 	return dpy;
}

/* XFreeDisplayStructure frees all the storage associated with a 
 * Display.  It is used by XOpenDisplay if it runs out of memory,
 * and also by XCloseDisplay.   It needs to check whether all pointers
 * are non-NULL before dereferencing them, since it may be called
 * by XOpenDisplay before the Display structure is fully formed.
 * XOpenDisplay must be sure to initialize all the pointers to NULL
 * before the first possible call on this.
 */
void
_XFreeDisplayStructure(Display * dpy)
{
	int i, j;

	if (!dpy)
		return;
	if (dpy->screens) {
		for (i = 0; i < dpy->nscreens; i++) {
			Screen *sp = &dpy->screens[i];
			if (sp->depths) {
				for (j = 0; j < sp->ndepths; j++) {
					Depth *dp = &sp->depths[j];
					if (dp->visuals)
						Xfree((char *) dp->visuals);
				}
				Xfree((char *) sp->depths);
			}
		}
		Xfree((char *) dpy->screens);
	}
	if (dpy->pixmap_format)
		Xfree((char *) dpy->pixmap_format);

#if 0 /* Xlib private data, not used by nx11lib */
	if (dpy->keysyms)
		Xfree((char *) dpy->keysyms);
	if (dpy->xdefaults)
		Xfree(dpy->xdefaults);
	if (dpy->error_vec)
		Xfree((char *) dpy->error_vec);
	if (dpy->free_funcs)
		Xfree((char *) dpy->free_funcs);
#endif

	Xfree((char *) dpy);
}

/* Called if mem alloc fails.  XOpenDisplay returns NULL if this happens*/
static Display *
OutOfMemory(Display * dpy)
{
	GrClose();
	_XFreeDisplayStructure(dpy);
	return NULL;
}

Colormap
XDefaultColormap(Display * display, int screen)
{
	if (screen > 1)
		return 0;
	return (display->screens[0].cmap);
}

Visual *
XDefaultVisual(Display * display, int screen)
{
	if (screen > 1)
		return 0;
	return (display->screens[0].root_visual);
}

int
XDefaultDepth(Display * display, int screen)
{
	if (screen > 1)
		return 0;
	return (display->screens[0].root_depth);
}

Window
XDefaultRootWindow(Display *display)
{
	return RootWindow(display, DefaultScreen(display));
}

int
XDefaultScreen(Display * display)
{
	return 0;
}

Screen *
XDefaultScreenOfDisplay(Display * display)
{
	return &display->screens[0];
}

Screen *
XScreenOfDisplay(Display * display, int screen)
{
	return &display->screens[screen];
}

int
XScreenNumberOfScreen(Screen *scr)
{
	return 0;
}

Window
XRootWindow(Display * display, int screen)
{
	return GR_ROOT_WINDOW_ID;
}

Window
XRootWindowOfScreen(Screen *scr)
{
	return GR_ROOT_WINDOW_ID;
}

char *
XDisplayName(_Xconst char *string)
{
	return static_display_name;
}

VisualID
XVisualIDFromVisual(Visual *vp)
{
	return vp->visualid;
}

Colormap
XDefaultColormapOfScreen(Screen *scr)
{
	return scr->cmap;
}

int
XConnectionNumber(Display *display)
{
	return display->fd;
}

char *
XServerVendor(Display *display)
{
	return ServerVendor(display);
}

unsigned long
XBlackPixel(Display * display, int screen_number)
{
	return display->screens[screen_number].black_pixel;
}

unsigned long
XWhitePixel(Display * display, int screen_number)
{
	return display->screens[screen_number].white_pixel;
}
