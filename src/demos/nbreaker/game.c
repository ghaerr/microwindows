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

/* game.c contains some overall game functions. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Reset the game state, called prior to starting a new game. */
void reset_game(nbstate *state)
{
	/* Start on the title screen, level 0: */
	state->state = STATE_TITLESCREEN;
	state->level = 0;
	state->scores.s = 0; /* Start with a score of 0. */
	state->numballs = state->startballs; /* Set the initial balls. */
	park_ball(state); /* Park the current ball. */
	/* Reset all the power-up and power-down timers: */
	state->powertimes.widebat = 0;
	state->powertimes.slowmotion = 0;
	state->powertimes.stickybat = 0;
	state->powertimes.powerball = 0;
	state->powertimes.narrowbat = 0;
	state->powertimes.fastmotion = 0;
	/* Reset to the normal bat size: */
	state->bat = NORMALBAT;
	/* Reset to the normal ball velocity: */
	state->ball.v = state->ball.nv;
	/* Draw the title screen: */
	set_level_active(state);
}
