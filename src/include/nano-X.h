#ifndef	_NANO_X_H
#define	_NANO_X_H
/* Copyright (c) 1999, 2000, 2001, 2002, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 * Copyright (c) 1991 David I. Bell
 *
 * Nano-X public definition header file:  user applications should
 * include only this header file.
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "mwtypes.h"			/* exported engine MW* types*/

/*
 * The following typedefs are inherited from the Microwindows
 * engine layer.
 */
typedef MWCOORD 	GR_COORD;	/* coordinate value */
typedef MWCOORD 	GR_SIZE;	/* size value */
typedef MWCOLORVAL 	GR_COLOR;	/* full color value */
typedef MWPIXELVAL 	GR_PIXELVAL;	/* hw pixel value */
typedef MWIMAGEBITS 	GR_BITMAP;	/* bitmap unit */
typedef MWUCHAR 	GR_CHAR;	/* filename, window title */
typedef MWTEXTFLAGS	GR_TEXTFLAGS;	/* text encoding flags */
typedef MWKEY	 	GR_KEY;		/* keystroke value */
typedef MWSCANCODE	GR_SCANCODE;	/* oem keystroke scancode value */
typedef MWKEYMOD	GR_KEYMOD;	/* keystroke modifiers */
typedef MWSCREENINFO	GR_SCREEN_INFO;	/* screen information */
typedef MWWINDOWFBINFO	GR_WINDOW_FB_INFO; /* direct client-mapped window info */
typedef MWFONTINFO	GR_FONT_INFO;	/* font information */
typedef MWIMAGEINFO	GR_IMAGE_INFO;	/* image information */
typedef MWIMAGEHDR	GR_IMAGE_HDR;	/* multicolor image representation */
typedef MWLOGFONT	GR_LOGFONT;	/* logical font descriptor */
typedef MWPALENTRY	GR_PALENTRY;	/* palette entry */
typedef MWPOINT		GR_POINT;	/* definition of a point */
typedef MWTIMEOUT	GR_TIMEOUT;	/* timeout value */
typedef MWFONTLIST	GR_FONTLIST;	/* list of fonts */
typedef MWKBINFO	GR_KBINFO;	/* keyboard information  */
typedef MWSTIPPLE       GR_STIPPLE;     /* Stipple information   */
typedef MWTRANSFORM     GR_TRANSFORM;   /* Transform information */

/* Basic typedefs. */
typedef int 		GR_COUNT;	/* number of items */
typedef unsigned char	GR_CHAR_WIDTH;	/* width of character */
typedef unsigned int	GR_ID;		/* resource ids */
typedef GR_ID		GR_DRAW_ID;	/* drawable id */
typedef GR_DRAW_ID	GR_WINDOW_ID;	/* window or pixmap id */
typedef GR_ID		GR_GC_ID;	/* graphics context id */
typedef GR_ID		GR_REGION_ID;	/* region id */
typedef GR_ID		GR_FONT_ID;	/* font id */
typedef GR_ID		GR_IMAGE_ID;	/* image id */
typedef GR_ID		GR_TIMER_ID;	/* timer id */
typedef GR_ID		GR_CURSOR_ID;	/* cursor id */
typedef unsigned short	GR_BOOL;	/* boolean value */
typedef int		GR_ERROR;	/* error types */
typedef int		GR_EVENT_TYPE;	/* event types */
typedef int		GR_UPDATE_TYPE;	/* window update types */
typedef uint32_t	GR_EVENT_MASK;	/* event masks */
typedef char		GR_FUNC_NAME[25];/* function name */
typedef uint32_t	GR_WM_PROPS;	/* window property flags */
typedef uint32_t	GR_SERIALNO;	/* Selection request ID number */
typedef unsigned short	GR_MIMETYPE;	/* Index into mime type list */
typedef uint32_t	GR_LENGTH;	/* Length of a block of data */
typedef unsigned int	GR_BUTTON;	/* mouse button value */

/** Nano-X rectangle, different from MWRECT */
typedef struct {
	GR_COORD x;		/**< upper left x coordinate*/
	GR_COORD y;		/**< upper left y coordinate*/
	GR_SIZE  width;		/**< rectangle width*/
	GR_SIZE  height;	/**< rectangle height*/
} GR_RECT;

/* The root window id. */
#define	GR_ROOT_WINDOW_ID	((GR_WINDOW_ID) 1)

/* GR_COLOR color constructor */
#define GR_RGB(r,g,b)		MWRGB(r,g,b)
#define GR_ARGB(a,r,g,b)	MWARGB(a,r,g,b)

/* Drawing modes for GrSetGCMode */
#define	GR_MODE_COPY		MWROP_COPY			/* src*/
#define	GR_MODE_XOR			MWROP_XOR			/* src ^ dst*/
#define	GR_MODE_OR			MWROP_OR			/* src | dst*/
#define	GR_MODE_AND			MWROP_AND			/* src & dst*/
#define	GR_MODE_CLEAR 		MWROP_CLEAR			/* 0*/
#define	GR_MODE_SET			MWROP_SET			/* 11111111*/
#define	GR_MODE_EQUIV		MWROP_EQUIV			/* ~(src ^ dst)*/
#define	GR_MODE_NOR			MWROP_NOR			/* ~(src | dst)*/
#define	GR_MODE_NAND		MWROP_NAND			/* ~(src & dst)*/
#define	GR_MODE_INVERT		MWROP_INVERT		/* ~dst*/
#define	GR_MODE_COPYINVERTED MWROP_COPYINVERTED	/* ~src*/
#define	GR_MODE_ORINVERTED	MWROP_ORINVERTED	/* ~src | dst*/
#define	GR_MODE_ANDINVERTED	MWROP_ANDINVERTED	/* ~src & dst*/
#define GR_MODE_ORREVERSE	MWROP_ORREVERSE		/* src | ~dst*/
#define	GR_MODE_ANDREVERSE	MWROP_ANDREVERSE	/* src & ~dst*/
#define	GR_MODE_NOOP		MWROP_NOOP			/* dst*/

#define GR_MODE_DRAWMASK	0x00FF
#define GR_MODE_EXCLUDECHILDREN	0x0100		/* exclude children on clip*/

/* Line modes */
#define GR_LINE_SOLID           MWLINE_SOLID
#define GR_LINE_ONOFF_DASH      MWLINE_ONOFF_DASH

#define GR_FILL_SOLID           MWFILL_SOLID
#define GR_FILL_STIPPLE         MWFILL_STIPPLE
#define GR_FILL_OPAQUE_STIPPLE  MWFILL_OPAQUE_STIPPLE
#define GR_FILL_TILE            MWFILL_TILE

/* Polygon regions*/
#define GR_POLY_EVENODD		MWPOLY_EVENODD
#define GR_POLY_WINDING		MWPOLY_WINDING

/* builtin font std names*/
#define GR_FONT_SYSTEM_VAR	MWFONT_SYSTEM_VAR
#define GR_FONT_SYSTEM_FIXED	MWFONT_SYSTEM_FIXED
#define GR_FONT_GUI_VAR		MWFONT_GUI_VAR		/* deprecated*/
#define GR_FONT_OEM_FIXED	MWFONT_OEM_FIXED	/* deprecated*/

/* GrText/GrGetTextSize encoding flags*/
#define GR_TFASCII		MWTF_ASCII
#define GR_TFUTF8		MWTF_UTF8
#define GR_TFUC16		MWTF_UC16
#define GR_TFUC32		MWTF_UC32
#define GR_TFXCHAR2B		MWTF_XCHAR2B
#define GR_TFPACKMASK		MWTF_PACKMASK

/* GrText alignment flags*/
#define GR_TFTOP		MWTF_TOP
#define GR_TFBASELINE		MWTF_BASELINE
#define GR_TFBOTTOM		MWTF_BOTTOM

/* GrSetFontAttr flags*/
#define GR_TFKERNING		MWTF_KERNING
#define GR_TFANTIALIAS		MWTF_ANTIALIAS
#define GR_TFUNDERLINE		MWTF_UNDERLINE

/* GrArc, GrArcAngle types*/
#define GR_ARC		MWARC		/* arc only*/
#define GR_ARCOUTLINE	MWARCOUTLINE	/* arc + outline*/
#define GR_PIE		MWPIE		/* pie (filled)*/

/* GrSetWindowRegion types*/
#define GR_WINDOW_BOUNDING_MASK	0	/* outer border*/
#define GR_WINDOW_CLIP_MASK	1	/* inner border*/

/* Booleans */
#define	GR_FALSE		0
#define	GR_TRUE			1

/* Loadable Image support definition */
#define GR_IMAGE_MAX_SIZE	(-1)

/* Button flags */
#define	GR_BUTTON_R		MWBUTTON_R 	/* right button*/
#define	GR_BUTTON_M		MWBUTTON_M	/* middle button*/
#define	GR_BUTTON_L		MWBUTTON_L	/* left button */
#define	GR_BUTTON_ANY		(MWBUTTON_R|MWBUTTON_M|MWBUTTON_L) /* any*/

/* GrSetBackgroundPixmap flags */
#define GR_BACKGROUND_TILE	0	/* Tile across the window */
#define GR_BACKGROUND_CENTER	1	/* Draw in center of window */
#define GR_BACKGROUND_TOPLEFT	2	/* Draw at top left of window */
#define GR_BACKGROUND_STRETCH	4	/* Stretch image to fit window*/
#define GR_BACKGROUND_TRANS	8	/* Don't fill in gaps */

/* GrNewPixmapFromData flags*/
#define GR_BMDATA_BYTEREVERSE	01	/* byte-reverse bitmap data*/
#define GR_BMDATA_BYTESWAP	02	/* byte-swap bitmap data*/

#if 0 /* don't define unimp'd flags*/
/* Window property flags */
#define GR_WM_PROP_NORESIZE	0x04	/* don't let user resize window */
#define GR_WM_PROP_NOICONISE	0x08	/* don't let user iconise window */
#define GR_WM_PROP_NOWINMENU	0x10	/* don't display a window menu button */
#define GR_WM_PROP_NOROLLUP	0x20	/* don't let user roll window up */
#define GR_WM_PROP_ONTOP	0x200	/* try to keep window always on top */
#define GR_WM_PROP_STICKY	0x400	/* keep window after desktop change */
#define GR_WM_PROP_DND		0x2000	/* accept drag and drop icons */
#endif

/* Window properties*/
#define GR_WM_PROPS_NOBACKGROUND 0x00000001L/* Don't draw window background*/
#define GR_WM_PROPS_NOFOCUS	 0x00000002L /* Don't set focus to this window*/
#define GR_WM_PROPS_NOMOVE	 0x00000004L /* Don't let user move window*/
#define GR_WM_PROPS_NORAISE	 0x00000008L /* Don't let user raise window*/
#define GR_WM_PROPS_NODECORATE	 0x00000010L /* Don't redecorate window*/
#define GR_WM_PROPS_NOAUTOMOVE	 0x00000020L /* Don't move window on 1st map*/
#define GR_WM_PROPS_NOAUTORESIZE 0x00000040L /* Don't resize window on 1st map*/
#define GR_WM_PROPS_NORESIZE	 0x00000080L /* Don't let user resize window*/

/* default decoration style*/
#define GR_WM_PROPS_APPWINDOW	0x00000000L /* Leave appearance to WM*/
#define GR_WM_PROPS_APPMASK	0xF0000000L /* Appearance mask*/
#define GR_WM_PROPS_BORDER	0x80000000L /* Single line border*/
#define GR_WM_PROPS_APPFRAME	0x40000000L /* 3D app frame (overrides border)*/
#define GR_WM_PROPS_CAPTION	0x20000000L /* Title bar*/
#define GR_WM_PROPS_CLOSEBOX	0x10000000L /* Close box*/
#define GR_WM_PROPS_MAXIMIZE	0x08000000L /* Application is maximized*/

/* Flags for indicating valid bits in GrSetWMProperties call*/
#define GR_WM_FLAGS_PROPS	0x0001	/* Properties*/
#define GR_WM_FLAGS_TITLE	0x0002	/* Title*/
#define GR_WM_FLAGS_BACKGROUND	0x0004	/* Background color*/
#define GR_WM_FLAGS_BORDERSIZE	0x0008	/* Border size*/
#define GR_WM_FLAGS_BORDERCOLOR	0x0010	/* Border color*/

/* NOTE: this struct must be hand-packed to a DWORD boundary for nxproto.h*/
/**
 * Window manager properties used by the GrGetWMProperties()/GrSetWMProperties() calls.
 */
typedef struct {
  GR_WM_PROPS flags;		/**< Which properties valid in struct for set*/
  GR_WM_PROPS props;		/**< Window property bits*/
  GR_CHAR *title;		/**< Window title*/
  GR_COLOR background;		/**< Window background color*/
  GR_SIZE bordersize;		/**< Window border size*/
  GR_COLOR bordercolor;		/**< Window border color*/
} GR_WM_PROPERTIES;

/**
 * Window properties returned by the GrGetWindowInfo() call.
 */
typedef struct {
  GR_WINDOW_ID wid;		/**< window id (or 0 if no such window) */
  GR_WINDOW_ID parent;		/**< parent window id */
  GR_WINDOW_ID child;		/**< first child window id (or 0) */
  GR_WINDOW_ID sibling;		/**< next sibling window id (or 0) */
  GR_BOOL inputonly;		/**< TRUE if window is input only */
  GR_BOOL mapped;		/**< TRUE if window is mapped */
  GR_BOOL realized;		/**< TRUE if window is mapped and visible */
  GR_COORD x;			/**< parent-relative x position of window */
  GR_COORD y;			/**< parent-relative  y position of window */
  GR_SIZE width;		/**< width of window */
  GR_SIZE height;		/**< height of window */
  GR_SIZE bordersize;		/**< size of border */
  GR_COLOR bordercolor;		/**< color of border */
  GR_COLOR background;		/**< background color */
  GR_EVENT_MASK eventmask;	/**< current event mask for this client */
  GR_WM_PROPS props;		/**< window properties */
  GR_CURSOR_ID cursor;		/**< cursor id*/
  uint32_t processid;	/**< process id of owner*/
} GR_WINDOW_INFO;

/**
 * Graphics context properties returned by the GrGetGCInfo() call.
 */
typedef struct {
  GR_GC_ID gcid;		/**< GC id (or 0 if no such GC) */
  int mode;			/**< drawing mode */
  GR_REGION_ID region;		/**< user region */
  int xoff;			/**< x offset of user region */
  int yoff;			/**< y offset of user region */
  GR_FONT_ID font;		/**< font number */
  GR_COLOR foreground;		/**< foreground RGB color or pixel value */
  GR_COLOR background;		/**< background RGB color or pixel value */
  GR_BOOL fgispixelval;		/**< TRUE if 'foreground' is actually a GR_PIXELVAL */
  GR_BOOL bgispixelval;		/**< TRUE if 'background' is actually a GR_PIXELVAL */
  GR_BOOL usebackground;	/**< use background in bitmaps */
  GR_BOOL exposure;		/**< send exposure events on GrCopyArea */
} GR_GC_INFO;

/**
 * color palette
 */
typedef struct {
  GR_COUNT count;		/**< # valid entries */
  GR_PALENTRY palette[256];	/**< palette */
} GR_PALETTE;

/** Calibration data passed to GrCalcTransform */
typedef struct {
  int xres;			/**< X resolution of the screen */
  int yres;			/**< Y resolution of the screen */
  int minx;			/**< min raw X value */
  int miny;			/**< min raw Y values */
  int maxx;			/**< max raw X value */
  int maxy;			/**< max raw Y value */
  GR_BOOL xswap;		/**< true if the x component should be swapped */
  GR_BOOL yswap;		/**< true if the y component should be swapped */
} GR_CAL_DATA;

/* Error codes */
#define	GR_ERROR_BAD_WINDOW_ID		1
#define	GR_ERROR_BAD_GC_ID		2
#define	GR_ERROR_BAD_CURSOR_SIZE	3
#define	GR_ERROR_MALLOC_FAILED		4
#define	GR_ERROR_BAD_WINDOW_SIZE	5
#define	GR_ERROR_KEYBOARD_ERROR		6
#define	GR_ERROR_MOUSE_ERROR		7
#define	GR_ERROR_INPUT_ONLY_WINDOW	8
#define	GR_ERROR_ILLEGAL_ON_ROOT_WINDOW	9
#define	GR_ERROR_TOO_MUCH_CLIPPING	10
#define	GR_ERROR_SCREEN_ERROR		11
#define	GR_ERROR_UNMAPPED_FOCUS_WINDOW	12
#define	GR_ERROR_BAD_DRAWING_MODE	13
#define GR_ERROR_BAD_LINE_ATTRIBUTE     14
#define GR_ERROR_BAD_FILL_MODE          15
#define GR_ERROR_BAD_REGION_ID		16

/* Event types.
 * Mouse motion is generated for every motion of the mouse, and is used to
 * track the entire history of the mouse (many events and lots of overhead).
 * Mouse position ignores the history of the motion, and only reports the
 * latest position of the mouse by only queuing the latest such event for
 * any single client (good for rubber-banding).
 */
#define	GR_EVENT_TYPE_ERROR		(-1)
#define	GR_EVENT_TYPE_NONE		0
#define	GR_EVENT_TYPE_EXPOSURE		1
#define	GR_EVENT_TYPE_BUTTON_DOWN	2
#define	GR_EVENT_TYPE_BUTTON_UP		3
#define	GR_EVENT_TYPE_MOUSE_ENTER	4
#define	GR_EVENT_TYPE_MOUSE_EXIT	5
#define	GR_EVENT_TYPE_MOUSE_MOTION	6
#define	GR_EVENT_TYPE_MOUSE_POSITION	7
#define	GR_EVENT_TYPE_KEY_DOWN		8
#define	GR_EVENT_TYPE_KEY_UP		9
#define	GR_EVENT_TYPE_FOCUS_IN		10
#define	GR_EVENT_TYPE_FOCUS_OUT		11
#define GR_EVENT_TYPE_FDINPUT		12
#define GR_EVENT_TYPE_UPDATE		13
#define GR_EVENT_TYPE_CHLD_UPDATE	14
#define GR_EVENT_TYPE_CLOSE_REQ		15
#define GR_EVENT_TYPE_TIMEOUT		16
#define GR_EVENT_TYPE_SCREENSAVER	17
#define GR_EVENT_TYPE_CLIENT_DATA_REQ	18
#define GR_EVENT_TYPE_CLIENT_DATA	19
#define GR_EVENT_TYPE_SELECTION_CHANGED 20
#define GR_EVENT_TYPE_TIMER             21
#define GR_EVENT_TYPE_PORTRAIT_CHANGED  22

/* Event masks */
#define	GR_EVENTMASK(n)			(((GR_EVENT_MASK) 1) << (n))

#define	GR_EVENT_MASK_NONE		GR_EVENTMASK(GR_EVENT_TYPE_NONE)
#define	GR_EVENT_MASK_ERROR		0x80000000L
#define	GR_EVENT_MASK_EXPOSURE		GR_EVENTMASK(GR_EVENT_TYPE_EXPOSURE)
#define	GR_EVENT_MASK_BUTTON_DOWN	GR_EVENTMASK(GR_EVENT_TYPE_BUTTON_DOWN)
#define	GR_EVENT_MASK_BUTTON_UP		GR_EVENTMASK(GR_EVENT_TYPE_BUTTON_UP)
#define	GR_EVENT_MASK_MOUSE_ENTER	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_ENTER)
#define	GR_EVENT_MASK_MOUSE_EXIT	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_EXIT)
#define	GR_EVENT_MASK_MOUSE_MOTION	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_MOTION)
#define	GR_EVENT_MASK_MOUSE_POSITION	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_POSITION)
#define	GR_EVENT_MASK_KEY_DOWN		GR_EVENTMASK(GR_EVENT_TYPE_KEY_DOWN)
#define	GR_EVENT_MASK_KEY_UP		GR_EVENTMASK(GR_EVENT_TYPE_KEY_UP)
#define	GR_EVENT_MASK_FOCUS_IN		GR_EVENTMASK(GR_EVENT_TYPE_FOCUS_IN)
#define	GR_EVENT_MASK_FOCUS_OUT		GR_EVENTMASK(GR_EVENT_TYPE_FOCUS_OUT)
#define	GR_EVENT_MASK_FDINPUT		GR_EVENTMASK(GR_EVENT_TYPE_FDINPUT)
#define	GR_EVENT_MASK_UPDATE		GR_EVENTMASK(GR_EVENT_TYPE_UPDATE)
#define	GR_EVENT_MASK_CHLD_UPDATE	GR_EVENTMASK(GR_EVENT_TYPE_CHLD_UPDATE)
#define	GR_EVENT_MASK_CLOSE_REQ		GR_EVENTMASK(GR_EVENT_TYPE_CLOSE_REQ)
#define	GR_EVENT_MASK_TIMEOUT		GR_EVENTMASK(GR_EVENT_TYPE_TIMEOUT)
#define GR_EVENT_MASK_SCREENSAVER	GR_EVENTMASK(GR_EVENT_TYPE_SCREENSAVER)
#define GR_EVENT_MASK_CLIENT_DATA_REQ	GR_EVENTMASK(GR_EVENT_TYPE_CLIENT_DATA_REQ)
#define GR_EVENT_MASK_CLIENT_DATA	GR_EVENTMASK(GR_EVENT_TYPE_CLIENT_DATA)
#define GR_EVENT_MASK_SELECTION_CHANGED GR_EVENTMASK(GR_EVENT_TYPE_SELECTION_CHANGED)
#define GR_EVENT_MASK_TIMER             GR_EVENTMASK(GR_EVENT_TYPE_TIMER)
#define GR_EVENT_MASK_PORTRAIT_CHANGED  GR_EVENTMASK(GR_EVENT_TYPE_PORTRAIT_CHANGED)
/* Event mask does not affect GR_EVENT_TYPE_HOTKEY_DOWN and
 * GR_EVENT_TYPE_HOTKEY_UP, hence no masks for those events. */

#define	GR_EVENT_MASK_ALL		((GR_EVENT_MASK) -1L)

/* update event types */
#define GR_UPDATE_MAP		1
#define GR_UPDATE_UNMAP		2
#define GR_UPDATE_MOVE		3
#define GR_UPDATE_SIZE		4
#define GR_UPDATE_UNMAPTEMP	5	/* unmap during window move/resize*/
#define GR_UPDATE_ACTIVATE	6	/* toplevel window [de]activate*/
#define GR_UPDATE_DESTROY	7
#define GR_UPDATE_REPARENT      8

/**
 * Event for errors detected by the server.
 * These events are not delivered to GrGetNextEvent, but instead call
 * the user supplied error handling function.  Only the first one of
 * these errors at a time is saved for delivery to the client since
 * there is not much to be done about errors anyway except complain
 * and exit.
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_FUNC_NAME name;		/**< function name which failed */
  GR_ERROR code;		/**< error code */
  GR_ID id;			/**< resource id (maybe useless) */
} GR_EVENT_ERROR;

/**
 * Event for a mouse button pressed down or released.
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< window id event delivered to */
  GR_WINDOW_ID subwid;		/**< sub-window id (pointer was in) */
  GR_COORD rootx;		/**< root window x coordinate */
  GR_COORD rooty;		/**< root window y coordinate */
  GR_COORD x;			/**< window x coordinate of mouse */
  GR_COORD y;			/**< window y coordinate of mouse */
  GR_BUTTON buttons;		/**< current state of all buttons */
  GR_BUTTON changebuttons;	/**< buttons which went down or up */
  GR_KEYMOD modifiers;		/**< modifiers (MWKMOD_SHIFT, etc)*/
  GR_TIMEOUT time;		/**< tickcount time value*/
} GR_EVENT_BUTTON;

/**
 * Event for a keystroke typed for the window with has focus.
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< window id event delived to */
  GR_WINDOW_ID subwid;		/**< sub-window id (pointer was in) */
  GR_COORD rootx;		/**< root window x coordinate */
  GR_COORD rooty;		/**< root window y coordinate */
  GR_COORD x;			/**< window x coordinate of mouse */
  GR_COORD y;			/**< window y coordinate of mouse */
  GR_BUTTON buttons;		/**< current state of buttons */
  GR_KEYMOD modifiers;		/**< modifiers (MWKMOD_SHIFT, etc)*/
  GR_KEY ch;			/**< 16-bit unicode key value, MWKEY_xxx */
  GR_SCANCODE scancode;		/**< OEM scancode value if available*/
  GR_BOOL hotkey;		/**< TRUE if generated from GrGrabKey(GR_GRAB_HOTKEY_x) */
} GR_EVENT_KEYSTROKE;

/**
 * Event for exposure for a region of a window.
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< window id */
  GR_COORD x;			/**< window x coordinate of exposure */
  GR_COORD y;			/**< window y coordinate of exposure */
  GR_SIZE width;		/**< width of exposure */
  GR_SIZE height;		/**< height of exposure */
} GR_EVENT_EXPOSURE;

/**
 * General events for focus in or focus out for a window, or mouse enter
 * or mouse exit from a window, or window unmapping or mapping, etc.
 * Server portrait mode changes are also sent using this event to
 * all windows that request it.
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< window id */
  GR_WINDOW_ID otherid;		/**< new/old focus id for focus events*/
  						/**< for mouse enter only the following are valid:*/
  GR_COORD rootx;		/**< root window x coordinate */
  GR_COORD rooty;		/**< root window y coordinate */
  GR_COORD x;			/**< window x coordinate of mouse */
  GR_COORD y;			/**< window y coordinate of mouse */
} GR_EVENT_GENERAL;

/**
 * Events for mouse motion or mouse position.
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< window id event delivered to */
  GR_WINDOW_ID subwid;		/**< sub-window id (pointer was in) */
  GR_COORD rootx;		/**< root window x coordinate */
  GR_COORD rooty;		/**< root window y coordinate */
  GR_COORD x;			/**< window x coordinate of mouse */
  GR_COORD y;			/**< window y coordinate of mouse */
  GR_BUTTON buttons;		/**< current state of buttons */
  GR_KEYMOD modifiers;		/**< modifiers (MWKMOD_SHIFT, etc)*/
} GR_EVENT_MOUSE;

/**
 * GrRegisterInput() event.
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  int		fd;		/**< input file descriptor*/
} GR_EVENT_FDINPUT;

/**
 * GR_EVENT_TYPE_UPDATE
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< select window id*/
  GR_WINDOW_ID subwid;		/**< update window id (=wid for UPDATE event)*/
  GR_COORD x;			/**< new window x coordinate */
  GR_COORD y;			/**< new window y coordinate */
  GR_SIZE width;		/**< new width */
  GR_SIZE height;		/**< new height */
  GR_UPDATE_TYPE utype;		/**< update_type */
} GR_EVENT_UPDATE;

/**
 * GR_EVENT_TYPE_SCREENSAVER
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_BOOL activate;		/**< true = activate, false = deactivate */
} GR_EVENT_SCREENSAVER;

/**
 * GR_EVENT_TYPE_CLIENT_DATA_REQ
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< ID of requested window */
  GR_WINDOW_ID rid;		/**< ID of window to send data to */
  GR_SERIALNO serial;		/**< Serial number of transaction */
  GR_MIMETYPE mimetype;		/**< Type to supply data as */
} GR_EVENT_CLIENT_DATA_REQ;

/**
 * GR_EVENT_TYPE_CLIENT_DATA
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< ID of window data is destined for */
  GR_WINDOW_ID rid;		/**< ID of window data is from */
  GR_SERIALNO serial;		/**< Serial number of transaction */
  uint32_t len;		/**< Total length of data */
  uint32_t datalen;	/**< Length of following data */
  void *data;			/**< Pointer to data (filled in on client side) */
} GR_EVENT_CLIENT_DATA;

/**
 * GR_EVENT_TYPE_SELECTION_CHANGED
 */
typedef struct {
  GR_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID new_owner;	/**< ID of new selection owner */
} GR_EVENT_SELECTION_CHANGED;

/**
 * GR_EVENT_TYPE_TIMER
 */
typedef struct {
  GR_EVENT_TYPE  type;		/**< event type, GR_EVENT_TYPE_TIMER */
  GR_WINDOW_ID   wid;		/**< ID of window timer is destined for */
  GR_TIMER_ID    tid;		/**< ID of expired timer */
} GR_EVENT_TIMER;

/**
 * Union of all possible event structures.
 * This is the structure returned by GrGetNextEvent() and similar routines.
 */
typedef union {
  GR_EVENT_TYPE type;			/**< event type */
  GR_EVENT_ERROR error;			/**< error event */
  GR_EVENT_GENERAL general;		/**< general window events */
  GR_EVENT_BUTTON button;		/**< button events */
  GR_EVENT_KEYSTROKE keystroke;		/**< keystroke events */
  GR_EVENT_EXPOSURE exposure;		/**< exposure events */
  GR_EVENT_MOUSE mouse;			/**< mouse motion events */
  GR_EVENT_FDINPUT fdinput;		/**< fd input events*/
  GR_EVENT_UPDATE update;		/**< window update events */
  GR_EVENT_SCREENSAVER screensaver; 	/**< Screen saver events */
  GR_EVENT_CLIENT_DATA_REQ clientdatareq; /**< Request for client data events */
  GR_EVENT_CLIENT_DATA clientdata;	  /**< Client data events */
  GR_EVENT_SELECTION_CHANGED selectionchanged; /**< Selection owner changed */
  GR_EVENT_TIMER timer;                 /**< Timer events */
} GR_EVENT;

typedef void (*GR_FNCALLBACKEVENT)(GR_EVENT *);

/* GR_BITMAP macros*/
/* size of GR_BITMAP image in words*/
#define	GR_BITMAP_SIZE(width, height)	MWIMAGE_SIZE(width, height)
#define	GR_BITMAPBITS			MWIMAGE_BITSPERIMAGE
#define	GR_BITVALUE(n)			MWIMAGE_BITVALUE(n)
#define	GR_FIRSTBIT			MWIMAGE_FIRSTBIT
#define	GR_NEXTBIT(m)			MWIMAGE_NEXTBIT(m)
#define	GR_TESTBIT(m)			MWIMAGE_TESTBIT(m)
#define	GR_SHIFTBIT(m)			MWIMAGE_SHIFTBIT(m)

/* GrGrabKey() types. */

/**
 * Key reservation type for GrGrabKey() - a key is reserved exclusively,
 * and hotkey events are sent regardless of focus.
 *
 * Hotkey events are sent to the client that reserved the key.  The window
 * ID passed to the GrGrabKey() call is passed as the source window.
 *
 * This type of reservation is useful for implementing a "main menu" key
 * or similar hotkeys.
 *
 * This can be used to implement the MHP method
 * org.dvb.event.EventManager.addUserEventListener(listener,client,events).
 *
 * @see GrGrabKey()
 * @see GrUngrabKey()
 */
#define GR_GRAB_HOTKEY_EXCLUSIVE        0

/**
 * Key reservation type for GrGrabKey() - hotkey events are sent when a key
 * is pressed, regardless of focus.  This is not an exclusive reservation,
 * so the app that has the focus will get a normal key event.
 *
 * Hotkey events are sent to the client that reserved the key.  The window
 * ID passed to the GrGrabKey() call is passed as the source window.
 *
 * Note that because this is not an exclusive grab, it does not stop
 * other applications from grabbing the same key (using #GR_GRAB_HOTKEY
 * or any other grab mode).  If an application has an exclusive grab on
 * a key, then any grabs of type #GR_GRAB_HOTKEY will be ignored when
 * dispatching that key event.
 *
 * This can be used to implement the MHP method
 * org.dvb.event.EventManager.addUserEventListener(listener,events).
 *
 * @see GrGrabKey()
 * @see GrUngrabKey()
 */
#define GR_GRAB_HOTKEY                  1

/**
 * Key reservation type for GrGrabKey() - a key is reserved exclusively,
 * and normal key events are sent if the specified window has focus.
 *
 * This stops other applications from getting events on the specified key.
 *
 * For example, an application could use this to reserve the number
 * keys before asking the user for a PIN, to prevent other applications
 * stealing the PIN using #GR_GRAB_TYPE_HOTKEY.  (Note that this assumes
 * the applications are running in a controlled environment, such as
 * Java, so they can only interact with the platform in limited ways).
 *
 * This can be used to implement the MHP method
 * org.dvb.event.EventManager.addExclusiveAccessToAWTEvent(client,events).
 *
 * @see GrGrabKey()
 * @see GrUngrabKey()
 */
#define GR_GRAB_EXCLUSIVE               2

/**
 * Key reservation type for GrGrabKey() - a key is reserved exclusively,
 * and normal key events are sent if the specified window has focus,
 * or the mouse pointer is over the window.
 *
 * This stops other applications from getting events on the specified key.
 *
 * This is for compatibility with the first GrGrabKey() API, which only
 * supported this kind of reservation.
 *
 * @see GrGrabKey()
 * @see GrUngrabKey()
 */
#define GR_GRAB_EXCLUSIVE_MOUSE       3

/**
 * Highest legal value of any GR_GRAB_xxx constant.  (Lowest legal value 
 * must be 0).
 *
 * @internal
 */
#define GR_GRAB_MAX                     GR_GRAB_EXCLUSIVE_MOUSE

/* GrGetSysColor colors*/
/* desktop background*/
#define GR_COLOR_DESKTOP           0

/* caption colors*/
#define GR_COLOR_ACTIVECAPTION     1
#define GR_COLOR_ACTIVECAPTIONTEXT 2
#define GR_COLOR_INACTIVECAPTION   3
#define GR_COLOR_INACTIVECAPTIONTEXT 4

/* 3d border shades*/
#define GR_COLOR_WINDOWFRAME       5
#define GR_COLOR_BTNSHADOW         6
#define GR_COLOR_3DLIGHT           7
#define GR_COLOR_BTNHIGHLIGHT      8

/* top level application window backgrounds/text*/
#define GR_COLOR_APPWINDOW         9
#define GR_COLOR_APPTEXT           10

/* button control backgrounds/text (usually same as app window colors)*/
#define GR_COLOR_BTNFACE           11
#define GR_COLOR_BTNTEXT           12

/* edit/listbox control backgrounds/text, selected highlights*/
#define GR_COLOR_WINDOW            13
#define GR_COLOR_WINDOWTEXT        14
#define GR_COLOR_HIGHLIGHT         15
#define GR_COLOR_HIGHLIGHTTEXT     16
#define GR_COLOR_GRAYTEXT          17

/* menu backgrounds/text*/
#define GR_COLOR_MENUTEXT          18
#define GR_COLOR_MENU              19

/**
 * Error strings per error number
 */
#define GR_ERROR_STRINGS		\
	"",				\
	"Bad window id: %d\n",		\
	"Bad graphics context: %d\n",	\
	"Bad cursor size\n",		\
	"Out of server memory\n",	\
	"Bad window size: %d\n",	\
	"Keyboard error\n",		\
	"Mouse error\n",		\
	"Input only window: %d\n",	\
	"Illegal on root window: %d\n",	\
	"Clipping overflow\n",		\
	"Screen error\n",		\
	"Unmapped focus window: %d\n",	\
	"Bad drawing mode gc: %d\n",    \
        "Bad line attribute gc: %d\n",  \
        "Bad fill mode gc: %d\n",       \
	"Bad region id: %d\n",
        
extern const char *nxErrorStrings[];

/* Public graphics routines. */
void		GrFlush(void);
int		GrOpen(void);
void		GrClose(void);
void		GrDelay(GR_TIMEOUT msecs);
void		GrGetScreenInfo(GR_SCREEN_INFO *sip);
GR_COLOR	GrGetSysColor(int index);
GR_WINDOW_ID	GrNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_SIZE bordersize,
			GR_COLOR background, GR_COLOR bordercolor);
GR_WINDOW_ID    GrNewPixmap(GR_SIZE width, GR_SIZE height, void *pixels);
GR_WINDOW_ID	GrNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
				GR_SIZE width, GR_SIZE height);
void		GrDestroyWindow(GR_WINDOW_ID wid);
GR_GC_ID	GrNewGC(void);
GR_GC_ID	GrCopyGC(GR_GC_ID gc);
void		GrGetGCInfo(GR_GC_ID gc, GR_GC_INFO *gcip);
void		GrDestroyGC(GR_GC_ID gc);
GR_REGION_ID	GrNewRegion(void);
GR_REGION_ID	GrNewBitmapRegion(GR_BITMAP *bitmap, GR_SIZE width,
			GR_SIZE height);
GR_REGION_ID	GrNewPolygonRegion(int mode, GR_COUNT count, GR_POINT *points);
void		GrDestroyRegion(GR_REGION_ID region);
void		GrUnionRectWithRegion(GR_REGION_ID region, GR_RECT *rect);
void		GrUnionRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
			GR_REGION_ID src_rgn2);
void		GrIntersectRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
			GR_REGION_ID src_rgn2);
void		GrSubtractRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
			GR_REGION_ID src_rgn2);
void		GrXorRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
			GR_REGION_ID src_rgn2);
void		GrSetGCRegion(GR_GC_ID gc, GR_REGION_ID region);
void		GrSetGCClipOrigin(GR_GC_ID gc, int x, int y);
GR_BOOL		GrPointInRegion(GR_REGION_ID region, GR_COORD x, GR_COORD y);
int			GrRectInRegion(GR_REGION_ID region, GR_COORD x, GR_COORD y,
			GR_COORD w, GR_COORD h);
GR_BOOL		GrEmptyRegion(GR_REGION_ID region);
GR_BOOL		GrEqualRegion(GR_REGION_ID rgn1, GR_REGION_ID rgn2);
void		GrOffsetRegion(GR_REGION_ID region, GR_SIZE dx, GR_SIZE dy);
int			GrGetRegionBox(GR_REGION_ID region, GR_RECT *rect);
void		GrMapWindow(GR_WINDOW_ID wid);
void		GrUnmapWindow(GR_WINDOW_ID wid);
void		GrRaiseWindow(GR_WINDOW_ID wid);
void		GrLowerWindow(GR_WINDOW_ID wid);
void		GrMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y);
void		GrResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height);
void		GrReparentWindow(GR_WINDOW_ID wid, GR_WINDOW_ID pwid,
			GR_COORD x, GR_COORD y);
void		GrGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr);
void		GrSetWMProperties(GR_WINDOW_ID wid, GR_WM_PROPERTIES *props);
void		GrGetWMProperties(GR_WINDOW_ID wid, GR_WM_PROPERTIES *props);

GR_FONT_ID	GrCreateFont(GR_CHAR *name, GR_COORD height, GR_LOGFONT *plogfont); // DEPRECATED
GR_FONT_ID	GrCreateFontEx(GR_CHAR *name, GR_COORD height, GR_COORD width, GR_LOGFONT *plogfont);
GR_FONT_ID	GrCreateFontFromBuffer(const void *buffer, unsigned length,
			const char *format, GR_COORD height, GR_COORD width);
GR_FONT_ID	GrCopyFont(GR_FONT_ID fontid, GR_COORD height, GR_COORD width);
void		GrGetFontList(GR_FONTLIST ***fonts, int *numfonts);
void		GrFreeFontList(GR_FONTLIST ***fonts, int numfonts);
void		GrSetFontSize(GR_FONT_ID fontid, GR_COORD size); // DEPRECATED
void		GrSetFontSizeEx(GR_FONT_ID fontid, GR_COORD height, GR_COORD width);
void		GrSetFontRotation(GR_FONT_ID fontid, int tenthsdegrees);
void		GrSetFontAttr(GR_FONT_ID fontid, int setflags, int clrflags);
void		GrDestroyFont(GR_FONT_ID fontid);
void		GrGetFontInfo(GR_FONT_ID font, GR_FONT_INFO *fip);
GR_WINDOW_ID	GrGetFocus(void);

void		GrSetFocus(GR_WINDOW_ID wid);
void		GrClearArea(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_BOOL exposeflag);
void		GrSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask);
void		GrGetNextEvent(GR_EVENT *ep);
int             GrGetTypedEvent(GR_WINDOW_ID wid, GR_EVENT_MASK mask, 
			GR_UPDATE_TYPE update, GR_EVENT *ep, GR_BOOL block);
typedef GR_BOOL (*GR_TYPED_EVENT_CALLBACK)(GR_WINDOW_ID, GR_EVENT_MASK,
			GR_UPDATE_TYPE, GR_EVENT *, void *);
int			GrGetTypedEventPred(GR_WINDOW_ID wid, GR_EVENT_MASK mask, 
			GR_UPDATE_TYPE update, GR_EVENT * ep, GR_BOOL block, 
			GR_TYPED_EVENT_CALLBACK matchfn, void *arg);
void		GrGetNextEventTimeout(GR_EVENT *ep, GR_TIMEOUT timeout);
void		GrCheckNextEvent(GR_EVENT *ep);
int			GrPeekEvent(GR_EVENT *ep);
void		GrPeekWaitEvent(GR_EVENT *ep);
void		GrCopyEvent(GR_EVENT *dst, GR_EVENT *src);
void		GrFreeEvent(GR_EVENT *ev);
void		GrLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1,
			GR_COORD x2, GR_COORD y2);
void		GrPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y);
void		GrPoints(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,
			GR_POINT *pointtable);
void		GrRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
void		GrFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
void		GrPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,
			GR_POINT *pointtable);
void		GrFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,
			GR_POINT *pointtable);
void		GrEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE rx, GR_SIZE ry);
void		GrFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x,
			GR_COORD y, GR_SIZE rx, GR_SIZE ry);
void		GrArc(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE rx, GR_SIZE ry, GR_COORD ax, GR_COORD ay,
			GR_COORD bx, GR_COORD by, int type);
void		GrArcAngle(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE rx, GR_SIZE ry, GR_COORD angle1,
			GR_COORD angle2, int type); /* floating point required*/
void		GrSetGCForeground(GR_GC_ID gc, GR_COLOR foreground);
void		GrSetGCForegroundPixelVal(GR_GC_ID gc, GR_PIXELVAL foreground);
void		GrSetGCBackground(GR_GC_ID gc, GR_COLOR background);
void		GrSetGCBackgroundPixelVal(GR_GC_ID gc, GR_PIXELVAL background);
void		GrSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag);
void		GrSetGCMode(GR_GC_ID gc, int mode);
void            GrSetGCLineAttributes(GR_GC_ID, int);
void            GrSetGCDash(GR_GC_ID, char *, int);
void            GrSetGCFillMode(GR_GC_ID, int);
void            GrSetGCStipple(GR_GC_ID, GR_BITMAP *, GR_SIZE, GR_SIZE);
void            GrSetGCTile(GR_GC_ID, GR_WINDOW_ID, GR_SIZE, GR_SIZE);
void            GrSetGCTSOffset(GR_GC_ID, GR_COORD, GR_COORD);
void            GrSetGCGraphicsExposure(GR_GC_ID gc, GR_BOOL exposure);
void		GrSetGCFont(GR_GC_ID gc, GR_FONT_ID font);
void		GrGetGCTextSize(GR_GC_ID gc, void *str, int count,
			GR_TEXTFLAGS flags, GR_SIZE *retwidth,
			GR_SIZE *retheight,GR_SIZE *retbase);
void		GrReadArea(GR_DRAW_ID id, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE height, GR_PIXELVAL *pixels);
void		GrArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width,GR_SIZE height,void *pixels,int pixtype);
void            GrCopyArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_DRAW_ID srcid,
			GR_COORD srcx, GR_COORD srcy, int op);
void		GrStretchArea(GR_DRAW_ID dstid, GR_GC_ID gc, GR_COORD dx1,
			GR_COORD dy1, GR_COORD dx2, GR_COORD dy2,
			GR_DRAW_ID srcid, GR_COORD sx1, GR_COORD sy1,
			GR_COORD sx2, GR_COORD sy2, int op);
void		GrBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_BITMAP *imagebits);
void		GrDrawImageBits(GR_DRAW_ID id,GR_GC_ID gc,GR_COORD x,GR_COORD y,
			GR_IMAGE_HDR *pimage);
void		GrDrawImageFromFile(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x,
			GR_COORD y, GR_SIZE width, GR_SIZE height,
			char *path, int flags);
GR_IMAGE_ID	GrLoadImageFromFile(char *path, int flags);
void		GrDrawImageFromBuffer(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x,
			GR_COORD y, GR_SIZE width, GR_SIZE height,
			void *buffer, int size, int flags);
GR_IMAGE_ID	GrLoadImageFromBuffer(void *buffer, int size, int flags);
void		GrDrawImageToFit(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x,
			GR_COORD y, GR_SIZE width, GR_SIZE height,
			GR_IMAGE_ID imageid);
void		GrDrawImagePartToFit(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD dx, GR_COORD dy,
			GR_SIZE dwidth, GR_SIZE dheight, GR_COORD sx, GR_COORD sy,
			GR_SIZE swidth, GR_SIZE sheight, GR_IMAGE_ID imageid);
void		GrFreeImage(GR_IMAGE_ID id);
void		GrGetImageInfo(GR_IMAGE_ID id, GR_IMAGE_INFO *iip);
void		GrText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			void *str, GR_COUNT count, GR_TEXTFLAGS flags);
GR_CURSOR_ID	GrNewCursor(GR_SIZE width, GR_SIZE height, GR_COORD hotx,
			GR_COORD hoty, GR_COLOR foreground, GR_COLOR background,
			GR_BITMAP *fgbitmap, GR_BITMAP *bgbitmap);
void		GrDestroyCursor(GR_CURSOR_ID cid);
void		GrSetWindowCursor(GR_WINDOW_ID wid, GR_CURSOR_ID cid);
void		GrSetWindowRegion(GR_WINDOW_ID wid, GR_REGION_ID rid, int type);
void		GrMoveCursor(GR_COORD x, GR_COORD y);
void		GrGetSystemPalette(GR_PALETTE *pal);
void		GrSetSystemPalette(GR_COUNT first, GR_PALETTE *pal);
void		GrFindColor(GR_COLOR c, GR_PIXELVAL *retpixel);
void		GrReqShmCmds(long shmsize);
void		GrInjectPointerEvent(MWCOORD x, MWCOORD y,
			int button, int visible);
void		GrInjectKeyboardEvent(GR_WINDOW_ID wid, GR_KEY keyvalue,
			GR_KEYMOD modifiers, GR_SCANCODE scancode,
			GR_BOOL pressed);
void		GrCloseWindow(GR_WINDOW_ID wid);
void		GrKillWindow(GR_WINDOW_ID wid);
void		GrSetScreenSaverTimeout(GR_TIMEOUT timeout);
void		GrSetSelectionOwner(GR_WINDOW_ID wid, GR_CHAR *typelist);
GR_WINDOW_ID	GrGetSelectionOwner(GR_CHAR **typelist);
void		GrRequestClientData(GR_WINDOW_ID wid, GR_WINDOW_ID rid,
			GR_SERIALNO serial, GR_MIMETYPE mimetype);
void		GrSendClientData(GR_WINDOW_ID wid, GR_WINDOW_ID did,
			GR_SERIALNO serial, GR_LENGTH len, GR_LENGTH thislen,
			void *data);
void		GrBell(void);
void		GrSetBackgroundPixmap(GR_WINDOW_ID wid, GR_WINDOW_ID pixmap,
			int flags);
void		GrQueryPointer(GR_WINDOW_ID *mwin, GR_COORD *x, GR_COORD *y,
			GR_BUTTON *bmask);
void		GrQueryTree(GR_WINDOW_ID wid, GR_WINDOW_ID *parentid,
			GR_WINDOW_ID **children, GR_COUNT *nchildren);
GR_BOOL         GrGrabKey(GR_WINDOW_ID wid, GR_KEY key, int type);
void            GrUngrabKey(GR_WINDOW_ID wid, GR_KEY key);

GR_TIMER_ID	GrCreateTimer(GR_WINDOW_ID wid, GR_TIMEOUT period);
void		GrDestroyTimer(GR_TIMER_ID tid);
void		GrSetPortraitMode(int portraitmode);

void		GrRegisterInput(int fd);
void		GrUnregisterInput(int fd);
void		GrMainLoop(GR_FNCALLBACKEVENT fncb);
GR_FNCALLBACKEVENT GrSetErrorHandler(GR_FNCALLBACKEVENT fncb);
void		GrDefaultErrorHandler(GR_EVENT *ep);

/* passive library entry points - available with client/server only*/
void		GrPrepareSelect(int *maxfd,void *rfdset);
void		GrServiceSelect(void *rfdset, GR_FNCALLBACKEVENT fncb);

/* Client side queue count - available only with client/server */
int             GrQueueLength(void);

void            GrSetTransform(GR_TRANSFORM *);

/* nxtransform.c - mouse utility routines (requires floating point)*/
int             GrCalcTransform(GR_CAL_DATA *, GR_TRANSFORM *);
int             GrLoadTransformData(char *filename, GR_TRANSFORM *);
int             GrSaveTransformData(GR_TRANSFORM *, char *filename);

/* nxutil.c - utility routines*/
GR_WINDOW_ID	GrNewWindowEx(GR_WM_PROPS props, GR_CHAR *title,
			GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_COLOR background);
void		GrDrawLines(GR_DRAW_ID w, GR_GC_ID gc, GR_POINT *points,
			GR_COUNT count);
GR_BITMAP *	GrNewBitmapFromData(GR_SIZE width, GR_SIZE height, GR_SIZE bits_width,
			GR_SIZE bits_height, void *bits, int flags);
GR_WINDOW_ID    GrNewPixmapFromData(GR_SIZE width, GR_SIZE height, 
			GR_COLOR foreground, GR_COLOR background, void * bits,
			int flags);
GR_BITMAP *	GrNewBitmapFromPixmap(GR_WINDOW_ID pixmap, int x, int y, GR_SIZE width,
			GR_SIZE height);
GR_BITMAP *	GrNewBitmapFromPixmap(GR_WINDOW_ID pixmap, int x, int y,
			GR_SIZE width, GR_SIZE height);
GR_REGION_ID	GrNewRegionFromPixmap(GR_WINDOW_ID src, MWCOORD x, MWCOORD y,
			GR_SIZE width, GR_SIZE height);

/* direct client-side framebuffer mapping routines*/
unsigned char * GrOpenClientFramebuffer(void);
void		GrCloseClientFramebuffer(void);
void		GrGetWindowFBInfo(GR_WINDOW_ID wid, GR_WINDOW_FB_INFO *fbinfo);

/* retrofit - no longer used*/
GR_CURSOR_ID	GrSetCursor(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height,
			GR_COORD hotx, GR_COORD hoty, GR_COLOR foreground,
			GR_COLOR background, GR_BITMAP *fbbitmap,
			GR_BITMAP *bgbitmap);
#define GrSetBorderColor		GrSetWindowBorderColor	/* retrofit*/
#define GrClearWindow(wid,exposeflag)	GrClearArea(wid,0,0,0,0,exposeflag) /* retrofit*/

/* useful function macros*/
#define GrSetWindowBackgroundColor(wid,color) \
		{	GR_WM_PROPERTIES props;	\
			props.flags = GR_WM_FLAGS_BACKGROUND; \
			props.background = color; \
			GrSetWMProperties(wid, &props); \
		}
#define GrSetWindowBorderSize(wid,width) \
		{	GR_WM_PROPERTIES props;	\
			props.flags = GR_WM_FLAGS_BORDERSIZE; \
			props.bordersize = width; \
			GrSetWMProperties(wid, &props); \
		}
#define GrSetWindowBorderColor(wid,color) \
		{	GR_WM_PROPERTIES props;	\
			props.flags = GR_WM_FLAGS_BORDERCOLOR; \
			props.bordercolor = color; \
			GrSetWMProperties(wid, &props); \
		}
#define GrSetWindowTitle(wid,name) \
		{	GR_WM_PROPERTIES props;	\
			props.flags = GR_WM_FLAGS_TITLE; \
			props.title = (GR_CHAR *)name; \
			GrSetWMProperties(wid, &props); \
		}

/* library window manager public routines*/
void wm_init(void);
int wm_handle_event(GR_EVENT *event);

#ifdef __cplusplus
}
#endif

/* client side event queue (client.c local)*/
typedef struct event_list EVENT_LIST;
struct event_list {
	EVENT_LIST *	next;
	GR_EVENT	event;
};

/* queued request buffer (nxproto.c local)*/
typedef struct {
	unsigned char *bufptr;		/* next unused buffer location*/
	unsigned char *bufmax;		/* max buffer location*/
	unsigned char *buffer;		/* request buffer*/
} REQBUF;

#if PSP
#include <pspkernel.h>
#include <psputils.h>
#define fprintf(...)	do {} while(0)
#define setbuf(...)		do {} while(0)
#define exit(...)		sceKernelExitGame()
#endif

#if RTEMS
  /* RTEMS requires rtems_main()*/
  int rtems_main(int, char **);
  #define main	rtems_main
#endif

#if __ECOS
#include <sys/select.h>
#include <cyg/kernel/kapi.h>
/*
 * In a single process, multi-threaded environment, we need to keep
 * all static data of shared code in a structure, with a pointer to
 * the structure to be stored in thread-local storage
 */
typedef struct {                                /* Init to: */
    int                 _nxSocket;              /*  -1 */
    MWMUTEX	 	_nxGlobalLock;
    int                 _storedevent;           /* 0 */
    GR_EVENT            _storedevent_data;      /* no init(0) */
    int                 _regfdmax;              /* -1 */
    fd_set		_regfdset;		/* FD_ZERO */
    GR_FNCALLBACKEVENT  _GrErrorFunc;           /* GrDefaultErrorHandler */
    REQBUF              _reqbuf;
    EVENT_LIST          *_evlist;
} ecos_nanox_client_data;

extern int     ecos_nanox_client_data_index;

#define ACCESS_PER_THREAD_DATA()                                        \
    ecos_nanox_client_data *data = (ecos_nanox_client_data*)            \
        cyg_thread_get_data((cyg_ucount32)ecos_nanox_client_data_index);

#define INIT_PER_THREAD_DATA()                                                  \
    {                                                                           \
        ecos_nanox_client_data *dptr = malloc(sizeof(ecos_nanox_client_data));  \
        ecos_nanox_client_data_index = data;                                    \
        dptr->_nxSocket = -1;                                                   \
	dptr->nxGlobalLock = 0;
        dptr->_storedevent = 0;                                                 \
        dptr->_regfdmax = -1;                                                   \
        FD_ZERO(&dptr->_regfdset);                                              \
        dptr->_GrErrorFunc = GrDefaultErrorHandler;                             \
        dptr->_reqbuf.bufptr = NULL;                                            \
        dptr->_reqbuf.bufmax = NULL;                                            \
        dptr->_reqbuf.buffer = NULL;                                            \
        dptr->_evlist = NULL;                                                   \
        cyg_thread_set_data(ecos_nanox_client_data_index,(CYG_ADDRWORD)dptr);   \
    }

#define nxSocket                (data->_nxSocket)
#define nxGlobalLock            (data->_nxGlobalLock)
#define storedevent             (data->_storedevent)
#define storedevent_data        (data->_storedevent_data)
#define regfdmax                (data->_regfdmax)
#define regfdset                (data->_regfdset)
#define ErrorFunc               (data->_GrErrorFunc)
#define reqbuf                  (data->_reqbuf)
#define evlist                  (data->_evlist)

#else
#define ACCESS_PER_THREAD_DATA()
#endif

#endif /* _NANO_X_H*/
