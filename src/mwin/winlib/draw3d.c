#include "windows.h"
#include "wintools.h"
/*
 * WIN Draw Library
 *	Draw3dShadow - draws a shadow with bottom-left and top-right missing
 *	Draw3dBox - draws a complete shadow
 *	Draw3dInset - draw a 2 line 3d inset
 *	Draw3dOutset - draw a 2 line 3d outset
 */

/*
 * Draw3dShadow
 * 	NOINDENT_BLACK	T=white, B=black
 * 	NOINDENT_GRAY	T=white, B=dkgray
 * 	INDENT_BLACK	T=black, B=white
 * 	INDENT_GRAY		T=dkgray, B=white
 *
 *	TTTTTTTTTTTTTT
 *	T             B
 *	T             B
 *	 BBBBBBBBBBBBBB
 */
void WINAPI
Draw3dShadow(HDC hDC,int x,int y,int w,int h,COLORREF crTop,COLORREF crBottom)
{
	HPEN	hPenTop, hPenBottom, holdPen;

	hPenTop = CreatePen( PS_SOLID, 1, crTop);
	hPenBottom = CreatePen( PS_SOLID, 1, crBottom);
	holdPen = SelectObject( hDC, hPenTop);
	MoveToEx( hDC, x, y+h-2, NULL);
	LineTo( hDC, x, y);				/* left side*/
	LineTo( hDC, x+w-1, y);				/* top side*/

	SelectObject( hDC, hPenBottom);
	MoveToEx( hDC, x+w-1, y+1, NULL);
	LineTo( hDC, x+w-1, y+h-1);			/* right side*/
	LineTo( hDC, x, y+h-1);				/* bottom side*/

	SelectObject( hDC, holdPen);
	DeleteObject( hPenTop);
	DeleteObject( hPenBottom);
}

/*
 * Draw3dBox
 *
 *	TTTTTTTTTTTTTTB
 *	T             B
 *	T             B
 *	BBBBBBBBBBBBBBB
 */
void WINAPI
Draw3dBox(HDC hDC,int x,int y,int w,int h,COLORREF crTop,COLORREF crBottom)
{
	HPEN		hPenTop, hPenBottom, holdPen;

	hPenTop = CreatePen( PS_SOLID, 1, crTop);
	hPenBottom = CreatePen( PS_SOLID, 1, crBottom);
	holdPen = SelectObject( hDC, hPenTop);
	MoveToEx( hDC, x, y+h-2, NULL);
	LineTo( hDC, x, y);				/* left side*/
	MoveToEx( hDC, x, y, NULL);
	LineTo( hDC, x+w-1, y);				/* top side*/

	SelectObject( hDC, hPenBottom);
	MoveToEx( hDC, x+w-1, y, NULL);
	LineTo( hDC, x+w-1, y+h-1);			/* right side*/
	LineTo( hDC, x-1, y+h-1);			/* bottom side*/

	SelectObject( hDC, holdPen);
	DeleteObject( hPenTop);
	DeleteObject( hPenBottom);
}

/*
 * Draw 2 line deep 3d inset
 */
void WINAPI
Draw3dInset(HDC hDC,int x,int y,int w,int h)
{
	Draw3dBox(hDC, x, y, w, h,
		GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT));
	++x; ++y; w -= 2; h -= 2;
	Draw3dBox(hDC, x, y, w, h,
		GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_3DLIGHT));
}

/*
 * Draw 2 line deep 3d outset
 */
void WINAPI
Draw3dOutset(HDC hDC,int x,int y,int w,int h)
{
	Draw3dBox(hDC, x, y, w, h,
		GetSysColor(COLOR_3DLIGHT), GetSysColor(COLOR_WINDOWFRAME));
	++x; ++y; w -= 2; h -= 2;
	Draw3dBox(hDC, x, y, w, h,
		GetSysColor(COLOR_BTNHIGHLIGHT), GetSysColor(COLOR_BTNSHADOW));
}

/*
 * Draw 1 line pushed down rectangle
 */
void WINAPI
Draw3dPushDown(HDC hDC, int x, int y, int w, int h)
{
	Draw3dBox(hDC, x, y, w, h, GetSysColor(COLOR_BTNSHADOW),
		GetSysColor(COLOR_BTNSHADOW));
}

/*
 * Draw either 3d up or down depending on state
 */
void WINAPI
Draw3dUpDownState(HDC hDC, int x, int y, int w, int h, BOOL fDown)
{
	if (fDown)
		Draw3dPushDown(hDC, x, y, w, h);
	else Draw3dOutset(hDC, x, y, w, h);
}

void WINAPI
Draw3dUpFrame(HDC hDC, int l, int t, int r, int b)
{
	RECT	rc;
	HBRUSH	hbr;

	SetRect(&rc, l, t, r, b);
	Draw3dBox(hDC, rc.left, rc.top,
		rc.right-rc.left, rc.bottom-rc.top,
		GetSysColor(COLOR_3DLIGHT),
		GetSysColor(COLOR_WINDOWFRAME));
	InflateRect(&rc, -1, -1);
	Draw3dBox(hDC, rc.left, rc.top,
		rc.right-rc.left, rc.bottom-rc.top,
		GetSysColor(COLOR_BTNHIGHLIGHT),
		GetSysColor(COLOR_BTNSHADOW));
	InflateRect(&rc, -1, -1);

	hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	FillRect(hDC, &rc, hbr);
	DeleteObject(hbr);
}
