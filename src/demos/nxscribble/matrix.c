/***********************************************************************

matrix.c - simple matrix operations

Copyright (C) 1991 Dean Rubine

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License. See ../COPYING for
the full agreement.

**********************************************************************/

/*
 Simple matrix operations
 Why I am writing this stuff over is beyond me
*/

#undef PIQ_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"
#include "matrix.h"


typedef	struct array_header *Array;

#define EPSILON		(1.0e-10)	/* zero range */

/*
 Allocation functions
*/


Vector
NewVector(r)
int r;
{
	register struct array_header *a;
	register Vector v;

	a = (struct array_header *)
	    allocate(sizeof(struct array_header) + r * sizeof(double), char);
	a->ndims = 1;
	a->nrows = r;
	a->ncols = 1;
	v = (Vector) (a + 1);

#define CHECK
#ifdef CHECK
	if(HEADER(v) != (struct array_header *) a ||
	   NDIMS(v) != 1 || NROWS(v) != r || NCOLS(v) != 1) {
	    	exit_error("NewVector error: v=%x H: %x,%x  D:%d,%d  R:%d,%d  C:%d,%d\n", v,  HEADER(v), a,  NDIMS(v), 1,  NROWS(v), r, NCOLS(v), 1);
	    }
#endif

	return v;
}

Matrix
NewMatrix(r, c)
int r, c;
{
	register struct array_header *a = (struct array_header *)
	   allocate(sizeof(struct array_header) + r * sizeof(double *), char);
	register int i;
	register Matrix m;

	m = (Matrix) (a + 1);
	for(i = 0; i < r; i++)
		m[i] = allocate(c, double);
	a->ndims = 2;
	a->nrows = r;
	a->ncols = c;
	return m;
}

void
FreeVector(v)
Vector v;
{
	free(HEADER(v));
}

void
FreeMatrix(m)
Matrix m;
{
	register int i;

	for(i = 0; i < NROWS(m); i++)
		free(m[i]);
	free(HEADER(m));
}

Vector
VectorCopy(v)
register Vector v;
{
	register Vector r = NewVector(NROWS(v));
	register int i;

	for(i = 0; i < NROWS(v); i++)
		r[i] = v[i];
	return r;
}

Matrix
MatrixCopy(m)
register Matrix m;
{
	register Matrix r = NewMatrix(NROWS(m), NCOLS(m));
	register int i, j;

	for(i = 0; i < NROWS(m); i++)
		for(j = 0; j < NROWS(m); j++)
			r[i][j] = m[i][j];
	return r;
}

/* Null vector and matrixes */


void
ZeroVector(v)
Vector v;
{
	register int i;
	for(i = 0; i < NROWS(v); i++) v[i] = 0.0;
}


void
ZeroMatrix(m)
Matrix m;
{
	register int i, j;
	for(i = 0; i < NROWS(m); i++)
		for(j = 0; j < NCOLS(m); j++)
			m[i][j] = 0.0;
}

void
FillMatrix(m, fill)
Matrix m;
double fill;
{
	register int i, j;
	for(i = 0; i < NROWS(m); i++)
		for(j = 0; j < NCOLS(m); j++)
			m[i][j] = fill;
}

double
InnerProduct(v1, v2)
register Vector v1, v2;
{
	double result = 0;
	register int n = NROWS(v1);
	if(n != NROWS(v2)) {
		exit_error("InnerProduct %d x %d ", n, NROWS(v2));
	    }
	while(--n >= 0)
		result += *v1++ * *v2++;
	return result;
}

void
MatrixMultiply(m1, m2, prod)
register Matrix m1, m2, prod;
{
	register int i, j, k;
	double sum;

	if(NCOLS(m1) != NROWS(m2)) {
		error("MatrixMultiply: Can't multiply %dx%d and %dx%d matrices",
			NROWS(m1), NCOLS(m1), NROWS(m2), NCOLS(m2));
		return;
	    }
	if(NROWS(prod) != NROWS(m1) || NCOLS(prod) != NCOLS(m2)) {
		error("MatrixMultiply: %dx%d times %dx%d does not give %dx%d product",
			NROWS(m1), NCOLS(m1), NROWS(m2), NCOLS(m2),
			NROWS(prod), NCOLS(prod));
		return;
	    }

	for(i = 0; i < NROWS(m1); i++)
		for(j = 0; j < NCOLS(m2); j++) {
			sum = 0;
			for(k = 0; k < NCOLS(m1); k++)
				sum += m1[i][k] * m2[k][j];
			prod[i][j] = sum;
		}
}

/*
Compute result = v'm where
	v is a column vector (r x 1)
	m is a matrix (r x c)
	result is a column vector (c x 1)
*/

void
VectorTimesMatrix(v, m, prod)
Vector v;
Matrix m;
Vector prod;
{
	register int i, j;

	if(NROWS(v) != NROWS(m)) {
		error("VectorTimesMatrix: Can't multiply %d vector by %dx%d",
			NROWS(v), NROWS(m), NCOLS(m));
		return;
	    }
	if(NROWS(prod) != NCOLS(m)) {
		error("VectorTimesMatrix: %d vector times %dx%d mat does not fit in %d product" ,
			NROWS(v), NROWS(m), NCOLS(m), NROWS(prod));
		return;
	    }

	for(j = 0; j < NCOLS(m); j++) {
		prod[j] = 0;
		for(i = 0; i < NROWS(m); i++)
			prod[j] += v[i] * m[i][j];
	}
}	

void
ScalarTimesVector(s, v, product)
double s;
register Vector v, product;
{
	register int n = NROWS(v);

	if(NROWS(v) != NROWS(product)) {
		error("ScalarTimesVector: result wrong size (%d!=%d)",
			NROWS(v), NROWS(product));
		return;
	    }

	while(--n >= 0)
		*product++ = s * *v++;
}

void
ScalarTimesMatrix(s, m, product)
double s;
register Matrix m, product;
{
	register int i, j;

	if(NROWS(m) != NROWS(product)  || 
	   NCOLS(m) != NCOLS(product)) {
		error("ScalarTimesMatrix: result wrong size (%d!=%d)or(%d!=%d)",
			NROWS(m), NROWS(product),
			NCOLS(m), NCOLS(product));
		return;
	    }

	for(i = 0; i < NROWS(m); i++)
		for(j = 0; j < NCOLS(m); j++)
			product[i][j] = s * m[i][j];
}

/*
 Compute v'mv
 */

double
QuadraticForm(v, m)
register Vector v;
register Matrix m;
{
	register int i, j, n;
	double result = 0;

	n = NROWS(v);

	if(n != NROWS(m) || n != NCOLS(m)) {
		exit_error("QuadraticForm: bad matrix size (%dx%d not %dx%d)",
			NROWS(m), NCOLS(m), n, n);
	    }
	for(i = 0; i < n; i++)
		for(j = 0; j < n; j++) {

#ifdef PIQ_DEBUG
			printf("%g*%g*%g [%g] %s ",
			m[i][j],v[i],v[j],
			m[i][j] * v[i] * v[j],
			i==n-1&&j==n-1? "=" : "+");
#endif

			result += m[i][j] * v[i] * v[j];
		}
	return result;
}

/* Matrix inversion using full pivoting.
 * The standard Gauss-Jordan method is used.
 * The return value is the determinant.
 * The input matrix may be the same as the result matrix
 *
 *	det = InvertMatrix(inputmatrix, resultmatrix);
 *
 * HISTORY
 * 26-Feb-82  David Smith (drs) at Carnegie-Mellon University
 *	Written.
 * Sun Mar 20 19:36:16 EST 1988 - converted to this form by Dean Rubine
 *
 */

int	DebugInvertMatrix = 0;

#define PERMBUFSIZE 200	/* Max mat size */

#define _abs(x) ((x)>=0 ? (x) : -(x))

double
InvertMatrix(ym, rm)
Matrix ym, rm;
{
	register int i, j, k;
	double det, biga, recip_biga, hold;
	int l[PERMBUFSIZE], m[PERMBUFSIZE];
	register int n;

	if(NROWS(ym) != NCOLS(ym)) {
		exit_error("InvertMatrix: not square");
	    }

	n = NROWS(ym);

	if(n != NROWS(rm) || n != NCOLS(rm)) {
		exit_error("InvertMatrix: result wrong size");
	    }

	/* Copy ym to rm */
	
	if(ym != rm)
		for(i = 0; i < n; i++)
			for(j = 0; j < n; j++)
				rm[i][j] = ym[i][j];

	/*if(DebugInvertMatrix) PrintMatrix(rm, "Inverting (det=%g)\n", det);*/

    /* Allocate permutation vectors for l and m, with the same origin
       as the matrix. */

	if (n >= PERMBUFSIZE) {
		exit_error("InvertMatrix: PERMBUFSIZE");
	    }

	det = 1.0;
	for (k = 0; k < n;  k++) {
		l[k] = k;  m[k] = k;
		biga = rm[k][k];

		/* Find the biggest element in the submatrix */
		for (i = k;  i < n;  i++)
			for (j = k; j < n; j++)
				if (_abs(rm[i][j]) > _abs(biga)) {
					biga = rm[i][j];
					l[k] = i;
					m[k] = j;
				}

		if(DebugInvertMatrix) 
			if(biga == 0.0)
				PrintMatrix(m, "found zero biga = %g\n", biga);

		/* Interchange rows */
		i = l[k];
		if (i > k)
			for (j = 0; j < n; j++) {
				hold = -rm[k][j];
				rm[k][j] = rm[i][j];
				rm[i][j] = hold;
			}

		/* Interchange columns */
		j = m[k];
		if (j > k)
			for (i = 0; i < n; i++) {
				hold = -rm[i][k];
				rm[i][k] = rm[i][j];
				rm[i][j] = hold;
			}

		/* Divide column by minus pivot
		    (value of pivot element is contained in biga). */
		if (biga == 0.0) {
			return 0.0;
		}

		if(DebugInvertMatrix) printf("biga = %g\n", biga);
		recip_biga = 1/biga;
		for (i = 0; i < n; i++)
			if (i != k)
				rm[i][k] *= -recip_biga;

		/* Reduce matrix */
		for (i = 0; i < n; i++)
			if (i != k) {
				hold = rm[i][k];
				for (j = 0; j < n; j++)
					if (j != k)
						rm[i][j] += hold * rm[k][j];
			}

		/* Divide row by pivot */
		for (j = 0; j < n; j++)
			if (j != k)
				rm[k][j] *= recip_biga;

		det *= biga;	/* Product of pivots */
		if(DebugInvertMatrix) printf("det = %g\n", det);
		rm[k][k] = recip_biga;

	}	/* K loop */

	/* Final row & column interchanges */
	for (k = n - 1; k >= 0; k--) {
		i = l[k];
		if (i > k)
			for (j = 0; j < n; j++) {
				hold = rm[j][k];
				rm[j][k] = -rm[j][i];
				rm[j][i] = hold;
			}
		j = m[k];
		if (j > k)
			for (i = 0; i < n; i++) {
				hold = rm[k][i];
				rm[k][i] = -rm[j][i];
				rm[j][i] = hold;
			}
	}

	if(DebugInvertMatrix) printf("returning, det = %g\n", det);

	return det;
}


#include "bitvector.h"

Vector
SliceVector(v, rowmask)
Vector v;
BitVector rowmask;
{
	register int i, ri;

	Vector r = NewVector(bitcount(NROWS(v), rowmask));
	for(i = ri = 0; i < NROWS(v); i++)
		if(IS_SET(i, rowmask) )
			r[ri++] = v[i];
	return r;
}

Matrix
SliceMatrix(m, rowmask, colmask)
Matrix m;
BitVector rowmask, colmask;
{
	register int i, ri, j, rj;

	Matrix r;
	
	r = NewMatrix(bitcount(NROWS(m), rowmask),
			     bitcount(NCOLS(m), colmask));
	for(i = ri = 0; i < NROWS(m); i++)
		if(IS_SET(i, rowmask) ) {
			for(j = rj = 0; j < NCOLS(m); j++)
				if(IS_SET(j, colmask))
					r[ri][rj++] = m[i][j];
			ri++;
		}

	return r;
}

Matrix
DeSliceMatrix(m, fill, rowmask, colmask, r)
Matrix m;
double fill;
BitVector rowmask, colmask;
Matrix r;
{
	register int i, ri, j, rj;

	FillMatrix(r, fill);

	for(i = ri = 0; i < NROWS(r); i++) {
		if(IS_SET(i, rowmask) )  {
			for(j = rj = 0; j < NCOLS(r); j++)
				if(IS_SET(j, colmask))
					r[i][j] = m[ri][rj++];
			ri++;
		}
	}

	return r;
}

void
OutputVector(f, v)
FILE *f;
register Vector v;
{
	register int i;
	fprintf(f, " V %d   ", NROWS(v));
	for(i = 0; i < NROWS(v); i++)
		fprintf(f, " %g", v[i]);
	fprintf(f, "\n");
}

Vector
InputVector(f)
FILE *f;
{
	register Vector v;
	register int i;
	char check[4];
	int nrows;

	if(fscanf(f, "%1s %d", check, &nrows) != 2) {
		exit_error("InputVector fscanf 1");
	    }
	if(check[0] != 'V') {
		exit_error("InputVector check");
	    }
	v = NewVector(nrows);
	for(i = 0; i < nrows; i++) {
		if(fscanf(f, "%lf", &v[i]) != 1) {
			exit_error("InputVector fscanf 2");
		    }
        }
	return v;
}

void
OutputMatrix(f, m)
FILE* f;
register Matrix m;
{
	register int i, j;
	fprintf(f, " M %d %d\n", NROWS(m), NCOLS(m));
	for(i = 0; i < NROWS(m);  i++) {
		for(j = 0; j < NCOLS(m); j++)
			fprintf(f, " %g", m[i][j]);
		fprintf(f, "\n");
	}
}

Matrix
InputMatrix(f)
FILE *f;
{
	register Matrix m;
	register int i, j;
	char check[4];
	int nrows, ncols;

	if(fscanf(f, "%1s %d %d", check, &nrows, &ncols) != 3) {
		exit_error("InputMatrix fscanf 1");
	    }
	if(check[0] != 'M') {
		exit_error("InputMatrix check");
	    }
	m = NewMatrix(nrows, ncols);
	for(i = 0; i < nrows; i++)
            for(j = 0; j < ncols; j++) {
			if(fscanf(f, "%lf", &m[i][j]) != 1) {
				exit_error("InputMatrix fscanf 2");
			    }
            }

	return m;
}

double
InvertSingularMatrix(m, inv)
Matrix m, inv;
{
	register int i, j, k;
	BitVector mask;
	Matrix sm;
	double det, maxdet;
	int mi = -1, mj = -1, mk = -1;

	maxdet = 0.0;
	for(i = 0; i < NROWS(m); i++) {
		printf("r&c%d, ", i); 
		SET_BIT_VECTOR(mask);
		BIT_CLEAR(i, mask);
		sm = SliceMatrix(m, mask, mask);
		det = InvertMatrix(sm, sm);
		if(det == 0.0)
			printf("det still 0\n");
		else {
			printf("det = %g\n", det);
		}
		if(_abs(det) > _abs(maxdet))
			maxdet = det, mi = i;
		FreeMatrix(sm);
	}
	printf("\n");

	printf("maxdet=%g when row %d left out\n", maxdet, mi);
	if(fabs(maxdet) > 1.0e-6) {
		goto found;
	}

	maxdet = 0.0;
	for(i = 0; i < NROWS(m); i++) {
	     for(j = i+1; j < NROWS(m); j++) {
		/* printf("leaving out row&col %d&%d, ", i, j); */
		SET_BIT_VECTOR(mask);
		BIT_CLEAR(i, mask);
		BIT_CLEAR(j, mask);
		sm = SliceMatrix(m, mask, mask);
		det = InvertMatrix(sm, sm);
		/*
		if(det == 0.0)
			printf("det still 0\n");
		else {
			printf("det = %g\n", det);
		}
		*/
		if(abs(det) > abs(maxdet))
			maxdet = det, mi = i, mj = j;
		FreeMatrix(sm);
	    }
	}

	printf("maxdet=%g when rows %d,%d left out\n", maxdet, mi, mj);
	if(_abs(maxdet) > 1.0e-6) {
		goto found;
	}

	maxdet = 0.0;
	for(i = 0; i < NROWS(m); i++) {
	   for(j = i+1; j < NROWS(m); j++) {
	      for(k = j+1; k < NROWS(m); k++) {
		/* printf("leaving out row&col %d,%d&%d, ", i, j, k); */
		SET_BIT_VECTOR(mask);
		BIT_CLEAR(i, mask);
		BIT_CLEAR(j, mask);
		BIT_CLEAR(k, mask);
		sm = SliceMatrix(m, mask, mask);
		det = InvertMatrix(sm, sm);
		/*
		if(det == 0.0)
			printf("det still 0\n");
		else {
			printf("det = %g\n", det);
		}
		*/
		if(_abs(det) > _abs(maxdet))
			maxdet = det, mi = i, mj = j, mk = k;
		FreeMatrix(sm);
	      }
	   }
	}
	printf("maxdet=%g when rows %d,%d&%d left out\n", maxdet, mi, mj, mk);
	if(mk == -1)
		return 0.0;

found:

	SET_BIT_VECTOR(mask);
	if(mi >= 0) BIT_CLEAR(mi, mask);
	if(mj >= 0) BIT_CLEAR(mj, mask);
	if(mk >= 0) BIT_CLEAR(mk, mask);
	sm = SliceMatrix(m, mask, mask);
	det = InvertMatrix(sm, sm);
	DeSliceMatrix(sm, 0.0, mask, mask, inv);
	FreeMatrix(sm);
	PrintMatrix(inv, "desliced:\n");
	return det;
}

/* You can fairly confidently ignore the compiler warnings after here */

void
PrintVector(v, s,a1,a2,a3,a4,a5,a6,a7,a8)
register Vector v;
char *s; int a1,a2,a3,a4,a5,a6,a7,a8;
{
	register int i;
	printf(s,a1,a2,a3,a4,a5,a6,a7,a8);

	for(i = 0; i < NROWS(v); i++) printf(" %8.4f", v[i]);
	printf("\n");
}

void
PrintMatrix(m, s,a1,a2,a3,a4,a5,a6,a7,a8)
register Matrix m;
char *s; int a1,a2,a3,a4,a5,a6,a7,a8;
{
	register int i, j;
	printf(s,a1,a2,a3,a4,a5,a6,a7,a8);
	for(i = 0; i < NROWS(m);  i++) {
		for(j = 0; j < NCOLS(m); j++)
			printf(" %8.4f", m[i][j]);
		printf("\n");
	}
}

void
PrintArray(a, s,a1,a2,a3,a4,a5,a6,a7,a8)
Array a;
char *s; int a1,a2,a3,a4,a5,a6,a7,a8;
{
	switch(NDIMS(a)) {
	case 1: PrintVector((Vector) a, s,a1,a2,a3,a4,a5,a6,a7,a8); break;
	case 2: PrintMatrix((Matrix) a, s,a1,a2,a3,a4,a5,a6,a7,a8); break;
	default: error("PrintArray");
	}
}

