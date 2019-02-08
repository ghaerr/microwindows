/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
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
#include <windows.h>
#include <wingdi.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#define APP_NAME "Microwindows"

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

HWND winRootWindow = NULL;
static HDC dcBuffer = NULL;
static HBITMAP dcBitmap = NULL;
static HBITMAP dcOldBitmap;
static MWCOORD upminX, upminY, upmaxX, upmaxY;	/* win32_preselect and win32_update*/

static void win32_draw(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);
static int win32_pollevents(void);

LRESULT CALLBACK
myWindowProc(HWND hWnd,	UINT Msg, WPARAM wParam, LPARAM lParam)
{
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
		BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top, 
			ps.rcPaint.right - ps.rcPaint.left,
			ps.rcPaint.bottom - ps.rcPaint.top, 
			dcBuffer, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		SelectObject(dcBuffer, dcOldBitmap);
		DeleteDC(dcBuffer);	
		DeleteObject(dcBitmap);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;	
}
 
/* open screen driver*/
static PSD
win32_open(PSD psd)
{
	/* init psd and allocate framebuffer*/
	int flags = PSF_SCREEN | PSF_ADDRMALLOC | PSF_DELAYUPDATE;

	if (!gen_initpsd(psd, MWPIXEL_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, flags))
		return NULL;

	HANDLE hInstance = GetModuleHandle(NULL);
	WNDCLASS wc;
	//HDC rootDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	//int depth = GetDeviceCaps(rootDC, BITSPIXEL);
	//DeleteDC(rootDC);

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
		dcBitmap = CreateCompatibleBitmap(dc, SCREEN_WIDTH, SCREEN_HEIGHT);
		dcBuffer = CreateCompatibleDC(dc);
		dcOldBitmap = SelectObject(dcBuffer, dcBitmap);
		ReleaseDC(winRootWindow, dc);

		ShowWindow(winRootWindow, SW_SHOW);
		UpdateWindow(winRootWindow);
	}

	return &scrdev;
}

/* return nonzero if event available*/
static int
win32_pollevents(void)
{
	MSG msg;

	if (PeekMessage(&msg, winRootWindow, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE))
		return 1;

	if (PeekMessage(&msg, winRootWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE))
		return 1;

	if (PeekMessage(&msg, winRootWindow, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

static void
win32_close(PSD psd)
{
	if (winRootWindow)
		SendMessage(winRootWindow, WM_DESTROY, 0, 0);
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
	psi->fbdriver = FALSE;	/* not running fb driver, no direct map */
}


#if 0000 /* from old driver format*/
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
win32_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL pixel)
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
		dstpsd->Blit(dstpsd, destx, desty, w, h, srcpsd, srcx, srcy, op);
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
        case MWPF_TRUECOLORRGB:
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
        case MWPF_TRUECOLORARGB:
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
#endif /* 0000*/

/* update window from Microwindows framebuffer*/
static void
win32_draw(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	BITMAPV4HEADER bmpInfo;

	HDC dc = GetDC(winRootWindow);

	/* MWPF_TRUECOLORARGB only for now*/
	bmpInfo.bV4AlphaMask = 0xff000000;
	bmpInfo.bV4RedMask  = 0xff0000;
	bmpInfo.bV4GreenMask= 0x00ff00;
	bmpInfo.bV4BlueMask = 0x0000ff;
	bmpInfo.bV4V4Compression = BI_BITFIELDS;
	bmpInfo.bV4XPelsPerMeter = 3078;
	bmpInfo.bV4YPelsPerMeter = 3078;
	bmpInfo.bV4ClrUsed = 0;
	bmpInfo.bV4ClrImportant = 0;   
	bmpInfo.bV4CSType = LCS_sRGB;
	SetDIBitsToDevice(dc, x, y, width, height, 0, 0, 0, psd->yres, psd->addr, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS);
	SetDIBitsToDevice(dcBuffer, x, y, width, height, 0, 0, 0, psd->yres, psd->addr, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS);
	ReleaseDC(winRootWindow, dc);
}

static void
win32_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

	/* window moves require delaying updates until preselect for speed*/
	if ((psd->flags & PSF_DELAYUPDATE)) {
			/* calc aggregate update rectangle*/
			upminX = min(x, upminX);
			upminY = min(y, upminY);
			upmaxX = max(upmaxX, x+width-1);
			upmaxY = max(upmaxY, y+height-1);
	} else
		win32_draw(psd, x, y, width, height);
}

/* called before select(), returns # pending events*/
static int
win32_preselect(PSD psd)
{
	/* perform single blit update of aggregate update region to SDL server*/
	if ((psd->flags & PSF_DELAYUPDATE) && (upmaxX || upmaxY)) {
		win32_draw(psd, upminX, upminY, upmaxX-upminX+1, upmaxY-upminY+1);

		/* reset update region*/
		upminX = upminY = ~(1 << ((sizeof(int)*8)-1));	// largest positive int
		upmaxX = upmaxY = 0;
	}

	/* return nonzero if SDL event available*/
	return win32_pollevents();
}
