/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Demo program for Microwindows
 */
#define MWINCLUDECOLORS
#include "windows.h"
#include "wintern.h"		/* for MwSetDesktopWallpaper*/

#include "device.h"

#if DOS_TURBOC
unsigned _stklen = 4096;
#endif

#define CLIPDEMO	0	/* set for region clipping demo*/

#ifndef ELKS
#define TIMERDEMO	1	/* set for WM_TIMER demo*/
#define GRAPH3D		0	/* 3d graphics demo*/
#define IMAGE		0	/* 256 color image demo*/
#endif
#define ARCDEMO		1	/* arc drawing demo*/
#define CHILD 		1	/* child window demo*/
#define CLIENT3D	0	/* old client draw test*/

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

#if CHILD
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

int
RegisterAppClass(void)
{
	WNDCLASS	wc;

#if !ELKS
	MwRegisterButtonControl(NULL);
	MwRegisterEditControl(NULL);
	MwRegisterListboxControl(NULL);
	MwRegisterProgressBarControl(NULL);
	/*MwRegisterComboboxControl(NULL);*/
#endif
	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0; /*LoadIcon(GetHInstance(), MAKEINTRESOURCE( 1));*/
	wc.hCursor = 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName =  APPCLASS;
	RegisterClass( &wc);

#if CHILD
	wc.lpfnWndProc = (WNDPROC)ChildWndProc;
	wc.lpszClassName =  APPCHILD;
	return RegisterClass( &wc);
#endif
	return 1;
}

HWND
CreateAppWindow(void)
{
	HWND	hwnd, hlist;
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
		NULL, (HMENU)nextid++, NULL, NULL);

#if CHILD
	if(hwnd
#if GRAPH3D
			&& (nextid & 03)!=2
#endif
								) {
		CreateWindowEx(0L, APPCHILD,
			"",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			4, 4, width / 3, height / 3,
			hwnd, (HMENU)2, NULL, NULL);
return hwnd;
		CreateWindowEx(0L, APPCHILD,
			"",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			width / 3, height / 3, width / 3, height / 3,
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

		hlist = CreateWindowEx(0L, "LISTBOX",
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

#if CHILD
LRESULT CALLBACK
ChildWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	HDC		hdcMem;
	HBITMAP		hbmp, hbmpOrg;
	RECT		rc;
	PAINTSTRUCT	ps;

	switch(msg) {
	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);

		/* redirect painting to offscreen dc*/
		hdcMem = CreateCompatibleDC(ps.hdc);
		//hbmp = CreateCompatibleBitmap(hdcMem, rc.right, rc.bottom);
		hbmp = CreateCompatibleBitmap(hdcMem, image->width, image->height);
		hbmpOrg = SelectObject(hdcMem, hbmp);

		/* draw onto offscreen dc*/
		DrawDIB(hdcMem, 0, 0, image);

		/* blit offscreen with physical screen*/
		//BitBlt(ps.hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, MWROP_SRCCOPY);
		StretchBlt(ps.hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, image->width, image->height, MWROP_SRCCOPY);
		DeleteObject(SelectObject(hdcMem, hbmpOrg));
		DeleteDC(hdcMem);

		EndPaint(hwnd, &ps);
		break;
	default:
		return DefWindowProc( hwnd, msg, wp, lp);
	}
	return( 0);
}
#endif

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
#if TIMERDEMO
	static POINT	mousept;
#endif
#if ARCDEMO
	static int	startdegrees = 0;
	static int	enddegrees = 30;
#endif

	switch( msg) {
#if TIMERDEMO
	case WM_CREATE:
		SetTimer(hwnd, 1, 100, NULL);
		mousept.x = 60;
		mousept.y = 20;
		break;

	case WM_TIMER:
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
		SendMessage(hwnd, WM_MOUSEMOVE, 0,
			MAKELONG(mousept.x, mousept.y));
#endif
#if ARCDEMO
		startdegrees += 10;
		if(startdegrees >= 360)
			startdegrees = 0;
		enddegrees += 15;
		if(enddegrees >= 360)
			enddegrees = 0;
		InvalidateRect(hwnd, NULL, TRUE);
#endif
		break;

	case WM_DESTROY:
		KillTimer(hwnd, 1);
		break;
#endif /* TIMERDEMO*/
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
		if((GetWindowLong(hwnd, GWL_ID) & 03) == 1)
			return 1;
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
#if ARCDEMO
{
	int x, y, w, h;
	RECT rc;

	if(hdc != NULL) {
		GetWindowRect(hwnd, &rc);
		rc.top += 13;
		InflateRect(&rc, -3, -3);
		//Ellipse(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top);
		//Arc(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);
		//Pie(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);
#if 1
		x = rc.left;
		y = rc.top;
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;
		w += 10;
		GdSetForeground(GdFindColor(RGB(0,255,0)));
		GdArcAngle(hdc->psd, x+w/2, y+h/2, w/2, h/2, startdegrees*64,
			enddegrees*64, MWPIE);
		GdSetForeground(GdFindColor(RGB(0,0,0)));
		GdArcAngle(hdc->psd, x+w/2, y+h/2, w/2, h/2, startdegrees*64,
			enddegrees*64, MWARCOUTLINE);
		//GdSetForeground(GdFindColor(RGB(255,255,255)));
		//GdPoint(hdc->psd, x+w/2, y+h/2);
#endif
	}
	EndPaint(hwnd, &ps);
	break;
}
#endif /* ARCDEMO*/
#if GRAPH3D
		id = (int)GetWindowLong(hwnd, GWL_ID) & 03;
		init3(hdc, id == 1? hwnd: NULL);
		switch(id) {
		case 0:
			rose(1.0, 7, 13);
			break;
		case 1:
			//look3(0.5, 0.7, 1.5);
			//look3(0.2, -2 * gy, 1.0+gx);
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
			case 1:
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

#endif /* GRAPH3D*/
		EndPaint(hwnd, &ps);
		break;

	case WM_LBUTTONDOWN:
		break;

	case WM_MOUSEMOVE:
#if GRAPH3D
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

	case WM_LBUTTONUP:
		break;

	case WM_RBUTTONDOWN:
		break;

	default:
		return DefWindowProc( hwnd, msg, wp, lp);
	}
	return( 0);
}

int WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nShowCmd)
{
	MSG 	msg;
	HWND	hwnd;
	RECT	rc;

	RegisterAppClass();
	GetWindowRect(GetDesktopWindow(), &rc);
#if !(ELKS | MSDOS)
	/* create penguin window*/
	//CreateWindowEx(0L, APPCHILD, "", WS_BORDER | WS_VISIBLE,
		//rc.right-130-1, rc.bottom-153-1, 130, 153,
		//GetDesktopWindow(), (HMENU)1000, NULL, NULL);
#endif
	CreateAppWindow();
	CreateAppWindow();
	//CreateAppWindow();
#if !(ELKS | MSDOS)
	//CreateAppWindow();
	//CreateAppWindow();
	//CreateAppWindow();
	//CreateAppWindow();
	//CreateAppWindow();
	//hwnd = CreateAppWindow();
	//GetWindowRect(hwnd, &rc);
	//OffsetRect(&rc, 50, 50);
	//MoveWindow(hwnd, rc.left, rc.top, rc.bottom-rc.top,
		//rc.right-rc.left, TRUE);
#endif
#if !(ELKS | MSDOS)
	/* set background wallpaper*/
	MwSetDesktopWallpaper(&image_microwin);
	/*MwSetDesktopWallpaper(&image_under4);*/
	/*MwSetDesktopWallpaper(&image_car8);*/
#endif

	/* type ESC to quit...*/
	while( GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
