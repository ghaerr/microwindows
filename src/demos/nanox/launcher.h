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
 * The Original Code is NanoLauncher.
 *
 * The Initial Developer of the Original Code is Alex Holden.
 * Portions created by Alex Holden are Copyright (C) 2000
 * Alex Holden <alex@linuxhacker.org>. All Rights Reserved.
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

#ifndef LAUNCHER_H
#define LAUNCHER_H

#define ITEM_WIDTH 100
#define ITEM_HEIGHT 60
#define ITEM_TEXT_COLOUR BLACK
#define ITEM_BORDER_COLOUR BLACK
#define ITEM_BACKGROUND_COLOUR LTGRAY
#define ICON_WIDTH 32
#define ICON_HEIGHT 32
#define ICON_X_POSITION ((ITEM_WIDTH - ICON_WIDTH) / 2)
#define ICON_Y_POSITION 6
#define TEXT_Y_POSITION (ITEM_HEIGHT - 6)
#define MAX_ARGUMENTS 12

struct command_argv {
	char *command;
	char *argv[MAX_ARGUMENTS];
};
typedef struct command_argv prog_item;

struct launcher_item {
	char *name;
	char *icon;
	prog_item *prog;
	struct launcher_item *next;
	struct launcher_item *prev;
	GR_IMAGE_ID iconid;
	GR_WINDOW_ID wid;
};
typedef struct launcher_item litem;

struct screensaver_item {
	prog_item *prog;
	struct screensaver_item *next;
};
typedef struct screensaver_item sitem;

struct launcher_state {
	char *config_file;
	GR_WINDOW_ID main_window;
	litem *litems;
	litem *lastlitem;
	int numlitems;
	sitem *sitems;
	sitem *cursitem;
	GR_GC_ID gc;
	GR_EVENT event;
	int window_background_mode;
	char *window_background_image;
	GR_WINDOW_ID background_pixmap;
};
typedef struct launcher_state lstate;

void reaper(int signum);
void *my_malloc(size_t size);
void usage(void);
prog_item *make_prog_item(char *command, int lineno);
void set_window_background_colour(char *buf, int lineno);
void parse_config_line(lstate *state, char *buf, int lineno);
void read_config(lstate *state);
void draw_item(lstate *state, litem *item);
void handle_exposure_event(lstate *state);
void launch_program(prog_item *prog);
void handle_mouse_event(lstate *state);
void handle_screensaver_event(lstate *state);
void handle_event(lstate *state);
void do_event_loop(lstate *state);
void initialise(lstate *state);

#endif
