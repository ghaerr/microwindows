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
 * The Original Code is "periodic".
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

#include <nano-X.h>
#include <nxcolors.h>

#define WINDOW_WIDTH 160
#define WINDOW_HEIGHT 160
#define TIMER_PERIOD 250

GR_WINDOW_ID main_window;
GR_GC_ID gc;
int current_colour = GR_COLOR_BLACK;
GR_TIMER_ID timer_id = 0;

void redraw(void)
{
	GrFillRect(main_window, gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void timeout(void)
{
	current_colour = (current_colour == GR_COLOR_BLACK) ? GR_COLOR_WHITE :
								GR_COLOR_BLACK;
	GrSetGCForeground(gc, current_colour);
	redraw();
}

void button_down(void)
{
	if(timer_id) {
		GrDestroyTimer(timer_id);
		timer_id = 0;
	} else timer_id = GrCreateTimer(main_window, TIMER_PERIOD, GR_TRUE);
}

int init(void)
{
	GR_WM_PROPERTIES props;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return -1;
	}

	main_window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, WINDOW_WIDTH,
				WINDOW_HEIGHT, 0, GR_COLOR_WHITE, 0);

	props.flags = GR_WM_FLAGS_TITLE | GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_BORDER | GR_WM_PROPS_CAPTION |
			GR_WM_PROPS_CLOSEBOX;
	props.title = "periodic";
	GrSetWMProperties(main_window, &props);

	GrSelectEvents(main_window, GR_EVENT_MASK_EXPOSURE |
				GR_EVENT_MASK_BUTTON_DOWN |
				GR_EVENT_MASK_CLOSE_REQ |
				GR_EVENT_MASK_TIMER);

	gc = GrNewGC();
	GrSetGCForeground(gc, current_colour);

	GrMapWindow(main_window);

	timer_id = GrCreateTimer(main_window, TIMER_PERIOD, GR_TRUE);

	return 0;
}

int handle_event(GR_EVENT *ev)
{
	switch(ev->type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			return 1;
		case GR_EVENT_TYPE_EXPOSURE:
			redraw();
			break;
		case GR_EVENT_TYPE_BUTTON_DOWN:
			button_down();
			break;
		case GR_EVENT_TYPE_TIMER:
			timeout();
			break;
		default:
			fprintf(stderr, "Got unknown event %d\n", ev->type);
			break;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	GR_EVENT ev;

	if(init()) return -1;

	do {
		GrGetNextEvent(&ev);
	} while(!handle_event(&ev));

	GrClose();

	return 0;
}
