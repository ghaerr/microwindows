/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 3D Graphics Library for Micro-Windows
 */
#define MWINCLUDECOLORS
#include "windows.h"
#include "device.h"
#include "graph3d.h"
#define USEBLIT		1	/* =1 to use memDC's*/

static int	nxpix;
static int	nypix;
static vec1	xscale;
static vec1	yscale;
static vec3	eye;
static vec3	direct;
static double 	Q[5][5];
static HDC	hdc;
static HDC	hdcMem;
static HBITMAP	hbmp, hbmpOrg;

/* setup eye, direction, calc observation matrix Q*/
void
look3(vec1 x, vec1 y, vec1 z)
{
	eye.x = x;
	eye.y = y;
	eye.z = z;
	direct.x = -eye.x;
	direct.y = -eye.y;
	direct.z = -eye.z;
	findQ();
}

void
init3(HDC hDC, HWND memhwnd)
{
	HBRUSH	hbr;

	hdc = hDC;
	if(hdc) {
		nxpix = hdc->hwnd->clirect.right - hdc->hwnd->clirect.left;
		nypix = hdc->hwnd->clirect.bottom - hdc->hwnd->clirect.top;
		xscale = (vec1)(nxpix-1) / nxpix * nxpix/2;
		yscale = (vec1)(nypix-1) / nypix * nypix/2;

		if(memhwnd) {
			hdcMem = CreateCompatibleDC(NULL);
			if(hdcMem) {
				hbmp = CreateCompatibleBitmap(hdcMem,
					nxpix, nypix);
				hbmpOrg = SelectObject(hdcMem, hbmp);
				hdc = hdcMem;
			}
			hbr = (HBRUSH)GetClassLong(memhwnd, GCL_HBRBACKGROUND);
			FillRect(hdc, NULL, hbr);
		}
		/* create pen for setcolor3() color override*/
		SelectObject(hdc, CreatePen(PS_SOLID, 1, BLACK));
	}
}

void
paint3(HDC hDC)
{
	if(hdcMem) {
		BitBlt(hDC, 0, 0, nxpix, nypix, hdcMem, 0, 0, SRCCOPY);
		DeleteObject(SelectObject(hdcMem, hbmpOrg));
		DeleteDC(hdcMem);
	}
	hdcMem = NULL;
}

int
fx(vec1 x)
{
	return (int)(x * xscale + nxpix*0.5 - 0.5);
}

int
fy(vec1 y)
{
	return (int)(y * yscale + nypix*0.5 - 0.5);
}

void
moveto3(vec2 pt)
{
	MoveToEx(hdc, fx(pt.x), fy(pt.y), NULL);
}

void
setcolor3(MWCOLORVAL c)
{
	if(hdc)
		hdc->pen->color = c;
}

void
lineto3(vec2 pt)
{
	LineTo(hdc, fx(pt.x), fy(pt.y));
}

void
polyfill(int n, vec2 points[])
{
	int	i;
	int	xoff, yoff;
	MWPOINT	pv[MAXPOLY];

	if(!hdc)
		return;

	/* calc window offset*/
	xoff = hdc->hwnd->clirect.left;
	yoff = hdc->hwnd->clirect.top;

	/* only plot non-trivial polygons*/
	if(n > 2) {
		for(i=0; i<n; ++i) {
			pv[i].x = fx(points[i].x) + xoff;
			pv[i].y = fy(points[i].y) + yoff;
			/* fix: floating round error, y intercept difference
			 * with GdLine
			 */
			/*pv[i].x = fx(points[i].x + xoff);*/
			/*pv[i].y = fy(points[i].y + yoff);*/
		}
		GdSetForeground(GdFindColor(hdc->pen->color));
		GdFillPoly(hdc->psd, n, pv);
	}
}

void
square(void)
{
	vec2	pt0, pt1, pt2, pt3;

	pt0.x = -1; pt0.y = 1;
	pt1.x = -1; pt1.y = -1;
	pt2.x = 1; pt2.y = -1;
	pt3.x = 1; pt3.y = 1;
	moveto3(pt0);
	lineto3(pt1);
	lineto3(pt2);
	lineto3(pt3);
	lineto3(pt0);
}

void
circle3(vec1 r)
{
	vec1 	theta = 0;
	vec1	thinc = 2*pi/100;
	int 	i;
	vec2	pt;

	pt.x = r;
	pt.y = 0.0;
	moveto3(pt);

	for(i=0; i<100; ++i) {
		theta = theta + thinc;
		pt.x = r*cos(theta);
		pt.y = r*sin(theta);
		lineto3(pt);
	}
}

void
daisy(vec1 r,int points)
{
	int	i, j;
	vec1	theta = 0;
	vec1	thinc;
	vec2	pt[100];

	/* calculate n points on a circle*/
	thinc = 2*pi/points;
	for(i=0; i<points; ++i) {
		pt[i].x = r*cos(theta);
		pt[i].y = r*sin(theta);
		theta += thinc;
	}

	/* join point i to point j for all 0 <= i < j < n */
	for(i=0; i<points-1; ++i) {
		for(j=i+1; j<points; ++j) {
			moveto3(pt[i]);
			lineto3(pt[j]);
		}
	}
}

void
rose(vec1 r,int levels,int points)
{
	int	i, j, m, n;
	vec1	r1, theta, thinc;
	vec2	inner[100];
	vec2	outer[100];
	vec2	triangle[3];

	m = levels;
	n = points;
	thinc = 2*pi/n;

	/* initial inner circle*/
	for(i=0; i<n; ++i) {
		inner[i].x = 0.0;
		inner[i].y = 0.0;
	}

	/* loop thru m levels*/
	for(j=1; j<=m; ++j) {
		theta = -j*pi/n;
		r1 = r * (vec1)j/m;

		/* calc n points on outer circle*/
		for(i=0; i<n; ++i) {
			theta += thinc;
			outer[i].x = r1*cos(theta);
			outer[i].y = r1*sin(theta);
		}

		/* construct/draw triangles with vertices on
		 * inner and outer circles
		 */
		for(i=0; i<n; ++i) {
			triangle[0] = outer[i];
			triangle[1] = outer[(i+1) % n];
			triangle[2] = inner[i];

			/* fill triangle in red*/
			setcolor3(RED);
			polyfill(3, triangle);

#if 1
			/* outline triangle in white*/
			setcolor3(WHITE);
			moveto3(triangle[0]);
			lineto3(triangle[1]);
			lineto3(triangle[2]);
			lineto3(triangle[0]);
#endif
		}

		/* copy points on outer circle to inner arrays*/
		for(i=0; i<n; ++i)
			inner[i] = outer[i];
	}
}

/* draw a triangle with cordners v0, v1, v2*/
void
triangle(vec2 v0, vec2 v1, vec2 v2)
{
	vec2	poly[3];

	poly[0] = v0;
	poly[1] = v1;
	poly[2] = v2;

	setcolor3(GREEN);
	polyfill(3, poly);
	setcolor3(BLACK);
	moveto3(poly[2]);
	lineto3(poly[0]);
	lineto3(poly[1]);
	lineto3(poly[2]);
}

/* draw a quadrilateral with corners v0, v1, v2, v3*/
void
quadrilateral(vec2 v0, vec2 v1, vec2 v2, vec2 v3)
{
	vec2	poly[4];

	poly[0] = v0;
	poly[1] = v1;
	poly[2] = v2;
	poly[3] = v3;
	setcolor3(GREEN);
	polyfill(4, poly);
	setcolor3(BLACK);
	moveto3(poly[3]);
	lineto3(poly[0]);
	lineto3(poly[1]);
	lineto3(poly[2]);
	lineto3(poly[3]);
}

/* find intersection of lines v0 to v1 and v2 to v3*/
static int
patch(vec2 v0, vec2 v1, vec2 v2, vec2 v3)
{
	vec1	denom;
	vec1	mu;
	vec2	v4;

	denom = (v1.x-v0.x)*(v3.y-v2.y) - (v1.y-v0.y)*(v3.x-v2.x);
	if(fabs(denom) > epsilon) {
		mu = ((v2.x-v0.x)*(v3.y-v2.y) - (v2.y-v0.y)*(v3.x-v2.x))/denom;

		/* if intersection between lines v0 to v1 and v2 to v3,
		 * call it v4 and form triangles v0,v2,v4 and v1,v3,v4
		 */
		if(mu >= 0 && mu <= 1) {
			v4.x = (1-mu)*v0.x + mu*v1.x;
			v4.y = (1-mu)*v0.y + mu*v1.y;
			triangle(v0, v2, v4);
			triangle(v1, v3, v4);
			return 0;
		}
	}

	/* else find intersection of lines v0 to v2 and v1 to v3*/
	denom = (v2.x-v0.x)*(v3.y-v1.y) - (v2.y-v0.y)*(v3.x-v1.x);
	if(fabs(denom) > epsilon) {
		mu = ((v1.x-v0.x)*(v3.y-v1.y) - (v1.y-v0.y)*(v3.x-v1.x))/denom;

		/* if intersection between v0 and v1, call it v4
		 * and form triangles v0,v1,v4 and v2,v3,v4
		 */
		if(mu >= 0 && mu <= 1) {
			v4.x = (1-mu)*v0.x + mu*v2.x;
			v4.y = (1-mu)*v0.y + mu*v2.y;
			triangle(v0, v1, v4);
			triangle(v2, v3, v4);
			return 0;
		}
	}

	/* there are no proper intersections so form quadrilateral v0,v1,v3,v2*/
	quadrilateral(v0, v1, v3, v2);
	return 1;
}

/* plotted function*/
static vec1
plotfn(vec1 x, vec1 z)
{
	vec1	t;

	/* y = 4sin(sqrt(x*x+z*z))/sqrt(x*x+z*z) */
	t = sqrt(x*x + z*z);
	if(fabs(t) < epsilon)
		return 4.0;
	return 4.0 * sin(t) / t;
}

/* draw mathematical function plotfn*/
void
drawgrid(vec1 xmin, vec1 xmax, int nx, vec1 zmin, vec1 zmax, int nz)
{
	int	i, j;
	vec1	xi, xstep, yij;
	vec1	zj, zstep;
	vec2	v[2][100];
	double	S[5][5];

	/* scale it down*/
	scale3(1.0/(xmax-xmin)*2, 1.0/(xmax-xmin)*2, 1.0/(zmax-zmin), S);
	mult3(Q, S, Q);

	/* grid from xmin to xmax in nx steps and zmin to xmax in nz steps*/
	xstep = (xmax-xmin)/nx;
	zstep = (zmax-zmin)/nz;
	xi = xmin;
	zj = zmin;

	/* calc grid points on first fixed-z line, fine the y-height
	 * and transfrorm the points (xi,yij,zj) into observed
	 * position.  Observed first set stored in v[0,1..nx]
	 */
	for(i=0; i<=nx; ++i) {
		yij = plotfn(xi, zj);
		v[0][i].x = Q[1][1]*xi + Q[1][2]*yij + Q[1][3]*zj;
		v[0][i].y = Q[2][1]*xi + Q[2][2]*yij + Q[2][3]*zj;
		xi += xstep;
	}

	/* run thru consecutive fixed-z lines (the second set)*/
	for(j=0; j<nz; ++j) {
		xi = xmin;
		zj += zstep;

		/* calc grid points on this second set, find the
		 * y-height and transform the points (xi,yij,zj)
		 * into observed position.  Observed second set
		 * stored in v[1,0..nx]
		 */
		for(i=0; i<=nx; ++i) {
			yij = plotfn(xi, zj);
			v[1][i].x = Q[1][1]*xi + Q[1][2]*yij + Q[1][3]*zj;
			v[1][i].y = Q[2][1]*xi + Q[2][2]*yij + Q[2][3]*zj;
			xi += xstep;
		}

		/* run thru the nx patches formed by these two sets*/
		for(i=0; i<nx; ++i)
			patch(v[0][i], v[0][i+1], v[1][i], v[1][i+1]);

		/* copy second set into first set*/
		for(i=0; i<=nx; ++i)
			v[0][i] = v[1][i];
	}
}

/* returns the angle whose tangent is y/x.
 * all anomalies such as x=0 are also checked
 */
vec1
angle(vec1 x, vec1 y)
{
	if(fabs(x) < epsilon)
		if(fabs(y) < epsilon)
			return 0.0;
		else 
			if(y > 0.0)
				return pi*0.5;
			else return pi*1.5;
	else
		if(x < 0.0)
			return atan(y/x) + pi;
		else return atan(y/x);
}

/* calc 3d scaling matrix A giving scaling vector sx,sy,sz.
 * one unit on the x axis becomes sx units, one unit on y, sy,
 * and one unit on the z axis becomes sz units
 */
void
scale3(vec1 sx, vec1 sy, vec1 sz, double A[][5])
{
	int	i, j;

	for(i=1; i<5; ++i)
		for(j=1; j<5; ++j)
			A[i][j] = 0.0;
	A[1][1] = sx;
	A[2][2] = sy;
	A[3][3] = sz;
	A[4][4] = 1.0;
}

/* calc 3d axes translation matrix A
 * origin translated by vectdor tx,ty,tz
 */
void
tran3(vec1 tx, vec1 ty, vec1 tz, double A[][5])
{
	int	i, j;

	for(i=1; i<5; ++i) {
		for(j=1; j<5; ++j)
			A[i][j] = 0.0;
		A[i][i] = 1.0;
	}
	A[1][4] = -tx;
	A[2][4] = -ty;
	A[3][4] = -tz;
}

/* calc 3d axes rotation matrix A.  The axes are
 * rotated anti-clockwise through an angle theta radians
 * about an axis specified by m: m=1 means x, m=2 y, m=3 z axis
 */
void
rot3(int m, vec1 theta, double A[][5])
{
	int	i, j, m1, m2;
	vec1	c, s;

	for(i=1; i<5; ++i) 
		for(j=1; j<5; ++j)
			A[i][j] = 0.0;
	A[m][m] = 1.0;
	A[4][4] = 1.0;
	m1 = (m % 3) + 1;
	m2 = (m1 % 3) + 1;
	c = cos(theta);
	s = sin(theta);
	A[m1][m1] = c;
	A[m2][m2] = c;
	A[m1][m2] = s;
	A[m2][m1] = s;
}

/* calc the matrix product C of two matrices A and B*/
void
mult3(double A[][5], double B[][5], double C[][5])
{
	int	i, j, k;
	vec1	ab;

	for(i=1; i<5; ++i) 
		for(j=1; j<5; ++j) {
			ab = 0;
			for(k=1; k<5; ++k)
				ab += A[i][k] * B[k][j];
			C[i][j] = ab;
		}
}

/* calc observation matrix Q for given observer*/
void
findQ(void)
{
	vec1	alpha, beta, gamma, v, w;
	double	E[5][5];
	double	F[5][5];
	double	G[5][5];
	double	H[5][5];
	double	U[5][5];

	/* calc translation matrix F*/
	tran3(eye.x, eye.y, eye.z, F);

	/* calc rotation matrix G*/
	alpha = angle(-direct.x, -direct.y);
	rot3(3, alpha, G);

	/* calc rotation matrix H*/
	v = sqrt(direct.x*direct.x + direct.y*direct.y);
	beta = angle(-direct.z, v);
	rot3(2, beta, H);

	/* calc rotation matrix U*/
	w = sqrt(v*v + direct.z*direct.z);
	gamma = angle(-direct.x*w, direct.y*direct.z);
	rot3(3, -gamma, U);

	/* combine the transformations to find Q*/
	mult3(G, F, Q);
	mult3(H, Q, E);
	mult3(U, E, Q);
}
