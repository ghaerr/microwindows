#ifndef _WINDOWS_H
#define _WINDOWS_H
/* windows.h*/
/*
 * Copyright (c) 1999, 2000, 2019 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Win32 API master public header file
 * Modifications:
 *  Date        Author                  Description
 *  2004/12/07  Gabriele Brugnoni       added lpfnWndProc param on wnd struct, so
 *                                      SetWindowLong(GWL_WNDPROC) may works on multiple wnd.
 *  2010/04/23	Ludwig Ertl				Added support for window Props
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "mwtypes.h"
#include "mwconfig.h"
#include "windef.h"
#include "wingdi.h"
#include "winfont.h"
#include "winkbd.h"
#include "winuser.h"	/* now includes winctl.h for resource compiler*/
#include "winres.h"
#include "windlg.h"

/* startup routines*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nShowCmd);
int WINAPI invoke_WinMain_Start(int ac, char **av);
void WINAPI invoke_WinMain_End(void);

/* non-Win32 Microwindows routines allowed to be called by user programs*/
BOOL WINAPI MwInitializeDialogs(HINSTANCE hInstance);	/* windlg.c*/
BOOL WINAPI MwSetDesktopWallpaper(PMWIMAGEHDR pImage);
void WINAPI MwRegisterFdInput(HWND hwnd,int fd);
void WINAPI MwUnregisterFdInput(HWND hwnd,int fd);
void WINAPI MwRegisterFdOutput(HWND hwnd,int fd);
void WINAPI MwUnregisterFdOutput(HWND hwnd,int fd);
void WINAPI MwRegisterFdExcept(HWND hwnd,int fd);
void WINAPI MwUnregisterFdExcept(HWND hwnd,int fd);
void WINAPI MwSetKeyboardTranslator(LPFN_KEYBTRANSLATE pFn);
void WINAPI MwSetTextCoding(LONG mode);
#define		MwError GdError		/* calls GdError in osdep.c*/

/* optional user hook routine called before anything*/
int	MwUserInit(int ac, char **av);

/* Internal Microwindows non-win32 definitions*/

/* GDI Objects*/

typedef struct {			/* GDI object hdr*/
	int		type;		/* OBJ_xxx type*/
	BOOL		stockobj;	/* TRUE if stock (unallocated) object*/
} MWGDIOBJHDR;

/* gdiobj*/
struct hgdiobj {
	MWGDIOBJHDR	hdr;		/* all gdi object start with this hdr*/
	/* additional data...*/		/* allocated per object type*/
};

typedef struct {
	MWGDIOBJHDR	hdr;
	int		style;		/* pen style*/
	COLORREF	color;		/* pen color*/
} MWPENOBJ;

typedef struct {
	MWGDIOBJHDR	hdr;
	int		style;		/* brush style*/
	COLORREF	color;		/* brush color*/
} MWBRUSHOBJ;

typedef struct {
	MWGDIOBJHDR	hdr;
	LOGPALETTE palette;
} MWPALOBJ;

typedef struct {
	MWGDIOBJHDR	hdr;
	PMWFONT		pfont;		/* allocated font*/
	char		name[32];	/* font name (stock objects only)*/
} MWFONTOBJ;

typedef struct {
	MWGDIOBJHDR	hdr;
	int		width;		/* width*/
	int		height;		/* height*/
	int		planes;		/* # planes*/
	int		bpp;		/* bits per pixel*/
	int		data_format;/* MWIF_ image data format*/
	unsigned int pitch;	/* bytes per line*/
	int		size;		/* allocated size in bytes*/
	char 	bits[1];	/* beginning of bitmap*/
} MWBITMAPOBJ;

typedef struct {
	MWGDIOBJHDR   	hdr;
	MWCLIPREGION  *	rgn;		/* clip region*/
} MWRGNOBJ;

typedef struct  _XFORM {
	FLOAT eM11;
	FLOAT eM12;
	FLOAT eM21;
	FLOAT eM22;
	FLOAT eDx;
	FLOAT eDy;
} XFORM, *PXFORM, *LPXFORM;

/* device context*/
struct hdc {
	struct _mwscreendevice *psd;	/* screen or memory device*/
	HWND		hwnd;		/* associated window*/
	DWORD		flags;		/* clipping flags*/
	int		bkmode;		/* background mode*/
	UINT		textalign;	/* text alignment flags*/
	MWCOLORVAL	bkcolor;	/* text background color*/
	MWCOLORVAL	textcolor;	/* text color*/
	MWBRUSHOBJ *	brush;		/* current brush*/
	MWPENOBJ *	pen;		/* current pen*/
	MWFONTOBJ *	font;		/* current font*/
	MWBITMAPOBJ *	bitmap;		/* current bitmap (mem dc's only)*/
	MWRGNOBJ *	region;		/* user specified clip region*/
	MWPALOBJ *	palette;
	int		drawmode;	/* rop2 drawing mode */
	POINT		pt;		/* current pen pos in client coords*/
#if WINEXTRA
	INT           GraphicsMode;      /* Graphics mode */
	XFORM         xformWorld2Wnd;    /* World-to-window transformation */
	XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
	XFORM         xformVport2World;  /* Inverse of the above transformation */
#endif
};

/* cursor*/
struct hcursor {
	int		usecount;	/* use counter */
	MWCURSOR	cursor;		/* software cursor definition*/
};

/* built-in scrollbars*/
typedef struct {
	int		minPos;        /* min value of scroll range.*/
	int		maxPos;        /* max value of scroll range.*/
	int		curPos;        /* current scroll pos.*/
	int     trackPos;      /* current scroll box track position */
	int		pageStep;      /* steps per page.*/
	int		barStart;      /* start pixel of bar.*/
	int		barLen;        /* length of bar.*/
	int		status;        /* status of scroll bar.*/
	RECT		rc;	       /* screen coordinates position*/
} MWSCROLLBARINFO, *PMWSCROLLBARINFO;

/* window*/
struct hwnd {
	RECT		winrect;	/* window rect in screen coords*/
	RECT		clirect;	/* client rect in screen coords*/
	RECT		restorerc;	/* restore rect from maximized*/
	DWORD		style;		/* window style*/
	DWORD		exstyle;	/* window extended style*/
	PWNDCLASS	pClass;		/* window class*/
	WNDPROC     lpfnWndProc;/* default window procedure (initialized from pClass)*/
	WNDPROC		lpfnWndProcBridge;	/* used externally only*/
	struct hwnd	*parent;	/* z-order parent window */
	struct hwnd	*owner;		/* owner window*/
	struct hwnd	*children;	/* first child window */
	struct hwnd	*siblings;	/* next sibling window */
	struct hwnd	*next;		/* next window in complete list */
	struct hcursor	*cursor;	/* cursor for this window */
	struct hdc *	owndc;		/* owndc if CS_OWNDC*/
	int		unmapcount;	/* count of reasons not really mapped */
	int		id;		/* window id */
	LPTSTR		szTitle;	/* window title*/
	int		gotPaintMsg;	/* window had WM_PAINT PostMessage*/
	int		paintSerial;	/* experimental serial # for alphblend*/
	int		paintNC;	/* experimental NC paint handling*/
	int		nEraseBkGnd;	/* for InvalidateXX erase bkgnd flag */
	HBRUSH		paintBrush;	/* brush created to paint some controls */
	HPEN		paintPen;	/* pen created to paint some controls */
	MWCLIPREGION *	update;		/* update region in screen coords*/
	LONG_PTR		userdata;	/* setwindowlong user data*/
	LONG_PTR		userdata2;	/* additional user data (will remove)*/
	MWLISTHEAD  	props;		/* windows property list */
	MWSCROLLBARINFO	hscroll;	/* NC scrollbars*/
	MWSCROLLBARINFO	vscroll;
	HINSTANCE	hInstance;	/* hInstance */
	unsigned int     color_key;       /* color key for a layered window */
	unsigned int     alpha;           /* alpha value for a layered window */
	unsigned int     layered_flags;   /* flags for a layered window */
	int		nextrabytes;	/* # window extra bytes*/
	char		extrabytes[1];	/* window extra bytes - must be last*/
};

/* misc apis in winmain.c*/
VOID /*WINAPI*/ Sleep(DWORD dwMilliseconds);

#ifdef __cplusplus
}
#endif

#if WINEXTRA
#include "winextra.h"		/* extra windows stuff set by WINEXTRA=Y in config*/
#endif

#endif /* _WINDOWS_H*/
