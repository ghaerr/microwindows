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

/* init.c includes init(), the function which initialises the game. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Print the usage message (used when bad command line arguments are given). */
static void usage(void)
{
	fprintf(stderr, "Usage: nbreaker [ -d game-dir ] [ game-file ]\n");
}

/* Parse the command line arguments. */
static int parse_cmd_args(nbstate *state, int argc, char *argv[])
{
	int i;

	/* For each of the arguments other than 0 (the program name): */
	for(i = 1; i < argc; i++) {
		/* If it doesn't start with a dash, assume it's the name of
		 * the game file: */
		if(*argv[i] != '-') state->gamefile = argv[i];
		else if(argv[i][1] == 'd') { /* If the parameter's "-d": */
			/* If there's another argument, use it as the game
			 * directory, otherwise print the usage message and
			 * return "failure": */
			if(++i < argc) state->gamedir = argv[i];
			else {
				usage();
				return 1;
			}
		} else {
			/* It was an unknown argument, so print the usage
			 * message and return "failure": */
			usage();
			return 1;
		}
	}
	return 0; /* Success. */
}

/* Set the contents of the state structure to default parameters. */
static void setup_default_state(nbstate *state)
{
	int i;
	GR_FONT_ID fid;
	GR_FONT_INFO fi;

	state->state = STATE_TITLESCREEN;
	state->gamedir = DEFAULT_GAME_DIR;
	state->gamefile = DEFAULT_GAME_FILE;
	state->titlebackground = NULL;
	state->titlebackgroundtiled = DEFAULT_BACKGROUND_TILED;
	state->titlesplash = NULL;
	state->gamewonsplash = NULL;
	state->gamelostsplash = NULL;
	state->spritelist = NULL;
	state->background = NULL;
	state->backgroundtiled = 1;
	state->normalpoints = DEFAULT_NORMALPOINTS;
	state->smallbonuspoints = DEFAULT_SMALLBONUSPOINTS;
	state->mediumbonuspoints = DEFAULT_MEDIUMBONUSPOINTS;
	state->largebonuspoints = DEFAULT_LARGEBONUSPOINTS;
	state->hugebonuspoints = DEFAULT_HUGEBONUSPOINTS;
	state->poweruppoints = DEFAULT_POWERUPPOINTS;
	state->powerdownpoints = DEFAULT_POWERDOWNPOINTS;
	state->startballs = DEFAULT_STARTBALLS;
	state->newlevelballs = DEFAULT_NEWLEVELBALLS;
	state->brickwidth = DEFAULT_BRICK_WIDTH;
	state->brickheight = DEFAULT_BRICK_HEIGHT;
	state->bricks = NULL;
	state->brickalpha = 0;
	state->width = DEFAULT_WIDTH;
	state->height = DEFAULT_HEIGHT;
	state->batheight = DEFAULT_BAT_HEIGHT;
	state->batwidths[NORMALBAT] = DEFAULT_NORMALBAT_WIDTH;
	state->batwidths[SMALLBAT] = DEFAULT_SMALLBAT_WIDTH;
	state->batwidths[LARGEBAT] = DEFAULT_LARGEBAT_WIDTH;
	for(i = 0; i < NUMBATS; i++) state->bats[i] = NULL;
	state->bat = NORMALBAT;
	state->batx = 0;
	state->batv = DEFAULT_BAT_VELOCITY;
	state->powerv = DEFAULT_POWER_VELOCITY;
	state->animateperiod = DEFAULT_ANIMATE_PERIOD;
	for(i = 0; i < NUMPOWERS; i++) state->powersprites[i] = NULL;
	state->splash = NULL;
	state->poweruptime = DEFAULT_POWERUP_TIME;
	state->powerdowntime = DEFAULT_POWERDOWN_TIME;
	for(i = 0; i < NUMCHEATS; i++) state->cheats[i] = NULL;
	memset(state->cheatstate, 0, MAXCHEATLEN + 1);
	state->flags.sf = 0;
	state->flags.nb = 0;
	state->flags.npd = 0;
	state->flags.nputo = 0;
	state->flags.paused = 0;
	state->flags.left = 0;
	state->flags.right = 0;
	state->levels = NULL;
	state->level = 0;
	state->numlevels = 0;
	state->wid = 0;
	state->winx = 0;
	state->canvas = 0;
	state->canvaswidth = 0;
	state->canvasheight = 0;
	state->grid = NULL;
	state->numbricks = 0;
	state->powers = NULL;
	state->scores.s = 0;
	state->scores.hi = 0;
	state->scores.fhi = 0;
	state->scores.p = 0;
	state->gc = GrNewGC();
	fid = GrCreateFont(SCORE_FONT, 0, NULL);
	GrGetFontInfo(fid, &fi);
	state->scores.h = (2 * SCORE_BORDER) + fi.height;
	GrSetGCFont(state->gc, fid);
	state->ball.x = 0;
	state->ball.y = 0;
	state->ball.d = 0;
	state->ball.v = DEFAULT_NORMAL_BALL_VELOCITY;
	state->ball.lx = 0;
	state->ball.ly = 0;
	state->ball.parked = 1;
	state->ball.s = NULL;
	state->ball.sv = DEFAULT_SLOW_BALL_VELOCITY;
	state->ball.nv = DEFAULT_NORMAL_BALL_VELOCITY;
	state->ball.fv = DEFAULT_FAST_BALL_VELOCITY;
	state->numballs = 0;
	gettimeofday(&state->lastanim, NULL);
	state->powertimes.widebat = 0;
	state->powertimes.slowmotion = 0;
	state->powertimes.stickybat = 0;
	state->powertimes.powerball = 0;
	state->powertimes.narrowbat = 0;
	state->powertimes.fastmotion = 0;
	state->faderate = DEFAULT_FADERATE;
	state->fadelevel = 0;
	state->nextstate = STATE_TITLESCREEN;
}

/* Initialise the game and return the state structure or NULL on failure: */
nbstate *init(int argc, char *argv[])
{
	GR_PROP *prop;
	nbstate *state;
	GR_SCREEN_INFO si;
	GR_BITMAP cursor = 0;
	GR_WM_PROPERTIES props;

	/* Try to connect to the Nano-X server: */
	if(GrOpen() < 1) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return NULL;
	}

	/* Check that the server was built with alpha blending support
	 * (necessary for the alpha blended sprites and special effects): */
	GrGetScreenInfo(&si);
	if(!si.alphablend) {
		fprintf(stderr, "Error: Nano-X server was built without alpha "
				"blending support\nSet ALPHABLENDING = 1 in "
				"include/device.h, rebuild, and try again.\n");
		return NULL;
	}

	/* Allocate the state structure and initialise it with defaults: */
	if(!(state = malloc(sizeof(nbstate)))) return NULL;
	setup_default_state(state);

	/* Try to parse the command line arguments: */
	if(parse_cmd_args(state, argc, argv)) {
		free(state);
		return NULL;
	}

	/* Try to load the game file: */
	if(load_game_file(state)) {
		free(state);
		return NULL;
	}

	/* Load the high score file: */
	load_hiscore(state);

	/* Calculate the canvas size: */
	state->canvaswidth = state->width * state->brickwidth;
	state->canvasheight = state->scores.h + state->ball.s->h +
				(2 * BALLS_BORDER) +
				(state->height * state->brickheight) +
				3 * state->brickheight + state->batheight;

	/* Place the bat in the centre of the window: */
	state->batx = state->canvaswidth / 2;

	/* Create various pixmaps and alpha channels: */
	state->scores.p = GrNewPixmap(state->canvaswidth, state->scores.h,
			NULL);
	state->scores.a = GrNewAlpha(state->canvaswidth, state->scores.h);
	state->canvas = GrNewPixmap(state->canvaswidth, state->canvasheight,
			NULL);
	/* The new and old canvasses are only used when screen fading is
	 * enabled: */
	if(state->faderate) {
		state->newcanvas = GrNewPixmap(state->canvaswidth,
					state->canvasheight, NULL);
		state->oldcanvas = GrNewPixmap(state->canvaswidth,
					state->canvasheight, NULL);
	}
	state->brickalpha = GrNewAlpha(state->brickwidth, state->brickheight);

	/* Start off with the canvas completely black: */
	GrSetGCForeground(state->gc, GR_COLOR_BLACK);
	GrFillRect(state->canvas, state->gc, 0, 0, state->canvaswidth,
						state->canvasheight);

	/* If there is a window manager running, place the window off the
	 * screen and let the window manager move it where it wants (avoids
	 * flicker if we were to draw the window on screen before the window
	 * manager moved it somewhere else): */
        if(GrGetWindowProperty(GR_ROOT_WINDOW_ID, "WINDOW_MANAGER", &prop)) {
		free(prop);
		state->winx = GR_OFF_SCREEN;
	}

	/* Create the output window: */
	state->wid = GrNewWindow(GR_ROOT_WINDOW_ID, state->winx, 0,
			state->canvaswidth, state->canvasheight, 0, 0, 0);

	/* Set the window title: */
	props.flags = GR_WM_FLAGS_TITLE;
	props.title = "NanoBreaker";
	GrSetWMProperties(state->wid, &props);

	/* Make the cursor over the output window be invisible: */
	GrSetCursor(state->wid, 1, 1, 1, 1, 0, 0, &cursor, &cursor);

	/* Select the events we want to receive for the output window: */
	GrSelectEvents(state->wid, GR_EVENT_MASK_CLOSE_REQ |
				GR_EVENT_MASK_EXPOSURE |
				GR_EVENT_MASK_BUTTON_DOWN |
				GR_EVENT_MASK_KEY_DOWN |
				GR_EVENT_MASK_KEY_UP |
				GR_EVENT_MASK_UPDATE |
				GR_EVENT_MASK_TIMER);

	/* Select for mouse position events on the root window so we can move
	 * the bat even when the pointer strays outside the window (which it
	 * often does because the cursor is invisible you don't know exactly
	 * where it is): */
	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_MOUSE_POSITION);

	/* Map the output window (make it visible): */
	GrMapWindow(state->wid);

	/* Create the one second periodic timer that is used to decrement the
	 * power-up and power-down timers: */
	state->tid = GrCreateTimer(state->wid, 1000, GR_TRUE);

	/* Reset the game: */
	reset_game(state);

	return state; /* Return the newly allocated state structure. */
}
