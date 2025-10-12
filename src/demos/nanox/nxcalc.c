/* nxcalc - Simple graphical calculator for Microwindows / Nano-X
 *
 * Inspired by nxclock example (Greg Haerr).
 *
 * Build:
 *   gcc -O2 nxcalc.c -o nxcalc -lNano-X -lpthread -lm
 *
 * Run inside an environment where the Nano-X server is available.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "nano-X.h"
#if ELKS
#include <sys/linksym.h>
#endif

#define WIN_W  160
#define WIN_H  200
#define DISP_H 35

static GR_WINDOW_ID win;
static GR_GC_ID gc_text, gc_button, gc_button_press, gc_border;

/* calculator state */
static char display[128];
static double acc = 0.0;
static char pending_op = 0;
static int entering = 0;
static int dot_entered = 0;

/* button layout */
static char *btn_labels[] = {
    "C", "<-", "+/-", "/",
    "7", "8", "9", "*",
    "4", "5", "6", "-",
    "1", "2", "3", "+",
    "0", ".", "=", NULL
};

#define COLS 4
#define ROWS 5

static GR_RECT btn_rects[ROWS][COLS];

/* function prototypes */
static void redraw(void);
static void press_button(const char *label);
static void compute_equal(void);
static void apply_pending(double val);

static double display_to_double(void) {
    if (strlen(display) == 0) return 0.0;
    return strtod(display, NULL);
}

static void set_display_from_double(double v) {
    char buf[128];

    if (fabs(v) < 1e-12) v = 0.0;
#if ELKS
    __LINK_SYMBOL(dtostr);
    if (v == 0.0) {
#else
    double iv;
    if (modf(v, &iv) == 0.0) {
#endif
        snprintf(buf, sizeof(buf), "%.0f", v);
    } else {
        snprintf(buf, sizeof(buf), "%.10g", v);
    }

    strncpy(display, buf, sizeof(display) - 1);
    display[sizeof(display) - 1] = '\0';
}

/* ========================= MAIN ========================= */

int main(int argc, char **argv) {
    GR_EVENT event;
    int r, c, idx;

    if (GrOpen() < 0) {
        fprintf(stderr, "nxcalc: cannot open Nano-X\n");
        return 1;
    }

    win = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "nxcalc", GR_ROOT_WINDOW_ID,
                        10, 10, WIN_W, WIN_H, GrGetSysColor(GR_COLOR_WINDOW));

    GrSelectEvents(win, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_DOWN |
                   GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_CLOSE_REQ);

    gc_text = GrNewGC();
    gc_button = GrNewGC();
    gc_button_press = GrNewGC();
    gc_border = GrNewGC();

    GrSetGCForeground(gc_text, GrGetSysColor(GR_COLOR_WINDOWTEXT));
    GrSetGCBackground(gc_text, GrGetSysColor(GR_COLOR_WINDOW));

    GrSetGCForeground(gc_button, GrGetSysColor(GR_COLOR_BTNFACE));
    GrSetGCBackground(gc_button, GrGetSysColor(GR_COLOR_WINDOW));

    GrSetGCForeground(gc_button_press, GrGetSysColor(GR_COLOR_BTNSHADOW));
    GrSetGCBackground(gc_button_press, GrGetSysColor(GR_COLOR_WINDOW));

    GrSetGCForeground(gc_border, GrGetSysColor(GR_COLOR_BTNTEXT));
    GrSetGCBackground(gc_border, GrGetSysColor(GR_COLOR_WINDOW));

    /* init state */
    display[0] = '\0';
    acc = 0.0;
    pending_op = 0;
    entering = 0;
    dot_entered = 0;

    /* layout buttons */
    {
        int grid_x = 10, grid_y = DISP_H + 10;
        int grid_w = WIN_W - 20;
        int grid_h = WIN_H - DISP_H - 20;
        int cell_w = grid_w / COLS;
        int cell_h = grid_h / ROWS;

        idx = 0;
        for (r = 0; r < ROWS; r++) {
            for (c = 0; c < COLS; c++) {
                int x = grid_x + c * cell_w;
                int y = grid_y + r * cell_h;
                btn_rects[r][c].x = x;
                btn_rects[r][c].y = y;
                btn_rects[r][c].width = cell_w - 6;
                btn_rects[r][c].height = cell_h - 6;
                idx++;
            }
        }
    }

    GrMapWindow(win);
    set_display_from_double(0.0);
    redraw();

    while (1) {
        GrGetNextEvent(&event);
        switch (event.type) {
            case GR_EVENT_TYPE_EXPOSURE:
                redraw();
                break;

            case GR_EVENT_TYPE_BUTTON_DOWN: {
                int mx = event.button.x;
                int my = event.button.y;
                int found = 0;
                for (r = 0; r < ROWS && !found; r++) {
                    for (c = 0; c < COLS && !found; c++) {
                        GR_RECT *rc = &btn_rects[r][c];
                        if (mx >= rc->x && mx <= rc->x + rc->width &&
                            my >= rc->y && my <= rc->y + rc->height) {
                            GrFillRect(win, gc_button_press, rc->x, rc->y, rc->width, rc->height);
                            found = 1;
                        }
                    }
                }
                break;
            }

            case GR_EVENT_TYPE_BUTTON_UP: {
                int mx = event.button.x;
                int my = event.button.y;
                int found = 0;
                for (r = 0; r < ROWS && !found; r++) {
                    for (c = 0; c < COLS && !found; c++) {
                        GR_RECT *rc = &btn_rects[r][c];
                        if (mx >= rc->x && mx <= rc->x + rc->width &&
                            my >= rc->y && my <= rc->y + rc->height) {
                            idx = r * COLS + c;
                            const char *lbl =
                                (idx < (int)(sizeof(btn_labels)/sizeof(btn_labels[0]))
                                 ? btn_labels[idx] : NULL);
                            if (lbl) {
                                press_button(lbl);
                                redraw();
                            }
                            found = 1;
                        }
                    }
                }
                break;
            }

            case GR_EVENT_TYPE_CLOSE_REQ:
                GrClose();
                return 0;
        }
    }

    return 0;
}

/* ========================= UI DRAWING ========================= */

static void redraw(void) {
    int r, c, idx, tw, th, tb;

    /* clear */
    GrFillRect(win, gc_button, 0, 0, WIN_W, WIN_H);

    /* display area */
    GrSetGCForeground(gc_button, GrGetSysColor(GR_COLOR_WINDOW));
    GrFillRect(win, gc_button, 5, 5, WIN_W - 10, DISP_H - 10);
    GrSetGCForeground(gc_border, GrGetSysColor(GR_COLOR_BTNSHADOW));
    GrRect(win, gc_border, 5, 5, WIN_W - 6, DISP_H - 6);

    /* display text (right aligned) */
    GrGetGCTextSize(gc_text, display, strlen(display), GR_TFTOP, &tw, &th, &tb);
    {
        int x = WIN_W - 12 - tw;
        if (x < 10) x = 10;
        GrText(win, gc_text, x, 5 + (DISP_H / 2) - 10, display, strlen(display), GR_TFTOP);
    }

    /* draw buttons */
    idx = 0;
    GrSetGCForeground(gc_button, GrGetSysColor(GR_COLOR_BTNFACE));
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            GR_RECT *rc = &btn_rects[r][c];
            char *lbl =
                (idx < (int)(sizeof(btn_labels)/sizeof(btn_labels[0]))
                 ? btn_labels[idx] : NULL);

            GrFillRect(win, gc_button, rc->x, rc->y, rc->width, rc->height);
            GrSetGCForeground(gc_border, GrGetSysColor(GR_COLOR_BTNSHADOW));
            GrRect(win, gc_border, rc->x, rc->y, rc->x + rc->width - 1, rc->y + rc->height - 1);

            if (lbl) {
                GrGetGCTextSize(gc_text, lbl, strlen(lbl), GR_TFTOP, &tw, &th, &tb);
                GrText(win, gc_text, rc->x + rc->width/2 - tw/2, rc->y + rc->height/2 - 2, lbl, strlen(lbl), GR_TFTOP);
            }
            idx++;
        }
    }

    GrFlush();
}

/* ========================= LOGIC ========================= */

static void press_button(const char *label) {
    char tmp[128];
    double val;

    if (strcmp(label, "C") == 0) {
        display[0] = '\0';
        acc = 0.0;
        pending_op = 0;
        entering = 0;
        dot_entered = 0;
        set_display_from_double(0.0);
        return;
    }

    if (strcmp(label, "<-") == 0) {
        int l = strlen(display);
        if (l > 0) {
            if (display[l-1] == '.') dot_entered = 0;
            display[l-1] = '\0';
            if (strlen(display) == 0) set_display_from_double(0.0);
        }
        entering = (strlen(display) > 0);
        return;
    }

    if (strcmp(label, "+/-") == 0) {
        if (display[0] == '-') {
            memmove(display, display+1, strlen(display));
        } else {
            if (strlen(display) == 0 || strcmp(display, "0") == 0) return;
            snprintf(tmp, sizeof(tmp), "-%s", display);
            strncpy(display, tmp, sizeof(display)-1);
        }
        return;
    }

    if (strcmp(label, "+") == 0 || strcmp(label, "-") == 0 ||
        strcmp(label, "*") == 0 || strcmp(label, "/") == 0) {

        val = display_to_double();
        if (pending_op == 0)
            acc = val;
        else
            apply_pending(val);

        pending_op = label[0];
        entering = 0;
        dot_entered = 0;
        set_display_from_double(acc);
        return;
    }

    if (strcmp(label, "=") == 0) {
        compute_equal();
        return;
    }

    if (strcmp(label, ".") == 0) {
        if (!dot_entered) {
            if (!entering) {
                strcpy(display, "0.");
                entering = 1;
            } else {
                strncat(display, ".", sizeof(display)-strlen(display)-1);
            }
            dot_entered = 1;
        }
        return;
    }

    if (strlen(label) == 1 && isdigit((unsigned char)label[0])) {
        if (!entering) {
            display[0] = label[0];
            display[1] = '\0';
            entering = 1;
        } else {
            if (strcmp(display, "0") == 0 && label[0] != '0' && !dot_entered) {
                display[0] = label[0]; display[1] = '\0';
            } else if (strlen(display) < sizeof(display) - 2) {
                strncat(display, label, 1);
            }
        }
        return;
    }
}

static void apply_pending(double val) {
    if (pending_op == '+') acc = acc + val;
    else if (pending_op == '-') acc = acc - val;
    else if (pending_op == '*') acc = acc * val;
    else if (pending_op == '/') {
        if (val == 0.0) {
            strncpy(display, "Error", sizeof(display)-1);
            display[sizeof(display)-1] = '\0';
            pending_op = 0;
            entering = 0;
            dot_entered = 0;
            redraw();
            return;
        } else {
            acc = acc / val;
        }
    }
}

static void compute_equal(void) {
    double val = display_to_double();
    if (pending_op != 0) {
        apply_pending(val);
        pending_op = 0;
        set_display_from_double(acc);
        entering = 0;
        dot_entered = 0;
    } else {
        set_display_from_double(val);
    }
}
