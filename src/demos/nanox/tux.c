/*
 * tux - demonstrate shaped window frames for Nano-X
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
 * The Original Code is Nano-X Tux.
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
#include "tuxmask.h"

#define DEFAULT_TUX_FILE "bin/tux.gif"

static int button_down = 0, x = 0, y = 0;

GR_WINDOW_ID init(char *tuxfile)
{
	GR_GC_ID gc;
	GR_IMAGE_ID iid;
	GR_WINDOW_ID wid;
	GR_REGION_ID rid;
	GR_WINDOW_ID pid;
	GR_IMAGE_INFO iif;
	GR_WM_PROPERTIES props;
	GR_SCREEN_INFO sinfo;
	int x, y;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 0;
	}
	GrGetScreenInfo(&sinfo);

	if(!(iid = GrLoadImageFromFile(tuxfile, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", tuxfile);
		return 0;
	}
	GrGetImageInfo(iid, &iif);
	pid = GrNewPixmap(iif.width, iif.height, NULL);
	gc = GrNewGC();
	GrDrawImageToFit(pid, gc, 0, 0, iif.width, iif.height, iid);
	GrDestroyGC(gc);
	GrFreeImage(iid);

	x = sinfo.cols - iif.width;
	y = sinfo.rows - iif.height;
	wid = GrNewWindowEx(GR_WM_PROPS_APPWINDOW|GR_WM_PROPS_NOAUTOMOVE, NULL,
		GR_ROOT_WINDOW_ID, x, y, iif.width, iif.height, GR_COLOR_GREEN);

	GrSetBackgroundPixmap(wid, pid, GR_BACKGROUND_TOPLEFT);

	rid = GrNewBitmapRegion(tuxmask_bits, TUXMASK_WIDTH, TUXMASK_HEIGHT);
	GrSetWindowRegion(wid, rid, GR_WINDOW_BOUNDING_MASK);
	GrDestroyRegion(rid);

	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NODECORATE;
	GrSetWMProperties(wid, &props);

	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ |
			GR_EVENT_MASK_MOUSE_POSITION |
			GR_EVENT_MASK_BUTTON_UP |
			GR_EVENT_MASK_BUTTON_DOWN);

	GrMapWindow(wid);

	return wid;
}

void button_event(GR_EVENT_BUTTON *ev)
{
	if(ev->type == GR_EVENT_TYPE_BUTTON_DOWN) {
		if(ev->buttons & GR_BUTTON_R) {
			GrClose();
			exit(0);
		}
		button_down = 1;
		x = ev->x;
		y = ev->y;
		GrRaiseWindow(ev->wid);
	} else button_down = 0;
}

void position_event(GR_EVENT_MOUSE *ev)
{
	static int newx = 0, newy = 0, oldx = 0, oldy = 0;

	if(!button_down) return;

	newx = ev->rootx - x;
	newy = ev->rooty - y;

	if(newx != oldx || newy != oldy) {
		GrMoveWindow(ev->wid, newx, newy);
		oldx = newx;
		oldy = newy;
	}
}

int main(int argc, char *argv[])
{
	char *tuxfile;
	GR_EVENT event;
	GR_WINDOW_ID wid;

	if(argc >= 2) tuxfile = argv[1];
	else tuxfile = DEFAULT_TUX_FILE;

	if(!(wid = init(tuxfile))) return 1;

	while(1) {
		GrGetNextEvent(&event);
		switch(event.type) {
			case GR_EVENT_TYPE_CLOSE_REQ:
				return 0;
				break;
			case GR_EVENT_TYPE_MOUSE_POSITION:
				position_event(&event.mouse);
				break;
			case GR_EVENT_TYPE_BUTTON_UP:
			case GR_EVENT_TYPE_BUTTON_DOWN:
				button_event(&event.button);
				break;
			default:
				break;
		}
	}

	return 0;
}
