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

/* animate.c contains animate(), which is called many times a second while the
 * game is running to move the ball. */

#include <stdio.h>
#include <stdlib.h>

#include <nano-X.h>
#include "nbreaker.h"

/* The main animate routine that is called many times a second to move the
 * ball, the bat (when using the cursor keys) and the power boxes: */
void animate(nbstate *state)
{
	coords newpos;
	int ret, i = 32;
	GR_PIXMAP_ID ctmp;

	/* Don't move anything if the game is paused: */
	if(state->flags.paused) return;

	/* If we're currently fading the screen in: */
	if(state->state == STATE_FADING) {
		/* Reduce the opacity: */
		state->fadelevel -= state->faderate;

		/* If we've finished fading: */
		if(state->fadelevel <= 0) {

			/* Go to the next state: */
			state->state = state->nextstate;

			/* Swap the canvasses around: */
			ctmp = state->newcanvas;
			state->newcanvas = state->canvas;
			state->canvas = ctmp;

			/* Unless we're on the title screen, we need to redraw
			 * the bat area (and ball too, if it is parked) now as
			 * the mouse may have moved whilst the fade was
			 * occuring, which means we don't know where they were
			 * when the fade started. */
			if(state->state != STATE_TITLESCREEN) {
				if(state->ball.parked) {
					/* Erase the area: */
					draw_background(state, 0,
						state->canvasheight -
						state->batheight -
						state->ball.s->h,
						state->canvaswidth,
						state->batheight +
						state->ball.s->h);
					/* Redraw the bat: */
					draw_bat(state);
					/* Redraw the ball: */
					draw_ball(state);
				} else {
					/* Erase the area: */
					draw_background(state, 0,
						state->canvasheight -
						state->batheight,
						state->canvaswidth,
						state->batheight);
					/* Redraw the bat: */
					draw_bat(state);
				}
			}

		/* Otherwise generate the fade: */
		} else {
			/* Copy the new canvas in: */
			GrCopyArea(state->canvas, state->gc, 0, 0,
				state->canvaswidth, state->canvasheight,
				state->newcanvas, 0, 0, 0);
			/* Blend the old canvas over it: */
			GrCopyArea(state->canvas, state->gc, 0, 0,
				state->canvaswidth, state->canvasheight,
				state->oldcanvas, 0, 0,
				GR_CONST_BLEND | state->fadelevel);
		}

		/* Copy the result to the output window: */
		draw_canvas(state, 0, 0, state->canvaswidth,
				state->canvasheight);

		/* Return without animating anything else: */
		return;
	}

	/* If the left cursor is pressed, move the bat to the left: */
	if(state->flags.left)
		move_bat(state, state->batx - state->batv);

	/* If the right cursor is pressed, move the bat to the right: */
	if(state->flags.right)
		move_bat(state, state->batx + state->batv);

	/* Move the power boxes: */
	animate_powers(state);

	/* If the ball is parked, don't move it: */
	if(state->ball.parked) return;

	/* Calculate the new ball position and check if it will collide with
	 * anything. If the collision detection routine returns 1, that means
	 * that it has detected a collision and changed the ball direction so
	 * recalculate the new ball position and try again. We have a limit on
	 * the maximum number of attempts because I don't trust the collision
	 * detection algorithms 100% never to get stuck in a loop. */
	do {
		calc_new_ball_coords(state, &newpos);
		ret = check_ball_collision(state, &newpos);
	} while(ret == 1 && --i);

	/* If the collision detection routine returns 2, it means that
	 * something exceptional happened and we shouldn't bother trying to
	 * move the ball after all. */
	if(ret == 2) return;

	/* If the collision detection code got stuck in a loop, park the ball
	 * and print an error message. */
	if(!i) {
		fprintf(stderr, "Internal error: stuck in collision loop\n");
		park_ball(state);
		move_ball(state);
		return;
	}

	/* Store the newly calculated ball position and move the ball to it: */
	state->ball.x = newpos.x;
	state->ball.y = newpos.y;
	move_ball(state);
}
