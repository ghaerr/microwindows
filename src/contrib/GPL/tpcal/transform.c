/*
 * transform.c
 * Calculate coefficients for tranformation equation
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
#include "windows.h"
#include "mou_tp.h"
#include "transform.h"

int CalcTransformationCoefficientsSimple(CALIBRATION_PAIRS *pcp, TRANSFORMATION_COEFFICIENTS *ptc)
{
	/*
	 * This is my simple way of calculating some of the coefficients -
	 * enough to do a simple scale and translate.
	 * It ignores any dependencies between axiis (rotation and/or skew).
	 */

	int min_screen_x = pcp->ul.screen.x;
	int min_screen_y = pcp->ul.screen.y;
	int max_screen_x = pcp->lr.screen.x;
	int max_screen_y = pcp->lr.screen.y;

	int min_device_x = pcp->ul.device.x;
	int min_device_y = pcp->ul.device.y;
	int max_device_x = pcp->lr.device.x;
	int max_device_y = pcp->lr.device.y;

	ptc->s = (1 << 16);
	ptc->a = ptc->s * (min_screen_x - max_screen_x) / (min_device_x - max_device_x);
	ptc->b = 0;
	ptc->c = (ptc->a * -min_device_x) + (ptc->s * min_screen_x);
	ptc->d = 0;
	ptc->e = ptc->s * (min_screen_y - max_screen_y) / (min_device_y - max_device_y);
	ptc->f = (ptc->e * -min_device_y) + (ptc->s * min_screen_y);

	return 0;
}

int CalcTransformationCoefficientsBetter(CALIBRATION_PAIRS *pcp, TRANSFORMATION_COEFFICIENTS *ptc)
{
	/*
	 * Janus (the man) <janus@place.org> came up with a much better way
	 * to figure the coefficients.  His original algorithm was written in MOO.
	 * Jay Carlson <> did the translation to C.
	 * This way takes into account inter-axis dependency like rotation and skew.
	 */

	double vector[3][2] =
	{
		{pcp->ul.screen.x, pcp->ul.screen.y},
		{pcp->ur.screen.x, pcp->ur.screen.y},
		{pcp->lr.screen.x, pcp->lr.screen.y}
	};

	double matrix[3][3] =
	{
		{pcp->ul.device.x, pcp->ul.device.y, 1.0},
		{pcp->ur.device.x, pcp->ur.device.y, 1.0},
		{pcp->lr.device.x, pcp->lr.device.y, 1.0}
	};

	int i, j, r, k;
	double p, q;
    
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
                         matrix[r][k] = matrix[r][k] - (q * matrix[i][k]);
                    }
                    
                    for (k = 0; k < 2; k++) {
                         vector[r][k] = vector[r][k] - (q * vector[i][k]);
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

int CalcTransformationCoefficientsEvenBetter(CALIBRATION_PAIRS *pcp, TRANSFORMATION_COEFFICIENTS *ptc)
{
	/*
	 * Mike Klar <> added the an xy term to correct for trapezoidial distortion.
	 */

	double vector[4][2] =
	{
		{pcp->ul.screen.x, pcp->ul.screen.y},
		{pcp->ur.screen.x, pcp->ur.screen.y},
		{pcp->lr.screen.x, pcp->lr.screen.y},
		{pcp->ll.screen.x, pcp->ll.screen.y}
	};

	double matrix[4][4] =
	{
		{pcp->ul.device.x, pcp->ul.device.x * pcp->ul.device.y, pcp->ul.device.y, 1.0},
		{pcp->ur.device.x, pcp->ur.device.x * pcp->ur.device.y, pcp->ur.device.y, 1.0},
		{pcp->lr.device.x, pcp->lr.device.x * pcp->lr.device.y, pcp->lr.device.y, 1.0},
		{pcp->ll.device.x, pcp->ll.device.x * pcp->ll.device.y, pcp->ll.device.y, 1.0}
	};

	int i, j, r, k;
	double p, q;
    
	for (i = 0; i < 4; i++) {
          p = matrix[i][i];
          
          for (j = 0; j < 4; j++) {
               matrix[i][j] = matrix[i][j] / p;
          }
          
          for (j = 0; j < 2; j++) {
               vector[i][j] = vector[i][j] / p;
          }

          for (r = 0; r < 4; r++) {
               if (r != i) {
                    q = matrix[r][i];
                    
                    matrix[r][i] = 0.0;
                    
                    for (k = i + 1; k < 4; k++) {
                         matrix[r][k] = matrix[r][k] - (q * matrix[i][k]);
                    }
                    
                    for (k = 0; k < 2; k++) {
                         vector[r][k] = vector[r][k] - (q * vector[i][k]);
                    }
               }
          }
	}

	/* I just drop the xy coefficient since it is so small. */
	ptc->s = 1 << 16;
	ptc->a = vector[0][0] * ptc->s;
	ptc->b = vector[2][0] * ptc->s;
	ptc->c = vector[3][0] * ptc->s;
	ptc->d = vector[0][1] * ptc->s;
	ptc->e = vector[2][1] * ptc->s;
	ptc->f = vector[3][1] * ptc->s;

	return 0;
}

int CalcTransformationCoefficientsBest(CALIBRATION_PAIRS *pcp, TRANSFORMATION_COEFFICIENTS *ptc)
{
	/*
	 * Mike Klar <> came up with a best-fit solution that works best.
	 */

	const int first_point = 0;
	const int last_point = 4;
	int i;

	double Sx=0, Sy=0, Sxy=0, Sx2=0, Sy2=0, Sm=0, Sn=0, Smx=0, Smy=0, Snx=0, Sny=0, S=0;
	double t1, t2, t3, t4, t5, t6, q;

	/* cast the struct to an array - hacky but ok */
	CALIBRATION_PAIR *cp = (CALIBRATION_PAIR*)pcp;

	/*
	 * Do a best-fit calculation for as many points as we want, as
	 * opposed to an exact fit, which can only be done against 3 points.
	 *
	 * The following calculates various sumnations of the sample data
	 * coordinates.  For purposes of naming convention, x and y
	 * refer to device coordinates, m and n refer to screen
	 * coordinates, S means sumnation.  x2 and y2 are x squared and
	 * y squared, S by itself is just a count of points (= sumnation
	 * of 1).
	 */

	for (i = first_point; i < last_point + 1; i++) {
		Sx += cp[i].device.x;
		Sy += cp[i].device.y;
		Sxy += cp[i].device.x * cp[i].device.y;
		Sx2 += cp[i].device.x * cp[i].device.x;
		Sy2 += cp[i].device.y * cp[i].device.y;
		Sm += cp[i].screen.x;
		Sn += cp[i].screen.y;
		Smx += cp[i].screen.x * cp[i].device.x;
		Smy += cp[i].screen.x * cp[i].device.y;
		Snx += cp[i].screen.y * cp[i].device.x;
		Sny += cp[i].screen.y * cp[i].device.y;
		S += 1;
	}

#if 0
	printf("%f, %f, %f, %f, "
	       "%f, %f, %f, %f, "
	       "%f, %f, %f, %f\n",
	        Sx, Sy, Sxy, Sx2, Sy2, Sm, Sn, Smx, Smy, Snx, Sny, S);
#endif

	/*
	 * Next we solve the simultaneous equations (these equations minimize
	 * the sum of the square of the m and n error):
	 *
	 *    | Sx2 Sxy Sx |   | a d |   | Smx Snx |
	 *    | Sxy Sy2 Sy | * | b e | = | Smy Sny |
	 *    | Sx  Sy  S  |   | c f |   | Sm  Sn  |
	 *
	 * We could do the matrix solution in code, but that leads to several
	 * divide by 0 conditions for cases where the data is truly solvable
	 * (becuase those terms cancel out of the final solution), so we just
	 * give the final solution instread.  t1 through t6 and q are just
	 * convenience variables for terms that are used repeatedly - we could
	 * calculate each of the coefficients directly at this point with a
	 * nasty long equation, but that would be extremly inefficient.
	 */

	t1 = Sxy * Sy - Sx * Sy2;
	t2 = Sxy * Sx - Sx2 * Sy;
	t3 = Sx2 * Sy2 - Sxy * Sxy;
	t4 = Sy2 * S - Sy * Sy;
	t5 = Sx * Sy - Sxy * S;
	t6 = Sx2 * S - Sx * Sx;

	q = t1 * Sx + t2 * Sy + t3 * S;

	/*
	 * If q = 0, then the data is unsolvable.  This should only happen
	 * when there are not enough unique data points (less than 3 points
	 * will give infinite solutions), or at least one of the 
	 * coefficients is infinite (which would indicate that the same
	 * device point represents an infinite area of the screen, probably
	 * as a result of the same device data point given for 2 different
	 * screen points).  The first condition should never happen, since
	 * we're always feeding in at least 3 unique screen points.  The
	 * second condition would probably indicate bad user input or the
	 * touchpanel device returning bad data.
	 */

	if (q == 0)
		return -1;

	ptc->s = 1 << 16;
	ptc->a = ((t4 * Smx + t5 * Smy + t1 * Sm) / q + 0.5/65536) * ptc->s;
	ptc->b = ((t5 * Smx + t6 * Smy + t2 * Sm) / q + 0.5/65536) * ptc->s;
	ptc->c = ((t1 * Smx + t2 * Smy + t3 * Sm) / q + 0.5/65536) * ptc->s;
	ptc->d = ((t4 * Snx + t5 * Sny + t1 * Sn) / q + 0.5/65536) * ptc->s;
	ptc->e = ((t5 * Snx + t6 * Sny + t2 * Sn) / q + 0.5/65536) * ptc->s;
	ptc->f = ((t1 * Snx + t2 * Sny + t3 * Sn) / q + 0.5/65536) * ptc->s;

	/*
	 * Finally, we check for overflow on the fp to integer conversion,
	 * which would also probably indicate bad data.
	 */

	if ( (ptc->a == 0x80000000) || (ptc->b == 0x80000000) ||
	     (ptc->c == 0x80000000) || (ptc->d == 0x80000000) ||
	     (ptc->e == 0x80000000) || (ptc->f == 0x80000000) )
		return -1;
	
	return 0;
}

