/*
 * Copyright (c) 2001 Greg Haerr <greg@censoft.com>
 *
 * Demo program for StretchBlt
 */
#define MWINCLUDECOLORS
#include "windows.h"
#include "device.h"

extern MWIMAGEHDR image_penguin;

PMWIMAGEHDR image = &image_penguin;

#define APPCHILD	"test2"

/* forward decls*/
LRESULT CALLBACK ChildWndProc(HWND hwnd,UINT uMsg,WPARAM wp,LPARAM lp);

int
RegisterAppClass(void)
{
	WNDCLASS	wc;

	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)ChildWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0; /*LoadIcon(GetHInstance(), MAKEINTRESOURCE( 1));*/
	wc.hCursor = 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName =  APPCHILD;
	return RegisterClass( &wc);
}

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
		hbmp = CreateCompatibleBitmap(hdcMem, image->width, image->height);
		hbmpOrg = SelectObject(hdcMem, hbmp);

		/* draw onto offscreen dc*/
		DrawDIB(hdcMem, 0, 0, image);

		/* stretch blit offscreen with physical screen*/
		StretchBlt(ps.hdc, 0, 0, rc.right, rc.bottom, hdcMem,
			0, 0, image->width, image->height, MWROP_SRCCOPY);
		DeleteObject(SelectObject(hdcMem, hbmpOrg));
		DeleteDC(hdcMem);
		EndPaint(hwnd, &ps);
		break;
	default:
		return DefWindowProc( hwnd, msg, wp, lp);
	}
	return 0;
}

int WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nShowCmd)
{
	MSG 	msg;
	RECT	rc;

	RegisterAppClass();
	GetWindowRect(GetDesktopWindow(), &rc);

	/* create penguin windows*/
	CreateWindowEx(0L, APPCHILD, "", WS_BORDER | WS_VISIBLE,
		10, 10, 50, 50,
		GetDesktopWindow(), (HMENU)1000, NULL, NULL);

	CreateWindowEx(0L, APPCHILD, "", WS_BORDER | WS_VISIBLE,
		10, 70, 200, 200,
		GetDesktopWindow(), (HMENU)1001, NULL, NULL);

	/* type ESC to quit...*/
	while( GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
