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
 */

/*
 * vncviewer.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef NANOX
#define MWINCLUDECOLORS
#include "nano-X.h"
/* required for rfbproto.h */
typedef unsigned long  CARD32;
typedef unsigned short CARD16;
typedef unsigned char  CARD8;
#else
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#endif
#include <rfbproto.h>

#ifdef NANOX
/* try and map some X stuff to nano-X */
#define Bool		GR_BOOL
#define True		GR_TRUE
#define	False		GR_FALSE
#define XGCValues	GR_GC_INFO
 
typedef GR_ID			Window;		/* from X.h */
typedef GR_ID			Colormap;	/* from X.h */
typedef CARD32			Atom;		/* from X.h */
typedef CARD32			Time;		/* from X.h */

#define None			0L		/* from X.h */
#define CurrentTime		0L		/* from X.h */
#define PropertyChangeMask	(1L<<22)	/* from X.h */
#define XA_PRIMARY		((Atom) 1)	/* from Xatom.h */

/* Flags used in StoreNamedColor, StoreColors */
#define DoRed			(1<<0)
#define DoGreen			(1<<1)
#define DoBlue			(1<<2)
 
#define ConnectionNumber(dpy)	((dpy)->fd)
#define ScreenOfDisplay(dpy, scr)	(&(dpy)->screens[scr])
#define DefaultScreen(dpy)	((dpy)->default_screen)
#define DefaultRootWindow(dpy)	(ScreenOfDisplay(dpy,DefaultScreen(dpy))->root)
#define DefaultGC(dpy, scr)	(ScreenOfDisplay(dpy,scr)->default_gc)

/* data structure used by color operations */
typedef struct {
	unsigned long pixel;
	unsigned short red, green, blue;
	char flags;  /* do_red, do_green, do_blue */
	char pad;
} XColor;

#define GCForeground	(1L<<2)
#define GCBackground	(1L<<3)

/*
 * the Screen structure is defined in Xlib.h
 */
typedef struct {
	Window root;		/* Root window id. */
} Screen;
/*
 * the Display structure is defined as _XDisplay in Xlib.h
 */
typedef struct {
	int fd;			/* Network socket */
	int default_screen;	/* default screen for operations */
	Screen *screens;	/* pointer to list of screens */
} Display;

/*
 * the XEvent union is defined in Xlib.h
 */
typedef struct {
	int dummy;
} XEvent;

#endif	/* NANOX */


extern int endianTest;

#define Swap16IfLE(s) \
    (*(char *)&endianTest ? ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)) : (s))

#define Swap32IfLE(l) \
    (*(char *)&endianTest ? ((((l) & 0xff000000) >> 24) | \
			     (((l) & 0x00ff0000) >> 8)  | \
			     (((l) & 0x0000ff00) << 8)  | \
			     (((l) & 0x000000ff) << 24))  : (l))

#define MAX_ENCODINGS 10


/* args.c */

extern char *programName;
extern char hostname[];
extern int port;
extern Bool listenSpecified;
extern int listenPort, flashPort;
extern char *displayname;
extern Bool shareDesktop;
extern Bool viewOnly;
extern CARD32 explicitEncodings[];
extern int nExplicitEncodings;
extern Bool addCopyRect;
extern Bool addRRE;
extern Bool addCoRRE;
extern Bool addHextile;
extern Bool useBGR233;
extern Bool forceOwnCmap;
extern Bool forceTruecolour;
extern int requestedDepth;
extern char *geometry;
extern int wmDecorationWidth;
extern int wmDecorationHeight;
extern char *passwdFile;
extern int updateRequestPeriodms;
extern int updateRequestX;
extern int updateRequestY;
extern int updateRequestW;
extern int updateRequestH;
extern int rawDelay;
extern int copyRectDelay;
extern Bool debug;

extern void processArgs(int argc, char **argv);
extern void usage();


/* rfbproto.c */

extern int rfbsock;
extern Bool canUseCoRRE;
extern Bool canUseHextile;
extern char *desktopName;
extern rfbPixelFormat myFormat;
extern rfbServerInitMsg si;
extern struct timeval updateRequestTime;
extern Bool sendUpdateRequest;

extern Bool ConnectToRFBServer(const char *hostname, int port);
extern Bool InitialiseRFBConnection();
extern Bool SetFormatAndEncodings();
extern Bool SendIncrementalFramebufferUpdateRequest();
extern Bool SendFramebufferUpdateRequest(int x, int y, int w, int h,
					 Bool incremental);
extern Bool SendPointerEvent(int x, int y, int buttonMask);
extern Bool SendKeyEvent(CARD32 key, Bool down);
extern Bool SendClientCutText(char *str, int len);
extern Bool HandleRFBServerMessage();

#ifdef NANOX

/* nanox.c */

extern Display *dpy;
extern Window canvas;
extern Colormap cmap;
extern GR_GC_ID gc;
extern GR_GC_ID srcGC, dstGC;
extern unsigned long BGR233ToPixel[];

extern Bool CreateXWindow();
extern void ShutdownX();
extern Bool HandleXEvents(GR_EVENT *ev);
extern Bool AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg);
extern void CopyDataToScreen(CARD8 *buf, int x, int y, int width, int height);

/* Xlib functions */
extern char *XDisplayName(char *display);
extern int XChangeGC(Display *dpy, GR_GC_ID gc, unsigned long vmask,
	GR_GC_INFO *gcv);
extern int XStoreColor(Display *dpy, Colormap cmap, XColor *xc);
extern int XFillRectangle(Display *dpy, Window canvas, GR_GC_ID gc,
	int x, int y, int w, int h);
extern int XSync(Display *dpy, Bool discard);
extern int XCopyArea(Display *dpy, Window src, Window dst, GR_GC_ID gc, \
	int x1, int y1, int w, int h, int x2, int y2);
extern int XBell(Display *dpy, int percent);
extern int XSelectInput(Display *dpy, Window win, long evmask);
extern int XStoreBytes(Display *dpy, char *bytes, int nbytes);
extern int XSetSelectionOwner(Display *dpy, Atom sel, Window own, Time t);

#else	/* NANOX */

/* x.c */

extern Display *dpy;
extern Window canvas;
extern Colormap cmap;
extern GC gc;
extern GC srcGC, dstGC;
extern unsigned long BGR233ToPixel[];

extern Bool CreateXWindow();
extern void ShutdownX();
extern Bool HandleXEvents();
extern Bool AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg);
extern void CopyDataToScreen(CARD8 *buf, int x, int y, int width, int height);

#endif	/* NANOX */

/* sockets.c */

extern Bool errorMessageFromReadExact;

extern Bool ReadExact(int sock, char *buf, int n);
extern Bool WriteExact(int sock, char *buf, int n);
extern int ListenAtTcpPort(int port);
extern int ConnectToTcpAddr(unsigned int host, int port);
extern int AcceptTcpConnection(int listenSock);
extern int StringToIPAddr(const char *str, unsigned int *addr);
extern Bool SameMachine(int sock);


/* listen.c */

extern void listenForIncomingConnections();
