/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
/* Uncomment this to get debugging output from this file */
/*#define DEBUG*/

#include "nanowm.h"

static win *windows = NULL;

/*
 * Find the windowlist entry for the specified window ID and return a pointer
 * to it, or NULL if it isn't in the list.
 */
win *find_window(GR_WINDOW_ID wid)
{
	win *w = windows;

	Dprintf("Looking for window %d... ", wid);

	while(w) {
		Dprintf("%d ", w->wid);
		if(w->wid == wid) {
			Dprintf("found it!\n");
			return w;
		}
		w = w->next;
	}

	Dprintf("Nope, %d is not in the list\n", wid);
	return NULL;
}

/*
 * Add a new entry to the front of the windowlist.
 * Returns -1 on failure or 0 on success.
 */
int add_window(win *window)
{
	win *w;

	Dprintf("Adding window %d\n", window->wid);

	if(!(w = malloc(sizeof(win)))) return -1;

	w->wid = window->wid;
	w->pid = window->pid;
	w->type = window->type;
	w->sizing = GR_FALSE;	/* window->sizing*/
	w->active = window->active;
	w->clientid = window->clientid;
	w->data = window->data;
	w->next = windows;
	windows = w;

	return 0;
}

/*
 * Remove an entry from the windowlist.
 * We must search through the list for it so that we can find the previous
 * entry in the list and fix the next pointer. The alternative is to add a
 * prev pointer to the structure which would increase the memory usage.
 * Returns -1 on failure or 0 on success.
 */
int remove_window(win *window)
{
	win *w = windows;
	win *prev = NULL;

	while(w) {
		if(w == window) {
			if(!prev) windows = w->next;
			else prev->next = w->next;
			if(w->data) free(w->data);
			free(w);
			return 0;
		}
		prev = w;
		w = w->next;
	}

	return -1;
}

/*
 * Remove an entry and all it's children from the windowlist.
 * Returns -1 on failure or 0 on success.
 */
int remove_window_and_children(win *window)
{
	win *t, *w = windows;
	win *prev = NULL;
	GR_WINDOW_ID pid = window->wid;

	Dprintf("Removing window %d and children\n", window->wid);

	while(w) {
		Dprintf("Examining window %d (pid %d)\n", w->wid, w->pid);
		if((w->pid == pid) || (w == window)) {
			Dprintf("Removing window %d (pid %d)\n", w->wid,
								w->pid);
			if(prev) prev->next = w->next;
			else windows = w->next;
			t = w->next;
			if(w->data) free(w->data);
			free(w);
			w = t;
			continue;
		}
		prev = w;
		w = w->next;
	}

	return -1;
}
