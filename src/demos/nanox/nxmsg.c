/*
 * nxmsg - Nano-X Message Box for ELKS
 *
 * Developed by: Anton Andreev
 *
 * Features:
 *  - Title and text from command line
 *  - Multiline text using '\n'
 *  - Text alignment: left, center, right
 *  - OK button prints "OK", otherwise "[]"
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <nano-X.h>

/* ---------- UI Modes ---------- */
#define UI_OK        0
#define UI_YESNO     1   /* stub */

/* ---------- Text Alignment ---------- */
#define ALIGN_LEFT   0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT  2

/* ---------- Layout ---------- */
#define MARGIN_TOP      18
#define MARGIN_BOTTOM   12
#define MARGIN_LEFT     10
#define MARGIN_RIGHT    10
#define LINE_SPACING     6
#define TEXT_TOP_PAD     6 

#define BUTTON_WIDTH    60
#define BUTTON_HEIGHT   22
#define BUTTON_MARGIN    8

/* ---------- Colors ---------- */
/*#define EGA_BLACK   GR_RGB(0,0,0)
#define EGA_WHITE   GR_RGB(255,255,255)
#define EGA_LGRAY   GR_RGB(192,192,192)
#define EGA_DGRAY   GR_RGB(128,128,128)
#define EGA_BROWN   GR_RGB(128,64,0)*/

/* ---------- Theme ---------- */
/*#define WIN_BG_COLOR   EGA_LGRAY
#define TEXT_COLOR     EGA_WHITE
#define BTN_BG_COLOR   EGA_BROWN
#define BTN_FG_COLOR   EGA_WHITE
#define FRAME_COLOR    EGA_DGRAY*/

/* ---------- Colors ---------- */
#define COLOR_BLACK   GR_RGB(0,0,0)
#define COLOR_WHITE   GR_RGB(255,255,255)
#define COLOR_GRAY    GR_RGB(192,192,192)

/* ---------- Theme ---------- */
#define WIN_BG_COLOR   COLOR_WHITE     /* window background */
#define TEXT_COLOR     COLOR_BLACK     /* text color */
#define BTN_BG_COLOR   COLOR_GRAY
#define BTN_FG_COLOR   COLOR_BLACK
#define FRAME_COLOR    COLOR_BLACK

/* ---------- Limits ---------- */
#define MAX_LINES      32
#define MAX_LINE_LEN   128

/* ---------- Globals ---------- */
static GR_WINDOW_ID win;
static GR_GC_ID gc;
static GR_SCREEN_INFO si;

static int ui_mode    = UI_OK;
static int text_align = ALIGN_CENTER;

static char title[64];
static char lines[MAX_LINES][MAX_LINE_LEN];
static int line_count;

static GR_COORD win_w, win_h;
static GR_COORD btn_x, btn_y;

static int accepted = 0;

/* ---------- Split text into lines ---------- */

static void split_lines(const char *text)
{
    const char *p = text;
    int j = 0;

    line_count = 0;

    while (*p && line_count < MAX_LINES) {
        if (*p == '\n') {
            lines[line_count][j] = 0;
            line_count++;
            j = 0;
            p++;
            continue;
        }
        if (j < MAX_LINE_LEN - 1)
            lines[line_count][j++] = *p;
        p++;
    }

    lines[line_count][j] = 0;
    line_count++;
}

/* ---------- Compute window size ---------- */

static void compute_window_size(void)
{
    int i;
    GR_SIZE w, h, b;
    int max_width = 0;
    int text_height = 0;

    for (i = 0; i < line_count; i++) {
        GrGetGCTextSize(gc, lines[i], strlen(lines[i]),
                        GR_TFASCII, &w, &h, &b);

        if ((int)w > max_width)
            max_width = w;

        text_height += h;
        if (i < line_count - 1)
            text_height += LINE_SPACING;
    }

    win_w = MARGIN_LEFT + max_width + MARGIN_RIGHT;
    win_h = MARGIN_TOP + TEXT_TOP_PAD +
            text_height +
            BUTTON_MARGIN + BUTTON_HEIGHT +
            MARGIN_BOTTOM;

    if (win_w < BUTTON_WIDTH + 2 * MARGIN_LEFT)
        win_w = BUTTON_WIDTH + 2 * MARGIN_LEFT;

    btn_x = (win_w - BUTTON_WIDTH) / 2;
    btn_y = win_h - BUTTON_HEIGHT - MARGIN_BOTTOM;
}

/* ---------- Drawing ---------- */

static void draw_window(void)
{
    int i;
    GR_SIZE w, h, b;
    GR_COORD y;

    /* Background */
    GrSetGCForeground(gc, WIN_BG_COLOR);
    GrFillRect(win, gc, 0, 0, win_w, win_h);

    /* Frame */
    GrSetGCForeground(gc, FRAME_COLOR);
    GrRect(win, gc, 0, 0, win_w, win_h);

    y = MARGIN_TOP + TEXT_TOP_PAD;

    for (i = 0; i < line_count; i++) {
        GR_COORD x;

        GrGetGCTextSize(gc, lines[i], strlen(lines[i]),
                        GR_TFASCII, &w, &h, &b);

        if (text_align == ALIGN_LEFT)
            x = MARGIN_LEFT;
        else if (text_align == ALIGN_RIGHT)
            x = win_w - MARGIN_RIGHT - w;
        else
            x = (win_w - w) / 2;

        /* Explicit text color every time */
        GrSetGCForeground(gc, TEXT_COLOR);
        GrText(win, gc, x, y + (h - b),
               lines[i], strlen(lines[i]), GR_TFASCII);

        y += h + LINE_SPACING;
    }

    /* OK button */
    if (ui_mode == UI_OK) {
        GrSetGCForeground(gc, BTN_BG_COLOR);
        GrFillRect(win, gc, btn_x, btn_y,
                   BUTTON_WIDTH, BUTTON_HEIGHT);

        GrSetGCForeground(gc, BTN_FG_COLOR);
        GrRect(win, gc, btn_x, btn_y,
               BUTTON_WIDTH, BUTTON_HEIGHT);

        GrSetGCForeground(gc, BTN_FG_COLOR);
        GrText(win, gc,
               btn_x + BUTTON_WIDTH / 2 - 6,
               btn_y + BUTTON_HEIGHT / 2 + 4,
               "OK", 2, GR_TFASCII);
    }
}

/* ---------- Input handling ---------- */

static int handle_button(GR_EVENT_BUTTON *b)
{
    if (ui_mode == UI_OK) {
        if (b->x >= btn_x && b->x <= btn_x + BUTTON_WIDTH &&
            b->y >= btn_y && b->y <= btn_y + BUTTON_HEIGHT)
			{
			  accepted = 1;
              return 1;
			}
    }
    return 0;
}

static int handle_key(GR_EVENT_KEYSTROKE *k)
{
    if (k->ch == MWKEY_ESCAPE)
        return 1;

    if (ui_mode == UI_OK &&
        (k->ch == MWKEY_ENTER || k->ch == '\r'))
        return 1;

    return 0;
}

/* ---------- Main ---------- */
int main(int argc, char *argv[])
{
    GR_EVENT ev;
    int running = 1;
    int argi = 1;

    /* defaults */
    text_align = ALIGN_CENTER;

    if (argc >= 3 && strcmp(argv[argi], "-ta") == 0) {
        if (argc < 5) {
            fprintf(stderr,
                "Usage: nxmsg [-ta 0|1|2] \"Title\" \"Text\"\n");
            return 1;
        }

        int ta = atoi(argv[argi + 1]);
        if (ta == 1)
            text_align = ALIGN_LEFT;
        else if (ta == 2)
            text_align = ALIGN_RIGHT;
        else
            text_align = ALIGN_CENTER; /* 0 or fallback */

        argi += 2;
    }

    if (argc - argi < 2) {
        fprintf(stderr,
            "Usage: nxmsg [-ta 0|1|2] \"Title\" \"Text\"\n");
        return 1;
    }

    strncpy(title, argv[argi], sizeof(title) - 1);
    title[sizeof(title) - 1] = 0;

    split_lines(argv[argi + 1]);

    GrOpen();
    GrGetScreenInfo(&si);

    gc = GrNewGC();
	
	GrSetGCUseBackground(gc, GR_FALSE);

    compute_window_size();

    win = GrNewWindow(
        GR_ROOT_WINDOW_ID,
        (si.cols - win_w) / 2,
        (si.rows - win_h) / 2,
        win_w,
        win_h,
        1,
        FRAME_COLOR,
        WIN_BG_COLOR
    );

    {
        GR_WM_PROPERTIES props;
        memset(&props, 0, sizeof(props));
        props.flags = GR_WM_FLAGS_TITLE;
        props.title = title;
        GrSetWMProperties(win, &props);
    }

    GrSelectEvents(
        win,
        GR_EVENT_MASK_EXPOSURE |
        GR_EVENT_MASK_BUTTON_DOWN |
        GR_EVENT_MASK_KEY_DOWN |
        GR_EVENT_MASK_CLOSE_REQ
    );

    GrMapWindow(win);

    while (running) {
        GrGetNextEvent(&ev);

        switch (ev.type) {
        case GR_EVENT_TYPE_EXPOSURE:
            draw_window();
            break;

        case GR_EVENT_TYPE_BUTTON_DOWN:
            if (handle_button(&ev.button))
                running = 0;
            break;

        case GR_EVENT_TYPE_KEY_DOWN:
            if (handle_key(&ev.keystroke))
                running = 0;
            break;

        case GR_EVENT_TYPE_CLOSE_REQ:
            running = 0;
            break;
        }
    }

    if (accepted)
        write(1, "OK\n", 3);
    else
        write(1, "[]\n", 3);

    GrClose();
    return 0;
}
