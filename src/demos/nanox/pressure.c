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
 * The Original Code is "pressure".
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

#define FIX_PRECISION 10
#include <fixed.h>

#if 1
/* These values are suitable for my TuxScreen. You will need to calibrate them
 * manually for your input device. */
#define DEFAULT_MIN_PRESSURE 190
#define DEFAULT_MAX_PRESSURE 215
#else
/* These values work with devices which don't support pressure information
 * such as mice (but only with fully on and fully off of course). */
#define DEFAULT_MIN_PRESSURE 0
#define DEFAULT_MAX_PRESSURE 100
#endif

#define MAX_BLOB_RAD 20
#define WINDOW_WIDTH 160
#define WINDOW_HEIGHT 160

GR_WINDOW_ID main_window, main_pixmap;
GR_GC_ID gc;
int min_z = DEFAULT_MIN_PRESSURE;
int max_z = DEFAULT_MAX_PRESSURE;
int max_d = MAX_BLOB_RAD;

int init(int argc, char *argv[])
{
	GR_PROP *prop;
	GR_COORD x = 0;
	GR_WM_PROPERTIES props;
	unsigned long minz, maxz;

	if(argc > 1) {
		if(argc != 3) {
			fprintf(stderr, "Usage: %s min_pressure max_pressure\n",
					argv[0]);
			return -1;
		}
		minz = atoi(argv[1]);
		maxz = atoi(argv[2]);
		if(minz == 0 || maxz == 0 || minz >= maxz) {
			fprintf(stderr, "Invalid calibration parameters\n");
			return -1;
		}
		min_z = minz;
		max_z = maxz;
	}

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return -1;
	}

	if(GrGetWindowProperty(GR_ROOT_WINDOW_ID, "WINDOW_MANAGER", &prop)) {
		free(prop);
		x = GR_OFF_SCREEN;
	}

	main_window = GrNewWindow(GR_ROOT_WINDOW_ID, x, 0, WINDOW_WIDTH,
				WINDOW_HEIGHT, 0, GR_COLOR_WHITE, 0);


	props.flags = GR_WM_FLAGS_TITLE | GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_BORDER | GR_WM_PROPS_CAPTION |
			GR_WM_PROPS_CLOSEBOX;
	props.title = "pressure";
	GrSetWMProperties(main_window, &props);

	GrSelectEvents(main_window, GR_EVENT_MASK_EXPOSURE |
				GR_EVENT_MASK_MOUSE_MOTION |
				/* GR_EVENT_MASK_MOUSE_POSITION | */
				GR_EVENT_MASK_BUTTON_DOWN |
				GR_EVENT_MASK_BUTTON_UP |
				GR_EVENT_MASK_CLOSE_REQ);

	main_pixmap = GrNewPixmap(WINDOW_WIDTH, WINDOW_HEIGHT, NULL);

	gc = GrNewGC();
	GrSetGCForeground(gc, GR_COLOR_WHITE);
	GrFillRect(main_pixmap, gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	GrSetGCForeground(gc, GR_COLOR_BLACK);

	GrMapWindow(main_window);

	return 0;
}

void handle_exposure_event(GR_EVENT_EXPOSURE *ev)
{
	GrCopyArea(main_window, gc, ev->x, ev->y, ev->width, ev->height,
			main_pixmap, ev->x, ev->y, 0);
}

void handle_mouse_event(GR_EVENT_MOUSE *ev)
{
	int z = ev->z, rad;

	/* If the pressure is below the minimum threshold, don't draw a dot */
	if(z < min_z) return;

	/* Clip the pressure to <= the calibrated maximum value. */
	z = (z > max_z) ? max_z : z;

	/* Scale the pressure to produce a value <= MAX_BLOB_RAD */
	rad = fixtoint(fixmult(fixdiv(inttofix(z - min_z),
		inttofix(max_z - min_z)), inttofix(MAX_BLOB_RAD)));

	GrFillEllipse(main_pixmap, gc, ev->x, ev->y, rad, rad);
	GrFillEllipse(main_window, gc, ev->x, ev->y, rad, rad);
}

int handle_event(GR_EVENT *ev)
{
	switch(ev->type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			return 1;
		case GR_EVENT_TYPE_EXPOSURE:
			handle_exposure_event((GR_EVENT_EXPOSURE *)ev);
			break;
		case GR_EVENT_TYPE_MOUSE_POSITION:
		case GR_EVENT_TYPE_MOUSE_MOTION:
			handle_mouse_event((GR_EVENT_MOUSE *)ev);
			break;
		case GR_EVENT_TYPE_BUTTON_DOWN:
		case GR_EVENT_TYPE_BUTTON_UP:
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

	if(init(argc, argv)) return -1;

	do {
		GrGetNextEvent(&ev);
	} while(!handle_event(&ev));

	GrClose();

	return 0;
}
