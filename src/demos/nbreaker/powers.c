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

/* powers.c contains functions related to power-ups and power-downs. */

#include <stdio.h>
#include <stdlib.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Called to activate the WideBat power-up. */
void activate_widebat(nbstate *state)
{
	/* Start the widebat timer: */
	state->powertimes.widebat = state->poweruptime;
	
	/* Cancel the NarrowBat power-down timer: */
	state->powertimes.narrowbat = 0;

	/* Set the new bat size: */
	change_bat(state, LARGEBAT);
}

/* Called to activate the SlowMotion power-up. */
void activate_slowmotion(nbstate *state)
{
	/* Start the slowmotion timer: */
	state->powertimes.slowmotion = state->poweruptime;

	/* Cancel the FastMotion power-down timer: */
	state->powertimes.fastmotion = 0;

	/* Set the new ball velocity: */
	state->ball.v = state->ball.sv;
}

/* Called to activate the StickyBat power-up. */
void activate_stickybat(nbstate *state)
{
	/* Start the stickybat timer: */
	state->powertimes.stickybat = state->poweruptime;
}

/* Called to activate the PowerBall power-up. */
void activate_powerball(nbstate *state)
{
	/* Start the powerball timer: */
	state->powertimes.powerball = state->poweruptime;
}

/* Called to activate the NarrowBat power-down. */
void activate_narrowbat(nbstate *state)
{
	/* Start the narrowbat timer: */
	state->powertimes.narrowbat = state->powerdowntime;

	/* Cancel the WideBat power-up timer: */
	state->powertimes.widebat = 0;

	/* Set the new bat size: */
	change_bat(state, SMALLBAT);
}

/* Called to activate the FastMotion power-down. */
void activate_fastmotion(nbstate *state)
{
	/* Start the fastmotion timer: */
	state->powertimes.fastmotion = state->powerdowntime;

	/* Cancel the SlowMotion power-up timer: */
	state->powertimes.slowmotion = 0;

	/* Set the new ball velocity: */
	state->ball.v = state->ball.fv;
}

/* Decrement a power timer, but don't allow it to go below 0. Returns the new
 * timer value if it was greater than or equal to 1 before the call, or -1
 * if it was 0. You can detect the change from 1 to 0 by checking for the
 * function returning 0. */
static int dec_power(int *i)
{
	int tmp = *i - 1; /* Decrement the timer. */
	if(tmp >= 0) *i = tmp; /* Store it unless it is < 0. */
	return tmp; /* Return the result or -1 if the timer was 0. */
}

/* Called once per second by handle_timer_event() to decrement all the power
 * timers and cancel them when they reach 0. */
void do_power_timers(nbstate *state)
{
	/* If the "No PowerUp TimeOut" cheat is active, don't decrement the
	 * power-up timers: */
	if(!state->flags.nputo) {
		/* Decrement the WideBat timer and set the bat size back to
		 * the normal size if it exires: */
		if(!dec_power(&state->powertimes.widebat))
			change_bat(state, NORMALBAT);
		/* Decrement the SlowMotion timer and set the ball velocity
		 * back to the normal speed if it expires: */
		if(!dec_power(&state->powertimes.slowmotion))
			state->ball.v = state->ball.nv;
		/* Decrement the StickyBat and PowerBall timers: */
		dec_power(&state->powertimes.stickybat);
		dec_power(&state->powertimes.powerball);
	}
	/* Decrement the NarrowBat timer and change the bat back to the normal
	 * size if it expires: */
	if(!dec_power(&state->powertimes.narrowbat))
		change_bat(state, NORMALBAT);
	/* Decrement the FastMotion timer and change the ball velocity back to
	 * the normal speed if it expires: */
	if(!dec_power(&state->powertimes.fastmotion))
		state->ball.v = state->ball.nv;
}

/* Called from destroy_brick() to create a new power box of the specified type
 * centred on the specified location. */
void new_power(nbstate *state, int type, int x, int y)
{
	power *p, *pn;
	sprite *s = state->powersprites[type];

	/* Allocate the new power structure: */
	if(!(p = malloc(sizeof(power)))) {
		oom();
		return;
	}

	/* Fill in the parameters: */
	p->type = type;
	p->y = y;

	/* Calculate the left hand coordinate of the power box (the passed in
	 * X coordinate specifies the centre, not the actual position): */
	p->x = x - (s->w / 2);

	/* Shift the box left or right if needed to make sure it is fully on
	 * the screen: */
	if(p->x < 0) p->x = 0;
	if(p->x + s->w > state->canvaswidth)
		p->x = state->canvaswidth - s->w;

	p->next = NULL; /* This is the last in the list so there is no next. */

	/* If the list is empty, make this power the head of the list: */
	if(!state->powers) state->powers = p;
	else {
		/* Otherwise loop through until the end of the list: */
		for(pn = state->powers; pn->next; pn = pn->next);
		pn->next = p; /* Tag this power onto the end of the list. */
	}

	/* Draw the new power: */
	draw_power(state, p, p->x, p->y, s->w, s->h);

	/* Draw the modified area of the canvas to the output window: */
	draw_canvas(state, p->x, p->y, s->w, s->h);
}

/* Draws all of the powers, limited to the specified rectangle, onto the
 * canvas. It is assumed that you have already redrawn the background and, if
 * necessary, the bricks under that area. */
void redraw_powers(nbstate *state, int x, int y, int w, int h)
{
	power *p;
	sprite *s;
	int px, py, pxx, pyy, pw, ph;

	/* For each of the powers in the current powers list: */
	for(p = state->powers; p; p = p->next) {

		/* Find the sprite to use for this power type: */
		s = state->powersprites[p->type];

		/* Calculate the left hand side of the resulting area: */
		if(x > p->x) px = x;
		else px = p->x;

		/* Calculate the top side of the resulting area: */
		if(y > p->y) py = y;
		else py = p->y;

		/* Calculate the right hand side of the resulting area: */
		if(x + w > p->x + s->w) pxx = p->x + s->w;
		else pxx = x + w;

		/* Calculate the bottom side of the resulting area: */
		if(y + h > p->y + s->h) pyy = p->y + s->h;
		else pyy = y + h;

		/* Calculate the size of the resulting area: */
		pw = pxx - px;
		ph = pyy - py;

		/* Only try to draw it if the resulting area is valid: */
		if(pw > 0 && ph > 0) draw_power(state, p, px, py, pw, ph);
	}
}

/* Unlinks the specified power structure from the list of powers. */
void destroy_power(nbstate *state, power *p)
{
	power *pp;

	/* If this is the head of the list, shuffle it up by one: */
	if(p == state->powers) state->powers = p->next;
	else {
		/* Find the previous entry in the power list: */
		for(pp = state->powers; pp && pp->next != p; pp = pp->next);
	
		/* Paranoid check to make sure we found it: */
		if(!pp) {
			fprintf(stderr, "Internal error: couldn't find "
					"power in list in destroy_power()\n");
			return;
		}

		/* Unlink it from the list: */
		pp->next = p->next;
	}

	/* Free the power: */
	free(p);
}

/* Called when a power box collides with the bat. Activates the relevant
 * power-up or power-down. */
void power_collision(nbstate *state, power *p)
{
	switch(p->type) {
		case POWERW:
			activate_widebat(state);
			break;
		case POWERS:
			activate_slowmotion(state);
			break;
		case POWERT:
			activate_stickybat(state);
			break;
		case POWERP:
			activate_powerball(state);
			break;
		case POWERN:
			activate_narrowbat(state);
			break;
		case POWERF:
			activate_fastmotion(state);
			break;
	}
}

/* Called every animation frame to move all of the power boxes down and perform
 * collision detection, etc. Doesn't draw the results to the canvas unless the
 * power collides with something and is destroyed. */
void animate_powers(nbstate *state)
{
	sprite *s;
	power *p, *pnext;

	/* For each power in the list: */
	for(p = state->powers; p; p = pnext) {
		/* Remember the next item first, because we may destroy this
		 * power, and then it would be wrong to use the pointer to
		 * find the pointer to the next one in the list: */
		pnext = p->next;

		/* Find the sprite for this power type: */
		s = state->powersprites[p->type];

		/* Clear the old power away: */
		draw_background(state, p->x, p->y, s->w, s->h);

		/* Redraw any bricks we may have accidentally erased: */
		draw_bricks(state, p->x, p->y, s->w, s->h);

		/* Move it down: */
		p->y += state->powerv;

		/* If it has reached the bottom of the screen: */
		if(p->y + s->h > state->canvasheight) {

			/* Copy the erased region to the screen: */
			draw_canvas(state, p->x, p->y - state->powerv,
							s->w, s->h);

			/* Destroy the power structure: */
			destroy_power(state, p);

			/* Check if it will collide with the bat: */
		} else if(p->y + s->h > state->canvasheight - state->batheight
				&& p->x + s->w > state->batx -
				(state->batwidths[state->bat] / 2) &&
				p->x < state->batx +
				(state->batwidths[state->bat] / 2)) {

			/* Copy the erased region to the screen: */
			draw_canvas(state, p->x, p->y - state->powerv,
							s->w, s->h);

			/* Activate the relevant power: */
			power_collision(state, p);

			/* Destroy the power structure: */
			destroy_power(state, p);

		} else {
			/* Redraw it in its new location: */
			draw_power(state, p, p->x, p->y, s->w, s->h);
		}
	}

	/* Redraw the ball in case we accidentally drew over it: */
	redraw_ball(state);

	/* Copy all the modified areas to the output window: */
	for(p = state->powers; p; p = p->next)
		draw_canvas(state, p->x, p->y - state->powerv, s->w,
				s->h + state->powerv);

}
