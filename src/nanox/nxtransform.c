/*
 * Nano-X touchscreen transform routines 
 *
 * These routines provide a uniform set of routines for
 * handling transforms of the raw data from a touchscreen.  
 *
 * Copyright (C) 2003, Jordan Crouse <jordan@cosmicpenguin.net>
 *
 * NOTE: these routines pull in floating point.
 */
#include <stdio.h>
#include <string.h>
#include "nano-X.h"

static int CalcTransformationCoefficientsBetter(GR_CAL_DATA * samples,
	GR_TRANSFORM * ptc);

int
GrCalcTransform(GR_CAL_DATA * data, GR_TRANSFORM * trans)
{
	return CalcTransformationCoefficientsBetter(data, trans);
}

/* Provide a standard way for saving the transform data in a file */
int
GrSaveTransformData(GR_TRANSFORM * trans, char *filename)
{
	FILE *out = fopen(filename, "w");
	if (!out)
		return -1;

	fprintf(out, "# Nano-X touchscreen data\n");
	fprintf(out, "# Generated automatically, do not edit\n\n");

	fprintf(out, "%d %d %d %d %d %d %d\n",
		trans->a, trans->b, trans->c,
		trans->d, trans->e, trans->f, trans->s);

	fprintf(out, "\n# ---\n");
	fclose(out);

	return 0;
}

int
GrLoadTransformData(char *filename, GR_TRANSFORM * trans)
{
	int ret = 0;
	FILE *in = 0;

	if (!trans)
		return -1;
	memset(trans, '\0', sizeof(GR_TRANSFORM));

	in = fopen(filename, "r");
	if (!in)
		return -1;

	while (!feof(in) && !ferror(in)) {
		char buffer[128];
		if (!fgets(buffer, sizeof(buffer) - 1, in))
			continue;

		if (buffer[0] == '#')
			continue;

		if (sscanf(buffer, "%d %d %d %d %d %d %d",
			   &trans->a, &trans->b, &trans->c,
			   &trans->d, &trans->e, &trans->f, &trans->s) == 7)
			goto exit_load;
	}

	ret = -1;
exit_load:

	fclose(in);
	return ret;
}

/* *** Calibration routines *****/

/* 
   Note:  These have had an interesting history.  They were originally
   written by Bradley LaRonde for the tpcal program that is part of 
   Microwindows, and then it was ported to PicoGUI, and now its back
   again, cleaned up and ready for service.
*/

static int
CalcTransformationCoefficientsBetter(GR_CAL_DATA * samples,
	GR_TRANSFORM * ptc)
{
	/*
	 * Janus (the man) <janus@place.org> came up with a much better way
	 * to figure the coefficients.  His original algorithm was written in MOO.
	 * Jay Carlson <> did the translation to C.
	 * This way takes into account inter-axis dependency like rotation and skew.
	 */
	int i, j, r, k;
	double vector[3][2];
	double matrix[3][3];
	double p, q;

	vector[0][0] = 0.0;           vector[0][1] = 0.0;
	vector[1][0] = samples->xres; vector[1][1] = 0.0;
	vector[2][0] = samples->xres; vector[2][1] = samples->yres;
	
	matrix[0][0] = samples->minx; matrix[0][1] = samples->miny; matrix[0][2] = 1.0;
	matrix[1][0] = samples->maxx; matrix[1][1] = samples->miny; matrix[1][2] = 1.0;
	matrix[2][0] = samples->maxx; matrix[2][1] = samples->maxy; matrix[2][2] = 1.0;
	
	for (i = 0; i < 3; i++) {
		p = matrix[i][i];

		for (j = 0; j < 3; j++) {
			matrix[i][j] = matrix[i][j] / p;
		}

		for (j = 0; j < 2; j++) {
			vector[i][j] = vector[i][j] / p;
		}

		for (r = 0; r < 3; r++) {
			if (r != i) {
				q = matrix[r][i];

				matrix[r][i] = 0.0;

				for (k = i + 1; k < 3; k++) {
					matrix[r][k] =
						matrix[r][k] -
						(q * matrix[i][k]);
				}

				for (k = 0; k < 2; k++) {
					vector[r][k] =
						vector[r][k] -
						(q * vector[i][k]);
				}
			}
		}
	}

	ptc->s = 1 << 16;
	ptc->a = vector[0][0] * ptc->s;
	ptc->b = vector[1][0] * ptc->s;
	ptc->c = vector[2][0] * ptc->s;
	ptc->d = vector[0][1] * ptc->s;
	ptc->e = vector[1][1] * ptc->s;
	ptc->f = vector[2][1] * ptc->s;

	return 0;
}
