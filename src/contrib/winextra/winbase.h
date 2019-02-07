#pragma once
/* winbase.h*/

//#include "guiddef.h"

typedef LONG_PTR SSIZE_T, *PSSIZE_T;
typedef ULONG_PTR SIZE_T, *PSIZE_T;


typedef struct
{
	WORD    fract;
	SHORT   value;
} FIXED;


/* for GetCharABCWidths() */
typedef struct
{
	INT   abcA;
	UINT  abcB;
	INT   abcC;
} ABC, *PABC, *LPABC;

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




typedef struct
{
	FIXED  eM11;
	FIXED  eM12;
	FIXED  eM21;
	FIXED  eM22;
} MAT2, *LPMAT2;


typedef LONG FXPT16DOT16, *LPFXPT16DOT16;
typedef LONG FXPT2DOT30, *LPFXPT2DOT30;

typedef struct tagCIEXYZ
{
	FXPT2DOT30 ciexyzX;
	FXPT2DOT30 ciexyzY;
	FXPT2DOT30 ciexyzZ;
} CIEXYZ, *LPCIEXYZ;

typedef struct tagCIEXYZTRIPLE
{
	CIEXYZ ciexyzRed;
	CIEXYZ ciexyzGreen;
	CIEXYZ ciexyzBlue;
} CIEXYZTRIPLE, *LPCIEXYZTRIPLE;

typedef struct {
	DWORD        bV5Size;
	LONG         bV5Width;
	LONG         bV5Height;
	WORD         bV5Planes;
	WORD         bV5BitCount;
	DWORD        bV5Compression;
	DWORD        bV5SizeImage;
	LONG         bV5XPelsPerMeter;
	LONG         bV5YPelsPerMeter;
	DWORD        bV5ClrUsed;
	DWORD        bV5ClrImportant;
	DWORD        bV5RedMask;
	DWORD        bV5GreenMask;
	DWORD        bV5BlueMask;
	DWORD        bV5AlphaMask;
	DWORD        bV5CSType;
	CIEXYZTRIPLE bV5Endpoints;
	DWORD        bV5GammaRed;
	DWORD        bV5GammaGreen;
	DWORD        bV5GammaBlue;
	DWORD        bV5Intent;
	DWORD        bV5ProfileData;
	DWORD        bV5ProfileSize;
	DWORD        bV5Reserved;
} BITMAPV5HEADER, *LPBITMAPV5HEADER, *PBITMAPV5HEADER;


#if _MSC_VER
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
#else
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
#endif

typedef union _LARGE_INTEGER {
	struct {
#ifdef WORDS_BIGENDIAN
		LONG     HighPart;
		DWORD    LowPart;
#else
		DWORD    LowPart;
		LONG     HighPart;
#endif
	} u;
#ifndef NONAMELESSSTRUCT
	struct {
#ifdef WORDS_BIGENDIAN
		LONG     HighPart;
		DWORD    LowPart;
#else
		DWORD    LowPart;
		LONG     HighPart;
#endif
	};
#endif
	LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

/* Device parameters for GetDeviceCaps() */
#define DRIVERVERSION     0
#define TECHNOLOGY        2
#define HORZSIZE          4
#define VERTSIZE          6
#define HORZRES           8
#define VERTRES           10
#define BITSPIXEL         12
#define PLANES            14
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
#define LOGPIXELSX        88
#define LOGPIXELSY        90
#define CAPS1             94
#define SIZEPALETTE       104
#define NUMRESERVED       106
#define COLORRES          108

#define PHYSICALWIDTH     110
#define PHYSICALHEIGHT    111
#define PHYSICALOFFSETX   112
#define PHYSICALOFFSETY   113
#define SCALINGFACTORX    114
#define SCALINGFACTORY    115
#define VREFRESH          116
#define DESKTOPVERTRES    117
#define DESKTOPHORZRES    118
#define BLTALIGNMENT      119
#define SHADEBLENDCAPS    120
#define COLORMGMTCAPS     121

/* TECHNOLOGY */
#define DT_PLOTTER        0
#define DT_RASDISPLAY     1
#define DT_RASPRINTER     2
#define DT_RASCAMERA      3
#define DT_CHARSTREAM     4
#define DT_METAFILE       5
#define DT_DISPFILE       6

/* SHADEBLENDCAPS */
#define SB_NONE           0x0000
#define SB_CONST_ALPHA    0x0001
#define SB_PIXEL_ALPHA    0x0002
#define SB_PREMULT_ALPHA  0x0004
#define SB_GRAD_RECT      0x0010
#define SB_GRAD_TRI       0x0020

#define AC_SRC_OVER  0x00
#define AC_SRC_ALPHA 0x01

#define PS_ENDCAP_ROUND  0x00000000
#define PS_ENDCAP_SQUARE 0x00000100
#define PS_ENDCAP_FLAT   0x00000200
#define PS_ENDCAP_MASK   0x00000f00

#define PS_JOIN_ROUND    0x00000000
#define PS_JOIN_BEVEL    0x00001000
#define PS_JOIN_MITER    0x00002000
#define PS_JOIN_MASK     0x0000f000

#define PS_COSMETIC      0x00000000
#define PS_GEOMETRIC     0x00010000
#define PS_TYPE_MASK     0x000f0000

  /* Polygon modes */
#define ALTERNATE         1
#define WINDING           2
#define POLYFILL_LAST     2

	/* Graphics Modes */
#define GM_COMPATIBLE     1
#define GM_ADVANCED       2
#define GM_LAST           2

	  /* Map modes */
#define MM_TEXT		  1
#define MM_LOMETRIC	  2
#define MM_HIMETRIC	  3
#define MM_LOENGLISH	  4
#define MM_HIENGLISH	  5
#define MM_TWIPS	  6
#define MM_ISOTROPIC	  7
#define MM_ANISOTROPIC	  8

	  typedef struct {
		  DWORD 	bV4Size;
		  LONG  	bV4Width;
		  LONG  	bV4Height;
		  WORD 	bV4Planes;
		  WORD 	bV4BitCount;
		  DWORD 	bV4Compression;
		  DWORD 	bV4SizeImage;
		  LONG  	bV4XPelsPerMeter;
		  LONG  	bV4YPelsPerMeter;
		  DWORD 	bV4ClrUsed;
		  DWORD 	bV4ClrImportant;
		  DWORD	bV4RedMask;
		  DWORD	bV4GreenMask;
		  DWORD	bV4BlueMask;
		  DWORD	bV4AlphaMask;
		  DWORD	bV4CSType;
		  CIEXYZTRIPLE	bV4Endpoints;
		  DWORD	bV4GammaRed;
		  DWORD	bV4GammaGreen;
		  DWORD	bV4GammaBlue;
	  } BITMAPV4HEADER, *PBITMAPV4HEADER;

#define GENERIC_READ               0x80000000
#define GENERIC_WRITE              0x40000000
#define GENERIC_EXECUTE            0x20000000
#define GENERIC_ALL                0x10000000

#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define TRUNCATE_EXISTING       5

#define INVALID_HANDLE_VALUE     ((HANDLE)~(ULONG_PTR)0)
#define INVALID_FILE_SIZE        (~0u)
#define INVALID_SET_FILE_POINTER (~0u)
#define INVALID_FILE_ATTRIBUTES  (~0u)

#define	PAGE_NOACCESS		0x01
#define	PAGE_READONLY		0x02
#define	PAGE_READWRITE		0x04
#define	PAGE_WRITECOPY		0x08
#define	PAGE_EXECUTE		0x10
#define	PAGE_EXECUTE_READ	0x20
#define	PAGE_EXECUTE_READWRITE	0x40
#define	PAGE_EXECUTE_WRITECOPY	0x80
#define	PAGE_GUARD		0x100
#define	PAGE_NOCACHE		0x200
#define	PAGE_WRITECOMBINE	0x400

#define FILE_MAP_COPY                   0x00000001
#define FILE_MAP_WRITE                  0x00000002
#define FILE_MAP_READ                   0x00000004
#define FILE_MAP_ALL_ACCESS             0x000f001f
#define FILE_MAP_EXECUTE                0x00000020

	  /* types of LoadImage */
#define IMAGE_BITMAP	0
#define IMAGE_ICON	1
#define IMAGE_CURSOR	2
#define IMAGE_ENHMETAFILE	3


/* loadflags to LoadImage */
#define LR_DEFAULTCOLOR		0x0000
#define LR_MONOCHROME		0x0001
#define LR_COLOR		0x0002
#define LR_COPYRETURNORG	0x0004
#define LR_COPYDELETEORG	0x0008
#define LR_LOADFROMFILE		0x0010
#define LR_LOADTRANSPARENT	0x0020
#define LR_DEFAULTSIZE		0x0040
#define LR_VGA_COLOR		0x0080
#define LR_LOADMAP3DCOLORS	0x1000
#define	LR_CREATEDIBSECTION	0x2000
#define LR_COPYFROMRESOURCE	0x4000
#define LR_SHARED		0x8000
