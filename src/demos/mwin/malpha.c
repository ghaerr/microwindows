/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Microwindows alpha blending demo
 *
 * This demo requires UPDATEREGIONS=N in microwin/src/config
 */
#include "windows.h"
#include "wintern.h"		/* for MwSetDesktopWallpaper*/
#include "wintools.h"

extern MWIMAGEHDR image_car8;

#define APPCLASS	"test"

/* forward decls*/
LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wp,LPARAM lp);
LRESULT CALLBACK ChildWndProc(HWND hwnd,UINT uMsg,WPARAM wp,LPARAM lp);

int
RegisterAppClass(void)
{
	WNDCLASS	wc;

	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0; /*LoadIcon(GetHInstance(), MAKEINTRESOURCE( 1));*/
	wc.hCursor = 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName =  APPCLASS;
	RegisterClass( &wc);

	return 1;
}

HWND
CreateAppWindow(void)
{
	HWND	hwnd;
	int 	width, height;
	RECT 	r;
	static int nextid = 0;

	GetWindowRect(GetDesktopWindow(), &r);
	width = height = r.right / 2;

	hwnd = CreateWindowEx(WS_EX_LAYERED, APPCLASS,
		"Microwindows Alpha Blending",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL, (HMENU)++nextid, NULL, NULL);

	return hwnd;
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT	ps;
	HWND		sibwp;
	HDC		hdcMem;
	HBITMAP		hbmp, hbmpOrg;
	HBRUSH		hbr;
	RECT		rc;
	extern int mwpaintSerial;
   
	switch(msg) {
	case WM_ERASEBKGND:
		/* don't erase with screen dc, must alpha blend bkgnd*/
		return 1;
	
	case WM_PAINT:
		/* NOTE: this routine needs to be generalized
		 * for arbitrary deep child relationships and
		 * moved into the Microwindows kernel.  In addition,
		 * the lower window repainting needs to occur
		 * offscreen and alpha blended offscreen with
		 * a final blit to the device screen.
		 */
		/* force NC painting - current NC regions don't work
		 * with this alpha blend algorithm
		 */
		mwforceNCpaint = TRUE;

		/* repaint lower windows before alpha blending this window*/
		++hwnd->unmapcount;	/* tricky don't clip this window*/
		SendMessage(rootwp, WM_PAINT, 0, 0);
		for(sibwp=hwnd->siblings; sibwp; sibwp=sibwp->siblings)
			SendMessage(sibwp, WM_PAINT, 0, 0);
		--hwnd->unmapcount;

		/* then queue repaint for higher windows*/
		for(sibwp=hwnd->parent->children; sibwp != hwnd;
							sibwp=sibwp->siblings)
			/* don't paint if already painted by above code*/
			if(sibwp->paintSerial != mwpaintSerial)
				PostMessage(sibwp, WM_PAINT, 0, 0);

		/* now paint this window offscreen and blend with screen*/
		BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);

		/* redirect painting to offscreen dc*/
		hdcMem = CreateCompatibleDC(ps.hdc);
		hbmp = CreateCompatibleBitmap(hdcMem, rc.right, rc.bottom);
		hbmpOrg = SelectObject(hdcMem, hbmp);

		/* paint window to offscreen*/
		hbr = (HBRUSH)GetClassLong(hwnd, GCL_HBRBACKGROUND);
		FillRect(hdcMem, &rc, hbr);
		SelectObject(hdcMem, GetStockObject(DEFAULT_GUI_FONT));
		SetBkMode(hdcMem, TRANSPARENT);
#define TEXTSTRING	"This demonstrates alpha blending"
		TextOut(hdcMem, 0, 20, TEXTSTRING, strlen(TEXTSTRING));

		/* alpha blend blit offscreen map with physical screen*/
		BitBlt(ps.hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0,
			MWROP_BLENDCONSTANT | 150);
		DeleteObject(SelectObject(hdcMem, hbmpOrg));
		DeleteDC(hdcMem);

		EndPaint(hwnd, &ps);
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
	MSG msg;

	/* Force XORMOVE window redraw algorithm, required
	 * for this version of alpha blend painting
	 */
	mwERASEMOVE = FALSE;

	RegisterAppClass();

	/* set background wallpaper*/
	MwSetDesktopWallpaper(&image_car8);

	/* must update root window until alpha blend blitting
	 * uses off screen memory for hidden windows, rather than
	 * screen memory*/
	UpdateWindow(GetDesktopWindow());

	CreateAppWindow();
	CreateAppWindow();

	/* type ESC to quit...*/
	while( GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
