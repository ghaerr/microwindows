/***********************************************************************

sc.c - creates classifiers from feature vectors of examples, as well as
   classifying example feature vectors.

Copyright (C) 1991 Dean Rubine

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (in ../COPYING); if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***********************************************************************/


#include "bitvector.h"
#include "matrix.h"
#include "util.h"
#include "sc.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "zdebug.h"

#define	EPS	(1.0e-6)	/* for singular matrix check */
sClassifier
sNewClassifier()
{
	register sClassifier sc = allocate(1, struct sclassifier);
	sc->nfeatures = -1;
	sc->nclasses = 0;
	sc->classdope = allocate(MAXSCLASSES, sClassDope);
	sc->w = NULL;
	return sc;
}

void
sFreeClassifier(sc)
register sClassifier sc;
{
	register int i;
	register sClassDope scd;

	for(i = 0; i < sc->nclasses; i++) {
		scd = sc->classdope[i];
		if(scd->name) free(scd->name);
		if(scd->sumcov) FreeMatrix(scd->sumcov);
		if(scd->average) FreeVector(scd->average);
		free(scd);
		if(sc->w && sc->w[i]) FreeVector(sc->w[i]);
	}
	free(sc->classdope);
	if(sc->w) free(sc->w);
	if(sc->cnst) FreeVector(sc->cnst);
	if(sc->invavgcov) FreeMatrix(sc->invavgcov);
	free(sc);
}

sClassDope
sClassNameLookup(sc, classname)
register sClassifier sc;
register char *classname;
{
	register int i;
	register sClassDope scd;
	static sClassifier lastsc = NULL;
	static sClassDope lastscd = NULL;

	/* quick check for last class name */
	if(lastsc == sc && lastscd != NULL && STREQ(lastscd->name, classname))
		return lastscd;

	/* linear search through all classes for name */
	for(i = 0; i < sc->nclasses; i++) {
		scd = sc->classdope[i];
		if(STREQ(scd->name, classname))
			return lastsc = sc, lastscd = scd;
	}
	lastsc = NULL;
	lastscd = NULL;
	return NULL;
}

static sClassDope
sAddClass(sc, classname)
register sClassifier sc;
char *classname;
{
	register sClassDope scd;

	sc->classdope[sc->nclasses] = scd = allocate(1, struct sclassdope);
	scd->name = scopy(classname);
	scd->number = sc->nclasses;
	scd->nexamples = 0;
	scd->sumcov = NULL;
	++sc->nclasses;
	return scd;
}

void
sAddExample(sc, classname, y)
register sClassifier sc;
char *classname;
Vector y;
{
	register sClassDope scd;
	register int i, j;
	double nfv[50];
	double nm1on, recipn;

	scd = sClassNameLookup(sc, classname);
	if(scd == NULL) {
/* fprintf(stderr, "sAddExample: calling sAddClass on %s.\n", classname); */
		scd = sAddClass(sc, classname);
	      }

	if(sc->nfeatures == -1) {
		sc->nfeatures = NROWS(y);
/*		fprintf(stderr, "sAddExample: setting sc->nfeatures to NROWS(y).\n"); */
	      }

	if(scd->nexamples == 0) {
/* 		fprintf(stderr, "sAddExample: allocating  & zeroing scd->average & scd->sumcov.\n"); */
		scd->average = NewVector(sc->nfeatures);
		ZeroVector(scd->average);
		scd->sumcov = NewMatrix(sc->nfeatures, sc->nfeatures);
		ZeroMatrix(scd->sumcov);

	}

	if(sc->nfeatures != NROWS(y)) {
		PrintVector(y, "sAddExample: funny feature vector nrows!=%d", 
			sc->nfeatures);
		return;
	}

	scd->nexamples++;
	nm1on = ((double) scd->nexamples-1)/scd->nexamples;
	recipn = 1.0/scd->nexamples;

	/* incrementally update covariance matrix */
        for(i = 0; i < sc->nfeatures; i++)
		nfv[i] = y[i] - scd->average[i];

	/* only upper triangular part computed */
        for(i = 0; i < sc->nfeatures; i++)
	   for(j = i; j < sc->nfeatures; j++)
		scd->sumcov[i][j] += nm1on * nfv[i] * nfv[j];

	/* incrementally update mean vector */
	for(i = 0; i < sc->nfeatures; i++)
		scd->average[i] = nm1on * scd->average[i] + recipn * y[i];

}

void
sDoneAdding(sc)
register sClassifier sc;
{
	register int i, j;
	int c;
	int ne, denom;
	double oneoverdenom;
	register Matrix s;
	register Matrix avgcov;
	double det;
	register sClassDope scd;

	if(sc->nclasses == 0) {
		error("No classes for adding to classifier");
		return;
	    }

	/* Given covariance matrices for each class (* number of examples - 1)
	    compute the average (common) covariance matrix */

	avgcov = NewMatrix(sc->nfeatures, sc->nfeatures);
	ZeroMatrix(avgcov);
	ne = 0;
	for(c = 0; c < sc->nclasses; c++) {
		scd = sc->classdope[c];
		ne += scd->nexamples;
		s = scd->sumcov;
		for(i = 0; i < sc->nfeatures; i++)
			for(j = i; j < sc->nfeatures; j++)
				avgcov[i][j] += s[i][j]; 
	}

	denom = ne - sc->nclasses;
	if(denom <= 0) {
	    error("Number of classes must be less than number of examples");
	    return;
	}

	oneoverdenom = 1.0 / denom;
	for(i = 0; i < sc->nfeatures; i++)
		for(j = i; j < sc->nfeatures; j++)
			avgcov[j][i] = avgcov[i][j] *= oneoverdenom;

	Z('a') PrintMatrix(avgcov, "Average Covariance Matrix\n");
	/* invert the avg covariance matrix */

	sc->invavgcov = NewMatrix(sc->nfeatures, sc->nfeatures);
	det = InvertMatrix(avgcov, sc->invavgcov);
	if(fabs(det) <= EPS)
		FixClassifier(sc, avgcov);
	
	/* now compute discrimination functions */
	sc->w = allocate(sc->nclasses, Vector);
	sc->cnst = NewVector(sc->nclasses);
	for(c = 0; c < sc->nclasses; c++) {
		scd = sc->classdope[c];
		sc->w[c] = NewVector(sc->nfeatures);
		VectorTimesMatrix(scd->average, sc->invavgcov, sc->w[c]);
		sc->cnst[c] = -0.5 * InnerProduct(sc->w[c], scd->average);
		/* could add log(priorprob class c) to cnst[c] */
	}

	FreeMatrix(avgcov);
	return;
}

sClassDope
sClassify(sc, fv) {
	return sClassifyAD(sc, fv, NULL, NULL);
}

sClassDope
sClassifyAD(sc, fv, ap, dp)
sClassifier sc;
Vector fv;
double *ap;
double *dp;
{
	double disc[MAXSCLASSES];
	register int i, maxclass;
	double denom, exp();
	register sClassDope scd;
	double d;

	if(sc->w == NULL) {
		error("%x not a trained classifier", sc);
		return(NULL);
       }

	for(i = 0; i < sc->nclasses; i++) {
/* ari */
	  double IP;
	  IP = InnerProduct(sc->w[i], fv);
/*	  fprintf(stderr, "sClassifyAD:  InnerProduct for class %s is %f.\n", sc->classdope[i]->name, IP); */
/*	  fprintf(stderr, "sClassifyAD:  sc->cnst[i] = %f.\n", sc->cnst[i]); */
	  disc[i] = IP + sc->cnst[i];
/*	  fprintf(stderr, "sClassifyAD:  Set disc = %f for class %s.\n", disc[i],sc->classdope[i]->name); */
	    
/*	  disc[i] = InnerProduct(sc->w[i], fv) + sc->cnst[i]; */
	}

	maxclass = 0;
	for(i = 1; i < sc->nclasses; i++)
		if(disc[i] > disc[maxclass])
			maxclass = i;

/* ari */
/* PF_INIT_COS	0	 initial angle (cos)                         */
/* PF_INIT_SIN	1	 initial angle (sin)                         */
/* PF_BB_LEN	2	 length of bounding box diagonal             */
/* PF_BB_TH	3	 angle of bounding box diagonal              */
/* PF_SE_LEN	4	 length between start and end points         */
/* PF_SE_COS	5	 cos of angle between start and end points   */
/* PF_SE_SIN	6	 sin of angle between start and end points   */
/* PF_LEN	7	 arc length of path                          */
/* PF_TH	8	 total angle traversed                       */
/* PF_ATH	9	 sum of abs vals of angles traversed         */
/* PF_SQTH	10	 sum of squares of angles traversed          */
/* PF_DUR	11	 duration of path                            */ 
/* ifndef USE_TIME                                                   */
/* 	NFEATURES	12                                           */
/* else                                                              */
/* 	PF_MAXV		12	   maximum speed                     */
/* 	NFEATURES	13                                           */
/* endif                                                             */

/*
* fprintf(stderr, "\nFeature vector:\n");
* fprintf(stderr, "    start cosine      %8.4f    path length       %8.4f\n",
* 	fv[PF_INIT_COS], fv[PF_LEN]);
* fprintf(stderr, "    start sine        %8.4f    total angle       %8.4f\n",
* 	fv[PF_INIT_SIN], fv[PF_TH]);
* fprintf(stderr, "    b.b. length       %8.4f    total abs. angle  %8.4f\n",
* 	fv[PF_BB_LEN], fv[PF_ATH]);
* fprintf(stderr, "    b.b. angle        %8.4f    total sq. angle   %8.4f\n",
* 	fv[PF_BB_TH], fv[PF_SQTH]);
* fprintf(stderr, "    st-end length     %8.4f    duration          %8.4f\n",
* 	fv[PF_SE_LEN], fv[PF_DUR]);
* fprintf(stderr, "    st-end cos        %8.4f\n", fv[PF_SE_COS]);
* fprintf(stderr, "    st-end sin        %8.4f\n", fv[PF_SE_SIN]);
*/
 	ZZ('C') {
		scd = sc->classdope[maxclass];
		PrintVector(fv, "%10.10s  ", scd->name);
		ZZZ('C') {
			for(i = 0; i < sc->nclasses; i++) {
				scd = sc->classdope[i];
				PrintVector(scd->average, "%5.5s %5g ",
					scd->name, disc[i]);
			}
		}
	}

	scd = sc->classdope[maxclass];
/* ari */
/* fprintf(stderr,"%s", scd->name); */
/*
   fprintf(stderr,"Stroke identified as %s [ ", scd->name);
   for (i = 0; i < sc->nclasses; i++) {
      if ( (disc[maxclass] - disc[i] < 5.0) && (i != maxclass) )
         fprintf(stderr,"%s ", sc->classdope[i]->name);
   }
   fprintf(stderr,"], ");
*/
	if(ap) {	/* calculate probability of non-ambiguity */
		for(denom = 0, i = 0; i < sc->nclasses; i++)
			/* quick check to avoid computing negligible term */
			if((d = disc[i] - disc[maxclass]) > -7.0)
				denom += exp(d);
		*ap = 1.0 / denom;
	}

	if(dp) 	/* calculate distance to mean of chosen class */
		*dp = MahalanobisDistance(fv, scd->average, sc->invavgcov);

	return scd;
}

/*
 Compute (v-u)' sigma (v-u)
 */

double
MahalanobisDistance(v, u, sigma)
register Vector v, u;
register Matrix sigma;
{
	register int i;
	static Vector space;
	double result;

	if(space == NULL || NROWS(space) != NROWS(v)) {
		if(space) FreeVector(space);
		space = NewVector(NROWS(v));
	}
	for(i = 0; i < NROWS(v); i++)
		space[i] = v[i] - u[i];
	result =  QuadraticForm(space, sigma);
	return result;
}

void
FixClassifier(sc, avgcov)
register sClassifier sc;
Matrix avgcov;
{
	int i;
	double det;
	BitVector bv;
	Matrix m, r;

	/* just add the features one by one, discarding any that cause
	   the matrix to be non-invertible */

	CLEAR_BIT_VECTOR(bv);
	for(i = 0; i < sc->nfeatures; i++) {
		BIT_SET(i, bv);
		m = SliceMatrix(avgcov, bv, bv);
		r = NewMatrix(NROWS(m), NCOLS(m));
		det = InvertMatrix(m, r);
		if(fabs(det) <= EPS)
			BIT_CLEAR(i, bv);
		FreeMatrix(m);
		FreeMatrix(r);
	}

	m = SliceMatrix(avgcov, bv, bv);
	r = NewMatrix(NROWS(m), NCOLS(m));
	det = InvertMatrix(m, r);
	if(fabs(det) <= EPS) {
		error("Can't fix classifier!");
		return;
	    }
	DeSliceMatrix(r, 0.0, bv, bv, sc->invavgcov);

	FreeMatrix(m);
	FreeMatrix(r);

}

void
sDumpClassifier(sc)
register sClassifier sc;
{
	register sClassIndex c;

	printf("\n----Classifier %x, %d features:-----\n", (int)sc, sc->nfeatures);
	printf("%d classes: ", sc->nclasses);
	for(c = 0; c < sc->nclasses; c++)
		printf("%s  ", sc->classdope[c]->name);
	printf("Discrimination functions:\n");
	for(c = 0; c < sc->nclasses; c++) {
		PrintVector(sc->w[c], "%s: %g + ", sc->classdope[c]->name, sc->cnst[c]);
		printf("Mean vectors:\n");
		PrintVector(sc->classdope[c]->average, "%s: ", sc->classdope[c]->name);
	    }
	if( sc->invavgcov != NULL ) {
	    PrintMatrix(sc->invavgcov, "Inverse average covariance matrix:\n");
	}
	printf("\n---------\n\n");
}

void
sWrite(outfile, sc)
FILE *outfile;
sClassifier sc;
{
	int i;
	register sClassDope scd;

	fprintf(outfile, "%d classes\n", sc->nclasses);
	for(i = 0; i < sc->nclasses; i++) {
		scd = sc->classdope[i];
		fprintf(outfile, "%s\n", scd->name);
	}
	for(i = 0; i < sc->nclasses; i++) {
		scd = sc->classdope[i];
		OutputVector(outfile, scd->average);
		OutputMatrix(outfile, scd->sumcov);
		OutputVector(outfile, sc->w[i]);
	}
	OutputVector(outfile, sc->cnst);
	OutputMatrix(outfile, sc->invavgcov);
}

sClassifier
sRead(infile)
FILE *infile;
{
	int i, n;
	register sClassifier sc;
	register sClassDope scd;
	char buf[100];

	
	Z('a') printf("Reading classifier \n");
	sc = sNewClassifier();
	fgets(buf, 100, infile);
	if(sscanf(buf, "%d", &n) != 1) {
	    error("Input error in classifier file");
	    sFreeClassifier(sc);
	    return(NULL);
	}
	Z('a') printf("  %d classes \n", n);
	for(i = 0; i < n; i++) {
		fscanf(infile, "%s", buf);
		scd = sAddClass(sc, buf);
		Z('a') printf("  %s \n", scd->name);
	}
	sc->w = allocate(sc->nclasses, Vector);
	for(i = 0; i < sc->nclasses; i++) {
		scd = sc->classdope[i];
		scd->average = InputVector(infile);
		scd->sumcov = InputMatrix(infile);
		sc->w[i] = InputVector(infile);
	}
	sc->cnst = InputVector(infile);
	sc->invavgcov = InputMatrix(infile);
	Z('a') printf("\n");
	return sc;
}


void
sDistances(sc, nclosest)
register sClassifier sc;
{
	register Matrix d = NewMatrix(sc->nclasses, sc->nclasses);
	register int i, j;
	double min, max = 0;
	int n, mi, mj;

	printf("-----------\n");
	printf("Computing %d closest pairs of classes\n", nclosest);
	for(i = 0; i < NROWS(d); i++) {
		for(j = i+1; j < NCOLS(d); j++) {
			d[i][j] = MahalanobisDistance(
						sc->classdope[i]->average,
						sc->classdope[j]->average,
						sc->invavgcov);
			if(d[i][j] > max) max = d[i][j];
		}
	}

	for(n = 1; n <= nclosest; n++) {
		min = max;
		mi = mj = -1;
		for(i = 0; i < NROWS(d); i++) {
			for(j = i+1; j < NCOLS(d); j++) {
				if(d[i][j] < min)
					min = d[mi=i][mj=j];
			}
		}
		if(mi == -1)
			break;

		printf("%2d) %10.10s to %10.10s d=%g nstd=%g\n",
			n,
			sc->classdope[mi]->name,
			sc->classdope[mj]->name,
			d[mi][mj],
			sqrt(d[mi][mj]));

		d[mi][mj] = max+1;
	}
	printf("-----------\n");
	FreeMatrix(d);
}
