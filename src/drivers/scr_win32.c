/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010, 2019 Greg Haerr <greg@censoft.com>
 *
 * Microsoft Windows screen driver for Microwindows
 *	Tested in NONETWORK mode only
 *	Needs updating in update_from_savevbits for new driver format
 *
 * by Wilson Loi
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include "..\include\device.h"
#include "..\include\genfont.h"
#include "fb.h"
#include "genmem.h"

/* specific driver entry points*/
static PSD win32_open(PSD psd);
static void win32_close(PSD psd);
static void win32_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void win32_getscreeninfo(PSD psd, PMWSCREENINFO psi);
static void win32_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
static int win32_preselect(PSD psd);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	win32_open,
	win32_close,
	win32_setpalette,       
	win32_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	win32_update,
	win32_preselect
};

HWND mwAppWindow = NULL;
static HDC dcBuffer = NULL;
static HBITMAP dcBitmap = NULL;
static HBITMAP dcOldBitmap;
static BITMAPV4HEADER bmpInfo;
static MWCOORD upminX, upminY, upmaxX, upmaxY;	/* win32_preselect and win32_update*/

static void win32_draw(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
static int win32_pollevents(void);

LRESULT CALLBACK
mwinWindowProc(HWND hWnd,	UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HDC dc;
	PAINTSTRUCT ps;

	switch (Msg) {
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP: 
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		GdError("BAD got message %x\n", Msg);
		break;
	case WM_SETCURSOR:
	{
		WORD hit = LOWORD(lParam);
		static int hiddencursor = 0;

		if (hit == HTCLIENT && !hiddencursor)
		{
			hiddencursor = 1;
			ShowCursor(FALSE);
		}
		else if (hit != HTCLIENT && hiddencursor)
		{
			hiddencursor = 0;
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			ShowCursor(TRUE);
		}
	}
		break;
	case WM_PAINT:
		dc = BeginPaint(hWnd, &ps);
		BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top, 
			ps.rcPaint.right - ps.rcPaint.left,
			ps.rcPaint.bottom - ps.rcPaint.top, 
			dcBuffer, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
		//GdError("paint %d,%d %d,%d\n", ps.rcPaint.left, ps.rcPaint.top, 
		//ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		SelectObject(dcBuffer, dcOldBitmap);
		DeleteDC(dcBuffer);	
		DeleteObject(dcBitmap);
		exit(0);			// exit Microwindows
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;	
}
 
/* init windows stuff for win32 screen driver*/
static int
win32_init(PSD psd)
{
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
	HDC hdc;
	WNDCLASS wc;

	wc.style           = CS_HREDRAW | CS_VREDRAW; // | CS_OWNDC;
	wc.lpfnWndProc     = (WNDPROC)mwinWindowProc;
	wc.cbClsExtra      = 0;
	wc.cbWndExtra      = 0;
	wc.hInstance       = hInstance;
	wc.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground   = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName    = NULL;
	wc.lpszClassName   = "mwinAppClass";
	RegisterClass(&wc);

	/* add non-client area to width and height of window*/
	mwAppWindow = CreateWindow("mwinAppClass", TEXT("Microwindows"),
			WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
			0, 0, SCREEN_WIDTH+6, SCREEN_HEIGHT+25, 0, 0, hInstance, NULL);
	if (!mwAppWindow)
		return 0;

	/* create offscreen buffer for paint update*/
	hdc = GetDC(mwAppWindow);
	dcBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
	dcBuffer = CreateCompatibleDC(hdc);
	dcOldBitmap = SelectObject(dcBuffer, dcBitmap);
	ReleaseDC(mwAppWindow, hdc);

	memset(&bmpInfo, 0, sizeof(bmpInfo));
	bmpInfo.bV4Size = sizeof(bmpInfo);
	bmpInfo.bV4Width = psd->xres;
	bmpInfo.bV4Height = -psd->yres; /* top-down image */
	bmpInfo.bV4Planes = 1;
	bmpInfo.bV4BitCount = psd->bpp;
	bmpInfo.bV4XPelsPerMeter = 3078;
	bmpInfo.bV4YPelsPerMeter = 3078;
	bmpInfo.bV4ClrUsed = 0;
	bmpInfo.bV4ClrImportant = 0;   
	bmpInfo.bV4CSType = LCS_sRGB;
	bmpInfo.bV4V4Compression = BI_BITFIELDS;

#if MWPIXEL_FORMAT == MWPF_TRUECOLORARGB
	bmpInfo.bV4AlphaMask = 0xff000000;
	bmpInfo.bV4RedMask  = 0xff0000;
	bmpInfo.bV4GreenMask= 0x00ff00;
	bmpInfo.bV4BlueMask = 0x0000ff;
#endif
#if MWPIXEL_FORMAT == MWPF_TRUECOLORABGR
	bmpInfo.bV4AlphaMask = 0xff000000;
	bmpInfo.bV4RedMask = 0x0000ff;
	bmpInfo.bV4GreenMask= 0x00ff00;
	bmpInfo.bV4BlueMask  = 0xff0000;
#endif
#if MWPIXEL_FORMAT == MWPF_TRUECOLORRGB
	bmpInfo.bV4V4Compression = BI_RGB;
#endif
#if MWPIXEL_FORMAT == MWPF_TRUECOLOR565
	bmpInfo.bV4RedMask  = 0xf800u;
	bmpInfo.bV4GreenMask= 0x07e0u;
	bmpInfo.bV4BlueMask = 0x001fu;
#endif
#if MWPIXEL_FORMAT == MWPF_TRUECOLOR555
	bmpInfo.bV4RedMask  = 0x7c00u;
	bmpInfo.bV4GreenMask= 0x03e0u;
	bmpInfo.bV4BlueMask = 0x001fu;
#endif
	ShowWindow(mwAppWindow, SW_SHOW);

	return 1;
}

/* open screen driver*/
static PSD
win32_open(PSD psd)
{
	/* init psd and allocate framebuffer*/
	int flags = PSF_SCREEN | PSF_ADDRMALLOC | PSF_DELAYUPDATE;

	if (!gen_initpsd(psd, MWPIXEL_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, flags))
		return NULL;

	if (!win32_init(psd))
		return NULL;

	return &scrdev;
}

/* return nonzero if event available*/
static int
win32_pollevents(void)
{
	MSG msg;

	if (PeekMessage(&msg, mwAppWindow, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE))
		return 1;

	if (PeekMessage(&msg, mwAppWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE))
		return 1;

	if (PeekMessage(&msg, mwAppWindow, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

static void
win32_close(PSD psd)
{
	DestroyWindow(mwAppWindow);
	free(psd->addr);
}

static void
win32_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
}

static void
win32_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);
}


/* update window from Microwindows framebuffer*/
static void
win32_draw(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	RECT rc;
	unsigned char *pixels;
//GdError("update %d,%d %d,%d\n", x, y, width, height);

	/* calc offset to framebuffer and buffer size*/
	pixels = psd->addr + y * psd->pitch + x * (psd->bpp >> 3);
	bmpInfo.bV4SizeImage = width * height * (psd->bpp >> 3);

#if MWPIXEL_FORMAT == MWPF_TRUECOLORRGB
	{
		/* convert RGB to BGR*/
		unsigned char *bits;
		if ((bits = (unsigned char *)_alloca(bmpInfo.bV4SizeImage)) != NULL) {
			for (i=0; i<bmpInfo.bV4SizeImage; i+=3) {
				bits[i] = pixels[i+2];
				bits[i+1] = pixels[i+1];
				bits[i+2] = pixels[i];
			}
			pixels = bits;
		}
	}
#endif

	/* copy framebuffer into win32 image buffer*/
	SetDIBitsToDevice(dcBuffer, x, y, width, height, 0, 0, 0, height,
		pixels, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS);

	/* force a paint update*/
	rc.left = x;
	rc.top = y;
	rc.right = x+width;
	rc.bottom = y+height;
	InvalidateRect(mwAppWindow, &rc, FALSE);
}

/* called before select(), returns # pending events*/
static int
win32_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region to SDL server*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX >= 0 || upmaxY >= 0)) {
		win32_draw(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);

		/* reset update region*/
		upminX = upminY = MAX_MWCOORD;
		upmaxX = upmaxY = MIN_MWCOORD;
	}

	/* return nonzero if SDL event available*/
	return win32_pollevents();
}

static void
win32_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = MWMIN(x, upminX);
			upminY = MWMIN(y, upminY);
			upmaxX = MWMAX(upmaxX, x+width-1);
			upmaxY = MWMAX(upmaxY, y+height-1);
	} else
		win32_draw(psd, x, y, width, height);
}
