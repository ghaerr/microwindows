#ifndef	_SERV_H
#define	_SERV_H
/*
 * Copyright (c) 2000, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Private definitions for the graphics server.
 * These definitions are not to be used by clients.
 */

#include "mwsystem.h"


#if defined(__ECOS) && !defined(_NO_SVR_MAPPING)
/*
 * Since eCos is a single task, multi-threaded environment, the
 * server and the client code share the same namespace.  This means
 * that server functions which are represented by dispatchers in
 * the client code need to have unique names, thus this remapping.
 */
#define nxErrorStrings		SVR_nxErrorStrings
#define GrArcAngle              SVR_GrArcAngle
#define GrArc                   SVR_GrArc
#define GrArea                  SVR_GrArea
#define GrBell                  SVR_GrBell
#define GrBitmap                SVR_GrBitmap
#define GrCheckNextEvent        SVR_GrCheckNextEvent
#define GrClearArea             SVR_GrClearArea
#define GrClose                 SVR_GrClose
#define GrCloseWindow           SVR_GrCloseWindow
#define GrCopyArea              SVR_GrCopyArea
#define GrCopyGC                SVR_GrCopyGC
#define GrCreateFont            SVR_GrCreateFont
#define GrCreateTimer		SVR_GrCreateTimer        
#define GrDelay			SVR_GrDelay
#define GrDestroyCursor		SVR_GrDestroyCursor      
#define GrDestroyFont           SVR_GrDestroyFont
#define GrDestroyGC             SVR_GrDestroyGC
#define GrDestroyRegion         SVR_GrDestroyRegion
#define GrDestroyTimer		SVR_GrDestroyTimer       
#define GrDestroyWindow         SVR_GrDestroyWindow
#define GrDrawImageBits         SVR_GrDrawImageBits
#define GrDrawImageFromBuffer	SVR_GrDrawImageFromBuffer
#define GrDrawImageFromFile     SVR_GrDrawImageFromFile
#define GrDrawImageToFit        SVR_GrDrawImageToFit
#define GrEllipse               SVR_GrEllipse
#define GrEmptyRegion           SVR_GrEmptyRegion
#define GrEqualRegion           SVR_GrEqualRegion
#define GrFillEllipse           SVR_GrFillEllipse
#define GrFillPoly              SVR_GrFillPoly
#define GrFillRect              SVR_GrFillRect
#define GrFindColor             SVR_GrFindColor
#define GrFreeFontList		SVR_GrFreeFontList       
#define GrFreeImage             SVR_GrFreeImage
#define GrGetFocus              SVR_GrGetFocus
#define GrGetFontInfo           SVR_GrGetFontInfo
#define GrGetFontList		SVR_GrGetFontList        
#define GrGetGCInfo             SVR_GrGetGCInfo
#define GrGetGCTextSize         SVR_GrGetGCTextSize
#define GrGetImageInfo          SVR_GrGetImageInfo
#define GrGetRegionBox          SVR_GrGetRegionBox
#define GrGetScreenInfo         SVR_GrGetScreenInfo
#define GrGetSelectionOwner     SVR_GrGetSelectionOwner
#define GrGetSysColor           SVR_GrGetSysColor
#define GrGetSystemPalette      SVR_GrGetSystemPalette
#define GrGetWindowInfo         SVR_GrGetWindowInfo
#define GrGetWMProperties       SVR_GrGetWMProperties
#define GrInjectKeyboardEvent   SVR_GrInjectKeyboardEvent
#define GrInjectPointerEvent    SVR_GrInjectPointerEvent
#define GrIntersectRegion       SVR_GrIntersectRegion
#define GrKillWindow            SVR_GrKillWindow
#define GrLine                  SVR_GrLine
#define GrLoadImageFromFile     SVR_GrLoadImageFromFile
#define GrLowerWindow           SVR_GrLowerWindow
#define GrMapWindow             SVR_GrMapWindow
#define GrMoveCursor            SVR_GrMoveCursor
#define GrMoveWindow            SVR_GrMoveWindow
#define GrNewCursor		SVR_GrNewCursor          
#define GrNewGC                 SVR_GrNewGC
#define GrNewInputWindow        SVR_GrNewInputWindow
#define GrNewPixmap             SVR_GrNewPixmap
#define GrNewPolygonRegion      SVR_GrNewPolygonRegion
#define GrNewRegion             SVR_GrNewRegion
#define GrNewWindow             SVR_GrNewWindow
#define GrOffsetRegion          SVR_GrOffsetRegion
#define GrOpen                  SVR_GrOpen
#define GrPeekEvent             SVR_GrPeekEvent
#define GrPointInRegion         SVR_GrPointInRegion
#define GrPoints                SVR_GrPoints
#define GrPoint                 SVR_GrPoint
#define GrPoly                  SVR_GrPoly
#define GrQueryTree		SVR_GrQueryTree          
#define GrRaiseWindow           SVR_GrRaiseWindow
#define GrReadArea              SVR_GrReadArea
#define GrRectInRegion          SVR_GrRectInRegion
#define GrRect                  SVR_GrRect
#define GrReparentWindow        SVR_GrReparentWindow
#define GrRequestClientData     SVR_GrRequestClientData
#define GrResizeWindow          SVR_GrResizeWindow
#define GrSelectEvents          SVR_GrSelectEvents
#define GrSendClientData        SVR_GrSendClientData
#define GrSetBackgroundPixmap   SVR_GrSetBackgroundPixmap
#define GrSetCursor             SVR_GrSetCursor
#define GrSetFocus              SVR_GrSetFocus
#define GrSetFontAttr           SVR_GrSetFontAttr
#define GrSetFontRotation       SVR_GrSetFontRotation
#define GrSetFontSize           SVR_GrSetFontSize
#define GrSetGCBackground       SVR_GrSetGCBackground
#define GrSetGCClipOrigin	SVR_GrSetGCClipOrigin
#define GrSetGCFont             SVR_GrSetGCFont
#define GrSetGCForeground       SVR_GrSetGCForeground
#define GrSetGCGraphicsExposure	SVR_GrSetGCGraphicsExposure
#define GrSetGCMode             SVR_GrSetGCMode
#define GrSetGCRegion           SVR_GrSetGCRegion
#define GrSetGCUseBackground    SVR_GrSetGCUseBackground
#define GrSetPortraitMode	SVR_GrSetPortraitMode
#define GrSetScreenSaverTimeout SVR_GrSetScreenSaverTimeout
#define GrSetSelectionOwner     SVR_GrSetSelectionOwner
#define GrSetSystemPalette      SVR_GrSetSystemPalette
#define GrSetWindowCursor	SVR_GrSetWindowCursor    
#define GrSetWMProperties       SVR_GrSetWMProperties
#define GrSubtractRegion        SVR_GrSubtractRegion
#define GrText                  SVR_GrText
#define GrUnionRectWithRegion   SVR_GrUnionRectWithRegion
#define GrUnionRegion           SVR_GrUnionRegion
#define GrUnmapWindow           SVR_GrUnmapWindow
#define GrXorRegion             SVR_GrXorRegion
#endif

#include "nano-X.h"
#include "device.h"


/*
 * Define the server-side mutex code.  This is a regular mutex (as defined
 * in lock.h) for linked-in mode, or a no-op for client/server mode.
 */

#if NONETWORK
/* Use a server-side mutex. */

#include "lock.h"

LOCK_EXTERN(gr_server_mutex);

#define SERVER_LOCK_DECLARE   LOCK_DECLARE(gr_server_mutex);
#define SERVER_LOCK_INIT()    LOCK_INIT(&gr_server_mutex)
#define SERVER_LOCK()         LOCK(&gr_server_mutex)
#define SERVER_UNLOCK()       UNLOCK(&gr_server_mutex)

#else /* !NONETWORK */
/* The Nano-X server is single threaded, so disable the server-side mutex (for speed). */

#define SERVER_LOCK_DECLARE /* no-op */
#define SERVER_LOCK_INIT()  do {} while(0) /* no-op, but require a ";" */
#define SERVER_LOCK()       do {} while(0) /* no-op, but require a ";" */
#define SERVER_UNLOCK()     do {} while(0) /* no-op, but require a ";" */

#endif /* !NONETWORK*/


/*
 * Drawing types.
 */
typedef	int	GR_DRAW_TYPE;

#define	GR_DRAW_TYPE_NONE	0	/* none or error */
#define	GR_DRAW_TYPE_WINDOW	1	/* windows */
#define	GR_DRAW_TYPE_PIXMAP	2	/* pixmaps */

#define	GR_MAX_MODE		MWMODE_MAX
/*
 * List of elements for events.
 */
typedef	struct gr_event_list GR_EVENT_LIST;
struct gr_event_list {
	GR_EVENT_LIST	*next;		/* next element in list */
	GR_EVENT	event;		/* event */
};

/*
 * Data structure to keep track of state of clients.
 */
typedef struct gr_client GR_CLIENT;
struct gr_client {
	int		id;		/* client id and socket descriptor */
	GR_EVENT_LIST	*eventhead;	/* head of event chain (or NULL) */
	GR_EVENT_LIST	*eventtail;	/* tail of event chain (or NULL) */
	GR_CLIENT	*next;		/* the next client in the list */
	GR_CLIENT	*prev;		/* the previous client in the list */
	int		waiting_for_event; /* used to implement GrGetNextEvent*/
	char		*shm_cmds;
	int		shm_cmds_size;
	int		shm_cmds_shmid;
	unsigned long	processid;	/* client process id*/
};

/*
 * Structure to remember clients associated with events.
 */
typedef	struct gr_event_client	GR_EVENT_CLIENT;
struct gr_event_client	{
	GR_EVENT_CLIENT	*next;		/* next interested client or NULL */
	GR_EVENT_MASK	eventmask;	/* events client wants to see */
	GR_CLIENT	*client;	/* client who is interested */
};

/*
 * Structure to remember graphics contexts.
 */
typedef	struct gr_gc	GR_GC;
struct gr_gc {
	GR_GC_ID	id;		/* graphics context id */
	int		mode;		/* drawing mode */
	GR_REGION_ID	regionid;	/* current clipping region */
 	GR_COORD	xoff;           /* X offset for the clip region */
	GR_COORD	yoff;           /* Y offset for the clip region */
	GR_FONT_ID	fontid;		/* current font id*/
	GR_COLOR	foreground;	/* foreground RGB color or pixel value*/
	GR_COLOR	background;	/* background RGB color or pixel value*/
	GR_BOOL		fgispixelval;	/* TRUE if 'foreground' is actually a GR_PIXELVAL */
	GR_BOOL		bgispixelval;	/* TRUE if 'background' is actually a GR_PIXELVAL */
	GR_BOOL		usebackground;	/* actually display the background */
        GR_BOOL		exposure;     	/* send expose events on GrCopyArea */

        int             linestyle;	/* GR_LINE_SOLID, GR_LINE_ONOFF_DASH */
        unsigned long   dashmask;
        char            dashcount;
   
        int             fillmode;	/* GR_FILL_SOLID, STIPPLE, OPAQUE_STIPPLE, TILE */
        GR_STIPPLE      stipple;	/* width,height,bitmap*/
        struct {
		PSD psd;
		GR_SIZE width;
		GR_SIZE height;
	} tile;
        GR_POINT        ts_offset;

	GR_BOOL		changed;	/* graphics context has been changed */
	GR_CLIENT 	*owner;		/* client that created it */
	GR_GC		*next;		/* next graphics context */
};

/*
 * Structure to remember regions.
 */
typedef struct gr_region	GR_REGION;
struct gr_region {
	MWCLIPREGION *	rgn;
	GR_REGION_ID	id;
	GR_CLIENT *	owner;		/* client that created it */
	GR_REGION *	next;
};
 
/*
 * Structure to remember fonts.
 */
typedef struct gr_font	GR_FONT;
struct gr_font {
	PMWFONT		pfont;		/* font*/
	int		fontsize;	/* font size*/
	GR_FONT_ID	id;		/* font id*/
	GR_CLIENT *	owner;		/* client that created it */
	GR_FONT *	next;		/* next font*/
};

/*
 * Cursor structure.
 */
typedef struct gr_cursor GR_CURSOR;
struct gr_cursor {
	GR_CURSOR_ID	id;		/* cursor id*/
	GR_CLIENT	*owner;		/* client that created it*/
	GR_CURSOR	*next;
	MWCURSOR	cursor;		/* mwin engine cursor structure*/
};

/*
 * Structure to remember selection owner and mime types it can produce.
 */
typedef struct {
	GR_WINDOW_ID wid;
	GR_CHAR *typelist;
} GR_SELECTIONOWNER;


#if MW_FEATURE_IMAGES
/*
 * Structure to remember images.
 */
typedef struct gr_image	GR_IMAGE;
struct gr_image {
	GR_IMAGE_ID	id;
	GR_CLIENT *	owner;		/* client that created it */
	GR_IMAGE *	next;
};
#endif /* MW_FEATURE_IMAGES */
 
#if MW_FEATURE_TIMERS
/*
 * Structure to remember timers.
 */
typedef struct gr_timer	GR_TIMER;
struct gr_timer 
{
    GR_TIMER_ID    id;       /* This instances ID */
    GR_CLIENT     *owner;    /* client that created it */
    GR_WINDOW_ID   wid;
    MWTIMER       *timer;    /* Device independent layer timer */
    GR_TIMER      *next;
};
#endif /* MW_FEATURE_TIMERS */

/*
 * Drawable structure.  This structure must be the first
 * elements in a GR_WINDOW or GR_PIXMAP, as GrPrepareWindow
 * returns a GR_DRAWABLE.  This structure includes
 * only those items that routines that use GrPrepareWindow
 * might dereference, and must be included in windows or pixmaps.
 */
typedef struct gr_drawable {
	GR_COORD	x;		/* x position (0)*/
	GR_COORD	y;		/* y position (0)*/
	GR_SIZE		width;		/* width */
	GR_SIZE		height;		/* height */
        struct _mwscreendevice *psd;    /* associated screen device */
	GR_WINDOW_ID	id;		/* window/pixmap id */
} GR_DRAWABLE;

/*
 * Window structure
 * Note: first elements must match GR_DRAWABLE
 */
typedef struct gr_pixmap GR_PIXMAP;
typedef struct gr_window GR_WINDOW;
struct gr_window {
	GR_COORD	x;		/* absolute x position */
	GR_COORD	y;		/* absolute y position */
	GR_SIZE		width;		/* width */
	GR_SIZE		height;		/* height */
        struct _mwscreendevice *psd;    /* associated screen device */
	GR_WINDOW_ID	id;		/* window id */
	/* end of GR_DRAWABLE common members*/

	GR_WINDOW	*next;		/* next window in complete list */
	GR_CLIENT	*owner;		/* client that created it */
	GR_WINDOW	*parent;	/* parent window */
	GR_WINDOW	*children;	/* first child window */
	GR_WINDOW	*siblings;	/* next sibling window */
	GR_SIZE		bordersize;	/* size of border */
	GR_COLOR	bordercolor;	/* color of border */
	GR_COLOR	background;	/* background color */
	GR_PIXMAP	*bgpixmap;	/* background pixmap */
	int		bgpixmapflags;	/* center, tile etc. */
	GR_EVENT_MASK	nopropmask;	/* events not to be propagated */
	GR_EVENT_CLIENT	*eventclients;	/* clients interested in events */
	GR_CURSOR_ID	cursorid;	/* cursor for this window */
	GR_BOOL		mapped;		/* TRUE means requested to be mapped */
	GR_BOOL		realized;	/* TRUE means window is visible */
	GR_BOOL		output;		/* TRUE if window can do output */
	GR_WM_PROPS	props;		/* window properties*/
	GR_CHAR		*title;		/* window title*/
	MWCLIPREGION	*clipregion;	/* window clipping region */
};

/*
 * Pixmap structure
 * Note: first elements must match GR_DRAWABLE
 */
struct gr_pixmap {
	GR_COORD	x;		/* x position (0)*/
	GR_COORD	y;		/* y position (0)*/
	GR_SIZE		width;		/* width */
	GR_SIZE		height;		/* height */
        struct _mwscreendevice *psd;    /* associated screen device */
	GR_WINDOW_ID	id;		/* pixmap id */
	/* end of GR_DRAWABLE common members*/

	GR_PIXMAP	*next;		/* next pixmap in list */
	GR_CLIENT	*owner;		/* client that created it */
};

/**
 * Structure to remember grabbed keys.
 */
typedef struct gr_grabbed_key GR_GRABBED_KEY;
struct gr_grabbed_key {
	GR_GRABBED_KEY	*next;	/**< Next entry in the linked list of all key grabs. */
	GR_CLIENT	*owner;	/**< Client to send hotkey events to. */
	int		type;	/**< The type parameter passed to GrGrabKey(). */
	GR_WINDOW_ID	wid;	/**< Window to send events to. */
	GR_KEY		key;	/**< 16-bit unicode key value, MWKEY_xxx. */
};

/*
 * Macros to obtain the client number from a resource id, and to
 * produce the first resource id to be used for a client number.
 * Client numbers must not be zero.  This allows for 255 clients.
#define	GR_ID_CLIENT(n)	(((GR_ID) (n)) >> 24)
#define	GR_ID_BASE(n)	(((GR_ID) (n)) << 24)
 */

/*
 * Graphics server routines.
 */
int		GsInitialize(void);
void		GsClose(int fd);
void		GsSelect(GR_TIMEOUT timeout);
void		GsTerminate(void);
GR_TIMEOUT	GsGetTickCount(void);
void		GsRedrawScreen(void);
void		GsError(GR_ERROR code, GR_ID id);
GR_BOOL		GsCheckMouseEvent(void);
GR_BOOL		GsCheckKeyboardEvent(void);
int		GsReadKeyboard(GR_CHAR *buf, int *modifiers);
int		GsOpenKeyboard(void);
void		GsGetButtonInfo(int *buttons);
void		GsGetModifierInfo(int *modifiers);
void		GsCloseKeyboard(void);
void		GsExposeArea(GR_WINDOW *wp, GR_COORD rootx, GR_COORD rooty,
			GR_SIZE width, GR_SIZE height, GR_WINDOW *stopwp);
void		GsCheckCursor(void);
void		GsWpNotifyActivate(GR_WINDOW *wp);
void		GsWpSetFocus(GR_WINDOW *wp);
void		GsWpDrawBackgroundPixmap(GR_WINDOW *wp, GR_PIXMAP *pm,
			GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height);
void		GsWpTileBackgroundPixmap(GR_WINDOW *wp, GR_PIXMAP *pm,
			GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height);
void		GsWpClearWindow(GR_WINDOW *wp, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_BOOL exposeflag);
void		GsWpUnrealizeWindow(GR_WINDOW *wp, GR_BOOL temp_unmap);
void		GsWpRealizeWindow(GR_WINDOW *wp, GR_BOOL temp);
void		GsWpDestroyWindow(GR_WINDOW *wp);
void		GsSetPortraitMode(int mode);
void		GsSetPortraitModeFromXY(GR_COORD rootx, GR_COORD rooty);
void		GsSetClipWindow(GR_WINDOW *wp, MWCLIPREGION *userregion,
			int flags);
void		GsHandleMouseStatus(GR_COORD newx, GR_COORD newy,
			int newbuttons);
void		GsFreePositionEvent(GR_CLIENT *client, GR_WINDOW_ID wid,
			GR_WINDOW_ID subwid);
void		GsDeliverButtonEvent(GR_EVENT_TYPE type, int buttons,
			int changebuttons, int modifiers);
void		GsDeliverMotionEvent(GR_EVENT_TYPE type, int buttons,
			MWKEYMOD modifiers);
void		GsDeliverKeyboardEvent(GR_WINDOW_ID wid, GR_EVENT_TYPE type,
			GR_KEY keyvalue, GR_KEYMOD modifiers,
			GR_SCANCODE scancode);
void		GsDeliverExposureEvent(GR_WINDOW *wp, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
void		GsFreeExposureEvent(GR_CLIENT *client, GR_WINDOW_ID wid,
			GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height);
void		GsDeliverUpdateEvent(GR_WINDOW *wp, GR_UPDATE_TYPE utype,
			GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height);
void		GsDeliverGeneralEvent(GR_WINDOW *wp, GR_EVENT_TYPE type,
			GR_WINDOW *other);
void		GsDeliverPortraitChangedEvent(void);
void		GsDeliverScreenSaverEvent(GR_BOOL activate);
void		GsDeliverRawMouseEvent(int x, int y, int buttons, int);
void		GsDeliverClientDataReqEvent(GR_WINDOW_ID wid, GR_WINDOW_ID rid,
			GR_SERIALNO serial, GR_MIMETYPE mimetype);
void		GsDeliverClientDataEvent(GR_WINDOW_ID wid, GR_WINDOW_ID rid,
			GR_SERIALNO serial, GR_LENGTH len, GR_LENGTH thislen,
			void *data);
void		GsDeliverSelectionChangedEvent(GR_WINDOW_ID old_owner,
			GR_WINDOW_ID new_owner);
#if MW_FEATURE_TIMERS
void		GsDeliverTimerEvent(GR_CLIENT *client, GR_WINDOW_ID wid,
			GR_TIMER_ID tid);
GR_TIMER	*GsFindTimer(GR_TIMER_ID timer_id);
void            GsTimerCB(void *arg);
#endif /* MW_FEATURE_TIMERS */

void		GsCheckMouseWindow(void);
void		GsCheckFocusWindow(void);
GR_DRAW_TYPE	GsPrepareDrawing(GR_DRAW_ID id, GR_GC_ID gcid,
			GR_DRAWABLE **retdp);
GR_BOOL		GsCheckOverlap(GR_WINDOW *topwp, GR_WINDOW *botwp);
GR_EVENT	*GsAllocEvent(GR_CLIENT *client);
GR_WINDOW	*GsFindWindow(GR_WINDOW_ID id);
GR_PIXMAP 	*GsFindPixmap(GR_WINDOW_ID id);
GR_GC		*GsFindGC(GR_GC_ID gcid);
GR_REGION	*GsFindRegion(GR_REGION_ID regionid);
GR_FONT 	*GsFindFont(GR_FONT_ID fontid);
GR_CURSOR 	*GsFindCursor(GR_CURSOR_ID cursorid);
GR_WINDOW	*GsPrepareWindow(GR_WINDOW_ID wid);
GR_WINDOW	*GsFindVisibleWindow(GR_COORD x, GR_COORD y);
void		GsDrawBorder(GR_WINDOW *wp);
int		GsCurrentVt(void);
void		GsRedrawVt(int t);
int		GsOpenSocket(void);
void		GsCloseSocket(void);
void		GsAcceptClient(void);
void		GsAcceptClientFd(int i);
int		GsPutCh(int fd, unsigned char c);
GR_CLIENT	*GsFindClient(int fd);
void		GsDestroyClientResources(GR_CLIENT * client);
void		GsDropClient(int fd);
int		GsRead(int fd, void *buf, int c);
int		GsWrite(int fd, void *buf, int c);
void		GsHandleClient(int fd);
void		GsResetScreenSaver(void);
void		GsActivateScreenSaver(void *arg);
void		GrGetNextEventWrapperFinish(int);

/*
 * External data definitions.
 */
extern	char *		curfunc;		/* current function name */
extern	GR_WINDOW_ID	cachewindowid;		/* cached window id */
extern  GR_WINDOW_ID    cachepixmapid;
extern	GR_GC_ID	cachegcid;		/* cached graphics context id */
extern	GR_GC		*cachegcp;		/* cached graphics context */
extern	GR_GC		*listgcp;		/* list of all gc */
extern	GR_REGION	*listregionp;		/* list of all regions */
extern	GR_FONT		*listfontp;		/* list of all fonts */
#if MW_FEATURE_IMAGES
extern	GR_IMAGE	*listimagep;		/* list of all images */
#endif
extern	GR_CURSOR	*listcursorp;		/* list of all cursors */
extern	GR_CURSOR	*stdcursor;		/* root window cursor */
extern	GR_GC		*curgcp;		/* current graphics context */
extern	GR_WINDOW	*cachewp;		/* cached window pointer */
extern  GR_PIXMAP       *cachepp;		/* cached pixmap pointer */
extern	GR_WINDOW	*listwp;		/* list of all windows */
extern	GR_PIXMAP	*listpp;		/* list of all pixmaps */
extern	GR_WINDOW	*rootwp;		/* root window pointer */
extern	GR_WINDOW	*clipwp;		/* window clipping is set for */
extern	GR_WINDOW	*focuswp;		/* focus window for keyboard */
extern	GR_WINDOW	*mousewp;		/* window mouse is currently in */
extern	GR_WINDOW	*grabbuttonwp;		/* window grabbed by button */
extern	GR_CURSOR	*curcursor;		/* currently enabled cursor */
extern	GR_COORD	cursorx;		/* x position of cursor */
extern	GR_COORD	cursory;		/* y position of cursor */
extern	GR_BUTTON	curbuttons;		/* current state of buttons */
extern	GR_CLIENT	*curclient;		/* current client */
extern	char		*current_shm_cmds;
extern	int		current_shm_cmds_size;
extern	GR_EVENT_LIST	*eventfree;		/* list of free events */
extern	GR_BOOL		focusfixed;		/* TRUE if focus is fixed */
extern	GR_SCREEN_INFO	sinfo;			/* screen information */
extern	PMWFONT		stdfont;		/* default font*/
extern	int		escape_quits;		/* terminate when pressing ESC*/
extern	int		connectcount;		/* # of connections to server */
#if MW_FEATURE_TIMERS
extern	GR_TIMEOUT	screensaver_delay;	/* time before screensaver */
						/* activates */
extern  GR_TIMER_ID     cache_timer_id;         /* cached timer ID */
extern  GR_TIMER        *cache_timer;           /* cached timer */
extern  GR_TIMER        *list_timer;            /* list of all timers */
#endif /* MW_FEATURE_TIMERS */
extern  GR_GRABBED_KEY  *list_grabbed_keys;

extern	GR_BOOL		screensaver_active;	/* screensaver is active */
extern	GR_SELECTIONOWNER selection_owner;	/* the selection owner */
extern  int		autoportrait;		/* auto portrait mode switching*/
extern  MWCOORD		nxres;			/* requested server x res*/
extern  MWCOORD		nyres;			/* requested server y res*/

/*
 * The filename to use for the named socket.  The environment variable
 * NXDISPLAY will override GR_NAMED_SOCKET for the AF_UNIX case, or
 * specify the nano-X server address in the AF_INET case (default 127.0.0.1)
 */
#define GR_NAMED_SOCKET	"/tmp/.nano-X"		/* AF_UNIX socket name*/
#define GR_NUM_SOCKET	6600			/* AF_INET socket number*/
#define GR_ELKS_SOCKET	79			/* AF_NANO socket number*/

#if VTSWITCH
/* temp framebuffer vt switch stuff at upper level
 * this should be handled at the lower level, just like vgalib does.
 */
void MwInitVt(void);
int  MwCurrentVt(void);
int  MwCheckVtChange(void);
void MwRedrawVt(int t);
void MwExitVt(void);
extern int mwvterm;
#endif /* VTSWITCH*/

#endif /* _SERV_H*/
