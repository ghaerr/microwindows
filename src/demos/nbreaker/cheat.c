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

/* cheat.c contains do_cheat() which is used to handle the cheat sequences. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Used by do_cheat to compare each of the cheat sequences in turn to the
 * current cheat state. It differs from strcmp() in that it returns 0
 * ("no match") if the second parameter is null (ie. that cheat sequence is
 * undefined), it returns 1 ("match") if there is a definite match, and it
 * returns 2 ("partial match") if the first string is shorter than the second
 * string but the letters which are there match the initial portion of the
 * second string. This latter is used to determine when the current cheat
 * state has no chance of matching any of the cheat sequences so it can throw
 * away the current cheat state and start again. */
static int cheatcmp(char *a, char *b)
{
	int m = 0;

	/* If the cheat sequence is undefined, return "no match": */
	if(!b) return 0;

	/* While the current character of the cheat sequence matches the
	 * current character of the cheat state: */
	while(*a == *b) {
		/* If they are 0 (ie. the end of the strings) then both strings
		 * must match for their entire length, so return "match": */
		if(!*a) return 1;
		/* Since we got past the first comparison, at least the first
		 * character must match, so set the "match found" flag: */
		m = 1;
		/* Increment to the next letter in the strings: */
		a++;
		b++;
	}

	/* If we have reached the end of the cheat state but not the end of the
	 * cheat sequence and at least one matching character was found, return
	 * "partial match": */
	if(!*a && *b && m) return 2;
	else return 0; /* Otherwise return "no match". */
}

/* This is called by handle_keystroke_event() when it gets a key down event
 * for a key that it isn't interested in. This includes all ascii characters.
 * We maintain a buffer with the current list of letters received by this
 * function, and every time we get a new letter the entire buffer is compared
 * against all of the cheat sequences. If it fully matches one of the cheat
 * sequences, that cheat is activated and the buffer reset. If it matches only
 * the initial portion of at least one of the cheat sequences, the buffer is
 * kept as is. If it doesn't match the initial portion of any of the cheat
 * sequences, the buffer is emptied. */
void do_cheat(nbstate *state, GR_KEY key)
{
	int i, len, partial_match = 0;

	/* We aren't interested in any non-ascii keystrokes: */
	if(key > 127) return;

	/* Find the end of the buffer and paste the new letter on the end.
	 * We know the buffer can't overrun because the cheat sequences are
	 * limited to one less than the size of the buffer, and the buffer
	 * cannot grow to longer than the longest cheat sequence because it
	 * will either match one of the cheat sequences and get reset, or fail
	 * to match any of them and also get reset. */
       	len = strlen(state->cheatstate);
	state->cheatstate[len] = key;

	/* Check for a match against the SolidFloor cheat: */
	if((i = cheatcmp(state->cheatstate, state->cheats[SFCHEAT]))) {
		if(i == 1) { /* A complete match. */
			/* Toggle the SolidFloor flag: */
			state->flags.sf ^= 1;
			/* Reset the cheat state and return: */
			memset(state->cheatstate, 0, MAXCHEATLEN + 1);
			return;
		} else partial_match = 1; /* It was at least a partial match. */
	}

	/* Check for a match against the TelePort cheat: */
	if((i = cheatcmp(state->cheatstate, state->cheats[TPCHEAT]))) {
		if(i == 1) {
			/* Teleport to the next level: */
			increment_level(state);
			memset(state->cheatstate, 0, MAXCHEATLEN + 1);
			return;
		} else partial_match = 1;
	}

	/* Check for a match against the NoBounce cheat: */
	if((i = cheatcmp(state->cheatstate, state->cheats[NBCHEAT]))) {
		if(i == 1) {
			/* Toggle the NoBounce flag: */
			state->flags.nb ^= 1;
			memset(state->cheatstate, 0, MAXCHEATLEN + 1);
			return;
		} else partial_match = 1;
	}

	/* Check for a match against the NoPowerDown cheat: */
	if((i = cheatcmp(state->cheatstate, state->cheats[NPDCHEAT]))) {
		if(i == 1) {
			/* Toggle the NoPowerDown flag: */
			state->flags.npd ^= 1;
			memset(state->cheatstate, 0, MAXCHEATLEN + 1);
			return;
		} else partial_match = 1;
	}

	/* Check for a match against the NoPowerUpTimeOut cheat: */
	if((i = cheatcmp(state->cheatstate, state->cheats[NPUTOCHEAT]))) {
		if(i == 1) {
			/* Toggle the NoPowerUpTimeOut flag: */
			state->flags.nputo ^= 1;
			memset(state->cheatstate, 0, MAXCHEATLEN + 1);
			return;
		} else partial_match = 1;
	}

	/* If there wasn't even a partial match against any of the cheat
	 * sequences, reset the cheat state: */
	if(!partial_match) memset(state->cheatstate, 0, MAXCHEATLEN + 1);
}
