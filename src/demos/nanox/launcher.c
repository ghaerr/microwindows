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

/*
 * A simple application launcher for Nano-X by Alex Holden.
 *
 * The application needs to be started with the first argument specifying
 * the location of it's configuration file. The format of the file is
 * extremely simple- each line can contain either a comment (indicated by
 * beginning the line with a '#' symbol) or an item description.
 * An item description consists of the name of the item (the title which
 * appears underneath the icon on the launcher button) followed by the name
 * of the icon file (or '-' for no icon) and the command to execute when the
 * item is clicked on. The command can optionally be followed by a limited
 * number of arguments to pass to the program when it is executed (increase
 * MAX_ARGUMENTS in launcher.h if you need more). The program will currently
 * only allow one icon size (specified at compile time by the ICON_WIDTH and
 * ICON_HEIGHT parameters). The program only loads each icon file once even if
 * it is used multiple times, so you can save a small amount of memory by
 * using the same icon for several programs. If you want to change the size
 * of the item buttons, change ITEM_WIDTH and ITEM_HEIGHT in launcher.h.
 * The way the launcher decides whether to draw a vertical panel on the left
 * hand side of the screen or a horizontal panel along the bottom is by
 * looking at the width and height of the screen- the panel will be placed on
 * the side on portrait screens and on the bottom on landscape screens.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "launcher.h"

void reaper(int signum) { while(waitpid(WAIT_ANY, NULL, WNOHANG) > 0); }

void *my_malloc(size_t size)
{
	void *ret;

	if(!(ret = malloc(size))) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	return ret;
}

void usage(void)
{
	fprintf(stderr, "Usage: launcher <config-file>\n");
	exit(3);
}

prog_item *make_prog_item(char *buf, int lineno)
{
	char *p, *pp, *command;
	prog_item *prog;
	int n;

	p = buf;

	prog = my_malloc(sizeof(prog_item));

	for(n = 0; n < MAX_ARGUMENTS; n++) prog->argv[n] = NULL;

	while(isspace(*p)) p++;
	if(!*p) {
		fprintf(stderr, "Premature end of line on line %d of config "
				"file\n", lineno);
		return 0;
	}
	command = p;
	while(*p && (!isspace(*p))) p++;
	*p++ = 0;
	if(!(prog->command = strdup(command))) {
		free(prog);
		goto nomem;
	}
	pp = p - 1;
	while(--pp != command) {
		if(*pp == '/') {
			pp++;
			break;
		}
	}
	if(!(prog->argv[0] = strdup(pp))) {
		free(prog->command);
		free(prog);
		goto nomem;
	}

	n = 1;
	while(*p) {
		while(isspace(*p)) p++;
		if(!*p) break;
		pp = p;
		while(*p && (!isspace(*p))) p++;
		*p++ = 0;
		if(!(prog->argv[n] = strdup(pp))) {
			for(n = MAX_ARGUMENTS; n; n--)
				if(prog->argv[n]) free(prog->argv[n]);
			free(prog->command);
			free(prog);
			goto nomem;
		}
		if(++n == (MAX_ARGUMENTS - 1)) {
			fprintf(stderr, "Too many arguments on line "
				"%d of the config file\n", lineno);
			break; 
		}
	}

	return prog;

nomem:
	fprintf(stderr, "Out of memory parsing line %d of the config "
			"file\n", lineno);
	return 0;
}

void set_window_background_colour(char *buf, int lineno)
{
	GR_WM_PROPERTIES props;
	char *p = buf, *pp;

	while(isspace(*p)) p++;
	if(!*p) {
		fprintf(stderr, "Premature end of line on line %d of config "
				"file\n", lineno);
		return;
	}
	pp = p;
	while(*p && (!isspace(*p))) p++;
	*p = 0;

	if(!strcmp(pp, "BLACK")) props.background = BLACK;
	else if(!strcmp(pp, "BLUE")) props.background = BLUE;
	else if(!strcmp(pp, "GREEN")) props.background = GREEN;
	else if(!strcmp(pp, "CYAN")) props.background = CYAN;
	else if(!strcmp(pp, "RED")) props.background = RED;
	else if(!strcmp(pp, "MAGENTA")) props.background = MAGENTA;
	else if(!strcmp(pp, "BROWN")) props.background = BROWN;
	else if(!strcmp(pp, "LTGRAY")) props.background = LTGRAY;
	else if(!strcmp(pp, "GRAY")) props.background = GRAY;
	else if(!strcmp(pp, "LTBLUE")) props.background = LTBLUE;
	else if(!strcmp(pp, "LTGREEN")) props.background = LTGREEN;
	else if(!strcmp(pp, "LTCYAN")) props.background = LTCYAN;
	else if(!strcmp(pp, "LTRED")) props.background = LTRED;
	else if(!strcmp(pp, "LTMAGENTA")) props.background = LTMAGENTA;
	else if(!strcmp(pp, "YELLOW")) props.background = YELLOW;
	else if(!strcmp(pp, "WHITE")) props.background = WHITE;
	else {
		fprintf(stderr, "Invalid colour \"%s\" on line %d of config "
							"file\n", pp, lineno);
		return;
	}

	props.flags = GR_WM_FLAGS_BACKGROUND;
	GrSetWMProperties(GR_ROOT_WINDOW_ID, &props);
}

void parse_config_line(lstate *state, char *buf, int lineno)
{
	char *p, *pp, *name, *icon;
	int n;
	litem *new_litem, *li;
	sitem *new_sitem;
	GR_IMAGE_INFO imageinfo;

	p = buf;

	if((!*p) || (*p == '#') || (*p == '\n')) return;

	while(isspace(*p)) p++;
	name = p;
	while(*p && (!isspace(*p))) p++;
	if(!*p) goto premature;
	*p++ = 0;

	if(!strcmp(name, "$screensaver")) {
		new_sitem = my_malloc(sizeof(sitem));
		if(!(new_sitem->prog = make_prog_item(p, lineno))) {
			free(new_sitem);
			return;
		}
		new_sitem->next = NULL;
		if(!state->sitems) {
			state->sitems = new_sitem;
			state->cursitem = new_sitem;
		} else {
			new_sitem->next = state->sitems;
			state->sitems = new_sitem;
		}
		return;
	} else if(!strcmp(name, "$screensaver_timeout")) {
		n = strtol(p, NULL, 10);
		GrSetScreenSaverTimeout(n);
		return;
	} else if(!strcmp(name, "$window_background_image")) {
		while(isspace(*p)) p++;
		if(!*p) goto premature;
		pp = p;
		while(*p && (!isspace(*p))) p++;
		*p = 0;
		state->window_background_image = strdup(pp);
		return;
	} else if(!strcmp(name, "$window_background_mode")) {
		state->window_background_mode = (int) strtol(p, NULL, 10);
		return;
	} else if(!strcmp(name, "$window_background_colour")) {
		set_window_background_colour(p, lineno);
		return;
	}

	while(isspace(*p)) p++;
	if(!*p) goto premature;
	icon = p;
	while(*p && (!isspace(*p))) p++;
	if(!*p) goto premature;
	*p++ = 0;

	new_litem = my_malloc(sizeof(litem));
	if(!(new_litem->name = strdup(name))) {
		free(new_litem);
		goto nomem;
	}
	if(!(new_litem->icon = strdup(icon))) {
		free(new_litem->name);
		free(new_litem);
		goto nomem;
	}
	if(!(new_litem->prog = make_prog_item(p, lineno))) {
		free(new_litem->name);
		free(new_litem->icon);
		free(new_litem);
		return;
	}
	new_litem->iconid = 0;
	if(strcmp("-", icon)) {
		li = state->litems;
		while(li) {
			if(!(strcmp(icon, li->name))) {
				new_litem->iconid = li->iconid;
				break;
			}
			li = li->next;
		}
		if(!new_litem->iconid) {
			if(!(new_litem->iconid = GrLoadImageFromFile(icon, 0))){
				fprintf(stderr, "Couldn't load icon \"%s\"\n",
									icon);
			} else {
				GrGetImageInfo(new_litem->iconid, &imageinfo);
				if((imageinfo.width != ICON_WIDTH) ||
					(imageinfo.height != ICON_HEIGHT)) {
					fprintf(stderr, "Icon \"%s\" is the "
					"wrong size (%dx%d instead of %dx%d)"
					"\n", icon, imageinfo.width,
					imageinfo.height, ICON_WIDTH,
					ICON_HEIGHT);
					GrFreeImage(new_litem->iconid);
					new_litem->iconid = 0;
				}
			}
		}
	}

	new_litem->prev = NULL;
	new_litem->next = NULL;
	if(!state->litems) {
		state->lastlitem = new_litem;
		state->litems = new_litem;
	} else {
		new_litem->next = state->litems;
		state->litems->prev = new_litem;
		state->litems = new_litem;
	}

	state->numlitems++;

	return;

nomem:
	fprintf(stderr, "Out of memory\n");
	exit(1);

premature:
	fprintf(stderr, "Premature end of line on line %d of config file\n",
								lineno);
}

void read_config(lstate *state)
{
	int lineno = 1;
	FILE *fp;
	char *buf = my_malloc(256);

	if(!(fp = fopen(state->config_file, "r"))) {
		fprintf(stderr, "Couldn't open config file \"%s\"\n",
							state->config_file);
		exit(2);
	}

	state->litems = NULL;
	state->numlitems = 0;
	state->sitems = NULL;

	while(fgets(buf, 256, fp)) {
		parse_config_line(state, buf, lineno);
		lineno++;
	}

	fclose(fp);
	free(buf);

	if(!state->numlitems) {
		fprintf(stderr, "No valid launcher items in config file\n");
		exit(5);
	}
}

void draw_item(lstate *state, litem *item)
{
	GR_SIZE width, height, base, x, len;

	GrDrawImageToFit(item->wid, state->gc, ICON_X_POSITION, ICON_Y_POSITION,
				ICON_WIDTH, ICON_HEIGHT, item->iconid);

	len = strlen(item->name);
	GrGetGCTextSize(state->gc, item->name, len, 0, &width, &height, &base);
	if(width >= ITEM_WIDTH) x = 0;
	else x = (ITEM_WIDTH - width) / 2;

	GrText(item->wid, state->gc, x, TEXT_Y_POSITION, item->name, len, 0);
}

void handle_exposure_event(lstate *state)
{
	GR_EVENT_EXPOSURE *event = &state->event.exposure;
	litem *i = state->litems;

	if(event->wid == state->main_window) return;

	while(i) {
		if(event->wid == i->wid) {
			draw_item(state, i);
			return;
		}
		i = i->next;
	}

	fprintf(stderr, "Got exposure event for unknown window %d\n",
							event->wid);
}

void launch_program(prog_item *prog)
{
	pid_t pid;

	if((pid = fork()) == -1) perror("Couldn't fork");
	else if(!pid) {
		if(execvp(prog->command, prog->argv) == -1)
			fprintf(stderr, "Couldn't start \"%s\": %s\n",
					prog->command, strerror(errno));
		exit(7);
	}
}

void handle_mouse_event(lstate *state)
{
	GR_EVENT_MOUSE *event = &state->event.mouse;
	litem *i = state->litems;

	if(event->wid == state->main_window) return;

	while(i) {
		if(event->wid == i->wid) {
			launch_program(i->prog);
			return;
		}
		i = i->next;
	}

	fprintf(stderr, "Got mouse event for unknown window %d\n", event->wid);
}

void handle_screensaver_event(lstate *state)
{
	GR_EVENT_SCREENSAVER *event = &state->event.screensaver;

	if(event->activate != GR_TRUE) return;

	if(!state->sitems) {
		fprintf(stderr, "Got screensaver activate event with no "
				"screensavers defined\n");
		return;
	}

	state->cursitem = state->cursitem->next;
	if(!state->cursitem) state->cursitem = state->sitems;

	launch_program(state->cursitem->prog);
}

void handle_event(lstate *state)
{
	switch(state->event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			handle_exposure_event(state);
			break;
		case GR_EVENT_TYPE_BUTTON_DOWN:
			handle_mouse_event(state);
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			break;
		case GR_EVENT_TYPE_SCREENSAVER:
			handle_screensaver_event(state);
			break;
		case GR_EVENT_TYPE_NONE:
			break;
		default:
			fprintf(stderr, "Got unknown event type %d\n",
							state->event.type);
			break;
	}
}

void do_event_loop(lstate *state)
{
	do {
		GrGetNextEvent(&state->event);
		handle_event(state);
	} while(state->event.type != GR_EVENT_TYPE_CLOSE_REQ);
}

void initialise(lstate *state)
{
	GR_SCREEN_INFO si;
	GR_IMAGE_ID back_image;
	GR_IMAGE_INFO imageinfo;
	int rows = 1, columns = 1, width, height, x = 0, y = 1;
	GR_WM_PROPERTIES props;
	litem *i;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		exit(4);
	}

	state->window_background_mode = 0;
	state->window_background_image = NULL;

	read_config(state);

	GrGetScreenInfo(&si);

	if(si.rows > si.cols) {
		rows = state->numlitems;
		while((((rows / columns) + rows % columns) * ITEM_HEIGHT) >
								si.rows) {
			columns++;
		}
		if((columns * ITEM_WIDTH) > si.cols) goto toomany;
		rows = (rows / columns) + (rows % columns);
		width = columns * ITEM_WIDTH + 1 + columns;
		height = rows * ITEM_HEIGHT + 1 + rows;
	} else {
		columns = state->numlitems;
		while((((columns / rows) + (columns % rows)) * ITEM_WIDTH) >
								si.cols) {
			rows++;
		}
		if((rows * ITEM_HEIGHT) > si.rows) goto toomany;
		columns = (columns / rows) + (columns % rows);
		width = columns * ITEM_WIDTH + 1 + columns;
		height = (rows * ITEM_HEIGHT) + 1 + rows;
		y = si.rows - (rows * ITEM_HEIGHT) - 1 - rows;
	}

	state->gc = GrNewGC();
	GrSetGCForeground(state->gc, ITEM_TEXT_COLOUR);
	GrSetGCBackground(state->gc, ITEM_BACKGROUND_COLOUR);

	if(state->window_background_image) {
		if(!(back_image = GrLoadImageFromFile(
					state->window_background_image, 0))) {
			fprintf(stderr, "Couldn't load background image\n");
		} else {
			GrGetImageInfo(back_image, &imageinfo);
			if(!(state->background_pixmap = GrNewPixmap(
							imageinfo.width,
						imageinfo.height, NULL))) {
				fprintf(stderr, "Couldn't allocate pixmap "	
						"for background image\n");
			} else {
				GrDrawImageToFit(state->background_pixmap,
					state->gc, 0, 0, imageinfo.width,
					imageinfo.height, back_image);
				GrFreeImage(back_image);
				GrSetBackgroundPixmap(GR_ROOT_WINDOW_ID,
					state->background_pixmap,
					state->window_background_mode);
				GrClearWindow(GR_ROOT_WINDOW_ID, GR_TRUE);
			}
		}
	}

	if(state->sitems)
		GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_SCREENSAVER);

	state->main_window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, y, width, height,
						0, ITEM_BACKGROUND_COLOUR, 0);
	GrSelectEvents(state->main_window, GR_EVENT_MASK_CLOSE_REQ);
	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NOMOVE | GR_WM_PROPS_NODECORATE |
			GR_WM_PROPS_NOAUTOMOVE | GR_WM_PROPS_NOAUTORESIZE;
	GrSetWMProperties(state->main_window, &props);

	i = state->lastlitem;
	y = 0;
	while(i) {
		i->wid = GrNewWindow(state->main_window,
					(x * ITEM_WIDTH) + x + 1,
					(y * ITEM_HEIGHT) + y + 1, ITEM_WIDTH,
					ITEM_HEIGHT, 1, ITEM_BACKGROUND_COLOUR,
							ITEM_BORDER_COLOUR);
		GrSelectEvents(i->wid, GR_EVENT_MASK_EXPOSURE |
					GR_EVENT_MASK_BUTTON_DOWN);
		GrMapWindow(i->wid);
		i = i->prev;
		if(++x == columns) {
			x = 0;
			y++;
		}
	}

	GrMapWindow(state->main_window);

	signal(SIGCHLD, &reaper);

	return;

toomany:
	fprintf(stderr, "Too many items to fit on screen\n");
	exit(6);
}

int main(int argc, char *argv[])
{
	lstate *state;

	if(argc != 2) usage();

	state = my_malloc(sizeof(lstate));
	state->config_file = strdup(argv[1]);

	initialise(state);

	do_event_loop(state);

	GrClose();

	return 0;
}
