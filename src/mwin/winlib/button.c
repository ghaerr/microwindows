#include "windows.h"
#include "windowsx.h"
#include "wintools.h"
#include "wintern.h"
/*
 * Copyright (c) 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * WINCTL Custom Control Library
 * Push button Custom Control
 *	This control implements a custom pushbutton control.
 *
 * 4/8/98 g haerr original version from control palette v2.00, Blaise Computing
 */

/*
** Modify records:
**
**  Who             When        Where       For What                Status
**-----------------------------------------------------------------------------
 * Gabriele Brugnoni 2003/08/30 Italy      WM_SETFONT implementation
 * Gabriele Brugnoni 2003/08/30 Italy      Accelerator chr '&'
 * Gabriele Brugnoni 2003/08/30 Italy      fixed some mouse events
 * Gabriele Brugnoni 2003/08/30 Italy      Modified WM_GETDLGCODE event.
 * Gabriele Brugnoni 2003/08/30 Italy      CheckRadioButton moved on windlg.c
 * Gabriele Brugnoni 2004/07/13 Italy      Radio button vertical centered
 * Ludwig Ertl       2010/04/27 Austria    Support for BS_PUSHLIKE
 */

#define CONFIG_AUTORADIOBUTTONSIZE

// GetWindowLong offsets
#define PBL_STATE		(0)
#define PBL_CAPTURE		(PBL_STATE+sizeof(LONG))
#define PBL_WASINSIDE		(PBL_CAPTURE+sizeof(LONG))
#define PBL_DELETEFONT		(PBL_WASINSIDE+sizeof(LONG))
#define PBL_FONT		(PBL_DELETEFONT+sizeof(LONG))
#define PBL_WND_FONT		(PBL_FONT+sizeof(LONG_PTR))
#define PBL_TXTLEFTTOP		(PBL_WND_FONT+sizeof(LONG_PTR))
#define PBL_TXTRIGHTBOTTOM	(PBL_TXTLEFTTOP+sizeof(LONG))
#define PB_EXTRABYTES		(PBL_TXTRIGHTBOTTOM+sizeof(LONG))

#define GET_PBSTATE(h)			(GetWindowLong(h, PBL_STATE))
#define GET_PBCAPTURE(h)		(GetWindowLong(h, PBL_CAPTURE))
#define GET_PBWASINSIDE(h)		(GetWindowLong(h, PBL_WASINSIDE))
#define GET_PBDELETEFONT(h)		(GetWindowLong(h, PBL_DELETEFONT))
#define GET_PBFONT(h)			(GetWindowLongPtr(h, PBL_FONT))
#define GET_WND_FONT(h)			((HFONT)(LONG_PTR)GetWindowLongPtr(h, PBL_WND_FONT))
#define GET_PBTXTRECT(h,t)		{(t).left=GetWindowLong(h, PBL_TXTLEFTTOP); (t).right=GetWindowLong(h, PBL_TXTRIGHTBOTTOM);}

#define SET_PBSTATE(h,x)		(SetWindowLong(h, PBL_STATE, x))
#define SET_PBCAPTURE(h,x)		(SetWindowLong(h, PBL_CAPTURE, x))
#define SET_PBWASINSIDE(h,x)		(SetWindowLong(h, PBL_WASINSIDE, x))
#define SET_PBDELETEFONT(h,x)		(SetWindowLong(h, PBL_DELETEFONT, x))
#define SET_PBFONT(h,x)			(SetWindowLongPtr(h, PBL_FONT, x))
#define SET_WND_FONT(h, f)		(SetWindowLongPtr(h, PBL_WND_FONT, (LONG_PTR)(HFONT)(f)))
#define SET_PBTXTRECT(h,t)		{ SetWindowLong(h, PBL_TXTLEFTTOP, MAKELONG((t).left, (t).top)); SetWindowLong(h, PBL_TXTRIGHTBOTTOM, MAKELONG((t).right, (t).bottom)); }

#define PARENT(hwnd)		((HWND)(LONG_PTR)GetWindowLongPtr(hwnd,GWL_HWNDPARENT))

/* Internal state variable bit positions				*/
#define PUSH_UP		0x0000
#define PUSH_DOWN	0x0001	/* Button is down			*/
#define PUSH_FOCUS	0x0002	/* Button is focused			*/
#define PUSH_DISABLED	0x0004	/* Button is disabled			*/
#define PUSH_DEFAULT	0x0008	/* Button is currently a default	*/
#define PUSH_CHECKED	0x0010

/* Push Button states */
#define PBS_UP		0x0000			/* Normal button state.	*/
#define PBS_FOCUSDOWN	0x0001			/* Button pressed.	*/
#define PBS_FOCUSUP	0x0002			/* Focused state.	*/
#define PBS_DISABLED	0x0004			/* Disabled state.	*/
#define PBS_DEFAULT	0x0008			/* Default state.	*/
#define PBS_CHECKED	0x0010			/* checked state.	*/

#define WM_PAINT_SPECIAL	WM_PAINT
#define HANDLE_WM_PAINT_SPECIAL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd),(HDC)(wParam)), 0L)

/* BOOL Cls_OnGetState( HWND hwnd); */
#define HANDLE_BM_GETSTATE(hwnd, wParam, lParam, fn) ((fn)(hwnd))
#define FORWARD_BM_GETSTATE(hwnd) \
	(LONG)(fn)((hwnd), BM_GETSTATE, (WPARAM)0, (LPARAM)0)

/* void Cls_OnSetState( HWND hwnd, WORD wState); */
#define HANDLE_BM_SETSTATE( hwnd, wParam, lParam, fn) \
	((fn)((hwnd), (WORD)wParam), 0)
#define FORWARD_BM_SETSTATE( hwnd, wState) \
	(fn)((hwnd), BM_SETSTATE, (WPARAM)wState, (LPARAM)0)

/* void Cls_OnSetStyle( HWND hwnd, WORD style, BOOL bRedraw); */
#define HANDLE_BM_SETSTYLE( hwnd, wParam, lParam, fn) \
	((fn)((hwnd), (WORD)wParam, (BOOL)LOWORD(lParam)), 0)
#define FORWARD_BM_SETSTYLE( hwnd, style, bRedraw, fn) \
	(fn)((hwnd), BM_SETSTYLE, (WPARAM)style, MAKELPARAM(bRedraw, 0))


/* local procs*/
static void cenButton_FnEnd( HWND, WORD);
static WORD cenButton_FnStart( HWND);
static BOOL cenButton_OnCreate( HWND, LPCREATESTRUCT);
/*static void cenButton_OnDestroy( HWND);*/
/*static void cenButton_OnEnable( HWND, BOOL);*/
static BOOL cenButton_OnEraseBkgnd( HWND, HDC);
static UINT cenButton_OnGetDlgCode( HWND, LPMSG);
static LONG cenButton_OnGetState( HWND);
/*static void cenButton_OnKey( HWND, UINT, BOOL, int, UINT);*/
static void cenButton_OnKillFocus( HWND, HWND);
static void cenButton_OnLButtonDown( HWND, BOOL, UINT, UINT, UINT);
static void cenButton_OnLButtonUp( HWND, UINT, UINT, UINT);
static void cenButton_OnMouseMove( HWND, UINT, UINT, UINT);
static void cenButton_OnPaint( HWND, HDC);
static void DrawPushButton(HWND hwnd,HDC hDCwParam,UINT wEnumState, DWORD dwStyle);
static void DrawGroupBox(HWND hwnd,HDC hDCwParam, DWORD dwStyle);
static void cenButton_OnSetFocus( HWND, HWND);
static void cenButton_OnSetStyle( HWND, WORD, BOOL);
static void cenButton_OnSetState( HWND, WORD);
static void cenButton_SetState( HWND, WORD, BOOL);
static void cenButton_OnSetText( HWND, LPCSTR);

LRESULT CALLBACK cenButtonWndFn(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void
cenButton_FnEnd(
HWND		hwnd,
WORD		wState)
{
	if( wState != GET_PBSTATE( hwnd)) {
		if( IsWindowVisible( hwnd))
			UpdateWindow( hwnd);
	}
}

static WORD
cenButton_FnStart(
HWND		hwnd)
{
	return GET_PBSTATE( hwnd);
}

static BOOL
cenButton_OnCreate(
HWND			hwnd,
LPCREATESTRUCT	lpCreate)
{
	/* Set initial states */
	/*SET_PBDELETEFONT( hwnd, FALSE);*/
    	/*SET_PBFONT( hwnd, NULL);*/
	SET_PBSTATE( hwnd, PUSH_UP );
	SET_PBCAPTURE( hwnd, FALSE );
	SET_PBWASINSIDE( hwnd, FALSE );
	SET_WND_FONT ( hwnd, GetStockObject(DEFAULT_GUI_FONT) );

	if ((lpCreate->style & 0x0f) == BS_DEFPUSHBUTTON)
		cenButton_SetState( hwnd, PUSH_DEFAULT, TRUE );

	if (lpCreate->style & WS_DISABLED)
		cenButton_SetState( hwnd, PUSH_DISABLED, TRUE );

	return( TRUE);
}

#if 0
static void
cenButton_OnDestroy(
HWND		hwnd)
{
	if( GET_PBDELETEFONT( hwnd)) {
		DeleteObject( GET_PBFONT( hwnd));
		SET_PBDELETEFONT( hwnd, FALSE);
	}
}

static void
cenButton_OnEnable(
HWND		hwnd,
BOOL		bEnable)
{
	WORD		wState;

	wState = cenButton_FnStart( hwnd);
	cenButton_SetState( hwnd, PUSH_DISABLED, !bEnable);
	cenButton_FnEnd( hwnd, wState);
}
#endif

static BOOL
cenButton_OnEraseBkgnd(
HWND		hwnd,
HDC			hdc)
{
	/* Background is erased at WM_PAINT time, so return TRUE*/
	return TRUE;
}

static UINT
cenButton_OnGetDlgCode(
HWND	hwnd,
LPMSG 	lpMsg)
{
	UINT flags = 0;
	UINT type;
	DWORD dwStyle;

	/* WM_GETDLGCODE is sent by the dialog manager to find	   */
	/* what type/style of control is responding and/or to	   */
	/* determine what keystrokes the control wants to process  */
	/* itself.	In this case, the pushbutton identifies itself */
	/* and also indicates whether it is currently the default  */
	/* pushbutton.											   */
	/* If lpMsg is not null and message is WM_CHAR,            */
	/* checks if the char is ' ' signal that we need it.       */
	if (lpMsg) {
		if (lpMsg->message == WM_CHAR && lpMsg->wParam == ' ')
			flags = DLGC_WANTCHARS;
		else if (lpMsg->message == WM_KEYDOWN &&
			     (lpMsg->wParam == VK_MBUTTON || lpMsg->wParam == VK_RETURN))
				  flags = DLGC_WANTMESSAGE;
	}

	dwStyle = GetWindowLong ( hwnd, GWL_STYLE );
	switch((int)(dwStyle & 0x0f)) {
		case BS_AUTORADIOBUTTON:
		case BS_RADIOBUTTON:
			type = DLGC_RADIOBUTTON;
			break;
		case BS_GROUPBOX:
			type = DLGC_STATIC;
			break;
		default:
			type = DLGC_BUTTON;
			break;
	}

	return( flags | type | ((GET_PBSTATE( hwnd) & PUSH_DEFAULT) ?
				DLGC_DEFPUSHBUTTON : DLGC_UNDEFPUSHBUTTON));
}

static LONG
cenButton_OnGetState(
HWND		hwnd)
{
	/* BM_GETSTATE is sent to enquire about the state of the   */
	/* control.	 It returns TRUE if the button is in the down  */
	/* state.												   */

	return( ( GET_PBSTATE( hwnd) & PUSH_DOWN) == PUSH_DOWN);
}

#if 0
static void
cenButton_OnKey(
HWND		hwnd,
UINT		vk,
BOOL		bDown,
int			cRepeat,
UINT		flag)
{
	WORD		wState;

	wState = cenButton_FnStart( hwnd);
	if (bDown) {
	 	/* WM_KEYDOWN is sent when a non-system key is pressed.	   */
		/* If a spacebar is detected and the previous key state	   */
		/* was up, then the control should switch to the down	   */
		/* state.												   */

		if ( (vk == ' ') && !(HIBYTE(flag) & 0x40) )
			cenButton_SetState( hwnd, PUSH_DOWN, TRUE );
	}
	else {
		/* WM_KEYUP is sent when a non-system key is released.	   */
		/* If a space bar is detected, change to the up state.	If */
		/* the control is the focused control, send the BN_CLICKED */
		/* notification message.								   */

		if ( vk == ' ' )
		{	cenButton_SetState( hwnd, PUSH_DOWN, FALSE );

			if (GET_PBSTATE( hwnd) & PUSH_FOCUS) {
				FORWARD_WM_COMMAND( PARENT( hwnd), GetDlgCtrlID( hwnd),
					hwnd, BN_CLICKED, SendMessage);
				if(!IsWindow(hwnd))
					return;
			}
		}
	}
	cenButton_FnEnd( hwnd, wState);
}
#endif

static void
cenButton_OnKillFocus(
HWND		hwnd,
HWND		hwndNewFocus)
{
	WORD		wState;

	wState = cenButton_FnStart( hwnd);
	cenButton_SetState( hwnd, PUSH_FOCUS, FALSE );
	cenButton_FnEnd( hwnd, wState);

	if( (hwnd->szTitle[0] != 0) && ((hwnd->style & (BS_FLAT/*|BS_NOFOCUSRECT*/)) == 0) )
		{
		RECT rect;
		HDC hdc = GetDC ( hwnd );
		GET_PBTXTRECT ( hwnd, rect );
		rect.top = HIWORD(rect.left); rect.left=LOWORD(rect.left);
		rect.bottom = HIWORD(rect.right); rect.right=LOWORD(rect.right);
		DrawFocusRect ( hdc, &rect );
		ReleaseDC ( hwnd, hdc );
		}
}

static void
cenButton_OnLButtonDown(
HWND		hwnd,
BOOL		bDblClick,
UINT		x,
UINT		y,
UINT		keyState)
{
	WORD		wState;

	if( !IsWindowEnabled(hwnd) ) return;

	wState = cenButton_FnStart( hwnd);
	/* capture the mouse*/
	SetCapture( hwnd );
	/* set focus to ourself*/
	SetFocus( hwnd );
	SET_PBCAPTURE( hwnd, TRUE );
	SET_PBWASINSIDE( hwnd, TRUE );
	/* set down state*/
	cenButton_SetState( hwnd, PUSH_DOWN, TRUE );
	cenButton_FnEnd( hwnd, wState);
}

static void
cenButton_OnLButtonUp(
HWND		hwnd,
UINT		x,
UINT		y,
UINT		keys)
{
	WORD		wState;
	DWORD		dwStyle;

	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	wState = cenButton_FnStart( hwnd);

	/* only draw up state if we captured mouse*/
	if(GetCapture() == hwnd)
		cenButton_SetState( hwnd, PUSH_DOWN, FALSE );
	/* release mouse capture*/
	ReleaseCapture();

	/* if cursor is inside control, send clicked notification to parent*/
	if (GET_PBWASINSIDE( hwnd)) {
		switch((int)(dwStyle & 0x0f)) {
	        case BS_AUTOCHECKBOX:
			SendMessage(hwnd,BM_SETCHECK,(wState & PBS_CHECKED)?0:1,0L);
			break;
	        case BS_AUTORADIOBUTTON:
			CheckRadioButton(PARENT(hwnd),-1,-1,hwnd->id);
			break;
		}
		FORWARD_WM_COMMAND( PARENT( hwnd), GetDlgCtrlID( hwnd), hwnd,
			BN_CLICKED, SendMessage);
		if(!IsWindow(hwnd))
			return;
	}

	SET_PBCAPTURE( hwnd, FALSE );
	SET_PBWASINSIDE( hwnd, FALSE );
	cenButton_FnEnd( hwnd, wState);
}

static void
cenButton_OnMouseMove(
HWND		hwnd,
UINT		x,
UINT		y,
UINT		keys)
{
	/* WM_MOUSEMOVE is sent at every discernable mouse		   */
	/* movement.  It is necessary to detect this because if	   */
	/* the mouse has been captured (because of a button down   */
	/* message), the location of the cursor needs to be		   */
	/* tracked.	 If it moves out of the confines of the		   */
	/* control, the control should change to the focus/up	   */
	/* state (and retain capture.)	If the cursor then moves   */
	/* back into the control, change back to the down state.   */

	WORD		wState;
	
	wState = cenButton_FnStart( hwnd);
	if( GET_PBCAPTURE( hwnd)) {
		if( !PtInsideWindow( hwnd, x, y) ) {
			if( GET_PBWASINSIDE( hwnd)) {
				cenButton_SetState( hwnd, PUSH_DOWN, FALSE);
				SET_PBWASINSIDE( hwnd, FALSE );
				ReleaseCapture();
				cenButton_FnEnd( hwnd, wState);
			}
		} else {
			if( !GET_PBWASINSIDE( hwnd) ) {
				cenButton_SetState( hwnd, PUSH_DOWN, TRUE );
				SET_PBWASINSIDE( hwnd, TRUE );
				GetCapture();
				cenButton_FnEnd( hwnd, wState);
			}
		}
	}
	cenButton_FnEnd( hwnd, wState);

}

static void
cenButton_OnPaint(
HWND		hwnd,
HDC		hDCwParam)
{
	UINT		wEnumState;
	DWORD		dwStyle;

	if( GET_PBSTATE( hwnd) & PUSH_DISABLED)
		wEnumState = PBS_DISABLED;
	else if( GET_PBSTATE( hwnd) & PUSH_DOWN)
		wEnumState = PBS_FOCUSDOWN;
	else if( GET_PBSTATE( hwnd) & PUSH_CHECKED)
		wEnumState = PBS_CHECKED;
	else
	{
		if( GET_PBSTATE( hwnd) & PUSH_FOCUS)
			wEnumState = PBS_FOCUSUP;
		else
			wEnumState = PBS_UP;
		if( GET_PBSTATE( hwnd) & PUSH_DEFAULT)
			wEnumState |= PBS_DEFAULT;
	}

	/* common draw code for button and checkbox*/
	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	switch((int)(dwStyle & 0x0f)) {
	case BS_GROUPBOX:
		DrawGroupBox( hwnd, hDCwParam, dwStyle);
		break;
	default:
		DrawPushButton( hwnd, hDCwParam, wEnumState, dwStyle);
	}
}

static void
DrawPushButton(HWND hwnd,HDC hDCwParam,UINT wEnumState,DWORD dwStyle)
{
	HDC		hdc;
	RECT		rect;
	RECT		rectClient;
	RECT		rectSave;
	RECT		rc;
	int		iFaceOffset = 0;
	INT		uiHeight;
	INT		uiWidth;
	COLORREF	crOld;
	COLORREF	crBkOld;
	int		oldBkMode;
	HFONT		hNewFont;
	HFONT		hOldFont;
	HPEN		hOldPen;
	COLORREF	hOldColor;
	PAINTSTRUCT	ps;
	char		buf[256];
	int			n;
#define uiWidthFrame	0
#define uiWidthShadow	2

	hdc = BeginPaint(hwnd, &ps);
	if(!hdc)
		goto Return;

	GetWindowText(hwnd, buf, sizeof(buf));
	GetClientRect( hwnd, &rectClient );
	uiWidth	= rectClient.right - rectClient.left;
	uiHeight = rectClient.bottom - rectClient.top;

	crOld = SetTextColor( hdc, GetSysColor( COLOR_BTNTEXT));
	crBkOld = SetBkColor( hdc, GetSysColor( COLOR_BTNFACE));

   /* "Convert" pushlike buttons to pushbuttons */
   if (dwStyle & BS_PUSHLIKE)
      dwStyle &= ~0x0F;

	rc = rectClient;
	switch((int)(dwStyle & 0x0f)) {
	case BS_PUSHBUTTON:
	case BS_DEFPUSHBUTTON:
		if( (wEnumState & PBS_FOCUSDOWN) || (wEnumState & PBS_CHECKED)) {
			if(dwStyle & BS_BITMAP)
				DrawDIB(hdc, rc.left+1, rc.top+1, (PMWIMAGEHDR)hwnd->userdata);
			Draw3dBox(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
				GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_WINDOWFRAME));
			InsetR(&rc, 1, 1);
			Draw3dBox(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
				GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNSHADOW));
			InsetR(&rc, 1, 1);
			if(!(dwStyle & BS_BITMAP))
				FastFillRect(hdc, &rc, GetSysColor(COLOR_BTNFACE));
			iFaceOffset = 1;
		} else {
			if(dwStyle & BS_BITMAP)
				DrawDIB(hdc, rc.left, rc.top, (PMWIMAGEHDR)hwnd->userdata);
			if(wEnumState & PBS_DEFAULT) {
				Draw3dBox(hdc, rc.left, rc.top,
					rc.right-rc.left, rc.bottom-rc.top,
					GetSysColor(COLOR_WINDOWFRAME),
					GetSysColor(COLOR_WINDOWFRAME));
				InsetR(&rc, 1, 1);
			}
			Draw3dBox(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
				GetSysColor(COLOR_BTNHIGHLIGHT),GetSysColor(COLOR_WINDOWFRAME));
			InsetR(&rc, 1, 1);
			Draw3dBox(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
				GetSysColor(COLOR_3DLIGHT), GetSysColor(COLOR_BTNSHADOW));
			InsetR(&rc, 1, 1);
			if(!(dwStyle & BS_BITMAP))
				FastFillRect(hdc, &rc, GetSysColor(COLOR_BTNFACE));
			iFaceOffset = 0;
		}
		break;

	case BS_CHECKBOX:
	case BS_AUTOCHECKBOX:
		FastFillRect(hdc, &rc, GetSysColor(COLOR_BTNFACE));
		/*rc.left += 2;*/
		/*rc.top += 2;*/
		rc.right = rc.left + 12;
		rc.bottom = rc.top + 12;
		/*Draw3dBox(hdc, rc.left, rc.top, 8, 8,
			GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_3DLIGHT));*/
		Draw3dInset(hdc, rc.left, rc.top, 12, 12);
		InsetR(&rc, 2, 2);
		FastFillRect(hdc, &rc, GetSysColor(COLOR_BTNHIGHLIGHT));
		iFaceOffset = 1;
		if(wEnumState & PBS_CHECKED) {
			MoveToEx(hdc, rc.left, rc.top,NULL);
			LineTo(hdc, rc.right, rc.bottom);
			MoveToEx(hdc, rc.left, rc.bottom,NULL);
			LineTo(hdc, rc.right, rc.top);
		}
		break;

       	case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
		FastFillRect(hdc, &rc, GetSysColor(COLOR_BTNFACE));
#ifdef CONFIG_AUTORADIOBUTTONSIZE
		n = (uiHeight >= 20) ? 18 : 10;
#else
		n = 10;
#endif
		rc.left = 0;
		rc.top += (uiHeight - n) / 2;
		rc.right = rc.left + n;
		rc.bottom = rc.top + n;

		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		hOldPen = SelectObject(hdc, CreatePen(PS_SOLID, 1,
			GetSysColor(COLOR_BTNSHADOW)));
		SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		Ellipse(hdc,rc.left,rc.top, rc.right,rc.bottom);
		InsetR(&rc, 1, 1);

		SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		DeleteObject(SelectObject(hdc,
			CreatePen(PS_SOLID, 1,GetSysColor(COLOR_WINDOWFRAME))));
		Ellipse(hdc,rc.left,rc.top, rc.right,rc.bottom);
		DeleteObject(SelectObject(hdc, hOldPen));

		iFaceOffset = 0;
		if(wEnumState & PBS_CHECKED) {
			SelectObject ( hdc, GetStockObject(BLACK_BRUSH) );
			Ellipse(hdc, rc.left+2, rc.top+2, rc.right-2,
					rc.bottom-2);
			}
		break;
	}

	/*
	 * draw text
	 */
	if(buf[ 0]) {
		hNewFont = GET_WND_FONT ( hwnd );
		hOldFont = SelectObject( hdc, hNewFont);

		/* calculate text bounding rect*/
		rect.top = rect.left = rect.bottom = rect.right = 0;
		DrawText( hdc, buf, -1, &rect, DT_CALCRECT | DT_LEFT |
			DT_SINGLELINE | DT_TOP);
		rectSave = rect;

		/*
		 * calculate text draw location
		 */
		switch((int)(dwStyle & (BS_LEFT|BS_CENTER|BS_RIGHT))) {
		case BS_CENTER:
		default:
			rect.left = (uiWidth - (rect.right - rect.left)) / 2
				+ iFaceOffset;
			break;
		case BS_LEFT:
			rect.left = uiWidthFrame + uiWidthShadow + 2
				+ iFaceOffset;
			break;
		case BS_RIGHT:
			rect.left = uiWidth - ((rect.right - rect.left) + uiWidthFrame
				+ uiWidthShadow + 4 + iFaceOffset);
			break;
		}

		switch((int)(dwStyle & 0x0f)) {
		case BS_CHECKBOX:
		case BS_AUTOCHECKBOX:
		case BS_AUTORADIOBUTTON:
		case BS_RADIOBUTTON:
			rect.left = uiHeight+2;
			break;
		}

		rect.right += rect.left - rectSave.left;

		switch((int)(dwStyle & (BS_TOP|BS_VCENTER|BS_BOTTOM))) {
		case BS_VCENTER:
		default:
			rect.top = (uiHeight - (rect.bottom - rect.top)) / 2
				+ iFaceOffset;
			break;
		case BS_TOP:
			rect.top = 2 + uiWidthFrame + uiWidthShadow
				+ iFaceOffset;
			break;
		case BS_BOTTOM:
			rect.top = uiHeight - uiWidthFrame - uiWidthShadow -
				(rect.bottom - rect.top) - 1 + iFaceOffset;
			break;
		}
		switch((int)(dwStyle & 0x0f)) {
		case BS_CHECKBOX:
		case BS_AUTOCHECKBOX:
		case BS_AUTORADIOBUTTON:
		case BS_RADIOBUTTON:
			rect.top = 0;
			break;
		}
		rect.bottom += rect.top - rectSave.top;

		oldBkMode = SetBkMode( hdc, TRANSPARENT);
		if( (wEnumState & PBS_DISABLED) || !IsWindowEnabled(hwnd) )
			hOldColor = SetTextColor( hdc,
				GetSysColor( COLOR_GRAYTEXT));
		else
			hOldColor = SetTextColor( hdc,
				GetSysColor( COLOR_BTNTEXT));

		SET_PBTXTRECT ( hwnd, rect );
		DrawText( hdc, buf, -1, &rect,DT_LEFT | DT_SINGLELINE | DT_TOP);

		if( (GetFocus() == hwnd) )
			DrawFocusRect ( hdc, &rect );

		SetBkMode( hdc, oldBkMode);
		SetTextColor( hdc, hOldColor);
		SelectObject( hdc, hOldFont);
	}

#if 0
	if( (!(wEnumState&PBS_CHECKED) && (wEnumState&PBS_FOCUSDOWN)) ||
						(wEnumState & PBS_FOCUSUP)) {
		rect = rectClient;
		uiWidth = uiWidthFrame + uiWidthShadow + 2;
		rect.left += uiWidth;
		rect.top += uiWidth;
		rect.right -= uiWidth;
		rect.bottom -= uiWidth;
		if((dwStyle & (BS_FLAT|BS_NOFOCUSRECT)) == 0)
			DrawFocusRect( hdc, &rect);
	}
#endif

	SetTextColor( hdc, crOld);
	SetBkColor( hdc, crBkOld);

Return:
	EndPaint(hwnd, &ps);
}

static void
cenButton_OnSetFocus(
HWND		hwnd,
HWND		hwndOldFocus)
{
	/* WM_SETFOCUS is sent when the user clicks on the control */
	/* or when the dialog manager determines that a keystroke  */
	/* should cause the control to be focused.	This affects   */
	/* the appearance of the control so the state is saved for */
	/* future drawing.										   */

	WORD		wState;

	wState = cenButton_FnStart( hwnd);
	/*if(!IsWindowEnabled(hwnd))
		cenButton_SetState( hwnd, PUSH_FOCUS, TRUE );*/
	cenButton_FnEnd( hwnd, wState);

	if(!IsWindowEnabled(hwnd)) return;

	if( (hwnd->szTitle[0] != 0) && ((hwnd->style & (BS_FLAT/*|BS_NOFOCUSRECT*/)) == 0) )
		{
		RECT rect;
		HDC hdc = GetDC ( hwnd );
		GET_PBTXTRECT ( hwnd, rect );
		rect.top = HIWORD(rect.left); rect.left=LOWORD(rect.left);
		rect.bottom = HIWORD(rect.right); rect.right=LOWORD(rect.right);
		DrawFocusRect ( hdc, &rect );
		ReleaseDC ( hwnd, hdc );
		}
}

static void
cenButton_OnSetStyle(
HWND		hwnd,
WORD		style,
BOOL		bRedraw)
{
	WORD		wState;

	wState = cenButton_FnStart( hwnd);
	cenButton_SetState( hwnd, PUSH_DEFAULT,
		(style & 0x0f) == BS_DEFPUSHBUTTON);
	cenButton_FnEnd( hwnd, wState);
}

static void
cenButton_OnSetState(
HWND		hwnd,
WORD		wState)
{
	WORD		wStateOld;

	wStateOld = cenButton_FnStart( hwnd);
	cenButton_SetState( hwnd, PUSH_DOWN, (wState ? TRUE : FALSE ) );
	cenButton_FnEnd( hwnd, wStateOld);
}

static void
cenButton_SetState(
HWND	hwnd,
WORD	wState,
BOOL	bEnable )
{
	/* Turn on/off state bits according to the bEnable flag.  If the   */
	/* new state is different, invalidate the client window so that	   */
	/* the proper bitmap is displayed.								   */

	WORD	wNewState;
	WORD	wOldState;
	RECT	rectClient;

	wOldState = GET_PBSTATE( hwnd);
	wNewState = (bEnable ? (wOldState | wState) : (wOldState & ~wState));

	if (wOldState != wNewState)
	{	SET_PBSTATE( hwnd, wNewState );
		GetClientRect(hwnd, &rectClient);
		InvalidateRect(hwnd, &rectClient, FALSE);
	}
}

#if 0
static void
cenButton_OnSetFont(
HWND		hwnd,
HFONT		hFont,
BOOL		bRedraw)
{
	BOOL		bDeleteFont = FALSE;
	HFONT		hFontNew = 0;
	LOGFONT		logFont;

	/* create a thin font*/
	if( GetObject( hFont, sizeof( logFont), &logFont) != 0) {
		if( logFont.lfWeight != FW_NORMAL) {
			logFont.lfWeight = FW_NORMAL;
			if( ( hFontNew = CreateFontIndirect( &logFont)) != 0) {
				hFont = hFontNew;
				bDeleteFont = TRUE;
			}
		}
	}

	if( GET_PBDELETEFONT( hwnd))
		DeleteObject( GET_PBFONT( hwnd));

	SET_PBDELETEFONT( hwnd, bDeleteFont);
	SET_PBFONT( hwnd, hFont);

	FORWARD_WM_SETFONT( hwnd, hFont, bRedraw, DefWindowProc);
}
#endif

static void
cenButton_OnSetText(
HWND		hwnd,
LPCSTR		lpszText)
{
	/* WM_SETTEXT is sent to change the text of the button	   */
	/* control.	 In this case we allow the default window proc */
	/* to handle the message first.	 But this only affects the */
	/* internal Windows data structure of the control, it does */
	/* not display the change.	To do this we invalidate and   */
	/* update the client area of the control which displays	   */
	/* the new text.										   */

	FORWARD_WM_SETTEXT( hwnd, lpszText, DefWindowProc);
	InvalidateRect( hwnd, NULL, FALSE);
	UpdateWindow( hwnd);
}

LRESULT CALLBACK
cenButtonWndFn(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	/* This is the window proc for the pushbutton control.	Most of	   */
	/* the drawing is accomplished in the DrawPushButton() function. */
	/* The code below is mainly concerned with the keyboard and mouse  */
	/* events that the control detects.								   */
	switch( message) {
	HANDLE_MSG( hwnd, WM_CREATE, cenButton_OnCreate);
	/*HANDLE_MSG( hwnd, WM_ENABLE, cenButton_OnEnable);*/
	HANDLE_MSG( hwnd, WM_SETFOCUS, cenButton_OnSetFocus);
	HANDLE_MSG( hwnd, WM_KILLFOCUS, cenButton_OnKillFocus);
	HANDLE_MSG( hwnd, WM_LBUTTONDOWN, cenButton_OnLButtonDown);
	HANDLE_MSG( hwnd, WM_LBUTTONDBLCLK, cenButton_OnLButtonDown);
	HANDLE_MSG( hwnd, WM_LBUTTONUP, cenButton_OnLButtonUp);
	HANDLE_MSG( hwnd, WM_MOUSEMOVE, cenButton_OnMouseMove);
	/*HANDLE_MSG( hwnd, WM_KEYDOWN, cenButton_OnKey);*/
	/*HANDLE_MSG( hwnd, WM_KEYUP, cenButton_OnKey);*/
	HANDLE_MSG( hwnd, WM_ERASEBKGND, cenButton_OnEraseBkgnd);
	HANDLE_MSG( hwnd, WM_PAINT_SPECIAL, cenButton_OnPaint);
	/*HANDLE_MSG( hwnd, WM_GETDLGCODE, cenButton_OnGetDlgCode);*/
	HANDLE_MSG( hwnd, BM_SETSTYLE, cenButton_OnSetStyle);
	HANDLE_MSG( hwnd, BM_GETSTATE, cenButton_OnGetState);
	HANDLE_MSG( hwnd, BM_SETSTATE, cenButton_OnSetState);
	/*HANDLE_MSG( hwnd, WM_DESTROY, cenButton_OnDestroy);*/
	/*HANDLE_MSG( hwnd, WM_SETFONT, cenButton_OnSetFont);*/
	HANDLE_MSG( hwnd, WM_SETTEXT, cenButton_OnSetText);

	case WM_GETDLGCODE:
		return cenButton_OnGetDlgCode ( hwnd, (LPMSG)lParam );

	case WM_KEYDOWN:
	case WM_CHAR:
	    if( (char)wParam == ' ' || wParam == VK_RETURN || wParam == VK_MBUTTON)
			{
			SendMessage ( hwnd, WM_LBUTTONDOWN, -1, -1 );
			UpdateWindow ( hwnd );
			SendMessage ( hwnd, WM_LBUTTONUP, -1, -1 );
			UpdateWindow ( hwnd );
			}
	    else if (message == WM_CHAR)
			return SendMessage ( GetParent(hwnd), message, wParam, lParam );
		break;

	case BM_GETCHECK:
#if 0
 		return cenButton_OnGetState(hwnd);
#else 
		return ((GET_PBSTATE(hwnd) & PUSH_CHECKED) == PUSH_CHECKED) ? BST_CHECKED : BST_UNCHECKED;
#endif

	case BM_SETCHECK:
#if 0
		cenButton_OnSetState(hwnd, (WORD)wParam);
#else
{
		WORD		wStateOld;

		wStateOld = cenButton_FnStart( hwnd);
		cenButton_SetState( hwnd, PUSH_CHECKED,
			((WORD)wParam ? TRUE : FALSE) );
		cenButton_FnEnd( hwnd, wStateOld);
}
#endif
		return 1;
	case BM_SETIMAGE:
            	hwnd->userdata = (DWORD)lParam;
            	InvalidateRect(hwnd, NULL, FALSE);
		return 0;

	// if cursor exit from client area, remove pushed state
	case WM_NCMOUSEMOVE:
	    if( GetCapture() == hwnd )
	    	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if( !PtInsideWindowNC(hwnd, x, y) )
		    {
		    if( GET_PBWASINSIDE( hwnd))
		        {
			cenButton_SetState( hwnd, PUSH_DOWN, FALSE);
			SET_PBWASINSIDE( hwnd, FALSE );
			}
		    }
		else
		    {
		    if( !GET_PBWASINSIDE( hwnd) )
		        {
			cenButton_SetState( hwnd, PUSH_DOWN, TRUE );
			SET_PBWASINSIDE( hwnd, TRUE );
			}
		    }
		}
	    break;

	// if LBUTT go up when outside client area, reset state
	case WM_NCLBUTTONUP:
	    if( GetCapture() == hwnd )
	    	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if( !PtInsideWindowNC(hwnd, x, y) )
		    {
		    SET_PBCAPTURE( hwnd, FALSE );
	            SET_PBWASINSIDE( hwnd, FALSE );
		    ReleaseCapture();
		    }
		}
	    break;

	case WM_SETFONT:
	    if( wParam == 0 ) break;
	    SET_WND_FONT ( hwnd, (HFONT)wParam );
	    if( LOWORD(lParam) != 0 )
	    	InvalidateRect ( hwnd, NULL, TRUE );
	    return 0;

    case WM_GETFONT:
		return (LRESULT) GET_WND_FONT ( hwnd );

	case WM_ENABLE:
		InvalidateRect ( hwnd, NULL, TRUE );
		if( (wParam == FALSE) && (GetFocus() == hwnd) )
			PostMessage ( GetParent(hwnd), WM_NEXTDLGCTL, 0, 0 );
		break;

	case WM_NCHITTEST:
		{
		DWORD dwStyle = GetWindowLong ( hwnd, GWL_STYLE );
		if( (dwStyle & 0x0F) == BS_GROUPBOX )
			return HTTRANSPARENT;
		break;
		}
	}

	return DefWindowProc( hwnd, message, wParam, lParam);
}

int
MwRegisterButtonControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)cenButtonWndFn;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= PB_EXTRABYTES;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "BUTTON";

	return RegisterClass(&wc);
}

static void
DrawGroupBox(HWND hwnd,HDC hDCwParam,DWORD dwStyle)
{
	HDC		hdc;
	HFONT 		hFont;
	RECT		rcClient;
	RECT		rcText;
	RECT		rc;
	PAINTSTRUCT	ps;
	char		buf[256];
	HPEN		hPenTop, hPenBottom, holdPen;
	COLORREF 	crTop,crBottom;


	hdc = BeginPaint(hwnd, &ps);
	if(!hdc)
		goto Return;

	GetWindowText(hwnd, buf, sizeof(buf));
	GetClientRect( hwnd, &rcClient );

	hFont = GET_WND_FONT ( hwnd );
	if (hFont)
		hFont = SelectObject(hdc,hFont);

	rc.left = 0;
	rc.top = 0;
	DrawText( hdc, buf, -1, &rc, DT_CALCRECT);
	FastFillRect(hdc, &rcClient, GetSysColor(COLOR_BTNFACE));

	if(buf[ 0]) {
		SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
		SetBkMode(hdc,TRANSPARENT);
		SetRect(&rcText,8,2,rc.right+8,rc.bottom+2);
		DrawText(hdc,buf,-1,&rcText,DT_CENTER|DT_VCENTER);
	}

	crTop=GetSysColor(COLOR_BTNHIGHLIGHT);
	crBottom=GetSysColor(COLOR_BTNSHADOW);

	hPenTop = CreatePen( PS_SOLID, 1, crTop);
	hPenBottom = CreatePen( PS_SOLID, 1, crBottom);
	holdPen = SelectObject( hdc, hPenTop);

	MoveToEx(hdc,0,rc.bottom/2,NULL);

	if(buf[ 0]) {
		LineTo(hdc,5,rc.bottom/2);
		MoveToEx(hdc,rc.right+11,rc.bottom/2,NULL);
		LineTo(hdc,rcClient.right-1,rc.bottom/2);
	}
	else
		LineTo(hdc,rcClient.right-1,rc.bottom/2);

	LineTo(hdc,rcClient.right-1,rcClient.bottom-1);

	SelectObject( hdc, hPenBottom);
	LineTo(hdc,rcClient.left,rcClient.bottom-1);
	LineTo(hdc,rcClient.left,rc.bottom/2);

	SelectObject( hdc, holdPen);
	DeleteObject( hPenTop);
	DeleteObject( hPenBottom);

	if (hFont)
		SelectObject(hdc,hFont);

Return:
	EndPaint(hwnd, &ps);
}
