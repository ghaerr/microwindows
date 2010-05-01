/*
 * Copyright (c) 2000, 2001, 2003, 2005 Greg Haerr <greg@censoft.com>
 *
 * Microsoft Windows screen driver for Microwindows
 *	Tested in NONETWORK mode only
 *
 * by Wilson Loi
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <wingdi.h>
#include <assert.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#define APP_NAME "Microwindows"

/* SCREEN_WIDTH, SCREEN_HEIGHT and MWPIXEL_FORMAT define window size*/
#ifndef SCREEN_WIDTH
#error SCREEN_WIDTH not defined
#endif

#ifndef SCREEN_HEIGHT
#error SCREEN_HEIGHT not defined
#endif

#ifndef MWPIXEL_FORMAT
#error MWPIXEL_FORMAT not defined
#endif

/* SCREEN_DEPTH is used only for palette modes*/
#if !defined(SCREEN_DEPTH) && (MWPIXEL_FORMAT == MWPF_PALETTE)
#error SCREEN_DEPTH not defined - must be set for palette modes
#endif

/* externally set override values from nanox/srvmain.c*/
MWCOORD	nxres;			/* requested server x res*/
MWCOORD	nyres;			/* requested server y res*/

/* specific driver entry points*/
static PSD win32_open(PSD psd);
static void win32_close(PSD psd);
static void win32_getscreeninfo(PSD psd, PMWSCREENINFO psi);
static void win32_setpalette(PSD psd, int first, int count, MWPALENTRY * pal);
static void win32_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL win32_readpixel(PSD psd, MWCOORD x, MWCOORD y);
static void win32_drawhline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y,
		MWPIXELVAL c);
static void win32_drawvline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2,
		MWPIXELVAL c);
static void win32_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2,
		MWCOORD y2, MWPIXELVAL c);
static void win32_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w,
		MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op);
static void win32_drawarea(PSD psd, driver_gc_t * gc, int op);
static void win32_stretchblitex(PSD dstpsd, PSD srcpsd, MWCOORD dest_x_start,
		MWCOORD dest_y_start, MWCOORD width, MWCOORD height, int x_denominator,
		int y_denominator, int src_x_fraction, int src_y_fraction,
		int x_step_fraction, int y_step_fraction, long op);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	win32_open,
	win32_close,
	win32_getscreeninfo,
	win32_setpalette,
	win32_drawpixel,
	win32_readpixel,
	win32_drawhline,
	win32_drawvline,
	win32_fillrect,
	gen_fonts,
	win32_blit,
	NULL,
	NULL,
	NULL,			/* SetIOPermissions */
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,			/* SetPortrait */
	0,			/* int portrait */
	NULL,			/* orgsubdriver */
	NULL,			/* StretchBlitEx subdriver*/
};

HWND winRootWindow = NULL;
static HDC dcBuffer = NULL;
static HBITMAP dcBitmap = NULL;
static HBITMAP dcOldBitmap;
static HANDLE dummyEvent = NULL;

LRESULT
myWindowProc(HWND hWnd,	UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL ret;
	HDC dc;
	PAINTSTRUCT ps;

	switch (Msg) {
	case WM_CREATE:
		break;
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
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
		break;
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		break;
	case WM_PAINT:
		dc = BeginPaint(hWnd, &ps);
		ret = BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top, 
			ps.rcPaint.right - ps.rcPaint.left,
			ps.rcPaint.bottom - ps.rcPaint.top, 
			dcBuffer, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
		assert(ret!=FALSE);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		SelectObject(dcBuffer, dcOldBitmap);
		DeleteDC(dcBuffer);	
		DeleteObject(dcBitmap);
		winRootWindow = NULL;
		CloseHandle(dummyEvent);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;	
}
 
/* called from keyboard/mouse/screen */
static PSD
win32_open(PSD psd)
{
	HANDLE hInstance = GetModuleHandle(NULL);
	HDC rootDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	int depth = GetDeviceCaps(rootDC, BITSPIXEL);
	int size;
	RECT rect;

	DeleteDC(rootDC);
	GetWindowRect(GetDesktopWindow(), &rect);
	psd->xvirtres = rect.right - rect.left;
	psd->yvirtres = rect.bottom - rect.top;
	if (psd->xvirtres > SCREEN_WIDTH)
		psd->xvirtres = SCREEN_WIDTH;
	if (psd->yvirtres > SCREEN_HEIGHT)
		psd->yvirtres = SCREEN_HEIGHT;
	psd->linelen = psd->xres = psd->xvirtres;
	psd->yres = psd->yvirtres;
	psd->planes = 1;
	psd->pixtype = MWPIXEL_FORMAT;
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR0888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)
	psd->bpp = 32;
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR888)
	psd->bpp = 24;
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
	psd->bpp = 16;
#else
#error "No support bpp < 16"
#endif 
	/* Calculate the correct linelen here */
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes,
			 psd->bpp, &size, &psd->linelen);

	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
	psd->size = 0;
	psd->addr = NULL;
	psd->portrait = MWPORTRAIT_NONE;
	{
		WNDCLASS wc;
				
		wc.style           = CS_HREDRAW | CS_VREDRAW; // | CS_OWNDC;
		wc.lpfnWndProc     = (WNDPROC)myWindowProc;
		wc.cbClsExtra      = 0;
		wc.cbWndExtra      = 0;
		wc.hInstance       = hInstance;
		wc.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground   = GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName    = NULL;
		wc.lpszClassName   = APP_NAME;
		RegisterClass(&wc);
	}

	winRootWindow = CreateWindow(APP_NAME, "", WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, 0, 0, 
			SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, hInstance, NULL);
	if (winRootWindow) {
		HDC dc = GetDC(winRootWindow);

		GetClientRect(winRootWindow, &rect);
		dcBitmap = CreateCompatibleBitmap(dc, rect.right-rect.left,rect.bottom-rect.top);   
		dcBuffer = CreateCompatibleDC(dc);
		dcOldBitmap = SelectObject(dcBuffer, dcBitmap);
		ReleaseDC(winRootWindow, dc);
		dummyEvent = CreateEvent(NULL, TRUE, FALSE, "");
		ShowWindow(winRootWindow, SW_SHOW);
		UpdateWindow(winRootWindow);
	}
	return &scrdev;
}

static void
win32_close(PSD psd)
{
	if (winRootWindow)
		SendMessage(winRootWindow, WM_DESTROY, 0, 0);
}


static void
win32_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->portrait = psd->portrait;
	psi->fonts = NUMBER_FONTS;
	psi->xdpcm = 0;
	psi->ydpcm = 0;

	psi->fbdriver = FALSE;	/* not running fb driver, no direct map */
	psi->pixtype = psd->pixtype;
	switch (psd->pixtype) {
	case MWPF_TRUECOLOR0888:
	case MWPF_TRUECOLOR8888:
	case MWPF_TRUECOLOR888:
		psi->rmask = 0xff0000;
		psi->gmask = 0x00ff00;
		psi->bmask = 0x0000ff;
		break;
	case MWPF_TRUECOLORABGR:
		psi->rmask = 0x0000ff;
		psi->gmask = 0x00ff00;
		psi->bmask = 0xff0000;
	case MWPF_TRUECOLOR565:
		psi->rmask = 0xf800;
		psi->gmask = 0x07e0;
		psi->bmask = 0x001f;
		break;
	case MWPF_TRUECOLOR555:
		psi->rmask = 0x7c00;
		psi->gmask = 0x03e0;
		psi->bmask = 0x001f;
		break;
	case MWPF_TRUECOLOR332:
		psi->rmask = 0xe0;
		psi->gmask = 0x1c;
		psi->bmask = 0x03;
		break;
	case MWPF_PALETTE:
	default:
		psi->rmask = 0xff;
		psi->gmask = 0xff;
		psi->bmask = 0xff;
		break;
	}
}

static void
win32_setpalette(PSD psd, int first, int count, MWPALENTRY * pal)
{
}


static void
win32_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL pixel)
{
	COLORREF c = PIXELVALTOCOLORVAL(pixel);
	HDC dc = GetDC(winRootWindow);

	SetPixel(dc, x, y, c);
	ReleaseDC(winRootWindow, dc);
	SetPixel(dcBuffer, x, y, c);
}

static MWPIXELVAL
win32_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	COLORREF ret = GetPixel(dcBuffer, x, y);

	return COLORVALTOPIXELVAL(ret);
}

static void
drawLine(HDC dc, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, COLORREF c)
{
	POINT p;
	HPEN pen = CreatePen(PS_SOLID, 1, c); 

	if (pen) {
		HGDIOBJ old = SelectObject(dc, pen);

		MoveToEx(dc, x1, y1, &p);
		LineTo(dc, x2, y2);
		SelectObject(dc, old);
		DeleteObject(pen);
	}
}

static void
win32_drawhline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL pixel)
{
	COLORREF c = PIXELVALTOCOLORVAL(pixel);
	HDC dc = GetDC(winRootWindow);

	drawLine(dc, x1, y, x2, y, c);
	ReleaseDC(winRootWindow, dc);
	drawLine(dcBuffer, x1, y, x2, y, c);
}

static void
win32_drawvline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL pixel)
{
	COLORREF c = PIXELVALTOCOLORVAL(pixel);
	HDC dc = GetDC(winRootWindow);

	drawLine(dc, x, y1, x, y2, c);
	ReleaseDC(winRootWindow, dc);
	drawLine(dcBuffer, x, y1, x, y2, c);
}

static void
win32_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL pixel)
{
	RECT rect;
	HDC dc;
	HBRUSH hbr = CreateSolidBrush(PIXELVALTOCOLORVAL(pixel));

	if (hbr) {
		rect.bottom = y2;
		rect.top = y1;
		rect.left = x1;
		rect.right = x2;
		dc = GetDC(winRootWindow);
		FillRect(dc, &rect, hbr);
		ReleaseDC(winRootWindow, dc);
		FillRect(dcBuffer, &rect, hbr);
		DeleteObject(hbr);
	}
}

static void
win32_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	 PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	unsigned char *addr = srcpsd->addr;
	HDC dc;

	if (op == MWMODE_NOOP)
		return;

	if (!(dstpsd->flags & PSF_SCREEN)) {
		/* memory to memory blit, use offscreen blitter */
		dstpsd->Blit(dstpsd, destx, desty, w, h,
			srcpsd, srcx, srcy, op);
		return;
	}
 
	dc = GetDC(winRootWindow);
	if (srcpsd->flags & PSF_MEMORY) {
            unsigned int i;
            unsigned char *bits = NULL;
            BITMAPV4HEADER bmpInfo;

            memset(&bmpInfo, 0, sizeof(bmpInfo));
            bmpInfo.bV4Size = sizeof(bmpInfo);
            bmpInfo.bV4Width = srcpsd->xres;
            bmpInfo.bV4Height = -srcpsd->yres; /* top-down image */
            bmpInfo.bV4Planes = 1;
            bmpInfo.bV4BitCount = srcpsd->bpp;
            bmpInfo.bV4SizeImage = srcpsd->size;
        
        switch (srcpsd->pixtype) {
        case MWPF_TRUECOLOR565:
            bmpInfo.bV4RedMask  = 0xf800u;
            bmpInfo.bV4GreenMask= 0x07e0u;
            bmpInfo.bV4BlueMask = 0x001fu;
            bmpInfo.bV4V4Compression = BI_BITFIELDS;
            break;
        case MWPF_TRUECOLOR555:
            bmpInfo.bV4RedMask  = 0x7c00u;
            bmpInfo.bV4GreenMask= 0x03e0u;
            bmpInfo.bV4BlueMask = 0x001fu;
            bmpInfo.bV4V4Compression = BI_BITFIELDS;
            break;
        case MWPF_TRUECOLOR888:
            if ((bits = (unsigned char *)malloc(bmpInfo.bV4SizeImage)) != NULL) {
                for (i=0; i<bmpInfo.bV4SizeImage; i+=3) {
                    bits[i] = addr[i+2];
                    bits[i+1] = addr[i+1];
                    bits[i+2] = addr[i];
                }
                addr = bits;
            }
            bmpInfo.bV4V4Compression = BI_RGB;
            break;
        case MWPF_TRUECOLORABGR:
            bmpInfo.bV4AlphaMask = 0xff000000;
            bmpInfo.bV4RedMask = 0x0000ff;
            bmpInfo.bV4GreenMask= 0x00ff00;
            bmpInfo.bV4BlueMask  = 0xff0000;
            bmpInfo.bV4V4Compression = BI_BITFIELDS;
			break;
        case MWPF_TRUECOLOR8888:
            bmpInfo.bV4AlphaMask = 0xff000000;
        case MWPF_TRUECOLOR0888:
        default:
            bmpInfo.bV4RedMask  = 0xff0000;
            bmpInfo.bV4GreenMask= 0x00ff00;
            bmpInfo.bV4BlueMask = 0x0000ff;
            bmpInfo.bV4V4Compression = BI_BITFIELDS;
            break;
        }
        
        bmpInfo.bV4XPelsPerMeter = 3078;
        bmpInfo.bV4YPelsPerMeter = 3078;
        bmpInfo.bV4ClrUsed = 0;
        bmpInfo.bV4ClrImportant = 0;   
        bmpInfo.bV4CSType = LCS_sRGB;
        srcy = srcpsd->yres - h - srcy;
#if 1
        SetDIBitsToDevice(dc, destx, desty, w, h, srcx, srcy, 0, srcpsd->yres,
		addr, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS);
        SetDIBitsToDevice(dcBuffer, destx, desty, w, h, srcx, srcy, 0, srcpsd->yres,
		addr, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS);
#else
        StretchDIBits(dc, destx, desty, w, h, srcx, srcy, w, h,
		addr, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
        StretchDIBits(dcBuffer, destx, desty, w, h, srcx, srcy, w, h,
		addr, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
#endif 
        free(bits);
	} else if (srcpsd->flags & PSF_SCREEN) {
		/* Use offscreen equivalent as the source */
		BitBlt(dc, destx, desty, w, h, dcBuffer, srcx, srcy, SRCCOPY);
		BitBlt(dcBuffer, destx, desty, w, h, dcBuffer, srcx, srcy, SRCCOPY);
	}
	ReleaseDC(winRootWindow, dc);
}
