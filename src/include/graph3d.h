/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 3D Graphics Library for Micro-Windows
 */

#ifndef _GRAPH3D_H_
#define _GRAPH3D_H_

#include <math.h>

#define pi	3.1415926535
#define epsilon	0.000001
#define MAXPOLY	32		/* max polygon points*/

typedef FLOAT	vec1;

typedef struct {
	FLOAT x, y;
} vec2;

typedef struct {
	FLOAT x, y, z;
} vec3;

/* entry points*/
void init3(HDC hDC,HWND hwndmem);
void paint3(HDC hDC);
void setcolor3(MWCOLORVAL c);
void moveto3(vec2 pt);
void lineto3(vec2 pt);
void polyfill(int n, vec2 points[]);
void square(void);
void circle3(vec1 r);
void daisy(vec1 r,int points);
void rose(vec1 r,int levels,int points);
void triangle(vec2 v0, vec2 v1, vec2 v2);
void quadrilateral(vec2 v0, vec2 v1, vec2 v2, vec2 v3);
void look3(vec1 x, vec1 y, vec1 z);
void drawgrid(vec1 xmin, vec1 xmax, int nx, vec1 zmin, vec1 zmax, int nz);
void scale3(vec1 sx, vec1 sy, vec1 sz, double A[][5]);
void tran3(vec1 tx, vec1 ty, vec1 tz, double A[][5]);
void rot3(int m, vec1 theta, double A[][5]);
void mult3(double A[][5], double B[][5], double C[][5]);
void findQ(void);

#endif
