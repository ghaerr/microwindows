/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Caret control for Microwindows win32 api.
 *
 * TODO: add SetSysTimer for blinking
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#ifndef __ECOS
#include <malloc.h>
#endif
#include "windows.h"
#include "device.h"

#define DEF_BLINK_TIME		500	/* default blink time in ms*/

typedef struct {
	HWND	hwnd;		/* != NULL if caret is created*/
	int	x;
	int	y;
	int	nWidth;
	int	nHeight;
	BOOL	fShown;		/* caret is currently visible*/
	int	nShowCount;	/* <= 0 for hidden caret*/
	UINT	nBlinkTime;
} CARETINFO;

/* local data*/
static CARETINFO sysCaret;	/* the system caret*/

/* local procs*/
static void MwShowCaret(void);
static void MwHideCaret(void);
static void MwUpdateCaret(void);

BOOL WINAPI
CreateCaret(HWND hwnd, HBITMAP hBitmap, int nWidth, int nHeight)
{

	DestroyCaret();			/* destroy old caret if any*/

	if (nWidth <= 0)
		nWidth = 1;
	if (nHeight <= 0)
		nHeight = 1;

        sysCaret.hwnd = hwnd;
        sysCaret.x = 0;
	sysCaret.y = 0;
	sysCaret.nWidth = nWidth;
        sysCaret.nHeight = nHeight;
        sysCaret.fShown = FALSE;
        sysCaret.nShowCount = 0;
        sysCaret.nBlinkTime = DEF_BLINK_TIME;
	return TRUE;
}

BOOL WINAPI
DestroyCaret(VOID)
{
	if (sysCaret.fShown)
		MwHideCaret();
	sysCaret.hwnd = NULL;
	sysCaret.fShown = FALSE;
	return TRUE;
}

BOOL WINAPI
HideCaret(HWND hwnd)
{
	if (hwnd == NULL)
		hwnd = sysCaret.hwnd;
	if (hwnd == NULL || hwnd != sysCaret.hwnd)
		return FALSE;

	/* hide caret if this call made it invisible*/
	if (--sysCaret.nShowCount == 0) {
		MwHideCaret();
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI
ShowCaret(HWND hwnd)
{
	if (hwnd == NULL)
		hwnd = sysCaret.hwnd;
	if (hwnd == NULL || hwnd != sysCaret.hwnd || sysCaret.nShowCount < 0)
		return FALSE;

	if (++sysCaret.nShowCount > 1)
		return TRUE;

	/* show caret, this call made it visible*/
	MwShowCaret();
	return TRUE;
}

BOOL WINAPI
SetCaretPos(int nX, int nY)
{
	if (sysCaret.fShown && (sysCaret.x != nX || sysCaret.y != nY)) {
		MwUpdateCaret();	/* toggle off*/
		sysCaret.x = nX;
		sysCaret.y = nY;
		MwUpdateCaret();	/* toggle on in new location*/
		return TRUE;
	}
	sysCaret.x = nX;
	sysCaret.y = nY;
	return TRUE;
}

BOOL WINAPI
GetCaretPos(LPPOINT lpPoint)
{
	lpPoint->x = sysCaret.x;
	lpPoint->y = sysCaret.y;
	return TRUE;
}

UINT WINAPI
GetCaretBlinkTime(VOID)
{
	return sysCaret.nBlinkTime;
}

BOOL WINAPI
SetCaretBlinkTime(UINT uMSeconds)
{
	sysCaret.nBlinkTime = uMSeconds;
	/* SetSysTimer */
	return TRUE;
}

static void
MwShowCaret(void)
{
	if (sysCaret.fShown)
		return;
	MwUpdateCaret();
	sysCaret.fShown = TRUE;
}

static void
MwHideCaret(void)
{
	if (!sysCaret.fShown)
		return;
	MwUpdateCaret();
	sysCaret.fShown = FALSE;
}

/* Draw the caret using XOR.  Same routine is used to show and hide caret.*/
static void
MwUpdateCaret(void)
{
	int	oldmode;
	HDC	hdc;
	HPEN	hpen;
	HBRUSH	hbr;

	oldmode = GdSetMode(MWMODE_XOR);
	hdc = GetDC(sysCaret.hwnd);
	hpen = SelectObject(hdc, GetStockObject(WHITE_PEN));

	/* it seems there's some problems with Rectangle with nWidth == 1*/
	if (sysCaret.nWidth == 1) {
		MoveToEx(hdc, sysCaret.x, sysCaret.y, NULL);
		LineTo(hdc, sysCaret.x, sysCaret.y+sysCaret.nHeight);
	} else {
		hbr = SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		Rectangle(hdc, sysCaret.x, sysCaret.y,
			sysCaret.x+sysCaret.nWidth,
			sysCaret.y+sysCaret.nHeight);
		SelectObject(hdc, hbr);
	}
	SelectObject(hdc, hpen);
	ReleaseDC(sysCaret.hwnd, hdc);
	GdSetMode(oldmode);
}
