#ifndef _WINDOWS_H
#define _WINDOWS_H
/* windows.h*/
/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Win32 API master public header file
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "mwtypes.h"
#include "windef.h"
#include "wingdi.h"
#include "winfont.h"
#include "winkbd.h"
#include "winuser.h"	/* now includes winctl.h for resource compiler*/

/* external routines*/
int WINAPI 	WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    			LPSTR lpCmdLine, int nShowCmd);

int		MwUserInit(int ac, char **av);

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
	PMWFONT		pfont;		/* allocated font*/
	char		name[32];	/* font name (stock objects only)*/
} MWFONTOBJ;

typedef struct {
	MWGDIOBJHDR	hdr;
	int		width;		/* width*/
	int		height;		/* height*/
	int		planes;		/* # planes*/
	int		bpp;		/* bits per pixel*/
	int		linelen;	/* bytes per line*/
	int		size;		/* allocated size in bytes*/
	char 		bits[1];	/* beginning of bitmap*/
} MWBITMAPOBJ;

typedef struct {
	MWGDIOBJHDR   	hdr;
	MWCLIPREGION  *	rgn;		/* clip region*/
} MWRGNOBJ;

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
	int		drawmode;	/* rop2 drawing mode */
	POINT		pt;		/* current pen pos in client coords*/
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
	struct hwnd	*parent;	/* z-order parent window */
	struct hwnd	*owner;		/* owner window*/
	struct hwnd	*children;	/* first child window */
	struct hwnd	*siblings;	/* next sibling window */
	struct hwnd	*next;		/* next window in complete list */
	struct hcursor	*cursor;	/* cursor for this window */
	struct hdc *	owndc;		/* owndc if CS_OWNDC*/
	int		unmapcount;	/* count of reasons not really mapped */
	int		id;		/* window id */
	CHAR		szTitle[64];	/* window title*/
	int		gotPaintMsg;	/* window had WM_PAINT PostMessage*/
	int		paintSerial;	/* experimental serial # for alphblend*/
	int		paintNC;	/* experimental NC paint handling*/
	MWCLIPREGION *	update;		/* update region in screen coords*/
	DWORD		userdata;	/* setwindowlong user data*/
	DWORD		userdata2;	/* additional user data (will remove)*/
	MWSCROLLBARINFO	hscroll;	/* NC scrollbars*/
	MWSCROLLBARINFO	vscroll;
	int		nextrabytes;	/* # window extra bytes*/
	char		extrabytes[1];	/* window extra bytes - must be last*/
};

/* misc apis - will move to another header file*/
DWORD WINAPI	GetTickCount(VOID);
VOID WINAPI	Sleep(DWORD dwMilliseconds);

#ifdef __cplusplus
}
#endif

#endif /* _WINDOWS_H*/
