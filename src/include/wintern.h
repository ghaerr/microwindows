/* wintern.h*/
/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Microwindows internal routines header file
 */
#include <string.h>

#if (UNIX | DOS_DJGPP)
#define strcmpi	strcasecmp
#elif (VXWORKS)
int strcmpi(const char *s1, const char *s2);
#endif

#ifdef __PACIFIC__
#define strcmpi		stricmp
#endif  

#define DBLCLICKSPEED	750		/* mouse dblclik speed msecs (was 450)*/

/* gotPaintMsg values*/
#define PAINT_PAINTED		0	/* WM_PAINT msg has been processed*/
#define PAINT_NEEDSPAINT	1	/* WM_PAINT seen, paint when can*/
#define PAINT_DELAYPAINT	2	/* WM_PAINT seen,paint after user move*/

/* non-win32 api access for microwin*/
BOOL		MwSetDesktopWallpaper(PMWIMAGEHDR pImage);
void		MwRegisterFdInput(HWND hwnd,int fd);
void		MwUnregisterFdInput(HWND hwnd,int fd);
void		MwRegisterFdOutput(HWND hwnd,int fd);
void		MwUnregisterFdOutput(HWND hwnd,int fd);
void		MwRegisterFdExcept(HWND hwnd,int fd);
void		MwUnregisterFdExcept(HWND hwnd,int fd);

/* internal routines*/

/* winuser.c*/
PWNDCLASS 	MwFindClassByName(LPCSTR lpClassName);
void		MwDestroyWindow(HWND hwnd,BOOL bSendMsg);
HWND		MwGetTopWindow(HWND hwnd);
void		MwCalcClientRect(HWND hwnd);
void		MwSendSizeMove(HWND hwnd, BOOL bSize, BOOL bMove);
void		MwSetCursor(HWND wp, PMWCURSOR pcursor);

/* wingdi.c*/
#define MwIsClientDC(hdc)	(((hdc)->flags & DCX_WINDOW) == 0)
#define MwIsMemDC(hdc)		((hdc)->psd->flags == PSF_MEMORY)
void		MwPaintNCArea(HWND hwnd);
HWND		MwPrepareDC(HDC hdc);
void		MwSetClipWindow(HDC hdc);

/* winsbar.c*/
void		MwAdjustNCScrollbars(HWND hwnd);
void		MwPaintNCScrollbars(HWND hwnd, HDC hdc);
void		MwHandleNCMessageScrollbar(HWND hwnd, UINT msg, WPARAM hitcode,
			LPARAM lParam);

/* winexpos.c*/
void		MwRedrawScreen(void);
void		MwHideWindow(HWND hwnd,BOOL bChangeFocus,BOOL bSendMsg);
void		MwShowWindow(HWND hwnd,BOOL bSendMsg);
void		MwRaiseWindow(HWND hwnd);
void		MwLowerWindow(HWND hwnd);
BOOL		MwCheckOverlap(HWND topwp, HWND botwp);
void		MwClearWindow(HWND wp,MWCOORD x,MWCOORD y,MWCOORD width,
			MWCOORD height,BOOL exposeflag);
void		MwExposeArea(HWND wp, MWCOORD rootx,MWCOORD rooty,
			MWCOORD width,MWCOORD height);
/* winevent.c*/
BOOL		MwCheckMouseEvent(void);
BOOL		MwCheckKeyboardEvent(void);
void 		MwHandleMouseStatus(MWCOORD newx, MWCOORD newy, int newbuttons);
void		MwTranslateMouseMessage(HWND hwnd,UINT msg,int hittest);
void		MwDeliverMouseEvent(int buttons, int changebuttons,
			MWKEYMOD modifiers);
void		MwDeliverKeyboardEvent(MWKEY keyvalue, MWKEYMOD modifiers,
			MWSCANCODE scancode, BOOL pressed);
void		MwDeliverExposureEvent(HWND wp, MWCOORD x, MWCOORD y,
			MWCOORD width,MWCOORD height);
void		MwUnionUpdateRegion(HWND wp, MWCOORD x, MWCOORD y,
			MWCOORD width,MWCOORD height, BOOL bUnion);
void		MwMoveCursor(MWCOORD x, MWCOORD y);
void		MwCheckCursor(void);
HWND		MwFindVisibleWindow(MWCOORD x, MWCOORD y);
void		MwCheckMouseWindow(void);
int		strzcpy(char *dst,const char *src,int dstsiz);

/* winuser.c*/
extern int	mwSYSMETRICS_CYCAPTION;
extern int	mwSYSMETRICS_CXBORDER;
extern int	mwSYSMETRICS_CYBORDER;
extern int	mwSYSMETRICS_CXFRAME;
extern int	mwSYSMETRICS_CYFRAME;
extern int	mwSYSMETRICS_CXDOUBLECLK;
extern int	mwSYSMETRICS_CYDOUBLECLK;
extern int	mwSYSMETRICS_CYHSCROLL;
extern int	mwSYSMETRICS_CXHSCROLL;
extern int	mwSYSMETRICS_CXVSCROLL;
extern int	mwSYSMETRICS_CYVSCROLL;

/* wingdi.c*/
extern BOOL	mwERASEMOVE;	/* default repaint algorithm*/

/* winmain.c*/
int		MwOpen(void);
void		MwClose(void);
void		MwSelect(void);
int		MwInitialize(void);
void		MwTerminate(void);
extern	HWND	listwp;			/* list of all windows */
extern	HWND	rootwp;			/* root window pointer */
extern	HWND	focuswp;		/* focus window for keyboard */
extern	HWND	mousewp;		/* window mouse is currently in */
extern	HWND	capturewp;		/* capture window*/
extern  HWND	dragwp;			/* window user is dragging*/
extern	HCURSOR	curcursor;		/* currently enabled cursor */
extern	MWCOORD	cursorx;		/* x position of cursor */
extern	MWCOORD	cursory;		/* y position of cursor */
extern	MWSCREENINFO	sinfo;		/* screen information */
extern  DWORD	startTicks;		/* tickcount on startup */
extern  int	mwpaintNC;		/* experimental nonclient regions*/
extern  BOOL	mwforceNCpaint;		/* force NC paint for alphablend*/

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
