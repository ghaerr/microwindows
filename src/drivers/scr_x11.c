/*
 * Copyright (c) 2000, 2001 Greg Haerr <greg@censoft.com>
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
#include <assert.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

/* define the Casio E-15 (allow override) */
#ifdef SCREEN_E15
#ifndef SCREEN_WIDTH
#define    SCREEN_WIDTH  200
#endif
#ifndef SCREEN_HEIGHT
#define    SCREEN_HEIGHT 320
#endif
#ifndef SCREEN_DEPTH
#define    SCREEN_DEPTH  4
#endif
#ifndef MWPIXEL_FORMAT
#define    MWPIXEL_FORMAT MWPF_PALETTE
#endif
#endif

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 200
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 320
#endif

#ifndef SCREEN_DEPTH
#define SCREEN_DEPTH 4
#endif

#ifndef MWPIXEL_FORMAT
#if SCREEN_DEPTH <= 8
#define MWPIXEL_FORMAT MWPF_PALETTE
#elif SCREEN_DEPTH == 16
#define MWPIXEL_FORMAT MWPF_TRUECOLOR565
#elif SCREEN_DEPTH == 15
#define MWPIXEL_FORMAT MWPF_TRUECOLOR555
#elif SCREEN_DEPTH == 24
#define MWPIXEL_FORMAT MWPF_TRUECOLOR888
#elif SCREEN_DEPTH == 32
#define MWPIXEL_FORMAT MWPF_TRUECOLOR0888
#else
#error "bad screen depth"
#endif
#endif


/* specific x11 driver entry points*/
static PSD  X11_open(PSD psd);
static void X11_close(PSD psd);
static void X11_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void X11_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void X11_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL X11_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void X11_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void X11_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void X11_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,MWPIXELVAL c);
static void X11_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
		     PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
static void X11_preselect(PSD psd);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	X11_open,
	X11_close,
	X11_getscreeninfo,
	X11_setpalette,
	X11_drawpixel,
	X11_readpixel,
	X11_drawhline,
	X11_drawvline,
	X11_fillrect,
	gen_fonts,
	X11_blit,
	X11_preselect,
	NULL,			/* DrawArea*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	fb_mapmemgc,
	gen_freememgc,
	NULL,			/* StretchBlit subdriver*/
	NULL			/* SetPortrait*/
};

/* called from keyboard/mouse/screen */
Display*     x11_dpy;
int          x11_scr;
Visual*      x11_vis;
Colormap     x11_colormap;
Window       x11_win;
GC           x11_gc;
unsigned int x11_event_mask;

static int   x11_colormap_installed = 0;
static SCREENDEVICE savebits;	/* permanent offscreen drawing buffer*/

DEFINE_applyOpR			/* define inline rop calculator*/

/* Color cache for true color lookups
** FIXME: for 24 bit i belive we could do the pixel direct but...
*/

#define COLOR_CACHE_SIZE 1001
struct color_cache {
	int    		init;  /* 1 if first use */
	unsigned short	r;
	unsigned short 	g;
	unsigned short 	b;
	XColor 		c;
};
static struct color_cache ccache[COLOR_CACHE_SIZE];

/* color palette for color indexe */
static XColor       x11_palette[256];
static int          x11_pal_max = 0;
static int          x11_pixtype;

static unsigned long
lookup_color(unsigned short r, unsigned short g, unsigned short b)
{
    int ix = ((r << 16) + (g << 8) + b) % COLOR_CACHE_SIZE;

    if (ccache[ix].init ||
	(ccache[ix].r != r) || (ccache[ix].g != g) || (ccache[ix].b != b)) {
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

static unsigned long
PIXELVAL_to_pixel(MWPIXELVAL c, int type)
{
	assert (type == MWPIXEL_FORMAT);
	
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR0888) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR888)
	/* calc truecolor conversions directly*/
	if (x11_vis->class >= TrueColor) {
		switch (x11_vis->bits_per_rgb) {
		case 8:
			return c;
		case 6:
			return RGB2PIXEL565(PIXEL888RED(c), PIXEL888GREEN(c),
				PIXEL888BLUE(c));
		case 5:
			return RGB2PIXEL555(PIXEL888RED(c), PIXEL888GREEN(c),
				PIXEL888BLUE(c));
		case 3:
		case 2:
			return RGB2PIXEL332(PIXEL888RED(c), PIXEL888GREEN(c),
				PIXEL888BLUE(c));
		}
	}
	return lookup_color(PIXEL888RED(c), PIXEL888GREEN(c), PIXEL888BLUE(c));
#endif
#if MWPIXEL_FORMAT == MWPF_TRUECOLOR565
	/* calc truecolor conversions directly*/
	if (x11_vis->class >= TrueColor) {
		switch (x11_vis->bits_per_rgb) {
		case 8:
			return RGB2PIXEL888(PIXEL565RED(c)<<3,
				PIXEL565GREEN(c)<<2, PIXEL565BLUE(c)<<3);
		case 6:
		case 5:
			return c;
		case 3:
		case 2:
			return RGB2PIXEL332(PIXEL565RED(c)<<3,
				PIXEL565GREEN(c)<<2, PIXEL565BLUE(c)<<3);
		}
	}
	return lookup_color(PIXEL565RED(c)<<3, PIXEL565GREEN(c)<<2,
			PIXEL565BLUE(c)<<3);
#endif
#if MWPIXEL_FORMAT == MWPF_TRUECOLOR555
	/* calc truecolor conversions directly*/
	if (x11_vis->class >= TrueColor) {
		switch (x11_vis->bits_per_rgb) {
		case 8:
			return RGB2PIXEL888(PIXEL555RED(c)<<3,
				PIXEL555GREEN(c)<<3, PIXEL555BLUE(c)<<3);
		case 6:
		case 5:
			return c;
		case 3:
		case 2:
			return RGB2PIXEL332(PIXEL555RED(c)<<3,
				PIXEL555GREEN(c)<<3, PIXEL555BLUE(c)<<3);
		}
	}
	return lookup_color(PIXEL555RED(c)<<3, PIXEL555GREEN(c)<<3,
			PIXEL555BLUE(c)<<3);
#endif
#if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
	/* calc truecolor conversions directly*/
	if (x11_vis->class >= TrueColor) {
		switch (x11_vis->bits_per_rgb) {
		case 8:
			return RGB2PIXEL888(PIXEL332RED(c)<<5,
				PIXEL332GREEN(c)<<5, PIXEL332BLUE(c)<<6);
		case 6:
			return RGB2PIXEL565(PIXEL332RED(c)<<5,
				PIXEL332GREEN(c)<<5, PIXEL332BLUE(c)<<6);
		case 5:
			return RGB2PIXEL555(PIXEL332RED(c)<<5,
				PIXEL332GREEN(c)<<5, PIXEL332BLUE(c)<<6);
		case 3:
		case 2:
			return c;
		}
	}
	return lookup_color(PIXEL332RED(c)<<5, PIXEL332GREEN(c)<<5,
			PIXEL332BLUE(c)<<6);
#endif
#if MWPIXEL_FORMAT == MWPF_PALETTE
	if (c > x11_pal_max) {
	    DPRINTF("Warning: palette index out of range (%ld)\n", c);
	    return 0;
	}
	return x11_palette[c].pixel;
#endif
#if 0
    switch(type) {
    case MWPF_TRUECOLOR0888:
    case MWPF_TRUECOLOR888:
	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);
	return lookup_color(r, g, b);
	
    case MWPF_TRUECOLOR565:
	r = PIXEL565RED(c) << 3;
	g = PIXEL565GREEN(c) << 2;
	b = PIXEL565BLUE(c) << 3;
	return lookup_color(r, g, b);

    case MWPF_TRUECOLOR555:
	r = PIXEL555RED(c) << 3;
	g = PIXEL555GREEN(c) << 3;
	b = PIXEL555BLUE(c) << 3;
	return lookup_color(r, g, b);

    case MWPF_TRUECOLOR332:
	r = PIXEL332RED(c) << 5;
	g = PIXEL332GREEN(c) << 5;
	b = PIXEL332BLUE(c) << 6;
	return lookup_color(r, g, b);

    case MWPF_PALETTE:
	if (c > x11_pal_max) {
	    DPRINTF("Warning: palette index out of range (%ld)\n", c);
	    return 0;
	}
	return x11_palette[c].pixel;
    }
#endif
    return 0;
}


static void set_color(MWPIXELVAL c)
{
    static unsigned long oldc = 0x80000001;

    if (c != oldc) {
        oldc = c;
        XSetForeground(x11_dpy, x11_gc, PIXELVAL_to_pixel(c, x11_pixtype));
    }
}


static void set_mode(int new_mode)
{
    static int old_mode = -1;
    
    if (new_mode != old_mode) {
	int func = GXcopy;
	switch(new_mode) {
	case MWMODE_COPY: 		func = GXcopy; break;
	case MWMODE_XOR: 		func = GXxor; break;
	case MWMODE_OR:  		func = GXor; break;
	case MWMODE_AND: 		func = GXand; break;
	case MWMODE_CLEAR:		func = GXclear; break;
	case MWMODE_SETTO1:		func = GXset; break;
	case MWMODE_EQUIV:		func = GXequiv; break;
	case MWMODE_NOR	:		func = GXnor; break;
	case MWMODE_NAND:		func = GXnand; break;
	case MWMODE_INVERT:		func = GXinvert; break;
	case MWMODE_COPYINVERTED:	func = GXcopyInverted; break;
	case MWMODE_ORINVERTED:		func = GXorInverted; break;
	case MWMODE_ANDINVERTED:	func = GXandInverted; break;
	case MWMODE_ORREVERSE:		func = GXorReverse; break;
	case MWMODE_ANDREVERSE:		func = GXandReverse; break;
	case MWMODE_NOOP:		func = GXnoop; break;
	default: return;
	}
	XSetFunction(x11_dpy, x11_gc, func);
	old_mode = new_mode;
    }
}

#ifdef USE_EXPOSURE
static void _expose(int _x, int _y, int w, int h)
{
  XImage* img;
  int depth = XDefaultDepth(x11_dpy, x11_scr);
  int x = _x, y = _y;
  char* data;
  
  /* allocate buffer */
  if (depth >= 24)
    data = malloc(w*4*h);
  else if (depth > 8) /* 15, 16 */
    data = malloc(w*2*h);
  else  /* 1,2,4,8 */
    data = malloc((w*depth+7)/8 * h);
  
  /* copy from offscreen to screen */
  img = XCreateImage(x11_dpy, x11_vis, depth, ZPixmap,
		     0, data, w, h, 8, 0);
  for (y = _y; y < h + _y; y++) {
    for (x = _x; x < w + _x; x++) {
      MWPIXELVAL c = savebits.ReadPixel(&savebits,x,y);
      unsigned long pixel = PIXELVAL_to_pixel(c, savebits.pixtype);
      XPutPixel(img, x - _x, y - _y, pixel);
    }
  }
  
  XPutImage(x11_dpy, x11_win, x11_gc, img, 0, 0, _x, _y, w, h);
  XDestroyImage(img);
}
#endif

/* called from mou_x11 (handels x11_event_mask events) */
void x11_handle_event(XEvent* ev)
{
    static int inited = 0;

    if (ev->type == ColormapNotify) {
	if (ev->xcolormap.window == x11_win) {
	    if (ev->xcolormap.state == ColormapInstalled) {
		DPRINTF("colormap uninstalled\n"); 
		x11_colormap_installed = 0;
	    }
	    else if (ev->xcolormap.state == ColormapInstalled) {
		x11_colormap_installed = 1;
		DPRINTF("colormap installed\n");
	    }
	}
    }
    else if (ev->type == FocusIn) {
	if (!x11_colormap_installed) {
	    DPRINTF("setting colormap\n");
	    XInstallColormap(x11_dpy, x11_colormap);
	    inited = 1;
	}
    }
    else if(ev->type == MappingNotify) {
	DPRINTF("Refreshing keyboard mapping\n");
	XRefreshKeyboardMapping(&ev->xmapping);
    }
#if 0
    else if (ev->type == EnterNotify) {
	    if(inited)
	    GdShowCursor(&scrdev);
    } else if (ev->type == LeaveNotify) {
	    if(inited)
	    GdHideCursor(&scrdev);
    }
#endif

#ifdef USE_EXPOSURE
    else if(ev->type == Expose) {
      _expose(ev->xexpose.x,ev->xexpose.y, ev->xexpose.width,ev->xexpose.height);
    }
#endif
}


static int x11_error(Display* dpy, XErrorEvent* ev)
{
    char err_string[256];

    XGetErrorText(dpy, ev->error_code, err_string, 256);
    EPRINTF("X11 error: %s\n", err_string);
    return 0;
}

char* classnm[] = { "StaticGray", "GrayScale", "StaticColor",
		    "PseudoColor", "TrueColor", "DirectColor" };

static void show_visual(Visual* v)
{
    char* name = ((v->class < 0) || (v->class > 5)) ? "???" : 
	classnm[v->class];
    DPRINTF("  Visual  class: %s (%d)\n", name, v->class);
    DPRINTF("             id: %ld\n", v->visualid);
    DPRINTF("   bits_per_rgb: %d\n", v->bits_per_rgb);
    DPRINTF("    map_entries: %d\n", v->map_entries);
}

static Visual* select_visual(Display* dpy, int scr)
{
    Visual* vis = XDefaultVisual(dpy, scr);
    Screen* screen = XScreenOfDisplay(dpy, scr);
    int d;

    DPRINTF("XDefaultVisual:\n");
    show_visual(vis);

    DPRINTF("Screen RootDepth: %d\n", screen->root_depth);
    
    DPRINTF("Screen RootVisual\n");
    show_visual(screen->root_visual);
    
    /* print all depths/visuals */

    for (d = 0; d < screen->ndepths; d++) {
	Depth* dp = screen->depths + d;
	int v;
	DPRINTF("Depth: %d\n", dp->depth);
	for (v = 0; v < dp->nvisuals; v++) {
	    DPRINTF("Visual: %d\n", v);
	    show_visual(dp->visuals + v);
	}
    }
    return vis;
}


int x11_setup_display()
{
    static int setup_needed = 1;
    
    if (setup_needed) {
	char* name;
	int i;

	if ((name = getenv("DISPLAY")) == NULL)
	    name = ":0";
	if ((x11_dpy = XOpenDisplay(name)) == NULL)
	    return -1;

	XSetErrorHandler(x11_error);

	x11_scr = XDefaultScreen(x11_dpy);
	x11_vis = select_visual(x11_dpy, x11_scr);

	x11_gc = XDefaultGC(x11_dpy, x11_scr);

	for (i = 0; i < COLOR_CACHE_SIZE; i++)
	    ccache[i].init = 1;

	set_mode(gr_mode);

	setup_needed = 0;
	return 0;
    }
    return 0;
}

/* Note: only single screen */
static PSD
X11_open(PSD psd)
{
    XSetWindowAttributes attr;
    Pixmap cur_empty;
    unsigned long valuemask;
    unsigned int  event_mask;
    XColor color;
    Cursor cursor;
    /*XEvent ev;*/
    PSUBDRIVER subdriver;
    int size, linelen;

    if (x11_setup_display() < 0)
	return NULL;

    x11_event_mask = ColormapChangeMask | FocusChangeMask;
/*x11_event_mask |= EnterWindowMask | LeaveWindowMask;*/

    event_mask = x11_event_mask |
                     ExposureMask |
	             KeyPressMask |       /* handled by kbd_x11 */
	             KeyReleaseMask |     /* handled by kbd_x11 */
	             ButtonPressMask |    /* handled by mou_x11 */
		     ButtonReleaseMask |  /* handled by mou_x11 */
		     PointerMotionMask;   /* handled by mou_x11 */
		     

#ifdef USE_EXPOSURE
    valuemask = CWSaveUnder |
      CWEventMask;
#else
    valuemask = CWSaveUnder |
      CWEventMask |
      CWBackingStore;
#endif
    
    attr.backing_store = Always;     /* auto expose */
    attr.save_under    = True;       /* popups ... */
    attr.event_mask    = event_mask;

    x11_win = XCreateWindow(x11_dpy,
			    XDefaultRootWindow(x11_dpy),
			    100,             /* x */
			    100,             /* y */
			    SCREEN_WIDTH,    /* width */
			    SCREEN_HEIGHT,   /* height */
			    2,               /* border */
			    CopyFromParent,  /* depth */
			    CopyFromParent,  /* depth */
			    x11_vis,         /* Visual */
			    valuemask,	     /* valuemask */
			    &attr            /* attributes */
			    );

    /* Create a new empty colormap, the colormap will be
     ** filled by lookup_color in the case of
     ** GrayScale, PseduoColor and DirectColor,
     ** or looked up in the case of 
     **  StaticGray, StaticColor and TrueColor
     */

    x11_colormap = XDefaultColormap(x11_dpy, x11_scr);
    if (x11_vis->class & 1)
	x11_colormap = XCopyColormapAndFree(x11_dpy, x11_colormap);

    /* If you need more colors, create it from scratch
     *
     * x11_colormap = XCreateColormap(x11_dpy, x11_win, x11_vis,
     * AllocNone);  
     *
     * or: for same visual etc.
     * x11_colormap = XCopyColormapAndFree(x11_dpy, x11_colormap);
     */

    /* Create an empty (invisible) cursor */
    cur_empty = XCreateBitmapFromData(x11_dpy, x11_win, "\0", 1, 1);
    cursor = XCreatePixmapCursor(x11_dpy, cur_empty, cur_empty,
				 &color, &color, 0, 0);
    XDefineCursor(x11_dpy, x11_win, cursor);
    XStoreName(x11_dpy, x11_win, "Microwindows");

    XMapWindow(x11_dpy, x11_win);
    XFlush(x11_dpy);

    /*
     * The following code insures that the colormap
     * is installed before display
     */
#if 0
    XMaskEvent(x11_dpy, x11_event_mask, &ev);
    XPutBackEvent(x11_dpy, &ev);
#endif
    XInstallColormap(x11_dpy, x11_colormap);

    psd->xres    = psd->xvirtres    = SCREEN_WIDTH;
    psd->yres    = psd->yvirtres    = SCREEN_HEIGHT;
    psd->linelen = SCREEN_WIDTH;
    psd->planes  = 1;
    psd->pixtype = x11_pixtype = MWPIXEL_FORMAT;
    switch(psd->pixtype) {
    case MWPF_PALETTE:
	    psd->bpp = SCREEN_DEPTH;
	    break;
    case MWPF_TRUECOLOR0888:
    default:
	    psd->bpp = 32;
	    break;
    case MWPF_TRUECOLOR888:
	    psd->bpp = 24;
	    break;
    case MWPF_TRUECOLOR565:
    case MWPF_TRUECOLOR555:
	    psd->bpp = 16;
	    break;
    case MWPF_TRUECOLOR332:
	    psd->bpp = 8;
	    break;
    }

    /* Calculate the correct linelen here */

    GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes,
		     psd->bpp, &size, &psd->linelen);
    
    psd->ncolors = psd->bpp >= 24? (1 << 24): (1 << psd->bpp);
    psd->flags   = PSF_SCREEN | PSF_HAVEBLIT;
    psd->size = 0;
    psd->addr = NULL;
    psd->portrait = MWPORTRAIT_NONE;

    /* create permanent savebits memory device from screen device*/
    savebits = *psd;
    savebits.flags = PSF_MEMORY | PSF_HAVEBLIT;

    /* select a fb subdriver matching our planes and bpp*/
    subdriver = select_fb_subdriver(&savebits);
    if (!subdriver)
	    return NULL;

    /* calc size and linelen of savebits alloc*/
    /* JHC - Is this redundant now?? */

    GdCalcMemGCAlloc(&savebits, savebits.xvirtres, savebits.yvirtres, 0, 0,
		&size, &linelen);
    savebits.linelen = linelen;
    savebits.size = size;
    if ((savebits.addr = malloc(size)) == NULL)
	return NULL;

    set_subdriver(&savebits, subdriver, TRUE);


    /* set X11 psd to savebits memaddr for screen->offscreen blits...*/
    psd->addr = savebits.addr;

    return psd;
}

static void
X11_close(PSD psd)
{
    /* free savebits memory*/
    free(savebits.addr);

    XCloseDisplay(x11_dpy);
}


static void
X11_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
    psi->rows = psd->yvirtres;
    psi->cols = psd->xvirtres;
    psi->planes = psd->planes;
    psi->bpp = psd->bpp;
    psi->ncolors = psd->ncolors;
    psi->portrait = psd->portrait;
    psi->fonts = NUMBER_FONTS;
    psi->xdpcm = (DisplayWidth(x11_dpy,x11_scr)*10)/
	    DisplayWidthMM(x11_dpy,x11_scr);
    psi->ydpcm = (DisplayHeight(x11_dpy,x11_scr)*10)/
	    DisplayHeightMM(x11_dpy,x11_scr);

	psi->fbdriver = FALSE;	/* not running fb driver, no direct map*/
	psi->pixtype = psd->pixtype;
	switch (psd->pixtype) {
	case MWPF_TRUECOLOR0888:
	case MWPF_TRUECOLOR888:
		psi->rmask 	= 0xff0000;
		psi->gmask 	= 0x00ff00;
		psi->bmask	= 0x0000ff;
		break;
	case MWPF_TRUECOLOR565:
		psi->rmask 	= 0xf800;
		psi->gmask 	= 0x07e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR555:
		psi->rmask 	= 0x7c00;
		psi->gmask 	= 0x03e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR332:
		psi->rmask 	= 0xe0;
		psi->gmask 	= 0x1c;
		psi->bmask	= 0x03;
		break;
	case MWPF_PALETTE:
	default:
		psi->rmask 	= 0xff;
		psi->gmask 	= 0xff;
		psi->bmask	= 0xff;
		break;
	}
}

static void
X11_setpalette(PSD psd, int first, int count, MWPALENTRY *pal)
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
	x11_palette[first+i] = def;
    }
    n = first + count - 1;
    if (n > x11_pal_max)
	x11_pal_max = n;
}


static void
X11_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	set_color(c);
	set_mode(gr_mode);
	XDrawPoint(x11_dpy, x11_win, x11_gc, x, y);

	/* draw savebits for readpixel or blit*/
	savebits.DrawPixel(&savebits, x, y, c);
}

static MWPIXELVAL
X11_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	/* read savebits for pixel value, rather than ask X11*/
	return savebits.ReadPixel(&savebits,x,y);
}

static void
X11_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	set_color(c);
	set_mode(gr_mode);
	XDrawLine(x11_dpy, x11_win, x11_gc, x1, y, x2, y);

	/* draw savebits for readpixel or blit*/
	savebits.DrawHorzLine(&savebits, x1, x2, y, c);
}

static void
X11_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	set_color(c);
	set_mode(gr_mode);
	XDrawLine(x11_dpy, x11_win, x11_gc, x, y1, x, y2);
	savebits.DrawVertLine(&savebits, x, y1, y2, c);
}

static void
X11_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	set_color(c);
	set_mode(gr_mode);
	XFillRectangle(x11_dpy, x11_win, x11_gc, x1, y1, (x2-x1)+1, (y2-y1)+1);

	/* draw savebits for readpixel or blit*/
	savebits.FillRect(&savebits, x1, y1, x2, y2, c);
}

static void
X11_srccopyblit_screen_to_screen(PMWBLITARGS pb)
{
}

static void
X11_blendconstantblit_screen_to_mem(PMWBLITARGS pb)
{
}

static void
X11_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	 PSD srcpsd,MWCOORD srcx,MWCOORD srcy, long op)
{
#if ALPHABLEND
    unsigned int alpha;
#endif

    set_mode(gr_mode);
    if (dstpsd == srcpsd) {
	if (dstpsd->flags & PSF_SCREEN) {
	    XCopyArea(x11_dpy, x11_win, x11_win, x11_gc,
		      srcx, srcy, w, h, destx, desty);

	    /* update screen savebits as well*/
	    savebits.Blit(&savebits, destx, desty, w, h,
			&savebits, srcx, srcy, op);
	}
	else
	    /* memory to memory blit, use offscreen blitter*/
	    dstpsd->Blit(dstpsd, destx, desty, w, h, srcpsd, srcx, srcy, op);
    }
    else if (dstpsd->flags & PSF_SCREEN) {
	XImage* img;
	int depth = XDefaultDepth(x11_dpy, x11_scr);
	int x, y;
	char* data;

	/* allocate buffer */
	if (depth >= 24)
	    data = malloc(w*4*h);
	else if (depth > 8) /* 15, 16 */
	    data = malloc(w*2*h);
	else  /* 1,2,4,8 */
	    data = malloc((w*depth+7)/8 * h);

	/* copy from offscreen to screen */
	img = XCreateImage(x11_dpy, x11_vis, depth, ZPixmap,
			   0, data, w, h, 8, 0);
#if ALPHABLEND && (MWPIXEL_FORMAT != MWPF_PALETTE)
	if ((op & MWROP_EXTENSION) == MWROP_BLENDCONSTANT) {
		alpha = op & 0xff;

		for (y = 0; y < h; y++) {
	  		for (x = 0; x < w; x++) {
				MWPIXELVAL c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
				MWPIXELVAL cd = dstpsd->ReadPixel(dstpsd,destx+x,desty+y);
				unsigned char nred = ALPHAPIXELRED(c, cd, alpha);
				unsigned char ngreen = ALPHAPIXELGREEN(c, cd, alpha);
				unsigned char nblue = ALPHAPIXELBLUE(c, cd, alpha);
				unsigned long pixel = PIXELVAL_to_pixel(RGB2PIXEL(nred, ngreen, nblue), srcpsd->pixtype);
				XPutPixel(img, x, y, pixel);

				/* update screen savebits*/
				savebits.DrawPixel(&savebits, destx+x, desty+y, RGB2PIXEL(nred, ngreen, nblue));
	    		}
		}
	}
	else {
    		MWPIXELVAL c = 0;
		unsigned long pixel;

		for (y = 0; y < h; y++) {
	    		for (x = 0; x < w; x++) {
				if (op == MWROP_COPY)
					c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
				else {
					c = applyOpR(op,
						srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y),
						dstpsd->ReadPixel(dstpsd,destx+x,desty+y));
					pixel = PIXELVAL_to_pixel(c, srcpsd->pixtype);
					XPutPixel(img, x, y, pixel);
				}
				pixel = PIXELVAL_to_pixel(c, srcpsd->pixtype);
				XPutPixel(img, x, y, pixel);

				/* update screen savebits*/
				savebits.DrawPixel(&savebits, destx+x, desty+y, c);
			}
		}
	}
#else
	for (y = 0; y < h; y++) {
	    for (x = 0; x < w; x++) {
		MWPIXELVAL c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
		unsigned long pixel = PIXELVAL_to_pixel(c, srcpsd->pixtype);
		XPutPixel(img, x, y, pixel);
		/* update screen savebits*/
		savebits.DrawPixel(&savebits, destx+x, desty+y, c);
	    }
	}
#endif

	XPutImage(x11_dpy, x11_win, x11_gc, img, 0, 0, destx, desty, w, h);
	XDestroyImage(img);
    }
    else if (srcpsd->flags & PSF_SCREEN) {
	int x, y;

#if ALPHABLEND && (MWPIXEL_FORMAT != MWPF_PALETTE)
	if ((op & MWROP_EXTENSION) == MWROP_BLENDCONSTANT) {
		alpha = op & 0xff;

		for (y = 0; y < h; y++) {
	  		for (x = 0; x < w; x++) {
				MWPIXELVAL c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
				MWPIXELVAL cd = dstpsd->ReadPixel(dstpsd,destx+x,desty+y);
				unsigned char nred = ALPHAPIXELRED(c, cd, alpha);
				unsigned char ngreen = ALPHAPIXELGREEN(c, cd, alpha);
				unsigned char nblue = ALPHAPIXELBLUE(c, cd, alpha);
				dstpsd->DrawPixel(dstpsd, destx+x, desty+y, RGB2PIXEL(nred, ngreen, nblue));
			}
		}
	}
	else {
		/* copy from screen to offscreen,
		 * emulated by copy from offscreen bits, no alpha
		 */
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {	
				MWPIXELVAL c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
				dstpsd->DrawPixel(dstpsd, destx+x, desty+y, c);
			}
		}
	}
#else
	/* copy from screen to offscreen, emulated by copy from offscreen bits*/
	for (y = 0; y < h; y++) {
	    for (x = 0; x < w; x++) {	
		MWPIXELVAL c = srcpsd->ReadPixel(srcpsd,srcx+x,srcy+y);
		dstpsd->DrawPixel(dstpsd, destx+x, desty+y, c);
	    }
	}
#endif
    }
}

/* perform pre-select() duties*/
static void
X11_preselect(PSD psd)
{
	XFlush(x11_dpy);
}
