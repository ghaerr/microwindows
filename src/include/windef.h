/* windef.h*/
/*
 * Copyright (c) 1999-2017 Greg Haerr <greg@censoft.com>
 *
 * Win32 API base type definitions
 */

#define _WINDEF_H


#ifdef VXWORKS
/* Don't include the internal Tornado header file <ntcontext.h>, **
** as the definitions in it conflict with these definitions.     */
#define __INCntcontexth
/* Bring in the core VxWorks definitions as they could conflict **
** with the ones below if they are brought in later.            */
#include <vxWorks.h>
#endif


#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef NOMINMAX
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#endif  /* NOMINMAX */

#if WIN32DLL
#ifdef EXPORTS
	#define WINAPI	__declspec(dllexport) __stdcall
#else
	#define WINAPI	__stdcall
#endif
#define STDCALL		__stdcall
#define CALLBACK	__stdcall
#else
#define STDCALL
#define CALLBACK
#define WINAPI
#endif /* !WIN32DLL*/

#define WINAPIV
#define APIENTRY    	WINAPI
#define APIPRIVATE
#define PASCAL

#define FAR
#define NEAR
#define CONST		const
#define CDECL
#define VOID		void

#ifndef VXWORKS
typedef unsigned char 		UCHAR;
typedef unsigned short 		USHORT;
typedef unsigned long		ULONG;
#ifndef __ITRON_TYPES_h_ /* FIXME RTEMS hack*/
typedef int                 	INT;
typedef unsigned int		UINT;
#ifndef COMMON_H	 /* MiniGUI hack*/
typedef int			BOOL;
#endif
#endif
#endif /* !VXWORKS*/

// LONG_PTR definition, must hold int, long and pointer
#if WIN64_PORT
// definition for 64-bit Microsoft Windows port (int=32, long=32, ptr=64)
typedef __int64				LONG_PTR;	// must hold long and pointer
typedef unsigned __int64	ULONG_PTR;	// must hold unsigned long and pointer
typedef unsigned __int64	UINT_PTR;	// must hold unsigned int and pointer
#else
// definition for 64bit port (int=32, long=64, ptr=64)
//             or 32bit port (int=32, long=32, ptr=32)
typedef long				LONG_PTR;	// must hold long and pointer
typedef unsigned long		ULONG_PTR;	// must hold unsigned long and pointer
typedef unsigned long		UINT_PTR;	// must hold unsigned int and pointer
// smaller definition for 16bit WPARAM (int=16, long=32, ptr=16)
//typedef unsigned int		UINT_PTR;	// holds int and pointer
#endif

// Window procedure arguments WPARAM,LPARAM and return value LRESULT must also hold pointers
typedef UINT_PTR		WPARAM;		// holds unsigned int and pointer
typedef LONG_PTR		LPARAM;		// holds long and pointer
typedef LONG_PTR		LRESULT;	// holds long and pointer

// Our dialog procedures return DLGBOOL (incompatible with win32 when sizeof(int) != sizeof(char *))
typedef LRESULT			DLGBOOL;	// mwin dialog procedures return BOOL and pointers
//typedef BOOL			DLGBOOL;	// compatible definition for 16/32 bit systems

// return field offset, must hold long and pointer, used only by MwItemAddr
#define MwItemOffset(type, field)    ((LONG_PTR)&(((type *)0)->field))

// return base item address from list ptr
#define MwItemAddr(p,type,list)	((type *)((LONG_PTR)p - MwItemOffset(type,list)))

// standard definitions
typedef void *PVOID;
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef UCHAR *PUCHAR;
typedef SHORT *PSHORT;  
typedef USHORT *PUSHORT;
typedef LONG *PLONG;    
typedef ULONG *PULONG;    
typedef char *PSZ;

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef uint32_t            DWORD;		// fixed size regardless of architecture

typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef BOOL NEAR           *PBOOL;
typedef BOOL FAR            *LPBOOL;
typedef BYTE NEAR           *PBYTE;
typedef BYTE FAR            *LPBYTE;
typedef int NEAR            *PINT;
typedef int FAR             *LPINT;
typedef unsigned int        *PUINT;
typedef WORD NEAR           *PWORD;
typedef WORD FAR            *LPWORD;
typedef long FAR            *LPLONG;
typedef DWORD NEAR          *PDWORD;
typedef DWORD FAR           *LPDWORD;
typedef void FAR            *LPVOID;
typedef CONST void FAR      *LPCVOID;

typedef unsigned short WCHAR;
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWCH, *PWCH;
typedef CONST WCHAR *LPCWCH, *PCWCH;
typedef WCHAR *NWPSTR;
typedef WCHAR *LPWSTR, *PWSTR;
typedef CONST WCHAR *LPCWSTR, *PCWSTR;

typedef CHAR *PCHAR;
typedef CHAR *LPCH, *PCH;
typedef CONST CHAR *LPCCH, *PCCH;
typedef CHAR *NPSTR;
typedef CHAR *LPSTR, *PSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;
typedef char TCHAR, *PTCHAR;
typedef unsigned char TBYTE , *PTBYTE ;
typedef LPSTR LPTCH, PTCH;
typedef LPSTR PTSTR, LPTSTR;
typedef LPCSTR LPCTSTR;

#define __TEXT(quote) quote
#define TEXT(quote) __TEXT(quote)

typedef int (FAR *FARPROC)();
typedef int (NEAR *NEARPROC)();
typedef int (*PROC)();

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#define PALETTEINDEX(i)     ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))

typedef DWORD   COLORREF;
typedef DWORD   *LPCOLORREF;

typedef LONG	HRESULT;

/* handle typedef*/
typedef PVOID HANDLE;

typedef HANDLE *PHANDLE;
typedef HANDLE NEAR         *SPHANDLE;
typedef HANDLE FAR          *LPHANDLE;
typedef HANDLE              HGLOBAL;
typedef HANDLE              HLOCAL;
typedef HANDLE              GLOBALHANDLE;
typedef HANDLE              LOCALHANDLE;

/* macro for checking if 16 bit "atom" is passed as pointer*/
#define PTR_IS_ATOM(ptr)		((((UINT_PTR)ptr) & ~(UINT_PTR)0xFFFFL) == 0)	/* any bits 16+ and higher zero*/
typedef WORD                ATOM;

typedef struct hwnd *	HWND;
typedef struct hdc *	HDC;
typedef struct hcursor *HCURSOR;
typedef struct hgdiobj *HGDIOBJ;
typedef struct hgdiobj *HBRUSH;
typedef struct hgdiobj *HPEN;
typedef struct hgdiobj *HFONT;
typedef struct hgdiobj *HBITMAP;
typedef struct hgdiobj *HRGN;
typedef struct hgdiobj *HPALETTE;
typedef HANDLE		HICON;
typedef HANDLE		HINSTANCE;
typedef HANDLE		HMODULE;
typedef HANDLE		HMENU;

/* moved to winuser.h for resource compiler*/
/*typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);*/
