/*
 * Nano-X GrGrabKey() demo program.
 *
 * Copyright(C) 2003 Jon Foster <jon@jon-foster.co.uk>
 * Based on demo2.c
 *
 * This program allows the GrGrabKey() API to be tested.
 * It creates a window, and displays key and hotkey events it recieves
 * in that window.  At startup it registers a key with GrGrabKey().
 * The key and the registration type can both be specified on the
 * command line.
 *
 * Usage:
 *    grabdemo key {n|h|x|s}
 * Key is numerical MWKEY value (default 55='a').
 * The second parameter is a single letter indicating the reservation
 * type.  Valid values:
 *   n - Normal    (GR_GRAB_HOTKEY_EXCLUSIVE) (default)
 *   h - Hotkey    (GR_GRAB_HOTKEY)
 *   x - eXclusive (GR_GRAB_EXCLUSIVE)
 *   m - Mouse     (GR_GRAB_EXCLUSIVE_MOUSE)
 */

#define MWINCLUDECOLORS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"

#include <signal.h>

static GR_WINDOW_ID	main_window;
static GR_GC_ID		main_gc;

static char	message1[100] = "";
static char	message2[100] = "";
static char	message3[100] = "";
static char	message4[100] = "";
static char	message5[100] = "Try pressing keys...";

#define WIDTH	500
#define HEIGHT	110

/* A counter - used so you can see the display being updated. */
static int getSerialNumber(void)
{
	static int serial = 0;
	if (++serial >= 1000)
		serial = 0;
	return serial;
}

/* If a character is ASCII, return it, else return a question mark. */
static char keyToChar(int key)
{
	if ((key >= 32) && (key <= 127)) {
		return (char)key;
	} else {
		return '?';
	}
}

/* Makes a GrGrabKey() type paramater into a human-readable string */
static const char *grabKindName(int kind)
{
	switch (kind) {
		case GR_GRAB_HOTKEY_EXCLUSIVE:
			return "GR_GRAB_HOTKEY_EXCLUSIVE";
		case GR_GRAB_HOTKEY:
			return "GR_GRAB_HOTKEY";
		case GR_GRAB_EXCLUSIVE:
			return "GR_GRAB_EXCLUSIVE";
		case GR_GRAB_EXCLUSIVE_MOUSE:
			return "GR_GRAB_EXCLUSIVE_MOUSE";
		default:
			return "Invalid GR_GRAB_ mode";
	}
}

/* Repaint a specified area of the window */
static void repaint(int x, int y, int width, int height)
{
	GrSetGCForeground(main_gc, GrGetSysColor(GR_COLOR_APPWINDOW));
	GrFillRect(main_window, main_gc, x, y, width, height);
	GrSetGCForeground(main_gc, GrGetSysColor(GR_COLOR_APPTEXT));
	GrSetGCUseBackground(main_gc, GR_FALSE);
	GrText(main_window, main_gc, 10,  20, message1, -1, GR_TFASCII);
	GrText(main_window, main_gc, 10,  40, message2, -1, GR_TFASCII);
	GrText(main_window, main_gc, 10,  60, message3, -1, GR_TFASCII);
	GrText(main_window, main_gc, 10,  80, message4, -1, GR_TFASCII);
	GrText(main_window, main_gc, 10, 100, message5, -1, GR_TFASCII);
	GrRect(main_window, main_gc, 5, 5, WIDTH - 10, HEIGHT - 10);
}

/* Repaint the entire window */
static void repaintAll(void)
{
	repaint(0,0,WIDTH,HEIGHT);
}

int
main(int argc,char **argv)
{
	GR_EVENT 	event;
	GR_WM_PROPERTIES props;
	int grabResult;
	int key = 'a';
	int grabKind = GR_GRAB_HOTKEY_EXCLUSIVE;

	if (argc >= 2)
		key = atoi(argv[1]);
	
	if (argc >= 3) {
		switch (argv[2][0]) {
			case 'n':
				grabKind = GR_GRAB_HOTKEY_EXCLUSIVE;
				break;
			case 'h':
				grabKind = GR_GRAB_HOTKEY;
				break;
			case 'x':
				grabKind = GR_GRAB_EXCLUSIVE;
				break;
			case 'm':
				grabKind = GR_GRAB_EXCLUSIVE_MOUSE;
				break;
			default:
				printf( "Usage: %s key {n|h|x|s}\n"
					"Key is numeric MWKEY value (default 55='a'), type is:\n"
					" n - Normal    (GR_GRAB_HOTKEY_EXCLUSIVE) (default)\n"
					" h - Hotkey    (GR_GRAB_HOTKEY)\n"
					" x - eXclusive (GR_GRAB_EXCLUSIVE)\n"
					" m - Mouse     (GR_GRAB_EXCLUSIVE_MOUSE)\n",
					argv[0]);
				break;
		}
	}
	
	if (GrOpen() < 0) {
		printf("Can't open graphics\n");
		exit(1);
	}

	/* pass errors through main loop*/
	GrSetErrorHandler(NULL);

	main_window = GrNewWindow(GR_ROOT_WINDOW_ID, 20, 20, WIDTH, HEIGHT,
		0, GREEN, BLACK);

	props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOBACKGROUND;
	props.title = "Nano-X GrabKey Demo";
	GrSetWMProperties(main_window, &props);

	main_gc = GrNewGC();

	printf("Exclusively reserving key %d ('%c'): ", key, keyToChar(key));
	grabResult = GrGrabKey(main_window, key, grabKind);
	printf("%d\n", grabResult);
	
	GrSelectEvents(main_window, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ
			| GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP);
	GrMapWindow(main_window);
	GrSetFocus(main_window);

	sprintf(message1,"Exclusively reserving key %d ('%c'): %s (Mode: %s)\n",
		key, keyToChar(key),
		(grabResult ? "success" : "failure"),
		grabKindName(grabKind));

	for (;;) {
		GrGetNextEvent(&event);
		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			repaint(event.exposure.x, event.exposure.y,
				event.exposure.width, event.exposure.height);
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
			break;
		case GR_EVENT_TYPE_ERROR:
			printf("\7grabdemo: Error (%s) ", event.error.name);
			printf(nxErrorStrings[event.error.code],event.error.id);
			break;
		case GR_EVENT_TYPE_KEY_DOWN:
			if (event.keystroke.hotkey)
				sprintf(message4, "%03d Hotkey %d ('%c') scan %d pressed\n",
					getSerialNumber(), event.keystroke.ch,
					keyToChar(event.keystroke.ch), event.keystroke.scancode);
			else
				sprintf(message2, "%03d Key %d ('%c') scan %d pressed\n",
					getSerialNumber(), event.keystroke.ch,
					keyToChar(event.keystroke.ch), event.keystroke.scancode);
			repaintAll();
			break;
		case GR_EVENT_TYPE_KEY_UP:
			if (event.keystroke.hotkey)
				sprintf(message5, "%03d Hotkey %d ('%c') released\n",
					getSerialNumber(), event.keystroke.ch, keyToChar(event.keystroke.ch));
			else
				sprintf(message3, "%03d Key %d ('%c') released\n",
					getSerialNumber(), event.keystroke.ch, keyToChar(event.keystroke.ch));
			repaintAll();
			break;
		}
	}

	GrClose();
	return 0;
}
