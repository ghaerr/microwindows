/* extra_windef.h*/
#pragma once

typedef struct _RECTL {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECTL, *PRECTL, *LPRECTL;

typedef const RECT *LPCRECT;

 typedef RECT NEAR *PRECT, FAR *LPRECT;
 
typedef struct _POINTL
{
	LONG x;
	LONG y;
} POINTL, *PPOINTL;

typedef SIZE SIZEL;
typedef SIZE *PSIZEL, *LPSIZEL;

BOOL WINAPI RestoreDC(HDC hdc,int nSavedDC);
int WINAPI SaveDC(HDC hdc);
BOOL WINAPI GdiFlush();

#pragma pack(2)
typedef struct tagMETAHEADER {
	WORD mtType;
	WORD mtHeaderSize;
	WORD mtVersion;
	DWORD mtSize;
	WORD mtNoObjects;
	DWORD mtMaxRecord;
	WORD mtNoParameters;
} METAHEADER, *PMETAHEADER, *LPMETAHEADER;
#pragma pack()

typedef struct tagENHMETAHEADER {
	DWORD iType;
	DWORD nSize;
	RECTL rclBounds;
	RECTL rclFrame;
	DWORD dSignature;
	DWORD nVersion;
	DWORD nBytes;
	DWORD nRecords;
	WORD nHandles;
	WORD sReserved;
	DWORD nDescription;
	DWORD offDescription;
	DWORD nPalEntries;
	SIZEL szlDevice;
	SIZEL szlMillimeters;
#if (WINVER >= 0x0400)
	DWORD cbPixelFormat;
	DWORD offPixelFormat;
	DWORD bOpenGL;
#endif
#if (WINVER >= 0x0500)
	SIZEL szlMicrometers;
#endif
} ENHMETAHEADER, *PENHMETAHEADER, *LPENHMETAHEADER;
typedef struct tagMETARECORD {
	DWORD rdSize;
	WORD rdFunction;
	WORD rdParm[1];
} METARECORD, *PMETARECORD, *LPMETARECORD;
typedef struct tagENHMETARECORD {
	DWORD iType;
	DWORD nSize;
	DWORD dParm[1];
} ENHMETARECORD, *LPENHMETARECORD;
typedef struct tagHANDLETABLE {
	HGDIOBJ objectHandle[1];
} HANDLETABLE, *PHANDLETABLE, *LPHANDLETABLE;


typedef struct _DOCINFOW {
	int cbSize;
	LPCWSTR lpszDocName;
	LPCWSTR lpszOutput;
	LPCWSTR lpszDatatype;
	DWORD fwType;
} DOCINFOW, *LPDOCINFOW;
typedef struct tagEMR {
	DWORD iType;
	DWORD nSize;
} EMR, *PEMR;

#if(WINVER >= 0x0400)
typedef struct tagEMRGLSRECORD {
	EMR emr;
	DWORD cbData;
	BYTE Data[1];
} EMRGLSRECORD, *PEMRGLSRECORD;
typedef struct tagEMRGLSBOUNDEDRECORD {
	EMR emr;
	RECTL rclBounds;
	DWORD cbData;
	BYTE Data[1];
} EMRGLSBOUNDEDRECORD, *PEMRGLSBOUNDEDRECORD;
#endif
typedef struct tagEMRANGLEARC {
	EMR emr;
	POINTL ptlCenter;
	DWORD nRadius;
	FLOAT eStartAngle;
	FLOAT eSweepAngle;
} EMRANGLEARC, *PEMRANGLEARC;
typedef struct tagEMRARC {
	EMR emr;
	RECTL rclBox;
	POINTL ptlStart;
	POINTL ptlEnd;
} EMRARC, *PEMRARC, EMRARCTO, *PEMRARCTO, EMRCHORD, *PEMRCHORD, EMRPIE, *PEMRPIE;

typedef struct tagEMRBITBLT {
	EMR emr;
	RECTL rclBounds;
	LONG xDest;
	LONG yDest;
	LONG cxDest;
	LONG cyDest;
	DWORD dwRop;
	LONG xSrc;
	LONG ySrc;
	XFORM xformSrc;
	COLORREF crBkColorSrc;
	DWORD iUsageSrc;
	DWORD offBmiSrc;
	DWORD cbBmiSrc;
	DWORD offBitsSrc;
	DWORD cbBitsSrc;
} EMRBITBLT, *PEMRBITBLT;
typedef struct tagLOGBRUSH {
	UINT lbStyle;
	COLORREF lbColor;
	ULONG_PTR lbHatch;
} LOGBRUSH, *PLOGBRUSH, *LPLOGBRUSH;

#define AC_SRC_OVER                 0x00

#pragma pack(1)
typedef struct _BLENDFUNCTION {
	BYTE BlendOp;
	BYTE BlendFlags;
	BYTE SourceConstantAlpha;
	BYTE AlphaFormat;
} BLENDFUNCTION, *PBLENDFUNCTION, *LPBLENDFUNCTION;
#pragma pack()

#define GRADIENT_FILL_RECT_H      0x00000000
#define GRADIENT_FILL_RECT_V      0x00000001
#define GRADIENT_FILL_TRIANGLE    0x00000002
#define GRADIENT_FILL_OP_FLAG     0x000000ff

typedef struct tagBITMAP {
	LONG	bmType;
	LONG	bmWidth;
	LONG	bmHeight;
	LONG	bmWidthBytes;
	WORD	bmPlanes;
	WORD	bmBitsPixel;
	LPVOID	bmBits;
} BITMAP, *PBITMAP, *LPBITMAP;
typedef struct tagBITMAPCOREHEADER {
	DWORD	bcSize;
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} BITMAPCOREHEADER, *LPBITMAPCOREHEADER, *PBITMAPCOREHEADER;

typedef struct _ICONINFO {
	BOOL fIcon;
	DWORD xHotspot;
	DWORD yHotspot;
	HBITMAP hbmMask;
	HBITMAP hbmColor;
} ICONINFO, *PICONINFO;

#define PT_MOVETO	6
#define PT_LINETO	2
#define PT_BEZIERTO	4
#define PT_CLOSEFIGURE 1


#ifndef _TEXTMETRIC_DEFINED
#define _TEXTMETRIC_DEFINED
typedef struct tagTEXTMETRICA {
	LONG tmHeight;
	LONG tmAscent;
	LONG tmDescent;
	LONG tmInternalLeading;
	LONG tmExternalLeading;
	LONG tmAveCharWidth;
	LONG tmMaxCharWidth;
	LONG tmWeight;
	LONG tmOverhang;
	LONG tmDigitizedAspectX;
	LONG tmDigitizedAspectY;
	BYTE tmFirstChar;
	BYTE tmLastChar;
	BYTE tmDefaultChar;
	BYTE tmBreakChar;
	BYTE tmItalic;
	BYTE tmUnderlined;
	BYTE tmStruckOut;
	BYTE tmPitchAndFamily;
	BYTE tmCharSet;
} TEXTMETRICA, *PTEXTMETRICA, *LPTEXTMETRICA;
typedef struct tagTEXTMETRICW {
	LONG tmHeight;
	LONG tmAscent;
	LONG tmDescent;
	LONG tmInternalLeading;
	LONG tmExternalLeading;
	LONG tmAveCharWidth;
	LONG tmMaxCharWidth;
	LONG tmWeight;
	LONG tmOverhang;
	LONG tmDigitizedAspectX;
	LONG tmDigitizedAspectY;
	WCHAR tmFirstChar;
	WCHAR tmLastChar;
	WCHAR tmDefaultChar;
	WCHAR tmBreakChar;
	BYTE tmItalic;
	BYTE tmUnderlined;
	BYTE tmStruckOut;
	BYTE tmPitchAndFamily;
	BYTE tmCharSet;
} TEXTMETRICW, *PTEXTMETRICW, *LPTEXTMETRICW;
#endif 

typedef struct tagPANOSE {
	BYTE bFamilyType;
	BYTE bSerifStyle;
	BYTE bWeight;
	BYTE bProportion;
	BYTE bContrast;
	BYTE bStrokeVariation;
	BYTE bArmStyle;
	BYTE bLetterform;
	BYTE bMidline;
	BYTE bXHeight;
} PANOSE, *LPPANOSE;

//#define LF_FACESIZE	32
#define LF_FULLFACESIZE	64
#define ELF_VENDOR_SIZE	4
#define ELF_VERSION	0
#define ELF_CULTURE_LATIN	0

typedef struct tagLOGFONTA {
	LONG	lfHeight;
	LONG	lfWidth;
	LONG	lfEscapement;
	LONG	lfOrientation;
	LONG	lfWeight;
	BYTE	lfItalic;
	BYTE	lfUnderline;
	BYTE	lfStrikeOut;
	BYTE	lfCharSet;
	BYTE	lfOutPrecision;
	BYTE	lfClipPrecision;
	BYTE	lfQuality;
	BYTE	lfPitchAndFamily;
	CHAR	lfFaceName[LF_FACESIZE];
} LOGFONTA, *PLOGFONTA, *LPLOGFONTA;
typedef struct tagLOGFONTW {
	LONG	lfHeight;
	LONG	lfWidth;
	LONG	lfEscapement;
	LONG	lfOrientation;
	LONG	lfWeight;
	BYTE	lfItalic;
	BYTE	lfUnderline;
	BYTE	lfStrikeOut;
	BYTE	lfCharSet;
	BYTE	lfOutPrecision;
	BYTE	lfClipPrecision;
	BYTE	lfQuality;
	BYTE	lfPitchAndFamily;
	WCHAR	lfFaceName[LF_FACESIZE];
} LOGFONTW, *PLOGFONTW, *LPLOGFONTW;
typedef struct tagEXTLOGFONTA {
	LOGFONTA	elfLogFont;
	BYTE	elfFullName[LF_FULLFACESIZE];
	BYTE	elfStyle[LF_FACESIZE];
	DWORD	elfVersion;
	DWORD	elfStyleSize;
	DWORD	elfMatch;
	DWORD	elfReserved;
	BYTE	elfVendorId[ELF_VENDOR_SIZE];
	DWORD	elfCulture;
	PANOSE	elfPanose;
} EXTLOGFONTA, *PEXTLOGFONTA, *LPEXTLOGFONTA;
typedef struct tagEXTLOGFONTW {
	LOGFONTW	elfLogFont;
	WCHAR	elfFullName[LF_FULLFACESIZE];
	WCHAR	elfStyle[LF_FACESIZE];
	DWORD	elfVersion;
	DWORD	elfStyleSize;
	DWORD	elfMatch;
	DWORD	elfReserved;
	BYTE	elfVendorId[ELF_VENDOR_SIZE];
	DWORD	elfCulture;
	PANOSE	elfPanose;
} EXTLOGFONTW, *PEXTLOGFONTW, *LPEXTLOGFONTW;

typedef struct _OUTLINETEXTMETRICA {
	UINT otmSize;
	TEXTMETRICA otmTextMetrics;
	BYTE otmFiller;
	PANOSE otmPanoseNumber;
	UINT otmfsSelection;
	UINT otmfsType;
	int otmsCharSlopeRise;
	int otmsCharSlopeRun;
	int otmItalicAngle;
	UINT otmEMSquare;
	int otmAscent;
	int otmDescent;
	UINT otmLineGap;
	UINT otmsCapEmHeight;
	UINT otmsXHeight;
	RECT otmrcFontBox;
	int otmMacAscent;
	int otmMacDescent;
	UINT otmMacLineGap;
	UINT otmusMinimumPPEM;
	POINT otmptSubscriptSize;
	POINT otmptSubscriptOffset;
	POINT otmptSuperscriptSize;
	POINT otmptSuperscriptOffset;
	UINT otmsStrikeoutSize;
	int otmsStrikeoutPosition;
	int otmsUnderscoreSize;
	int otmsUnderscorePosition;
	PSTR otmpFamilyName;
	PSTR otmpFaceName;
	PSTR otmpStyleName;
	PSTR otmpFullName;
} OUTLINETEXTMETRIC, OUTLINETEXTMETRICA, *POUTLINETEXTMETRICA, *LPOUTLINETEXTMETRICA;

typedef struct _OUTLINETEXTMETRICW {
	UINT otmSize;
	TEXTMETRICW otmTextMetrics;
	BYTE otmFiller;
	PANOSE otmPanoseNumber;
	UINT otmfsSelection;
	UINT otmfsType;
	int otmsCharSlopeRise;
	int otmsCharSlopeRun;
	int otmItalicAngle;
	UINT otmEMSquare;
	int otmAscent;
	int otmDescent;
	UINT otmLineGap;
	UINT otmsCapEmHeight;
	UINT otmsXHeight;
	RECT otmrcFontBox;
	int otmMacAscent;
	int otmMacDescent;
	UINT otmMacLineGap;
	UINT otmusMinimumPPEM;
	POINT otmptSubscriptSize;
	POINT otmptSubscriptOffset;
	POINT otmptSuperscriptSize;
	POINT otmptSuperscriptOffset;
	UINT otmsStrikeoutSize;
	int otmsStrikeoutPosition;
	int otmsUnderscoreSize;
	int otmsUnderscorePosition;
	PSTR otmpFamilyName;
	PSTR otmpFaceName;
	PSTR otmpStyleName;
	PSTR otmpFullName;
} OUTLINETEXTMETRICW, *POUTLINETEXTMETRICW, *LPOUTLINETEXTMETRICW;
 
 
 /* ExTextOut options*/
#define BS_HATCHED	    2
#define BS_PATTERN	    3
#define BS_INDEXED	    4
#define	BS_DIBPATTERN	    5
#define	BS_DIBPATTERNPT	    6
#define BS_PATTERN8X8	    7
#define	BS_DIBPATTERN8X8    8
#define BS_MONOPATTERN      9
 
 /* Pen Styles */
/* CreatePenIndirect */
//#define PS_SOLID         0x00000000
//#define PS_DASH          0x00000001
//#define PS_DOT           0x00000002
//#define PS_DASHDOT       0x00000003
//#define PS_DASHDOTDOT    0x00000004
//#define PS_NULL          0x00000005
#define PS_INSIDEFRAME   0x00000006
#define PS_USERSTYLE     0x00000007
#define PS_ALTERNATE     0x00000008
#define PS_STYLE_MASK    0x0000000F
 
typedef struct tagDIBSECTION {
	BITMAP dsBm;
	BITMAPINFOHEADER dsBmih;
	DWORD dsBitfields[3];
	HANDLE dshSection;
	DWORD dsOffset;
} DIBSECTION, *PDIBSECTION, *LPDIBSECTION;

 
BOOL WINAPI TransparentBlt(
	HDC  hdcDest,
	int  xoriginDest,
	int  yoriginDest,
	int  wDest,
	int  hDest,
	HDC  hdcSrc,
	int  xoriginSrc,
	int  yoriginSrc,
	int  wSrc,
	int  hSrc,
	UINT crTransparent
);
int WINAPI StretchDIBits(
	HDC              hdc,
	int              xDest,
	int              yDest,
	int              DestWidth,
	int              DestHeight,
	int              xSrc,
	int              ySrc,
	int              SrcWidth,
	int              SrcHeight,
	const VOID       *lpBits,
	const BITMAPINFO *lpbmi,
	UINT             iUsage,
	DWORD            rop
);
 
 /* GetDeviceCaps parameters*/
#define DRIVERVERSION     0
#define TECHNOLOGY        2
#define HORZSIZE          4
#define VERTSIZE          6
// #define HORZRES       8     /* Horizontal width in pixels               */
// #define VERTRES       10    /* Vertical height in pixels                */
// #define BITSPIXEL     12    /* Number of bits per pixel                 */
// #define PLANES        14    /* Number of planes                         */
#define NUMBRUSHES        16
#define NUMPENS           18
#define NUMMARKERS        20
#define NUMFONTS          22
#define NUMCOLORS         24
#define PDEVICESIZE       26
#define CURVECAPS         28
#define LINECAPS          30
#define POLYGONALCAPS     32
#define TEXTCAPS          34
#define CLIPCAPS          36
#define RASTERCAPS        38
#define ASPECTX           40
#define ASPECTY           42
#define ASPECTXY          44
// #define LOGPIXELSX    88    /* Logical pixels/inch in X                 */
// #define LOGPIXELSY    90    /* Logical pixels/inch in Y                 */
// #define SIZEPALETTE  104    /* Number of entries in physical palette    */
#define CAPS1             94
#define NUMRESERVED       106
#define COLORRES          108

/* RASTERCAPS */
#define RC_NONE           0x0000
#define RC_BITBLT         0x0001
#define RC_BANDING        0x0002
#define RC_SCALING        0x0004
#define RC_BITMAP64       0x0008
#define RC_GDI20_OUTPUT   0x0010
#define RC_GDI20_STATE    0x0020
#define RC_SAVEBITMAP     0x0040
#define RC_DI_BITMAP      0x0080
#define RC_PALETTE        0x0100
#define RC_DIBTODEV       0x0200
#define RC_BIGFONT        0x0400
#define RC_STRETCHBLT     0x0800
#define RC_FLOODFILL      0x1000
#define RC_STRETCHDIB     0x2000
#define RC_OP_DX_OUTPUT   0x4000
#define RC_DEVBITS        0x8000
 
HRGN WINAPI ExtCreateRegion(const XFORM   *lpx,DWORD         nCount,const RGNDATA *lpData);
int WINAPI GetClipBox(	HDC    hdc,	LPRECT lprect);
int WINAPI GetClipRgn(	HDC  hdc,	HRGN hrgn);
int WINAPI GetGraphicsMode(HDC hdc);
int WINAPI IntersectClipRect(HDC hdc,int left,	int top,int right,int bottom);
int WINAPI SetGraphicsMode(HDC hdc,int iMode);
BOOL WINAPI GetWorldTransform(	HDC     hdc,	LPXFORM lpxf);
BOOL WINAPI SetWorldTransform(	HDC         hdc,	const XFORM *lpxf);
BOOL WINAPI ModifyWorldTransform(HDC         hdc,	const XFORM *lpxf,	DWORD       mode);
int WINAPI	SetMapMode(	HDC hdc,	int iMode);
int WINAPI GetObjectW(	HANDLE h,	int    c,	LPVOID pv);

 
BOOL WINAPI SetViewportOrgEx(
	HDC     hdc,
	int     x,
	int     y,
	LPPOINT lppt
);
// added form gdiplust

#define EMR_HEADER	1
#define EMR_POLYBEZIER 2
#define EMR_POLYGON	3
#define EMR_POLYLINE	4
#define EMR_POLYBEZIERTO	5
#define EMR_POLYLINETO 6
#define EMR_POLYPOLYLINE	7
#define EMR_POLYPOLYGON 8
#define EMR_SETWINDOWEXTEX	9
#define EMR_SETWINDOWORGEX	10
#define EMR_SETVIEWPORTEXTEX 11
#define EMR_SETVIEWPORTORGEX 12
#define EMR_SETBRUSHORGEX 13
#define EMR_EOF 14
#define EMR_SETPIXELV 15
#define EMR_SETMAPPERFLAGS 16
#define EMR_SETMAPMODE 17
#define EMR_SETBKMODE 18
#define EMR_SETPOLYFILLMODE 19
#define EMR_SETROP2 20
#define EMR_SETSTRETCHBLTMODE 21
#define EMR_SETTEXTALIGN 22
#define EMR_SETCOLORADJUSTMENT 23
#define EMR_SETTEXTCOLOR 24
#define EMR_SETBKCOLOR 25
#define EMR_OFFSETCLIPRGN 26
#define EMR_MOVETOEX 27
#define EMR_SETMETARGN 28
#define EMR_EXCLUDECLIPRECT 29
#define EMR_INTERSECTCLIPRECT 30
#define EMR_SCALEVIEWPORTEXTEX 31
#define EMR_SCALEWINDOWEXTEX 32
#define EMR_SAVEDC 33
#define EMR_RESTOREDC 34
#define EMR_SETWORLDTRANSFORM 35
#define EMR_MODIFYWORLDTRANSFORM 36
#define EMR_SELECTOBJECT 37
#define EMR_CREATEPEN 38
#define EMR_CREATEBRUSHINDIRECT 39
#define EMR_DELETEOBJECT 40
#define EMR_ANGLEARC 41
#define EMR_ELLIPSE  42
#define EMR_RECTANGLE 43
#define EMR_ROUNDRECT 44
#define EMR_ARC 45
#define EMR_CHORD 46
#define EMR_PIE 47
#define EMR_SELECTPALETTE 48
#define EMR_CREATEPALETTE 49
#define EMR_SETPALETTEENTRIES 50
#define EMR_RESIZEPALETTE 51
#define EMR_REALIZEPALETTE 52
#define EMR_EXTFLOODFILL 53
#define EMR_LINETO 54
#define EMR_ARCTO 55
#define EMR_POLYDRAW 56
#define EMR_SETARCDIRECTION 57
#define EMR_SETMITERLIMIT 58
#define EMR_BEGINPATH 59
#define EMR_ENDPATH 60
#define EMR_CLOSEFIGURE 61
#define EMR_FILLPATH 62
#define EMR_STROKEANDFILLPATH 63
#define EMR_STROKEPATH 64
#define EMR_FLATTENPATH 65
#define EMR_WIDENPATH 66
#define EMR_SELECTCLIPPATH 67
#define EMR_ABORTPATH 68
#define EMR_GDICOMMENT 70
#define EMR_FILLRGN 71
#define EMR_FRAMERGN 72
#define EMR_INVERTRGN 73
#define EMR_PAINTRGN 74
#define EMR_EXTSELECTCLIPRGN 75
#define EMR_BITBLT 76
#define EMR_STRETCHBLT 77
#define EMR_MASKBLT 78
#define EMR_PLGBLT 79
#define EMR_SETDIBITSTODEVICE 80
#define EMR_STRETCHDIBITS 81
#define EMR_EXTCREATEFONTINDIRECTW 82
#define EMR_EXTTEXTOUTA 83
#define EMR_EXTTEXTOUTW 84
#define EMR_POLYBEZIER16 85
#define EMR_POLYGON16 86
#define EMR_POLYLINE16 87
#define EMR_POLYBEZIERTO16 88
#define EMR_POLYLINETO16 89
#define EMR_POLYPOLYLINE16 90
#define EMR_POLYPOLYGON16 91
#define EMR_POLYDRAW16 92
#define EMR_CREATEMONOBRUSH 93
#define EMR_CREATEDIBPATTERNBRUSHPT 94
#define EMR_EXTCREATEPEN 95
#define EMR_POLYTEXTOUTA 96
#define EMR_POLYTEXTOUTW 97
#define EMR_SETICMMODE 98
#define EMR_CREATECOLORSPACE 99
#define EMR_SETCOLORSPACE 100
#define EMR_DELETECOLORSPACE 101
#define EMR_GLSRECORD 102
#define EMR_GLSBOUNDEDRECORD 103
#define EMR_PIXELFORMAT 104
#define EMR_DRAWESCAPE 105
#define EMR_EXTESCAPE 106
#define EMR_STARTDOC 107
#define EMR_SMALLTEXTOUT 108
#define EMR_FORCEUFIMAPPING 109
#define EMR_NAMEDESCAPE 110
#define EMR_COLORCORRECTPALETTE 111
#define EMR_SETICMPROFILEA 112
#define EMR_SETICMPROFILEW 113
#define EMR_ALPHABLEND 114
#define EMR_ALPHADIBBLEND 115
#define EMR_SETLAYOUT 115
#define EMR_TRANSPARENTBLT 116
#define EMR_TRANSPARENTDIB 117
#define EMR_RESERVED_117 117
#define EMR_GRADIENTFILL 118
#define EMR_SETLINKEDUFIS 119
#define EMR_SETTEXTJUSTIFICATION 120
#define EMR_COLORMATCHTOTARGETW 121
#define EMR_CREATECOLORSPACEW 122

#ifndef MW
#define EMR_MIN 1
#define EMR_MAX 122 


#define META_SETBKCOLOR	0x201
#define META_SETBKMODE	0x102
#define META_SETMAPMODE	0x103
#define META_SETROP2	0x104
#define META_SETRELABS	0x105
#define META_SETPOLYFILLMODE	0x106
#define META_SETSTRETCHBLTMODE	0x107
#define META_SETTEXTCHAREXTRA	0x108
#define META_SETTEXTCOLOR	0x209
#define META_SETTEXTJUSTIFICATION	0x20A
#define META_SETWINDOWORG	0x20B
#define META_SETWINDOWEXT	0x20C
#define META_SETVIEWPORTORG	0x20D
#define META_SETVIEWPORTEXT	0x20E
#define META_OFFSETWINDOWORG	0x20F
#define META_SCALEWINDOWEXT	0x410
#define META_OFFSETVIEWPORTORG	0x211
#define META_SCALEVIEWPORTEXT	0x412
#define META_LINETO	0x213
#define META_MOVETO	0x214
#define META_EXCLUDECLIPRECT	0x415
#define META_INTERSECTCLIPRECT	0x416
#define META_ARC	0x817
#define META_ELLIPSE	0x418
#define META_FLOODFILL	0x419
#define META_PIE	0x81A
#define META_RECTANGLE	0x41B
#define META_ROUNDRECT	0x61C
#define META_PATBLT	0x61D
#define META_SAVEDC	0x1E
#define META_SETPIXEL	0x41F
#define META_OFFSETCLIPRGN	0x220
#define META_TEXTOUT	0x521
#define META_BITBLT	0x922
#define META_STRETCHBLT	0xB23
#define META_POLYGON	0x324
#define META_POLYLINE	0x325
#define META_ESCAPE	0x626
#define META_RESTOREDC	0x127
#define META_FILLREGION	0x228
#define META_FRAMEREGION	0x429
#define META_INVERTREGION	0x12A
#define META_PAINTREGION	0x12B
#define META_SELECTCLIPREGION	0x12C
#define META_SELECTOBJECT	0x12D
#define META_SETTEXTALIGN	0x12E
#define META_CHORD	0x830
#define META_SETMAPPERFLAGS	0x231
#define META_EXTTEXTOUT	0xa32
#define META_SETDIBTODEV	0xd33
#define META_SELECTPALETTE	0x234
#define META_REALIZEPALETTE	0x35
#define META_ANIMATEPALETTE	0x436
#define META_SETPALENTRIES	0x37
#define META_POLYPOLYGON	0x538
#define META_RESIZEPALETTE	0x139
#define META_DIBBITBLT	0x940
#define META_DIBSTRETCHBLT	0xb41
#define META_DIBCREATEPATTERNBRUSH	0x142
#define META_STRETCHDIB	0xf43
#define META_EXTFLOODFILL	0x548
#define META_DELETEOBJECT	0x1f0
#define META_CREATEPALETTE	0xf7
#define META_CREATEPATTERNBRUSH	0x1F9
#define META_CREATEPENINDIRECT	0x2FA
#define META_CREATEFONTINDIRECT	0x2FB
#define META_CREATEBRUSHINDIRECT	0x2FC
#define META_CREATEREGION	0x6FF
#define META_DRAWTEXT	0x062F
#define META_RESETDC	0x014C
#define META_STARTDOC	0x014D
#define META_STARTPAGE	0x004F
#define META_ENDPAGE	0x0050
#define META_ABORTDOC	0x0052
#define META_ENDDOC	0x005E
#define META_CREATEBRUSH	0x00F8
#define META_CREATEBITMAPINDIRECT	0x02FD
#define META_CREATEBITMAP	0x06FE 
#endif


#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x0002
#define TRUETYPE_FONTTYPE   0x0004

#define GGO_METRICS         0
#define GGO_BITMAP          1
#define GGO_NATIVE          2
#define GGO_BEZIER          3
#define GGO_GRAY2_BITMAP    4
#define GGO_GRAY4_BITMAP    5
#define GGO_GRAY8_BITMAP    6
#define GGO_GLYPH_INDEX     0x80
#define GGO_UNHINTED        0x100

/* Rasterizer status */
typedef struct
{
	SHORT nSize;
	SHORT wFlags;
	SHORT nLanguageID;
} RASTERIZER_STATUS, *LPRASTERIZER_STATUS;

#define TT_AVAILABLE        0x0001
#define TT_ENABLED          0x0002

#define TT_PRIM_LINE    1
#define TT_PRIM_QSPLINE 2
#define TT_PRIM_CSPLINE 3
#define TT_POLYGON_TYPE 24

/* TECHNOLOGY */
#define DT_PLOTTER        0
#define DT_RASDISPLAY     1
#define DT_RASPRINTER     2
#define DT_RASCAMERA      3
#define DT_CHARSTREAM     4
#define DT_METAFILE       5
#define DT_DISPFILE       6


  /* ExtTextOut() parameters */
#define ETO_GRAYED          0x0001
#define ETO_OPAQUE          0x0002
#define ETO_CLIPPED         0x0004
#define ETO_GLYPH_INDEX     0x0010
#define ETO_RTLREADING      0x0080
#define ETO_NUMERICSLOCAL   0x0400
#define ETO_NUMERICSLATIN   0x0800
#define ETO_IGNORELANGUAGE  0x1000
#define ETO_PDY             0x2000

  /* Graphics Modes */
#define GM_COMPATIBLE     1
#define GM_ADVANCED       2
#define GM_LAST           2

/* Flags for ModifyWorldTransform */
#define MWT_IDENTITY      1
#define MWT_LEFTMULTIPLY  2
#define MWT_RIGHTMULTIPLY 3
#define MWT_MIN           MWT_IDENTITY
#define MWT_MAX           MWT_RIGHTMULTIPLY

  /* Map modes */
#define MM_TEXT		  1
#define MM_LOMETRIC	  2
#define MM_HIMETRIC	  3
#define MM_LOENGLISH	  4
#define MM_HIENGLISH	  5
#define MM_TWIPS	  6
#define MM_ISOTROPIC	  7
#define MM_ANISOTROPIC	  8

#define MM_MIN            MM_TEXT
#define MM_MAX            MM_ANISOTROPIC
#define MM_MAX_FIXEDSCALE MM_TWIPS

typedef struct tagGCP_RESULTSA {
	DWORD  lStructSize;
	LPSTR  lpOutString;
	UINT   *lpOrder;
	INT    *lpDx;
	INT    *lpCaretPos;
	LPSTR  lpClass;
	LPWSTR lpGlyphs;
	UINT   nGlyphs;
	UINT   nMaxFit;
} GCP_RESULTS, GCP_RESULTSA, *LPGCP_RESULTSA;

typedef struct tagGCP_RESULTSW
{
	DWORD  lStructSize;
	LPWSTR lpOutString;
	UINT   *lpOrder;
	INT    *lpDx;
	INT    *lpCaretPos;
	LPSTR  lpClass;
	LPWSTR lpGlyphs;
	UINT   nGlyphs;
	UINT   nMaxFit;
} GCP_RESULTSW, *LPGCP_RESULTSW;

/* for GetCharacterPlacement () */
#define __MSABI_LONG(x)         x ## l

#define GCP_DBCS          0x0001
#define GCP_REORDER       0x0002
#define GCP_USEKERNING    0x0008
#define GCP_GLYPHSHAPE    0x0010
#define GCP_LIGATE        0x0020
#define GCP_DIACRITIC     0x0100
#define GCP_KASHIDA       0x0200
#define GCP_ERROR         0x8000
#define FLI_MASK          0x103b
#define GCP_JUSTIFY         __MSABI_LONG(0x00010000)
#define FLI_GLYPHS          __MSABI_LONG(0x00040000)
#define GCP_CLASSIN         __MSABI_LONG(0x00080000)
#define GCP_MAXEXTENT       __MSABI_LONG(0x00100000)
#define GCP_JUSTIFYIN       __MSABI_LONG(0x00200000)
#define GCP_DISPLAYZWG      __MSABI_LONG(0x00400000)
#define GCP_SYMSWAPOFF      __MSABI_LONG(0x00800000)
#define GCP_NUMERICOVERRIDE __MSABI_LONG(0x01000000)
#define GCP_NEUTRALOVERRIDE __MSABI_LONG(0x02000000)
#define GCP_NUMERICSLATIN   __MSABI_LONG(0x04000000)
#define GCP_NUMERICSLOCAL   __MSABI_LONG(0x08000000)

#define GCPCLASS_LATIN                     1
#define GCPCLASS_HEBREW                    2
#define GCPCLASS_ARABIC                    3
#define GCPCLASS_NEUTRAL                   4
#define GCPCLASS_LOCALNUMBER               5
#define GCPCLASS_LATINNUMBER               6
#define GCPCLASS_LATINNUMERICTERMINATOR    7
#define GCPCLASS_LATINNUMERICSEPARATOR     8
#define GCPCLASS_NUMERICSEPARATOR          9
#define GCPCLASS_PREBOUNDLTR               0x80
#define GCPCLASS_PREBOUNDRTL               0x40
#define GCPCLASS_POSTBOUNDLTR              0x20
#define GCPCLASS_POSTBOUNDRTL              0x10

#define GCPGLYPH_LINKBEFORE                0x8000
#define GCPGLYPH_LINKAFTER                 0x4000

typedef struct tagPOINTFX
{
	FIXED x;
	FIXED y;
} POINTFX, *LPPOINTFX;



typedef struct tagTTPOLYGONHEADER
{
	DWORD cb;
	DWORD dwType;
	POINTFX pfxStart;
} TTPOLYGONHEADER, *LPTTPOLYGONHEADER;


typedef struct
{
	UINT	gmBlackBoxX;
	UINT	gmBlackBoxY;
	POINT	gmptGlyphOrigin;
	SHORT	gmCellIncX;
	SHORT	gmCellIncY;
} GLYPHMETRICS, *LPGLYPHMETRICS;




typedef struct tagTTPOLYCURVE
{
	WORD wType;
	WORD cpfx;
	POINTFX apfx[1];
} TTPOLYCURVE, *LPTTPOLYCURVE;

typedef struct
{
	LOGFONTA elfLogFont;
	BYTE       elfFullName[LF_FULLFACESIZE];
	BYTE       elfStyle[LF_FACESIZE];
} ENUMLOGFONTA, *LPENUMLOGFONTA;

typedef struct
{
	LOGFONTW elfLogFont;
	WCHAR      elfFullName[LF_FULLFACESIZE];
	WCHAR      elfStyle[LF_FACESIZE];
} ENUMLOGFONTW, *LPENUMLOGFONTW;

typedef struct {
	EMR   emr;
	RECTL rclBounds;
	DWORD cbRgnData;
	DWORD ihBrush;
	SIZEL szlStroke;
	BYTE  RgnData[1];
} EMRFRAMERGN, *PEMRFRAMERGN;

typedef struct {
	EMR   emr;
	DWORD cbData;
	BYTE  Data[1];
} EMRGDICOMMENT, *PEMRGDICOMMENT;

typedef USHORT COLOR16;

typedef struct _TRIVERTEX
{
	LONG    x;
	LONG    y;
	COLOR16 Red;
	COLOR16 Green;
	COLOR16 Blue;
	COLOR16 Alpha;
} TRIVERTEX, *PTRIVERTEX, *LPTRIVERTEX;

typedef struct _GRADIENT_TRIANGLE
{
	ULONG Vertex1;
	ULONG Vertex2;
	ULONG Vertex3;
} GRADIENT_TRIANGLE, *PGRADIENT_TRIANGLE, *LPGRADIENT_TRIANGLE;

typedef struct _GRADIENT_RECT
{
	ULONG UpperLeft;
	ULONG LowerRight;
} GRADIENT_RECT, *PGRADIENT_RECT, *LPGRADIENT_RECT;

BOOL WINAPI GradientFill(
	HDC        hdc,
	PTRIVERTEX pVertex,
	ULONG      nVertex,
	PVOID      pMesh,
	ULONG      nMesh,
	ULONG      ulMode
);
