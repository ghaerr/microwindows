#ifndef NXCAL_H
#define NXCAL_H
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

/* Some configuration parameters you may need to change for your hardware. */
#define CALIBRATION_FILE "/etc/nanocal"
#define ZTHRESH_REDUCE(Z) (Z * 3 / 4)
#define CHECKCAL_TIMEOUT 20
#define BACK_BUTTON "BACK"
#define OK_BUTTON "OK"
#define YES_BUTTON "YES"
#define NO_BUTTON "NO"
#define TARGET_DIST 15
#define CROSS_SIZE 20
#define BOXOVERSIZE 15
#define BUTTONOVERSIZE 5
#define BUTTONSPACING 10
#define DEBOUNCE_TIME 300
#define FOREGROUND_COLOUR GR_COLOR_BLACK
#define BACKGROUND_COLOUR MWRGB(170,205,255)
#define TITLE_FONT GR_FONT_SYSTEM_VAR
#define BODY_FONT GR_FONT_GUI_VAR

enum {
	STATE_TARGET1,
	STATE_TARGET2,
	STATE_TARGET3,
	STATE_TARGET4,
	STATE_CHECKCAL,
	STATE_ASKPOINTER,
	STATE_FINISHED
};

static const char *screen1_text[] = {
	"Calibrating Touch Screen",
	"Please press the stylus on each",
	"of the targets in turn.",
	"Use a light but firm pressure.",
	NULL
};

static const char *screen2_text[] = {
	"Check Calibration",
	"Try moving the pen around the",
	"screen. Check that the target",
	"is directly under the pen and",
	"that the pointer changes when",
	"a reasonable pressure is used.",
	"If you are satisfied, then",
	"click on OK to continue.",
	"To try again, click on BACK.",
	"If you do not click on either",
	"button within 20 seconds you",
	"will be taken back to the",
	"calibration stage automatically.",
	NULL
};

static const char *screen3_text[] = {
	"Mouse Pointer",
	"Do you want to be able to",
	"see the mouse cursor",
	"under the pen or not?",
	NULL
};

typedef struct {
	int xmin;
	int xmax;
	int ymin;
	int ymax;
	int xswap;
	int yswap;
	int zthresh;
	int showcursor;
} cal_parms;

typedef struct {
	int pressed;
	int minx;
	int maxx;
	int miny;
	int maxy;
} cal_button;

typedef struct {
	GR_GC_ID tgc;
	GR_GC_ID bgc;
	GR_TIMER_ID tid;
	GR_WINDOW_ID wid;
	GR_SCREEN_INFO si;
	GR_CURSOR_ID upcurs;
	GR_CURSOR_ID downcurs;
	int state;
	int maxz[4];
	int debounce;
	int xpoints[4];
	int ypoints[4];
	int button_down;
	cal_parms *parms;
	cal_button buttons[2];
} cal_state;

#define CURSOR(a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, \
		a1, a0) (GR_BITMAP)(a0 | (a1 << 1) | (a2 << 2) | (a3 << 3) \
		| (a4 << 4) | (a5 << 5) | (a6 << 6) | (a7 << 7) | (a8 << 8) \
		| (a9 << 9) | (a10 << 10) | (a11 << 11) | (a12 << 12) \
		| (a13 << 13) | (a14 << 14) | (a15 << 15))

#define UPCURSORWIDTH 15
#define UPCURSORHEIGHT 15
#define UPCURSORXHOT 7
#define UPCURSORYHOT 7
#define UPCURSORBITS	{ \
				CURSOR(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0), \
				CURSOR(0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0), \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0), \
				CURSOR(0,0,0,1,1,0,0,1,0,0,1,1,0,0,0,0), \
				CURSOR(0,0,1,1,0,0,0,1,0,0,0,1,1,0,0,0), \
				CURSOR(0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0), \
				CURSOR(0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0), \
				CURSOR(0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0), \
				CURSOR(0,0,1,1,0,0,0,1,0,0,0,1,1,0,0,0), \
				CURSOR(0,0,0,1,1,0,0,1,0,0,1,1,0,0,0,0), \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0), \
				CURSOR(0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0), \
				CURSOR(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)  \
			}
#define UPCURSORMASK	{ \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0), \
				CURSOR(0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(1,1,1,1,1,0,0,1,0,0,1,1,1,1,1,0), \
				CURSOR(1,1,1,1,0,0,0,1,0,0,0,1,1,1,1,0), \
				CURSOR(1,1,1,1,0,0,0,1,0,0,0,1,1,1,1,0), \
				CURSOR(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0), \
				CURSOR(1,1,1,1,0,0,0,1,0,0,0,1,1,1,1,0), \
				CURSOR(1,1,1,1,0,0,0,1,0,0,0,1,1,1,1,0), \
				CURSOR(1,1,1,1,1,0,0,1,0,0,1,1,1,1,1,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0), \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0)  \
			}

#define DOWNCURSORWIDTH 15
#define DOWNCURSORHEIGHT 15
#define DOWNCURSORXHOT 7
#define DOWNCURSORYHOT 7
#define DOWNCURSORBITS	{ \
				CURSOR(0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0), \
				CURSOR(0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0), \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0), \
				CURSOR(0,0,0,1,1,0,1,1,1,0,1,1,0,0,0,0), \
				CURSOR(0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,0), \
				CURSOR(0,0,1,0,0,0,1,1,1,0,0,0,1,0,0,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(0,0,1,0,0,0,1,1,1,0,0,0,1,0,0,0), \
				CURSOR(0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,0), \
				CURSOR(0,0,0,1,1,0,1,1,1,0,1,1,0,0,0,0), \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0), \
				CURSOR(0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0), \
				CURSOR(0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0)  \
			}
#define DOWNCURSORMASK	{ \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0), \
				CURSOR(0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(0,1,1,1,1,0,1,1,1,0,1,1,1,1,0,0), \
				CURSOR(1,1,1,1,0,0,1,1,1,0,0,1,1,1,1,0), \
				CURSOR(1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0), \
				CURSOR(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0), \
				CURSOR(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0), \
				CURSOR(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0), \
				CURSOR(1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0), \
				CURSOR(1,1,1,1,0,0,1,1,1,0,0,1,1,1,1,0), \
				CURSOR(0,1,1,1,1,0,1,1,1,0,1,1,1,1,0,0), \
				CURSOR(0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0), \
				CURSOR(0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0), \
				CURSOR(0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0)  \
			}

static void usage(void);
static void oom(void);
static void set_calibration(cal_parms *c);
static cal_parms *load_calibration(void);
static int save_calibration(cal_parms *cal);
static void calc_button_size(GR_GC_ID gc, char *str, int len, GR_SIZE *retwidth,
		GR_SIZE *retheight);
static void draw_button(GR_WINDOW_ID wid, GR_GC_ID gc, char *str, int len,
		GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height);
static void draw_text(GR_WINDOW_ID wid, GR_GC_ID tgc, GR_GC_ID bgc,
		GR_COORD xbase, GR_COORD ybase, const char *text[],
		char *button1, char *button2, cal_button *b1, cal_button *b2);
static void draw_target(GR_WINDOW_ID wid, GR_GC_ID gc, GR_COORD xbase,
		GR_COORD ybase);
static void redraw(cal_state *state);
static void calculate_parms(cal_state *state);
static void button_down(cal_button *button, int x, int y);
static int button_up(cal_button *button, int x, int y);
static void mouse_checkcal(cal_state *state, GR_EVENT_MOUSE *event);
static void mouse_askpointer(cal_state *state, GR_EVENT_MOUSE *event);
static void start_debounce(cal_state *state);
static void mouse_motion(cal_state *state, GR_EVENT_MOUSE *event);
static void make_cursors(cal_state *state);
static void timer_event(cal_state *state, GR_EVENT_TIMER *event);
static cal_parms *calibrate(void);

#endif
