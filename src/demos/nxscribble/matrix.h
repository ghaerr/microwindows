/***********************************************************************

matrix.h - matrix operations

Copyright (C) 1991 Dean Rubine

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License. See ../COPYING for
the full agreement.

**********************************************************************/
/*
 
 Simple matrix operations
 Why I am writing this stuff over is beyond me

*/

/*

This package provides the Matrix and Vector data types

The difference between this matrix package and others is that:
	Vectors may be accessed as 1d arrays
	Matrices may still be accessed like two dimensional arrays
This is accomplished by putting a structure containing the bounds
of the matrix before the pointer to the (array of) doubles (in the
case of a Vector) or before the pointer to an (array of) pointers
to doubles (in the case of a Matrix).


Vectors and matrices are collectively called "arrays" herein.
*/

#define HEADER(a)	( ((struct array_header *) a) - 1 )

#define	NDIMS(a)	(int)(HEADER(a)->ndims)
#define NROWS(a)	(int)(HEADER(a)->nrows)
#define NCOLS(a)	(int)(HEADER(a)->ncols)
#define	ISVECTOR(a)	(NDIMS(a) == 1)
#define	ISMATRIX(a)	(NDIMS(a) == 2)

/* Note: this structure is prepended at the beginning of a Vector, and causes
   the Vector data type to be 32-byte aligned, but not 64-byte aligned.
   If this were a problem, filler could be filler[5] (or more) instead. 
   --Sharon Perl, 12/17/98. */

struct array_header {
	unsigned char	ndims;	/* 1 = vector, 2 = matrix */
	unsigned char	nrows;
	unsigned char	ncols;
	unsigned char	filler;
};

typedef double **Matrix;
typedef double *Vector;

Vector	NewVector();	/* int r; (number of rows) */
Matrix	NewMatrix();	/* int r, c; (number of rows, number of columns) */
void	FreeVector();	/* Vector v; */
void	FreeMatrix();	/* Matrix m; */
void	PrintVector();	/* Vector v; char *fmt; any a1,a2,a3,a4,a5,a6,a7,a8 */
void	PrintMatrix();	/* Matrix m; char *fmt; any a1,a2,a3,a4,a5,a6,a7,a8 */
double	InnerProduct();	/* Vector v1, v2 */
void	MatrixMultiply(); /* Matrix m1, m2, prod; */
void	VectorTimesMatrix(); /* Vector v; Matrix m; Vector prod; */
void	ScalarTimesVector(); /* double s; Vector v; Vector prod; */
double	QuadraticForm(); /* Vector v; Matrix m; (computes v'mv) */
double	InvertMatrix();	/* Matrix input_matrix, result_matrix (returns det) */
Vector	SliceVector();	/* Vector v; BitVector rowmask */
Matrix	SliceMatrix();	/* Matrix m; Bitvector rowmask, colmask; */
Vector	VectorCopy();	/* Vector v; */
Matrix	MatrixCopy();	/* Matrix m; */
Vector	InputVector();	/* FILE *f; */
Matrix	InputMatrix();	/* FILE *f; */

double	InvertSingularMatrix();	/* Matrix input, result (returns det) */
Matrix  DeSliceMatrix(); /* Matrix m, double fill, BitVector rowmask, colmask;
				Matrix result */
void	OutputVector();
void	OutputMatrix();
void	ZeroVector();
void	ZeroMatrix();	/* Matrix m; */
void	FillMatrix();	/* Matrix m; double fill; */
