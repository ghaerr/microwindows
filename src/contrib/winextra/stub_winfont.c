/* stub_winfont.c*/
#include "windows.h"
#include "winextra.h"

DWORD WINAPI GetGlyphOutlineA(HDC            hdc, UINT           uChar, UINT           fuFormat, LPGLYPHMETRICS lpgm, DWORD          cjBuffer, LPVOID         pvBuffer, const MAT2     *lpmat2)
{
	printf("GetGlyphOutlineA IMPLEMENT ME");
	return 0;
}

DWORD WINAPI GetGlyphOutlineW(HDC            hdc, UINT           uChar, UINT           fuFormat, LPGLYPHMETRICS lpgm, DWORD          cjBuffer, LPVOID         pvBuffer, const MAT2     *lpmat2)
{
	printf("GetGlyphOutlineW IMPLEMENT ME");
	return 0;
}

UINT WINAPI GetOutlineTextMetricsA(HDC                  hdc, UINT                 cjCopy, LPOUTLINETEXTMETRICA potm) {
	printf("GetOutlineTextMetricsA IMPLEMENT ME");
	return 0;
}


DWORD WINAPI GetFontUnicodeRanges(HDC        hdc, LPGLYPHSET lpgs) {
	printf("GetFontUnicodeRanges IMPLEMENT ME");
	return 0;
}

DWORD WINAPI GetFontData(HDC   hdc, DWORD dwTable, DWORD dwOffset, PVOID pvBuffer, DWORD cjBuffer) {
	printf("GetFontData IMPLEMENT ME"); 
	return 0;
}

DWORD WINAPI GetGlyphIndicesA(HDC    hdc, LPCSTR lpstr, int    c, LPWORD pgi, DWORD  fl) {
	printf("GetGlyphIndicesA IMPLEMENT ME");
	return 0;
}

DWORD WINAPI GetGlyphIndicesW(HDC     hdc, LPCWSTR lpstr, int     c, LPWORD  pgi, DWORD   fl) {
	printf("GetGlyphIndicesW IMPLEMENT ME");
	return 0;
}

static int
strlen16(unsigned short *ptr)
{
	int len = 0;

	while (*ptr++)
		len++;
	return len;
}
 
HFONT WINAPI
CreateFontIndirectW(CONST LOGFONTW *lplf)
{
	LOGFONT lf;
	char buffer[256];
	extern int uc16_to_utf8(const unsigned short *us, int cc, unsigned char *s);

	lf.lfCharSet = lplf->lfCharSet;
	lf.lfClipPrecision = lplf->lfClipPrecision;
	lf.lfEscapement = lplf->lfEscapement;
	uc16_to_utf8(lplf->lfFaceName, strlen16((unsigned short *)lplf->lfFaceName), (unsigned char *)buffer);
	strncpy(lf.lfFaceName, buffer, 32);
	lf.lfHeight = lplf->lfHeight;
	lf.lfItalic = lplf->lfItalic;
	lf.lfOrientation = lplf->lfOrientation;
	lf.lfOutPrecision = lplf->lfOutPrecision;
	lf.lfPitchAndFamily = lplf->lfPitchAndFamily;
	lf.lfQuality = lplf->lfQuality;
	lf.lfStrikeOut = lplf->lfStrikeOut;
	lf.lfUnderline = lplf->lfUnderline;
	lf.lfWeight = lplf->lfWeight;
	lf.lfWidth = lplf->lfWidth;

	return CreateFontIndirect(&lf);
}

 
BOOL WINAPI
GetTextMetricsW(HDC hdc, LPTEXTMETRICW  lptm)
{
	printf("GetTextMetricsW IMPLEMENT ME");
	return 0;
}

BOOL WINAPI
GetTextMetricsA(HDC hdc, LPTEXTMETRIC lptm)
{
	return GetTextMetrics(hdc,lptm);
}

BOOL WINAPI
GetCharWidth32A(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer) {
	return GetCharWidth(hdc,  iFirstChar,  iLastChar,  lpBuffer);
}

