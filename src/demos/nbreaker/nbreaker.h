/*
 * NanoBreaker, a Nano-X Breakout clone by Alex Holden.
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
 * The Original Code is NanoBreaker.
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

/* nbreaker.h is the main header file, included by all of the other
 * NanoBreaker source files. */

#ifndef NBREAKER_H
#define NBREAKER_H

/* Define this to include code which dumps (almost) the entire game state when
 * F10 is pressed. */
#define NB_DEBUG

/* Define this to make it possible to activate the power-ups and power-downs
 * at any time by pressing F3-F8. */
#define DEBUG_POWERS

/* Define this to explicitly destroy every Nano-X resource and free every piece
 * of malloc()ed memory on exit (technically not necessary, but it can be
 * useful with a debugging malloc for tracking down memory leaks). */
#define THOROUGH_CLEANUP

/* The default game directory: */
#define DEFAULT_GAME_DIR "/usr/share/nbreaker"
/* The default name of the game file: */
#define DEFAULT_GAME_FILE "default.nbk"
/* Whether to tile backgrounds by default: */
#define DEFAULT_BACKGROUND_TILED 1
/* The default number of points when a normal brick is destroyed: */
#define DEFAULT_NORMALPOINTS 1
/* The default number of points when a small bonus brick is destroyed: */
#define DEFAULT_SMALLBONUSPOINTS 5
/* The default number of points when a medium bonus brick is destroyed: */
#define DEFAULT_MEDIUMBONUSPOINTS 25
/* The default number of points when a large bonus brick is destroyed: */
#define DEFAULT_LARGEBONUSPOINTS 150
/* The default number of points when a huge bonus brick is destroyed: */
#define DEFAULT_HUGEBONUSPOINTS 1000
/* The default number of points gained when a power-up is caught: */
#define DEFAULT_POWERUPPOINTS 20
/* The default number of points lost when a power-down is caught: */
#define DEFAULT_POWERDOWNPOINTS -10
/* The default number of balls given at the start of the game: */
#define DEFAULT_STARTBALLS 5
/* The default number of extra balls given when a level is completed: */
#define DEFAULT_NEWLEVELBALLS 2
/* The default width of the bricks: */
#define DEFAULT_BRICK_WIDTH 40
/* The default height of the bricks: */
#define DEFAULT_BRICK_HEIGHT 20
/* The default height of the bat: */
#define DEFAULT_BAT_HEIGHT 25
/* The default width of the normal width bat: */
#define DEFAULT_NORMALBAT_WIDTH 60
/* The default width of the small width bat: */
#define DEFAULT_SMALLBAT_WIDTH 40
/* The default width of the large width bat: */
#define DEFAULT_LARGEBAT_WIDTH 80
/* The default number of seconds that power-ups last for: */
#define DEFAULT_POWERUP_TIME 30
/* The default number of seconds that power-downs last for: */
#define DEFAULT_POWERDOWN_TIME 20
/* The default width of the brick area: */
#define DEFAULT_WIDTH 15
/* The default height of the brick area: */
#define DEFAULT_HEIGHT 15
/* The default width of power-up and power-down boxes: */
#define DEFAULT_POWER_WIDTH 80
/* The default height of power-up and power-down boxes: */
#define DEFAULT_POWER_HEIGHT 35
/* The default width and height of the ball graphic: */
#define DEFAULT_BALL_SIZE 20
/* The default bat velocity when it is moved with the cursors: */
#define DEFAULT_BAT_VELOCITY 10
/* The default period (in ms) between animation frames: */
#define DEFAULT_ANIMATE_PERIOD 20
/* The default slow ball velocity: */
#define DEFAULT_SLOW_BALL_VELOCITY 4
/* The default normal ball velocity: */
#define DEFAULT_NORMAL_BALL_VELOCITY 8
/* The default fast ball velocity: */
#define DEFAULT_FAST_BALL_VELOCITY 12
/* The default power box velocity: */
#define DEFAULT_POWER_VELOCITY 10
/* The rate to fade new screens in at: */
#define DEFAULT_FADERATE 10

/* The name of the file (in the game directory) to store the high score in: */
#define HISCORE_FILE "hiscore"
/* The font to use for the score text: */
#define SCORE_FONT GR_FONT_SYSTEM_VAR
/* The foreground colour to use when drawing the score text: */
#define SCORE_FGCOLOUR GR_COLOR_WHITE
/* The background colour to use when drawing the score text: */
#define SCORE_BGCOLOUR GR_COLOR_BLACK
/* The grey shade to use as the alpha level for the score text background: */
#define SCORE_ALPHACOLOUR GR_COLOR_GREY40
/* The space in pixels to leave around the score text: */
#define SCORE_BORDER 4
/* The space in pixels to leave around the balls row under the score text: */
#define BALLS_BORDER 4
/* The maximum allowable length of a cheat sequence: */
#define MAXCHEATLEN 16

/* -------------- No user configurable parameters below here. --------------- */

/* The number of different bat sizes: */
#define NUMBATS 3
/* The number of different power types: */
#define NUMPOWERS 6
/* The number of different cheats: */
#define NUMCHEATS 5

/* Used to represent the new ball coordinates when it is moved in animate(). */
typedef struct {
	double x;
	double y;
} coords;

/* A sprite structure (manipulated by the routines in sprite.c): */
struct _sprite {
	/* The name of the file this sprite was loaded from, or NULL if it was
	 * drawn manually: */
	char *fname;
	/* The width and height of the sprite: */
	int w;
	int h;
	/* The IDs of the pixmap and alpha channels containing this sprites
	 * image data: */
	GR_PIXMAP_ID p;
	GR_ALPHA_ID a;
	/* The number of times this sprite has been loaded (a reference count
	 * used to decide when nobody is using it any more so that it can
	 * really be destroyed): */
	int usage;
	/* Each sprite is doubly linked into the list of sprites (arguably a
	 * singly linked list might make more sense since they aren't often
	 * destroyed): */
	struct _sprite *next;
	struct _sprite *prev;
};
typedef struct _sprite sprite;

/* A brick structure. Each brick described by a "Brick" line in the game file
 * is stored as one of these: */
struct _brick {
	/* The sprite containing the image data for this brick: */
	sprite *s;
	/* This bricks identifier (the letter used to represent it in the
	 * game file): */
	char identifier;
	/* The flags associated with this brick (immutable, bonuses, etc.): */
	int flags;
	/* The bricks are stored in a list so they can be destroyed when
	 * switching levels: */
	struct _brick *next;
};
typedef struct _brick brick;

/* A grid entry. The brick area in each level is stored as an array of
 * state->width * state->height grid structures, and so is the current brick
 * area when the game is being played: */
typedef struct {
	/* The brick located at this position or NULL if it is empty. */
	brick *b;
	/* The window coordinates of this brick. It is precalculated and stored
	 * in the grid to save CPU time when doing the collision detection: */
	int x;
	int y;
	/* The number of times this brick has been hit (used by the current
	 * game grid to determine how many times a brick which needs multiple
	 * hits to destroy has been hit): */
	int hits;
	/* The ID of the power-up or power-down hidden behind this brick
	 * (released when the brick is destroyed), or 0 if there isn't one: */
	int power;
} grid;

/* A level. This structure contains all the information used to describe a
 * game level: */
struct _level {
	/* The list of bricks specific to this level: */
	brick *bricks;
	/* The name of the file to use as the background image: */
	char *backgroundname;
	/* Whether the background image file should be tiled: */
	int backgroundtiled;
	/* The number of non-immutable bricks in this level (used to determine
	 * when all the bricks have been destroyed and we can move on to the
	 * next level): */
	int numbricks;
	/* The grid that describes the brick area: */
	grid *grid;
	/* The levels are stored in a singly linked list: */
	struct _level *next;
};
typedef struct _level level;

/* A power-up or power-down. This represents one of those floating boxes that
 * appear when you destroy a brick with a power behind it, and you have to
 * catch in order to activate it: */
struct _power {
	/* The type of power (WideBat, PowerBall, FastMotion, etc.): */
	int type;
	/* The current coordinates of the power box: */
	int x;
	int y;
	/* The current powers are stored in a singly linked list: */
	struct _power *next;
};
typedef struct _power power;

/* Various information associated with the scores bar: */
typedef struct {
	/* The current score: */
	int s;
	/* The high score: */
	int hi;
	/* The high score in the high score file (used to determine whether
	 * we need to write the current high score out or not): */
	int fhi;
	/* The pixmap used to draw the score text onto before blending it onto
	 * the canvas: */
	GR_PIXMAP_ID p;
	/* The alpha channel used when blending the score text onto the
	 * canvas: */
	GR_ALPHA_ID a;
	/* The height of the score area (depends on the height of the font and
	 * the border, but the width is always the full canvas width): */
	int h;
} scores_t;

/* Various information associated with the ball: */
typedef struct {
	/* The current coordinates. They are stored as doubles to make sure we
	 * get smooth straight movement. */
	double x;
	double y;
	/* The current ball direction in radians (0 == north, PI / 2 == east,
	 * PI == south, * PI * 1.5 == west): */
	double d;
	/* The current velocity of the ball: */
	int v;
	/* The coordinates of the last ball position (used when determining
	 * the area that needs to be redrawn when the ball is moved): */
	int lx;
	int ly;
	/* Whether the ball is parked (ie. stuck to the bat and not moving): */
	int parked;
	/* The sprite containing the ball image: */
	sprite *s;
	/* The SlowMotion velocity: */
	int sv;
	/* The normal velocity: */
	int nv;
	/* The FastMotion velocity: */
	int fv;
} ball_t;

/* Various boolean flags: */
typedef struct {
	/* Whether the SolidFloor cheat is active: */
	unsigned int sf : 1;
	/* Whether the NoBounce cheat is active: */
	unsigned int nb : 1;
	/* Whether the NoPowerDown cheat is active: */
	unsigned int npd : 1;
	/* Whether the NoPowerUpTimeOut cheat is active: */
	unsigned int nputo : 1;
	/* Whether the game is paused: */
	unsigned int paused : 1;
	/* Whether the left cursor is currently pressed: */
	unsigned int left : 1;
	/* Whether the right cursor is currently pressed: */
	unsigned int right : 1;
} flags_t;

/* The timers which are used to count (in seconds) how long is left until each
 * of the power-ups and power-downs deactivate (0 means currently inactive): */
typedef struct {
	int widebat;
	int slowmotion;
	int stickybat;
	int powerball;
	int narrowbat;
	int fastmotion;
} powertimes_t;

#include <sys/time.h> /* For struct timeval. */

/* The game state structure. This stores (surprise, surprise) the entire
 * game state. */
typedef struct {
	/* The current game state (TITLESCREEN, RUNNING, etc.): */
	int state;
	/* The name of the game directory: */
	char *gamedir;
	/* The name of the game file: */
	char *gamefile;
	/* The name of the image to use as the title screen background: */
	char *titlebackground;
	/* Whether to tile the title screen background image: */
	int titlebackgroundtiled;
	/* The name of the image to use as the title screen splash box: */
	char *titlesplash;
	/* The name of the image to use as the game won splash box: */
	char *gamewonsplash;
	/* The name of the image to use as the game lost splash box: */
	char *gamelostsplash;
	/* The entire list of sprites: */
	sprite *spritelist;
	/* The sprite containing the current background image: */
	sprite *background;
	/* Whether the current background should be tiled or not: */
	int backgroundtiled;
	/* The number of points gained when a normal brick is destroyed: */
	int normalpoints;
	/* The number of extra points when a small bonus brick is destroyed: */
	int smallbonuspoints;
	/* The number of extra points when a medium bonus brick is destroyed: */
	int mediumbonuspoints;
	/* The number of extra points when a large bonus brick is destroyed: */
	int largebonuspoints;
	/* The number of extra points when a huge bonus brick is destroyed: */
	int hugebonuspoints;
	/* The number of points gained when a power-up is caught: */
	int poweruppoints;
	/* The number of points gained (typically negative) when a power-down
	 * is caught: */
	int powerdownpoints;
	/* The number of balls to start the game with: */
	int startballs;
	/* The number of extra balls given when a new level is started: */
	int newlevelballs;
	/* The width of each brick: */
	int brickwidth;
	/* The height of each brick: */
	int brickheight;
	/* The list of global (ie. not per-level) bricks: */
	brick *bricks;
	/* An alpha channel the same size as a brick, used when drawing a
	 * brick which has reduced transparency because it's a multiple hit
	 * brick which has been hit at least once but not yet destroyed: */
	GR_ALPHA_ID brickalpha;
	/* The width in bricks of the brick area: */
	int width;
	/* The height in bricks of the brick area: */
	int height;
	/* The height of the bat: */
	int batheight;
	/* The widths of each of the different sizes of bat: */
	int batwidths[NUMBATS];
	/* The current bat type in use: */
	int bat;
	/* The current X coordinate of the centre of the bat: */
	int batx;
	/* The speed the bat moves at when using the cursor keys: */
	int batv;
	/* The speed the power boxes move at: */
	int powerv;
	/* The period between animation frames: */
	int animateperiod;
	/* The sprites containing the images used to draw each of the bats: */
	sprite *bats[NUMBATS];
	/* The sprites containing the images used to draw each of the powers: */
	sprite *powersprites[NUMPOWERS];
	/* The sprite containing the current splash image (or NULL): */
	sprite *splash;
	/* The time in seconds that power-ups last for: */
	int poweruptime;
	/* The time in seconds that power-downs last for: */
	int powerdowntime;
	/* The cheat sequences: */
	char *cheats[NUMCHEATS];
	/* The current state of the cheat recogniser engine (see do_cheat()
	 * for details): */
	char cheatstate[MAXCHEATLEN + 1];
	/* Various boolean flags (see the flags_t definition): */
	flags_t flags;
	/* The full list of levels: */
	level *levels;
	/* The current level number: */
	int level;
	/* The total number of levels: */
	int numlevels;
	/* The graphics context used for drawing everything: */
	GR_GC_ID gc;
	/* The ID of the output window: */
	GR_WINDOW_ID wid;
	/* The absolute X position of the output window (used to convert root
	 * window coordinates into our window coordinates so we can get mouse
	 * position events from the root window instead of our own which means
	 * that the bat will continue to respond even when the pointer strays
	 * outside our window): */
	GR_COORD winx;
	/* The ID of the canvas (the double buffer pixmap used to draw
	 * everything onto before it is copied to the screen- this speeds up
	 * exposures a lot, and can potentially speed up blending a lot): */
	GR_PIXMAP_ID canvas;
	/* The IDs of the two temporary canvasses used when fading in: */
	GR_PIXMAP_ID oldcanvas;
	GR_PIXMAP_ID newcanvas;
	/* The ID of the one-second periodic timer used to decrement the power-
	 * up and power-down timers: */
	GR_TIMER_ID tid;
	/* The width in pixels of the canvas (and output window) area: */
	int canvaswidth;
	/* The height in pixels of the canvas (and output window) area: */
	int canvasheight;
	/* The current game grid: */
	grid *grid;
	/* The number of bricks left to be destroyed in the current level: */
	int numbricks;
	/* The list of powers displayed on the screen (or NULL): */
	power *powers;
	/* Various ball information (see the ball_t definition): */
	ball_t ball;
	/* The number of balls currently left: */
	int numballs;
	/* Various scores information (see the scores_t definition): */
	scores_t scores;
	/* The time that animate() was last called (used to determine how
	 * long the GrGetNextEvent timeout should be): */
	struct timeval lastanim;
	/* The power-up and power-down timers: */
	powertimes_t powertimes;
	/* The rate to fade screens in at: */
	int faderate;
	/* The fade level used fading a screen in: */
	int fadelevel;
	/* The next state after the fading has finished: */
	int nextstate;
} nbstate;

/* The flags that can be associated with a brick: */
/* Immutable, ie. the brick can't be destroyed unless the PowerBall power-up
 * is active: */
#define BRICK_FLAG_IMMUTABLE (1 << 0)
/* 2 hits are required to destroy the brick (except with PowerBall): */
#define BRICK_FLAG_2_HITS (1 << 1)
/* 3 hits are required to destroy the brick (except with PowerBall): */
#define BRICK_FLAG_3_HITS (1 << 2)
/* A small points bonus is given when this brick is destroyed: */
#define BRICK_FLAG_SMALL_BONUS (1 << 3)
/* A medium points bonus is given when this brick is destroyed: */
#define BRICK_FLAG_MEDIUM_BONUS (1 << 4)
/* A large points bonus is given when this brick is destroyed: */
#define BRICK_FLAG_LARGE_BONUS (1 << 5)
/* A huge points bonus is given when this brick is destroyed: */
#define BRICK_FLAG_HUGE_BONUS (1 << 6)

/* Symbolic names for the different bat sizes: */
enum {
	/* The normal sized bat used most of the time: */
	NORMALBAT = 0,
	/* The small bat used when the NarrowBat power-down is active: */
	SMALLBAT,
	/* The large bat used when the WideBat power-up is active: */
	LARGEBAT
};

/* Symbolic names for the different power-ups and power-downs: */
enum {
	/* No power-up or power-down: */
	NOPOWER = -1,
	/* WideBat (makes the bat larger): */
	POWERW,
	/* SlowMotion (makes the ball move more slowly): */
	POWERS,
	/* sTickyBat (makes the ball stick to the bat when it hits it): */
	POWERT,
	/* PowerBall (makes the ball destroy any brick with one hit): */
	POWERP,
	/* NarrowBat (makes the bat narrower): */
	POWERN,
	/* FastMotion (makes the ball move faster): */
	POWERF,
};

/* Symbolic names for the different cheats: */
enum {
	/* SolidFloor (makes the ball bounce off the floor under the bat): */
	SFCHEAT = 0,
	/* TelePort (skips to the next level): */
	TPCHEAT,
	/* NoBounce (makes the ball go straight through the bricks): */
	NBCHEAT,
	/* NoPowerDown (stops power-downs from being activated): */
	NPDCHEAT,
	/* NoPowerUpTimeOut (stops power-ups from timing out): */
	NPUTOCHEAT
};

/* Symbolic names for the different game states: */
enum {
	/* The title screen is being displayed: */
	STATE_TITLESCREEN,
	/* The next screen is being faded in: */
	STATE_FADING,
	/* The game is being played: */
	STATE_RUNNING,
	/* The game is over; the player won (completed all levels): */
	STATE_GAMEWON,
	/* The game is over; the player lost: */
	STATE_GAMELOST,
	/* The program is about to exit: */
	STATE_FINISHED
};

/* The function prototypes: */

/* animate.c */
void animate(nbstate *state);

/* ball.c */
void lost_ball(nbstate *state);
void park_ball(nbstate *state);
void move_ball(nbstate *state);
void calc_new_ball_coords(nbstate *state, coords *c);
int check_ball_collision(nbstate *state, coords *c);
void redraw_ball(nbstate *state);

/* bat.c */
void change_bat(nbstate *state, int newbat);
void move_bat(nbstate *state, int x);

/* brick.c */
int brick_collision(nbstate *state, grid *g);

/* cheat.c */
void do_cheat(nbstate *state, GR_KEY key);

/* draw.c */
void draw_background(nbstate *state, int x, int y, int w, int h);
void draw_splash(nbstate *state);
void draw_brick(nbstate *state, grid *g, int x, int y, int w, int h);
void draw_bricks(nbstate *state, int x, int y, int w, int h);
void draw_bat(nbstate *state);
void draw_ball(nbstate *state);
void draw_balls(nbstate *state);
void draw_scores(nbstate *state);
void draw_power(nbstate *state, power *p, int x, int y, int w, int h);
void draw_canvas(nbstate *state, int x, int y, int w, int h);

#ifdef NB_DEBUG
/* dump.c */
void dump_state(nbstate *state);
#endif

/* event.c */
void handle_exposure_event(nbstate *state, GR_EVENT_EXPOSURE *ev);
void handle_button_event(nbstate *state, GR_EVENT_BUTTON *ev);
void handle_mouse_event(nbstate *state, GR_EVENT_MOUSE *ev);
int handle_keystroke_event(nbstate *state, GR_EVENT_KEYSTROKE *ev);
void handle_update_event(nbstate *state, GR_EVENT_UPDATE *ev);
void handle_timer_event(nbstate *state, GR_EVENT_TIMER *ev);

/* game.c */
void reset_game(nbstate *state);

/* init.c */
nbstate *init(int argc, char *argv[]);

/* levels.c */
void increment_level(nbstate *state);
void set_level_active(nbstate *state);

/* loader.c */
int load_game_file(nbstate *state);

/* misc.c */
void myfree(void *p);
void oom(void);

/* powers.c */
void activate_widebat(nbstate *state);
void activate_slowmotion(nbstate *state);
void activate_stickybat(nbstate *state);
void activate_powerball(nbstate *state);
void activate_narrowbat(nbstate *state);
void activate_fastmotion(nbstate *state);
void do_power_timers(nbstate *state);
void new_power(nbstate *state, int type, int x, int y);
void redraw_powers(nbstate *state, int x, int y, int w, int h);
void destroy_power(nbstate *state, power *p);
void power_collision(nbstate *state, power *p);
void animate_powers(nbstate *state);

/* scores.c */
void increment_score(nbstate *state, int points);
void load_hiscore(nbstate *state);
void save_hiscore(nbstate *state);

/* sprite.c */
sprite *make_empty_sprite(nbstate *state, char *fname, int width, int height);
sprite *load_sprite(nbstate *state, char *fname, int width, int height);
void destroy_sprite(nbstate *state, sprite *s);
void destroy_all_sprites(nbstate *state);

#endif
