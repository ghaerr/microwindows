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

/* brick.c contains functions related to bricks. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Destroy the specified brick, clear the relevant piece of the game area,
 * increment the score by the appropriate amount, decrement the current
 * number of bricks, and if we run out of bricks, increment to the next level.
 * Returns 0 if the level wasn't incremented and 1 if it was. */
static int destroy_brick(nbstate *state, grid *g)
{
	int immutable;

	/* Wipe the canvas where the brick is back to the background: */
	draw_background(state, g->x, g->y, state->brickwidth,
			state->brickheight);
	/* Redraw any powers we may just have accidentally erased: */
	redraw_powers(state, g->x, g->y, state->brickwidth, state->brickheight);
	/* Copy the affected area to the output window: */
	draw_canvas(state, g->x, g->y, state->brickwidth,
			state->brickheight);

	/* Increment the score by the appropriate amount: */
	if(g->b->flags & BRICK_FLAG_HUGE_BONUS)
		increment_score(state, state->hugebonuspoints);
	else if(g->b->flags & BRICK_FLAG_LARGE_BONUS)
		increment_score(state, state->largebonuspoints);
	else if(g->b->flags & BRICK_FLAG_MEDIUM_BONUS)
		increment_score(state, state->mediumbonuspoints);
	else if(g->b->flags & BRICK_FLAG_SMALL_BONUS)
		increment_score(state, state->smallbonuspoints);
	else increment_score(state, state->normalpoints);

	/* Remember whether this was an immutable brick (which does not factor
	 * when counting the number of bricks remaining because it cannot
	 * normally be destroyed- only the PowerBall power-up and the
	 * NoBounce cheat can cause immutable bricks to be destroyed. */
	immutable = g->b->flags & BRICK_FLAG_IMMUTABLE;
	g->b = NULL; /* Wipe the brick from this grid location. */

	/* If this wasn't an immutable brick, decrement the current count of
	 * bricks and if we run out, increment to the next level and return. */
	if(!immutable && !--state->numbricks) {
		increment_level(state);
		return 1; /* Level was incremented. */
	}

	/* Create the power-up or power-down (if any): */
	if(g->power != NOPOWER)
		new_power(state, g->power, g->x + (state->brickwidth / 2),
									g->y);

	return 0; /* Level wasn't incremented. */
}

/* Called whenever the ball collides with a brick, and handles such things as
 * destroying the brick if appropriate, redrawing it with more transparency
 * if appropriate, incrementing to the next level when we destroy the last brick
 *  (actually, that's done by destroy_brick() which is called from
 * brick_collision()), etc. Returns 0 if the level was not incremented and 1
 * if it was. */
int brick_collision(nbstate *state, grid *g)
{
	/* If either the PowerBall power-up or the NoBounce cheat is active,
	 * simply destroy the brick in one hit even if it is immutable. */
	if(state->powertimes.powerball || state->flags.nb)
		return destroy_brick(state, g);

	/* If this is an immutable brick and PowerBall and NoBounce are not
	 * active, it can't be destroyed so return normally: */
	if(g->b->flags & BRICK_FLAG_IMMUTABLE) return 0;

	/* Increment the number of hits on this brick. This is used to keep
	 * track of when to destroy a "2 hits or "3 hits" brick, and also how
	 * transparent to draw those bricks as: */
	g->hits++;

	/* If this is a 2-hit brick and it has only been hit once or it is a
	 * 3 hit brick that has been hit either once or twice: */
	if(((g->b->flags & BRICK_FLAG_2_HITS) && g->hits < 2) ||
			((g->b->flags & BRICK_FLAG_3_HITS) && g->hits < 3)) {
		/* Clear the area where the brick is: */
		draw_background(state, g->x, g->y, state->brickwidth,
				state->brickheight);
		/* Redraw the brick (with additional transparency to indicate
		 * that it has been hit): */
		draw_brick(state, g, 0, 0, state->brickwidth,
				state->brickheight);
		/* Redraw any powers we may just have accidentally erased: */
		redraw_powers(state, g->x, g->y, state->brickwidth,
				state->brickheight);
		/* Copy the changed area to the output window: */
		draw_canvas(state, g->x, g->y, state->brickwidth,
				state->brickheight);
	/* Otherwise, destroy the brick: */
	} else return destroy_brick(state, g);

	return 0; /* Level was not incremented. */
}
