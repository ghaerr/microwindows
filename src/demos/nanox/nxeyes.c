/* 
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
 * The Original Code is NanoXEyes.
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

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nxeyes.h"

#define EYE_SPACING 4
#define BLINK_CLOSED_TIME 100
#define BLINK_MIN_OPEN_TIME 1000
#define BLINK_MAX_OPEN_TIME 8000
#define MAX_EYE_MOVEMENT 10

typedef struct {
	GR_WINDOW_ID wid;
	GR_GC_ID gc;
	int button_down;
	int mouse_moved;
	GR_COORD x;
	GR_COORD y;
	GR_COORD mousex;
	GR_COORD mousey;
	GR_COORD oldx;
	GR_COORD oldy;
	int eyes_closed;
	GR_TIMER_ID tid;
	GR_COORD lx;
	GR_COORD ly;
	GR_COORD rx;
	GR_COORD ry;
	GR_COORD olx;
	GR_COORD oly;
	GR_COORD orx;
	GR_COORD ory;
	int quit;
} nxeyes_state;

nxeyes_state *init(void);
void button_event(GR_EVENT_BUTTON *ev, nxeyes_state *state);
void position_event(GR_EVENT_MOUSE *ev, nxeyes_state *state);
void start_blink_timer(nxeyes_state *state);
void timer_event(GR_EVENT_TIMER *ev, nxeyes_state *state);
void calculate_pupil_position(GR_COORD window_x, GR_COORD window_y,
		GR_COORD eyecentre_x, GR_COORD eyecentre_y,
		GR_COORD mousex, GR_COORD mousey,
		GR_COORD *retx, GR_COORD *rety);
void draw_eyes(nxeyes_state *state, int full_redraw);

nxeyes_state *init(void)
{
	nxeyes_state *state;
	GR_REGION_ID rid1, rid2;
	GR_SCREEN_INFO si;
	GR_WM_PROPERTIES props;

	if(!(state = malloc(sizeof(nxeyes_state)))) return NULL;
	state->oldx = state->oldy = state->x = state->y = 0;
	state->button_down = state->eyes_closed = state->quit = 0;
	state->olx = state->orx = EYEMASK_WIDTH / 2;
	state->oly = state->ory = EYEMASK_HEIGHT / 2;
	GrGetScreenInfo(&si);
	state->mousex = si.xpos;
	state->mousey = si.ypos;
	state->mouse_moved = 1;
	state->gc = GrNewGC();
	GrSetGCForeground(state->gc, GR_COLOR_WHITE);
	GrSetGCBackground(state->gc, GR_COLOR_BLACK);

	state->wid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, (EYEFG_HEIGHT * 2) +
			EYE_SPACING, EYEFG_HEIGHT, 0, GR_COLOR_WHITE, 0);

	rid1 = GrNewBitmapRegion(eyemask_bits, EYEMASK_WIDTH, EYEMASK_HEIGHT);
	rid2 = GrNewBitmapRegion(eyemask_bits, EYEMASK_WIDTH, EYEMASK_HEIGHT);
	GrOffsetRegion(rid2, EYEMASK_WIDTH + EYE_SPACING, 0);
	GrUnionRegion(rid1, rid1, rid2);
	GrSetWindowRegion(state->wid, rid1, GR_WINDOW_BOUNDING_MASK);
	GrDestroyRegion(rid1);
	GrDestroyRegion(rid2);

	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NODECORATE;
	GrSetWMProperties(state->wid, &props);

	GrSelectEvents(state->wid, GR_EVENT_MASK_CLOSE_REQ |
			GR_EVENT_MASK_MOUSE_POSITION |
			GR_EVENT_MASK_BUTTON_UP |
			GR_EVENT_MASK_BUTTON_DOWN |
			GR_EVENT_MASK_EXPOSURE |
			GR_EVENT_MASK_TIMER);

	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(state->wid);

	srand(time(0));
	start_blink_timer(state);
	
	return state;
}

void button_event(GR_EVENT_BUTTON *ev, nxeyes_state *state)
{
	if(ev->wid != state->wid) return;
	if(ev->type == GR_EVENT_TYPE_BUTTON_DOWN) {
		if(ev->buttons & GR_BUTTON_L) {
			state->button_down = 1;
			state->x = ev->x;
			state->y = ev->y;
			GrRaiseWindow(state->wid);
		}
		if(ev->buttons & GR_BUTTON_R) state->quit = 1;
	} else state->button_down = 0;
}

void position_event(GR_EVENT_MOUSE *ev, nxeyes_state *state)
{
	GR_COORD newx, newy;

	state->mouse_moved = 1;
	state->mousex = ev->rootx;
	state->mousey = ev->rooty;

	if(ev->wid == GR_ROOT_WINDOW_ID) return;

	if(!state->button_down) return;

	newx = ev->rootx - state->x;
	newy = ev->rooty - state->y;

	if(newx != state->oldx || newy != state->oldy) {
		GrMoveWindow(ev->wid, newx, newy);
		state->oldx = newx;
		state->oldy = newy;
	}
}

void start_blink_timer(nxeyes_state *state)
{
	int time;

	if(state->eyes_closed) time = BLINK_CLOSED_TIME;
	else time = (rand() % (BLINK_MAX_OPEN_TIME - BLINK_MIN_OPEN_TIME)) +
		BLINK_MIN_OPEN_TIME;

	state->tid = GrCreateTimer(state->wid, time); /*, GR_FALSE); */
}

void timer_event(GR_EVENT_TIMER *ev, nxeyes_state *state)
{
	if(ev->tid != state->tid) return;

	/* Must free timer - even though it's fired, the handle and data structure live on. */
	GrDestroyTimer(state->tid);
	state->tid = 0;
	
	if(state->eyes_closed) {
		state->eyes_closed = 0;
		start_blink_timer(state);
	} else {
		state->eyes_closed = 1;
		start_blink_timer(state);
	}

	draw_eyes(state, 1);
}

void calculate_pupil_position(GR_COORD window_x, GR_COORD window_y,
		GR_COORD eyecentre_x, GR_COORD eyecentre_y,
		GR_COORD mousex, GR_COORD mousey,
		GR_COORD *retx, GR_COORD *rety)
{
	double a;
	int revx, revy;
	GR_COORD rx, ry, dx, dy, x, y;

	rx = eyecentre_x + window_x;
	dx = mousex - rx;
	if(dx < 0) {
		dx = -dx;
		revx = 1;
	} else revx = 0;

	ry = eyecentre_y + window_y;
	dy = mousey - ry;
	if(dy < 0) {
		dy = -dy;
		revy = 1;
	} else revy = 0;

	if(hypot(dx, dy) <= MAX_EYE_MOVEMENT) {
		x = dx;
		y = dy;
	} else {
		a = atan((double)dy / (double)dx);
		x = (GR_COORD)(cos(a) * (double)MAX_EYE_MOVEMENT);
		y = (GR_COORD)(sin(a) * (double)MAX_EYE_MOVEMENT);
	}

	if(revx) x = -x;
	if(revy) y = -y;

	*retx = x + eyecentre_x - (PUPIL_WIDTH / 2);
	*rety = y + eyecentre_y - (PUPIL_WIDTH / 2);
}

void draw_eyes(nxeyes_state *state, int full_redraw)
{
	GR_COORD x, y;
	GR_BITMAP *bits;

	if(full_redraw) {
		if(state->eyes_closed) bits = eyeclose_bits;
		else bits = eyefg_bits;

		GrBitmap(state->wid, state->gc, 0, 0, EYEFG_WIDTH,
				EYEFG_HEIGHT, bits);
		GrBitmap(state->wid, state->gc, EYEFG_WIDTH + EYE_SPACING, 0,
				EYEFG_WIDTH, EYEFG_HEIGHT, bits);
	}

	if(state->mouse_moved) {
		x = EYEFG_WIDTH / 2;
		y = EYEFG_HEIGHT / 2;
		calculate_pupil_position(state->oldx, state->oldy,
			(EYEFG_WIDTH / 2),
			(EYEFG_HEIGHT / 2), state->mousex,
			state->mousey, &state->lx, &state->ly);
		calculate_pupil_position(state->oldx, state->oldy,
			(EYEFG_WIDTH / 2) + EYEFG_WIDTH
			+ EYE_SPACING, (EYEFG_WIDTH / 2),
			state->mousex, state->mousey, &state->rx, &state->ry);
	}

	if(state->eyes_closed) return;

	GrClearArea(state->wid, state->olx, state->oly, PUPIL_WIDTH,
			PUPIL_HEIGHT, 0);
	GrBitmap(state->wid, state->gc, state->lx, state->ly, PUPIL_WIDTH,
			PUPIL_HEIGHT, pupil_bits);
	GrClearArea(state->wid, state->orx, state->ory, PUPIL_WIDTH,
			PUPIL_HEIGHT, 0);
	GrBitmap(state->wid, state->gc, state->rx, state->ry, PUPIL_WIDTH,
			PUPIL_HEIGHT, pupil_bits);

	state->olx = state->lx;
	state->oly = state->ly;
	state->orx = state->rx;
	state->ory = state->ry;
}

int main(void)
{
	GR_EVENT event;
	nxeyes_state *state;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	if(!(state = init())) {
		fprintf(stderr, "Couldn't allocate state structure\n");
		return 1;
	}

	while(!state->quit) {
		GrGetNextEvent(&event);
		switch(event.type) {
			case GR_EVENT_TYPE_CLOSE_REQ:
				state->quit = 1;
				break;
			case GR_EVENT_TYPE_MOUSE_POSITION:
				position_event(&event.mouse, state);
				break;
			case GR_EVENT_TYPE_BUTTON_UP:
			case GR_EVENT_TYPE_BUTTON_DOWN:
				button_event(&event.button, state);
				break;
			case GR_EVENT_TYPE_EXPOSURE:
				draw_eyes(state, 1);
				break;
			case GR_EVENT_TYPE_TIMER:
				timer_event(&event.timer, state);
				break;
			default:
				break;
		}
		if(state->mouse_moved) {
			draw_eyes(state, 0);
			state->mouse_moved = 0;
		}
	}

	GrClose();
	free(state);
	
	return 0;
}
