/*
 * nxmag - A Nano-X real time magnifier program by Alex Holden.
 *
 * The command line parameters are:
 * -w <width> gives the width in pixels of the area around the pointer to use.
 * -h <height> gives the height in pixels of the area around the pointer to use.
 * -m <factor> gives the magnification factor.
 * -u <update period> gives the time in ms between updating the output window
 * if the pointer has moved.
 * -f <force rate> gives the number of updates periods which should be missed
 * the update is forced even though the pointer has not moved. Set to 0 to
 * always update even if the pointer has not moved.
 */

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
 * The Original Code is Nano-X-Magnifier.
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
#include <stdlib.h>
#include <time.h>

#include <nano-X.h>
#include <nxcolors.h>

#define DEFAULT_MAGNIFICATION 10
#define DEFAULT_WIDTH 15
#define DEFAULT_HEIGHT 15
#define DEFAULT_UPDATE_PERIOD 40
#define DEFAULT_FORCEUPDATE 15

typedef struct {
	int x;
	int y;
	int lx;
	int ly;
	int fu;
	int width;
	int height;
	int pwidth;
	int pheight;
	int forceupdate;
	int magnification;
	GR_GC_ID gc;
	GR_WINDOW_ID wid;
	GR_PIXELVAL *inbuf;
	GR_PIXELVAL *outbuf;
} nmstate;

static void read_input(nmstate *state)
{
	GrReadArea(GR_ROOT_WINDOW_ID, state->x, state->y, state->width,
			state->height, state->inbuf);
}

static void magnify(nmstate *state)
{
	int x, y, i;
	GR_PIXELVAL pix, *p, *pp = state->outbuf;
	
	for(y = 0; y < state->pheight; y++) {
		p = state->inbuf + ((y / state->magnification) * state->width);
		for(x = 0; x < state->width; x++) {
			pix = *p++;
			for(i = 0; i < state->magnification; i++)
				*pp++ = pix;
		}
	}
}

static void draw_output(nmstate *state)
{
	GrArea(state->wid, state->gc, 0, 0, state->pwidth, state->pheight,
			state->outbuf, MWPF_PIXELVAL);
}

static void do_update(nmstate *state)
{
	if(state->x == state->lx && state->y == state->ly &&
			state->forceupdate && --state->fu)
		return;

	read_input(state);
	magnify(state);
	draw_output(state);

	state->lx = state->x;
	state->ly = state->y;
	state->fu = state->forceupdate;
}

static void mouse_moved(nmstate *state, GR_EVENT_MOUSE *ev)
{
	state->x = ev->rootx - state->width / 2;
	state->y = ev->rooty - state->height / 2;
}

static nmstate *init(int argc, char **argv)
{
	int i;
	char *p;
	GR_COORD x = 0;
	nmstate *state;
	GR_SCREEN_INFO si;
	GR_WM_PROPERTIES props;
	int updateperiod = DEFAULT_UPDATE_PERIOD;

	if(!(state = malloc(sizeof(nmstate)))) return NULL;

	state->width = DEFAULT_WIDTH;
	state->height = DEFAULT_HEIGHT;
	state->forceupdate = DEFAULT_FORCEUPDATE;
	state->magnification = DEFAULT_MAGNIFICATION;

	argv++;
	argc--;
	while(argc > 0) {
		if(argv[0][0] != '-') goto badargs;
		if(argc < 2) goto badargs;
		i = strtol(argv[1], &p, 10);
		if(*p) goto badargs;
		switch(argv[0][1]) {
			case 'w':
				state->width = i;
				break;
			case 'h':
				state->height = i;
				break;
			case 'm':
				state->magnification = i;
				break;
			case 'u':
				updateperiod = i;
				break;
			case 'f':
				state->forceupdate = i;
				break;
			default:
				goto badargs;
		}
		argc -= 2;
		argv += 2;
	}

	if(GrOpen() < 0) {
		free(state);
		return NULL;
	}

	state->pwidth = state->width * state->magnification;
	state->pheight = state->height * state->magnification;

	if(!(state->inbuf = malloc(sizeof(GR_PIXELVAL) * state->width *
					state->height))) {
		free(state);
		return NULL;
	}

	if(!(state->outbuf = calloc(sizeof(GR_PIXELVAL) * state->pwidth *
					state->pheight, 1))) {
		free(state->inbuf);
		free(state);
		return NULL;
	}

	GrGetScreenInfo(&si);
	state->x = si.xpos - state->width / 2;
	state->y = si.ypos - state->height / 2;
	state->lx = -1;
	state->ly = -1;

	state->wid = GrNewWindow(GR_ROOT_WINDOW_ID, x, 0, state->pwidth,
				state->pheight, 0, GR_COLOR_WHITE, 0);

	GrSelectEvents(state->wid, GR_EVENT_TYPE_EXPOSURE |
					GR_EVENT_MASK_TIMER |
					GR_EVENT_MASK_CLOSE_REQ);

	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_MOUSE_POSITION);

	props.flags = GR_WM_FLAGS_TITLE;
	props.title = "nxmag";
	GrSetWMProperties(state->wid, &props);

	GrMapWindow(state->wid);

	state->gc = GrNewGC();

	GrCreateTimer(state->wid, updateperiod); /*, GR_TRUE); */

	do_update(state);

	return state;

badargs:
	fprintf(stderr, "usage: -w <width> -h <height> -m <magnification> "
			"-u <update-period> -f <force-update-rate>\n");
	free(state);
	return NULL;
}

int main(int argc, char *argv[])
{
	GR_EVENT ev;
	nmstate *state;
	
	if(!(state = init(argc, argv))) return 1;

	while(1) {
		GrGetNextEvent(&ev);
		switch(ev.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				draw_output(state);
				break;
			case GR_EVENT_TYPE_TIMER:
				do_update(state);
				break;
			case GR_EVENT_TYPE_MOUSE_POSITION:
				mouse_moved(state, &ev.mouse);
				break;
			case GR_EVENT_TYPE_CLOSE_REQ:
				free(state->inbuf);
				free(state->outbuf);
				free(state);
				GrClose();
				return 0;
			default:
				fprintf(stderr, "Got unknown event type %d\n",
						ev.type);
				break;
		}
	}

	return 0;
}
