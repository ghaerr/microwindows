/* winfont.h*/
/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Win32 font structures and API
 */

/* tmPitchAndFamily flags*/
#define TMPF_FIXED_PITCH    0x01	/* win32 bug: means variable*/
#define TMPF_VECTOR         0x02
#define TMPF_TRUETYPE       0x04
#define TMPF_DEVICE         0x08

typedef struct tagTEXTMETRIC {
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BYTE        tmFirstChar;
    BYTE        tmLastChar;
    BYTE        tmDefaultChar;
    BYTE        tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
} TEXTMETRIC, *PTEXTMETRIC, NEAR *NPTEXTMETRIC, FAR *LPTEXTMETRIC;

BOOL WINAPI GetTextMetrics(HDC hdc, LPTEXTMETRIC lptm);
BOOL WINAPI GetCharWidth(HDC hdc,UINT iFirstChar,UINT iLastChar,LPINT lpBuffer);
BOOL WINAPI GetTextExtentPoint(HDC hdc,LPCTSTR lpszStr,int cchString,
		LPSIZE lpSize);
BOOL WINAPI GetTextExtentExPoint(HDC hdc,LPCTSTR lpszStr,int cchString,
		int nMaxExtent,LPINT lpnFit,LPINT alpDx,LPSIZE lpSize);

/* inherit logical font descriptor from engine*/

typedef struct {
	LONG	lfHeight;		/* desired height in pixels*/
	LONG	lfWidth;		/* desired width in pixels or 0*/
	LONG	lfEscapement;		/* rotation in tenths of degree*/
	LONG	lfOrientation;		/* not used*/
	LONG	lfWeight;		/* font weight*/
	BYTE	lfItalic;		/* =1 for italic*/
	BYTE	lfUnderline;		/* =1 for underline*/
	BYTE	lfStrikeOut;		/* not used*/
	BYTE	lfCharSet;		/* font character set*/
	BYTE	lfOutPrecision;		/* font type selection*/
	BYTE	lfClipPrecision;	/* not used*/
	BYTE	lfQuality;		/* not used*/
	BYTE	lfPitchAndFamily;	/* font pitch and family*/
	CHAR	lfFaceName[MWLF_FACESIZE];	/* font name, may be aliased*/
} LOGFONT, *PLOGFONT, NEAR *NPLOGFONT, FAR *LPLOGFONT;

#define LF_FACESIZE		MWLF_FACESIZE

/* font weights*/
#define FW_DONTCARE		MWLF_WEIGHT_DEFAULT
#define FW_THIN			MWLF_WEIGHT_THIN
#define FW_EXTRALIGHT		MWLF_WEIGHT_EXTRALIGHT
#define FW_LIGHT		MWLF_WEIGHT_LIGHT
#define FW_NORMAL		MWLF_WEIGHT_NORMAL
#define FW_MEDIUM		MWLF_WEIGHT_MEDIUM
#define FW_SEMIBOLD		MWLF_WEIGHT_DEMIBOLD
#define FW_BOLD			MWLF_WEIGHT_BOLD
#define FW_EXTRABOLD		MWLF_WEIGHT_EXTRABOLD
#define FW_HEAVY		MWLF_WEIGHT_BLACK
#define FW_ULTRALIGHT		FW_EXTRALIGHT
#define FW_REGULAR		FW_NORMAL
#define FW_DEMIBOLD		FW_SEMIBOLD
#define FW_ULTRABOLD		FW_EXTRABOLD
#define FW_BLACK		FW_HEAVY

/* output precision - used for font selection*/
#define OUT_DEFAULT_PRECIS          MWLF_TYPE_DEFAULT
#define OUT_STRING_PRECIS           1
#define OUT_CHARACTER_PRECIS        2
#define OUT_STROKE_PRECIS           3
#define OUT_TT_PRECIS               MWLF_TYPE_SCALED
#define OUT_DEVICE_PRECIS           MFLF_TYPE_RASTER
#define OUT_RASTER_PRECIS           MFLF_TYPE_RASTER
#define OUT_TT_ONLY_PRECIS          MWLF_TYPE_TRUETYPE
#define OUT_OUTLINE_PRECIS          8
#define OUT_SCREEN_OUTLINE_PRECIS   9

/* clip precision - unused*/
#define CLIP_DEFAULT_PRECIS     0
#define CLIP_CHARACTER_PRECIS   1
#define CLIP_STROKE_PRECIS      2
#define CLIP_MASK               0xf
#define CLIP_LH_ANGLES          (1<<4)
#define CLIP_TT_ALWAYS          (2<<4)
#define CLIP_EMBEDDED           (8<<4)

/* output quality - unused*/
#define DEFAULT_QUALITY         0
#define DRAFT_QUALITY           1
#define PROOF_QUALITY           2
#define NONANTIALIASED_QUALITY  3
#define ANTIALIASED_QUALITY     4

/* font charset*/
#define ANSI_CHARSET            MWLF_CHARSET_ANSI
#define DEFAULT_CHARSET         MWLF_CHARSET_DEFAULT
#define SYMBOL_CHARSET          2
#define OEM_CHARSET             MWLF_CHARSET_OEM

/* font pitch - lfPitchAndFamily*/
#define MWLF_PITCH_DEFAULT	0	/* any pitch*/
#define MWLF_PITCH_FIXED	1	/* fixed pitch*/
#define MWLF_PITCH_VARIABLE	2	/* variable pitch*/

/* font pitch*/
#define DEFAULT_PITCH           MWLF_PITCH_DEFAULT
#define FIXED_PITCH             MWLF_PITCH_FIXED
#define VARIABLE_PITCH          MWLF_PITCH_VARIABLE
#define MONO_FONT               8

/* 
 * font family - lfPitchAndFamily
 *
 * MWF_FAMILY_SERIF	- Times Roman, Century Schoolbook
 * MWF_FAMILY_SANSSERIF	- Helvetica, Swiss
 * MWF_FAMILY_MODERN	- Pica, Elite, Courier
 */
#define MWLF_FAMILY_DEFAULT	(0<<4)	/* any family*/
#define MWLF_FAMILY_SERIF	(1<<4)	/* variable stroke width, serif*/
#define MWLF_FAMILY_SANSSERIF	(2<<4)	/* variable stroke width, sans-serif*/
#define MWLF_FAMILY_MODERN	(3<<4)	/* constant stroke width*/

/* add definitions here for font mapper extensions*/
#define MWLF_FAMILY_BITSTREAM	(6<<4)	/* bitstream*/
#define MWLF_FAMILY_ADOBE	(7<<4)	/* adobe*/

/* font family*/
#define FF_DONTCARE         	MWLF_FAMILY_DEFAULT
#define FF_ROMAN            	MWLF_FAMILY_SERIF
#define FF_SWISS            	MWLF_FAMILY_SANSSERIF
#define FF_MODERN           	MWLF_FAMILY_MODERN
#define FF_SCRIPT           	(4<<4)  /* Cursive, etc. */
#define FF_DECORATIVE       	(5<<4)  /* Old English, etc. */

HFONT WINAPI CreateFont(int nHeight, int nWidth, int nEscapement,
		int nOrientation, int fnWeight, DWORD fdwItalic,
		DWORD fdwUnderline, DWORD fdwStrikeOut,DWORD fdwCharSet,
		DWORD fdwOutputPrecision,DWORD fdwClipPrecision,
		DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR lpszFace);
HFONT WINAPI CreateFontIndirect(CONST LOGFONT *lplf);
