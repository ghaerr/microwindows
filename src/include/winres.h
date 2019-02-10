#ifndef __WINRES_H__
#define __WINRES_H__
/*
 *  winres.h
 *
 * Microwindows Resource declarations
 *
 * Copyright (C) 2010 - Greg Haerr greg@censoft.com
 * Copyright (C) 2003 - Gabriele Brugnoni
 *
 * gabrielebrugnoni@dveprojects.com
 * DVE Prog. El. - Varese, Italy
 */
#include <stdio.h>	/* for FILE...*/

/*
 *  Application instance data
 */
typedef struct tagMWAPPINSTANCE
{
    LPCSTR szExecCommand;
    LPCSTR szResFilename;
    LPCSTR szCmdLine;
    int argc;
    char **argv;
    FILE *fResources;
} MWAPPINSTANCE, *PMWAPPINSTANCE;


//  Some system functions, used by mw core
void mwFreeInstance ( HINSTANCE hInst );
HINSTANCE mwCreateInstance ( int argc, char *argv[] );


#define MAKEINTRESOURCE(id)	(LPTSTR) MAKELONG((id), 0xFFFF)

/*
 *	Resource header
 */
typedef struct tagMWRESOURCEHEADER
{
    DWORD DataSize;
    DWORD HeaderSize;
    // ordinal type and name are here in file.
    DWORD DataVersion; 
    WORD PACKEDDATA MemoryFlags;		
    WORD PACKEDDATA LanguageId;		
    DWORD PACKEDDATA Version; 
    DWORD PACKEDDATA Characteristics;
} MWRESOURCEHEADER, *PMWRESOURCEHEADER;


/*
 *  Resource type
 */
enum MW_RESOURCE_TYPES {
    W_RT_CURSOR=1,
    W_RT_BITMAP,
    W_RT_ICON,
    W_RT_MENU,
    W_RT_DIALOG,
    W_RT_STRING,
    W_RT_FONTDIR,
    W_RT_FONT,
    W_RT_ACCELERATOR,
    W_RT_RCDATA,
    W_RT_MESSAGETABLE,
    W_RT_GROUP_CURSOR,
    W_RT_GROUP_ICON,
    W_RT_VERSION,
    W_RT_DLGINCLUDE,
    W_RT_PLUGPLAY,
    W_RT_VXD,
    W_RT_ANICURSOR,
    W_RT_ANIICON,
    W_RT_HTML,
};

#define RT_CURSOR		MAKEINTRESOURCE(W_RT_CURSOR)
#define RT_BITMAP		MAKEINTRESOURCE(W_RT_BITMAP)
#define RT_ICON			MAKEINTRESOURCE(W_RT_ICON)
#define RT_MENU			MAKEINTRESOURCE(W_RT_MENU)
#define RT_DIALOG		MAKEINTRESOURCE(W_RT_DIALOG)
#define RT_STRING		MAKEINTRESOURCE(W_RT_STRING)
#define RT_FONTDIR		MAKEINTRESOURCE(W_RT_FONTDIR)
#define RT_FONT			MAKEINTRESOURCE(W_RT_FONT)
#define RT_ACCELERATOR	MAKEINTRESOURCE(W_RT_ACCELERATOR)
#define RT_RCDATA		MAKEINTRESOURCE(W_RT_RCDATA)
#define RT_MESSAGETABLE	MAKEINTRESOURCE(W_RT_MESSAGETABLE)
#define RT_GROUP_CURSOR	MAKEINTRESOURCE(W_RT_GROUP_CURSOR)
#define RT_GROUP_ICON	MAKEINTRESOURCE(W_RT_GROUP_ICON)
#define RT_VERSION		MAKEINTRESOURCE(W_RT_VERSION)
#define RT_DLGINCLUDE	MAKEINTRESOURCE(W_RT_DLGINCLUDE)
#define RT_PLUGPLAY		MAKEINTRESOURCE(W_RT_PLUGPLAY)
#define RT_VXD			MAKEINTRESOURCE(W_RT_VXD)
#define RT_ANICURSOR	MAKEINTRESOURCE(W_RT_ANICURSOR)
#define RT_ANIICON		MAKEINTRESOURCE(W_RT_ANIICON)
#define RT_HTML			MAKEINTRESOURCE(W_RT_HTML)


/*
 *  DIALOG RESOURCE HEADER
 */
typedef struct tagMWDLGTEMPLATE
{
    DWORD style;
    DWORD dwExtendedStyle;
    WORD  PACKEDDATA cdit;
    short PACKEDDATA x;
    short PACKEDDATA y;
    short PACKEDDATA cx;
    short PACKEDDATA cy;
    /* other information that follows, var length */
	char extraData[1];
} MWDLGTEMPLATE, *PMWDLGTEMPLATE;

#define FIXSZ_MWDLGTEMPLATE		18


// Dialog Extra data, dynamic allocated
typedef struct tagMWDLGTEMPLEXTRA
{
	LPTSTR szIdMenu;
	LPTSTR szClassName;
    LPTSTR szDlgName;
    WORD fontSize;
    LPTSTR szFontName;
	int nItems;
	struct tagMWDLGITEMTEMPLATE **pItems;
	struct tagMWDLGITEMTEMPLEXTRA *pItemsExtra;
} MWDLGTEMPLEXTRA, *PMWDLGTEMPLEXTRA;



/*
 *  DIALOG ITEMS
 */
typedef struct tagMWDLGITEMTEMPLATE
{
    DWORD style;
    DWORD dwExtendedStyle;
    short PACKEDDATA x;
    short PACKEDDATA y;
    short PACKEDDATA cx;
    short PACKEDDATA cy;
    WORD  PACKEDDATA id;
    /* other information that follows, var length
	   will be allocated in MWDLGITEMTEMPLEXTRA */
	char extraData[1];
} MWDLGITEMTEMPLATE, *PMWDLGITEMTEMPLATE;

#define FIXSZ_MWDLGITEMTEMPLATE		18

// Dialog ITEMS Extra data, dynamic allocated
typedef struct tagMWDLGITEMTEMPLEXTRA
{
    LPTSTR szClassName;
    LPTSTR szCaption;
    LPWORD lpData;
} MWDLGITEMTEMPLEXTRA, *PMWDLGITEMTEMPLEXTRA;


//  if szClassName length is only 1 char, the class is one of:
enum MW_DLGITEMS_CLASSID
{
    DLGITEM_CLASS_BUTTON = 0x80,
    DLGITEM_CLASS_EDIT,				// 0x81
    DLGITEM_CLASS_STATIC,			// 0x82
    DLGITEM_CLASS_LISTBOX,			// 0x83
    DLGITEM_CLASS_SCROLLBAR,		// 0x84
    DLGITEM_CLASS_COMBOBOX,			// 0x85
}; 

#define DLGITEM_CLASS_FIRSTID	DLGITEM_CLASS_BUTTON

/*
 * In-memory dialog template creating routines
 */
BYTE *resDialogTemplate(BYTE *dest, LPCSTR caption, DWORD style, DWORD dwExtendedStyle,
		int x, int y, int cx, int cy, LPSTR menu, LPSTR classname, int cdit);
BYTE *resDialogItemTemplate(BYTE *dest, DWORD style, DWORD dwExtendedStyle, int id,
		int x, int y, int cx, int cy, int classname, LPCSTR data);

/*
 *  Identify a resource.
 */
typedef struct tagMWRSRC
{
	FILE *f;
	long fPos;
	MWRESOURCEHEADER head;
	LPCTSTR type;
	LPCTSTR name;
	int cLock;
	void *pData;
	struct tagMWRSRC *next;
} MWRSRC, *HRSRC;


//  Resource access internal functions
FILE *mwFindResource ( HINSTANCE hInst, LPCTSTR resType, LPCTSTR resName, PMWRESOURCEHEADER pResHead );

PMWDLGITEMTEMPLATE resNextDlgItem ( PMWDLGITEMTEMPLATE pItem );
PMWDLGITEMTEMPLATE resFirstDlgItem ( PMWDLGTEMPLATE pDlg );
void resDiscardDlgTemplExtra ( PMWDLGTEMPLEXTRA pDlgExtra );
void resGetDlgTemplExtra ( PMWDLGTEMPLATE pDlg, PMWDLGTEMPLEXTRA pDlgExtra );
void resDiscardDlgItemTemplate ( PMWDLGITEMTEMPLEXTRA pItemExtra );
PMWDLGITEMTEMPLATE resGetDlgItemTemplExtra ( PMWDLGITEMTEMPLATE pItem,
							   				 PMWDLGITEMTEMPLEXTRA pItemExtra );
PMWIMAGEHDR resLoadBitmap ( HINSTANCE hInst, LPCTSTR resName );
void resFreeBitmap ( PMWIMAGEHDR pimage );



/*
 *  Exported functions
 */
HRSRC WINAPI FindResource ( HMODULE hModule, LPCTSTR resName, LPCTSTR resType );
DWORD SizeofResource ( HMODULE hModule, HRSRC hResInfo );
HGLOBAL WINAPI LoadResource ( HMODULE hModule, HRSRC hRes );
BOOL WINAPI FreeResource ( HGLOBAL hObj );
int WINAPI UnlockResource ( HGLOBAL hObj );
LPVOID WINAPI LockResource ( HGLOBAL hObj );

int WINAPI LoadString ( HINSTANCE hInstance, UINT uid, LPTSTR lpBuffer, int nMaxBuff );

#endif /*__WINRES_H__*/
