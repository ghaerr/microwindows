/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * GetTextExtent*Point by Roman Guseynov
 * Original contributions by Shane Nay
 *
 * Win32 API upper level font selection routines
 */
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include <stdlib.h>
#include <string.h>

HFONT WINAPI
CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation,
	int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut,
	DWORD fdwCharSet,DWORD fdwOutputPrecision,DWORD fdwClipPrecision,
	DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR lpszFace)
{
	LOGFONT	lf;

	lf.lfHeight = nHeight;
	lf.lfWidth = nWidth;
	lf.lfEscapement = nEscapement;
	lf.lfOrientation = nOrientation;
	lf.lfWeight = fnWeight;
	lf.lfItalic = fdwItalic;
	lf.lfUnderline = fdwUnderline;
	lf.lfStrikeOut = fdwStrikeOut;
	lf.lfCharSet = fdwCharSet;
	lf.lfOutPrecision = fdwOutputPrecision;
	lf.lfClipPrecision = fdwClipPrecision;
	lf.lfQuality = fdwQuality;
	lf.lfPitchAndFamily = fdwPitchAndFamily;
	strncpy(lf.lfFaceName, lpszFace, sizeof(lf.lfFaceName));

	return CreateFontIndirect(&lf);
}

HFONT WINAPI
CreateFontIndirect(CONST LOGFONT *lplf)
{
	MWFONTOBJ * 	hfont;
	int		family, pitch;
	MWLOGFONT	mwlf;
	char	szFacename[32];

	/* create a gdi font object*/
	hfont = GdItemNew(MWFONTOBJ);
	if(!hfont)
		return NULL;
	hfont->hdr.type = OBJ_FONT;
	hfont->hdr.stockobj = FALSE;

	/* convert LOGFONT to MWLOGFONT*/
	memset(&mwlf, 0, sizeof(mwlf));
	mwlf.lfHeight = lplf->lfHeight;
	mwlf.lfWidth = lplf->lfWidth;
	mwlf.lfEscapement = lplf->lfEscapement;
	mwlf.lfOrientation = lplf->lfOrientation;
	mwlf.lfWeight = lplf->lfWeight;
	mwlf.lfItalic = lplf->lfItalic;
	mwlf.lfUnderline = lplf->lfUnderline;
	mwlf.lfStrikeOut = lplf->lfStrikeOut;
	mwlf.lfCharSet = lplf->lfCharSet;
	mwlf.lfOutPrecision = lplf->lfOutPrecision;
	mwlf.lfClipPrecision = lplf->lfClipPrecision;
	mwlf.lfQuality = lplf->lfQuality;
	strncpy(mwlf.lfFaceName, lplf->lfFaceName, sizeof(mwlf.lfFaceName));

	family = lplf->lfPitchAndFamily & 0xf0;
	switch(family) {
	case FF_DONTCARE:
		break;
	case FF_ROMAN:
		mwlf.lfRoman = 1;
		mwlf.lfSerif = 1;
		break;
	case FF_SWISS:
		mwlf.lfSansSerif = 1;
		break;
	case FF_MODERN:
		mwlf.lfModern = 1;
		break;
	}

	pitch = lplf->lfPitchAndFamily & 0x0f;
	switch(pitch) {
	case DEFAULT_PITCH:
		break;
	case FIXED_PITCH:
	case MONO_FONT:
		mwlf.lfMonospace = 1;
		break;
	case VARIABLE_PITCH:
		mwlf.lfProportional = 1;
		break;
	}
	/*mwlf.lfOblique = 0;*/
	/*mwlf.lfSmallCaps = 0;*/
	/*mwlf.lfPitch = 0;*/

	/* select a font based on facename, bold/italic and height*/
	strncpy(szFacename, lplf->lfFaceName, sizeof(szFacename));
	if (lplf->lfWeight==FW_BOLD)
		strcat(szFacename, "B");
	if (lplf->lfItalic)
		strcat(szFacename, "I");
	hfont->pfont = GdCreateFont(&scrdev, szFacename, lplf->lfHeight, &mwlf);

	return (HFONT)hfont;
}

BOOL WINAPI
GetTextMetrics(HDC hdc, LPTEXTMETRIC lptm)
{
	MWFONTINFO 	fi;

	if(!hdc)
		return FALSE;

	GdGetFontInfo(hdc->font->pfont, &fi);

	/* FIXME many items are guessed for the time being*/
	lptm->tmHeight = fi.height;

   	 /* reversed for kaffe port
	lptm->tmAscent = fi.height - fi.baseline;
	lptm->tmDescent= fi.baseline;
	 */

	lptm->tmDescent = fi.height - fi.baseline;
	lptm->tmAscent= fi.baseline;
	lptm->tmInternalLeading = 0;
	lptm->tmExternalLeading = 0;
	lptm->tmAveCharWidth = fi.widths['x'];
	lptm->tmMaxCharWidth = fi.maxwidth;
	lptm->tmWeight = FW_NORMAL;
	lptm->tmOverhang = 0;
	lptm->tmDigitizedAspectX = fi.maxwidth;
	lptm->tmDigitizedAspectY = fi.height;
	lptm->tmFirstChar = 32;
	lptm->tmLastChar = 255;
	lptm->tmDefaultChar = '?';
	lptm->tmBreakChar = 0;
	lptm->tmItalic = 0;
	lptm->tmUnderlined = 0;
	lptm->tmStruckOut = 0;
	/* note that win32 has the TMPF_FIXED_PITCH flags REVERSED...*/
	lptm->tmPitchAndFamily = fi.fixed?
			FF_DONTCARE: (FF_DONTCARE | TMPF_FIXED_PITCH);
	lptm->tmCharSet = OEM_CHARSET;
	return TRUE;
}

BOOL WINAPI
GetCharWidth(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer)
{
	int 		i;
	int		j = 0;
	MWFONTINFO	fi;

	if(!hdc || iLastChar < iFirstChar)
		return FALSE;

	GdGetFontInfo(hdc->font->pfont, &fi);
	for(i=iFirstChar; i <= iLastChar; ++i)
		if(i < fi.firstchar || i > fi.lastchar || i > 255)
			lpBuffer[j++] = 0;
		else lpBuffer[j++] = fi.widths[i];
		lpBuffer[j++] = fi.widths[i];

	return TRUE;
}

BOOL WINAPI
GetTextExtentPoint(
	HDC hdc,		/* handle to DC*/
	LPCTSTR lpszStr,	/* character string*/
	int cchString,		/* number of characters*/
	LPSIZE lpSize)		/* string dimensions*/
{
	int width = 1, height = 1, baseline = 0;

	if (lpSize) {
		lpSize->cx = 0;
		lpSize->cy = 0;
	}
	if (!hdc || !lpszStr || !cchString || !lpSize)
		return FALSE;
	GdGetTextSize(hdc->font->pfont, lpszStr, cchString, &width, &height,
		&baseline, MWTF_UTF8);
	lpSize->cx = width;
	lpSize->cy = height;

	/*printf("<MWIN>: lpszStr=\"%s\", cchString=%d, lpsize->cx=%d, lpSize->cy=%d\n", lpszStr, cchString, lpSize->cx, lpSize->cy);*/
	return TRUE;
}

BOOL WINAPI
GetTextExtentExPoint(HDC hdc,	/* handle to DC*/
	  LPCTSTR lpszStr,	/* character string*/
	  int cchString,	/* number of characters*/
	  int nMaxExtent,	/* maximum width of formatted string*/
	  LPINT lpnFit,		/* maximum number of characters*/
	  LPINT alpDx,	 	/* array of partial string widths*/
	  LPSIZE lpSize)	/* string dimensions*/

{
	int attr,width=0,height=0;

	if(!hdc || !lpszStr)
		return FALSE;
	if (cchString<0)
		cchString = strlen((char *)lpszStr);
	attr=hdc->font->pfont->fontattr;
	if (attr&FS_FREETYPE)
	{ 
		if (GdGetTextSizeEx(hdc->font->pfont,lpszStr,cchString,
			nMaxExtent,lpnFit,alpDx,&width,&height,NULL,MWTF_UTF8))
		{
			lpSize->cx=width;
			lpSize->cy=height;
			return TRUE;
		}
		return FALSE;
	}
	else
	{
		SIZE sz;
		int i;

		if (!GetTextExtentPoint(hdc, lpszStr, cchString, lpSize))
			return FALSE;
		if ((!nMaxExtent)||(!lpnFit)||(!alpDx))
			return TRUE;
		for (i=0; i<cchString; i++) {
			if (!GetTextExtentPoint(hdc, lpszStr, i+1, &sz))
				return FALSE;
			if (sz.cx <= nMaxExtent)
				alpDx[i] = sz.cx;
			else {
				(*lpnFit) = i;
				return TRUE;
			}
		}
		(*lpnFit) = cchString;
		return TRUE;	
	}
}     
