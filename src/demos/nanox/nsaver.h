#ifndef NSAVER_H
#define NSAVER_H
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
 * Portions created by Alex Holden are Copyright (C) 2000
 * Alex Holden <alex@linuxhacker.org>. All Rights Reserved.
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

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define RANDRANGE(LO, HI) ((LO) + ((double)((HI) - (LO)) *  rand() \
						/ (RAND_MAX + 1.0)))

typedef struct {
	GR_WINDOW_ID main_window;
	GR_GC_ID main_gc;
	GR_EVENT event;
	long animate_interval;
	struct timeval timeout;
	int saver;
	void *priv;
	GR_SCREEN_INFO si;
} nstate;

void *my_malloc(size_t size);
void get_random_point_on_screen(nstate *state, GR_COORD *x, GR_COORD *y,
							GR_COLOR *c);
int init(nstate *state);
void calculate_timeout(nstate *state);
unsigned long timeout_delay(nstate *state);
unsigned long timeout_delay(nstate *state);
void do_animate(nstate *state);
int do_screensaver_event(nstate *state);
int handle_event(nstate *state);

#define NUM_SAVERS 8

void saver1_init(nstate *state);
void saver1_exposure(nstate *state);
void saver1_animate(nstate *state);

#define SAVER2_DELAY 1
#define SAVER2_MAXPIXELS 65535
#define SAVER2_PIXELS_PER_FRAME 16

void saver2_init(nstate *state);
void saver2_exposure(nstate *state);
void saver2_animate(nstate *state);

#define SAVER3_DELAY 1
#define SAVER3_MAXSEGMENTS 65535
#define SAVER3_SEGMENTS_PER_FRAME 4

typedef struct {
	int maxsegments;
	GR_COORD lastx;
	GR_COORD lasty;
} s3state;

void saver3_init(nstate *state);
void saver3_exposure(nstate *state);
void saver3_animate(nstate *state);

#define SAVER4_DELAY 4
#define SAVER4_NUMWORMS 10
#define SAVER4_WORMLENGTH 100
#define SAVER4_COLLISION_RELUCTANCE 20

typedef struct {
	GR_COLOR colour;
	GR_POINT points[SAVER4_WORMLENGTH];
} s4worm;

typedef struct {
	int length;
	int tip;
	s4worm worms[SAVER4_NUMWORMS];
} s4state;

void saver4_init(nstate *state);
void saver4_exposure(nstate *state);
void saver4_get_new_worm_position(nstate *state, s4state *s, int worm,
					GR_COORD *newx, GR_COORD *newy);
int saver4_worm_collides(nstate *state, s4state *s, int x, int y, int thisworm,
								int thispoint);
void saver4_animate(nstate *state);

#define SAVER5_DELAY 1
#define SAVER5_NUMSTARS 300
#define SAVER5_STARS_INCREMENT 2
#define SAVER5_STARS_ACCEL_RATE 30

typedef struct {
	double angle;
	int pos;
} s5star;

typedef struct {
	int numstars;
	s5star stars[SAVER5_NUMSTARS];
} s5state;

void saver5_init(nstate *state);
int saver5_drawstar(nstate *state, s5state *s, int star, int delete);
void saver5_exposure(nstate *state);
void saver5_animate(nstate *state);

#define SAVER6_DELAY 5
#define SAVER6_MAXVERTICES 20
#define SAVER6_MINFULLVERTICES 5
#define SAVER6_MINVERTICES 3
#define SAVER6_MINDROP 10
#define SAVER6_MAXBOLTS 4
#define SAVER6_MAXFORKS 20
#define SAVER6_MAXEND_Y 50
#define SAVER6_THICKNESS 6
#define SAVER6_MINDURATION 10
#define SAVER6_MAXDURATION 20
#define SAVER6_MAXNEWBOLTTIME 300
#define SAVER6_MAXZIGZAG 10
#define SAVER6_MAXPERTURBATION 5
#define SAVER6_FORK_PROBABILITY 0.5

typedef struct {
	int valid;
	GR_POINT vertices[SAVER6_THICKNESS][SAVER6_MAXVERTICES];
} s6fork;

typedef struct {
	int duration;
	s6fork forks[SAVER6_MAXFORKS];
} s6bolt;

typedef struct {
	int new_bolt_time;
	s6bolt bolts[SAVER6_MAXBOLTS];
} s6state;

void saver6_init(nstate *state);
void saver6_drawfork(nstate *state, s6state *s, int bolt, int fork, int delete);
void saver6_drawbolt(nstate *state, s6state *s, int bolt, int delete);
void saver6_drawlightning(nstate *state, s6state *s, int delete);
void saver6_exposure(nstate *state);
void saver6_setvertices(s6state *s, int bolt, int fork, int vert, GR_COORD x,
								GR_COORD y);
void saver6_makefork(nstate *state, s6state *s, int bolt, int fork, GR_COORD x,
								GR_COORD y);
void saver6_perturb(nstate *state, GR_COORD *x, GR_COORD *y, int maxperturb);
void saver6_makebolt(nstate *state, s6state *s, int bolt);
void saver6_newbolt(nstate *state, s6state *s);
void saver6_perturb_bolt(nstate *state, s6state *s, int bolt, int fork);
void saver6_perturb_lightning(nstate *state, s6state *s);
void saver6_animate(nstate *state);

/* The algorithm used in saver7 was adapted from "grav" by Greg Bowering */

#define SAVER7_DELAY 2
#define SAVER7_PLANETS 15
#define SAVER7_PLANETS_USE_DAMPING
#define SAVER7_STARCOLOUR YELLOW
#define SAVER7_STARRADIUS 5
#define SAVER7_PLANETRADIUS 3
#define SAVER7_DIST 16.0
#define SAVER7_ALMOSTDIST (SAVER7_DIST - 0.01)
#define SAVER7_MAX_STARTDIM (SAVER7_ALMOSTDIST / 2.0)
#define SAVER7_MIN_STARTDIM -SAVER7_MAX_STARTDIM
#define SAVER7_MAX_STARTVEL 0.04
#define SAVER7_MIN_STARTVEL -SAVER7_MAX_STARTVEL
#define SAVER7_G -0.02
#define SAVER7_COLLIDE 0.0001
#define SAVER7_DAMPING_FACTOR 0.999999
#define SAVER7_MAX_ACCEL 0.1

typedef struct {
	double r;
	double rv;
	double x;
	double xv;
	double y;
	double yv;
	GR_COORD ax;
	GR_COORD ay;
	GR_COLOR colour;
} s7planet;

typedef struct {
	s7planet planets[SAVER7_PLANETS];
	GR_COORD starx;
	GR_COORD stary;
} s7state;

void saver7_init(nstate *state);
void saver7_exposure(nstate *state);
void saver7_animate(nstate *state);

/* The algorithm used in saver8 is based on that found at:
   http://www.go2net.com/internet/deep/1997/04/16/body.html */

#define SAVER8_DELAY 1
#define SAVER8_NUMCOLOURS 64 /* Don't set this higher than 512! */
#define SAVER8_MINFACTOR 1
#define SAVER8_MAXFACTOR 20
#define SAVER8_LINES_PER_FRAME 5

typedef struct {
	GR_COLOR colours[SAVER8_NUMCOLOURS];
	int current_line;
	int factor;
} s8state;

void saver8_init(nstate *state);
void saver8_drawpattern(nstate *state);
void saver8_exposure(nstate *state);
void saver8_animate(nstate *state);

typedef void(*saver_function)(nstate *);

saver_function init_functions[NUM_SAVERS] = {
	saver1_init,
	saver2_init,
	saver3_init,
	saver4_init,
	saver5_init,
	saver6_init,
	saver7_init,
	saver8_init
};
saver_function exposure_functions[NUM_SAVERS] = {
	saver1_exposure,
	saver2_exposure,
	saver3_exposure,
	saver4_exposure,
	saver5_exposure,
	saver6_exposure,
	saver7_exposure,
	saver8_exposure
};
saver_function animate_functions[NUM_SAVERS] = {
	saver1_animate,
	saver2_animate,
	saver3_animate,
	saver4_animate,
	saver5_animate,
	saver6_animate,
	saver7_animate,
	saver8_animate
};
#endif
