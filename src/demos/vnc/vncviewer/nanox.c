/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 *
 *  Microwindows interface by George Harvey
 *
 *  07/03/00  GH	created nanox.c to replace x.c, development
 *			being done using Microwindows 0.88pre3
 *  16/03/00  GH	try to match the VNC palette to the current
 *			palette using a lookup table
 *  06/05/00  GH	update for mwin 0.88pre7, use GrSetSystemPalette()
 *			instead of lookup table
 *  27/05/00  GH	update for mwin 0.88pre8
 *  03/06/00  GH	remove colour lookup code
 */

/*
 * nanox.c - functions to deal with nano-X display.
 */

#include <vncviewer.h>
#include <unistd.h>

#define VW_WIDTH	1024	/* VNC window width */
#define VW_HEIGHT	768	/* VNC window height */
#define VW_X		0	/* VNC window origin */
#define VW_Y		0	/* VNC window origin */

#define SCROLLBAR_SIZE 10
#define SCROLLBAR_BG_SIZE (SCROLLBAR_SIZE + 2)

#define INVALID_PIXEL 0xffffffff
#define COLORMAP_SIZE 256

/*
 * global data
 */
Colormap	cmap;
Display		*dpy;
Window		canvas;
GR_GC_ID	gc;
GR_GC_ID	srcGC;
GR_GC_ID	dstGC;

/* BGR233ToPixel array */
unsigned long BGR233ToPixel[COLORMAP_SIZE] = { \
	0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0xf7, 0xc7, 0x87, 0x47, 0x07, 0xc6, 0x86, 0x46, \
	0x0c, 0x4c, 0x8c, 0xcc, 0x0d, 0x4d, 0x8d, 0xcd, \
	0xcb, 0x80 \
	};

/* colour palette for 8-bit displays */
static GR_PALETTE srv_pal;	/* VNC server palette */


/* temporary keyboard mapping array */
/* ^T = up, ^F = left, ^G = right, ^V = down
 */
CARD32 kmap[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0xff51, 0xff53, \
	0xff08, 0xff09, 0x0a, 0x0b, 0x0c, 0xff0d, 0x0e, 0x0f, \
	0x10, 0x11, 0x12, 0x13, 0xff52, 0x15, 0xff54, 0x17, \
	0x18, 0x19, 0x1a, 0xff1b, 0x1c, 0x1d, 0x1e, 0x1f, \
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, \
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, \
	'0', '1', '2', '3', '4', '5', '6', '7', \
	'8', '9', 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, \
	0x40, 'A', 'B', 'C', 'D', 'E', 'F', 'G', \
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', \
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', \
	'X', 'Y', 'Z', 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, \
	0x60, 'a', 'b', 'c', 'd', 'e', 'f', 'g', \
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', \
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w', \
	'x', 'y', 'z', 0x7b, 0x7c, 0x7d, 0x7e, 0x7f };

static Display		nx_dpy;
static GR_WINDOW_ID	wid;
static int		pixtype;	/* format of pixel value */

static void CopyBGR233ToScreen(CARD8 *buf, int x, int y, int width, int height);

extern MWPIXELVAL gr_foreground;	/* for debugging only */

/*
 * Initialize graphics and open a window for the viewer
 */
Bool
CreateXWindow(void)
{
	int fd;
	GR_SIZE w, h;
	GR_SCREEN_INFO si;

	if ((fd = GrOpen()) < 0)
		return(False);
	nx_dpy.fd = fd;
	dpy = &nx_dpy;

	GrGetScreenInfo(&si);
	/* pass screen details to RFB handler */
	myFormat.bitsPerPixel = si.bpp;
	myFormat.depth = si.bpp;	/* is this right? */
	myFormat.bigEndian = 0;		/* how do I find this out? */
	myFormat.trueColour = (myFormat.depth == 8 && !useBGR233) ? 0 : 1;
	if (myFormat.trueColour) {
		myFormat.redMax = myFormat.greenMax = 7;
		myFormat.blueMax = 3;
		myFormat.redShift = 0;
		myFormat.greenShift = 3;
		myFormat.blueShift = 6;
	}
	pixtype = si.pixtype;
	/* get the initial server palette */
	GrGetSystemPalette(&srv_pal);
#if 0
	/* DEBUG */
	for (i = 0; i < srv_pal.count; i++) {
		printf("0x%02x  %03d  %03d  %03d\n", i, \
			srv_pal.palette[i].r, srv_pal.palette[i].g, \
			srv_pal.palette[i].b );
	}
#endif
	/* create the top-level window */
	w = (VW_WIDTH > (si.cols - VW_X)) ? (si.cols - VW_X) : VW_WIDTH;
	h = (VW_HEIGHT > (si.rows - VW_Y)) ? (si.rows - VW_Y) : VW_HEIGHT;
	if ((wid = GrNewWindow(GR_ROOT_WINDOW_ID, VW_X, VW_Y, w, h,
		2, LTGRAY, BLACK)) == 0) {
		fprintf(stderr, "Unable to create top-level window\n");
		GrClose();
		return False;
	}
	/* select events to receive */
	GrSelectEvents(wid, GR_EVENT_MASK_BUTTON_DOWN |
		GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_KEY_DOWN |
		GR_EVENT_MASK_KEY_UP | GR_EVENT_MASK_MOUSE_POSITION);
	/* make thw window visible */
	GrMapWindow(wid);
	canvas = wid;
	/* create the graphics contexts */
	gc = GrNewGC();
	srcGC = GrNewGC();
	dstGC = GrNewGC();

	return True;
}

/*
 * set the server palette to the requested colour
 * NOTE: this has only been tested for 8-bit colour!
 */
int
XStoreColor(Display *dpy, Colormap cmap, XColor *xc)
{
	unsigned char ind;

	ind = xc->pixel & 0xff;		/* colour map index */
	/*
	 * the colours are passed as 16-bit values so divide by 256 to
	 * get 8-bit RGB values
	 */
	srv_pal.palette[0].r = (xc->red / 256) & 0xff;
	srv_pal.palette[0].g = (xc->green / 256) & 0xff;
	srv_pal.palette[0].b = (xc->blue / 256) & 0xff;
	srv_pal.count = 1;
#if 0
	/* DEBUG */
	printf("XStoreColor: ind=%d, r=%02x, g=%02x, b=%02x\n", ind, \
		srv_pal.palette[0].r, srv_pal.palette[0].g, \
		srv_pal.palette[0].b);
#endif
	GrSetSystemPalette(ind, &srv_pal);

	return(0);
}

/*
 * Copy a rectangular block of pixels
 */
int
XCopyArea(Display *dpy, Window src, Window dst, GR_GC_ID gc,
        int x1, int y1, int w, int h, int x2, int y2)
{
/*	printf("XCopyArea: src=%d, dst=%d, w=%d, h=%d\n",src, dst, w, h); */
	GrCopyArea(dst, gc, x2, y2, w, h, src, x1, y1, MWROP_SRCCOPY);
	return(0);
}

/*
 * Fill a rectangular block
 */
int
XFillRectangle(Display *dpy, Window canvas, GR_GC_ID gc,
        int x, int y, int w, int h)
{
	GrFillRect(canvas, gc, x, y, w, h);
/*	printf("XFillRectangle: gr_foreground=%08x\n", (int)gr_foreground); */
	return(0);
}

/*
 * get the X display name
 */
char *
XDisplayName(char *display)
{
	return((char *)NULL);
}

/*
 * Change the graphics context.
 * VNC only uses this to set the foreground colour.
 */
int
XChangeGC(Display *dpy, GR_GC_ID gc, unsigned long vmask, GR_GC_INFO *gcv)
{

        /* all we need is the foreground colour */
/*	printf("XChangeGC: foreground=%08x\n", gcv->foreground); */
	if (pixtype == MWPF_PALETTE) {
		/*
		 * The MWF_PALINDEX bit tells GdFindColor() to skip the palette
		 * lookup. This is OK because we have already set the palette.
		 */
		GrSetGCForeground(gc, gcv->foreground | MWF_PALINDEX);
	} else {
		GrSetGCForeground(gc, gcv->foreground);
	}
        return(0);
}

/*
 * Ring the bell.
 */
int
XBell(Display *dpy, int pc)
{
        return(0);
}

/*
 *
 */
int
XSync(Display *dpy, Bool disc)
{
        return(0);
}

/*
 *
 */
int
XSelectInput(Display *dpy, Window win, long evmask)
{
        return(0);
}

/*
 *
 */
int
XStoreBytes(Display *dpy, char *bytes, int nbytes)
{
        return(0);
}

/*
 *
 */
int
XSetSelectionOwner(Display *dpy, Atom sel, Window own, Time t)
{
        return(0);
}

/*
 * Copy raw pixel data to the screen
 */
void
CopyDataToScreen(CARD8 *buf, int x, int y, int width, int height)
{
#if 0
	/* DEBUG */
	printf("CDTS ");
	fflush(stdout);
#endif
	if (rawDelay != 0) {
#if 0
		XFillRectangle(dpy, canvas, DefaultGC(dpy,DefaultScreen(dpy)),
		       x, y, width, height);
#endif
		XSync(dpy,False);
		usleep(rawDelay * 1000);
	}
	if (!useBGR233) {
		GrArea(canvas, gc, x, y, width, height, buf, MWPF_PALETTE);
	} else {
		CopyBGR233ToScreen(buf, x, y, width, height);
	}
}

/*
 * Copy BGR233 data to the screen.
 */
static void
CopyBGR233ToScreen(CARD8 *buf, int x, int y, int width, int height)
{

}

/*
 * Handle all X events (keyboard and mouse).
 */
Bool
HandleXEvents(GR_EVENT *ev)
{
	GR_BOOL ret = GR_TRUE;
	int buttons;

#if 0
	printf("H");
	fflush(stdout);
#endif
	switch (ev->type) {
	case GR_EVENT_TYPE_NONE:
		ret = GR_TRUE;
		break;
	case GR_EVENT_TYPE_MOUSE_POSITION:
		buttons = (ev->mouse.buttons & GR_BUTTON_R) << 2;
		buttons |= ev->mouse.buttons & GR_BUTTON_M;
		buttons |= (ev->mouse.buttons & GR_BUTTON_L) >> 2;
		ret = SendPointerEvent(ev->mouse.x, ev->mouse.y,
			buttons);
		break;
	case GR_EVENT_TYPE_BUTTON_DOWN:
	case GR_EVENT_TYPE_BUTTON_UP:
		buttons = (ev->button.buttons & GR_BUTTON_R) << 2;
		buttons |= ev->button.buttons & GR_BUTTON_M;
		buttons |= (ev->button.buttons & GR_BUTTON_L) >> 2;
		ret = SendPointerEvent(ev->button.x, ev->button.y,
			buttons);
		break;
	case GR_EVENT_TYPE_KEY_DOWN:
	case GR_EVENT_TYPE_KEY_UP:
		ret = SendKeyEvent(kmap[ev->keystroke.ch & 0x7f],
			(ev->type == GR_EVENT_TYPE_KEY_DOWN));
		break;
	default:
		break;
	}
        return(ret);
}

/*
 * Close everything down before exiting.
 */
void
ShutdownX(void)
{
	GrClose();
}

