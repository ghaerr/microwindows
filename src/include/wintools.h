/*
 * wintools.h - MS Windows tools library header
 *
 * WINGEN	General Purpose Library
 * WINMEM	Windows Memory Mgmt Library
 * WINFX	Special Effects Library
 * WINDRAW	Draw Library
 * WINCTL	Custom Control Library
 * WIN Class Procedures: ToolsTips, AutoMove, AutoEdit
 * WINVID	Windows Digital Video
 *
 * original version 1/6/95 by g haerr
 */

void WINAPI	Draw3dShadow(HDC hDC,int x,int y,int w,int h,COLORREF crTop,
			COLORREF crBottom);
void WINAPI 	Draw3dBox(HDC hDC,int x,int y,int w,int h,COLORREF crTop,
			COLORREF crBottom);
void WINAPI	Draw3dInset(HDC hDC,int x,int y,int w,int h);
void WINAPI	Draw3dOutset(HDC hDC,int x,int y,int w,int h);
void WINAPI	Draw3dPushDown(HDC hDC, int x, int y, int w, int h);
void WINAPI	Draw3dUpDownState(HDC hDC, int x, int y, int w, int h,
			BOOL fDown);
void WINAPI	Draw3dUpFrame(HDC hDC, int l, int t, int r, int b);
void WINAPI	FastFillRect(HDC hdc,LPRECT lprect,COLORREF cr);
void WINAPI	InsetR(LPRECT lprc,int h,int v);
BOOL WINAPI	PtInsideWindow(HWND hwnd,UINT x,UINT y);
