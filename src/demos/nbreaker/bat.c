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

/* bat.c contains functions related to the bat. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Change the bat to the specified size (NORMALBAT, SMALLBAT, or LARGEBAT),
 * then redraws it. */
void change_bat(nbstate *state, int newbat)
{
	int ox1, ox2, nx1, nx2, x, w;

	/* If the new bat is wider than the old bat: */
	if(state->batwidths[newbat] > state->batwidths[state->bat]) {
		/* Calculate the position of the left side of the new bat: */
		x = state->batx - (state->batwidths[newbat] / 2);
		/* If it is off the screen, move the bat to the right: */
		if(x < 0) move_bat(state, state->batwidths[newbat] / 2);
		else {
			/* Calculate the position of the right side: */
			x += state->batwidths[newbat];
			/* If it is off the screen, move it to the left: */
			if(x > state->canvaswidth)
				move_bat(state, state->canvaswidth -
						state->batwidths[newbat] / 2);
		}
	}

	/* Calculate the left and right sides of the old and new bats: */
	ox1 = state->batx - (state->batwidths[state->bat] / 2);
	ox2 = ox1 + state->batwidths[state->bat];
	nx1 = state->batx - (state->batwidths[newbat] / 2);
	nx2 = nx1 + state->batwidths[newbat];

	/* Erase the old bat from the canvas: */
	draw_background(state, ox1, state->canvasheight - state->batheight,
			state->batwidths[state->bat], state->batheight);

	/* Set the new bat and draw it onto the canvas: */
	state->bat = newbat;
	draw_bat(state);

	/* Calculate the bounding rect of the area that needs to be redrawn: */
	if(ox1 < nx1) x = ox1;
	else x = nx1;
	if(ox2 > nx2) w = ox2 - x;
	else w = nx2 - x;

	/* Copy the affected area onto the output window: */
	draw_canvas(state, x, state->canvasheight - state->batheight, w,
			state->batheight);
}

/* Move the bat to the specified X coordinate and copy the result to the
 * output window. Also moves the ball if it is parked. */
void move_bat(nbstate *state, int x)
{
	sprite *s;
	power *p, *pnext;
	int ox, nx, l, r, w, h, y, y2;

	/* Don't allow the bat to move if the game is paused. This reduces the
	 * temptation to cheat by pausing the game when you see the ball is
	 * too far away for you to catch it, moving the bat, then unpausing: */
	if(state->flags.paused) return;

	/* Find the width and Y position of the bat: */
	w = state->batwidths[state->bat];
	y = state->canvasheight - state->batheight;

	/* Clip the bat position to the left and right sides of the window: */
	if(x - (w / 2) < 0) x = w / 2;
	if(x + (w / 2) > state->canvaswidth) x = state->canvaswidth - (w / 2);

	/* If the ball is parked, move it as well: */
	if(state->ball.parked) state->ball.x = x - (state->ball.s->w / 2);

	/* If the game isn't running, just remember the new bat position and
	 *  return: */
	if(state->state != STATE_RUNNING) {
		state->batx = x;
		return;
	}

	/* Check that the bat really has moved: */
	if(state->batx == x) return;

	/* Calculate the location and dimensions of the area which will need
	 * to be copied to the output window after the move: */
	ox = state->batx - (w / 2);
	state->batx = x;
	nx = state->batx - (w / 2);
	if(ox > nx) {
		l = nx;
		r = ox + w;
	} else {
		l = ox;
		r = nx + w;
	}	
	w = r - l;
	h = state->batheight;

	/* If the ball is parked, move it and add on the height of the ball to
	 * the area that needs copying to the output window: */
	y2 = y;
	if(state->ball.parked) {
		move_ball(state);
		y2 -= state->ball.s->h;
		h += state->ball.s->h;
	}

	/* For each power box: */
	for(p = state->powers; p; p = pnext) {
		pnext = p->next;

		/* Find the sprite used to draw this power box: */
		s = state->powersprites[p->type];

		/* Check if it collides with the bat: */
		if(p->y + s->h > state->canvasheight - state->batheight &&
						p->x + s->w > l && p->x < r) {

			/* Erase the area under the power box: */
			draw_background(state, p->x, p->y, s->w, s->h);

			/* Copy the erased region to the screen: */
			draw_canvas(state, p->x, p->y - state->powerv, s->w,
									s->h);

			/* Activate the relevant power: */
			power_collision(state, p);

			/* Destroy the power structure: */
			 destroy_power(state, p);
		}
	}

	/* Erase the old bat image by drawing the background over it: */
	draw_background(state, l, y, w, state->batheight);
	/* Draw the bat in the new position: */
	draw_bat(state);
	/* Copy the modified region of the canvas to the output window: */
	draw_canvas(state, l, y2, w, h);
}
