/*
 * Copyright (c) 1999, 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Demo program for Microwindows
 *
 *  GB: 10-14-2004: Modified to store degrees data on each window, 
 *                  for new Timers features.
 */
#include <time.h>
#include <stdlib.h>
#include "uni_std.h"
#define MWINCLUDECOLORS
#include "windows.h"
#include "device.h"

#if DOS_TURBOC
unsigned _stklen = 4096;
#endif

/* define either GRAPH3D demo or CHILD control demo*/
#define GRAPH3D		1	/* 3d graphics demo (requires floating point)*/
#define CONTROLS	0	/* win32 controls demo*/

/* add clipping to GRAPH3D demo*/
#define CLIPDEMO	0	/* set for region clipping demo*/

/* CONTROLS demo options*/
#define ARCDEMO		1	/* add arc drawing to CONTROLS demo*/
#define USEBLIT		1	/* use blit rather than DrawDIB()*/
#define IMAGE		0	/* add 256 color image to CONTROLS demo*/

/* test timer system with either demo*/
#define TIMERDEMO	1	/* set for WM_TIMER demo*/

/* not used*/
#define CLIENT3D	0	/* old client draw test*/

#if RTEMS
#undef GRAPH3D
#undef CONTROLS
#define CONTROLS	1	/* win32 controls demo*/
#undef  IMAGE
#define IMAGE		1	/* 256 color image demo*/
#undef  USEBLIT
#define USEBLIT		0	/* use blit rather than DrawDIB()*/
#endif

#if GRAPH3D
#include "graph3d.h"
#endif

extern MWIMAGEHDR image_car8;
extern MWIMAGEHDR image_zion208;
extern MWIMAGEHDR image_penguin;
extern MWIMAGEHDR image_under4;
extern MWIMAGEHDR image_microwin;
extern MWIMAGEHDR image_cs1;
extern MWIMAGEHDR image_rle8;

#if CONTROLS
#if ELKS | MSDOS
PMWIMAGEHDR image = &image_cs1;		/* 2 color bitmap for 16 color systems*/
#else
PMWIMAGEHDR image = &image_penguin;
#endif
#endif

#if IMAGE
PMWIMAGEHDR image2 = &image_zion208;
#endif

#define APPCLASS	"test"
#define APPCHILD	"test2"

/* forward decls*/
LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wp,LPARAM lp);
LRESULT CALLBACK ChildWndProc(HWND hwnd,UINT uMsg,WPARAM wp,LPARAM lp);

int
MwUserInit(int ac, char **av)
{
	/* test user init procedure - do nothing*/
	return 0;
}

static int
RegisterAppClass(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	/* register builtin controls and dialog classes*/
	MwInitializeDialogs(hInstance);

	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(LONG_PTR);
	wc.hInstance = 0;
	wc.hIcon = 0; /*LoadIcon(GetHInstance(), MAKEINTRESOURCE( 1));*/
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName =  APPCLASS;
	RegisterClass( &wc);

#if CONTROLS
	wc.lpfnWndProc = (WNDPROC)ChildWndProc;
	wc.lpszClassName =  APPCHILD;
	return RegisterClass( &wc);
#endif
	return 1;
}

static HWND
CreateAppWindow(void)
{
	HWND	hwnd;
	static int nextid = 1;
	int width, height;
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);
	width = height = r.right / 2;

	hwnd = CreateWindowEx(0L, APPCLASS,
		"Microwindows Application",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL, (HMENU)(LONG_PTR)nextid++, NULL, NULL);

#if CONTROLS
	if(hwnd
#if GRAPH3D
			&& (nextid & 03)!=2
#endif
								) {
		CreateWindowEx(0L, APPCHILD,
			"",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			4, 4, width / 3-6, height / 3,
			hwnd, (HMENU)2, NULL, NULL);
		CreateWindowEx(0L, APPCHILD,
			"",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			width / 3, height / 3, width / 3-6, height / 3,
			hwnd, (HMENU)3, NULL, NULL);
		CreateWindowEx(0L, APPCHILD,
			"",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			width * 3 / 5, height * 3 / 5,
			width * 2 / 3, height * 2 / 3,
			hwnd, (HMENU)4, NULL, NULL);
		CreateWindowEx(0L, "EDIT",
			"OK",
			WS_BORDER|WS_CHILD | WS_VISIBLE,
			width * 5 / 8, 10, 100, 18,
			hwnd, (HMENU)5, NULL, NULL);
		CreateWindowEx(0L, "PROGBAR",
			"OK",
			WS_BORDER|WS_CHILD | WS_VISIBLE,
			width * 5 / 8, 32, 100, 18,
			hwnd, (HMENU)6, NULL, NULL);

		HWND hlist = CreateWindowEx(0L, "LISTBOX",
			"OK",
			WS_HSCROLL|WS_VSCROLL|WS_BORDER|WS_CHILD | WS_VISIBLE,
			width * 5 / 8, 54, 100, 48,
			hwnd, (HMENU)7, NULL, NULL);
		SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Cherry");
		SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Apple");
		SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Orange");

		CreateWindowEx(0L, "BUTTON",
			"Cancel",
			BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
			width * 5 / 8 + 50, 106, 50, 14,
			hwnd, (HMENU)8, NULL, NULL);
	}
#endif
	return hwnd;
}

#if CONTROLS
LRESULT CALLBACK
ChildWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	RECT		rc;
	PAINTSTRUCT	ps;

	switch(msg) {
	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);
#if USEBLIT
{
		HDC		hdcMem;
		HBITMAP		hbmp, hbmpOrg;

		/* redirect painting to offscreen dc, then use blit function*/
		hdcMem = CreateCompatibleDC(ps.hdc);
		/*
		 * Note: rc.right, rc.bottom happens to be smaller than image
		 * width/height.  We use the image size, so we can stretchblit
		 * from the whole image.
		 */
		hbmp = CreateCompatibleBitmap(hdcMem, image->width, image->height);
		hbmpOrg = SelectObject(hdcMem, hbmp);

		/* draw onto offscreen dc*/
		DrawDIB(hdcMem, 0, 0, image);

		/* blit offscreen with physical screen*/
		//BitBlt(ps.hdc, 0, 0, rc.right*4/5, rc.bottom*4/5, hdcMem,
			//0, 0, MWROP_COPY);
		StretchBlt(ps.hdc, 0, 0, rc.right*4/5, rc.bottom*4/5, hdcMem,
			0, 0, image->width, image->height, MWROP_COPY);
		DeleteObject(SelectObject(hdcMem, hbmpOrg));
		DeleteDC(hdcMem);
}
#else
		DrawDIB(ps.hdc, rc.left, rc.top, image);
#endif
		EndPaint(hwnd, &ps);
		break;
	default:
		return DefWindowProc( hwnd, msg, wp, lp);
	}
	return( 0);
}
#endif

typedef struct {
	int	startdegrees;
	int	enddegrees;
	unsigned long reserved;
} demoWndData, *pdemoWndData;

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT	ps;
	HDC		hdc;
#if CLIENT3D | IMAGE | GRAPH3D
	RECT		rc;
#endif
#if GRAPH3D
	static int	countup = 1;
	int		id;
	static vec1 	gx, gy;
#endif
	static POINT	mousept;
	pdemoWndData pData;

	switch( msg) {
	case WM_CREATE:
		pData = (pdemoWndData) malloc(sizeof(demoWndData));
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)pData);
		mousept.x = 60;
		mousept.y = 20;
		pData->startdegrees = 0;
		pData->enddegrees = 30;
#if TIMERDEMO
		SetTimer(hwnd, 1, (50+random()%250), NULL);
#endif
		break;

	case WM_TIMER:
		pData = (pdemoWndData)(LONG_PTR)GetWindowLongPtr(hwnd, 0);
#if GRAPH3D
		GetClientRect(hwnd, &rc);
		if(countup) {
			mousept.y += 20;
			if(mousept.y >= rc.bottom) {
				mousept.y -= 20;
				countup = 0;
			}
		} else {
			mousept.y -= 20;
			if(mousept.y < 20) {
				mousept.y += 20;
				countup = 1;
			}
		}
		SendMessage(hwnd, WM_MOUSEMOVE, 0, MAKELONG(mousept.x, mousept.y));
#elif ARCDEMO
		pData->startdegrees += 10;
		if(pData->startdegrees >= 360)
			pData->startdegrees = 0;
		pData->enddegrees += 15;
		if(pData->enddegrees >= 360)
			pData->enddegrees = 0;
		InvalidateRect(hwnd, NULL, TRUE);
#endif
		break;
	case WM_DESTROY:
		KillTimer(hwnd, 1);
		pData = (pdemoWndData)(LONG_PTR)GetWindowLongPtr(hwnd, 0);
		free ( pData );
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)0);
		break;
	case WM_SIZE:
		break;
	case WM_MOVE:
		break;

#if CLIENT3D
	case WM_SETFOCUS:
		PostMessage((HWND)wp, WM_PAINT, 0, 0L);
		break;
	case WM_KILLFOCUS:
		PostMessage((HWND)wp, WM_PAINT, 0, 0L);
		break;
	case WM_ERASEBKGND:
		if(GetFocus() != hwnd)
			return DefWindowProc(hwnd, msg, wp, lp);
		return 1;
#endif
#if GRAPH3D
	case WM_ERASEBKGND:
		if((GetWindowLong(hwnd, GWL_ID) & 03) == 1) {
			return 1;
		}
		return DefWindowProc(hwnd, msg, wp, lp);
#endif
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);

#if CLIENT3D
		if(GetFocus() == hwnd) {
			GetClientRect(hwnd, &rc);
			Draw3dShadow(hdc, rc.left, rc.top,
				rc.right-rc.left, rc.bottom-rc.top,
				GetSysColor(COLOR_3DDKSHADOW),
				GetSysColor(COLOR_3DLIGHT));
			InflateRect(&rc, -1, -1);
			FillRect(hdc, &rc, GetStockObject(GRAY_BRUSH));
		}
#endif
#if IMAGE
		GetClientRect(hwnd, &rc);
		DrawDIB(hdc, rc.left+2, rc.top+2, image2);
#endif
#if GRAPH3D
		id = (int)GetWindowLong(hwnd, GWL_ID) & 03;
		init3(hdc, id == 1? hwnd: NULL);
		switch(id) {
		case 0:
			rose(1.0, 7, 13);
			break;
		case 1:
			/*look3(0.5, 0.7, 1.5);*/
			/*look3(0.2, -2 * gy, 1.0+gx);*/
			look3(-2 * gx, -2 * gy, 1.2);
			drawgrid(-8.0, 8.0, 10, -8.0, 8.0, 10);
			break;
		case 2:
			setcolor3(BLACK);
			circle3(1.0);
			break;
		case 3:
			setcolor3(BLUE);
			daisy(1.0, 20);
			break;
		}
#if CLIPDEMO
		if(id == 1) {
			HRGN	hrgn, hrgn2;

			/* create circular clip region for effect*/
			GetClientRect(hwnd, &rc);
			InflateRect(&rc, -80, -80);
			switch((int)GetWindowLong(hwnd, GWL_ID)) {
			default:
				hrgn = CreateEllipticRgnIndirect(&rc);
				break;
			case 5:
				hrgn = CreateRoundRectRgn(rc.left, rc.top,
					rc.right, rc.bottom, 100, 100);
				break;
			case 2:
				hrgn = CreateRectRgnIndirect(&rc);
				break;
			}

			/* erase background, clip out blit area*/
			GetClientRect(hwnd, &rc);
			hrgn2 = CreateRectRgnIndirect(&rc);
			SelectClipRgn(hdc, hrgn2);
			ExtSelectClipRgn(hdc, hrgn, RGN_XOR);
			DeleteObject(hrgn2);

			GetClientRect(hwnd, &rc);
			FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));

			/* clip in only blit area*/
			SelectClipRgn(hdc, hrgn);
			DeleteObject(hrgn);
		}
#endif /* CLIPDEMO*/
		paint3(hdc);
#elif ARCDEMO
{
	int x, y, w, h;
	RECT rc;

	if(hdc != NULL) {
		pData = (pdemoWndData) GetWindowLongPtr(hwnd, 0);
		GetWindowRect(hwnd, &rc);
		rc.top += 13;
		InflateRect(&rc, -3, -3);
		/*Ellipse(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top);*/
		/*Arc(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);*/
		/*Pie(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);*/

		x = rc.left;
		y = rc.top;
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;
		w += 10;
		GdSetForegroundColor(hdc->psd, RGB(0,255,0));
		GdArcAngle(hdc->psd, x+w/2, y+h/2, w/2, h/2, pData->startdegrees*64,
			pData->enddegrees*64, MWPIE);
		GdSetForegroundColor(hdc->psd, RGB(0,0,0));
		GdArcAngle(hdc->psd, x+w/2, y+h/2, w/2, h/2, pData->startdegrees*64,
			pData->enddegrees*64, MWARCOUTLINE);
		/*GdSetForegroundColor(hdc->psd, RGB(255,255,255)));*/
		/*GdPoint(hdc->psd, x+w/2, y+h/2);*/
	}
	EndPaint(hwnd, &ps);
	break;
}
#endif /* ARCDEMO*/
		EndPaint(hwnd, &ps);
		break;

	case WM_MOUSEMOVE:
#if GRAPH3D
		pData = (pdemoWndData)(LONG_PTR)GetWindowLongPtr(hwnd, 0);
		if((GetWindowLong(hwnd, GWL_ID) & 03) == 1) {
			POINT pt;

			POINTSTOPOINT(pt, lp);
			GetClientRect(hwnd, &rc);
			gx = (vec1)pt.x / rc.right;
			gy = (vec1)pt.y / rc.bottom;
			InvalidateRect(hwnd, NULL, FALSE);
			mousept.x = pt.x;
			mousept.y = pt.y;
		}
#endif
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
		break;
	case WM_RBUTTONDOWN:
		break;
	default:
		return DefWindowProc( hwnd, msg, wp, lp);
	}
	return 0;
}

#if EMSCRIPTEN && MULTIAPP
#define WinMain	mwdemo_WinMain
#endif

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	MSG 	msg;
	HWND	hwnd;
	RECT	rc;

	srandom(time(NULL));
	RegisterAppClass(hInstance);
	GetWindowRect(GetDesktopWindow(), &rc);

	/* create penguin window*/
	CreateWindowEx(0L, APPCHILD, "", WS_BORDER | WS_VISIBLE,
		rc.right-130-1, rc.bottom-153-1, 130, 153,
		GetDesktopWindow(), (HMENU)1000, NULL, NULL);

#if CONTROLS
	CreateAppWindow();
	CreateAppWindow();
	CreateAppWindow();
	CreateAppWindow();
	CreateAppWindow();
	CreateAppWindow();
	CreateAppWindow();
	CreateAppWindow();
#endif
#if !MULTIAPP
	/* set background wallpaper*/
	MwSetDesktopWallpaper(&image_microwin);
	/*MwSetDesktopWallpaper(&image_under4);*/
	/*MwSetDesktopWallpaper(&image_car8);*/
#endif

	hwnd = CreateAppWindow();
	GetWindowRect(hwnd, &rc);
	OffsetRect(&rc, 50, 50);
	MoveWindow(hwnd, rc.left, rc.top, rc.bottom-rc.top,
		rc.right-rc.left, TRUE);

#if !MULTIAPP
	while( GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
	return 0;
}
