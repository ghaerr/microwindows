/***********************************************************************

sc.h - creates classifiers from feature vectors of examples, as well as
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


/*
 single path classifier
*/

#ifndef _SC_H_

#define _SC_H_

#define	MAXSCLASSES	100

typedef struct sclassifier *sClassifier; /* single feature-vector classifier */
typedef int		    sClassIndex; /* single feature-vector class index */
typedef struct sclassdope  *sClassDope;	 /* single feature-vector class dope */

struct sclassdope {
	char 		*name;
	sClassIndex	number;
	Vector		average;
	Matrix		sumcov;
	int		nexamples;
};

struct sclassifier {
	int		nfeatures;
	int		nclasses;
	sClassDope	*classdope;

	Vector		cnst;	/* constant term of discrimination function */
	Vector		*w;	/* array of coefficient weights */
	Matrix		invavgcov; /* inverse covariance matrix */
};

sClassifier	sNewClassifier();
sClassifier	sRead();		/* FILE *f */
void		sWrite();		/* FILE *f; sClassifier sc; */
void	 	sFreeClassifier();	/* sc */
void		sAddExample();		/* sc, char *classname; Vector y */
void		sRemoveExample();	/* sc, classname, y */
void		sDoneAdding();		/* sc */
sClassDope	sClassify();		/* sc, y */
sClassDope	sClassifyAD();		/* sc, y, double *ap; double *dp */
sClassDope	sClassNameLookup();	/* sc, classname */
double		MahalanobisDistance();	/* Vector v, u; Matrix sigma */
void		FixClassifier();
void		sDumpClassifier();
void		sDistances();

#endif
