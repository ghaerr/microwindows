
#ifdef MW_DIALOGS_CORE
	#ifdef DEBUG_DLG_INWINDOWS
		#include "..\makedlg\resource.h"
	#else
		#include "resource.h"
	#endif
#endif

#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE(r)	((LPCSTR)r)
#endif

#ifndef DS_MODALFRAME
#define DS_MODALFRAME	0
#endif
#ifndef DS_ABSALIGN
#define DS_ABSALIGN	0
#endif
#ifndef IsChild
#define IsChild(Parent, Children)	((Children)->parent == (Parent))
#endif

#ifndef IDOK
#define IDOK		2
#endif
#ifndef IDCANCEL
#define IDCANCEL	1
#endif


//#ifndef DLGPROC
//typedef BOOL (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
//#endif




#define IDC_STATIC -1
#define NOT		0-

typedef struct tagDlgParams
{
DLGPROC lpFnDlg;
BOOL running;
LRESULT nResult;
} DlgParams;

typedef struct tagDlgDesc
{
	UINT nDlgTemplate;
	HWND (*fnCreate)(HINSTANCE hInst, HWND hParent, HWND *hFocus);
} DlgDesc;

#ifndef WM_INITDIALOG
#define WM_INITDIALOG                   0x0110
#endif

 
#ifndef WM_NCDESTROY
#define WM_NCDESTROY	WM_DESTROY
#endif


#ifdef DEBUG_DLG_INWINDOWS
	#undef DefDlgProc
	#undef CreateDialogParam
	#undef CreateDialog
	#undef DialogBoxParam
	#undef DialogBox
	#undef EndDialog
	#define DefDlgProc				test_DefDlgProc
	#define CreateDialogParam		test_CreateDialogParam
	#define CreateDialog			test_CreateDialog
	#define DialogBoxParam			test_DialogBoxParam
	#define DialogBox				test_DialogBox
	#define EndDialog				test_EndDialog
#else
	#define DefDlgProc				DefDlgProc
	#define CreateDialogParam		CreateDialogParam
	#define CreateDialog			CreateDialog
	#define DialogBoxParam			DialogBoxParam
	#define DialogBox				DialogBox
	#define EndDialog				EndDialog
#endif

extern DlgDesc dialogTemplates[];



LRESULT CALLBACK DefDlgProc ( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam );

HWND WINAPI CreateDialogParam ( HINSTANCE hInstance, LPCSTR lpTemplate, 
								HWND hWndParent, DLGPROC lpDialogFunc, 
								LPARAM dwInitParam );

HWND WINAPI CreateDialog ( HINSTANCE hInstance, LPCSTR lpTemplate, 
						   HWND hWndParent, DLGPROC lpDialogFunc );

int WINAPI DialogBoxParam ( HINSTANCE hInstance, LPCTSTR lpTemplate, 
						    HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParam );

int WINAPI DialogBox ( HINSTANCE hInstance, LPCTSTR lpTemplate, 
					   HWND hWndParent, DLGPROC lpDialogFunc );

BOOL WINAPI EndDialog ( HWND hDlg, int nResult );

BOOL WINAPI InitializeDialogs ( HINSTANCE hInstance );



#define GetDlgItemText(dlg, id, dst, max) \
	GetWindowText(GetDlgItem(dlg, id), dst, max)
#define SetDlgItemText(dlg, id, txt) \
	SetWindowText(GetDlgItem(dlg, id), txt)

#define SendDlgItemMessage(dlg, id, msg, wp, lp) \
	SendMessage ( GetDlgItem(dlg, id), msg, wp, lp )

/*
 *	If this is the dialog module, define the functions.
 */
#ifdef MW_DIALOGS_CORE

/*
 *  Search the first button in the childrens list.
 */
static HWND firstDefButton ( HWND hdlg )
{
    HWND hwnd = hdlg->children;
    while ( hwnd )
	{
	if( hwnd->pClass && !strcasecmp(hwnd->pClass->szClassName, "BUTTON") &&
	    (hwnd->style & BS_DEFPUSHBUTTON) )
	    return hwnd;
	    
	hwnd = hwnd->siblings;
	}
	
    return NULL;
}

/*
 *  Find the next children in the dialogbox.
 */
static HWND nextTabStop ( HWND hDlg, HWND hChild )
{
    HWND hwnd = hDlg->children;
    HWND start = NULL;
    
    while ( hwnd && !start )
	{
	if( hwnd == hChild )
	    {
	    if( hwnd->siblings )
		start = hwnd->siblings;
	    else
	      if( hwnd != hDlg->children )
	        start = hDlg->children;
	    }
	else    
	    hwnd = hwnd->siblings;
	}
	
    while ( start && start != hwnd )
	{
	if( (start->style & WS_TABSTOP) )
	    return start;
	    
	start = start->siblings;
	if( !start )
	    start = hDlg->children;
	}
	
    return NULL;
}


LRESULT CALLBACK DefDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	HWND hChild;
	
	switch ( Msg )
		{
		case WM_CTLCOLORSTATIC:
		    return NULL;
		    
		case WM_INITDIALOG:
			return TRUE;
			
		case WM_CHAR:
		    switch ( wParam )
			{
			case 13:
			    hChild = firstDefButton ( hDlg );
			    if( hChild )
				{
				PostMessage ( hDlg, WM_COMMAND, 
				    MAKELONG(GetDlgCtrlID(hChild), BN_CLICKED), (LPARAM)hChild );
				}
			    else
				SendMessage ( hDlg, WM_COMMAND, IDOK, 0 );
			    break;
			    
			case 0x1b: // Escape
			    PostMessage ( hDlg, WM_COMMAND, IDCANCEL, 0 );
			    break;
			    
			case 9: // tab
			    hChild = nextTabStop ( hDlg, GetFocus() );
			    if( hChild )
				{
				SetFocus ( hChild );
				}
			    break;
			}
		    break;
		}

	return DefWindowProc ( hDlg, Msg, wParam, lParam );
}



static LRESULT CALLBACK defDlgProc ( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	DlgParams *dlgParams;
	LRESULT retV = 0;

	dlgParams = (DlgParams *) GetWindowLong ( hWnd, 0 );
	if( dlgParams && dlgParams->lpFnDlg )
	    retV = dlgParams->lpFnDlg(hWnd, Msg, wParam, lParam);
	if( !retV )
	    retV = DefDlgProc ( hWnd, Msg, wParam, lParam );

	if( Msg == WM_NCDESTROY && dlgParams )
		{
		free ( dlgParams );
		SetWindowLong ( hWnd, 0, 0 );
		}

	return retV;
}


static HWND hCtrlFocus;

HWND WINAPI CreateDialogParam ( HINSTANCE hInstance, LPCSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam )
{
	DlgDesc *obj = dialogTemplates;
	DlgParams *params;
	
	HWND hFocus = NULL;
	hCtrlFocus = NULL;
	
	while ( obj->fnCreate )
		{
		if( obj->nDlgTemplate == (UINT) lpTemplate )
			{
			HWND hDlg = obj->fnCreate ( hInstance, hWndParent, &hFocus );
			if( hDlg )
				{
				params = (DlgParams*) malloc ( sizeof(DlgParams) );
				params->lpFnDlg = (DLGPROC) lpDialogFunc;
				params->running = FALSE;
				params->nResult = 0;
				SetWindowLong ( hDlg, 0, (LONG) params );
				if( SendMessage(hDlg, WM_INITDIALOG, (WPARAM)hFocus, dwInitParam) &&
					hFocus )
					{
					hCtrlFocus = hFocus;
					}
				}
			return hDlg;
			}
	
		obj++;
		}

	return NULL;
}


HWND WINAPI CreateDialog ( HINSTANCE hInstance, LPCSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc )
{
	return CreateDialogParam ( hInstance, lpTemplate, hWndParent, lpDialogFunc, 0 );
}


int WINAPI DialogBoxParam ( HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParam )
{
	int retV = 0;
	MSG msg;
	DlgParams *params;
	HWND hDlg = CreateDialogParam ( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParam );
	if( hDlg )
		{
		if( hWndParent ) EnableWindow ( hWndParent, FALSE );
		params = (DlgParams *) GetWindowLong ( hDlg, 0 );
		params->running = TRUE;
		ShowWindow ( hDlg, SW_SHOW );
		UpdateWindow ( hDlg );
		if( hCtrlFocus && IsChild(hDlg, hCtrlFocus) )
		    {
		    SetFocus ( hCtrlFocus );
		    }
		    
		while ( IsWindow(hDlg) && params->running )
			{
			if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
				{
				TranslateMessage ( &msg );
				DispatchMessage ( &msg );
				if( msg.message == WM_QUIT ) break;
				}
			MwHandleTimers();
			idle_handler ();
			}
		if( !params->running )
			{
			retV = params->nResult;
			DestroyWindow ( hDlg );
			}

		if( hWndParent ) EnableWindow ( hWndParent, TRUE );
		}

	return retV;
}


int WINAPI DialogBox ( HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc )
{
	return DialogBoxParam ( hInstance, lpTemplate, hWndParent, lpDialogFunc, 0 );
}


BOOL WINAPI EndDialog ( HWND hDlg, int nResult )
{
	DlgParams *params = (DlgParams*) GetWindowLong ( hDlg, 0 );
	if( params )
		{
		params->running = FALSE;
		params->nResult = nResult;
		return TRUE;
		}

	return FALSE;
}







BOOL WINAPI InitializeDialogs ( HINSTANCE hInstance )
{
#ifdef  WNDCLASSEX
    WNDCLASSEX wcl;
#else
    WNDCLASS wcl;
#endif

    MwRegisterStaticControl(NULL);
    MwRegisterButtonControl(NULL);
    MwRegisterEditControl(NULL);
    MwRegisterListboxControl(NULL);
    MwRegisterProgressBarControl(NULL);
    MwRegisterComboboxControl(NULL);


    memset ( &wcl, 0, sizeof(wcl) );
#ifdef  WNDCLASSEX
    wcl.cbSize = sizeof(wcl);
#endif
    wcl.style = CS_BYTEALIGNCLIENT;
    wcl.cbWndExtra = 4;
    wcl.lpfnWndProc = (WNDPROC) defDlgProc;
    wcl.hInstance = hInstance;
    wcl.lpszClassName = "GDLGCLASS";
    wcl.hbrBackground = GetStockObject ( LTGRAY_BRUSH );

#ifdef  WNDCLASSEX
    return (RegisterClassEx(&wcl) != NULL);
#else
    return (RegisterClass(&wcl) != NULL);
#endif
}

#endif
