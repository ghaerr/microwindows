/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
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
#define SCREEN_WIDTH	800
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT	600
#endif

#ifndef MWPIXEL_FORMAT
#define MWPIXEL_FORMAT	MWPF_TRUECOLOR8888
#endif

/* externally set override values from nanox/srvmain.c*/
MWCOORD	nxres;			/* requested server x res*/
MWCOORD	nyres;			/* requested server y res*/

/* specific driver entry points*/
static PSD win32_open(PSD psd);
static void win32_close(PSD psd);
static void win32_getscreeninfo(PSD psd, PMWSCREENINFO psi);
static void win32_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	gen_fonts,
	win32_open,
	win32_close,
	NULL,				/* SetPalette*/
	win32_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,				/* SetPortrait */
	win32_update,
	NULL				/* PreSelect*/
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
	PSUBDRIVER subdriver;
	WNDCLASS wc;

	DeleteDC(rootDC);
	GetWindowRect(GetDesktopWindow(), &rect);
	psd->xvirtres = rect.right - rect.left;
	psd->yvirtres = rect.bottom - rect.top;
	if (psd->xvirtres > SCREEN_WIDTH)
		psd->xvirtres = SCREEN_WIDTH;
	if (psd->yvirtres > SCREEN_HEIGHT)
		psd->yvirtres = SCREEN_HEIGHT;
	psd->xres = psd->xvirtres;
	psd->yres = psd->yvirtres;
	psd->planes = 1;
	psd->pixtype = MWPIXEL_FORMAT;
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)
	psd->bpp = 32;
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR888)
	psd->bpp = 24;
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
	psd->bpp = 16;
#else
#error "No support bpp < 16"
#endif 
	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* Calculate the correct linelen here */
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes,
			 psd->bpp, &size, &psd->linelen, &psd->pitch);
	if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;
	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
	psd->portrait = MWPORTRAIT_NONE;
printf("win32 emulated bpp %d\n", psd->bpp);

	/* select an fb subdriver matching our planes and bpp for backing store*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver)
		return NULL;

	/* set and initialize subdriver into screen driver*/
	set_subdriver(psd, subdriver, TRUE);

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
	gen_getscreeninfo(psd, psi);
	psi->fbdriver = FALSE;	/* not running fb driver, no direct map */
}


#if 0000
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

	if (op == MWROP_NOOP)
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
#endif

static void
update_from_savebits(PSD psd, int destx, int desty, int w, int h)
{
#if 0
	XImage *img;
	int x, y;
	char *data;

	/* allocate buffer */
	if (x11_depth >= 24)
		data = malloc(w * 4 * h);
	else if (x11_depth > 8)	/* 15, 16 */
		data = malloc(w * 2 * h);
	else			/* 1,2,4,8 */
		data = malloc((w * x11_depth + 7) / 8 * h);

	/* copy from offscreen to screen */
	img = XCreateImage(x11_dpy, x11_vis, x11_depth, ZPixmap,
			   0, data, w, h, 8, 0);

	/* Use optimized loops for most common framebuffer modes */

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
	{
		ADDR8 dbuf = ((ADDR8) psd->addr) + destx + desty * psd->linelen;
		int linedelta = psd->linelen - w;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = *dbuf++;
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			dbuf += linedelta;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
	{
		ADDR16 dbuf = ((ADDR16) psd->addr) + destx + desty * psd->linelen;
		int linedelta = psd->linelen - w;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = *dbuf++;
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			dbuf += linedelta;
		}
	}
#elif MWPIXEL_FORMAT == MWPF_TRUECOLOR888
	{
		ADDR8 dbuf = ((ADDR8) psd->addr) + 3 * (destx + desty * psd->linelen);
		int linedelta = 3 * (psd->linelen - w);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = RGB2PIXEL888(dbuf[2], dbuf[1], dbuf[0]);
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
				dbuf += 3;
			}
			dbuf += linedelta;
		}
	}
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)
	{
		ADDR32 dbuf = ((ADDR32) psd->addr) + destx + desty * psd->linelen;
		int linedelta = psd->linelen - w;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = *dbuf++;
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			dbuf += linedelta;
		}
	}
#else /* MWPF_PALETTE*/
	{
		ADDR8 dbuf = ((ADDR8) psd->addr) + destx + desty * psd->linelen;
		int linedelta = psd->linelen - w;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				MWPIXELVAL c = *dbuf++;
				unsigned long pixel = PIXELVAL_to_pixel(c);
				XPutPixel(img, x, y, pixel);
			}
			dbuf += linedelta;
		}
	}
#endif

	XPutImage(x11_dpy, x11_win, x11_gc, img, 0, 0, destx, desty, w, h);
	XDestroyImage(img);
#endif
}

static void
win32_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;
	update_from_savebits(psd, x, y, width, height);
}
