/*
 * Touch-panel calibration program
 * Copyright (C) 1999 Bradley D. LaRonde <brad@ltc.com>
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "windows.h"
#include "mou_tp.h"
#include "transform.h"

static int xext, yext;
static CALIBRATION_PAIRS cps;
static CALIBRATION_PAIR* pcp = 0;

void DrawAbout(HDC hdc, RECT r)
{
	const int ver_major = 0;
	const int ver_minor = 5;
	const char app_name[] = "Touch Panel Calibrator";
	const char title[] = "%s, version %d.%d";
	const char copyright[] = "(C) 1999 Bradley D. LaRonde <brad@ltc.com>";
	const char warranty[] = "ABSOLUTELY NO WARRANTY";
	const char license1[] = "This is free software, and you are welcome to";
	const char license2[] = "redistribute it under certain conditions.";

	const int leading = 15;

	char s[1024];

	sprintf(s, title, app_name, ver_major, ver_minor);
	SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
	DrawText(hdc, s, -1, &r, DT_CENTER);

	r.top += leading;
	DrawText(hdc, copyright, -1, &r, DT_CENTER);

	r.top += leading;
	DrawText(hdc, warranty, -1, &r, DT_CENTER);

	r.top += leading;
	DrawText(hdc, license1, -1, &r, DT_CENTER);
	r.top += leading;
	DrawText(hdc, license2, -1, &r, DT_CENTER);
}

void DrawDone(HDC hdc, RECT r)
{
        const char donemsg[] = "Calibration is done!";
	SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        DrawText(hdc, donemsg, -1, &r, DT_CENTER);
}


void DrawTarget(HDC hdc, POINT p)
{
	const int scale = 9;
	const int center = 3;

	// I never knew that Windows GDI always leaves off
	// the last point in a line, and the right and bottom side of a rectangle.
	// Why would they make use always figure +1?  It isn't always obvious where
	// +1 is (i.e. a line going at a 10 degree angle).  Blech.
	// I can only hope that there is some overwhelmingly compelling reason why.

	Rectangle(hdc, p.x - center, p.y - center, p.x + center + 1, p.y + center + 1);

	// scale
	MoveToEx(hdc, p.x - scale, p.y, NULL);
	LineTo(hdc, p.x + scale + 1, p.y);

	MoveToEx(hdc, p.x, p.y - scale, NULL);
	LineTo(hdc, p.x, p.y + scale + 1);
}

void DrawLabelAlign(HDC hdc, POINT p, LPCSTR psz, int align)
{
	RECT r;
	const int w = 180;
	const int h = 14;
	const int m = 15;

	switch(align)
	{
		case 1:
			// right
			r.left = p.x + m;
			r.right = p.x + w + m;
			r.top =  p.y - (h/2);
			r.bottom = p.y + (h/2);
			break;

		case 2:
			// left
			r.left = p.x - (w + m);
			r.right = p.x - m;
			r.top =  p.y - (h/2);
			r.bottom = p.y + (h/2);
			break;

		case 3:
			// below
			r.left = p.x - (w/2);
			r.right = p.x + (w/2);
			r.top =  p.y + m;
			r.bottom = p.y + m + h;
			break;

		default:
			// at
			r.left = p.x;
			r.right = p.x + w;
			r.top =  p.y;
			r.bottom = p.y + h;
			break;
	}

	SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
	DrawText(hdc, psz, -1, &r, DT_LEFT);
}

void DrawLabel(HDC hdc, POINT p, LPCSTR psz)
{
	if ( p.x < (xext * 1 / 4) )
		return DrawLabelAlign(hdc, p, psz, 1);

	if ( p.x > (xext * 3 / 4) )
		return DrawLabelAlign(hdc, p, psz, 2);

	return DrawLabelAlign(hdc, p, psz, 3);
}

POINT GetTarget(int n)
{
	const int inset = 10;
	POINT p;

 	switch (n)
	{
		case 0:
			p.x = xext / 2; p.y = yext / 2;
			pcp = &cps.center;
			break;

		case 1:
			p.x = inset; p.y = inset;
			pcp = &cps.ul;
			break;

		case 2:
			p.x = xext - inset; p.y = inset;
			pcp = &cps.ur;
			break;

		case 3:
			p.x = xext - inset; p.y = yext - inset;
			pcp = &cps.lr;
			break;

		case 4:
			p.x = inset; p.y = yext - inset;
			pcp = &cps.ll;
			break;
		
		default:
			// return a random target
			p.x = random() / (RAND_MAX / xext);
			p.y = random() / (RAND_MAX / yext);
			pcp = 0;
			break;
	}

	return p;
}

static int total_targets = 5;
static int current_target = 0;
static POINT current_target_location;

void DoPaint(HDC hdc)
{
	const char szInstructions[] = "Please touch the center of the target.";

	POINT p;
	int i, n;
	int old_rop;
	POINT last = current_target_location;
	HPEN hOldPen;
	
	if (current_target == total_targets) {
		RECT r = {10, yext/2,  xext - 10, yext/2 + 40};
		DrawDone(hdc, r);
		return;
	}
	
	if (current_target == 0)
	{
		RECT r = {10, yext - 85, xext - 10, yext - 10};
		DrawAbout(hdc, r);
	}

	current_target_location = GetTarget(current_target);

	old_rop = SetROP2(hdc, R2_XORPEN);
	hOldPen = SelectObject(hdc, GetStockObject(WHITE_PEN));

	n = 20;
	for (i=0; i < n; i++)
	{
		p.x = last.x + ((current_target_location.x - last.x) * i / n);
		p.y = last.y + ((current_target_location.y - last.y) * i / n);
		DrawTarget(hdc, p);
		Sleep(60);
		DrawTarget(hdc, p);
	}

	// final position
	SetROP2(hdc, R2_COPYPEN);
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	DrawTarget(hdc, current_target_location);
	DrawLabel(hdc, current_target_location, szInstructions);

	// put things back
	SetROP2(hdc, old_rop);
	SelectObject(hdc, hOldPen);
}

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
	HDC hdc;
  
	switch(msg) {
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			DoPaint(hdc);
			EndPaint(hwnd, &ps);
			break;

		case WM_NCHITTEST:
			return HTCLIENT;

		case WM_LBUTTONUP:
			if ( pcp != 0 )
			{
				pcp->screen.x = current_target_location.x * TRANSFORMATION_UNITS_PER_PIXEL;
				pcp->screen.y = current_target_location.y * TRANSFORMATION_UNITS_PER_PIXEL;
				pcp->device.x = GET_X_LPARAM(lp);
				pcp->device.y = GET_Y_LPARAM(lp);
			}

			if ( ++current_target == total_targets )
			{
				TRANSFORMATION_COEFFICIENTS tc;
#if 0
				CalcTransformationCoefficientsSimple(&cps, &tc);
				printf("%d %d %d %d %d %d %d\n",
					tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
				CalcTransformationCoefficientsBetter(&cps, &tc);
				printf("%d %d %d %d %d %d %d\n",
					tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
				CalcTransformationCoefficientsEvenBetter(&cps, &tc);
				printf("%d %d %d %d %d %d %d\n",
					tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
#endif
				CalcTransformationCoefficientsBest(&cps, &tc);
				printf("%d %d %d %d %d %d %d\n",
					tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
				InvalidateRect(hwnd, NULL, TRUE);
				UpdateWindow(hwnd);
				PostQuitMessage(0);
				break;
			}
			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
			break;

		default:
			return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASS wc;
	RECT r;
	HWND hwnd;
	MSG msg;
	MWCOORD big;

	srandom(time(NULL));

	/* WndButtonRegister(NULL); */

	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "tpcal";
	RegisterClass(&wc);

	GetWindowRect(GetDesktopWindow(), &r);
	xext = r.right;
	yext = r.bottom;

	hwnd = CreateWindowEx(0L, "tpcal", "Touch Panel Calibration",
		WS_VISIBLE, 0, 0, xext, yext,
		NULL, (HMENU)1, NULL, NULL);

	// Don't restrict mouse much in order to handle uncalibrated points.
	big = 1000000;
	GdRestrictMouse(-big, -big, big, big);

	// We want all mouse events - even ones outside our window.
	SetCapture(hwnd);

	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

int enable_pointing_coordinate_transform;

int MwUserInit(int ac, char** av)
{
	enable_pointing_coordinate_transform = 0;

	if ( ac > 1 )
		total_targets = atol(av[1]);

	return 0;
}

