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

/* levels.c contains functions functions that deal with the game level. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Increment to the next level. */
void increment_level(nbstate *state)
{
	/* If we are already on the last level: */
	if(state->level == state->numlevels) {
		/* Go the the "game won" state. */
		state->state = STATE_GAMEWON;
		/* Update the high score and save it if appropriate: */
		save_hiscore(state);
	} else { /* Not on the last level. */
		/* If we're on the title screen, go to the running state. */
		if(state->state == STATE_TITLESCREEN) {
			state->state = STATE_RUNNING;
		/* Otherwise give the player some more balls for completing a
		 * level: */
		} else state->numballs += state->newlevelballs;
		state->level++; /* Increment the level counter. */
	}
	/* We always start a new level with the ball parked: */
	park_ball(state);
	/* Draw the new level and set up various other things for it: */
	set_level_active(state);
}

/* Make the current level active. This mainly consists of destroying the old
 * background image and loading the new one, destroying the splash image,
 * possibly loading a new splash image depending on the state, copying over the
 * new level data into the current level state, then redrawing the entire game
 * area. */
void set_level_active(nbstate *state)
{
	int i;
	level *lev;
	grid *g, *gg;
	int bgchanged;
	sprite *s = NULL;
	power *p, *pnext;
	GR_PIXMAP_ID ctmp;
	char *backgroundfile;

	/* Destroy the old splash image sprite: */
	destroy_sprite(state, state->splash);

	/* If we're on the title screen: */
	if(state->state == STATE_TITLESCREEN) {
		/* Set the background file to the title background file: */
		backgroundfile = state->titlebackground;
		/* Set the tiled state appropriately: */
		state->backgroundtiled = state->titlebackgroundtiled;
		/* Try to load the title screen splash graphic (if it doesn't
		 * work nothing bad will happen- load_sprite() will print an
		 * error message and draw_splash() will not draw anything.) */
		state->splash = load_sprite(state, state->titlesplash, -1, -1);
	} else { /* Not on the title screen. */
		/* Find the level structure for the current level number: */
		for(lev = state->levels, i = 1; i < state->level;
				lev = lev->next, i++);
		/* Set the current number of bricks and background info: */
		state->numbricks = lev->numbricks;
		backgroundfile = lev->backgroundname;
		state->backgroundtiled = lev->backgroundtiled;
		/* If we're in the "game won" state, try to load the appropriate
		 * splash image: */
		if(state->state == STATE_GAMEWON) {
			state->splash = load_sprite(state, state->gamewonsplash,
									-1, -1);
		/* If we're in the "game lost" state, try to load the
		 * appropriate splash image: */
		} else if(state->state == STATE_GAMELOST) {
			state->splash = load_sprite(state,
						state->gamelostsplash, -1, -1);
		} else { /* We must be in the STATE_RUNNING state. */
			/* No splash image: */
			state->splash = NULL;
			/* Copy this levels game grid into the current game
			 * grid: */
			g = state->grid;
			gg = lev->grid;
			for(i = 0; i < state->width * state->height; i++)
				*g++ = *gg++;
		}
	}

	/* If there was a background filename specified: */
	if(backgroundfile) {
		/* If there is a current background sprite with a filename
		 * and the filename is the same as the new background
		 * filename, the background file has not changed. Otherwise,
		 * assume that it has. */
		if(state->background && state->background->fname &&
				!strcmp(backgroundfile,
					state->background->fname))
			bgchanged = 0;
		else bgchanged = 1;
	/* No background filename was specified, so assume it has changed (to
	 * a blank black frame): */
	} else bgchanged = 1;

	/* If the background image has changed, try to load the new one: */
	if(bgchanged && !(s = load_sprite(state, backgroundfile, -1, -1))) {
		/* If it fails, try to make a new empty sprite and colour it
		 * in black (the 16*16 pixels is purely arbitrary- make it too
		 * large and it uses a lot of memory, make it too small and
		 * we spend ages painting hundreds of tiny tiles onto the
		 * background. */
		if(!(s = make_empty_sprite(state, backgroundfile, 16, 16))) {
			/* If that fails too (shouldn't happen under normal
			 * circumstances), issue a warning and keep the old
			 * background image sprite: */
			s = state->background;
		} else {
			/* Fill in the new dummy background black: */
			GrSetGCForeground(state->gc, GR_COLOR_BLACK);
			GrFillRect(s->p, state->gc, 0, 0, 16, 16);
			/* Make it tiled. FIXME: it would make more sense to
			 * have a "no background image" option which simply
			 * coloured in the background black: */
			state->backgroundtiled = 1;
		}
	}

	/* If we have made a new background image sprite: */
	if(bgchanged && s != state->background) {
		/* Destroy the old one: */
		destroy_sprite(state, state->background);
		/* Set the background to the new sprite: */
		state->background = s;
	}

	/* Empty the list of power boxes: */
	for(p = state->powers; p; p = pnext) {
		pnext = p->next;
		free(p);
	}
	state->powers = NULL;

	/* If fading has been requested, we want to fade the new level in, so
	 * swap the canvasses around so the current screen is the old canvas,
	 * then draw the new screen and make the new canvas, then start the
	 * process of fading it in: */
	if(state->faderate) {
		/* Swap the canvasses around: */
		ctmp = state->oldcanvas;
		state->oldcanvas = state->canvas;
		state->canvas = ctmp;

		/* Remember the state we're fading into: */
		state->nextstate = state->state;

		/* Go into the fading state: */
		state->state= STATE_FADING;

		/* Initialise the fade level as completely opaque: */
		state->fadelevel = 256;
	}

	/* Clear the whole game area to the background image: */
	draw_background(state, 0, 0, state->canvaswidth, state->canvasheight);

	/* If we're not on the title screen or fading to the title screen: */
	if(state->state != STATE_TITLESCREEN && !(state->state == STATE_FADING
				&& state->nextstate == STATE_TITLESCREEN)) {
		/* Draw the bricks: */
		draw_bricks(state, 0, 0, state->canvaswidth,
						state->canvasheight);
		draw_scores(state); /* Draw the scores bar. */
		draw_balls(state); /* Draw the row of balls. */
		draw_bat(state); /* Draw the bat. */
		/* Draw the current ball unless the game is over: */
		if(state->state != STATE_GAMELOST &&
				state->state != STATE_GAMEWON)
			draw_ball(state);
	}

	draw_splash(state); /* Draw the splash graphic (if there is one). */

	/* If we're fading, remember the new canvas and generate the first
	 * frame of the fade: */
	if(state->state == STATE_FADING) {

		/* Swap the canvasses around: */
		ctmp = state->newcanvas;
		state->newcanvas = state->canvas;
		state->canvas = ctmp;

		/* Reduce the opacity: */
		state->fadelevel -= state->faderate;

		/* Generate the first frame: */
		GrCopyArea(state->canvas, state->gc, 0, 0, state->canvaswidth,
				state->canvasheight, state->newcanvas, 0, 0, 0);
		GrCopyArea(state->canvas, state->gc, 0, 0, state->canvaswidth,
				state->canvasheight, state->oldcanvas, 0, 0,
				GR_CONST_BLEND | state->fadelevel);
	}

	/* Copy the entire redrawn canvas to the output window: */
	draw_canvas(state, 0, 0, state->canvaswidth, state->canvasheight);
}
