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

/* nbreaker.c contains main() and the main event loop. */

#include <stdio.h>
#include <stdlib.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/*
 * Clean up various things when the game exits, including saving the high
 * score and closing the connection to the server. If THOROUGH_CLEANUP is
 * defined in nbreaker.h, also destroy every Nano-X resource before closing
 * the connection and free every piece of malloced memory (technically not
 * necessary as Nano-X and the OS can do those for you, but it can be useful
 * when using a debugging malloc to track down memory leaks).
 */
static void cleanup(nbstate *state)
{
	int i;
#ifdef THOROUGH_CLEANUP
	brick *b, *bnext = state->bricks;
	level *l, *lnext = state->levels;
	power *p, *pnext = state->powers;
#endif

	save_hiscore(state); /* Save the high score. */

#ifdef THOROUGH_CLEANUP
	/* Free the global brick list. We don't need to free the sprites
	 * because destroy_all_sprites() does that more efficiently: */
	while((b = bnext)) {
		bnext = b->next;
		free(b);
	}
	
	destroy_all_sprites(state); /* Destroy every sprite. */

	/* Destroy the remaining Nano-X windows, pixmaps, timers, etc. */
	GrDestroyWindow(state->wid);
	GrDestroyWindow(state->canvas);
	/* The new and old canvasses are only needed when fading is enabled: */
	if(state->faderate) {
		GrDestroyWindow(state->newcanvas);
		GrDestroyWindow(state->oldcanvas);
	}
	GrDestroyWindow(state->scores.p);
	GrDestroyWindow(state->brickalpha);
	GrDestroyTimer(state->tid);

	/* Free various global strings, if they exist: */
	myfree(state->titlebackground);
	myfree(state->titlesplash);
	myfree(state->gamewonsplash);
	myfree(state->gamelostsplash);

	/* Free the cheat sequences: */
	for(i = 0; i < NUMCHEATS; i++) myfree(state->cheats[i]);

	/* Destroy every level: */
	while((l = lnext)) {
		lnext = l->next;
		bnext = l->bricks;
		/* The level specific bricks: */
		while((b = bnext)) {
			bnext = b->next;
			free(b);
		}
		myfree(l->grid); /* The level brick area grid. */
		myfree(l->backgroundname);
		free(l);
	}

	myfree(state->grid); /* The current brick area grid. */

	/* Free all the power structures: */
	while((p = pnext)) {
		pnext = p->next;
		free(p);
	}

	free(state);
#endif
	GrClose(); /* Close the connection to the Nano-X server. */
}

/*
 * Calculates the time remaining to the next animate time in milliseconds. If
 * we've missed the time slot already, the result will be negative.
 */
static long calc_timeout(nbstate *state)
{
	long elapsed;
	struct timeval tv;

	gettimeofday(&tv, NULL); /* Get the current time. */

	/* Calculate how long it is in milliseconds since the last time
	 * animate() was called: */
	elapsed = ((tv.tv_sec - state->lastanim.tv_sec) * 1000) +
			((tv.tv_usec - state->lastanim.tv_usec) / 1000);

	/* Return the time remaining to the next animate() slot: */
	return state->animateperiod - elapsed;
}

/*
 * The main game event loop.
 */
static void nbreaker(nbstate *state)
{
	GR_EVENT ev;
	long timeout;

	do {
		/* When the game is running, calculate how long remains to the
		 * next animate() time slot: */
		if(state->state == STATE_RUNNING ||
				state->state == STATE_FADING) {
			if((timeout = calc_timeout(state)) <= 0) {
				/* The time slot has already passed, so store
				 * the current time as the "last animate()"
				 * time and call the animate() function: */
				gettimeofday(&state->lastanim, NULL);
				animate(state);
				/* This shouldn't be necessary under normal
				 * circumstances, but if animate() takes
				 * longer then the animate time, don't call it
				 * again (we need to go through the event
				 * loop) but do set the timeout as low as
				 * possible (not 0 because that means "never
				 * time out"): */
				if((timeout = calc_timeout(state)) <= 0)
					timeout = 1;
			}
		} else timeout = 0; /* Game isn't running, so don't time out. */

		/* Get the next event from the Nano-X server. If the call times
		 * out, we will get a GR_EVENT_TYPE_TIMEOUT. */
		GrGetNextEventTimeout(&ev, timeout);

		/* Call the event handler functions for each of the event
		 * types: */
		switch(ev.type) {
			case GR_EVENT_TYPE_CLOSE_REQ:
				state->state = STATE_FINISHED;
				break;
			case GR_EVENT_TYPE_EXPOSURE:
				handle_exposure_event(state, &ev.exposure);
				break;
			case GR_EVENT_TYPE_BUTTON_DOWN:
				handle_button_event(state, &ev.button);
				break;
			case GR_EVENT_TYPE_MOUSE_POSITION:
				handle_mouse_event(state, &ev.mouse);
				break;
			case GR_EVENT_TYPE_KEY_UP:
			case GR_EVENT_TYPE_KEY_DOWN:
				if(handle_keystroke_event(state, &ev.keystroke))
					state->state = STATE_FINISHED;
				break;
			case GR_EVENT_TYPE_UPDATE:
				handle_update_event(state, &ev.update);
				break;
			case GR_EVENT_TYPE_TIMER:
				handle_timer_event(state, &ev.timer);
				break;
			case GR_EVENT_TYPE_TIMEOUT:
				/* Do nothing on time out. */
				break;
			default:
				fprintf(stderr, "Got unknown event type %d\n",
						ev.type);
				break;
		}
	/* If the current state is STATE_FINISHED (typically because we've
	 * been asked to quit), exit the event loop: */
	} while(state->state != STATE_FINISHED);
}

int main(int argc, char *argv[])
{
	nbstate *state;

	if(!(state = init(argc, argv))) return 1; /* Initialise the game. */
	nbreaker(state); /* The main event loop. */
	cleanup(state); /* Clean up the game state before exit. */

	return 0; /* Success. */
}
