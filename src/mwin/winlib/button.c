#include "windows.h"
#include "windowsx.h"
#include "wintools.h"
/*
 * WINCTL Custom Control Library
 * Push button Custom Control
 *	This control implements a custom pushbutton control.
 *
 * 4/8/98 g haerr original version from control palette v2.00, Blaise Computing
 */

#define GET_PBSTATE(h)			(GetWindowWord(h, 0))
#define GET_PBCAPTURE(h)		(GetWindowWord(h, 2))
#define GET_PBWASINSIDE(h)		(GetWindowWord(h, 4))
#define GET_PBDELETEFONT(h)		(GetWindowWord(h, 6))
#define GET_PBFONT(h)			(GetWindowWord(h, 8))

#define SET_PBSTATE(h,x)		(SetWindowWord(h, 0, x))
#define SET_PBCAPTURE(h,x)		(SetWindowWord(h, 2, x))
#define SET_PBWASINSIDE(h,x)		(SetWindowWord(h, 4, x))
#define SET_PBDELETEFONT(h,x)		(SetWindowWord(h, 6, x))
#define SET_PBFONT(h,x)			(SetWindowWord(h, 8, x))

#define PARENT(hwnd)		((HWND)GetWindowLong(hwnd,GWL_HWNDPARENT))

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

/* entry points*/
void WINAPI		CheckRadioButton(HWND hDlg, int nIDFirst,int nIDLast,
				int nIDCheckButton);

/* local procs*/
static void WINAPI	cenButton_FnEnd( HWND, WORD);
static WORD WINAPI	cenButton_FnStart( HWND);
static BOOL WINAPI	cenButton_OnCreate( HWND, LPCREATESTRUCT);
/*static void WINAPI	cenButton_OnDestroy( HWND);*/
/*static void WINAPI	cenButton_OnEnable( HWND, BOOL);*/
static BOOL WINAPI	cenButton_OnEraseBkgnd( HWND, HDC);
/*static UINT WINAPI	cenButton_OnGetDlgCode( HWND, LPMSG);*/
static LONG WINAPI	cenButton_OnGetState( HWND);
/*static void WINAPI	cenButton_OnKey( HWND, UINT, BOOL, int, UINT);*/
static void WINAPI	cenButton_OnKillFocus( HWND, HWND);
static void WINAPI	cenButton_OnLButtonDown( HWND, BOOL, UINT, UINT, UINT);
static void WINAPI	cenButton_OnLButtonUp( HWND, UINT, UINT, UINT);
static void WINAPI	cenButton_OnMouseMove( HWND, UINT, UINT, UINT);
static void WINAPI	cenButton_OnPaint( HWND, HDC);
static void WINAPI 	DrawPushButton(HWND hwnd,HDC hDCwParam,UINT wEnumState,
				DWORD dwStyle);
static void WINAPI 	DrawGroupBox(HWND hwnd,HDC hDCwParam, DWORD dwStyle);
static void WINAPI	cenButton_OnSetFocus( HWND, HWND);
static void WINAPI	cenButton_OnSetStyle( HWND, WORD, BOOL);
static void WINAPI	cenButton_OnSetState( HWND, WORD);
static void WINAPI	cenButton_SetState( HWND, WORD, BOOL);
static void WINAPI	cenButton_OnSetText( HWND, LPCSTR);

static void WINAPI
cenButton_FnEnd(
HWND		hwnd,
WORD		wState)
{
	if( wState != GET_PBSTATE( hwnd)) {
		if( IsWindowVisible( hwnd))
			UpdateWindow( hwnd);
	}
}

static WORD WINAPI
cenButton_FnStart(
HWND		hwnd)
{
	return GET_PBSTATE( hwnd);
}

static BOOL WINAPI
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

	if ((lpCreate->style & 0x0f) == BS_DEFPUSHBUTTON)
		cenButton_SetState( hwnd, PUSH_DEFAULT, TRUE );

	if (lpCreate->style & WS_DISABLED)
		cenButton_SetState( hwnd, PUSH_DISABLED, TRUE );

	return( TRUE);
}

#if 0
static void WINAPI
cenButton_OnDestroy(
HWND		hwnd)
{
	if( GET_PBDELETEFONT( hwnd)) {
		DeleteObject( GET_PBFONT( hwnd));
		SET_PBDELETEFONT( hwnd, FALSE);
	}
}

static void WINAPI
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

static BOOL WINAPI
cenButton_OnEraseBkgnd(
HWND		hwnd,
HDC			hdc)
{
	/* Background is erased at WM_PAINT time, so return TRUE*/
	return TRUE;
}

#if 0
static UINT WINAPI
cenButton_OnGetDlgCode(
HWND	hwnd,
LPMSG 	lpMsg)
{
	/* WM_GETDLGCODE is sent by the dialog manager to find	   */
	/* what type/style of control is responding and/or to	   */
	/* determine what keystrokes the control wants to process  */
	/* itself.	In this case, the pushbutton identifies itself */
	/* and also indicates whether it is currently the default  */
	/* pushbutton.											   */

	/*return( DLGC_BUTTON | ((GET_PBSTATE( hwnd) & PUSH_DEFAULT) ?
				DLGC_DEFPUSHBUTTON : DLGC_UNDEFPUSHBUTTON));*/
	return( DLGC_BUTTON);
}
#endif

static LONG WINAPI
cenButton_OnGetState(
HWND		hwnd)
{
	/* BM_GETSTATE is sent to enquire about the state of the   */
	/* control.	 It returns TRUE if the button is in the down  */
	/* state.												   */

	return( ( GET_PBSTATE( hwnd) & PUSH_DOWN) == PUSH_DOWN);
}

#if 0
static void WINAPI
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

static void WINAPI
cenButton_OnKillFocus(
HWND		hwnd,
HWND		hwndNewFocus)
{
	WORD		wState;

	wState = cenButton_FnStart( hwnd);
	cenButton_SetState( hwnd, PUSH_FOCUS, FALSE );
	cenButton_FnEnd( hwnd, wState);
}

static void WINAPI
cenButton_OnLButtonDown(
HWND		hwnd,
BOOL		bDblClick,
UINT		x,
UINT		y,
UINT		keyState)
{
	WORD		wState;

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

static void WINAPI
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
			CheckRadioButton(PARENT(hwnd),0,0xffff,hwnd->id);
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

static void WINAPI
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
	DWORD		dwStyle;

	wState = cenButton_FnStart( hwnd);
	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	if( GET_PBCAPTURE( hwnd)) {
		if( !PtInsideWindow( hwnd, x, y) ) {
			if( GET_PBWASINSIDE( hwnd)) {
				cenButton_SetState( hwnd, PUSH_DOWN, FALSE);
				SET_PBWASINSIDE( hwnd, FALSE );
			}
		} else {
			if( !GET_PBWASINSIDE( hwnd) ) {
				cenButton_SetState( hwnd, PUSH_DOWN, TRUE );
				SET_PBWASINSIDE( hwnd, TRUE );
			}
		}
	}
	cenButton_FnEnd( hwnd, wState);
}

static void WINAPI
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

static void WINAPI
DrawPushButton(HWND hwnd,HDC hDCwParam,UINT wEnumState,DWORD dwStyle)
{
	HDC		hdc;
	HBRUSH		hNewBrush;
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
#define uiWidthFrame	0
#define uiWidthShadow	2

	hdc = BeginPaint(hwnd, &ps);
	if(!hdc)
		goto Return;

	GetWindowText(hwnd, buf, sizeof(buf));
	GetClientRect( hwnd, &rectClient );
	uiWidth	= rectClient.right - rectClient.left;
	uiHeight = rectClient.bottom - rectClient.top;

	hNewBrush = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
	crOld = SetTextColor( hdc, GetSysColor( COLOR_BTNTEXT));
	crBkOld = SetBkColor( hdc, GetSysColor( COLOR_BTNFACE));

	rc = rectClient;
	switch((int)(dwStyle & 0x0f)) {
	case BS_PUSHBUTTON:
	case BS_DEFPUSHBUTTON:
		if( wEnumState & PBS_FOCUSDOWN) {
			Draw3dBox(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
				GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_WINDOWFRAME));
			InsetR(&rc, 1, 1);
			Draw3dBox(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
				GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNSHADOW));
			InsetR(&rc, 1, 1);
			FastFillRect(hdc, &rc, GetSysColor(COLOR_BTNFACE));
			iFaceOffset = 1;
		} else {
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
		rc.left = 0;
		rc.top += 1;
		rc.right = rc.left + 10;
		rc.bottom = rc.top + 10;

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
		if(wEnumState & PBS_CHECKED)
			Ellipse(hdc, rc.left+2, rc.top+2, rc.right-2,
					rc.bottom-2);
		break;
	}

	/*
	 * draw text
	 */
	if(buf[ 0]) {
		hNewFont = GetStockObject( DEFAULT_GUI_FONT);
		hOldFont = SelectObject( hdc, hNewFont);

		/* calculate text bounding rect*/
		rect.left = 0;
		rect.top = 0;
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
			rect.left = (rect.right - rect.left) + uiWidthFrame
				+ uiWidthShadow + 4 + iFaceOffset;
			break;
		}

		switch((int)(dwStyle & 0x0f)) {
		case BS_CHECKBOX:
		case BS_AUTOCHECKBOX:
		case BS_AUTORADIOBUTTON:
		case BS_RADIOBUTTON:
			rect.left = 12;
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
		if(wEnumState & PBS_DISABLED)
			hOldColor = SetTextColor( hdc,
				GetSysColor( COLOR_GRAYTEXT));
		else
			hOldColor = SetTextColor( hdc,
				GetSysColor( COLOR_BTNTEXT));

		DrawText( hdc, buf, -1, &rect,DT_LEFT | DT_SINGLELINE | DT_TOP);

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

static void WINAPI
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
}

static void WINAPI
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

static void WINAPI
cenButton_OnSetState(
HWND		hwnd,
WORD		wState)
{
	WORD		wStateOld;

	wStateOld = cenButton_FnStart( hwnd);
	cenButton_SetState( hwnd, PUSH_DOWN, (wState ? TRUE : FALSE ) );
	cenButton_FnEnd( hwnd, wStateOld);
}

static void WINAPI
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
static void WINAPI
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

static void WINAPI
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
cenButtonWndFn(
HWND	hwnd,
UINT	message,
WPARAM	wParam,
LPARAM	lParam)
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

	case BM_GETCHECK:
#if 0
 		return cenButton_OnGetState(hwnd);
#else 
		return( ( GET_PBSTATE(hwnd) & PUSH_CHECKED) == PUSH_CHECKED);
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
		return 0;
	}

	return DefWindowProc( hwnd, message, wParam, lParam);
}

/* backwards compatibility*/
int WINAPI
MwButtonRegister(HINSTANCE hInstance)
{
	return MwRegisterButtonControl(hInstance);
}

int WINAPI
MwRegisterButtonControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)cenButtonWndFn;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 10;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "BUTTON";

	return RegisterClass(&wc);
}

static void WINAPI
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

	hFont = GetStockObject( DEFAULT_GUI_FONT);
	if (hFont)
		hFont = SelectObject(hdc,hFont);

	rc.left = 0;
	rc.top = 0;
	DrawText( hdc, buf, -1, &rc, DT_CALCRECT);

	if(buf[ 0]) {
		SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
		SetBkMode(hdc,TRANSPARENT);
		SetRect(&rcText,8,2,rc.right+8,rc.bottom+2);
		DrawText(hdc,buf,-1,&rcText,DT_CENTER);
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

/* temporarily here, should move to winuser.c*/
void WINAPI
CheckRadioButton(HWND hDlg, int nIDFirst,int nIDLast,int nIDCheckButton)
{
	HWND	hWndCheck,hWndTemp;
	DWORD 	dwStyle;

	if (!(hWndCheck = GetDlgItem(hDlg,nIDCheckButton)))
	    return;

	for(hWndTemp=hDlg->children; hWndTemp; hWndTemp=hWndTemp->siblings) {
		if(hWndCheck == hWndTemp) continue;
		dwStyle = GetWindowLong(hWndTemp,GWL_STYLE);
		if ((hWndTemp->id >= (WORD)nIDFirst) && 
		    (hWndTemp->id <= (WORD)nIDLast) &&
			((LOWORD(dwStyle) == BS_RADIOBUTTON) ||
			 (LOWORD(dwStyle) == BS_AUTORADIOBUTTON)))
			SendMessage(hWndTemp,BM_SETCHECK,FALSE,0);
	}
	SendMessage(hWndCheck,BM_SETCHECK,TRUE,0);
}
