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

/* dump.c contains dump_state(), but only if NB_DEBUG is defined in
 * nbreaker.h. */

#include <stdio.h>

#include <nano-X.h>
#include "nbreaker.h"

#ifdef NB_DEBUG

/* Returns the string passed to it, or a pointer to the string "<unnamed>" if
 * the passed string is NULL. Used when printing the file names in the state,
 * which may be NULL if they don't have a name. */
static char *fname(char *f)
{
	return f ? f : "<unnamed>";
}

/* Prints out the contents of the specified brick list. */
static void dump_bricks(brick *b)
{
	/* Print "<empty>" if the list is empty: */
	if(!b) {
		printf("\t<empty>\n");
		return;
	}

	/* Iterate through the list: */
	while(b) {
		/* Print the identifier (the character that symbolises the
		 * brick in the game file): */
		printf("\t\"%c\"", b->identifier);
		/* If there is a sprite associated with this brick, print the
		 * name of the file the sprite was loaded from: */
		if(b->s) printf(" (\"%s\")", fname(b->s->fname));
		/* Print the flags associated with this brick: */
		if(b->flags & BRICK_FLAG_IMMUTABLE) printf(" Immutable");
		if(b->flags & BRICK_FLAG_2_HITS) printf(" 2hits");
		if(b->flags & BRICK_FLAG_3_HITS) printf(" 3hits");
		if(b->flags & BRICK_FLAG_SMALL_BONUS) printf(" smallbonus");
		if(b->flags & BRICK_FLAG_MEDIUM_BONUS) printf(" mediumbonus");
		if(b->flags & BRICK_FLAG_LARGE_BONUS) printf(" largebonus");
		if(b->flags & BRICK_FLAG_HUGE_BONUS) printf(" hugebonus");
		printf("\n");
		b = b->next;
	}
}

/* Prints out the specified game grid (it's assumed that if it exists, it is
 * state->width * state->height in size, which should always be true): */
static void dump_grid(nbstate *state, grid *g)
{
	grid *gp;
	int x, y;
	/* The letters that symbolise each of the power-ups and power-downs: */
	char powers[] = " WSTPNFRI";

	/* If the grid is empty, just print "<empty>" and return: */
	if(!g) {
		printf("\t<empty>\n");
		return;
	}

	/* Print the title and top line of the bricks description box: */
	printf("\tbricks:\n\t\t");
	for(x = 0; x < state->width + 2; x++) putchar('-');
	putchar('\n');

	gp = g;
	for(y = 0; y < state->height; y++) { /* For each grid row. */
		printf("\t\t|"); /* Left side of the box. */
		for(x = 0; x < state->width; x++) { /* For each grid column. */
			/* Print a space if there is no brick in this grid
			 * box or the brick identifier if there is one: */
			if(!gp->b) putchar(' ');
			else putchar(gp->b->identifier);
			gp++;
		}
		printf("|\n"); /* Right side of the box. */
	}
	/* The bottom line of the box: */
	printf("\t\t");
	for(x = 0; x < state->width + 2; x++) putchar('-');

	/* Print the title and top line of the powers description box: */
	printf("\n\tpowers:\n\t\t");
	for(x = 0; x < state->width + 2; x++) putchar('-');
	putchar('\n');

	gp = g;
	for(y = 0; y < state->height; y++) { /* For each grid row. */
		printf("\t\t|"); /* Left side of the box. */
		for(x = 0; x < state->width; x++) { /* For each grid row. */
			/* Print the symbol associated with this power: */
			putchar(powers[gp->power + 1]);
			gp++;
		}
		printf("|\n"); /* Right side of the box. */
	}
	/* The bottom line of the box: */
	printf("\t\t");
	for(x = 0; x < state->width + 2; x++) putchar('-');
	putchar('\n');
}

/* Print the contents of each level in turn: */
static void dump_levels(nbstate *state)
{
	level *l;
	int num = 1;

	/* If there are no levels, just print "<empty>" and return: */
	printf("Levels:\n");
	if(!state->levels) {
		printf("\t<empty>\n");
		return;
	}

	/* Iterate through the list of levels: */
	for(l = state->levels; l; l = l->next) {
		/* Print the level number: */
		printf("\tLevel %d\n\tlevel bricks:\n", num++);
		/* Print out the level specific bricks: */
		dump_bricks(l->bricks);
		/* Various other information associated with this level: */
		printf("\tbackgroundname = \"%s\"\n", fname(l->backgroundname));
		printf("\tbackgroundtiled = %s\n", l->backgroundtiled ?
							"Yes" : "No");
		printf("\tnumbricks = %d\n", l->numbricks);
		/* Print the actual brick area definition: */
		dump_grid(state, l->grid);
	}
}

/* Print the ASCII name of the specified state number. */
static void printstate(int state)
{
	switch(state) {
		case STATE_TITLESCREEN:
			printf("STATE_TITLESCREEN\n");
			break;
		case STATE_FADING:
			printf("STATE_FADING\n");
			break;
		case STATE_RUNNING:
			printf("STATE_RUNNING\n");
			break;
		case STATE_GAMEWON:
			printf("STATE_GAMEWON\n");
			break;
		case STATE_GAMELOST:
			printf("STATE_GAMELOST\n");
			break;
		case STATE_FINISHED:
			printf("STATE_FINISHED\n");
			break;
		default:
			printf("unknown (%d)\n", state);
			break;
	}
}

/* Dumps (almost) the entire game state on request. Called by
 * handle_keystroke_event() when the F10 key is pressed. */
void dump_state(nbstate *state)
{
	int i;
	power *p;
	sprite *s;
	/* The letters symbolising each of the powers: */
	char powers[NUMPOWERS] = "WSTPNF";
	/* The names of the cheats: */
	char *cheats[] = { "SolidFloor", "Teleport", "NoBounce",
		"NoPowerDown", "NoPowerUpTimeout" };

	/* Print the title: */
	printf("Printing NanoBreaker Game State\n"
		"-------------------------------\n");

	/* Print the current game state: */
	printf("state = ");
	printstate(state->state);

	/* Print the name of the game directory and the game file: */
	printf("gamedir = \"%s\"\ngamefile = \"%s\"\n", fname(state->gamedir),
		fname(state->gamefile));

	/* Print the name of the title screen background image and whether it
	 * should be tiled or not: */
	printf("titlebackground = \"%s\"\n", fname(state->titlebackground));
	printf("titlebackgroundtiled = %s\n", state->titlebackgroundtiled ?
								"Yes" : "No");

	/* Print the file names of the various splash graphics: */
	printf("titlesplash = \"%s\"\n", fname(state->titlesplash));
	printf("gamewonsplash = \"%s\"\n", fname(state->gamewonsplash));
	printf("gamelostsplash = \"%s\"\n", fname(state->gamelostsplash));

	/* Print the full list of sprites: */
	printf("sprites = \n");
	if(!state->spritelist) printf("\t<empty>\n");
	else for(s = state->spritelist; s; s = s->next)
		printf("\t\"%s\" (%d * %d) is in use %d time%s\n",
				fname(s->fname), s->w, s->h, s->usage,
				s->usage > 1 ? "s" : "");

	/* Print the name and dimensions of the current background image (or
	 * "none" if there isn't one, and whether or not is tiled: */
	printf("background = ");
	if(!state->background) printf("none\n");
	else printf("\"%s\" (%d * %d), %stiled\n",
		fname(state->background->fname), state->background->w,
		state->background->h, state->backgroundtiled ? "" : "not ");

	/* Print the points gained in various different circumstances, the
	 * balls given at the start of the game, and the balls given at the
	 * start of each new level: */
	printf("normalpoints = %d\nsmallbonuspoints = %d\nmediumbonuspoints = "
		"%d\nlargebonuspoints = %d\nhugebonuspoints = %d\n"
		"poweruppoints = %d\npowerdownpoints = %d\nstartballs = %d\n"
		"newlevelballs = %d\n", state->normalpoints,
		state->smallbonuspoints, state->mediumbonuspoints,
		state->largebonuspoints, state->hugebonuspoints,
		state->poweruppoints, state->powerdownpoints,
		state->startballs, state->newlevelballs);

	/* Print the dimensions of the brick area, the size of the bricks,
	 * the ID of the brick drawing alpha channel, and the global brick
	 * list: */
	printf("width = %d\nheight = %d\nbrickwidth = %d\nbrickheight = %d\n"
		"brickalpha = %d\n", state->width, state->height,
		state->brickwidth, state->brickheight, state->brickalpha);
	printf("bricks =\n");
	dump_bricks(state->bricks);

	/* Print the bat sizes, the names of all the various bat graphics, the
	 * current bat, and the current bat X position: */
	printf("batheight = %d\nsmall bat width = %d\nnormal bat width = %d\n"
		"large bat width = %d\n", state->batheight,
		state->batwidths[SMALLBAT], state->batwidths[NORMALBAT],
		state->batwidths[LARGEBAT]);
	printf("small bat = ");
	if(!state->bats[SMALLBAT]) printf("none\n");
	else printf("\"%s\"\n", fname(state->bats[SMALLBAT]->fname));
	printf("normal bat = ");
	if(!state->bats[NORMALBAT]) printf("none\n");
	else printf("\"%s\"\n", fname(state->bats[NORMALBAT]->fname));
	printf("large bat = ");
	if(!state->bats[SMALLBAT]) printf("none\n");
	else printf("\"%s\"\n", fname(state->bats[LARGEBAT]->fname));
	printf("current bat = ");
	switch(state->bat) {
		case NORMALBAT:
			printf("NORMALBAT\n");
			break;
		case SMALLBAT:
			printf("SMALLBAT\n");
			break;
		case LARGEBAT:
			printf("LARGEBAT\n");
			break;
		default:
			printf("unknown (%d)\n", state->bat);
	}
	printf("batx = %d\nbatv = %d\n", state->batx, state->batv);

	/* Print the names of each of the power sprites: */
	printf("Power sprites:\n");
	for(i = 0; i < NUMPOWERS; i++) {
		printf("\t%c ", powers[i]);
		if(state->powersprites[i])
			printf("\"%s\"\n",
				fname(state->powersprites[i]->fname));
		else printf("<empty>\n");
	}

	/* If there is a splash sprite, print its name and dimensions: */
	printf("splash = ");
	if(!state->splash) printf("none\n");
	else printf("\"%s\" (%d * %d)\n", fname(state->splash->fname),
					state->splash->w, state->splash->h);

	/* Print the times that power-ups and power-downs last for: */
	printf("poweruptime = %d\npowerdowntime = %d\n", state->poweruptime,
			state->powerdowntime);

	/* Print the cheat sequences, the cheat status, and the cheat flags: */
	printf("Cheat sequences:\n");
	for(i = 0; i < NUMCHEATS; i++) {
		printf("\t%s = ", cheats[i]);
		if(state->cheats[i]) printf("\"%s\"\n", state->cheats[i]);
		else printf("disabled\n");
	}
	printf("cheatstate = \"%s\"\n", state->cheatstate);
	printf("SolidFloor flag is %s\n", state->flags.sf ? "active" :
								"inactive");
	printf("NoBounce flag is %s\n", state->flags.nb ? "active" :
								"inactive");
	printf("NoPowerDown flag is %s\n", state->flags.npd ?
							"active" : "inactive");
	printf("NoPowerUpTimeout flag is %s\n", state->flags.nputo ?
							"active" : "inactive");

	/* Print the other various boolean flags: */
	printf("Paused flag is %s\n", state->flags.paused ?  "active" :
								"inactive");
	printf("Left flag is %s\n", state->flags.left ?  "active" :
								"inactive");
	printf("Right flag is %s\n", state->flags.right ?  "active" :
								"inactive");

	/* Print the current level and the total number of levels: */
	printf("level = %d\nnumlevels = %d\n", state->level, state->numlevels);

	/* Print the Nano-X IDs of the output window, the canvases, and the
	 * one second periodic timer, as well as the absolute X position of
	 * the output window and the dimensions of the canvas area: */
	printf("wid = %d\nwinx = %d\ncanvas = %d\noldcanvas = %d\n"
		"newcanvas = %d\ntid = %d\ncanvaswidth = %d\n"
		"canvasheight = %d\n", state->wid, state->winx, state->canvas,
		state->oldcanvas, state->newcanvas, state->tid,
		state->canvaswidth, state->canvasheight);

	/* The current brick area: */
	dump_grid(state, state->grid);

	/* The number of bricks remaining in the current level: */
	printf("numbricks = %d\n", state->numbricks);

	/* Print the ball related data: */
	printf("ball = ");
	if(!state->ball.s) printf("none\n");
	else printf("\"%s\" (%d * %d)\n", fname(state->ball.s->fname),
					state->ball.s->w, state->ball.s->h);
	printf("ball x = %f\nball y = %f\nball d = %f\nball v = %d\n"
			"lx = %d\nly = %d\nparked = %s\nslow ball velocity = "
			"%d\nnormal ball velocity = %d\nfast ball velocity = "
			"%d\n", state->ball.x, state->ball.y, state->ball.d,
			state->ball.v, state->ball.lx, state->ball.ly,
			state->ball.parked ?  "Yes" : "No", state->ball.sv,
			state->ball.nv, state->ball.fv);

	/* Print the current list of powers: */
	printf("current powers:\n");
	if(!state->powers) printf("\t<empty>\n");
	else for(p = state->powers; p; p = p->next)
		printf("\t%c at (%d, %d)\n", powers[p->type], p->x, p->y);

	/* Print the score related data: */
	printf("score = %d\nhiscore = %d\n file hiscore = %d\n"
			"score pixmap = %d\n", state->scores.s,
			state->scores.hi, state->scores.fhi, state->scores.p);

	/* Print out the power velocity and the animate period: */
	printf("powerv = %d\nanimateperiod = %d\n", state->powerv,
						state->animateperiod);

	/* Print the time of the last animate() call: */
	printf("lastanim = %ldS %lduS\n", state->lastanim.tv_sec,
					state->lastanim.tv_usec);

	/* Print the current power-up and power-down timer values: */
	printf("powertimes:\n"
		"\twidebat = %d\n"
		"\tslowmotion = %d\n"
		"\tstickybat = %d\n"
		"\tpowerball = %d\n"
		"\tnarrowbat = %d\n"
		"\tfastmotion = %d\n",
		state->powertimes.widebat,
		state->powertimes.slowmotion,
		state->powertimes.stickybat,
		state->powertimes.powerball,
		state->powertimes.narrowbat,
		state->powertimes.fastmotion);

	/* Print the fade rate and current fade level: */
	printf("faderate = %d\nfadelevel = %d\n", state->faderate,
						state->fadelevel);

	/* Print the next game state: */
	printf("nextstate = ");
	printstate(state->nextstate);

	/* Print all of the level data: */
	dump_levels(state);
}
#endif
