/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is NanoScreenSaver.
 *
 * The Initial Developer of the Original Code is Alex Holden.
 * Portions created by Alex Holden are Copyright (C) 2000, 2002
 * Alex Holden <alex@alexholden.net>. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public license (the  "[GNU] License"), in which case the
 * provisions of [GNU] License are applicable instead of those
 * above.  If you wish to allow use of your version of this file only
 * under the terms of the [GNU] License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting  the provisions above and replace  them with the notice and
 * other provisions required by the [GNU] License.  If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the [GNU] License.
 */
/*
 * A collection of screen savers for Nano-X by Alex Holden.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nsaver.h"

void *my_malloc(size_t size)
{
	void *ret;

	if(!(ret = malloc(size))) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	return ret;
}

GR_COLOR get_random_colour(int min_brightness)
{
	int r, g, b;

	do {	
		r = RANDRANGE(0, 255);
		g = RANDRANGE(0, 255);
		b = RANDRANGE(0, 255);
	} while((r + g + b) / 3 < min_brightness);

	return(MWRGB(r, g, b));
}

void get_random_point_on_screen(nstate *state, GR_COORD *x, GR_COORD *y,
							GR_COLOR *c)
{
	if(x) *x = RANDRANGE(0, state->si.cols - 1);
	if(y) *y = RANDRANGE(0, state->si.rows - 1);
	if(c) *c = get_random_colour(GRP_MINBRIGHTNESS);
}

int not_square(int n)
{
	while(n) {
		if(n & 1) {
			if(n - 1) return 1;
			else return 0;
		}
		n >>= 1;
	}
	return 1;
}

void make_random_square(nstate *state, int min_brightness, int min_size,
			int max_size, int need_square, GR_COORD *retx,
			GR_COORD *rety, GR_SIZE *retsize)
{
	int x1, x2, y, size, tmp;
	GR_COLOR colour = get_random_colour(min_brightness); 

	GrSetGCForeground(state->main_gc, colour);

	do {
		x1 = RANDRANGE(0, state->si.cols);
		x2 = RANDRANGE(0, state->si.cols);
		if(x1 > x2) {
			tmp = x1;
			x1 = x2;
			x2 = tmp;
		}
		size = x2 - x1;
	} while(size < min_size || size > max_size ||
			(need_square && not_square(size)));

	y = RANDRANGE(0, state->si.rows - size - 1);

	*retx = x1;
	*rety = y;
	*retsize = size;
}

#ifdef HAVE_USLEEP
void msleep(long ms)
{
	usleep(ms * 1000);
}
#else
void msleep(long ms)
{
	struct timespec req, rem;

	req.tv_sec = ms / 1000000;
	req.tv_nsec = (ms % 1000000) * 1000000;

	while(nanosleep(&req, &rem) == -1) {
		if(errno == EINTR) {
			req.tv_sec = rem.tv_sec;
			req.tv_nsec = rem.tv_nsec;
			continue;
		} else {
			perror("nanosleep() failed");
			return;
		}
	}
}
#endif

GR_WINDOW_ID capture_screen(nstate *state)
{
	GR_WINDOW_ID pid;

#if CAPTURESCREEN_DELAY
	/* This is a hack to give the system enough time to redraw the screen
	 * before capturing it when we switch from one screensaver to another.
	 * without it, we often capture an only partially redrawn screen. */
	msleep(CAPTURESCREEN_DELAY);
#endif
	
	pid = GrNewPixmap(state->si.cols, state->si.rows, NULL);
	GrCopyArea(pid, state->main_gc, 0, 0, state->si.cols,
			state->si.rows, GR_ROOT_WINDOW_ID, 0, 0, 0);

	return pid;
}

void saver1_init(nstate *state) {}

void saver1_exposure(nstate *state)
{
	GrClearWindow(state->main_window, 0);
}

void saver1_animate(nstate *state) {}

void saver2_init(nstate *state)
{
	s2state *s = my_malloc(sizeof(s2state));
	state->priv = s;
	s->pixels = SAVER2_MAXPIXELS;
	state->animate_interval = SAVER2_DELAY;
}

void saver2_exposure(nstate *state)
{
	GrClearWindow(state->main_window, 0);
}

void saver2_animate(nstate *state)
{
	GR_COORD x, y;
	GR_COLOR c;
	int pixels = SAVER2_PIXELS_PER_FRAME;
	s2state *s = state->priv;

	while(pixels--) {
		if(!(s->pixels--)) {
			s->pixels = SAVER2_MAXPIXELS;
			GrClearWindow(state->main_window, 0);
		}
		get_random_point_on_screen(state, &x, &y, &c);
		GrSetGCForeground(state->main_gc, c);
		GrPoint(state->main_window, state->main_gc, x, y);
	}
}

void saver3_init(nstate *state)
{
	s3state *s = my_malloc(sizeof(s3state));
	state->priv = s;
	s->maxsegments = SAVER3_MAXSEGMENTS;
	s->lastx = 0;
	s->lasty = 0;
	state->animate_interval = SAVER3_DELAY;
}

void saver3_exposure(nstate *state)
{
	GrClearWindow(state->main_window, 0);
}

void saver3_animate(nstate *state)
{
	GR_COORD newx, newy;
	GR_COLOR c;
	s3state *s = state->priv;
	int pixels = SAVER3_SEGMENTS_PER_FRAME;

	while(pixels--) {
		if(!(s->maxsegments--)) {
			s->maxsegments = SAVER3_MAXSEGMENTS;
			GrClearWindow(state->main_window, 0);
		}
		get_random_point_on_screen(state, &newx, &newy, &c);
		GrSetGCForeground(state->main_gc, c);
		GrLine(state->main_window, state->main_gc, s->lastx, s->lasty,
							newx, newy);
		s->lastx = newx;
		s->lasty = newy;
	}
}

void saver4_init(nstate *state)
{
	int i;
	GR_COORD x, y;

	s4state *s = my_malloc(sizeof(s4state));
	state->priv = s;

	s->length = 0;

	for(i = 0; i < SAVER4_NUMWORMS; i++) {
		s->tip = 0;
		get_random_point_on_screen(state, &x, &y, &s->worms[i].colour);
		s->worms[i].x = x;
		s->worms[i].points[0].x = x;
		s->worms[i].y = y;
		s->worms[i].points[0].y = y;	
	}

	state->animate_interval = SAVER4_DELAY;
}

void saver4_exposure(nstate *state)
{
	int i;
	s4state *s = state->priv;

	GrClearWindow(state->main_window, 0);

	if(!s->length) return;

	for(i = 0; i < SAVER4_NUMWORMS; i++) {
		GrSetGCForeground(state->main_gc, s->worms[i].colour);
		GrPoints(state->main_window, state->main_gc, s->length,
							s->worms[i].points);
	}
}

void saver4_get_new_worm_position(nstate *state, int worm, int newtip)
{
	s4state *s = state->priv;
	s4worm *w = &s->worms[worm];

	w->d += FRANDRANGE(-SAVER4_MAXROTATION, SAVER4_MAXROTATION);
	w->x += cos(w->d) * SAVER4_VELOCITY;
	w->y += sin(w->d) * SAVER4_VELOCITY;
	if(w->x < 0) w->x = state->si.cols - 1 + w->x;
	if(w->x >= state->si.cols) w->x -= state->si.cols;
	if(w->y < 0) w->y = state->si.rows - 1 + w->y;
	if(w->y >= state->si.rows) w->y -= state->si.rows;
	w->points[newtip].x = (GR_COORD)w->x;
	w->points[newtip].y = (GR_COORD)w->y;
}

void saver4_animate(nstate *state)
{
	int i, tail, newtip;
	s4state *s = state->priv;

	if(s->length == SAVER4_WORMLENGTH) tail = s->tip + 1;
	else tail = 0;
	if(tail == SAVER4_WORMLENGTH) tail = 0;
	newtip = s->tip + 1;
	if(newtip == SAVER4_WORMLENGTH) newtip = 0;
	
	for(i = 0; i < SAVER4_NUMWORMS; i++) {
		GrSetGCForeground(state->main_gc, GR_COLOR_BLACK);
		GrFillRect(state->main_window, state->main_gc,
					s->worms[i].points[tail].x,
					s->worms[i].points[tail].y,
					SAVER4_WORMTHICKNESS,
					SAVER4_WORMTHICKNESS);
		saver4_get_new_worm_position(state, i, newtip);
		GrSetGCForeground(state->main_gc, s->worms[i].colour);
		GrFillRect(state->main_window, state->main_gc,
					s->worms[i].points[newtip].x,
					s->worms[i].points[newtip].y,
					SAVER4_WORMTHICKNESS,
					SAVER4_WORMTHICKNESS);
	}

	s->tip = newtip;
	if(s->length < SAVER4_WORMLENGTH) s->length++;
}

void saver5_init(nstate *state)
{
	int i;

	s5state *s = my_malloc(sizeof(s5state));
	state->priv = s;

	s->numstars = 0;

	for(i = 0; i < SAVER5_NUMSTARS; i++) {
		s->stars[i].angle = FRANDRANGE(0, (2 * M_PI));
		s->stars[i].pos = 1;
	}

	state->animate_interval = SAVER5_DELAY;
}

int saver5_drawstar(nstate *state, s5state *s, int star, int delete)
{
	int opp, adj;
	GR_COORD x, y;

	if(delete) GrSetGCForeground(state->main_gc, GR_COLOR_BLACK);
	else GrSetGCForeground(state->main_gc, GR_COLOR_WHITE);

	opp = (int)(sin(s->stars[star].angle) * s->stars[star].pos);
	adj = (int)(cos(s->stars[star].angle) * s->stars[star].pos);

	x = (state->si.cols / 2) + adj;
	y = (state->si.rows / 2) + opp;

	if((x < 0) || (y < 0) || (x >= state->si.cols) || (y >= state->si.rows))
		return 1;

	GrPoint(state->main_window, state->main_gc, x, y);

	return 0;
}

void saver5_exposure(nstate *state)
{
	int i;
	s5state *s = state->priv;

	GrClearWindow(state->main_window, 0);

	for(i = 0; i < SAVER5_NUMSTARS; i++) {
		saver5_drawstar(state, s, i, 0);
	}
}

void saver5_animate(nstate *state)
{
	int i;
	double position, scale, increment;
	s5state *s = state->priv;

	if(s->numstars < SAVER5_NUMSTARS) {
		s->numstars += SAVER5_STARS_INCREMENT;
		if(s->numstars > SAVER5_NUMSTARS)
			s->numstars = SAVER5_NUMSTARS;
	}

	for(i = 0; i < s->numstars; i++) {
		saver5_drawstar(state, s, i, 1);
		position = (double)s->stars[i].pos /
				(double)(state->si.cols / 2);
		scale = sin((position * M_PI_2) + M_PI + M_PI_2) + 1.0;
		increment = (scale * SAVER5_STARS_ACCEL_RATE) + 1;
		s->stars[i].pos += (int) increment;
		if(saver5_drawstar(state, s, i, 0)) {
			s->stars[i].pos = 1;
			s->stars[i].angle = FRANDRANGE(0, (2 * M_PI));
			saver5_drawstar(state, s, i, 0);
		}
	}
}

void saver6_init(nstate *state)
{
	int i, n;

	s6state *s = my_malloc(sizeof(s6state));
	state->priv = s;

	s->new_bolt_time = 0;

	for(i = 0; i < SAVER6_MAXBOLTS; i++) {
		s->bolts[i].duration = 0;
		for(n = 0; n < SAVER6_MAXFORKS; n++) {
			s->bolts[i].forks[n].valid = 0;
		}
	}

	state->animate_interval = SAVER6_DELAY;
}

void saver6_drawfork(nstate *state, s6state *s, int bolt, int fork, int delete)
{
	int i;

	if(delete) GrSetGCForeground(state->main_gc, GR_COLOR_BLACK);
	for(i = 0; i < SAVER6_THICKNESS; i++) {
		if(!delete) {
			if((i < 2) || (i >= SAVER6_THICKNESS - 2))
				 GrSetGCForeground(state->main_gc,
						 GR_COLOR_CORNFLOWERBLUE);
			else GrSetGCForeground(state->main_gc, GR_COLOR_WHITE);
		}
		GrPoly(state->main_window, state->main_gc,
				s->bolts[bolt].forks[fork].valid,
				s->bolts[bolt].forks[fork].vertices[i]);
	}
}

void saver6_drawbolt(nstate *state, s6state *s, int bolt, int delete)
{
	int n;

	for(n = 0; n < SAVER6_MAXFORKS; n++)
		if(s->bolts[bolt].forks[n].valid)
			saver6_drawfork(state, s, bolt, n, delete);
}

void saver6_drawlightning(nstate *state, s6state *s, int delete)
{
	int i;

	for(i = 0; i < SAVER6_MAXBOLTS; i++) {
		if(s->bolts[i].duration) {
			if(delete) s->bolts[i].duration--;
			saver6_drawbolt(state, s, i, delete);
		}
	}
}

void saver6_exposure(nstate *state)
{
	s6state *s = state->priv;

	GrClearWindow(state->main_window, 0);

	saver6_drawlightning(state, s, 0);
}

void saver6_setvertices(s6state *s, int bolt, int fork, int vert, GR_COORD x,
								GR_COORD y)
{
	int i;

	for(i = 0; i < SAVER6_THICKNESS; i++) {
		s->bolts[bolt].forks[fork].vertices[i][vert].x = x + i;
		s->bolts[bolt].forks[fork].vertices[i][vert].y = y;
	}
}

void saver6_perturb(nstate *state, GR_COORD *x, GR_COORD *y, int maxperturb)
{
	*x += (int)FRANDRANGE(0, (maxperturb - 1.0)) -
				(double)(maxperturb / 2.0);
	if(*x < 0) *x = 0;
	if(*x > (state->si.cols - 1)) *x = state->si.cols - 1;

	*y += (int)FRANDRANGE(0, (maxperturb - 1.0)) -
				(double)(maxperturb / 2.0);
	if(*y < 0) *y = 0;
	if(*y > (state->si.cols - 1)) *y = state->si.cols - 1;
}

void saver6_makefork(nstate *state, s6state *s, int bolt, int fork, GR_COORD x,
								GR_COORD y)
{
	int i, vertices;
	double length, incr, pos, angle, scale;
	GR_COORD ex, ey, nx, ny, xlen, ylen;

	saver6_setvertices(s, bolt, fork, 0, x , y);

	scale = (double)(state->si.rows - y) / (double)state->si.rows;

	vertices = (int)(scale * (double)RANDRANGE(SAVER6_MINFULLVERTICES,
						SAVER6_MAXVERTICES));

	if(vertices < SAVER6_MINVERTICES) vertices = SAVER6_MINVERTICES;

	s->bolts[bolt].forks[fork].valid = vertices;

	ey = state->si.rows - SAVER6_MAXEND_Y +
		(int)FRANDRANGE(0, SAVER6_MAXEND_Y - 1.0);
	if((ey - y) <= 0) ey = SAVER6_MINDROP;
	if(ey >= (state->si.rows - 1)) ey = state->si.rows - 1;

	if(!fork) {
		ex = x + FRANDRANGE(0, ((state->si.cols - 1.0) / 2.0));
	} else {
		ex = x + (FRANDRANGE(0, (ey - y)) / 2.0) - ((ey - y) / 2.0);
	}

	if(ex >= state->si.cols) ex = state->si.cols - 1;
	if(ex < 0) ex = 0;

	xlen = MAX(x, ex) - MIN(x, ex);
	ylen = MAX(y, ey) - MIN(y, ey);

	length = sqrt(((double)(xlen * xlen) + (double)(ylen * ylen)));
	incr = length / (vertices - 1);
	angle = atan(((double)xlen / (double)ylen));

	for(i = vertices - 1; i ; i--) {
		pos = (incr * (i - 1)) + (FRANDRANGE(0, SAVER6_MAXZIGZAG) -
					((double)SAVER6_MAXZIGZAG / 2.0));
		if(pos < 0) pos = 0;
		if(pos > length) pos = length;
		nx = x - (pos * sin(angle));
		ny = y + pos * cos(angle);
		saver6_perturb(state, &nx, &ny, SAVER6_MAXZIGZAG);
		saver6_setvertices(s, bolt, fork, i, nx , ny);
	}
}

int saver6_makeforks(nstate *state, s6state *s, int bolt, int fork,
						int *vert, int *nextfork)
{
	int thisvert = 1, thisfork;
	double prob;

	if(*vert == (s->bolts[bolt].forks[fork].valid - 1)) return 0;
	if(*nextfork == SAVER6_MAXFORKS) return 0;

 	prob = (double)SAVER6_FORK_PROBABILITY * ((double)*vert /
				(double)s->bolts[bolt].forks[fork].valid) *
					(1.0 / ((double)fork + 1.0));
	if((double)FRANDRANGE(0, 1) < prob) {
		thisfork = *nextfork;
		saver6_makefork(state, s, bolt, thisfork,
			s->bolts[bolt].forks[fork].vertices[0][*vert].x,
			s->bolts[bolt].forks[fork].vertices[0][*vert].y);
		*nextfork += 1;
		while(saver6_makeforks(state, s, bolt, thisfork, &thisvert,
								nextfork));
	}

	*vert += 1;

	return 1;
}

void saver6_makebolt(nstate *state, s6state *s, int bolt)
{
	GR_COORD x;
	int vert = 1, nextfork = 1, n;

	for(n = 0; n < SAVER6_MAXFORKS; n++)
		s->bolts[bolt].forks[n].valid = 0;

	x = RANDRANGE(0, state->si.cols - 1);

	saver6_makefork(state, s, bolt, 0, x, 0);

	while(saver6_makeforks(state, s, bolt, 0, &vert, &nextfork));
}

void saver6_newbolt(nstate *state, s6state *s)
{
	int i;

	for(i = 0; i < SAVER6_MAXBOLTS; i++) {
		if(!s->bolts[i].duration) {
			saver6_makebolt(state, s, i);
			s->bolts[i].duration = RANDRANGE(SAVER6_MINDURATION,
							SAVER6_MAXDURATION);
			saver6_drawbolt(state, s, i, 0);
			break;
		}
	}

	s->new_bolt_time = RANDRANGE(1, SAVER6_MAXNEWBOLTTIME);
}

void saver6_perturb_bolt(nstate *state, s6state *s, int bolt, int fork)
{
	int m, o;
	GR_COORD x, ox, y, oy;

	for(m = 1; m < s->bolts[bolt].forks[fork].valid; m++) {
		ox = x = s->bolts[bolt].forks[fork].vertices[0][m].x;
		oy = y = s->bolts[bolt].forks[fork].vertices[0][m].y;
		saver6_perturb(state, &x, &y, SAVER6_MAXZIGZAG);
		saver6_setvertices(s, bolt, fork, m, x, y);
		for(o = fork + 1; o < SAVER6_MAXFORKS; o++) {
			if((s->bolts[bolt].forks[o].vertices[0][0].x == ox) &&
				(s->bolts[bolt].forks[o].vertices[0][0].y
								== oy)) {
				saver6_setvertices(s, bolt, o, 0, x, y);
			}
		}
	}
}

void saver6_perturb_lightning(nstate *state, s6state *s)
{
	int i, n;

	for(i = 0; i < SAVER6_MAXBOLTS; i++) {
		if(!s->bolts[i].duration) continue;
		for(n = 0; n < SAVER6_MAXFORKS; n++) {
			if(!s->bolts[i].forks[n].valid) continue;
			saver6_perturb_bolt(state, s, i, n);
		}
	}
}

void saver6_animate(nstate *state)
{
	s6state *s = state->priv;

	saver6_drawlightning(state, s, 1);
	saver6_perturb_lightning(state, s);
	saver6_drawlightning(state, s, 0);

	if(!s->new_bolt_time--) saver6_newbolt(state, s);
}

/* The algorithm used in saver7 was adapted from "grav" by Greg Bowering */

void saver7_drawstar(nstate *state, s7state *s)
{
	GrSetGCForeground(state->main_gc, SAVER7_STARCOLOUR);
	GrFillEllipse(state->main_window, state->main_gc, s->starx, s->stary,
				SAVER7_STARRADIUS, SAVER7_STARRADIUS);
}

void saver7_drawplanet(nstate *state, s7state *s, int planet, int erase)
{
	if(erase) GrSetGCForeground(state->main_gc, GR_COLOR_BLACK);
	else GrSetGCForeground(state->main_gc, s->planets[planet].colour);

	if((s->planets[planet].ax < 0) || (s->planets[planet].ay < 0) ||
			(s->planets[planet].ax >= state->si.cols) ||
			(s->planets[planet].ay >= state->si.rows)) {
		return;
	}

	GrFillEllipse(state->main_window, state->main_gc, s->planets[planet].ax,
				s->planets[planet].ay,
				SAVER7_PLANETRADIUS, SAVER7_PLANETRADIUS);
}

void saver7_calc_planet_position(nstate *state, s7state *s, int planet)
{
	if(s->planets[planet].r > -SAVER7_ALMOSTDIST) {
		s->planets[planet].ax = (int)((double) state->si.cols *
			(0.5 + (s->planets[planet].x / (s->planets[planet].r +
							SAVER7_DIST))));
		s->planets[planet].ay = (int)((double) state->si.rows *
			(0.5 + (s->planets[planet].y / (s->planets[planet].r +
							SAVER7_DIST))));
	} else {
		s->planets[planet].ax = -1;
		s->planets[planet].ay = -1;
	}
}

void saver7_init(nstate *state)
{
	int i;
	s7state *s = my_malloc(sizeof(s7state));
	state->priv = s;

	s->starx = state->si.cols / 2;
	s->stary = state->si.rows / 2;

	for(i = 0; i < SAVER7_PLANETS; i++) {
		s->planets[i].r = FRANDRANGE(SAVER7_MIN_STARTDIM,
						SAVER7_MAX_STARTDIM);
		s->planets[i].x = FRANDRANGE(SAVER7_MIN_STARTDIM,
						SAVER7_MAX_STARTDIM);
		s->planets[i].y = FRANDRANGE(SAVER7_MIN_STARTDIM,
						SAVER7_MAX_STARTDIM);
		s->planets[i].rv = FRANDRANGE(SAVER7_MIN_STARTVEL,
						SAVER7_MAX_STARTVEL);
		s->planets[i].xv = FRANDRANGE(SAVER7_MIN_STARTVEL,
						SAVER7_MAX_STARTVEL);
		s->planets[i].yv = FRANDRANGE(SAVER7_MIN_STARTVEL,
						SAVER7_MAX_STARTVEL);
		s->planets[i].colour = RANDRANGE(0, state->si.ncolors - 1);
		saver7_calc_planet_position(state, s, i);
		saver7_drawplanet(state, s, i, 0);
	}

	saver7_drawstar(state, s);

	state->animate_interval = SAVER7_DELAY;
}

void saver7_exposure(nstate *state)
{
	int i;
	s7state *s = state->priv;

	GrClearWindow(state->main_window, 0);

	for(i = 0; i < SAVER7_PLANETS; i++)
		saver7_drawplanet(state, s, i, 0);

	saver7_drawstar(state, s);
}

void saver7_moveplanet(nstate *state, s7state *s, int planet)
{
	double dist;
	double accel;

	dist = (s->planets[planet].x * s->planets[planet].x) +
		(s->planets[planet].y * s->planets[planet].y) +
		(s->planets[planet].r * s->planets[planet].r);
	if(dist < SAVER7_COLLIDE) dist = SAVER7_COLLIDE;
	dist = sqrt(dist);
	dist = dist * dist * dist;

#ifdef SAVER7_USE_DAMPING
	accel = s->planets[planet].r * SAVER7_G / dist;
	if(accel > SAVER7_MAX_ACCEL) accel = SAVER7_MAX_ACCEL;
	else if(accel < -SAVER7_MAX_ACCEL) accel = -SAVER7_MAX_ACCEL;
	s->planets[planet].rv = (s->planets[planet].rv + accel) *
						SAVER7_DAMPING_FACTOR;
	s->planets[planet].r += s->planets[planet].rv;
	accel = s->planets[planet].x * SAVER7_G / dist;
	if(accel > SAVER7_MAX_ACCEL) accel = SAVER7_MAX_ACCEL;
	else if(accel < -SAVER7_MAX_ACCEL) accel = -SAVER7_MAX_ACCEL;
	s->planets[planet].xv = (s->planets[planet].xv + accel) *
						SAVER7_DAMPING_FACTOR;
	s->planets[planet].x += s->planets[planet].xv;
	accel = s->planets[planet].y * SAVER7_G / dist;
	if(accel > SAVER7_MAX_ACCEL) accel = SAVER7_MAX_ACCEL;
	else if(accel < -SAVER7_MAX_ACCEL) accel = -SAVER7_MAX_ACCEL;
	s->planets[planet].yv = (s->planets[planet].yv + accel) *
						SAVER7_DAMPING_FACTOR;
	s->planets[planet].y += s->planets[planet].yv;
#else
	accel = s->planets[planet].r * SAVER7_G / dist;
	s->planets[planet].rv += accel;
	s->planets[planet].r += s->planets[planet].rv;
	accel = s->planets[planet].x * SAVER7_G / dist;
	s->planets[planet].xv += accel;
	s->planets[planet].x += s->planets[planet].xv;
	accel = s->planets[planet].y * SAVER7_G / dist;
	s->planets[planet].yv += accel;
	s->planets[planet].y += s->planets[planet].yv;
#endif
}

void saver7_animate(nstate *state)
{
	int i;
	s7state *s = state->priv;

	for(i = 0; i < SAVER7_PLANETS; i++) {
		saver7_moveplanet(state, s, i);
		saver7_drawplanet(state, s, i, 1);
		saver7_calc_planet_position(state, s, i);
		saver7_drawplanet(state, s, i, 0);
	}
	saver7_drawstar(state, s);
}

/* The algorithm used in saver8 is based on that found at: 
   http://www.go2net.com/internet/deep/1997/04/16/body.html */

void saver8_init(nstate *state)
{
	int red = 0, green = 0, blue = 0, step, i = 0;

	s8state *s = my_malloc(sizeof(s8state));
	state->priv = s;

	s->current_line = 0;

	step = 512 / SAVER8_NUMCOLOURS;

	for(green = 255; green > 0; green -= step, blue += step, i++)
		GrFindColor(GR_RGB(0, green, blue), &s->colours[i]);
	for(blue = 255; blue > 0; blue -= step, red += step, i++)
		GrFindColor(GR_RGB(red, 0, blue), &s->colours[i]);
	
	s->rows = my_malloc(sizeof(GR_PIXELVAL) * state->si.cols *
			SAVER8_LINES_PER_FRAME);
	
	state->animate_interval = SAVER8_DELAY1;
}

void saver8_drawpattern(nstate *state)
{
	int x, y, col, newfactor, lines = SAVER8_LINES_PER_FRAME;
	s8state *s = state->priv;
	GR_PIXELVAL *p = s->rows;

	if(!s->current_line) {
		state->animate_interval = SAVER8_DELAY1;
		do {
			newfactor = RANDRANGE(SAVER8_MINFACTOR,
					SAVER8_MAXFACTOR);
		} while(newfactor == s->factor);
		s->factor = newfactor;
	}

	y = s->current_line;
	while(s->current_line < state->si.rows) {
		if(!--lines) break;
		for(x = 0; x < state->si.cols; x++) {
			col = ((((x * x) + (s->current_line * s->current_line))
					/ s->factor) % SAVER8_NUMCOLOURS);
			*p++ = s->colours[col];
		}
		s->current_line++;
	}

	GrArea(state->main_window, state->main_gc, 0, y, state->si.cols,
			s->current_line - y, s->rows, MWPF_PIXELVAL);

	if(lines) {
		state->animate_interval = SAVER8_DELAY2;
		s->current_line = 0;
	}
}

void saver8_exposure(nstate *state)
{
	s8state *s = state->priv;

	GrClearWindow(state->main_window, 0);
	s->current_line = 0;
	saver8_drawpattern(state);
}

void saver8_animate(nstate *state)
{
	saver8_drawpattern(state);
}

/* saver9 is based on the melt mode of decay from xscreensaver. */

void saver9_init(nstate *state)
{
	s9state *s = my_malloc(sizeof(s9state));
	state->priv = s;

	s->pid = capture_screen(state);

	GrSetGCForeground(state->main_gc, GR_COLOR_BLACK);
	GrLine(s->pid, state->main_gc, 0, 0, state->si.cols, 0);
	
	state->animate_interval = SAVER9_DELAY;
}

void saver9_exposure(nstate *state)
{
	s9state *s = state->priv;

	GrCopyArea(state->main_window, state->main_gc, 0, 0, state->si.cols,
			state->si.rows, s->pid, 0, 0, 0);
}

void saver9_animate(nstate *state)
{
	GR_COORD x, y;
	GR_SIZE w, h;
	s9state *s = state->priv;

	x = RANDRANGE(0, state->si.cols - 2);
	y = RANDRANGE(0, state->si.rows - 2);
	w = RANDRANGE(1, state->si.cols - x);
	h = RANDRANGE(1, state->si.rows - y);

	GrCopyArea(s->pid, state->main_gc, x, y + 1, w, h, s->pid, x, y, 0);
	GrCopyArea(state->main_window, state->main_gc, x, y + 1, w, h, s->pid,
								x, y, 0);
}

/* saver10 is quite loosely based on spotlight from xscreensaver. */

void saver10_init(nstate *state)
{
	s10state *s = my_malloc(sizeof(s10state));
	state->priv = s;

	s->screen = capture_screen(state);
	s->spot = GrNewPixmap(SAVER10_SPOTDIA + SAVER10_OVERSIZE * 2,
			SAVER10_SPOTDIA + SAVER10_OVERSIZE * 2, NULL);

	s->x = RANDRANGE((SAVER10_SPOTDIA / 2) + SAVER10_OVERSIZE,
			state->si.cols - 1 - (SAVER10_SPOTDIA / 2) -
			SAVER10_OVERSIZE);
	s->y = RANDRANGE((SAVER10_SPOTDIA / 2) + SAVER10_OVERSIZE,
			state->si.rows - 1 - (SAVER10_SPOTDIA / 2) -
			SAVER10_OVERSIZE);
	s->direction = FRANDRANGE(0, 2 * PI) - PI;

	state->animate_interval = SAVER10_DELAY;
}

void saver10_exposure(nstate *state)
{
	GrClearWindow(state->main_window, 0);
}

void saver10_animate(nstate *state)
{
	int i, x, y, w, xx, yy, retry;
	s10state *s = state->priv;
	double d, newx, newy;

	do {
		newx = s->x + cos(s->direction) * SAVER10_VELOCITY;
		newy = s->y + sin(s->direction) * SAVER10_VELOCITY;
		if((newx < (SAVER10_SPOTDIA / 2) + SAVER10_OVERSIZE) ||
				(newx > state->si.cols - (SAVER10_SPOTDIA / 2)
				 - SAVER10_OVERSIZE) ||
				(newy < (SAVER10_SPOTDIA / 2) + 3) ||
				(newy > state->si.rows - (SAVER10_SPOTDIA / 2)
				 - SAVER10_OVERSIZE)) {
			s->direction = FRANDRANGE(0, 2 * PI);
			retry = 1;
		} else retry = 0;
	} while(retry);

	s->x = newx;
	s->y = newy;
	
	y = s->y - (SAVER10_SPOTDIA / 2);
	yy = SAVER10_OVERSIZE;
	for(i = 0; i < SAVER10_SPOTDIA; i++) {
		d = sin(acos(((double)i/((double)SAVER10_SPOTDIA / 2.0)) - 1))
			* (double)SAVER10_SPOTDIA / 2.0;
		w = (int) 2 * d;
		x = (int) s->x - d;
		xx = (int) (SAVER10_SPOTDIA / 2) - d + SAVER10_OVERSIZE;
		if(!w) continue;
		GrCopyArea(s->spot, state->main_gc, xx, yy, w, 1, s->screen,
								x, y, 0);
		y++;
		yy++;
	}

	x = s->x - (SAVER10_SPOTDIA / 2) - SAVER10_OVERSIZE;
	y = s->y - (SAVER10_SPOTDIA / 2) - SAVER10_OVERSIZE;
	if(x < 0) x = 0;
	if(y < 0) y = 0;

	GrCopyArea(state->main_window, state->main_gc, x, y, SAVER10_SPOTDIA +
			2 * SAVER10_OVERSIZE, SAVER10_SPOTDIA +
			2 * SAVER10_OVERSIZE, s->spot, 0, 0, 0);
}

void saver11_init(nstate *state)
{
	s11state *s = my_malloc(sizeof(s11state));
	state->priv = s;

	state->animate_interval = SAVER11_DELAY;
}

void saver11_exposure(nstate *state)
{
	s11state *s = state->priv;
	
	GrClearWindow(state->main_window, 0);

	make_random_square(state, SAVER11_MIN_BRIGHTNESS, SAVER11_MIN_SIZE,
			SAVER11_MAX_SIZE, 0, &s->x, &s->y, &s->size);
}

void saver11_animate(nstate *state)
{
	s11state *s = state->priv;

	make_random_square(state, SAVER11_MIN_BRIGHTNESS, SAVER11_MIN_SIZE,
			SAVER11_MAX_SIZE, 0, &s->x, &s->y, &s->size);

	GrFillRect(state->main_window, state->main_gc, s->x, s->y, s->size,
			s->size);
}

void saver12_new_shape(nstate *state)
{
	s12state *s = state->priv;

	make_random_square(state, SAVER12_MIN_BRIGHTNESS, SAVER12_MIN_SIZE,
			SAVER12_MAX_SIZE, 1, &s->x, &s->y, &s->size);

	s->t = 0;
	s->kt = RANDRANGE(0, s->size);
}

void saver12_init(nstate *state)
{
	s12state *s = my_malloc(sizeof(s12state));
	state->priv = s;

	saver12_new_shape(state);

	state->animate_interval = SAVER12_DELAY;
}

void saver12_exposure(nstate *state)
{
	GrClearWindow(state->main_window, 0);

	saver12_new_shape(state);
}

void saver12_animate(nstate *state)
{
	int x1, y1, x2, y2;
	s12state *s = state->priv;

	if(s->t >= s->size) saver12_new_shape(state);

	for(x1 = 0; x1 < s->size; x1++) {
		y1 = (x1 ^ ((s->t + s->kt) % s->size)) % s->size;
		x2 = (x1 % s->size) + s->x;
		y2 = y1 + s->y;
		GrPoint(state->main_window, state->main_gc, x2, y2);
	}

	s->t++;
}

/* Zoom from xscreensavers was the inspiration for saver13 */

void saver13_init(nstate *state)
{
	s13state *s = my_malloc(sizeof(s13state));
	state->priv = s;

	s->screen = my_malloc(sizeof(GR_PIXELVAL) * state->si.rows *
							state->si.cols);
	s->rows = my_malloc(sizeof(GR_PIXELVAL) * state->si.cols *
			SAVER13_ZOOM_FACTOR * SAVER13_ROWS_BUFFER);
	s->rownum = 0;

	s->x = RANDRANGE(0, state->si.cols - (state->si.cols /
			SAVER13_ZOOM_FACTOR));
	s->x = RANDRANGE(0, state->si.rows - (state->si.rows /
			SAVER13_ZOOM_FACTOR));
	s->direction = FRANDRANGE(0, 2 * PI) - PI;

	state->animate_interval = 1;

#if CAPTURESCREEN_DELAY
	msleep(CAPTURESCREEN_DELAY);
#endif
	
	GrReadArea(GR_ROOT_WINDOW_ID, 0, 0, state->si.cols, state->si.rows,
			s->screen);
}

void saver13_exposure(nstate *state)
{
	s13state *s = state->priv;
	GrClearWindow(state->main_window, 0);
	s->rownum = 0;
}

void saver13_move_portal(nstate *state)
{
	int retry;
	double newx, newy;
	s13state *s = state->priv;

	do {
		newx = s->x + cos(s->direction) * SAVER13_VELOCITY;
		newy = s->y + sin(s->direction) * SAVER13_VELOCITY;
		if(newx < 0 || newx > state->si.cols -
				(state->si.cols / SAVER13_ZOOM_FACTOR) ||
				newy < 0 || newy > state->si.rows -
				(state->si.rows / SAVER13_ZOOM_FACTOR)) {
			s->direction = FRANDRANGE(0, 2 * PI);
			retry = 1;
		} else retry = 0;
	} while(retry);
	s->x = newx;
	s->y = newy;
}

void saver13_animate(nstate *state)
{
	int rows, r, y, i, n;
	s13state *s = state->priv;
	GR_PIXELVAL *src, *dst;

	rows = (state->si.rows / SAVER13_ZOOM_FACTOR) - s->rownum;
	if(rows > SAVER13_ROWS_BUFFER) rows = SAVER13_ROWS_BUFFER;
	dst = s->rows;
	y = s->y + s->rownum;
	for(r = 0; r < rows; r++) {
		src = &s->screen[(y++ * state->si.cols) + (int)s->x];
		for(i = 0; i < SAVER13_ZOOM_FACTOR; i++)
			for(n = 0; n < state->si.cols; n++)
				*dst++ = src[n / SAVER13_ZOOM_FACTOR];
	}

	GrArea(state->main_window, state->main_gc, 0, s->rownum *
			SAVER13_ZOOM_FACTOR, state->si.cols,
			SAVER13_ZOOM_FACTOR * rows,
			s->rows, MWPF_PIXELVAL);
	
	s->rownum += rows;
	if(s->rownum >= state->si.rows / SAVER13_ZOOM_FACTOR) {
		s->rownum = 0;
		saver13_move_portal(state);
		state->animate_interval = SAVER13_DELAY;
	} else state->animate_interval = 1;

}

int init(nstate *state)
{
	GR_WM_PROPERTIES props;
	GR_BITMAP cursor = 0;

	if(!GrOpen()) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 3;
	}

	GrGetScreenInfo(&state->si);

	state->main_window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0,
			state->si.cols, state->si.rows, 0, GR_COLOR_BLACK, 0);

	GrSelectEvents(state->main_window, GR_EVENT_MASK_EXPOSURE |
						GR_EVENT_MASK_BUTTON_UP |
						GR_EVENT_MASK_BUTTON_DOWN |
						GR_EVENT_MASK_MOUSE_MOTION |
						GR_EVENT_MASK_KEY_UP |
						GR_EVENT_MASK_KEY_DOWN |
						GR_EVENT_MASK_FOCUS_OUT |
						GR_EVENT_MASK_CLOSE_REQ);

	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NOMOVE | GR_WM_PROPS_NODECORATE |
			GR_WM_PROPS_NOAUTOMOVE | GR_WM_PROPS_NOAUTORESIZE;
	GrSetWMProperties(state->main_window, &props);

	state->main_gc = GrNewGC();
	GrSetGCForeground(state->main_gc, GR_COLOR_WHITE);
	GrSetGCBackground(state->main_gc, GR_COLOR_BLACK);

	state->animate_interval = 0;

	srand(time(0));

	init_functions[state->saver](state);

	calculate_timeout(state);

	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_SCREENSAVER);

	GrSetCursor(state->main_window, 1, 1, 1, 1, 0, 0, &cursor, &cursor);

	GrMapWindow(state->main_window);

	GrSetFocus(state->main_window);

	return 0;
}

void calculate_timeout(nstate *state)
{
	struct timeval t;
	long u;

	gettimeofday(&t, NULL);
	u = t.tv_usec + (state->animate_interval * 1000);
	state->timeout.tv_sec = t.tv_sec + (u / 1000000);
	state->timeout.tv_usec = u % 1000000;
}

unsigned long timeout_delay(nstate *state)
{
	struct timeval t;
	signed long s, m, ret;

	gettimeofday(&t, NULL);

	if(!state->animate_interval) return 0;

	if((t.tv_sec > state->timeout.tv_sec) ||
			((t.tv_sec == state->timeout.tv_sec) &&
			t.tv_usec >= state->timeout.tv_usec)) return 1;

	s = state->timeout.tv_sec - t.tv_sec;
	m = ((state->timeout.tv_usec - t.tv_usec) / 1000);
	ret = (unsigned long)((1000 * s) + m);

	if(ret <= 0) return 1;
	else return ret;
}

void do_animate(nstate *state)
{
	struct timeval t;

	if(!state->animate_interval) return;

	gettimeofday(&t, NULL);

	if((t.tv_sec > state->timeout.tv_sec) ||
			((t.tv_sec == state->timeout.tv_sec) &&
			(t.tv_usec >= state->timeout.tv_usec))) {
		animate_functions[state->saver](state);
		calculate_timeout(state);
	}
}

int do_screensaver_event(nstate *state)
{
	GR_EVENT_SCREENSAVER *event = &state->event.screensaver;

	if(event->activate != GR_FALSE) {
		fprintf(stderr, "Got a non-deactivate screensaver event\n");
		return 0;
	}

	return 1;
}

int handle_event(nstate *state)
{
	switch(state->event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			exposure_functions[state->saver](state);
		case GR_EVENT_TYPE_TIMEOUT:
		case GR_EVENT_TYPE_NONE:
			break;
		case GR_EVENT_TYPE_SCREENSAVER:
			if(do_screensaver_event(state)) return 0;
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
		case GR_EVENT_TYPE_BUTTON_UP:
		case GR_EVENT_TYPE_BUTTON_DOWN:
		case GR_EVENT_TYPE_MOUSE_MOTION:
		case GR_EVENT_TYPE_KEY_UP:
		case GR_EVENT_TYPE_KEY_DOWN:
		case GR_EVENT_TYPE_FOCUS_OUT:
			return 0;
		default:
			fprintf(stderr, "Got unknown event type %d\n",
							state->event.type);
			break;
	}
	do_animate(state);
	return(1);
}

int main(int argc, char *argv[])
{
	int ret;
	nstate *state = my_malloc(sizeof(nstate));

	if(argc == 2) {
		state->saver = atoi(argv[1]) - 1;
		if((state->saver) < 0 || (state->saver >= NUM_SAVERS)) {
			fprintf(stderr, "Invalid saver number \"%s\"\n",
								argv[1]);
			return 2;
		}
	} else state->saver = 0;

	if((ret = init(state))) return ret;

	do {
		GrGetNextEventTimeout(&state->event, timeout_delay(state));
	} while(handle_event(state));

	GrClose();

	return 0;
}
