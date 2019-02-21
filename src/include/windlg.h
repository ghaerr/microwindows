/*
 *  windlg.h
 *
 * Microwindows Dialog implementation
 *
 * Copyrigth (C) 2003 - Gabriele Brugnoni
 *
 * gabrielebrugnoni@dveprojects.com
 * DVE Prog. El. - Varese, Italy
 */
#ifndef __WINDLG_H__
#define __WINDLG_H__

/*
 * For GetWindowLongPtr - SetWindowLongPtr
 */
#define DWL_MSGRESULT   0
#define DWL_DLGPROC     (sizeof(LONG_PTR))
#define DWL_DLGDATA	(2*sizeof(LONG_PTR))
#define DWL_USER        (3*sizeof(LONG_PTR))
#define DWL_EXTRABYTES	(4*sizeof(LONG_PTR))



/*
 * Dialog Styles
 */
#define DS_ABSALIGN         0x0001L
#define DS_SYSMODAL         0x0002L
#define DS_LOCALEDIT        0x0020L
#define DS_SETFONT          0x0040L
#define DS_MODALFRAME       0x0080L
#define DS_NOIDLEMSG        0x0100L
#define DS_SETFOREGROUND    0x0200L

#define DS_3DLOOK           0x0004L
#define DS_FIXEDSYS         0x0008L
#define DS_NOFAILCREATE     0x0010L
#define DS_CONTROL          0x0400L
#define DS_CENTER           0x0800L
#define DS_CENTERMOUSE      0x1000L
#define DS_CONTEXTHELP      0x2000L


#ifndef IsChild
#define IsChild(Parent, Children)	((Children)->parent == (Parent))
#endif


#ifndef IDOK
#define IDOK		1
#endif
#ifndef IDCANCEL
#define IDCANCEL	2
#endif
#ifndef IDC_STATIC
#define IDC_STATIC	-1
#endif

/*
 *  Struct for WM_DRAWITEM
 */
typedef struct tagDRAWITEMSTRUCT
{
	UINT CtlType;
	UINT CtlID;
	UINT itemID;
	UINT itemAction;
	UINT itemState;
	HWND hwndItem;
	HDC hDC;
	RECT rcItem;
	ULONG_PTR itemData;
} DRAWITEMSTRUCT, *LPDRAWITEMSTRUCT;

enum DRWITYPE {
	ODT_BUTTON,
	ODT_COMBOBOX,
	ODT_LISTBOX,
	ODT_LITVIEW,
	ODT_MENU,
	ODT_STATIC,
	ODT_TAB
};

#define ODA_DRAWENTIRE		0x0001
#define ODA_FOCUS			0x0002
#define ODA_SELECT			0x0004

#define ODS_CHECKED			0x0001
#define ODS_COMBOBOXEDIT	0x0002
#define ODS_DEFAULT			0x0004
#define ODS_DISABLED		0x0008
#define ODS_FOCUS			0x0010
#define ODS_GRAYED			0x0020
#define ODS_SELECTED		0x0040


typedef struct tagMEASUREITEMSTRUCT
{
	UINT CtlType;
	UINT CtlID;
	UINT itemID;
	UINT itemWidth;
	UINT itemHeight;
	ULONG_PTR itemData;
} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT;

/*
 * Real windows resource definitions from WINUSER.H
 * Compatible with MW* types
 */
#pragma pack(2)
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    WORD PACKEDDATA cdit;
    short PACKEDDATA x;
    short PACKEDDATA y;
    short PACKEDDATA cx;
    short PACKEDDATA cy;
} PACKEDDATA DLGTEMPLATE;

typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    short PACKEDDATA x;
    short PACKEDDATA y;
    short PACKEDDATA cx;
    short PACKEDDATA cy;
    WORD PACKEDDATA id;
} PACKEDDATA DLGITEMTEMPLATE;
#pragma pack()

typedef CONST DLGTEMPLATE *LPCDLGTEMPLATE;

BOOL WINAPI MapDialogRect ( HWND hWnd, LPRECT lpRc );

HWND WINAPI CreateDialogParam ( HINSTANCE hInstance, LPCTSTR lpTemplate,
				HWND hWndParent, DLGPROC lpDialogFunc,
				LPARAM dwInitParam );

HWND WINAPI CreateDialogIndirectParam(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate,
		  HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

HWND WINAPI CreateDialog ( HINSTANCE hInstance, LPCSTR lpTemplate,
						   HWND hWndParent, DLGPROC lpDialogFunc );

int WINAPI DialogBox ( HINSTANCE hInstance, LPCTSTR lpTemplate,
		       HWND hWndParent, DLGPROC lpDialogFunc );

int WINAPI DialogBoxParam ( HINSTANCE hInstance, LPCTSTR lpTemplate,
			    HWND hWndParent, DLGPROC lpDialogFunc,
			    LPARAM lParam );

int WINAPI DialogBoxIndirectParam(HINSTANCE hInstance, LPCDLGTEMPLATE hDialogTemplate,
	HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParam);

LONG WINAPI GetDialogBaseUnits(VOID);

BOOL WINAPI EndDialog ( HWND hDlg, int nResult );
DLGBOOL CALLBACK DefDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );


LRESULT WINAPI SendDlgItemMessage ( HWND hwnd, int id, UINT Msg, WPARAM wParam, LPARAM lParam );

BOOL WINAPI IsDialogMessage ( HWND hDlg, LPMSG lpMsg );

UINT WINAPI GetDlgItemText ( HWND hwnd, int id, LPTSTR pStr, int nSize );
BOOL WINAPI SetDlgItemText ( HWND hwnd, int id, LPTSTR pStr );
BOOL WINAPI SetDlgItemInt ( HWND hwnd, int id, UINT val, BOOL bSigned );
UINT WINAPI GetDlgItemInt ( HWND hwnd, int id, BOOL *pbTransl, BOOL bSigned );
UINT IsDlgButtonChecked ( HWND hDlg, int id );
BOOL WINAPI CheckDlgButton ( HWND hDlg, int id, UINT mode );
BOOL WINAPI CheckRadioButton ( HWND hDlg, int idFirst, int idLast, int idCheck );


#endif /*__WINDLG_H__*/
