/* wingdi.h*/
/*
 * Copyright (c) 1999,2000,2001 Greg Haerr <greg@censoft.com>
 *
 * Win32 GDI structures and API
 */

/* portable coordinate definition*/
typedef MWCOORD	GDICOORD;

/* inherit RECT and POINT from mwtypes.h*/
typedef MWRECT	RECT;
typedef MWPOINT	POINT;

#if 0000
typedef struct  tagRECT {
    GDICOORD left;
    GDICOORD top;
    GDICOORD right;
    GDICOORD bottom;
} RECT;

typedef struct  tagPOINT {
    GDICOORD x;
    GDICOORD y;
} POINT; 
#endif

typedef RECT NEAR *PRECT, FAR *LPRECT;

typedef POINT NEAR *PPOINT, FAR *LPPOINT;

typedef struct  tagSIZE {
    GDICOORD cx;
    GDICOORD cy;
} SIZE, NEAR *PSIZE, FAR *LPSIZE;

/* GetDCEx flags*/
#define DCX_WINDOW           0x00000001L
#define DCX_CACHE            0x00000002L
#define DCX_NORESETATTRS     0x00000004L
#define DCX_CLIPCHILDREN     0x00000008L
#define DCX_CLIPSIBLINGS     0x00000010L
#define DCX_PARENTCLIP       0x00000020L
#define DCX_EXCLUDERGN       0x00000040L
#define DCX_INTERSECTRGN     0x00000080L
#define DCX_EXCLUDEUPDATE    0x00000100L
#define DCX_INTERSECTUPDATE  0x00000200L
#define DCX_LOCKWINDOWUPDATE 0x00000400L
#define DCX_VALIDATE         0x00200000L
#define DCX_DEFAULTCLIP      0x80000000L	/* microwin only*/

HDC WINAPI	GetDCEx(HWND hwnd,HRGN hrgnClip,DWORD flags);
HDC WINAPI	GetDC(HWND hWnd);
HDC WINAPI 	GetWindowDC(HWND hWnd);
int WINAPI 	ReleaseDC(HWND hWnd, HDC hDC);
BOOL WINAPI	DeleteDC(HDC hdc);

typedef struct tagPAINTSTRUCT {
    HDC         hdc;
    BOOL        fErase;			/* indicates bkgnd needs erasing*/
    RECT        rcPaint;		/* nyi*/
    BOOL        fRestore;		/* nyi*/
    BOOL        fIncUpdate;		/* nyi*/
    BYTE        rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, FAR *LPPAINTSTRUCT;

HDC WINAPI 	BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
BOOL WINAPI 	EndPaint(HWND hWnd, CONST PAINTSTRUCT *lpPaint);

#define RGB(r,g,b)	    MWRGB(r,g,b)
#define GetRValue(rgb)      ((BYTE)(rgb))
#define GetGValue(rgb)      ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)      ((BYTE)((rgb)>>16))

/* Background Modes */
#define TRANSPARENT	1
#define OPAQUE		2

/* Text Alignment*/
#define TA_NOUPDATECP                0
#define TA_UPDATECP                  1

#define TA_LEFT                      0
#define TA_RIGHT                     2
#define TA_CENTER                    6

#define TA_TOP                       0
#define TA_BOTTOM                    8
#define TA_BASELINE                  24
#define TA_RTLREADING                256
#define TA_MASK       (TA_BASELINE+TA_CENTER+TA_UPDATECP+TA_RTLREADING)

COLORREF WINAPI	SetTextColor(HDC, COLORREF);
COLORREF WINAPI	SetBkColor(HDC, COLORREF);
int WINAPI 	SetBkMode(HDC, int);
UINT WINAPI     SetTextAlign(HDC hdc, UINT fMode);

/* Binary raster ops*/
#define R2_BLACK            (MWMODE_CLEAR+1)		/*  0       */
#define R2_NOTMERGEPEN      (MWMODE_NOR+1)		/* DPon     */
#define R2_MASKNOTPEN       (MWMODE_ANDINVERTED+1)	/* DPna     */
#define R2_NOTCOPYPEN       (MWMODE_COPYINVERTED+1)	/* Pn       */
#define R2_MASKPENNOT       (MWMODE_ANDREVERSE+1)	/* PDna     */
#define R2_NOT              (MWMODE_INVERT+1)		/* Dn       */
#define R2_XORPEN           (MWMODE_XOR+1)		/* DPx      */
#define R2_NOTMASKPEN       (MWMODE_NAND+1)		/* DPan     */
#define R2_MASKPEN          (MWMODE_AND+1)		/* DPa      */
#define R2_NOTXORPEN        (MWMODE_EQUIV+1)		/* DPxn     */
#define R2_NOP              (MWMODE_NOOP+1)		/* D        */
#define R2_MERGENOTPEN      (MWMODE_ORINVERTED+1)	/* DPno     */
#define R2_COPYPEN          (MWMODE_COPY+1)		/* P        */
#define R2_MERGEPENNOT      (MWMODE_ORREVERSE+1)	/* PDno     */
#define R2_MERGEPEN         (MWMODE_OR+1)		/* DPo      */
#define R2_WHITE            (MWMODE_SETTO1+1)		/*  1       */
#define R2_LAST             16

int WINAPI	SetROP2(HDC hdc, int fnDrawMode);

#define GDI_ERROR	(0xFFFFFFFFL)
#define CLR_INVALID     0xFFFFFFFF

COLORREF WINAPI GetPixel(HDC hdc, int x, int y);
COLORREF WINAPI	SetPixel(HDC hdc, int x, int y, COLORREF crColor);
BOOL WINAPI 	MoveToEx(HDC hdc, int x, int y, LPPOINT lpPoint);
BOOL WINAPI 	LineTo(HDC hdc, int x, int y);
BOOL WINAPI	Polyline(HDC hdc, CONST POINT *lppt, int cPoints);
BOOL WINAPI	PolyPolygon(HDC hdc, CONST POINT *lpPoints, LPINT lpPolyCounts,
			int nCount);
BOOL WINAPI	Rectangle(HDC hdc, int nLeft, int nTop, int nRight,int nBottom);
BOOL WINAPI	Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect,
			int nBottomRect);
BOOL WINAPI	Arc(HDC hdc, int nLeftRect, int nTopRect, int nRightRect,
			int nBottomRect, int nXStartArc, int nYStartArc,
			int nXEndArc, int nYEndArc);
BOOL WINAPI	Pie(HDC hdc, int nLeftRect, int nTopRect, int nRightRect,
			int nBottomRect, int nXRadial1, int nYRadial1,
			int nXRadial2, int nYRadial2);
BOOL WINAPI	Polygon(HDC hdc, CONST POINT *lpPoints, int nCount);
int WINAPI	FillRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr);

/* ExTextOut options*/
#define ETO_OPAQUE	0x0002
#define ETO_CLIPPED	0x0004		/* nyi*/

BOOL WINAPI	TextOut(HDC hdc, int x, int y, LPCSTR lpszString, int cbString);
BOOL WINAPI	ExtTextOut(HDC hdc, int x, int y, UINT fuOptions,
			CONST RECT *lprc, LPCSTR lpszString, UINT cbCount,
			CONST INT *lpDx);
BOOL WINAPI	ExtTextOutW(HDC hdc, int x, int y, UINT fuOptions,
			CONST RECT *lprc, LPCWSTR lpszString, UINT cbCount,
			CONST INT *lpDx);

/* DrawText options*/
#define DT_TOP              0x00000000
#define DT_LEFT             0x00000000
#define DT_CENTER           0x00000001
#define DT_RIGHT            0x00000002
#define DT_VCENTER          0x00000004
#define DT_BOTTOM           0x00000008
#define DT_WORDBREAK        0x00000010
#define DT_SINGLELINE       0x00000020
#define DT_EXPANDTABS       0x00000040
#define DT_TABSTOP          0x00000080
#define DT_NOCLIP           0x00000100
#define DT_EXTERNALLEADING  0x00000200
#define DT_CALCRECT         0x00000400
#define DT_NOPREFIX         0x00000800
#define DT_INTERNAL         0x00001000
#define DT_EDITCONTROL      0x00002000
#define DT_PATH_ELLIPSIS    0x00004000
#define DT_END_ELLIPSIS     0x00008000
#define DT_MODIFYSTRING     0x00010000
#define DT_RTLREADING       0x00020000
#define DT_WORD_ELLIPSIS    0x00040000

int WINAPI	DrawTextA(HDC hdc, LPCSTR lpString, int nCount, LPRECT lpRect,
			UINT uFormat);
int WINAPI	DrawTextW(HDC hdc, LPCWSTR lpString, int nCount, LPRECT lpRect,
			UINT uFormat);
#define DrawText DrawTextA

BOOL WINAPI	DrawDIB(HDC hdc,int x, int y,PMWIMAGEHDR pimage); /* microwin*/

/* GetSysColor, FillRect colors*/
#define COLOR_SCROLLBAR         0
#define COLOR_BACKGROUND        1
#define COLOR_ACTIVECAPTION     2
#define COLOR_INACTIVECAPTION   3
#define COLOR_MENU              4
#define COLOR_WINDOW            5
#define COLOR_WINDOWFRAME       6
#define COLOR_MENUTEXT          7
#define COLOR_WINDOWTEXT        8
#define COLOR_CAPTIONTEXT       9
#define COLOR_ACTIVEBORDER      10
#define COLOR_INACTIVEBORDER    11
#define COLOR_APPWORKSPACE      12
#define COLOR_HIGHLIGHT         13
#define COLOR_HIGHLIGHTTEXT     14
#define COLOR_BTNFACE           15
#define COLOR_BTNSHADOW         16
#define COLOR_GRAYTEXT          17
#define COLOR_BTNTEXT           18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT      20
#define COLOR_3DDKSHADOW        21
#define COLOR_3DLIGHT           22
#define COLOR_INFOTEXT          23
#define COLOR_INFOBK            24
#define COLOR_DESKTOP           COLOR_BACKGROUND
#define COLOR_3DFACE            COLOR_BTNFACE
#define COLOR_3DSHADOW          COLOR_BTNSHADOW
#define COLOR_3DHIGHLIGHT       COLOR_BTNHIGHLIGHT
#define COLOR_3DHILIGHT         COLOR_BTNHIGHLIGHT
#define COLOR_BTNHILIGHT        COLOR_BTNHIGHLIGHT
#define COLOR_ALTERNATEBUTTONFACE	25
#define COLOR_HOTLIGHT                  26
#define COLOR_GRADIENTACTIVECAPTION     27
#define COLOR_GRADIENTINACTIVECAPTION   28

DWORD WINAPI	GetSysColor(int nIndex);
COLORREF WINAPI	SetSysColor(int nIndex, COLORREF crColor);/* Microwindows only*/

/* Stock Logical Objects */
#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
#define DEFAULT_GUI_FONT    17
#define DC_BRUSH            18
#define DC_PEN              19
#define STOCK_LAST          19

/* Object types*/
#define OBJ_PEN             1
#define OBJ_BRUSH           2
#define OBJ_DC              3
#define OBJ_METADC          4
#define OBJ_PAL             5
#define OBJ_FONT            6
#define OBJ_BITMAP          7
#define OBJ_REGION          8
#define OBJ_METAFILE        9
#define OBJ_MEMDC           10
#define OBJ_EXTPEN          11
#define OBJ_ENHMETADC       12
#define OBJ_ENHMETAFILE     13

HGDIOBJ WINAPI	GetStockObject(int nObject);
HGDIOBJ WINAPI	SelectObject(HDC hdc, HGDIOBJ hObject);
BOOL WINAPI	DeleteObject(HGDIOBJ hObject);
int WINAPI	SelectClipRgn(HDC hdc, HRGN hrgn);
int WINAPI	ExtSelectClipRgn(HDC hdc, HRGN hrgn, int fnMode);
int WINAPI	GetUpdateRgn(HWND hwnd, HRGN hrgn, BOOL bErase);
BOOL WINAPI	GetUpdateRect(HWND hwnd, LPRECT lpRect, BOOL bErase);

/* Brush Styles */
#define BS_SOLID            0
#define BS_NULL             1
#define BS_HOLLOW           BS_NULL

HBRUSH WINAPI	CreateSolidBrush(COLORREF crColor);

/* Pen Styles */
#define PS_SOLID            0
#define PS_NULL             5

HPEN WINAPI	CreatePen(int nPenStyle, int nWidth, COLORREF crColor);

HBITMAP WINAPI	CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight);
HDC WINAPI	CreateCompatibleDC(HDC hdc);

/* BitBlit raster opcodes*/
#define SRCCOPY         (DWORD)MWROP_COPY	  /* source                   */
#define SRCPAINT        (DWORD)MWROP_OR		  /* source OR dest           */
#define SRCAND          (DWORD)MWROP_AND	  /* source AND dest          */
#define SRCINVERT       (DWORD)MWROP_XOR	  /* source XOR dest          */
#define SRCERASE        (DWORD)MWROP_ANDREVERSE	  /* source AND (NOT dest )   */
#define NOTSRCCOPY      (DWORD)MWROP_COPYINVERTED /* (NOT source)             */
#define NOTSRCERASE     (DWORD)MWROP_NOR	  /* (NOT src) AND (NOT dest) */
#define MERGEPAINT      (DWORD)MWROP_ORINVERTED   /* (NOT source) OR dest     */
#define DSTINVERT       (DWORD)MWROP_INVERT	  /* (NOT dest)               */
#define BLACKNESS       (DWORD)MWROP_CLEAR	  /* BLACK                    */
#define WHITENESS       (DWORD)MWROP_SET	  /* WHITE                    */
#if 0
#define MERGECOPY       (DWORD)0x00C000CA /* (source AND pattern)     */
#define PATCOPY         (DWORD)0x00F00021 /* pattern                  */
#define PATPAINT        (DWORD)0x00FB0A09 /* DPSnoo                   */
#define PATINVERT       (DWORD)0x005A0049 /* pattern XOR dest         */
#endif

BOOL WINAPI	BitBlt(HDC hdcDest,int nXDest,int nYDest,int nWidth,int nHeight,
			HDC hdcSrc,int nXSrc,int nYSrc,DWORD dwRop);
BOOL WINAPI	StretchBlt(HDC hdcDest,int nXOriginDest,int nYOriginDest,
			int nWidthDest,int nHeightDest,HDC hdcSrc,
			int nXOriginSrc,int nYOriginSrc,int nWidthSrc,
			int nHeightSrc, DWORD dwRop);

/* Palette entry flags*/
#define PC_RESERVED	0x01
#define PC_EXPLICIT	0x02
#define PC_NOCOLLAPSE	0x04

typedef struct tagPALETTEENTRY {
	BYTE	peRed;
	BYTE	peGreen;
	BYTE	peBlue;
	BYTE	peFlags;
} PALETTEENTRY, *PPALETTEENTRY, FAR *LPPALETTEENTRY;

UINT WINAPI	GetSystemPaletteEntries(HDC hdc,UINT iStartIndex,UINT nEntries,
			LPPALETTEENTRY lppe);

/* GetDeviceCaps parameters*/
#define HORZRES       8     /* Horizontal width in pixels               */
#define VERTRES       10    /* Vertical height in pixels                */
#define BITSPIXEL     12    /* Number of bits per pixel                 */
#define PLANES        14    /* Number of planes                         */
#define LOGPIXELSX    88    /* Logical pixels/inch in X                 */
#define LOGPIXELSY    90    /* Logical pixels/inch in Y                 */
#define SIZEPALETTE  104    /* Number of entries in physical palette    */

int WINAPI	GetDeviceCaps(HDC hdc, int nIndex);

/* Region flags*/
#define ERRORREGION		MWREGION_ERROR
#define NULLREGION		MWREGION_NULL
#define SIMPLEREGION		MWREGION_SIMPLE
#define COMPLEXREGION		MWREGION_COMPLEX
/* kluge for VxWorks*/
#ifdef ERROR
#undef ERROR
#endif
#define ERROR			ERRORREGION
#define RGN_ERROR		ERRORREGION

/* CombineRgn() Styles */
#define RGN_AND             1
#define RGN_OR              2
#define RGN_XOR             3
#define RGN_DIFF            4
#define RGN_COPY            5
#define RGN_MIN             RGN_AND
#define RGN_MAX             RGN_COPY

/* GetRegionData/ExtCreateRegion */
#define RDH_RECTANGLES  1
typedef struct _RGNDATAHEADER {
    DWORD   dwSize;
    DWORD   iType;
    DWORD   nCount;
    DWORD   nRgnSize;
    RECT    rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER   rdh;
    char            Buffer[1];
} RGNDATA, *PRGNDATA, *NPRGNDATA, *LPRGNDATA;

/* Region entry points*/
INT  WINAPI OffsetRgn(HRGN hrgn, INT x, INT y );
INT  WINAPI GetRgnBox(HRGN hrgn, LPRECT rect );
HRGN WINAPI CreateRectRgn(INT left, INT top, INT right, INT bottom);
HRGN WINAPI CreateRectRgnIndirect(const RECT* rect );
VOID WINAPI SetRectRgn(HRGN hrgn, INT left, INT top, INT right, INT bottom );
HRGN WINAPI CreateRoundRectRgn(INT left, INT top, INT right, INT bottom,
		INT ellipse_width, INT ellipse_height );
HRGN WINAPI CreateEllipticRgn(INT left, INT top, INT right, INT bottom );
HRGN WINAPI CreateEllipticRgnIndirect(const RECT *rect );
DWORD WINAPI GetRegionData(HRGN hrgn, DWORD count, LPRGNDATA rgndata);
BOOL WINAPI PtInRegion(HRGN hrgn, INT x, INT y );
BOOL WINAPI RectInRegion(HRGN hrgn, const RECT *rect );
BOOL WINAPI EqualRgn(HRGN hrgn1, HRGN hrgn2 );
INT  WINAPI CombineRgn(HRGN hDest, HRGN hSrc1, HRGN hSrc2, INT mode);

/* Rect entry points*/
BOOL WINAPI IntersectRect(LPRECT dest, const RECT *src1, const RECT *src2 );
BOOL WINAPI UnionRect(LPRECT dest, const RECT *src1, const RECT *src2 );
BOOL WINAPI EqualRect(const RECT* rect1, const RECT* rect2 );
BOOL WINAPI SubtractRect(LPRECT dest, const RECT *src1, const RECT *src2 );

