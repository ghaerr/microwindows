#ifndef __XTONX_H
#define __XTONX_H
/*
 * Copyright (c) 2001, 2003 by Greg Haerr <greg@centurysoftware.com>
 *
 * XtoNX.h - X to Nano-X macro conversion header file
 *
 * Yes, this is a vain attempt at making things easier when
 * porting Xlib programs to Nano-X!
 */

#ifdef __cplusplus
extern "C" {
#endif

#define MWINCLUDECOLORS
#include "nano-X.h"


/* ***********************************************************************/
/* Types                                                                 */
/* ***********************************************************************/

typedef GR_CURSOR_ID		Cursor;
typedef GR_DRAW_ID		Drawable;
typedef GR_FONT_ID		Font;
typedef GR_GC_ID		GC;
typedef GR_WINDOW_ID		Pixmap;
typedef GR_REGION_ID		Region;
typedef unsigned long		Time;		/* change to GR_TIME*/
typedef GR_WINDOW_ID		Window;
typedef GR_EVENT		XEvent;
typedef GR_FONT_INFO		XFontStruct;
typedef GR_POINT		XPoint;
typedef GR_RECT			XRectangle;

/* static display structed initialized by XOpenDisplay()*/
typedef struct { 
	GR_COORD display_width;
	GR_COORD display_height;
	int	display_bpp;
} Display;

/* kluge structs*/
#define XCharStruct		char


/* ***********************************************************************/
/* Constants                                                             */
/* ***********************************************************************/

#define False			0
#define True			1

#define GXcopy			GR_MODE_COPY
#define GXxor			GR_MODE_XOR
#define None			0
#define RectangleOut		MWRECT_OUT
#define RectangleIn		MWRECT_ALLIN
#define RectanglePart		MWRECT_PARTIN

/* events*/
#define ButtonPressMask		GR_EVENT_MASK_BUTTON_DOWN
#define ButtonReleaseMask	GR_EVENT_MASK_BUTTON_UP
#define EnterWindowMask		GR_EVENT_MASK_MOUSE_ENTER
#define ExposureMask		GR_EVENT_MASK_EXPOSURE
#define KeyPressMask		GR_EVENT_MASK_KEY_DOWN
#define KeyReleaseMask		GR_EVENT_MASK_KEY_UP
#define LeaveWindowMask		GR_EVENT_MASK_MOUSE_EXIT
#define SubstructureNotifyMask	GR_EVENT_MASK_CHLD_UPDATE

#define ButtonPress		GR_EVENT_TYPE_BUTTON_DOWN
#define ButtonRelease		GR_EVENT_TYPE_BUTTON_UP
#define ConfigureNotify		GR_EVENT_TYPE_CHLD_UPDATE
#define EnterNotify		GR_EVENT_TYPE_MOUSE_ENTER
#define Expose			GR_EVENT_TYPE_EXPOSURE
#define KeyPress		GR_EVENT_TYPE_KEY_DOWN
#define KeyRelease		GR_EVENT_TYPE_KEY_UP
#define LeaveNotify		GR_EVENT_TYPE_MOUSE_EXIT


/* ***********************************************************************/
/* Functions                                                             */
/* ***********************************************************************/

#define BlackPixel(d,s)				BLACK
#define DefaultScreen(d)			0
#define RootWindow(d,s)				GR_ROOT_WINDOW_ID
#define WhitePixel(d,s)				WHITE
#define XClearArea(d,w,X,Y,W,H,e)		GrClearArea(w,X,Y,W,H,e)
#define XClearWindow(d,w)			GrClearArea(w,0,0,0,0,GR_FALSE)
#define XCloseDisplay(d)			GrClose()
#define XCopyArea(s,sw,dw,gc,sx,sy,W,H,dx,dy)	GrCopyArea(dw,gc,dx,dy,W,H,sw,sx,sy,0)
#define XCreateBitmapFromData(d,w,bm,W,H) \
			GrNewPixmapFromData(W,H,WHITE,BLACK,(void *)bm, \
				GR_BMDATA_BYTEREVERSE|GR_BMDATA_BYTESWAP)
#define XCreateGC(d,a,b,c)			GrNewGC()
#define XCreatePixmap(d,w,W,H,depth)		GrNewPixmap(W,H,(void *)0)
#define XCreatePixmapFromBitmapData(d,w,bm,W,H,f,b,depth) \
			GrNewPixmapFromData(W,H,f,b,(void *)bm, \
				GR_BMDATA_BYTEREVERSE|GR_BMDATA_BYTESWAP)
#define XCreateRegion()				GrNewRegion()
#define XCreateSimpleWindow(d,p,x,y,w,h,bw,bordc,backc) \
			GrNewWindow(p,x,y,w,h,bw,backc,bordc)
#define XCreateWindow(d,p,x,y,w,h,bw,depth,cl,vis,vm,attr) \
			GrNewWindow(p,x,y,w,h,bw,WHITE,BLACK)
#define XDefineCursor(d,w,c)			GrSetWindowCursor(w,c)
#define XDestroyRegion(r)			GrDestroyRegion(r)
#define XDestroyWindow(d,w)			GrDestroyWindow(w)
#define XDrawArc(d,w,g,x,y,W,H,a1,a2)		GrArcAngle(w,g,(x)+(W)/2,(y)+(H)/2,(W)/2,(H)/2,a1,((a1)+(a2)),GR_ARC)
#define XDrawLine(d,w,g,x1,y1,x2,y2)		GrLine(w,g,x1,y1,x2,y2)
#define XDrawLines(d,w,g,ar,cnt,B)		GrDrawLines(w,g,ar,cnt)
#define XDrawRectangle(d,w,g,x,y,W,H)		GrRect(w,g,x,y,W,H)
#define XDrawString(d,w,g,x,y,s,c)		GrText(w,g,x,y,(void *)s,c,GR_TFASCII)
#define XFillArc(d,w,g,x,y,W,H,a1,a2)		GrArcAngle(w,g,(x)+(W)/2,(y)+(H)/2,(W)/2,(H)/2,a1,((a1)+(a2)),GR_PIE)
#define XFillPolygon(d,w,gc,ar,cnt,ign1,ign2)	GrFillPoly(w,gc,cnt,ar)
#define XFillRectangle(d,w,g,x,y,W,H)		GrFillRect(w,g,x,y,W,H)
#define XFlush(d)				GrFlush()
#define XFree(p)				free(p)
#define XFreeGC(w,g)				GrDestroyGC(g)
#define XFreePixmap(d,w)			GrDestroyWindow(w)
#define XLoadFont(d,f)				GrCreateFont(0, 0, 0)
#define XLowerWindow(d,w)			GrLowerWindow(w)
#define XMapWindow(d,w)				GrMapWindow(w)
#define XMoveWindow(d,w,x,y)			GrMoveWindow(w,x,y)
#define XNextEvent(d,ep)			GrGetNextEvent(ep)
#define XQueryTree(d,w,r,p,c,nc)		GrQueryTree(w,p,c,nc)
#define XRaiseWindow(d,w)			GrRaiseWindow(w)
#define XRectInRegion(r,x,y,w,h)		GrRectInRegion(r,x,y,w,h)
#define XReparentWindow(d,w,p,x,y) 		GrReparentWindow(w,p,x,y)
#define XResizeWindow(d,w,W,H)			GrResizeWindow(w,W,H)
#define XSelectInput(d,w,m)			GrSelectEvents(w,m)
#define XSetFont(d,g,f)				GrSetGCFont(g,f)
#define XSetForeground(d,g,c)			GrSetGCForeground(g,c)
#define XSetFunction(d,g,f)			GrSetGCMode(g,f)
#define XSetIconName(d,w,n)			/* nyi*/
#define XSetRegion(d,g,c)			GrSetGCRegion(g,c)
#define XSetWindowBackground(d,w,c)		GrSetWindowBackgroundColor(w,c)
#define XSetWindowBackgroundPixmap(d,w,p)	GrSetBackgroundPixmap(w,p,GR_BACKGROUND_TILE)
#define XSetWindowBorder(d,w,c)			GrSetWindowBorderColor(w,c)
#define XSetWindowBorderWidth(d,w,bw)		GrSetWindowBorderSize(w,bw)
#define XStoreName(d,w,n) 			GrSetWindowTitle(w,n)
#define XSubtractRegion(sra,srb,dr)		GrSubtractRegion(dr,sra,srb)
#define XSync(d,f)				GrFlush()
#define XUndefineCursor(d,w)			GrSetWindowCursor(w,0)
#define XUnionRectWithRegion(rect,sr,dr)	GrUnionRectWithRegion(dr,rect)	/* FIXME*/
#define XUnmapWindow(d,w)			GrUnmapWindow(w)


/* ***********************************************************************/
/* client-statically declared functions                                  */
/* ***********************************************************************/

#define DisplayWidth(d,s)	(_display.display_width)
#define DisplayHeight(d,s)	(_display.display_height)
#define DefaultDepth(d,s)	(_display.display_bpp)

#define STATIC_FUNCTIONS			\
static Display	_display; 			\
						\
Display *					\
XOpenDisplay(char *name)			\
{						\
	GR_SCREEN_INFO	sinfo;			\
						\
	if (GrOpen() < 0)			\
		return NULL;			\
						\
	GrGetScreenInfo(&sinfo);		\
	_display.display_width = sinfo.cols;	\
	_display.display_height = sinfo.rows;	\
	_display.display_bpp = sinfo.bpp;	\
						\
	return &_display;			\
}						\
						\
int						\
XPending(Display *d)				\
{						\
	GR_EVENT ev;				\
						\
	GrFlush();				\
	return GrPeekEvent(&ev);		\
}

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif /* __XTONX_H*/
