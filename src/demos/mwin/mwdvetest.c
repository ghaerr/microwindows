/*
 *
 * TEST program for the new DVE implementations
 *
 * Copyright (C) 2004, Gabriele Brugnoni
 * <gabrielebrugnoni@dveprojects.com>
 * DVE Prog. El. - Varese, Italy
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "windows.h"
#include "../../images/demos/mwin/mwdvetest/dveres.h"


HINSTANCE hInst;

static HFONT hFntCyber12 = NULL;
static HFONT hFntCyber14 = NULL;

DLGBOOL CALLBACK defDemoDlg ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK dlgDemoStatic ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK dlgDemoMultiline ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK dlgDemoInternational ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK dlgDemoStrings ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK dlgDemoEdit ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK dlgDemoList ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK dlgDemoTimers ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
DLGBOOL CALLBACK mainDlgProc ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK kbdTranslate ( WPARAM *pVK, LPARAM *pControlMask, BOOL *pressed );

DLGBOOL CALLBACK defDemoDlg ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
		{
		case WM_INITDIALOG:
			{
#ifdef DEBUGBASEUNIT
			char s[256];
			RECT rc, rcb;
			TEXTMETRIC tm;
			HFONT hfnt = (HFONT) SendMessage ( hWnd, WM_GETFONT, 0, 0 );
			HDC hdc = GetDC ( hWnd );
			rcb.left = rcb.top = 0;
			rcb.right = 4, rcb.bottom = 8;
			SelectObject ( hdc, hfnt );
			GetTextMetrics ( hdc, &tm );
			GetClientRect(hWnd, &rc);
			MapDialogRect ( hWnd, &rcb );
			sprintf ( s, "AW=%d (%d), Ht=%d (%d), %d x %d", 
					  tm.tmAveCharWidth, rcb.right, tm.tmHeight, rcb.bottom,
					  rc.right, rc.bottom );
			SetWindowText ( hWnd, s );
			ReleaseDC ( hWnd, hdc );
#endif
			}
			break;

		case WM_CTLCOLORSTATIC:
			{
			if( GetDlgCtrlID((HWND)lParam) == IDC_CTLCOLOR )
				{
				HBRUSH hbr = (HBRUSH)(LRESULT)DefWindowProc ( hWnd, message, wParam, lParam );
				SetTextColor ( (HDC)wParam, RGB(255,0,0) );
				SetBkColor ( (HDC)wParam, GetSysColor(COLOR_3DFACE) );
				return (DLGBOOL)hbr;
				}
			}
			return DefWindowProc ( hWnd, message, wParam, lParam );

		case WM_COMMAND:
			switch ( LOWORD(wParam) )
				{
				case IDOK:
					EndDialog ( hWnd, TRUE );
					break;

				case IDCANCEL:
					EndDialog ( hWnd, FALSE );
					break;
				}
			break;

		default:
			return FALSE;
		}

	return TRUE;
}



DLGBOOL CALLBACK dlgDemoStatic ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	// Vedere: 	GdConvertEncoding 
	if( message == WM_INITDIALOG ) {
		if( hFntCyber12 == NULL )
			hFntCyber12 = CreateFont ( -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, "Cyberbit" );
		SendDlgItemMessage(hWnd, IDC_ARABICDEMO, WM_SETFONT, 
			(WPARAM)hFntCyber12, 0 );
		SetWindowText(GetDlgItem(hWnd, IDC_ARABICDEMO), 
			"Arabic \xd8\xaa\xd8\xab\xd8\xac\xd8\xad demo:\n"
			"\xd8\xaa\xd8\xab\xd8\xac\xd8\xad \xd8\xaa\xd8\xab\xd8\xac\xd8\xad\xd8\xae \xd8\xaa\xd8\xab\xd8\xac\xd8\xad\xd8\xaf\n"
			"\xd8\xaa\xd8\xab\xd8\xac\xd8\xad 12345\n"
			 );
		SendDlgItemMessage(hWnd, IDC_STATICFONT1, WM_SETFONT, 
			(WPARAM)(HGDIOBJ)GetStockObject(SYSTEM_FONT), 0 );
	}
	
	return defDemoDlg ( hWnd, message, wParam, lParam );
}


DLGBOOL CALLBACK dlgDemoInternational ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	/* References: 
		Cyberbit: http://www.math.nus.edu.sg/aslaksen/cs/cjk.html
				  ftp://ftp.netscape.com/pub/communicator/extras/fonts/windows/Cyberbit.ZIP
		Text: http://www.columbia.edu/~fdc/pace/
	 */
	if( message == WM_INITDIALOG ) {
		//char *arabic_utf8 = "اةتثجحخدذرزسشصضطظعغ اةتثجحخدذرزسشصضطظعغ اةتثجحخدذرزسشصضطظعغ اةتثجحخدذرزسشصضطظعغ";
		if( hFntCyber12 == NULL )
			hFntCyber12 = CreateFont ( -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, "Cyberbit" );
		SendDlgItemMessage(hWnd, IDC_STATICFONT1, WM_SETFONT, 
			(WPARAM)hFntCyber12, 0 );
		SetWindowText(GetDlgItem(hWnd, IDC_STATICFONT1), 
			"Arabic:\n"
			" أنا قادر على أكل الزجاج و هذا لا يؤلمني. \n"
			"Mixed Latin + Arabic:\n"
			"Arabic: أنا قادر على أكل الزجاج و هذا لا يؤلمن\n"
			"Mixed Arabic + Latin:\n"
			" أنا قادر على أكل الزجاج و هذا لا يؤلمني.  (arabic)\n"
			"\n'Peace' in some languages:\n"
			"Arabic: سلام (salām)\n"
			"Darja: عسلامة (esslama)\n"
			"Greek:  Ειρήνη (iríni)\n"
			"Kurdish: Hasîtî, Һашити (hasiti), ھاسیتی (hasītī)"
			 );
	}
	
	return defDemoDlg ( hWnd, message, wParam, lParam );
}


DLGBOOL CALLBACK dlgDemoMultiline ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	/* References: 
		Cyberbit: http://www.math.nus.edu.sg/aslaksen/cs/cjk.html
				  ftp://ftp.netscape.com/pub/communicator/extras/fonts/windows/Cyberbit.ZIP
		Text: http://www.columbia.edu/~fdc/pace/
	 */
	if( message == WM_INITDIALOG ) {
		//char *arabic_utf8 = "اةتثجحخدذرزسشصضطظعغ اةتثجحخدذرزسشصضطظعغ اةتثجحخدذرزسشصضطظعغ اةتثجحخدذرزسشصضطظعغ";
		if( hFntCyber14 == NULL )
			hFntCyber14 = CreateFont ( -14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, "Cyberbit" );
		SendDlgItemMessage(hWnd, IDC_EDIT1, WM_SETFONT, (WPARAM)hFntCyber14, 0 );
		SetWindowLong(GetDlgItem(hWnd, IDC_EDIT1), GWL_STYLE, GetWindowLong(GetDlgItem(hWnd, IDC_EDIT1), GWL_STYLE) & ~ES_AUTOHSCROLL);
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), 
			"Arabic:\n"
			" أنا قادر على أكل الزجاج و هذا لا يؤلمني. \n"
			"Mixed Latin + Arabic:\n"
			"Arabic: أنا قادر على أكل الزجاج و هذا لا يؤلمن\n"
			"Mixed Arabic + Latin:\n"
			" أنا قادر على أكل الزجاج و هذا لا يؤلمني.  (arabic)\n"
			"\n'Peace' in some languages:\n"
			"Arabic: سلام (salām)\n"
			"Darja: عسلامة (esslama)\n"
			"Greek:  Ειρήνη (iríni)\n"
			"Kurdish: Hasîtî, Һашити (hasiti), ھاسیتی (hasītī)"
			 );
	}
	
	return defDemoDlg ( hWnd, message, wParam, lParam );
}


DLGBOOL CALLBACK dlgDemoStrings ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	if( message == WM_INITDIALOG )
		{
		char s[256];
		int i;
		for ( i=0; i < 4; i++ )
			{
			if( !LoadString(hInst, IDS_STRING1+i, s, sizeof(s)) )
				strcpy ( s, "NOT FOUND!!" );
			
			SendDlgItemMessage(hWnd, IDC_STRING1+i, WM_SETTEXT, 0, (LPARAM)s );
			}
		}
	return defDemoDlg ( hWnd, message, wParam, lParam );
}


DLGBOOL CALLBACK dlgDemoEdit ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
		{
		case WM_INITDIALOG:
			if( hFntCyber14 == NULL )
				hFntCyber14 = CreateFont ( -14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, "Cyberbit" );
			SendDlgItemMessage(hWnd, IDC_EDIT1, WM_SETFONT, (WPARAM)hFntCyber14, 0 );
			SendDlgItemMessage(hWnd, IDC_EDIT4, WM_SETFONT, (WPARAM)hFntCyber14, 0 );
			break;
			
		case WM_COMMAND:
			switch ( LOWORD(wParam) )
				{
				case IDC_BUTT_COPY:
					{
					char s[256];
					GetDlgItemText ( hWnd, IDC_EDIT1, s, sizeof(s) );
					SetDlgItemText ( hWnd, IDC_EDIT4, s );
					return TRUE;
					}
				}
			break;
		}

	return defDemoDlg ( hWnd, message, wParam, lParam );
}



DLGBOOL CALLBACK dlgDemoList ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
		{
		case WM_INITDIALOG:
			{
			int tabs[] = {31-7, 81-7};
			SendDlgItemMessage ( hWnd, IDC_LIST_TABS, LB_SETTABSTOPS, 
				sizeof(tabs)/sizeof(tabs[0]), (LPARAM)tabs );
			SendDlgItemMessage ( hWnd, IDC_LIST_TABS, LB_SETHORIZONTALEXTENT, 500, 0 );
			SendDlgItemMessage ( hWnd, IDC_LIST_TABS, LB_ADDSTRING, 0, (LPARAM) "[1]\tThis is tabbed\tYes" );
			SendDlgItemMessage ( hWnd, IDC_LIST_TABS, LB_ADDSTRING, 0, (LPARAM) "This exits from tab space\tOK" );

			SendDlgItemMessage ( hWnd, IDC_LIST_OD, LB_ADDSTRING, 0, (LPARAM)"String 1" );
			SendDlgItemMessage ( hWnd, IDC_LIST_OD, LB_ADDSTRING, 0, (LPARAM)"String 2" );
			SendDlgItemMessage ( hWnd, IDC_LIST_OD, LB_ADDSTRING, 0, (LPARAM)"String 3" );

			SendDlgItemMessage ( hWnd, IDC_LIST_ODV, LB_ADDSTRING, 0, (LPARAM)"String 1" );
			SendDlgItemMessage ( hWnd, IDC_LIST_ODV, LB_ADDSTRING, 0, (LPARAM)"String 2" );
			SendDlgItemMessage ( hWnd, IDC_LIST_ODV, LB_ADDSTRING, 0, (LPARAM)"String 3" );
			SendDlgItemMessage ( hWnd, IDC_LIST_ODV, LB_ADDSTRING, 0, (LPARAM)"String 4" );
			SendDlgItemMessage ( hWnd, IDC_LIST_ODV, LB_ADDSTRING, 0, (LPARAM)"String 5" );
			SendDlgItemMessage ( hWnd, IDC_LIST_ODV, LB_ADDSTRING, 0, (LPARAM)"String 6" );

			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 1" );
			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 2" );
			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 3" );
			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 4" );
			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 5" );
			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 6" );
			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 7" );
			SendDlgItemMessage ( hWnd, IDC_LIST_MSEL, LB_ADDSTRING, 0, (LPARAM)"String 8" );

			break;
			}

		case WM_DRAWITEM:
			{
			LPDRAWITEMSTRUCT lpDrw = (LPDRAWITEMSTRUCT)lParam;
			if( lpDrw->CtlType != ODT_LISTBOX ) return 0;
			if( lpDrw->itemAction & ODA_DRAWENTIRE )
				{
				char txt[128];
				FillRect ( lpDrw->hDC, &lpDrw->rcItem, GetStockObject(WHITE_BRUSH) );
				SendMessage ( lpDrw->hwndItem, LB_GETTEXT, lpDrw->itemID, (LPARAM)txt );
				TextOut ( lpDrw->hDC, lpDrw->rcItem.left+(lpDrw->rcItem.bottom-lpDrw->rcItem.top)+8,
						  lpDrw->rcItem.top, txt, strlen(txt) );
				if( lpDrw->itemState & ODS_FOCUS )
					{
					lpDrw->rcItem.right--;
					lpDrw->rcItem.left++;
					DrawFocusRect ( lpDrw->hDC, &lpDrw->rcItem );
					}
				}
			if( (lpDrw->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)) != 0 )
				{
				SelectObject ( lpDrw->hDC, GetStockObject(BLACK_PEN) );
				if( !(lpDrw->itemState & ODS_SELECTED) )
					SelectObject ( lpDrw->hDC, GetStockObject(DKGRAY_BRUSH) );
				else
					SelectObject ( lpDrw->hDC, GetStockObject(WHITE_BRUSH) );
				Ellipse ( lpDrw->hDC, lpDrw->rcItem.left, lpDrw->rcItem.top, 
						  lpDrw->rcItem.left+(lpDrw->rcItem.bottom-lpDrw->rcItem.top),
						  lpDrw->rcItem.bottom );
				}
			if( (lpDrw->itemAction & ODA_FOCUS) )
				{
				lpDrw->rcItem.right--;
				lpDrw->rcItem.left++;
				DrawFocusRect ( lpDrw->hDC, &lpDrw->rcItem );
				}
			return TRUE;
			}

		case WM_MEASUREITEM:
			{
			LPMEASUREITEMSTRUCT lpMs = (LPMEASUREITEMSTRUCT)lParam;
			//printf ( "WM_MEASUREITEM\n" );
			if( lpMs->CtlID == IDC_LIST_ODV )
				{
				lpMs->itemHeight = 14+3*abs(3-(INT)lpMs->itemID);
				}
			else
				lpMs->itemHeight = 20;
			return TRUE;
			}
		}

	return defDemoDlg ( hWnd, message, wParam, lParam );
}



DLGBOOL CALLBACK dlgDemoTimers ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	if( message == WM_TIMER )
		SetDlgItemInt ( hWnd, wParam, 1+GetDlgItemInt(hWnd, wParam, NULL, TRUE), TRUE );
	
	if( message == WM_INITDIALOG )
		{
		SetTimer ( hWnd, IDC_TIMER1, 200, NULL );
		SetTimer ( hWnd, IDC_TIMER2, 1000, NULL );
		SetTimer ( hWnd, IDC_TIMER3, 2000, NULL );
		SetTimer ( hWnd, IDC_TIMER4, 5000, NULL );
		}

	return defDemoDlg ( hWnd, message, wParam, lParam );
}



DLGBOOL CALLBACK mainDlgProc ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static struct tagDlgFunctions { 
		int id; 
		DLGPROC fnProc; 
	} dlgDemos[] =
		{
			{IDD_DLGCONTROLS,	(DLGPROC) defDemoDlg},
			{IDD_DLGSTATIC,		(DLGPROC) dlgDemoStatic},
			{IDD_DLGEDIT,		(DLGPROC) dlgDemoEdit},
			{IDD_DLGLIST,		(DLGPROC) dlgDemoList},
			{IDD_DLGINTERNAT,	(DLGPROC) dlgDemoInternational},
			{IDD_DLGMULTILINE,	(DLGPROC) dlgDemoMultiline},
			{IDD_DLGTIMERS,		(DLGPROC) dlgDemoTimers},			
			{IDD_DLGSTRINGS,	(DLGPROC) dlgDemoStrings},
			{IDD_DLGABOUT,		(DLGPROC) defDemoDlg},
		};
	int i;


    switch ( message )
		{
		case WM_COMMAND:
			switch ( LOWORD(wParam) )
				{
				case IDOK:
					i = SendDlgItemMessage ( hWnd, IDC_LIST1, LB_GETCURSEL, 0, 0 );
					if( i < 0 || i >= (sizeof(dlgDemos)/sizeof(dlgDemos[0])) )
						return 0;
					DialogBox ( hInst, MAKEINTRESOURCE(dlgDemos[i].id), hWnd, dlgDemos[i].fnProc );
					break;

				case IDCANCEL:
					EndDialog ( hWnd, FALSE );
					break;

				case IDC_LIST1:
					if( HIWORD(wParam) == LBN_SELCHANGE )
						EnableWindow ( GetDlgItem(hWnd, IDOK), TRUE );
					break;
				}
			break;

		case WM_INITDIALOG:
			{
			int tabs[] = { 10, 50, 70 };
#ifdef DEBUGBASEUNIT
			char s[256];
			RECT rc, rcb;
			TEXTMETRIC tm;
			HFONT hfnt = (HFONT) SendMessage ( hWnd, WM_GETFONT, 0, 0 );
			HDC hdc = GetDC ( hWnd );
			rcb.left = rcb.top = 0;
			rcb.right = 4, rcb.bottom = 8;
			SelectObject ( hdc, hfnt );
			GetTextMetrics ( hdc, &tm );
			GetClientRect(hWnd, &rc);
			MapDialogRect ( hWnd, &rcb );
			sprintf ( s, "AW=%d (%d), Ht=%d (%d), %d x %d", 
					  tm.tmAveCharWidth, rcb.right, tm.tmHeight, rcb.bottom,
					  rc.right, rc.bottom );
			SetWindowText ( hWnd, s );
			ReleaseDC ( hWnd, hdc );
#endif
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_SETTABSTOPS, 3, (LPARAM)tabs );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "1)\tControl Navigation" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "2)\tStatic" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "3)\tEdit" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "4)\tListbox" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "5)\tInternational\tسلام " );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "6)\tMultiline Edit" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "7)\tTimers" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "8)\tLoadString" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "9)\tCredits" );
			SendDlgItemMessage ( hWnd, IDC_LIST1, LB_SETSEL, 0, 0);
			//EnableWindow ( GetDlgItem(hWnd, IDOK), FALSE );
			}
			break;
		}

	return FALSE;
}



BOOL CALLBACK kbdTranslate ( WPARAM *pVK, LPARAM *pControlMask, BOOL *pressed )
{
	static const unsigned short arabChar[] = {
	0x0621, 0x0622, 0x0623, 0x0624,
	0x0625, 0x0626, 0x0627, 0x0628,
	0x0629, 0x062A, 0x062B, 0x062C,
	0x062D, 0x062E, 0x062F, 0x0630,
	0x0631, 0x0632, 0x0633, 0x0634,
	0x0635, 0x0636, 0x0637, 0x0638,
	0x0639, 0x063A
	};
	static int lang = 0;
	int forceChar = 0;

	if( (*pControlMask & (1 << 24)) && (*pVK == VK_F10) && (*pressed) )
		{
		//printf ( "KEYBOARD: old lang=%d, new lang=", lang );
		lang = !lang;
		//printf ( "%d\n", lang );
		return FALSE;
		}	
	
	if( !(*pControlMask & (1 << 24)) && lang )
		{
		int chr = toupper(*pVK);
		switch ( chr )
			{
			case '?': forceChar = 0x061f; break;
			case ';': forceChar = 0x061b; break;
			case ',': forceChar = 0x060c; break;
			case '~': forceChar = 0x0653; break;
			case '"': forceChar = 0x0654; break;
			case '_': forceChar = 0x0640; break;
			default:
				if( chr >= 'A' && chr <= 'Z' )
					forceChar = arabChar[chr-'A'];
				break;
			}
		}
		
	if( forceChar ) 
		{
		*pControlMask &= ~(1 << 24);
		*pVK = forceChar;
		return TRUE;
		}
	return FALSE;
}



int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nShow )
{
#if MICROWINDOWS
	MwInitializeDialogs ( hInstance );
	MwSetTextCoding ( MWTF_UTF8 );
	MwSetKeyboardTranslator ( kbdTranslate );
#endif
	UpdateWindow(GetDesktopWindow());
	hInst = hInstance;
	DialogBox ( hInstance, MAKEINTRESOURCE(IDD_DLGMAIN), NULL, (DLGPROC)mainDlgProc );
	return 0;
}
