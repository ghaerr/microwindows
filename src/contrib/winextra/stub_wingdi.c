/* stub_wingdi.c*/
#include "windows.h"
#include "winextra.h"

BOOL WINAPI RestoreDC(
	HDC hdc,
	int nSavedDC
) {
	printf("RestoreDC IMPLEMENT ME");
	return FALSE;
}
int WINAPI SaveDC(
	HDC hdc
) {
	printf("SaveDC IMPLEMENT ME");
	return FALSE;
}

BOOL WINAPI GdiFlush() {
	printf("GdiFlush IMPLEMENT ME");
	return FALSE;
}

BOOL WINAPI GradientFill(
	HDC        hdc,
	PTRIVERTEX pVertex,
	ULONG      nVertex,
	PVOID      pMesh,
	ULONG      nMesh,
	ULONG      ulMode
) {
	printf("GradientFill IMPLEMENT ME");
	return FALSE;
}

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
) {
	printf("StretchDIBits IMPLEMENT ME");
	return 0;
}

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
) {
	printf("TransparentBlt IMPLEMENT ME");
	return 0;
}
BOOL WINAPI SetViewportOrgEx(
	HDC     hdc,
	int     x,
	int     y,
	LPPOINT lppt
) {
	printf("SetViewportOrgEx IMPLEMENT ME");
	return FALSE;

}
