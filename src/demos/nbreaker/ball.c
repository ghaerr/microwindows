/*
 * NanoBreaker, a Nano-X Breakout clone by Alex Holden.
 *
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
 * The Original Code is NanoBreaker.
 * 
 * The Initial Developer of the Original Code is Alex Holden.
 * Portions created by Alex Holden are Copyright (C) 2002
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

/* ball.c contains functions related to the ball. */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Called whenever a ball is lost, either by it falling off the bottom of the
 * screen, or the player pressing the "suicide key". */
void lost_ball(nbstate *state)
{
	/* Decrement the balls count and if there are none left: */
	if(!state->numballs--) {
		/* Go to the "game lost" state and update the high score if
		 * appropriate: */
		state->state = STATE_GAMELOST;
		/* Update the high score and save it if necessary: */
		save_hiscore(state);
		/* This could probably be done better by just drawing the
		 * splash- set_level_active() redraws the entire game area: */
		set_level_active(state);
		return;
	}

	/* Erase the balls line at the top of the screen: */
	draw_background(state, 0, state->scores.h, state->canvaswidth,
			state->ball.s->h + (BALLS_BORDER * 2));
	/* Draw the balls again, but with one less than there was before: */
	draw_balls(state);
	/* Copy the balls row to the output window: */
	draw_canvas(state, 0, state->scores.h, state->canvaswidth,
				state->ball.s->h + (BALLS_BORDER * 2));
	/* Park the new ball and erase the old one: */
	park_ball(state);
	move_ball(state);
	/* Redraw the bat. This is a bit of a hack because sometimes when the
	 * ball falls below the top of the bat on its way to the bottom of the
	 * screen it can clip the bat and erase a bit of it. This redraws the
	 * bat to hide that: */
	draw_bat(state);
	/* Copy the redrawn bat to the output window: */
	draw_canvas(state, state->batx - (state->batwidths[state->bat] / 2),
			state->canvasheight - state->batheight,
			state->batwidths[state->bat], state->batheight);
}

/* Park the ball. This means make it sit on the middle of the bat and stay
 * there until a mouse button is clicked or the space bar pressed. */
void park_ball(nbstate *state)
{
	state->ball.parked = 1; /* Set the parked flag. */
	/* Set the ball position: */
	state->ball.x = state->batx - (state->ball.s->w / 2);
	state->ball.y = state->canvasheight -
					state->batheight - state->ball.s->h;
	/* Set the ball direction to "straight up": */
	state->ball.d = 0;
}

/* Given the current ball position and the direction and velocity of movement,
 * calculate the next ball position and put it in the passed coords
 * structure. This uses floating point math for accuracy and convenience, but
 * it could probably be converted to fixed point math without too much
 * difficulty. */
void calc_new_ball_coords(nbstate *state, coords *c)
{
	c->x = state->ball.x + sin(state->ball.d) * state->ball.v;
	c->y = state->ball.y - cos(state->ball.d) * state->ball.v;
}

/* If the specified angle (in radians) has strayed outside the range 0 to
 * 2 * PI, add 2 * PI to or subtract 2 * PI from it to get it back into the
 * correct range. */
static double normalise_angle(double d)
{
	if(d < 0) return d + (2 * M_PI);
	else if(d > (2 * M_PI)) d -= (2 * M_PI);
	return d;
}

/* Check if the ball will collide with something if it is moved to the
 * specified coordinates. If so, the direction is changed and 1 is returned
 * to indicate that the caller should recalculate the new coordinates and
 * try again. If there was no collision it returns 0. If something exceptional
 * happens (eg. the last brick is destroyed or the last ball is lost) it
 * returns 2 to indicate that the caller should give up trying to move the
 * ball. */
int check_ball_collision(nbstate *state, coords *c)
{
	int i, bc;
	grid *g = state->grid;

	/* Check for a collision with the top of the game area: */
	if(c->y < state->ball.s->h + (2 * BALLS_BORDER) + state->scores.h) {
		/* Bounce the ball back down and ask the caller to try again: */
		state->ball.d = normalise_angle(M_PI - state->ball.d);
		return 1;
	}

	/* Check for a collision with the bottom of the game area: */
	if(c->y > state->canvasheight - state->ball.s->h) {
		/* If the solidfloor cheat is active, bounce the ball back up
		 * and ask the caller to try again: */
		if(state->flags.sf) {
			state->ball.d = normalise_angle(M_PI - state->ball.d);
			return 1;
		} else {
			/* Otherwise destroy the ball, move the new ball to
			 * the parked position (park_ball() is called by
			 * lost_ball()) and ask the caller to give up trying
			 * to move the ball: */
			lost_ball(state);
			move_ball(state);
			return 2;
		}
	}

	/* Check for a collision with the left hand side of the game area: */
	if(c->x < 0) {
		/* Bounce the ball back and ask the caller to try again: */
		state->ball.d = normalise_angle((2 * M_PI) - state->ball.d);
		return 1;
	}

	/* Check for a collision with the right hand side of the game area: */
	if(c->x > state->canvaswidth - state->ball.s->w) {
		/* Bounce the ball back and ask the caller to try again: */
		state->ball.d = normalise_angle((2 * M_PI) - state->ball.d);
		return 1;
	}

	/* Check for a collision with the bat: */
	if(c->y > state->canvasheight - state->batheight - state->ball.s->h &&
			c->x > state->batx -
			(state->batwidths[state->bat] / 2) - state->ball.s->w &&
			c->x < state->batx +
			(state->batwidths[state->bat] / 2)) {

		/* If the collision happened with the side of the bat instead
		 * of the top, we don't care so just tell the caller there
		 * was no collision: */
		if(state->ball.y > state->canvasheight - state->batheight -
				state->ball.s->h)
			return 0;

		/* If the StickyBat power-up is active, park the ball: */
		if(state->powertimes.stickybat) {
			park_ball(state);
			move_ball(state);
			return 2;
		} else {
			/* Otherwise bounce it back up and ask the caller to
			 * try again: */
			state->ball.d = normalise_angle(((c->x +
						(state->ball.s->w / 2)
						- state->batx) /
						state->batwidths[state->bat] /
						2) * M_PI);
			return 1;
		}
	}

	/* Check for collisions with the bricks: */
	bc = 0; /* No collisions have happened yet. */
	/* For each brick in the grid: */
	for(i = 0; i < state->width * state->height; i++) {
		/* If there is a brick in this grid position and the ball
		 * intersects it:  */
		if(g->b && c->y + state->ball.s->h > g->y && c->y < g->y +
				state->brickheight && c->x + state->ball.s->w
				> g->x && c->x < g->x + state->brickwidth) {

			/* Perform the brick collision actions, and if
			 * something exceptional happens (ie. we destroy the
			 * last brick), return straight away asking the caller
			 * to give up trying to move the ball: */
			if(brick_collision(state, g)) return 2;

			/* Unless the NoBounce cheat is active, bounce the
			 * ball off the brick. Only do this on the first brick
			 * collision we find. */
			if(!state->flags.nb && !bc) {
				bc = 1;
				/* Bounce off the left face: */
				if(state->ball.x + state->ball.s->w < g->x) {
					state->ball.d = normalise_angle((2 *
							M_PI) - state->ball.d);
				/* Bounce off the right face: */
				} else if(state->ball.x >= g->x +
						state->brickwidth) {
					state->ball.d = normalise_angle((2 *
							M_PI) - state->ball.d);
				/* Bounce off the upper face: */
				} else if(state->ball.y + state->ball.s->h
								< g->y) {
					state->ball.d = normalise_angle(M_PI -
								state->ball.d);
				/* Bounce off the lower face: */
				} else if(state->ball.y >= g->y +
							state->brickheight) {
					state->ball.d = normalise_angle(M_PI -
								state->ball.d);
				} else {
					/* This shouldn't happen, but I don't
					 * trust the above algorithm 100%. */
					fprintf(stderr, "Internal error: "
						"couldn't figure out brick "
						"collision face\n");
				}
			}
		}
		g++; /* Increment to the next grid position. */
	}

	/* If a brick collision occured, ask the caller to try again: */
	if(bc) return 1;
	return 0; /* Otherwise tell the caller that no collision occured. */
}

/* Move the ball to from the position specified by state->ball.lx and
 * state->ball.ly to state->ball.x and state->ball.y, and update the relevant
 * area of the output window. */
void move_ball(nbstate *state)
{
	int x, w, y, h;

	/* Check that the ball really has moved: */
	if(state->ball.lx == state->ball.x && state->ball.ly == state->ball.y)
		return;

	/* Calculate the position and dimensions of the rectangle which
	 * encloses both the old ball and the new ball so we know what area
	 * to copy to the output window later. FIXME: this is quite inefficient
	 * when doing stuff like parking the ball, where the old ball position
	 * could be at the other side of the screen to the place we want to
	 * move it to. It is however OK in the common case of only moving the
	 * ball a few pixels at a time. */
	if(state->ball.x < state->ball.lx) {
		x = (int)state->ball.x;
		w = state->ball.lx - x + state->ball.s->w;
	} else {
		x = state->ball.lx;
		w = (int)state->ball.x - x + state->ball.s->w;
	}
	if(state->ball.y < state->ball.ly) {
		y = (int)state->ball.y;
		h = state->ball.ly - y + state->ball.s->h;
	} else {
		y = state->ball.ly;
		h = (int)state->ball.y - y + state->ball.s->h;
	}

	/* Draw the background where the old ball image is to erase it: */
	draw_background(state, state->ball.lx, state->ball.ly,
			state->ball.s->w, state->ball.s->h);

	/* Redraw any powers that are under that position: */
	redraw_powers(state, state->ball.lx, state->ball.ly, state->ball.s->w,
							state->ball.s->h);

	/* Draw the ball in the new position and update ball.lx and ball.ly: */
	draw_ball(state);

	/* Draw the modified area of the canvas to the output window: */
	draw_canvas(state, x, y, w, h);
}

/* Redraw the ball in its current position, including copying it to the output
 * window. */
void redraw_ball(nbstate *state)
{
	/* Erase the ball area to the background: */
	draw_background(state, (int)state->ball.x, (int)state->ball.y,
			state->ball.s->w, state->ball.s->h);

	/* Redraw any powers we may have erased: */
	redraw_powers(state, (int)state->ball.x, (int)state->ball.y,
			state->ball.s->w, state->ball.s->h);

	/* Redraw the ball: */
	draw_ball(state);

	/* Copy it to the output window: */
	draw_canvas(state, state->ball.lx, state->ball.ly, state->ball.s->w,
			state->ball.s->h);
}
