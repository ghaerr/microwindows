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

/* event.c contains handlers for (almost) all of the Nano-X events we get. */

#include <stdio.h>
#include <stdlib.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Called when an "action" input (ie. a mouse button or the space bar) is
 * pressed. Starts the game if we're on the title screen, jumps to the title
 * screen if the game is over, or unparks the ball if the game is running and
 * the ball is parked. */
static void action_pressed(nbstate *state)
{
	if(state->state == STATE_TITLESCREEN) increment_level(state);
	else if(state->state == STATE_GAMEWON ||
			state->state == STATE_GAMELOST)
		reset_game(state);
	else state->ball.parked = 0;
}

/* Because the canvas is a full double buffer, all we need to do when we get
 * an exposure event is to copy the specified area of the canvas to the
 * output window. */
void handle_exposure_event(nbstate *state, GR_EVENT_EXPOSURE *ev)
{
	draw_canvas(state, ev->x, ev->y, ev->width, ev->height);
}

/* When a mouse button (any button) is pressed, treat it as an "action". */
void handle_button_event(nbstate *state, GR_EVENT_BUTTON *ev)
{
	action_pressed(state);
}

/* When the mouse moves, move the bat. */
void handle_mouse_event(nbstate *state, GR_EVENT_MOUSE *ev)
{
	move_bat(state, ev->x - state->winx);
}

/* The key event handler. */
int handle_keystroke_event(nbstate *state, GR_EVENT_KEYSTROKE *ev)
{
	/* If the event is a "key up" event and the key is either the left or
	 * right cursor key, cancel bat movement in that direction. Otherwise
	 * we don't care about the event. */
	if(ev->type == GR_EVENT_TYPE_KEY_UP) {
		switch(ev->ch) {
			case MWKEY_LEFT:
				state->flags.left = 0;
				break;
			case MWKEY_RIGHT:
				state->flags.right = 0;
				break;
			default:
				/* Ignore the event. */
				break;
		}
		return 0; /* Normal return. */
	}

	/* If we get to here, it's a "key down" event. Perform various
	 * different actions depending on which key was pressed: */
	switch(ev->ch) {
		case ' ': /* The space bar is the "action" key: */
			action_pressed(state);
			break;
		case MWKEY_F1: /* F1 is the "pause" key: */
			state->flags.paused ^= 1; /* Set the pause flag. */
			break;
		case MWKEY_F2: /* F2 is the "suicide" key: */
			lost_ball(state); /* Throw away the current ball. */
			break;
#ifdef DEBUG_POWERS
		/* If DEBUG_POWERS is defined in nbreaker.h, keys F3-F8 are
		 * used to trigger the power ups and power downs without
		 * needing to catch a falling power box. */
		case MWKEY_F3: /* WideBat */
			activate_widebat(state);
			break;
		case MWKEY_F4: /* SlowMotion */
			activate_slowmotion(state);
			break;
		case MWKEY_F5: /* StickyBat */
			activate_stickybat(state);
			break;
		case MWKEY_F6: /* PowerBall */
			activate_powerball(state);
			break;
		case MWKEY_F7: /* NarrowBat */
			activate_narrowbat(state);
			break;
		case MWKEY_F8: /* FastMotion */
			activate_fastmotion(state);
			break;
#endif
		case MWKEY_LEFT: /* The left cursor key: */
			/* Set the "moving left" flag and cancel the
			 * "moving right" one. */
			state->flags.left = 1;
			state->flags.right = 0;
			break;
		case MWKEY_RIGHT: /* The right cursor key: */
			/* Set the "moving right" flag and cancel the
			 * "moving left" one. */
			state->flags.left = 0;
			state->flags.right = 1;
			break;
#ifdef NB_DEBUG
		/* If NB_DEBUG is set in nbreaker.h, dump the game state when
		 * F10 is pressed. */
		case MWKEY_F10:
			dump_state(state);
			break;
#endif
		/* If the escape key is pressed, return 1 which means "exit
		 * the game". */
		case MWKEY_ESCAPE:
			return 1;

		/* Feed all other key presses to the cheat recogniser engine: */
		default:
			do_cheat(state, ev->ch);
			break;
	}

	return 0; /* Normal return. */
}

/* Handle update events (window move, etc.) by setting the output window X
 * dimension to the root X dimension specified in the event structure. This is
 * used in move_bat() to convert the absolute dimensions we get in mouse
 * position events because we select for them on the root window (so we can
 * still move the bat when the cursor strays out of the window). */
void handle_update_event(nbstate *state, GR_EVENT_UPDATE *ev)
{
	state->winx = ev->rootx;
}

/* Handle a timer event. Currently there is only one timer; a 1 second
 * periodic timer that is used to decrement the power-up and power-down timers
 * once per second, and cancel the powers when the timers reach 0. */
void handle_timer_event(nbstate *state, GR_EVENT_TIMER *ev)
{
	do_power_timers(state);
}
