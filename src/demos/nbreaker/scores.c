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

/* scores.c contains functions related to scores. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Increment the score by the specifed amount and redraw the score bar: */
void increment_score(nbstate *state, int points)
{
	state->scores.s += points; /* Increment the current score. */
	draw_scores(state); /* Redraw the score bar onto the canvas. */
	/* Copy the bar onto the output window. */
	draw_canvas(state, 0, 0, state->canvaswidth, state->scores.h);
}

/* Load the high score from the high score file. */
void load_hiscore(nbstate *state)
{
	FILE *fp;
	int i, n;
	char buf[256];

	/* Generate the full path to the high score file: */
	snprintf(buf, 256, "%s/"HISCORE_FILE, state->gamedir);

	/* Try to open the file: */
	if(!(fp = fopen(buf, "r"))) {
		/* It failed, but we don't care about "file doesn't exist"
		 * errors because that may well just mean that this is the
		 * first time the game has been run. If it was any other error,
		 * Print a warning. */
		if(errno != ENOENT)
			fprintf(stderr, "Couldn't open high score file \"%s\""
				"for reading: %s\n", buf, strerror(errno));
		/* Set the high score to 0: */
		state->scores.hi = state->scores.fhi = 0;
		return;
	}

	i = fscanf(fp, "%d", &n); /* Try to read the score from the file. */

	fclose(fp); /* Close the high score file. */

	/* If we failed to read a valid number from the file, print a warning
	 * and set the high score to 0: */
	if(i != 1) {
		fprintf(stderr, "Couldn't read high score file\n");
		n = 0;
	}

	/* Set both the high score and the file high score to the read value.
	 * The file high score is used by save_hiscore() (see below): */
	state->scores.hi = state->scores.fhi = n;
}

/* Save the high score if it has increased. */
void save_hiscore(nbstate *state)
{
	FILE *fp;
	char buf[256];

	/* Make sure the current score isn't higher than the high score: */
	if(state->scores.s > state->scores.hi)
		state->scores.hi = state->scores.s;

	/* Don't bother writing the high score out if it isn't any higher than
	 * the stored high score (most of the time this is true- the high
	 * score only needs writing out to the file when you beat your
	 * all-time-high. */
	if(state->scores.hi <= state->scores.fhi) return;

	/* Generate the full path to the high score file: */
	snprintf(buf, 256, "%s/"HISCORE_FILE, state->gamedir);

	/* Try to open the high score file for writing. If there is an error,
	 * print a warning and return: */
	if(!(fp = fopen(buf, "w"))) {
		fprintf(stderr, "Couldn't open high score file for writing: "
				"%s\n", strerror(errno));
		return;
	}

	/* Try to print the new high score to the high score file. If it
	 * fails print an error message: */
	if((fprintf(fp, "%d", state->scores.hi)) == -1) {
		fprintf(stderr, "Couldn't write to high score file: %s",
							strerror(errno));
		/* Otherwise remember the new file high score: */
	} else state->scores.fhi = state->scores.hi;

	fclose(fp); /* Close the high score file. */
}
