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

/* loader.c mainly consists of routines for loading the game file. It also
 * contains the high score file loader and saver. */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nano-X.h>
#include <nxcolors.h>

#include "nbreaker.h"

/* Print a warning indicating that a particular parameter has been defined
 * more than once, with the line number of the second definition. */
static void redefinewarning(char *name, int line, char *gamefile)
{
	fprintf(stderr, "Warning: redefining %s on line %d of game file "
					"\"%s\"\n", name, line, gamefile);
}

/* Print a warning indicating that some parameter that is only allowed inside
 * a level block was encountered outside a level block. */
static void notinlevblockerr(char *name, int line, char *gamefile)
{
	fprintf(stderr, "Error: %s while not in a level block on "
			"line %d of game file \"%s\"\n", name, line, gamefile);
}

/* Parse a Rows block by reading each line until it reaches a line containing
 * just EndRows, and convert each row into an appropriate line in the grid of
 * the specified level (and perform quite a bit of error checking along the
 * way). Returns 0 on success or 1 on encountering an error. */
static int parse_rows(nbstate *state, level *lev, FILE *fp, int *line)
{
	brick *b;
	int x, y;
	char buf[256], *p;
	grid *g = lev->grid;
	int startline = *line;

	y = 0; /* Start on the first row. */

	/* Read lines from the game file in a loop: */
	while(fgets(buf, 256, fp)) {

		x = 0; /* start the row in the first column. */
		*line = *line + 1; /* increment the line number. */

		/* Look for the newline at the end of the line. */
		if(!(p = strchr(buf, '\n'))) {
			/* There wasn't one, which probably means that the
			 * line was longer than 255 characters. */
			fprintf(stderr, "Too long line on line %d of game file "
					"\"%s\"\n", *line, state->gamefile);
			return 1;
		}
		*p = 0; /* Get rid of the newline. */

		/* If this is the end of the rows block, return "success": */
		if(!memcmp(buf, "EndRows", 7)) return 0;
	
		/* If we've gone past the number of rows in the grid: */
		if(y == state->height) {
			/* Print an error and return "failure": */
			fprintf(stderr, "Too many rows in level definition on"
					"line %d of game file \"%s\"\n",
					*line, state->gamefile);
			return 1;
		}

		/* For each letter in the row: */
		for(p = buf; *p; p++) {

			/* Increment the column and if we go past the width of
			 * the grid, print an error and return "failure": */
			if(++x > state->width) {
				fprintf(stderr, "Error: row too long on line "
					"%d of game file \"%s\" (Width = %d)\n",
					*line, state->gamefile,
					state->width);
				return 1;
			}

			if(*p == ' ') b = NULL; /* A space means "no brick". */
			else {
				/* Search through the global brick list for
				 * one with a matching identifier: */
				for(b = state->bricks; b && b->identifier != *p;
								b = b->next);
				/* If one wasn't found: */
				if(!b) {
					/* Search through the level specific
					 * brick list for one with a matching
					 * identifier: */
					for(b = lev->bricks; b &&
						b->identifier != *p;
						b = b->next);
				}
				/* If one wasn't found: */
				if(!b) {
					/* Print an error message and return
					 * "failure": */
					fprintf(stderr, "Error: undefined brick"
						" \"%c\" on line %d of game "
						"file \"%s\"\n", *p, *line,
						state->gamefile);
					return 1;
				}
			}
			
			/* A matching brick was found if we get to here.
			 * Unless the brick is immutable (which don't count
			 * because they're not normally destroyable), increment
			 * the count of bricks in this level: */
			if(b && !(b->flags & BRICK_FLAG_IMMUTABLE))
				lev->numbricks++;
			
			/* Set the current grid location brick pointer: */
			g->b = b;
			g++; /* Increment to the next grid location. */
		}
		g += state->width - x; /* skip past the rest of the row. */
		y++; /* Increment to the next row. */
	}

	/* We shouldn't reach here unless the fgets() fails to get a new line
	 * either because we reach the end of the file or because an I/O error
	 * of some sort occurred. Print an appropriate error message and return
	 * "failure" to the caller: */
	if(feof(fp)) {
		fprintf(stderr, "Error: premature end of file inside rows "
				"block that started on line %d of game file "
				"\"%s\"\n", startline, state->gamefile);
	} else fprintf(stderr, "Error reading from game file \"%s\": %s\n",
			state->gamefile, strerror(errno));
	return 1;
}

static int parse_powers(nbstate *state, level *lev, FILE *fp, int *line)
{
	int x, y, i;
	char buf[256], *p;
	grid *g = lev->grid;
	int startline = *line;
	char powers[] = " WSTPNF";

	y = 0; /* Start on the first row. */

	/* Read lines from the game file in a loop: */
	while(fgets(buf, 256, fp)) {

		x = 0; /* start the row in the first column. */
		*line = *line + 1; /* increment the line number. */

		/* Look for the newline at the end of the line. */
		if(!(p = strchr(buf, '\n'))) {
			/* There wasn't one, which probably means that the
			 * line was longer than 255 characters. */
			fprintf(stderr, "Too long line on line %d of game file "
					"\"%s\"\n", *line, state->gamefile);
			return 1;
		}
		*p = 0; /* Get rid of the newline. */

		/* If this is the end of the powers block, return "success": */
		if(!memcmp(buf, "EndPowers", 9))
			return 0;

		/* If we've gone past the number of rows in the grid: */
		if(y == state->height) {
			/* Print an error and return "failure": */
			fprintf(stderr, "Too many rows in level definition on"
					"line %d of game file \"%s\"\n",
					*line, state->gamefile);
			return 1;
		}

		/* For each letter in the row: */
		for(p = buf; *p; p++) {

			/* Increment the column and if we go past the width of
			 * the grid, print an error and return "failure": */
			if(++x > state->width) {
				fprintf(stderr, "Error: row too long on line "
					"%d of game file \"%s\" (Width = %d)\n",
					*line, state->gamefile,
					state->width);
				return 1;
			}

			/* Search for a power with a matching identifier: */
			for(i = 0; powers[i] && powers[i] != *p; i++);

			/* If we didn't find one print an error message and
			 * return "failure": */
			if(!powers[i]) {
				fprintf(stderr, "Error: invalid power \"%c\" "
					"on line %d of game file \"%s\"\n",
					*p, *line, state->gamefile);
				return 1;
			}

			/* Set the power of the current grid location: */
			g->power = i - 1;
			g++; /* Increment to the next grid location. */
		}
		g += state->width - x; /* skip past the rest of the row. */
		y++; /* Increment to the next row. */
	}

	/* We shouldn't reach here unless the fgets() fails to get a new line
	 * either because we reach the end of the file or because an I/O error
	 * of some sort occurred. Print an appropriate error message and return
	 * "failure" to the caller: */
	if(feof(fp)) {
		fprintf(stderr, "Error: premature end of file inside powers "
				"block that started on line %d of game file "
				"\"%s\"\n", startline, state->gamefile);
	} else fprintf(stderr, "Error reading from game file \"%s\": %s\n",
			state->gamefile, strerror(errno));
	return 1;
}

/* Parse a brick definition line. */
static int parse_brick(nbstate *state, int inlevel, level *lev, int line,
		char *buf)
{
	int i;
	char *flags, f;
	brick *b = NULL;

	/* Search for an existing brick with the same identifier in the global
	 * brick list: */
	for(b = state->bricks; b && b->identifier != *buf; b = b->next);
	
	/* If we didn't find one and we're in a level definition, search for
	 * a matching brick in the level specific brick list: */
	if(!b && inlevel)
		for(b = lev->bricks; b && b->identifier != *buf; b = b->next);
	
	/* If we have found an existing brick with the same identifier, print
	 * a warning and return: */
	if(b) {
		fprintf(stderr, "Warning: ignoring duplicate brick definition "
				"on line %d of game file \"%s\"\n", line,
				state->gamefile);
		return 0;
	}

	/* Allocate the new brick structure: */
	if(!(b = malloc(sizeof(brick)))) {
		oom();
		return 1;
	}

	b->identifier = *buf; /* Set the brick identifier. */

	buf += 2; /* Skip past the identifier and the space after it. */

	/* Look for a space after the filename, and if one is found set it to
	 * 0 and set flags to the character after the space. After this, buf
	 * points to a null terminated string which is the image filename, and
	 * flags is either NULL if there was no space (and no flags) after the
	 * filename, or a pointer to a null terminated list of flags. */
	if((flags = strchr(buf, ' '))) *flags++ = 0;

	b->flags = 0; /* Initialise the flags with all of them turned off. */

	/* If there is a flags string: */
	if(flags) {
		/* For each flag in the string: */
		while((f = *flags++)) {
			/* Set the flag which matches this letter: */
			if(f == 'I') b->flags |= BRICK_FLAG_IMMUTABLE;
			else if(f == '2') b->flags |= BRICK_FLAG_2_HITS;
			else if(f == '3') b->flags |= BRICK_FLAG_3_HITS;
			else if(f == 'S') b->flags |= BRICK_FLAG_SMALL_BONUS;
			else if(f == 'M') b->flags |= BRICK_FLAG_MEDIUM_BONUS;
			else if(f == 'L') b->flags |= BRICK_FLAG_LARGE_BONUS;
			else if(f == 'H') b->flags |= BRICK_FLAG_HUGE_BONUS;
			else {
				/* The letter wasn't one of the above, so print
				 * a warning message and ignore the flag. */
				fprintf(stderr, "Warning: ignoring invalid "
						"flag \"%c\" specified for "
						"brick \"%c\" on line %d "
						"of game file \"%s\"\n", f,
						b->identifier, line,
						state->gamefile);
			}
		}
	}

	/* Count the number of flags which affect the number of times you have
	 * to hit the brick to destroy it: */
	i = 0;
	if(b->flags & BRICK_FLAG_IMMUTABLE) i++;
	if(b->flags & BRICK_FLAG_2_HITS) i++;
	if(b->flags & BRICK_FLAG_3_HITS) i++;
	/* If there was more than one of the above flags set, print a warning
	 * and cancel all of them: */
	if(i > 1) {
		fprintf(stderr, "Warning: brick flags I, 2, and 3 are mutually "
				"exclusive (see brick \"%c\" on line %d of "
				"game file \"%s\")\n", b->identifier, line,
				state->gamefile);
		b->flags &= ~(BRICK_FLAG_IMMUTABLE | BRICK_FLAG_2_HITS |
				BRICK_FLAG_3_HITS);
	}

	/* Count the number of flags which tell what bonus is associated with
	 * this brick: */
	i = 0;
	if(b->flags & BRICK_FLAG_SMALL_BONUS) i++;
	if(b->flags & BRICK_FLAG_MEDIUM_BONUS) i++;
	if(b->flags & BRICK_FLAG_LARGE_BONUS) i++;
	if(b->flags & BRICK_FLAG_HUGE_BONUS) i++;
	/* If there was more than one of the above flags set, print a warning
	 * and cancel all of them: */
	if(i > 1) {
		fprintf(stderr, "Warning: brick flags S, M, L, and H are "
				"mutually exclusive (see brick \"%c\" on line "
				"%d of game file \"%s\")\n", b->identifier,
				line, state->gamefile);
		b->flags &= ~(BRICK_FLAG_SMALL_BONUS | BRICK_FLAG_LARGE_BONUS |
			BRICK_FLAG_MEDIUM_BONUS | BRICK_FLAG_HUGE_BONUS);
	}

	/* Try to load the sprite for this brick: */
	if(!(b->s = load_sprite(state, buf, state->brickwidth,
					state->brickheight))) {
		/* It failed, so try to make an empty sprite: */
		if(!(b->s = make_empty_sprite(state, buf, state->brickwidth,
						state->brickheight))) {
			/* That failed too, so print an error message and
			 * return "failure": */
			fprintf(stderr, "Error: failed to create brick "
				"sprite on line %d of game file \"%s\"\n",
						line, state->gamefile);
			free(b);
			return 1;
		}
		/* Fill in the dummy sprite with a solid colour: */
		GrSetGCForeground(state->gc, GR_COLOR_BLUE);
		GrFillRect(b->s->p, state->gc, 0, 0, state->brickwidth,
				state->brickheight);
		GrSetGCForeground(state->gc, GR_COLOR_WHITE);
		GrFillRect(b->s->a, state->gc, 0, 0, state->brickwidth,
				state->brickheight);
	}

	/* If we're in a level definition, link the new brick into the level
	 * specific brick list: */
	if(inlevel) {
		b->next = lev->bricks;
		lev->bricks = b;
	} else { /* Otherwise link it into the global brick list: */
		b->next = state->bricks;
		state->bricks = b;
	}

	return 0; /* Success. */
}

/* Create and initialise a new level grid with default parameters. */
static grid *newgrid(nbstate *state)
{
	int x, xx, y, yy;
	grid *g, *gg;

	/* Allocate the new grid: */
	if(!(gg = g = malloc(state->width * state->height * sizeof(grid))))
		return NULL;

	/* Initialise the Y coordinate to the start of the brick area within the
	 * canvas area: */
	yy = state->scores.h + state->ball.s->h + (BALLS_BORDER * 2);

	/* For each row of bricks: */
	for(y = 0; y < state->height; y++, yy += state->brickheight) {
		/* For each brick in the row: */
		for(x = 0, xx = 0; x < state->width; x++,
						xx += state->brickwidth) {
			/* Initialise the parameters for this grid location: */
			g->b = NULL;
			g->x = xx;
			g->y = yy;
			g->hits = 0;
			g->power = -1;
			g++; /* Increment to the next grid location. */
		}
	}

	return gg; /* Return the newly allocated grid. */
}

/* Parse a PowerSprite line. */
static int parse_powersprite(nbstate *state, char *buf, int line)
{
	sprite *s;
	int power;

	/* Determine which power is being referred to: */
	switch(*buf) {
		case 'W':
			power = POWERW;
			break;
		case 'S':
			power = POWERS;
			break;
		case 'T':
			power = POWERT;
			break;
		case 'P':
			power = POWERP;
			break;
		case 'N':
			power = POWERN;
			break;
		case 'F':
			power = POWERF;
			break;
		default:
			power = NOPOWER;
	}

	/* If it wasn't a valid power identifier, print an error message and
	 * return "failure": */
	if(power == NOPOWER) {
		fprintf(stderr, "Invalid power \"%c\" on line %d of game file "
				"\"%s\"\n", *buf, line, state->gamefile);
		return 1;
	}

	/* If this power sprite has already been defined, print a warning and
	 * destroy the old sprite: */
	if(state->powersprites[power]) {
		redefinewarning("PowerSprite", line, state->gamefile);
		destroy_sprite(state, state->powersprites[power]);
	}

	/* Try to load the image for this power sprite: */
	if(!(s = load_sprite(state, buf + 2, -1, -1))) {
		/* If that failed, try making an empty sprite: */
		if(!(s = make_empty_sprite(state, buf + 2, DEFAULT_POWER_WIDTH,
						DEFAULT_POWER_HEIGHT))) {
			/* That failed too, so print an error message and
			 * return "failure": */
			fprintf(stderr, "Error: failed to create power sprite "
					"on line %d of game file \"%s\"\n",
					line, state->gamefile);
			return 1;
		}
		/* If it's a power-up, fill the power rectangle in green;
		 * if it's a power-down, fill it in red: */
		if(power == POWERW || power == POWERS || power == POWERT ||
				power == POWERP)
			GrSetGCForeground(state->gc, GR_COLOR_GREEN);
		else GrSetGCForeground(state->gc, GR_COLOR_RED);
		GrFillRect(s->p, state->gc, 0, 0, DEFAULT_POWER_WIDTH,
						DEFAULT_POWER_HEIGHT);
		GrSetGCForeground(state->gc, GR_COLOR_WHITE);
		GrFillRect(s->a, state->gc, 0, 0, DEFAULT_POWER_WIDTH,
						DEFAULT_POWER_HEIGHT);
	}

	/* Set this power sprite to the newly created sprite. */
	state->powersprites[power] = s;

	return 0; /* Success. */
}

/* Load the game file. */
int load_game_file(nbstate *state)
{
	FILE *fp;
	int line = 1;
	level *l, *lev = NULL;
	char buf[256], *p;
	int inlevelblock = 0;

	/* Generate the full game file name including the directory: */
	snprintf(buf, 256, "%s/%s", state->gamedir, state->gamefile);

	/* Try to open the game file: */
	if(!(fp = fopen(buf, "r"))) {
		/* It failed, so print an error message and return "failure": */
		fprintf(stderr, "Failed to open game file \"%s\": %s\n",
						buf, strerror(errno));
		return 1;
	}

	/* Read the file one line at a time in a loop: */
	while(fgets(buf, 256, fp)) {

		/* Look for the newline at the end of the line. */
		if(!(p = strchr(buf, '\n'))) {
			/* There wasn't one, which probably means that the
			 * line was longer than 255 characters. */
			fprintf(stderr, "Too long line on line %d of game file "
					"\"%s\"\n", line, state->gamefile);
			return 1;
		}
		*p = 0; /* Get rid of the newline. */

		/* Ignore comments and blank lines: */
		if(*buf == '#' || *buf == 0) {
		/* Compare the line against each of the different keywords: */
		} else if(!memcmp(buf, "TitleBackground ", 16)) {
			if(state->titlebackground) {
				redefinewarning("TitleBackground", line,
						state->gamefile);
				free(state->titlebackground);
			}
			if(!(state->titlebackground = strdup(buf + 16))) {
				oom();
				goto err;
			}
		} else if(!memcmp(buf, "TitleBackgroundTiled ", 21)) {
			/* Check whether the parameter to TitleBackgroundTiled
			 * is "Yes" (1), "No" (0), or something else (parse
			 * error): */
			if(!strcmp(buf + 21, "Yes"))
				state->backgroundtiled = 1;
			else if(!strcmp(buf + 21, "No"))
				state->backgroundtiled = 0;
			else goto parseerr;
		} else if(!memcmp(buf, "TitleSplash ", 12)) {
			if(state->titlesplash) {
				redefinewarning("TitleSplash", line,
						state->gamefile);
				free(state->titlesplash);
			}
			if(!(state->titlesplash = strdup(buf + 12))) {
				oom();
				goto err;
			}
		} else if(!memcmp(buf, "GameWonSplash ", 14)) {
			if(state->gamewonsplash) {
				redefinewarning("GameWonSplash", line,
						state->gamefile);
				free(state->gamewonsplash);
			}
			if(!(state->gamewonsplash = strdup(buf + 14))) {
				oom();
				goto err;
			}
		} else if(!memcmp(buf, "GameLostSplash ", 15)) {
			if(state->gamelostsplash) {
				redefinewarning("GameLostSplash", line,
						state->gamefile);
				free(state->gamelostsplash);
			}
			if(!(state->gamelostsplash = strdup(buf + 15))) {
				oom();
				goto err;
			}
		} else if(!memcmp(buf, "NormalPoints ", 13)) {
			/* Convert the parameter to NormalPoints into a
			 * number: */
			state->normalpoints = strtol(buf + 13, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "SmallBonusPoints ", 17)) {
			state->smallbonuspoints = strtol(buf + 17, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "MediumBonusPoints ", 18)) {
			state->mediumbonuspoints = strtol(buf + 18, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "LargeBonusPoints ", 17)) {
			state->largebonuspoints = strtol(buf + 17, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "HugeBonusPoints ", 16)) {
			state->hugebonuspoints = strtol(buf + 16, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "PowerUpPoints ", 14)) {
			state->poweruppoints = strtol(buf + 14, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "PowerDownPoints ", 16)) {
			state->powerdownpoints = strtol(buf + 16, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "StartBalls ", 11)) {
			state->startballs = strtol(buf + 11, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "NewLevelBalls ", 14)) {
			state->newlevelballs = strtol(buf + 14, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "BallImage ", 10)) {
			/* If the ball sprite has already been defined, print
			 * a warning and destroy the old sprite: */
			if(state->ball.s) {
				redefinewarning("BallImage", line,
						state->gamefile);
				destroy_sprite(state, state->ball.s);
			}
			/* Try to load the ball sprite: */
			if(!(state->ball.s = load_sprite(state, buf + 10,
								-1, -1))) {
				/* That failed so try to make an empty one: */
				if(!(state->ball.s = make_empty_sprite(state,
							buf + 10,
							DEFAULT_BALL_SIZE,
							DEFAULT_BALL_SIZE))) {
					/* That failed too so print an error
					 * message and give up: */
					fprintf(stderr, "Couldn't create "
							"ball sprite on line "
							"%d of game file "
							"\"%s\"\n", line,
							state->gamefile);
					goto err;
				}
				/* Fill in the dummy sprite with a white
				 * circle: */
				GrSetGCForeground(state->gc, GR_COLOR_WHITE);
				GrFillEllipse(state->ball.s->p, state->gc,
							DEFAULT_BALL_SIZE / 2,
							DEFAULT_BALL_SIZE / 2,
						(DEFAULT_BALL_SIZE / 2) - 1,
						(DEFAULT_BALL_SIZE / 2) - 1);
				GrFillEllipse(state->ball.s->a, state->gc,
							DEFAULT_BALL_SIZE / 2,
							DEFAULT_BALL_SIZE / 2,
						(DEFAULT_BALL_SIZE / 2) - 1,
						(DEFAULT_BALL_SIZE / 2) - 1);
			}
		} else if(!memcmp(buf, "SlowBallVelocity ", 17)) {
			state->ball.sv = strtol(buf + 17, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "NormalBallVelocity ", 19)) {
			state->ball.nv = strtol(buf + 19, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "FastBallVelocity ", 17)) {
			state->ball.fv = strtol(buf + 17, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "BatVelocity ", 12)) {
			state->batv = strtol(buf + 12, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "PowerVelocity ", 14)) {
			state->powerv = strtol(buf + 14, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "AnimatePeriod ", 14)) {
			state->animateperiod = strtol(buf + 14, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "BrickWidth ", 11)) {
			state->brickwidth = strtol(buf + 11, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "BrickHeight ", 12)) {
			state->brickheight = strtol(buf + 12, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "Brick ", 6)) {
			/* Parse the brick line: */
			if(parse_brick(state, inlevelblock, lev, line, buf + 6))
				goto err;
		} else if(!memcmp(buf, "Width ", 6)) {
			if(lev) {
				fprintf(stderr, "Error: Width must be set "
					"before the first level is defined "
					"(see line %d of game file \"%s\")\n",
					line, state->gamefile);
				goto err;
			}
			state->width = strtol(buf + 6, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "Height ", 7)) {
			if(lev) {
				fprintf(stderr, "Error: Height must be set "
					"before the first level is defined "
					"(see line %d of game file \"%s\")\n",
					line, state->gamefile);
				goto err;
			}
			state->height = strtol(buf + 7, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "BatHeight ", 10)) {
			state->batheight = strtol(buf + 10, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "NormalBatWidth ", 15)) {
			state->batwidths[NORMALBAT] = strtol(buf + 15, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "SmallBatWidth ", 14)) {
			state->batwidths[SMALLBAT] = strtol(buf + 14, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "LargeBatWidth ", 14)) {
			state->batwidths[LARGEBAT] = strtol(buf + 14, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "NormalBat ", 10)) {
			if(state->bats[NORMALBAT]) {
				redefinewarning("NormalBat", line,
						state->gamefile);
				destroy_sprite(state, state->bats[NORMALBAT]);
			}
			if(!(state->bats[NORMALBAT] = load_sprite(state,
					buf + 10, state->batwidths[NORMALBAT],
					state->batheight))) {
				if(!(state->bats[NORMALBAT] =
					make_empty_sprite(state, buf + 10,
						state->batwidths[NORMALBAT],
						state->batheight))) {
					fprintf(stderr, "Couldn't create "
							"normal bat sprite on "
							"line %d of game file "
							"\"%s\"\n", line,
							state->gamefile);
					goto err;
				}
				GrSetGCForeground(state->gc, GR_COLOR_RED);
				GrFillRect(state->bats[NORMALBAT]->p,
						state->gc, 0, 0,
						state->batwidths[NORMALBAT],
						state->batheight);
				GrSetGCForeground(state->gc, GR_COLOR_WHITE);
				GrFillRect(state->bats[NORMALBAT]->a,
						state->gc, 0, 0,
						state->batwidths[NORMALBAT],
						state->batheight);
			}
		} else if(!memcmp(buf, "SmallBat ", 9)) {
			if(state->bats[SMALLBAT]) {
				redefinewarning("SmallBat", line,
						state->gamefile);
				destroy_sprite(state, state->bats[SMALLBAT]);
			}
			if(!(state->bats[SMALLBAT] = load_sprite(state,
					buf + 9, state->batwidths[SMALLBAT],
					state->batheight))) {
				if(!(state->bats[SMALLBAT] =
					make_empty_sprite(state, buf + 9,
						state->batwidths[SMALLBAT],
						state->batheight))) {
					fprintf(stderr, "Couldn't create "
							"small bat sprite on "
							"line %d of game file "
							"\"%s\"\n", line,
							state->gamefile);
					goto err;
				}
				GrSetGCForeground(state->gc, GR_COLOR_RED);
				GrFillRect(state->bats[SMALLBAT]->p,
						state->gc, 0, 0,
						state->batwidths[SMALLBAT],
						state->batheight);
				GrSetGCForeground(state->gc, GR_COLOR_WHITE);
				GrFillRect(state->bats[SMALLBAT]->a,
						state->gc, 0, 0,
						state->batwidths[SMALLBAT],
						state->batheight);
			}
		} else if(!memcmp(buf, "LargeBat ", 9)) {
			if(state->bats[LARGEBAT]) {
				redefinewarning("LargeBat", line,
						state->gamefile);
				destroy_sprite(state, state->bats[LARGEBAT]);
			}
			if(!(state->bats[LARGEBAT] = load_sprite(state,
					buf + 9, state->batwidths[LARGEBAT],
					state->batheight))) {
				if(!(state->bats[LARGEBAT] =
					make_empty_sprite(state, buf + 9,
						state->batwidths[LARGEBAT],
						state->batheight))) {
					fprintf(stderr, "Couldn't create "
							"large bat sprite on "
							"line %d of game file "
							"\"%s\"\n", line,
							state->gamefile);
					goto err;
				}
				GrSetGCForeground(state->gc, GR_COLOR_RED);
				GrFillRect(state->bats[LARGEBAT]->p,
						state->gc, 0, 0,
						state->batwidths[LARGEBAT],
						state->batheight);
				GrSetGCForeground(state->gc, GR_COLOR_WHITE);
				GrFillRect(state->bats[LARGEBAT]->a,
						state->gc, 0, 0,
						state->batwidths[LARGEBAT],
						state->batheight);
			}
		} else if(!memcmp(buf, "PowerSprite ", 12)) {
			/* Parse the PowerSprite line: */
			if(parse_powersprite(state, buf + 12, line)) goto err;
		} else if(!memcmp(buf, "PowerUpTimeout ", 15)) {
			state->poweruptime = strtol(buf + 15, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "PowerDownTimeout ", 17)) {
			state->powerdowntime = strtol(buf + 17, &p, 10);
			if(*p) goto parseerr;
		} else if(!memcmp(buf, "FadeRate ", 9)) {
			state->faderate = strtol(buf + 9, &p, 10);
			if(*p) goto parseerr;
			if(state->faderate > 255 || state->faderate < 0) {
				fprintf(stderr, "Invalid fade rate on line "
					"%d of game file \"%s\"\n", line,
					state->gamefile);
				goto err;
			}
		} else if(!memcmp(buf, "SolidFloorCheat ", 16)) {
			if(state->cheats[SFCHEAT]) {
				redefinewarning("SolidFloorCheat", line,
						state->gamefile);
				free(state->cheats[SFCHEAT]);
			}
			if(!(state->cheats[SFCHEAT] = strdup(buf + 16))) {
				oom();
				goto err;
			}
			if(strlen(state->cheats[SFCHEAT]) > MAXCHEATLEN) {
				fprintf(stderr, "Cheat sequence too long on "
						"line %d of game file \"%s\"\n",
						line, state->gamefile);
				goto err;
			}
		} else if(!memcmp(buf, "TeleportCheat ", 14)) {
			if(state->cheats[TPCHEAT]) {
				redefinewarning("TeleportCheat", line,
						state->gamefile);
				free(state->cheats[TPCHEAT]);
			}
			if(!(state->cheats[TPCHEAT] = strdup(buf + 14))) {
				oom();
				goto err;
			}
			if(strlen(state->cheats[TPCHEAT]) > MAXCHEATLEN) {
				fprintf(stderr, "Cheat sequence too long on "
						"line %d of game file \"%s\"\n",
						line, state->gamefile);
				goto err;
			}
		} else if(!memcmp(buf, "NoBounceCheat ", 14)) {
			if(state->cheats[NBCHEAT]) {
				redefinewarning("NoBounceCheat", line,
						state->gamefile);
				free(state->cheats[NBCHEAT]);
			}
			if(!(state->cheats[NBCHEAT] = strdup(buf + 14))) {
				oom();
				goto err;
			}
			if(strlen(state->cheats[NBCHEAT]) > MAXCHEATLEN) {
				fprintf(stderr, "Cheat sequence too long on "
						"line %d of game file \"%s\"\n",
						line, state->gamefile);
				goto err;
			}

		} else if(!memcmp(buf, "NoPowerDownCheat ", 17)) {
			if(state->cheats[NPDCHEAT]) {
				redefinewarning("NoPowerDownCheat", line,
						state->gamefile);
				free(state->cheats[NPDCHEAT]);
			}
			if(!(state->cheats[NPDCHEAT] = strdup(buf + 17))) {
				oom();
				goto err;
			}
			if(strlen(state->cheats[NPDCHEAT]) > MAXCHEATLEN) {
				fprintf(stderr, "Cheat sequence too long on "
						"line %d of game file \"%s\"\n",
						line, state->gamefile);
				goto err;
			}

		} else if(!memcmp(buf, "NoPowerUpTimeoutCheat ", 22)) {
			if(state->cheats[NPUTOCHEAT]) {
				redefinewarning("NoPowerUpTimeoutCheat", line,
						state->gamefile);
				free(state->cheats[NPUTOCHEAT]);
			}
			if(!(state->cheats[NPUTOCHEAT] = strdup(buf + 22))) {
				oom();
				goto err;
			}
			if(strlen(state->cheats[NPUTOCHEAT]) > MAXCHEATLEN) {
				fprintf(stderr, "Cheat sequence too long on "
						"line %d of game file \"%s\"\n",
						line, state->gamefile);
				goto err;
			}
		} else if(!memcmp(buf, "BeginLevel", 10)) {

			/* Check to make sure we haven't got another BeginLevel
			 * line while already in a level definition block: */
			if(inlevelblock) {
				fprintf(stderr, "Error: BeginLevel while "
					"already in a level block on line %d "
					"of game file \"%s\"\n", line,
					state->gamefile);
				goto err;
			}
			inlevelblock = 1;

			/* Allocate a new level structure: */
			if(!(lev = malloc(sizeof(level)))) {
				oom();
				goto err;
			}
			/* Fill in the structure members with some defaults: */
			lev->bricks = NULL;
			lev->backgroundname = NULL;
			lev->backgroundtiled = DEFAULT_BACKGROUND_TILED;
			lev->numbricks = 0;
			lev->next = NULL;

			/* Allocate and initialise the level grid: */
			if(!(lev->grid = newgrid(state))) {
				oom();
				goto err;
			}

			/* Link the new level structure onto the end of the
			 * levels list: */
			if(!state->levels) state->levels = lev;
			else {
				for(l = state->levels; l->next; l = l->next);
				l->next = lev;
			}
		} else if(!memcmp(buf, "LevelBackground ", 16)) {
			if(!inlevelblock) {
				notinlevblockerr("LevelBackground", line,
						state->gamefile);
				goto err;
			}
			if(lev->backgroundname) {
				redefinewarning("LevelBackground", line,
						state->gamefile);
				free(lev->backgroundname);
			}
			if(!(lev->backgroundname = strdup(buf + 16))) {
				oom();
				goto err;
			}
		} else if(!memcmp(buf, "LevelBackgroundTiled ", 21)) {
			if(!inlevelblock) {
				notinlevblockerr("LevelBackgroundTiled", line,
						state->gamefile);
				goto err;
			}
			if(!strcmp(buf + 21, "Yes")) lev->backgroundtiled = 1;
			else if(!strcmp(buf + 21, "No"))
				lev->backgroundtiled = 0;
			else goto parseerr;
		} else if(!memcmp(buf, "BeginRows", 9)) {
			if(!inlevelblock) {
				notinlevblockerr("BeginRows", line,
						state->gamefile);
				goto err;
			}
			/* Parse the rows block: */
			if(parse_rows(state, lev, fp, &line)) goto err;
		} else if(!memcmp(buf, "EndRows", 7)) {
			/* We should never see an EndRows here in a valid
			 * level file because parse_rows() consumes it. */
			fprintf(stderr, "Error: EndRows without corresponding "
					"BeginRows on line %d of game file "
					"\"%s\"\n", line, state->gamefile);
			goto err;
		} else if(!memcmp(buf, "BeginPowers", 11)) {
			if(!inlevelblock) {
				notinlevblockerr("BeginPowers", line,
						state->gamefile);
				goto err;
			}
			if(parse_powers(state, lev, fp, &line)) goto err;
		} else if(!memcmp(buf, "EndPowers", 9)) {
			fprintf(stderr, "Error: EndPowers without "
					"corresponding BeginPowers on line %d "
					"of game file \"%s\"\n", line,
					state->gamefile);
			goto err;
		} else if(!memcmp(buf, "EndLevel", 8)) {
			if(!inlevelblock) {
				fprintf(stderr, "Error: EndLevel while not in "
					"in a level block on line %d of game "
					"file \"%s\"\n", line, state->gamefile);
				goto err;
			}
			inlevelblock = 0;
			state->numlevels++;
		} else {
			fprintf(stderr, "Unknown command \"%s\" on line %d "
					"of game file \"%s\"\n", buf,
					line, state->gamefile);
		}
		/* We keep a count of the line we're on so that errors and
		 * warnings can print out the number of the bad line: */
		line++;
	}

	/* Check if the reason fgets() failed was because of an I/O error
	 * instead of simply reaching the end of the file: */
	if(ferror(fp)) {
		fprintf(stderr, "Error reading from game file \"%s\" on line "
				"%d: %s\n", state->gamefile, line,
				strerror(errno));
		goto err;
	}

	/* Allocate and initialise the current game grid: */
	if(!(state->grid = newgrid(state))) {
		oom();
		goto err;
	}

	fclose(fp); /* Close the game file. */

	return 0; /* Success. */

parseerr: /* A parse error occured so print an error message: */
	fprintf(stderr, "Parse error on line %d of game file \"%s\"\n", line,
							state->gamefile);
err: /* Some other error occured and we've already printed the error message. */
	fclose(fp); /* Close the game file (may fail but we don't care). */
	return 1; /* Failure. */
}
