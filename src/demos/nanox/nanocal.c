/* 
 * A calibration program designed to use the Nano-X GrCalibrateMouse() API.
 * This program should theoretically work with any touch screen driver which
 * supports the API.
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
 * The Original Code is Nano-X-Calibrate
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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include <nxcolors.h>
#define FIX_PRECISION 10
#include <fixed.h>

#include "nanocal.h"

/*
 * Print the usage message and exit. Used when the wrong command line
 * parameters are entered.
 */
static void
usage(void)
{
	fprintf(stderr, "Usage: nxcal [-c]\n");
	exit(1);
}

/*
 * Print "Out of memory" and exit. Used when a memory allocation fails.
 */
static void
oom(void)
{
	fprintf(stderr, "Out of memory\n");
	exit(1);
}

/*
 * Set the Nano-X mouse calibration parameters from the values in the specified
 * calibration structure, and set the mouse mode to cooked (the normal mode
 * where the scaling and filtering is performed on the data).
 */
static void
set_calibration(cal_parms *c)
{
	GrCalibrateMouse(GR_MOUSE_MODE_COOKED, c->xmin, c->xmax, c->ymin,
		c->ymax, c->xswap, c->yswap, c->zthresh, c->showcursor);
}

/*
 * Loads the calibration parameters from the parameters file (usually /etc/
 * nxcal.cal). Returns a pointer to a newly allocated and filled in calibration
 * parameters structure on success, and a NULL pointer on failure. The format
 * is fairly strict in order to simplify the parsing code, and because the file
 * is meant to be generated automatically (it can be tweaked by hand if
 * necessary but it shouldn't usually be necessary).
 */
static cal_parms *
load_calibration(void)
{
	FILE *fp;
	char buf[50];
	int line = 0;
	cal_parms *cal, *g;
	
	if(!(fp = fopen(CALIBRATION_FILE, "r"))) {
		fprintf(stderr, "Couldn't open calibration file "
			CALIBRATION_FILE " for reading: %s\n", strerror(errno));
		return NULL;
	}

	if(!(cal = malloc(sizeof(cal_parms)))) oom();
	if(!(g = calloc(sizeof(cal_parms), 1))) oom();

	while(fgets(buf, 50, fp)) {
		line++;
		if(buf[0] == '#' || buf[0] == '\n') continue;
		if(!memcmp(buf, "xmin ", 5)) {
			cal->xmin = atoi(&buf[5]);
			g->xmin++;
		} else if(!memcmp(buf, "xmax ", 5)) {
			cal->xmax = atoi(&buf[5]);
			g->xmax++;
		} else if(!memcmp(buf, "ymin ", 5)) {
			cal->ymin = atoi(&buf[5]);
			g->ymin++;
		} else if(!memcmp(buf, "ymax ", 5)) {
			cal->ymax = atoi(&buf[5]);
			g->ymax++;
		} else if(!memcmp(buf, "xswap ", 6)) {
			cal->xswap = atoi(&buf[6]);
			g->xswap++;
		} else if(!memcmp(buf, "yswap ", 6)) {
			cal->yswap = atoi(&buf[6]);
			g->yswap++;
		} else if(!memcmp(buf, "zthresh ", 8)) {
			cal->zthresh = atoi(&buf[8]);
			g->zthresh++;
		} else if(!memcmp(buf, "showcursor ", 11)) {
			cal->showcursor = atoi(&buf[11]);
			g->showcursor++;
		} else fprintf(stderr, "Unknown parameter on line %d of "
				CALIBRATION_FILE "\n", line);
	}

	if(ferror(fp)) {
		fprintf(stderr, "An error occured while reading the "
				"calibration file " CALIBRATION_FILE ": %s\n",
				strerror(errno));
		free(cal);
		free(g);
		fclose(fp);
		return NULL;
	}

	if(!g->xmin || !g->xmax || !g->ymin || !g->ymax || !g->xswap ||
			!g->yswap || !g->zthresh || !g->showcursor) {
		fprintf(stderr, "Not all parameters were present in the "
				"calibration file " CALIBRATION_FILE "\n");
		free(cal);
		cal = NULL;
	}

	free(g);

	fclose(fp);

	return cal;
}

/*
 * Saves the contents of the specified calibration parameters structure to the
 * calibration file.
 */
static int
save_calibration(cal_parms *cal)
{
	FILE *fp;

	if(!(fp = fopen(CALIBRATION_FILE, "w"))) {
		fprintf(stderr, "Couldn't open calibration file "
				CALIBRATION_FILE " for writing: %s\n",
				strerror(errno));
		return 1;
	}

	fprintf(fp, "# Nano-X Touchscreen Calibration File\n");
	fprintf(fp, "xmin %d\n", cal->xmin);
	fprintf(fp, "xmax %d\n", cal->xmax);
	fprintf(fp, "ymin %d\n", cal->ymin);
	fprintf(fp, "ymax %d\n", cal->ymax);
	fprintf(fp, "xswap %d\n", cal->xswap);
	fprintf(fp, "yswap %d\n", cal->yswap);
	fprintf(fp, "zthresh %d\n", cal->zthresh);
	fprintf(fp, "showcursor %d\n", cal->showcursor);

	if(fclose(fp)) {
		fprintf(stderr, "Error writing to calibration file: %s\n",
			strerror(errno));
		return 1;
	} else return 0;
}

/*
 * Calculates how large a button containing the specified string would be when
 * using the font associated with the specified graphics context and fully
 * packed. Returns the width and height in the specified pointers.
 */ 
static void
calc_button_size(GR_GC_ID gc, char *str, int len, GR_SIZE *retwidth,
		GR_SIZE *retheight)
{
	int width, height, base;

	/* Calculate the text dimensions. */
	GrGetGCTextSize(gc, str, len, GR_TFTOP, &width, &height, &base);

	/* Calculate the total dimensions. */
	*retwidth = width + (2 * BUTTONOVERSIZE) + 2;
	*retheight = base + (2 * BUTTONOVERSIZE) + 2;
}

/*
 * Draws a button with the specified dimensions at the specified position on
 * the specified window with the specified contents using the specified
 * graphics context. The button is expected to be at least as large as the
 * values returned from a calc_button_size() call, and possibly larger.
 */
static void
draw_button(GR_WINDOW_ID wid, GR_GC_ID gc, char *str, int len, GR_COORD x,
		GR_COORD y, GR_SIZE width, GR_SIZE height)
{
	int w, h, base;
	GR_COORD textx, texty;

	/* Draw the enclosing rectangle */
	GrRect(wid, gc, x, y, width, height);

	/* Calculate the text dimensions. */
	GrGetGCTextSize(gc, str, len, GR_TFTOP, &w, &h, &base);

	/* Draw the text. */
	textx = x + (width / 2) - (w / 2);
	texty = y + (height / 2) - (base / 2);
	GrText(wid, gc, textx, texty, str, len, GR_TFTOP);
}

/*
 * Draws the box in the middle of the screen with the specified text and the
 * specified buttons in it (if any).
 */
static void
draw_text(GR_WINDOW_ID wid, GR_GC_ID tgc, GR_GC_ID bgc, GR_COORD xbase,
	GR_COORD ybase, const char *text[], char *button1, char *button2,
	cal_button *b1, cal_button *b2)
{
	GR_COORD x, y;
	int lines, i, b1len = 0, b2len = 0;
	GR_SIZE totalheight, *widths, *heights;
	GR_SIZE width, buttonwidth, height, buttonheight, base, maxwidth;

	/* Count the number of lines in the array. */
	for(lines = 0; text[lines]; lines++);
	if(!lines) return;

	/*
	 *  Work out the width and height in pixels of each line, the width of
	 * the widest line, and the total height of all lines combined.
	 */
	if(!(widths = malloc(sizeof(GR_SIZE) * lines))) oom();
	if(!(heights = malloc(sizeof(GR_SIZE) * lines))) oom();
	totalheight = maxwidth = 0;
	for(i = 0; i < lines; i++) {
		GrGetGCTextSize(i ? bgc : tgc, (char *)text[i], -1, GR_TFTOP,
				&width, &height, &base);
		heights[i] = height;
		totalheight += heights[i];
		widths[i] = width;
		if(width > maxwidth) maxwidth = width;
	}

	/* Work out the button size */
	if(button1) {
		b1len = strlen(button1);
		b2len = strlen(button2);
		calc_button_size(bgc, button1, b1len, &width, &height);
		calc_button_size(bgc, button2, b2len, &buttonwidth,
				&buttonheight);
		if(height > buttonheight) buttonheight = height;
		if(width > buttonwidth) buttonwidth = width;
		totalheight += buttonheight + BUTTONSPACING;
		width = (2 * buttonwidth) + BUTTONSPACING;
		if(width > maxwidth) maxwidth = width;
	} else {
		buttonwidth = 0;
		buttonheight = 0;
	}

	/* Draw a box around the text. */
	GrRect(wid, tgc, xbase - (maxwidth / 2) - (BOXOVERSIZE / 2),
			ybase - (totalheight / 2) - (BOXOVERSIZE / 2),
			maxwidth + BOXOVERSIZE, totalheight + BOXOVERSIZE);

	/* Calculate where to start drawing the text block. */
	y = ybase - (totalheight / 2);

	/* Draw the text. */
	for(i = 0; i < lines; i++) {
		x = xbase - (widths[i] / 2);
		GrText(wid, i ? bgc : tgc, x, y, (char *)text[i],
				strlen(text[i]), GR_TFTOP);
		y += heights[i];
	}

	/* Draw the buttons. */
	if(button1) {
		y += BUTTONSPACING;
		x = xbase - (BUTTONSPACING / 2) - buttonwidth;
		draw_button(wid, bgc, button1, b1len, x, y, buttonwidth,
				buttonheight);
		b1->maxx = xbase - (BUTTONSPACING / 2) - 1;
		b1->minx = x;
		b1->maxy = y + buttonheight - 1;
		b1->miny = y;
		x = xbase + (BUTTONSPACING / 2);
		draw_button(wid, bgc, button2, b2len, x, y, buttonwidth,
				buttonheight);
		b2->maxx = x + buttonwidth - 1;
		b2->minx = x;
		b2->maxy = y + buttonheight - 1;
		b2->miny = y;
	}

	free(heights);
	free(widths);
}

/*
 * Draws a cross hair target on the specified window with the specified
 * graphics context centred on the specified coordinates.
 */
static void
draw_target(GR_WINDOW_ID wid, GR_GC_ID gc, GR_COORD xbase, GR_COORD ybase)
{
	GrLine(wid, gc, xbase, ybase - (CROSS_SIZE / 2),
			xbase, ybase + CROSS_SIZE / 2);
	GrLine(wid, gc, xbase - (CROSS_SIZE / 2), ybase,
			xbase + (CROSS_SIZE / 2), ybase);
	GrEllipse(wid, gc, xbase, ybase, (CROSS_SIZE * 3) / 8,
			(CROSS_SIZE * 3) / 8);
}

/*
 * Redraws the screen, taking into account the current program state to decide
 * exactly what should displayed. We don't bother with only redrawing the
 * parts of the screen which have been exposed because it shouldn't happen
 * very often (this is supposed to be the top window on the screen above
 * everything else for the duration of the calibration procedure).
 */
static void
redraw(cal_state *state)
{
	char *button1 = NULL, *button2 = NULL;
	const char **text;
	int targetx = -1, targety = -1;

	/* The state machine which decides what to draw in the current state. */
	switch(state->state) {
		case STATE_TARGET1:
			targetx = TARGET_DIST;
			targety = TARGET_DIST;
			text = screen1_text;
			break;
		case STATE_TARGET2:
			targetx = state->si.cols - TARGET_DIST;
			targety = TARGET_DIST;
			text = screen1_text;
			break;
		case STATE_TARGET3:
			targetx = state->si.cols - TARGET_DIST;
			targety = state->si.rows - TARGET_DIST;
			text = screen1_text;
			break;		
		case STATE_TARGET4:
			targetx = TARGET_DIST;
			targety = state->si.rows - TARGET_DIST;
			text = screen1_text;
			break;
		case STATE_CHECKCAL:
			button1 = BACK_BUTTON;
			button2 = OK_BUTTON;
			text = screen2_text;
			break;
		case STATE_ASKPOINTER:
			button1 = YES_BUTTON;
			button2 = NO_BUTTON;
			text = screen3_text;
			break;
		case STATE_FINISHED:
		default:
			return;
	}

	/* Clear the whole screen. */
	GrClearArea(state->wid, 0, 0, state->si.cols, state->si.rows, 0);

	/* If this state calls for a target, draw it. */
	if(targetx > 0) draw_target(state->wid, state->tgc, targetx, targety);

	/* Draw the text and possibly buttons in the middle of the screen. */
	draw_text(state->wid, state->tgc, state->bgc, state->si.cols / 2,
			state->si.rows / 2, text, button1, button2,
			&state->buttons[0], &state->buttons[1]);
}

/*
 * The heart of the program and the hardest part to get right, this is the
 * routine which decides what the calibration values should be based on four
 * readings taken by pressing the pen on a target in each corner of the screen.
 * The order of the readings taken is NW, NE, SE, SW, and the targets are
 * TARGET_DIST pixels away from the corner of the screen..
 */
static void
calculate_parms(cal_state *state)
{
	int xcomp, ycomp;
	fix x_ratio, y_ratio;
	int i, zthresh = 0, xmax = 0, xmin = 0, ymax = 0, ymin = 0, temp;

	/* Work out the mean peak pressure of the four clicks. */
	for(i = 0; i < 4; i++) zthresh += state->maxz[i];
	zthresh /= 4;

	/* Set the threshold to about 7/8 of the mean peak. */
	state->parms->zthresh = ZTHRESH_REDUCE(zthresh);

	/* Calculate the mean maximum and minimum X and Y */
	xmin = (state->xpoints[0] + state->xpoints[3]) / 2;
	xmax = (state->xpoints[1] + state->xpoints[2]) / 2;
	ymin = (state->ypoints[0] + state->ypoints[1]) / 2;
	ymax = (state->ypoints[2] + state->ypoints[3]) / 2;

	/* Figure out whether we need to swap the values or not, and if so then
	 * do it. */
	if(xmin > xmax) {
		state->parms->xswap = GR_TRUE;
		temp = xmin;
		xmin = xmax;
		xmax = temp;
	} else state->parms->xswap = GR_FALSE;
	if(ymin > ymax) {
		state->parms->yswap = GR_TRUE;
		temp = ymin;
		ymin = ymax;
		ymax = temp;
	} else state->parms->yswap = GR_FALSE;

	/* Calculate (using fixed point) the ratio of screen pixels to
	 * touchscreen values. */
	x_ratio = fixdiv(inttofix(xmax - xmin),
			inttofix(state->si.cols - (TARGET_DIST * 2)));
	y_ratio = fixdiv(inttofix(ymax - ymin),
			inttofix(state->si.rows - (TARGET_DIST * 2)));

	/* Calculate how many touchscreen values is equal to TARGET_DIST
	 * pixels so we can compensate for the distance between the edge
	 * of the screen and the centre of the crosses. */
	xcomp = fixtoint(fixmult(inttofix(TARGET_DIST), x_ratio));
	ycomp = fixtoint(fixmult(inttofix(TARGET_DIST), y_ratio));

	/* Apply the compensation values. */
	xmax += xcomp;
	xmin -= xcomp;
	ymax += ycomp;
	ymin -= ycomp;

	/* Store the calculated values in the calibration parameters struct. */
	state->parms->xmin = xmin;
	state->parms->xmax = xmax;
	state->parms->ymin = ymin;
	state->parms->ymax = ymax;
}

/*
 * Called whenever we get a mouse button down event for each button structure.
 * If the pointer is within the bounds of the button, it is marked as
 * pressed, otherwise it is marked as unpressed.
 */
static void
button_down(cal_button *button, int x, int y)
{
	if(x >= button->minx && x <= button->maxx && y >= button->miny &&
						y <= button->maxy) {
		button->pressed = GR_TRUE;
	} else button->pressed = GR_FALSE;
}

/*
 * Called whenever we get a mouse button up event for each button structure.
 * If the pointer is within the bounds of the button and it is marked as
 * pressed, then the function returns TRUE, otherwise it returns false. In
 * either case the button is marked as unpressed. This behaviour means that
 * in order to activate a button you have to touch the pen down and up on the
 * button, sliding the pen onto the button after you have already pressed it
 * down or sliding the pen off the button after you have pressed down will not
 * activate it.
 */
static int
button_up(cal_button *button, int x, int y)
{
	int click = GR_FALSE;

	if(button->pressed) {
		button->pressed = GR_FALSE;
		if(x >= button->minx && x <= button->maxx &&
				y >= button->miny && y <= button->maxy) {
			click = GR_TRUE;
		}
	}

	return click;
}

/*
 * Called whenever we get a mouse event and are in the "Check Calibration"
 * state. When the pen is pressed down or released we change the mouse pointer
 * to indicate the change so that the user can tell exactly how much pressure
 * is required to activate the click. Also does the button processing for the
 * BACK and OK buttons.
 */ 
static void
mouse_checkcal(cal_state *state, GR_EVENT_MOUSE *event)
{
	int i;

	if(event->buttons & GR_BUTTON_L) {
		if(!state->button_down) {
			GrSetWindowCursor(state->wid, state->downcurs);
			button_down(&state->buttons[0], event->x, event->y);
			button_down(&state->buttons[1], event->x, event->y);
			state->button_down = GR_TRUE;
		}
	} else {
		if(state->button_down) {
			GrSetWindowCursor(state->wid, state->upcurs);
			if(button_up(&state->buttons[0], event->x, event->y)) {
				GrDestroyTimer(state->tid);
				GrCalibrateMouse(GR_MOUSE_MODE_RAW, 0, 0, 0,
								0, 0, 0, 0, 0);
				state->state = STATE_TARGET1;
				redraw(state);
				for(i = 0; i < 4; i++) state->maxz[i] = 0;
			}
			if(button_up(&state->buttons[1], event->x, event->y)) {
				GrDestroyTimer(state->tid);
				state->state = STATE_ASKPOINTER;
				redraw(state);
				state->buttons[0].pressed = GR_FALSE;
				state->buttons[1].pressed = GR_FALSE;
			}
			state->button_down = GR_FALSE;
		}
	}
}

/*
 * Called whenever we get a mouse event and are in the "Ask if the pointer
 * should be visible" state. Handles the YES and NO buttons.
 */
static void
mouse_askpointer(cal_state *state, GR_EVENT_MOUSE *event)
{
	if(event->buttons & GR_BUTTON_L) {
		if(!state->button_down) {
			button_down(&state->buttons[0], event->x, event->y);
			button_down(&state->buttons[1], event->x, event->y);
			state->button_down = GR_TRUE;
		}
	} else {
		if(state->button_down) {
			if(button_up(&state->buttons[0], event->x, event->y)) {
				state->state = STATE_FINISHED;
				state->parms->showcursor = GR_TRUE;
			}
			if(button_up(&state->buttons[1], event->x, event->y)) {
				state->state = STATE_FINISHED;
				state->parms->showcursor = GR_FALSE;
			}
			state->button_down = GR_FALSE;
		}
	}
}

/*
 * Called to enter debounce mode, where mouse events are ignored until a timer
 * expires. This is so we don't end up entering two calibration points in the 
 * same place when the pointer bounces when you tap it against the screen.
 */
static void
start_debounce(cal_state *state)
{
	state->debounce = GR_TRUE;
	state->tid = GrCreateTimer(state->wid, DEBOUNCE_TIME, GR_FALSE);
}

/*
 * Called whenever we get a mouse motion event. Contains a state machine which
 * handles the four target states and calls seperate functions to handle the
 * "check calibration" and "ask if the pointer should be visible" states.
 */
static void
mouse_motion(cal_state *state, GR_EVENT_MOUSE *event)
{
	/* Ignore events which occur during the debounce period. */
	if(state->debounce) return;

	switch(state->state) {
		case STATE_TARGET1:
			if(event->z > state->maxz[0]) {
				state->maxz[0] = event->z;
				state->xpoints[0] = event->x;
				state->ypoints[0] = event->y;
			} else if(event->z < (state->maxz[0] / 2)) {
				state->state = STATE_TARGET2;
				redraw(state);
				start_debounce(state);
			}
			break;
		case STATE_TARGET2:
			if(event->z > state->maxz[1]) {
				state->maxz[1] = event->z;
				state->xpoints[1] = event->x;
				state->ypoints[1] = event->y;
			} else if(event->z < (state->maxz[1] / 2)) {
				state->state = STATE_TARGET3;
				redraw(state);
				start_debounce(state);
			}
			break;
		case STATE_TARGET3:
			if(event->z > state->maxz[2]) {
				state->maxz[2] = event->z;
				state->xpoints[2] = event->x;
				state->ypoints[2] = event->y;
			} else if(event->z < (state->maxz[2] / 2)) {
				state->state = STATE_TARGET4;
				redraw(state);
				start_debounce(state);
			}
			break;
		case STATE_TARGET4:
			if(event->z > state->maxz[3]) {
				state->maxz[3] = event->z;
				state->xpoints[3] = event->x;
				state->ypoints[3] = event->y;
			} else if(event->z < (state->maxz[3] / 2)) {
				state->state = STATE_CHECKCAL;
				redraw(state);
				calculate_parms(state);
				state->parms->showcursor = GR_TRUE;
				set_calibration(state->parms);
				GrSetWindowCursor(state->wid, state->upcurs);
				state->buttons[0].pressed = GR_FALSE;
				state->buttons[1].pressed = GR_FALSE;
				state->tid = GrCreateTimer(state->wid,
						1000 * CHECKCAL_TIMEOUT,
						GR_FALSE);
			}
			break;
		case STATE_CHECKCAL:
			mouse_checkcal(state, event);
			break;
		case STATE_ASKPOINTER:
			mouse_askpointer(state, event);
			break;
		default:
			break;
	}
}

/*
 * Creates the two cursors which are used to indicate "pen pressed down" and
 * "pen not pressed down" in the "check calibration" screen.
 */
static void
make_cursors(cal_state *state)
{
	GR_BITMAP ufg[] = UPCURSORBITS;
	GR_BITMAP ubg[] = UPCURSORMASK;
	GR_BITMAP dfg[] = DOWNCURSORBITS;
	GR_BITMAP dbg[] = DOWNCURSORMASK;

	state->upcurs = GrNewCursor(UPCURSORWIDTH, UPCURSORHEIGHT,
			UPCURSORXHOT, UPCURSORYHOT, BACKGROUND_COLOUR,
			FOREGROUND_COLOUR, ufg, ubg);

	state->downcurs = GrNewCursor(DOWNCURSORWIDTH, DOWNCURSORHEIGHT,
			DOWNCURSORXHOT, DOWNCURSORYHOT, BACKGROUND_COLOUR,
			FOREGROUND_COLOUR, dfg, dbg);
}

/*
 * Called when a timer expires. We use timers for two things, but not both at
 * the same state. The first one is the debounce timer, which is used to ignore
 * mouse events which come in less than a certain minimum time after we change
 * to a new target (because sometimes the pen bounces slightly when you tap a
 * target and without it you could easily register two presses in the same
 * place). The second one we use is the one shot that is used to automatically
 * go back from the "Check Calibration" screen to try calibrating again if the
 * user does not click on a button within a certain amont of time (this is in
 * case the user has clicked in the wrong places and the calibration is so bad
 * as a result that they can't click on the "BACK" button).
 */
static void
timer_event(cal_state *state, GR_EVENT_TIMER *event)
{
	int i;

	/* A quick sanity check to make sure this is the timer event that we
	 * expected. */
	if(event->tid != state->tid) return;

	switch(state->state) {
		case STATE_CHECKCAL:
			/* Change the state back to the first calibration
			 *state (TARGET1). */
			GrCalibrateMouse(GR_MOUSE_MODE_RAW, 0, 0, 0, 0, 0, 0,
									0, 0);
			state->state = STATE_TARGET1;
			redraw(state);
			for(i = 0; i < 4; i++) state->maxz[i] = 0;
			break;
		case STATE_TARGET2:
		case STATE_TARGET3:
		case STATE_TARGET4:
			state->debounce = GR_FALSE;
			break;
		default:
			break;
	}
}

/*
 * The main calibration function. Called when no valid calibration file could
 * be read or when the "force recalibration" command line option was used.
 * Returns a filled in calibration parameters structure on success or a NULL
 * pointer on failure.
 */
static cal_parms *
calibrate(void)
{
	int i;
	GR_EVENT ev;
	cal_parms *parms;
	cal_state *state;
	GR_FONT_ID tfid, bfid;
	GR_WM_PROPERTIES props;

	/* Allocate the calibration state structure and the calibration
	 * parameters structure. */
	if(!(state = malloc(sizeof(cal_state)))) oom();
	if(!(parms = malloc(sizeof(cal_parms)))) oom();
	state->parms = parms;

	/* Set up some default initial state variables. */
	state->button_down = GR_FALSE;
	state->debounce = GR_FALSE;
	state->state = 0;
	for(i = 0; i < 4; i++) state->maxz[i] = 0;

	/* Put Nano-X into raw mouse mode, which causes it to return
	 * touchscreen events directly to the client with no scaling and
	 * minimal filtration. */
	GrCalibrateMouse(GR_MOUSE_MODE_RAW, 0, 0, 0, 0, 0, 0, 0, 0);

	/* Get some information about the Nano-X display (in particular, its
	 * width and height. */
	GrGetScreenInfo(&state->si);

	/* Set up the graphics context used when drawing the title of the text
	 * box in the middle of the screen. */
	state->tgc= GrNewGC();
	GrSetGCForeground(state->tgc, FOREGROUND_COLOUR);
	GrSetGCBackground(state->tgc, BACKGROUND_COLOUR);
	tfid = GrCreateFont(TITLE_FONT, 0, NULL);
	GrSetFontAttr(tfid, GR_TFUNDERLINE, 0);
	GrSetGCFont(state->tgc, tfid);

	/* Set up the graphics context used when drawing the ordinary body text
	 * in the middle of the screen, based on the title graphics context
	 * with a different font. */
	state->bgc = GrCopyGC(state->tgc);
	bfid = GrCreateFont(BODY_FONT, 0, NULL);
	GrSetGCFont(state->bgc, bfid);

	/* Create the main window covering the full screen. */
	state->wid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, state->si.cols,
			state->si.rows, 0, BACKGROUND_COLOUR, 0);

	/* Select the events we want to receive for the main window. */
	GrSelectEvents(state->wid, GR_EVENT_MASK_EXPOSURE |
				GR_EVENT_MASK_MOUSE_MOTION |
				GR_EVENT_MASK_TIMER);

	/* Set up the window properties such that the window manager will
	 * leave our window totally alone. */
	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NOMOVE | GR_WM_PROPS_NODECORATE |
			GR_WM_PROPS_NOAUTOMOVE | GR_WM_PROPS_NOAUTORESIZE;
	GrSetWMProperties(state->wid, &props);

	/* Make the two cursors used by the "Check Calibration" screen. */
	make_cursors(state);

	/* Make the main window visible. */
	GrMapWindow(state->wid);

	/* Draw the initial contents of the main window. */
	redraw(state);

	/* The main event loop which is called continuously until we enter the
	 * finished state. */
	while(state->state != STATE_FINISHED) {
		GrGetNextEvent(&ev);
		switch(ev.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				redraw(state);
				break;
			case GR_EVENT_TYPE_MOUSE_MOTION:
				mouse_motion(state, &ev.mouse);
				break;
			case GR_EVENT_TYPE_TIMER:
				timer_event(state, &ev.timer);
				break;
			default:
				fprintf(stderr, "Got unknown event type %d\n",
						ev.type);
		}
	}

	/* Remove the underline attribute from the font we used for the title
	 * text, otherwise will stay active and other applications will end
	 * up drawing underlined text when they don't want to. */
	GrSetFontAttr(tfid, 0, GR_TFUNDERLINE);

	free(state);

	/* Save the newly generated calibration parameters out to the
	 * calibration parameters file so that they will be read from there
	 * the next time the program is started instead of requiring the user
	 * to go through the calibration procedure again. */
	save_calibration(parms);

	return parms;
}

int main(int argc, char *argv[])
{
	int force_calibrate = 0;
	cal_parms *calibration = NULL;

	/* Parse the command line options. */
	if(argc > 2) usage();
	if(argc == 2) {
		if(!strcmp(argv[1], "-c")) force_calibrate = 1;
		else usage();
	}

	/* Try to load the calibration parameters from the parameters file
	 * unless the "force calibration" command line parameter was set. */
	if(!force_calibrate) calibration = load_calibration();

	/* Connect to the Nano-X server. */
	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	/* If the file didn't load successfully (perhaps this is the first time
	 * we have run on this machine and it doesn't exist yet), or we were
	 * forced to recalibrate using the command line parameter, then perform
	 * the calibration procedure. */
	if(!calibration) calibration = calibrate();

	/* If we have managed to obtain a set of calibration parameters, either
	 * from the parameters file or from performing the calibration
	 * procedure, then tell the Nano-X server what they are. */
	if(calibration) {
		set_calibration(calibration);
	} else {
		fprintf(stderr, "Unable to determine calibration parameters\n");
		GrClose();
		return 1;
	}

	GrClose();

	return 0;
}
