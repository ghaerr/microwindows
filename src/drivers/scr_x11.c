/*
 * Copyright (c) 2000, 2001, 2003, 2010, 2017, 2019 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 Koninklijke Philips Electronics
 * Copyright (c) 1999 Tony Rogvall <tony@bluetail.com>
 * 	Rewritten to avoid multiple function calls by Greg Haerr
 *      Alpha blending added by Erik Hill
 *      (brought to life in a dark laboratory on a stormy night
 *      in an ancient castle by Dr. Frankenstein)
 *
 * X11 screen driver for Microwindows
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
//#include <X11/extensions/xf86dga.h>
#include <assert.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"
#include "osdep.h"

#if !defined(SCREEN_DEPTH) && (MWPIXEL_FORMAT == MWPF_PALETTE)
/* SCREEN_DEPTH is used only for palette modes*/
#error SCREEN_DEPTH not defined - must be set for palette modes
#endif

/* externally set override values from nanox/srvmain.c*/
MWCOORD	nxres;			/* requested server x res*/
MWCOORD	nyres;			/* requested server y res*/

/* specific x11 driver entry points*/
static PSD X11_open(PSD psd);
static void X11_close(PSD psd);
static void X11_getscreeninfo(PSD psd, PMWSCREENINFO psi);
static void X11_setpalette(PSD psd, int first, int count, MWPALENTRY * pal);
static int  X11_preselect(PSD psd);
static void X11_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	X11_open,
	X11_close,
	X11_setpalette,
	X11_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	X11_update,
	X11_preselect,
};

/* called from keyboard/mouse/screen */
Display *x11_dpy;
int x11_scr;
Visual *x11_vis;
Colormap x11_colormap;
Window x11_win;
GC x11_gc;
unsigned int x11_event_mask;

static int x11_width, x11_height;
static int x11_depth;		/* Screen depth in bpp */
static int x11_is_palette;	/* Nonzero for palette, zero for true color */

/* For a truecolor X11 system, the following constants define the colors */
static int x11_r_shift;
static unsigned long x11_r_mask;
static int x11_g_shift;
static unsigned long x11_g_mask;
static int x11_b_shift;
static unsigned long x11_b_mask;

static int x11_colormap_installed = 0;

/* color palette for color indexe */
static XColor x11_palette[256];
static int x11_pal_max = 0;

static MWCOORD upminX, upminY, upmaxX, upmaxY;
/* called from mou_x11.c*/
void x11_handle_event(XEvent * ev);
int x11_setup_display(void);

#if MWPIXEL_FORMAT != MWPF_PALETTE
/* Color cache for true color lookups */
#define COLOR_CACHE_SIZE 1001
struct color_cache {
	int init;		/* 1 if first use */
	unsigned short r;
	unsigned short g;
	unsigned short b;
	XColor c;
};
static struct color_cache ccache[COLOR_CACHE_SIZE];

static unsigned long
lookup_color(unsigned short r, unsigned short g, unsigned short b)
{
	int ix = ((r << 16) + (g << 8) + b) % COLOR_CACHE_SIZE;

	if (ccache[ix].init || (ccache[ix].r != r) || (ccache[ix].g != g)
	    || (ccache[ix].b != b)) {
		char spec[20];
		XColor exact;
		XColor def;

		ccache[ix].init = 0;
		ccache[ix].r = r;
		ccache[ix].g = g;
		ccache[ix].b = b;
		sprintf(spec, "#%02x%02x%02x", r, g, b);
		XLookupColor(x11_dpy, x11_colormap, spec, &exact, &def);
		XAllocColor(x11_dpy, x11_colormap, &def);
		ccache[ix].c = def;

		/* DPRINTF("lookup: exact(%d,%d,%d) = %d, def(%d,%d,%d)=%d\n",
		   exact.red>>8, exact.green>>8, exact.blue>>8, exact.pixel,
		   def.red>>8, def.green>>8, def.blue>>8, def.pixel); */
	}
	return (unsigned long) ccache[ix].c.pixel;
}
#endif /* MWPIXEL_FORMAT != MWPF_PALETTE */


#if MWPIXEL_FORMAT != MWPF_PALETTE

#if (MWPIXEL_FORMAT == MWPF_TRUECOLORRGB) || (MWPIXEL_FORMAT == MWPF_TRUECOLORARGB)
#define PIXEL2RED8(p)           PIXEL888RED8(p)
#define PIXEL2GREEN8(p)         PIXEL888GREEN8(p)
#define PIXEL2BLUE8(p)          PIXEL888BLUE8(p)
#define PIXEL2RED32(p)          PIXEL888RED32(p)
#define PIXEL2GREEN32(p)        PIXEL888GREEN32(p)
#define PIXEL2BLUE32(p)         PIXEL888BLUE32(p)
#elif MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
#define PIXEL2RED8(p)           PIXELABGRRED8(p)
#define PIXEL2GREEN8(p)         PIXELABGRGREEN8(p)
#define PIXEL2BLUE8(p)          PIXELABGRBLUE8(p)
#define PIXEL2RED32(p)          PIXELABGRRED32(p)
#define PIXEL2GREEN32(p)        PIXELABGRGREEN32(p)
#define PIXEL2BLUE32(p)         PIXELABGRBLUE32(p)
#elif MWPIXEL_FORMAT == MWPF_TRUECOLOR565
#define PIXEL2RED8(p)           PIXEL565RED8(p)
#define PIXEL2GREEN8(p)         PIXEL565GREEN8(p)
#define PIXEL2BLUE8(p)          PIXEL565BLUE8(p)
#define PIXEL2RED32(p)          PIXEL565RED32(p)
#define PIXEL2GREEN32(p)        PIXEL565GREEN32(p)
#define PIXEL2BLUE32(p)         PIXEL565BLUE32(p)
#elif MWPIXEL_FORMAT == MWPF_TRUECOLOR555
#define PIXEL2RED8(p)           PIXEL555RED8(p)
#define PIXEL2GREEN8(p)         PIXEL555GREEN8(p)
#define PIXEL2BLUE8(p)          PIXEL555BLUE8(p)
#define PIXEL2RED32(p)          PIXEL555RED32(p)
#define PIXEL2GREEN32(p)        PIXEL555GREEN32(p)
#define PIXEL2BLUE32(p)         PIXEL555BLUE32(p)
#elif MWPIXEL_FORMAT == MWPF_TRUECOLOR332
#define PIXEL2RED8(p)           PIXEL332RED8(p)
#define PIXEL2GREEN8(p)         PIXEL332GREEN8(p)
#define PIXEL2BLUE8(p)          PIXEL332BLUE8(p)
#define PIXEL2RED32(p)          PIXEL332RED32(p)
#define PIXEL2GREEN32(p)        PIXEL332GREEN32(p)
#define PIXEL2BLUE32(p)         PIXEL332BLUE32(p)
#endif

/* calc truecolor conversions directly */
#define PIXELVAL_to_pixel(c) (x11_is_palette                                \
            ? lookup_color(PIXEL2RED8(c), PIXEL2GREEN8(c), PIXEL2BLUE8(c))  \
            : ( ((PIXEL2RED32(c)   >> x11_r_shift) & x11_r_mask)            \
              | ((PIXEL2GREEN32(c) >> x11_g_shift) & x11_g_mask)            \
              | ((PIXEL2BLUE32(c)  >> x11_b_shift) & x11_b_mask) ) )

#else /* MWPIXEL_FORMAT == MWPF_PALETTE */
#define PIXELVAL_to_pixel(c) ((unsigned)(c) > (unsigned)x11_pal_max ? 0 : x11_palette[c].pixel)
#endif

/* called from mou_x11 (handles x11_event_mask events) */
void
x11_handle_event(XEvent * ev)
{
	static int inited = 0;

	if (ev->type == ColormapNotify) {
		if (ev->xcolormap.window == x11_win) {
			if (ev->xcolormap.state == ColormapInstalled) {
				//DPRINTF("colormap uninstalled\n");
				x11_colormap_installed = 0;
			} else if (ev->xcolormap.state == ColormapInstalled) {
				x11_colormap_installed = 1;
				//DPRINTF("colormap installed\n");
			}
		}
	} else if (ev->type == FocusIn) {
		if (!x11_colormap_installed) {
			//DPRINTF("setting colormap\n");
			XInstallColormap(x11_dpy, x11_colormap);
			inited = 1;
		}
	} else if (ev->type == MappingNotify) {
		DPRINTF("Refreshing keyboard mapping\n");
		XRefreshKeyboardMapping(&ev->xmapping);
	}
#if 0
	else if (ev->type == EnterNotify) {
		if (inited)
			GdShowCursor(&scrdev);
	} else if (ev->type == LeaveNotify) {
		if (inited)
			GdHideCursor(&scrdev);
	}
#endif
#if USE_EXPOSURE
	else if (ev->type == Expose) {
		scrdev.Update(&scrdev, ev->xexpose.x, ev->xexpose.y, ev->xexpose.width, ev->xexpose.height);
	}
#endif
}

static int
x11_error(Display * dpy, XErrorEvent * ev)
{
	char err_string[256];

	XGetErrorText(dpy, ev->error_code, err_string, 256);
	EPRINTF("X11 error: %s\n", err_string);
	return 0;
}

static void
show_visual(Visual * v)
{
#if DEBUG
	static const char * const classnm[] = {
		"StaticGray", "GrayScale", "StaticColor",
		"PseudoColor", "TrueColor", "DirectColor"
	};

	const char *name = ((v->class < 0) || (v->class > 5)) ? "???" : classnm[v->class];
	DPRINTF("  Visual  class: %s (%d)\n", name, v->class);
	DPRINTF("             id: %ld\n", v->visualid);
	DPRINTF("   bits_per_rgb: %d\n", v->bits_per_rgb);
	DPRINTF("    map_entries: %d\n", v->map_entries);
	DPRINTF("       red_mask: 0x%08lx\n", v->red_mask);
	DPRINTF("     green_mask: 0x%08lx\n", v->green_mask);
	DPRINTF("      blue_mask: 0x%08lx\n", v->blue_mask);
#endif
}

static Visual *
select_visual(Display * dpy, int scr)
{
	Visual *vis = XDefaultVisual(dpy, scr);
#if MWPIXEL_FORMAT != MWPF_PALETTE	/* Patch by Jon to try for 8-bit TrueColor */
	Visual *vis2 = NULL;
#endif
	Screen *screen = XScreenOfDisplay(dpy, scr);
	int d;

//	DPRINTF("XDefaultVisual:\n");
//	show_visual(vis);
//	DPRINTF("Screen RootDepth: %d\n", screen->root_depth);
//	DPRINTF("Screen RootVisual\n");
//	show_visual(screen->root_visual);

	/* print all depths/visuals */
	for (d = 0; d < screen->ndepths; d++) {
		Depth *dp = screen->depths + d;
		int v;
		Visual *cur_vis;
//		DPRINTF("Depth: %d\n", dp->depth);
		for (v = 0; v < dp->nvisuals; v++) {
//			DPRINTF("Visual: %d\n", v);
			cur_vis = dp->visuals + v;
//			show_visual(cur_vis);
#if MWPIXEL_FORMAT != MWPF_PALETTE	/* Patch by Jon to try for 8-bit TrueColor */
			if ((vis->class != TrueColor)
			    && (dp->depth == screen->root_depth)
			    && (cur_vis->class == TrueColor)) {
				/* Maybe useful... */
				if ((vis2 == NULL)
				    || (vis2->bits_per_rgb <
					cur_vis->bits_per_rgb)) {
					/* Better than what we had before */
					vis2 = cur_vis;
				}
			}
#endif
		}
	}
#if MWPIXEL_FORMAT != MWPF_PALETTE	/* Patch by Jon to try for 8-bit TrueColor */
	if ((vis->class != TrueColor) && (vis2 != NULL))
		vis = vis2;

	DPRINTF("Selected Visual:\n");
	show_visual(vis);
#endif

	return vis;
}


int
x11_setup_display(void)
{
	static int setup_needed = 1;

	if (setup_needed) {
		char *name;
#if MWPIXEL_FORMAT != MWPF_PALETTE
		int i;
#endif

		if ((name = getenv("DISPLAY")) == NULL)
			name = ":0";
		if ((x11_dpy = XOpenDisplay(name)) == NULL)
			return -1;

		XSetErrorHandler(x11_error);
#if 0
{
	int	events, errors, major, minor;

	if (XDGAQueryExtension(x11_dpy, &events, &errors) &&
	    XDGAQueryVersion(x11_dpy, &major, &minor)) {
			if (major >= 2 && XDGAOpenFramebuffer(x11_dpy, DefaultScreen(x11_dpy))) {
				DPRINTF("GOT it!\n");
				XDGACloseFramebuffer(x11_dpy, DefaultScreen(x11_dpy));
			}
	}
}
#endif
		x11_width = nxres? nxres: SCREEN_WIDTH;
		x11_height = nyres? nyres: SCREEN_HEIGHT;
		x11_scr = XDefaultScreen(x11_dpy);
		x11_vis = select_visual(x11_dpy, x11_scr);

		if (x11_vis->class == StaticGray || x11_vis->class == GrayScale) {
			EPRINTF("Nano-X Error: Your X server appears to be grayscale only.\nThis is not supported.\n");
			return -1;
		} else if (x11_vis->class == DirectColor) {
			EPRINTF("Nano-X Error: Your X server appears to use the 'DirectColor' format.\nThis is not supported.\n");
			return -1;
		} else if (x11_vis->class == PseudoColor || x11_vis->class == StaticColor) {
			x11_is_palette = 1; 		/* Palette based. */
		} else if (x11_vis->class == TrueColor) {
			unsigned long mask;
			int shift;

			x11_is_palette = 0;

			/*
			 * What we want to do:
			 *
			 * Given the RGB components:
			 *   0 <= r <= 255,
			 *   0 <= g <= 255,
			 *   0 <= b <= 255
			 *
			 * We need to compute the X11 color, c:
			 *
			 *   xr = ((r << 24) >> r_shift) & r_mask;
			 *   xg = ((g << 24) >> g_shift) & g_mask;
			 *   xb = ((b << 24) >> b_shift) & b_mask;
			 *
			 *   c  = xr | xg | xb;
			 *
			 * This code computes appropriate values for r/g/b_shift, given
			 * the r/g/b_mask values (which are taken from the X11 Visual).
			 *
			 *
			 * The reason for doing the wierd <<24 before the >>r_shift is
			 * that without it we may have to shift in either direction (<< or
			 * >>).  You can't shift by a negative amount, so you end up with
			 * either an "if" (slow!) or two shifts.  The <<24 actually has no
			 * cost, since "r" is created using a shift, we just change that
			 * shift.
			 */

			x11_r_mask = x11_vis->red_mask;
			x11_g_mask = x11_vis->green_mask;
			x11_b_mask = x11_vis->blue_mask;

			/* Calculate red shift */
			mask = x11_r_mask;
			shift = 0;
			assert(mask);
			if (mask) {
				while ((mask & 0x80000000UL) == 0) {
					mask <<= 1;
					shift++;
				}
			}
			x11_r_shift = shift;

			/* Calculate green shift */
			mask = x11_g_mask;
			shift = 0;
			assert(mask);
			if (mask) {
				while ((mask & 0x80000000UL) == 0) {
					mask <<= 1;
					shift++;
				}
			}
			x11_g_shift = shift;

			/* Calculate blue shift */
			mask = x11_b_mask;
			shift = 0;
			assert(mask);
			if (mask) {
				while ((mask & 0x80000000UL) == 0) {
					mask <<= 1;
					shift++;
				}
			}
			x11_b_shift = shift;
		} else {
			assert(0);
			return -1;
		}

		x11_gc = XDefaultGC(x11_dpy, x11_scr);

#if MWPIXEL_FORMAT != MWPF_PALETTE
		for (i = 0; i < COLOR_CACHE_SIZE; i++)
			ccache[i].init = 1;
#endif

		XSetFunction(x11_dpy, x11_gc, GXcopy);

		setup_needed = 0;
	}
	return 0;
}

#if MW_FEATURE_RESIZEFRAME
/* resize X11 frame*/
void
GdResizeFrameWindow(int width, int height, const char *title)
{
	XSizeHints *sizehints = XAllocSizeHints();
	if (sizehints != NULL) {
		sizehints->flags = PMinSize | PMaxSize;
		sizehints->min_width = width;
		sizehints->min_height = height;
		sizehints->max_width = width;
		sizehints->max_height = height;
		XSetWMNormalHints(x11_dpy, x11_win, sizehints);
		XFree(sizehints);
	}
	XResizeWindow(x11_dpy, x11_win, width, height);
	if (title)
		XStoreName(x11_dpy, x11_win, title);
	XMapWindow(x11_dpy, x11_win);
}
#endif

/* Note: only single screen */
static PSD
X11_open(PSD psd)
{
	XSetWindowAttributes attr;
	Pixmap cur_empty;
	unsigned long valuemask;
	unsigned int event_mask;
	XColor color;
	Cursor cursor;
	/*XEvent ev; */
	XSizeHints *sizehints;
	int flags;

	if (x11_setup_display() < 0)
		return NULL;

	x11_event_mask = ColormapChangeMask | FocusChangeMask;
#if USE_EXPOSURE
	x11_event_mask |= ExposureMask;	/* handled by mou_x11*/
	valuemask = CWSaveUnder | CWEventMask;
#else
	valuemask = CWSaveUnder | CWEventMask | CWBackingStore;
#endif
	/*x11_event_mask |= EnterWindowMask | LeaveWindowMask;*/

	event_mask = x11_event_mask | 
		KeyPressMask |		/* handled by kbd_x11 */
		KeyReleaseMask |	/* handled by kbd_x11 */
		ButtonPressMask |	/* handled by mou_x11 */
		ButtonReleaseMask |	/* handled by mou_x11 */
		PointerMotionMask;	/* handled by mou_x11 */

	attr.backing_store = Always;	/* auto expose */
	attr.save_under = True;		/* popups ... */
	attr.event_mask = event_mask;

#if 1				/* Patch by Jon to try for 8-bit TrueColor */
	x11_colormap = XCreateColormap(x11_dpy, XDefaultRootWindow(x11_dpy), x11_vis, AllocNone);
	attr.colormap = x11_colormap;
	valuemask |= CWColormap;
#endif

	x11_win = XCreateWindow(x11_dpy, XDefaultRootWindow(x11_dpy), 100,	/* x */
				100,		/* y */
				x11_width,	/* width */
				x11_height,	/* height */
				2,		/* border */
				CopyFromParent,	/* depth */
				InputOutput,	/* class */
				x11_vis,	/* Visual */
				valuemask,	/* valuemask */
				&attr		/* attributes */
		);

	sizehints = XAllocSizeHints();
	if (sizehints != NULL) {
		sizehints->flags = PMinSize | PMaxSize;
		sizehints->min_width = x11_width;
		sizehints->min_height = x11_height;
		sizehints->max_width = x11_width;
		sizehints->max_height = x11_height;
		XSetWMNormalHints(x11_dpy, x11_win, sizehints);
		XFree(sizehints);
	}

	x11_depth = XDefaultDepth(x11_dpy, x11_scr);

#if 0				/* REMOVED by Patch by Jon to try for 8-bit TrueColor */
	/* Create a new empty colormap, the colormap will be
	 ** filled by lookup_color in the case of
	 ** GrayScale, PseduoColor and DirectColor,
	 ** or looked up in the case of 
	 **  StaticGray, StaticColor and TrueColor
	 */
	x11_colormap = XDefaultColormap(x11_dpy, x11_scr);
	if (x11_vis->class & 1)
		x11_colormap = XCopyColormapAndFree(x11_dpy, x11_colormap);
#endif

	/* If you need more colors, create it from scratch
	 *
	 * x11_colormap = XCreateColormap(x11_dpy, x11_win, x11_vis,
	 * AllocNone);  
	 *
	 * or: for same visual etc.
	 * x11_colormap = XCopyColormapAndFree(x11_dpy, x11_colormap);
	 */

	/* Create an empty (invisible) cursor.  This is because
	 * Microwindows will draw it's own cursor.
	 */
	cur_empty = XCreateBitmapFromData(x11_dpy, x11_win, "\0", 1, 1);
	cursor = XCreatePixmapCursor(x11_dpy, cur_empty, cur_empty, &color, &color, 0, 0);
	XDefineCursor(x11_dpy, x11_win, cursor);
	XStoreName(x11_dpy, x11_win, "Microwindows");

#if !MW_FEATURE_RESIZEFRAME
	XMapWindow(x11_dpy, x11_win);
#endif
	XFlush(x11_dpy);

#if 0
	/*
	 * The following code insures that the colormap
	 * is installed before display
	 */
	XMaskEvent(x11_dpy, x11_event_mask, &ev);
	XPutBackEvent(x11_dpy, &ev);
#endif
	XInstallColormap(x11_dpy, x11_colormap);

	/* init psd and allocate framebuffer*/
	flags = PSF_SCREEN | PSF_ADDRMALLOC | PSF_DELAYUPDATE;

	if (!gen_initpsd(psd, MWPIXEL_FORMAT, x11_width, x11_height, flags))
		return NULL;

DPRINTF("x11 emulated bpp %d\n", psd->bpp);
	return psd;
}

static void
X11_close(PSD psd)
{
	/* free framebuffer memory */
	free(psd->addr);

	XCloseDisplay(x11_dpy);
}

static void
X11_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);

	psi->xdpcm = (DisplayWidth(x11_dpy, x11_scr) * 10) / DisplayWidthMM(x11_dpy, x11_scr);
	psi->ydpcm = (DisplayHeight(x11_dpy, x11_scr) * 10) / DisplayHeightMM(x11_dpy, x11_scr);
}

static void
X11_setpalette(PSD psd, int first, int count, MWPALENTRY * pal)
{
	int i;
	int n;

	for (i = 0; i < count; i++) {
		char spec[20];
		unsigned short r, g, b;
		XColor exact;
		XColor def;

		r = pal[i].r;
		g = pal[i].g;
		b = pal[i].b;
		sprintf(spec, "#%02x%02x%02x", r, g, b);
		XLookupColor(x11_dpy, x11_colormap, spec, &exact, &def);
		XAllocColor(x11_dpy, x11_colormap, &def);
		/* DPRINTF("lookup: exact(%d,%d,%d) = %d, def(%d,%d,%d)=%d\n",
		   exact.red, exact.green, exact.blue, exact.pixel,
		   def.red, def.green, def.blue, def.pixel); */
		x11_palette[first + i] = def;
	}
	n = first + count - 1;
	if (n > x11_pal_max)
		x11_pal_max = n;
}

static void
update_from_savebits(PSD psd, unsigned int destx, unsigned int desty, int w, int h)
{
	XImage *img;
	unsigned int x, y;
	char *data;

	/* allocate buffer */
	if (x11_depth >= 24)
		data = malloc(w * 4 * h);
	else if (x11_depth > 8)	/* 15, 16 */
		data = malloc(w * 2 * h);
	else			/* 1,2,4,8 */
		data = malloc((w * x11_depth + 7) / 8 * h);

	/* copy from offscreen to screen */
	img = XCreateImage(x11_dpy, x11_vis, x11_depth, ZPixmap, 0, data, w, h, 8, 0);

	/* Use optimized loops for most common framebuffer modes */

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = addr[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = ((ADDR16)addr)[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#elif MWPIXEL_FORMAT == MWPF_TRUECOLORRGB
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		unsigned int extra = psd->pitch - w * 3;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = RGB2PIXEL888(addr[2], addr[1], addr[0]);
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
				addr += 3;
			}
			addr += extra;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLORARGB) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = ((ADDR32)addr)[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#else /* MWPF_PALETTE*/
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = addr[x];
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			addr += psd->pitch;
		}
	}
#endif

	XPutImage(x11_dpy, x11_win, x11_gc, img, 0, 0, destx, desty, w, h);
	XDestroyImage(img);
}

/* called before select(), returns # pending events*/
static int
X11_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region to X11 server*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX >= 0 || upmaxY >= 0)) {
		update_from_savebits(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);

		/* reset update region*/
		upminX = upminY = MAX_MWCOORD;
		upmaxX = upmaxY = MIN_MWCOORD;
	}

	XFlush(x11_dpy);
	return XPending(x11_dpy);
}

static void
X11_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = MWMIN(x, upminX);
			upminY = MWMIN(y, upminY);
			upmaxX = MWMAX(upmaxX, x+width-1);
			upmaxY = MWMAX(upmaxY, y+height-1);
	} else
		update_from_savebits(psd, x, y, width, height);
}
