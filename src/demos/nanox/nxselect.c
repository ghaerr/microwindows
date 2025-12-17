/*
 * nxselect - simple Nano-X "Open File" selector (for ELKS currently)
 *
 * Created by: Anton Andreev
 *	
 *  History 
 *   Version 1.0:
 * 		- Displays files and directories in the current working directory
 * 		- Special entry "." means "go to parent directory"
 * 		- Click on a directory entry displays its contents
 * 		- Click on a file entry to select it (highlighted)
 * 		- Scroll arrows scroll by PAGES (not single items)
 * 		- Page indicator near OK button shows "Page n/k"
 * 		- OK button prints full path to selected file and exits
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <nano-X.h>

/* ---------- Constants ---------- */

#define MAX_FILES        70 /* might not be enough */
#define MAX_NAME_LEN     40
#define MAX_PATH_LEN     128

#define ITEM_HEIGHT      18
#define ITEM_SPACING     3
#define MARGIN           4

#define OK_BUTTON_WIDTH  60
#define OK_BUTTON_HEIGHT 22

/* Right-side scroll "gutter" width */
#define SCROLL_WIDTH     16

/* Window placement (offset from centered position) */
#define WIN_CENTER_X_OFFSET   (-40)   /* left from center */
#define WIN_CENTER_Y_OFFSET   (0)

/* ---------- Valid RGB colors from real EGA16 palette ---------- */

#define EGA_BLACK        GR_RGB(0,0,0)
#define EGA_BLUE         GR_RGB(0,0,128)
#define EGA_GREEN        GR_RGB(0,128,0)
#define EGA_CYAN         GR_RGB(0,128,128)
#define EGA_RED          GR_RGB(128,0,0)
#define EGA_MAGENTA      GR_RGB(128,0,128)
#define EGA_BROWN        GR_RGB(128,64,0)     /* our ORANGE */
#define EGA_LGRAY        GR_RGB(192,192,192)
#define EGA_DGRAY        GR_RGB(128,128,128)
#define EGA_BBLUE        GR_RGB(0,0,255)
#define EGA_BGREEN       GR_RGB(0,255,0)
#define EGA_BCYAN        GR_RGB(0,255,255)
#define EGA_BRED         GR_RGB(255,0,0)
#define EGA_BMAGENTA     GR_RGB(255,0,255)
#define EGA_YELLOW       GR_RGB(255,255,0)
#define EGA_WHITE        GR_RGB(255,255,255)

/* ---------- UI Color Theme ---------- */

#define WIN_BG_COLOR     EGA_LGRAY

#define FILE_BG_COLOR    EGA_BROWN
#define FILE_FG_COLOR    EGA_WHITE

#define DIR_BG_COLOR     EGA_BROWN
#define DIR_FG_COLOR     EGA_WHITE

#define SEL_BG_COLOR     EGA_BLUE
#define SEL_FG_COLOR     EGA_WHITE

#define TITLE_FG_COLOR   EGA_WHITE

#define OK_BG_COLOR      EGA_BROWN
#define OK_FG_COLOR      EGA_WHITE

/* ---------- Nano-X globals ---------- */

static GR_WINDOW_ID win;
static GR_GC_ID gc;

static GR_COORD win_width  = 200;
static GR_COORD win_height = 220;

static char cwd[MAX_PATH_LEN];
static char file_list[MAX_FILES][MAX_NAME_LEN];
static unsigned char file_is_dir[MAX_FILES];
static int file_count = 0;

static int selected_index = -1;

/*
 * scroll_offset counts how many entries AFTER index 0 are skipped.
 * Index 0 ("." up) is always shown and never scrolled away.
 * So the first visible "real" entry is at index (1 + scroll_offset).
 */
static int scroll_offset = 0;

/* OK button geometry */
static GR_COORD ok_x, ok_y;
static GR_SIZE  ok_w, ok_h;

/* Cached layout for hit-testing + paging */
static int g_list_top = 0;
static int g_list_height = 0;
static int g_visible_rows = 0;     /* total rows drawn including "." row */
static int g_page_size = 0;        /* rows per page excluding "." row */
static int g_list_width = 0;

/* ---------- Helpers ---------- */

static void safe_strcpy(char *dst, const char *src, int size)
{
    if (size <= 0) return;
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

static void path_join(const char *dir, const char *name,
                      char *out, int outsize)
{
    int len;

    safe_strcpy(out, dir, outsize);
    len = strlen(out);

    if (len == 0) {
        safe_strcpy(out, name, outsize);
        return;
    }

    if (out[len-1] != '/' && len < outsize-1) {
        out[len] = '/';
        out[len+1] = '\0';
        len++;
    }

    strncat(out, name, outsize-1-len);
    out[outsize-1] = '\0';
}

static void go_parent(void)
{
    int len = strlen(cwd);

    if (!strcmp(cwd, "/"))
        return;

    while (len > 0 && cwd[len-1] == '/')
        cwd[--len] = '\0';

    while (len > 0 && cwd[len-1] != '/')
        cwd[--len] = '\0';

    if (len == 0)
        safe_strcpy(cwd, "/", MAX_PATH_LEN);
    else
        cwd[len] = '\0';
}

/* ---------- Directory loading ---------- */

static void load_directory(const char *path)
{
    DIR *d;
    struct dirent *de;
    struct stat st;
    char full[MAX_PATH_LEN];

    file_count = 0;
    selected_index = -1;
    scroll_offset = 0;

    /* Parent entry at index 0 */
    safe_strcpy(file_list[0], ".", MAX_NAME_LEN);
    file_is_dir[0] = 1;
    file_count = 1;

    d = opendir(path);
    if (!d)
        return;

    while ((de = readdir(d)) && file_count < MAX_FILES) {
        if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
            continue;

        safe_strcpy(file_list[file_count], de->d_name, MAX_NAME_LEN);
        path_join(path, de->d_name, full, sizeof(full));

        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode))
            file_is_dir[file_count] = 1;
        else
            file_is_dir[file_count] = 0;

        file_count++;
    }

    closedir(d);
}

/* ---------- 3D button + triangle arrows (ELKS-safe: no GrFillPoly) ---------- */

static void draw_3d_box(GR_COORD x, GR_COORD y, GR_SIZE w, GR_SIZE h, GR_COLOR fill)
{
    GrSetGCForeground(gc, fill);
    GrFillRect(win, gc, x, y, w, h);

    /* light top/left */
    GrSetGCForeground(gc, EGA_WHITE);
    GrLine(win, gc, x, y, x+w-1, y);
    GrLine(win, gc, x, y, x, y+h-1);

    /* dark bottom/right */
    GrSetGCForeground(gc, EGA_DGRAY);
    GrLine(win, gc, x, y+h-1, x+w-1, y+h-1);
    GrLine(win, gc, x+w-1, y, x+w-1, y+h-1);
}

static void draw_ok_button(void)
{
    draw_3d_box(ok_x, ok_y, ok_w, ok_h, OK_BG_COLOR);

    GrSetGCForeground(gc, OK_FG_COLOR);
    GrText(win, gc,
           ok_x + ok_w/2 - 6,
           ok_y + ok_h/2 + 4,
           (void *)"OK", 2, GR_TFASCII);
}

/* Outline-only triangles (grey) */
static void draw_triangle_up(GR_COORD x, GR_COORD y)
{
    GR_COORD cx = x + SCROLL_WIDTH/2;
    GR_COORD left = x + 2;
    GR_COORD right = x + SCROLL_WIDTH - 3;
    GR_COORD top = y + 3;
    GR_COORD base = y + SCROLL_WIDTH - 3;

    GrSetGCForeground(gc, EGA_DGRAY);
    GrLine(win, gc, cx, top, left, base);
    GrLine(win, gc, cx, top, right, base);
    GrLine(win, gc, left, base, right, base);
}

static void draw_triangle_down(GR_COORD x, GR_COORD y)
{
    GR_COORD cx = x + SCROLL_WIDTH/2;
    GR_COORD left = x + 2;
    GR_COORD right = x + SCROLL_WIDTH - 3;
    GR_COORD base = y + 3;
    GR_COORD tip = y + SCROLL_WIDTH - 3;

    GrSetGCForeground(gc, EGA_DGRAY);
    GrLine(win, gc, left, base, right, base);
    GrLine(win, gc, left, base, cx, tip);
    GrLine(win, gc, right, base, cx, tip);
}

/* ---------- Layout computation ---------- */

static void compute_layout(void)
{
    /* OK button */
    ok_w = OK_BUTTON_WIDTH;
    ok_h = OK_BUTTON_HEIGHT;
    ok_x = win_width - ok_w - MARGIN;
    ok_y = win_height - ok_h - MARGIN;

    /* List geometry */
    g_list_top = MARGIN + 20;
    g_list_width = win_width - SCROLL_WIDTH - 2*MARGIN;

    g_list_height = ok_y - g_list_top - MARGIN;
    if (g_list_height < (ITEM_HEIGHT + ITEM_SPACING) * 2)
        g_list_height = (ITEM_HEIGHT + ITEM_SPACING) * 2;

    g_visible_rows = g_list_height / (ITEM_HEIGHT + ITEM_SPACING);
    if (g_visible_rows < 2) g_visible_rows = 2;  /* includes "." row */

    g_page_size = g_visible_rows - 1;            /* excluding "." row */
    if (g_page_size < 1) g_page_size = 1;
}

/* ---------- Drawing ---------- */

static void draw_window(void)
{
    int i, y;
    GR_COLOR bg, fg;
    char title[200];
    int total_items, pages, page;
    char pagebuf[32];

    compute_layout();

    /* Clear window */
    GrSetGCForeground(gc, WIN_BG_COLOR);
    GrFillRect(win, gc, 0, 0, win_width, win_height);

    /* Title */
    safe_strcpy(title, "Dir: ", sizeof(title));
    strncat(title, cwd, sizeof(title)-strlen(title)-1);

    GrSetGCForeground(gc, TITLE_FG_COLOR);
    GrText(win, gc,
           MARGIN, MARGIN + 10,
           (void *)title, strlen(title), GR_TFASCII);

    /* Always draw "." row at top (never scrolled away) */
    y = g_list_top;

    if (selected_index == 0) { bg = SEL_BG_COLOR; fg = SEL_FG_COLOR; }
    else                     { bg = DIR_BG_COLOR; fg = DIR_FG_COLOR; }

    GrSetGCForeground(gc, bg);
    GrFillRect(win, gc, MARGIN, y, g_list_width, ITEM_HEIGHT);

    GrSetGCForeground(gc, fg);
    GrText(win, gc,
           MARGIN + 2, y + ITEM_HEIGHT - 4,
           (void *)file_list[0], strlen(file_list[0]), GR_TFASCII);

    y += ITEM_HEIGHT + ITEM_SPACING;

    /* Draw page of items (indices 1..file_count-1) */
    for (i = 0; i < g_page_size; i++) {
        int idx = 1 + scroll_offset + i;
        if (idx >= file_count) break;

        if (idx == selected_index) { bg = SEL_BG_COLOR; fg = SEL_FG_COLOR; }
        else if (file_is_dir[idx]) { bg = DIR_BG_COLOR; fg = DIR_FG_COLOR; }
        else                       { bg = FILE_BG_COLOR; fg = FILE_FG_COLOR; }

        GrSetGCForeground(gc, bg);
        GrFillRect(win, gc, MARGIN, y, g_list_width, ITEM_HEIGHT);

        GrSetGCForeground(gc, fg);
        GrText(win, gc,
               MARGIN + 2, y + ITEM_HEIGHT - 4,
               (void *)file_list[idx], strlen(file_list[idx]), GR_TFASCII);

        y += ITEM_HEIGHT + ITEM_SPACING;
    }

    /* Right-side arrows (grey triangles) */
    draw_triangle_up(win_width - SCROLL_WIDTH - MARGIN, g_list_top);
    draw_triangle_down(win_width - SCROLL_WIDTH - MARGIN,
                       g_list_top + g_list_height - SCROLL_WIDTH);

    /* Page indicator (same color as button text) */
    total_items = (file_count > 1) ? (file_count - 1) : 0;
    pages = (total_items + g_page_size - 1) / g_page_size;
    if (pages < 1) pages = 1;
    page = (scroll_offset / g_page_size) + 1;
    if (page < 1) page = 1;
    if (page > pages) page = pages;

    snprintf(pagebuf, sizeof(pagebuf), "Page %d/%d", page, pages);

    GrSetGCForeground(gc, OK_FG_COLOR);
    GrText(win, gc,
           ok_x - 80,
           ok_y + ok_h/2 + 4,
           (void *)pagebuf, strlen(pagebuf), GR_TFASCII);

    /* OK button (3D) */
    draw_ok_button();
}

/* ---------- Input handling ---------- */

static void open_dir_index(int idx)
{
    if (idx == 0) {
        go_parent();
        load_directory(cwd);
        return;
    }

    /* idx is a directory entry */
    {
        char tmp[MAX_PATH_LEN];
        path_join(cwd, file_list[idx], tmp, sizeof(tmp));
        safe_strcpy(cwd, tmp, MAX_PATH_LEN);
    }
    load_directory(cwd);
}

static void handle_list_click(int x, int y)
{
    int idx = -1;
    int row;

    compute_layout();

    /* Click inside list area only (exclude right gutter) */
    if (x < MARGIN || x > (MARGIN + g_list_width))
        return;

    /* Determine which row was clicked */
    if (y < g_list_top) return;
    row = (y - g_list_top) / (ITEM_HEIGHT + ITEM_SPACING);

    if (row == 0) {
        idx = 0; /* "." */
    } else {
        idx = 1 + scroll_offset + (row - 1);
        if (idx >= file_count) return;
    }

    if (file_is_dir[idx]) {
        open_dir_index(idx);
    } else {
        selected_index = idx;
    }
}

static int hit_scroll_up(int x, int y)
{
    int ax = win_width - SCROLL_WIDTH - MARGIN;
    int ay = g_list_top;
    return (x >= ax && x <= ax + SCROLL_WIDTH &&
            y >= ay && y <= ay + SCROLL_WIDTH);
}

static int hit_scroll_down(int x, int y)
{
    int ax = win_width - SCROLL_WIDTH - MARGIN;
    int ay = g_list_top + g_list_height - SCROLL_WIDTH;
    return (x >= ax && x <= ax + SCROLL_WIDTH &&
            y >= ay && y <= ay + SCROLL_WIDTH);
}

static void scroll_page_up(void)
{
    if (scroll_offset <= 0) {
        scroll_offset = 0;
        return;
    }
    scroll_offset -= g_page_size;
    if (scroll_offset < 0) scroll_offset = 0;
}

static void scroll_page_down(void)
{
    int total_items = (file_count > 1) ? (file_count - 1) : 0;
    int max_offset;

    if (total_items <= g_page_size) {
        scroll_offset = 0;
        return;
    }

    max_offset = total_items - g_page_size;
    if (max_offset < 0) max_offset = 0;

    scroll_offset += g_page_size;
    if (scroll_offset > max_offset) scroll_offset = max_offset;
}

static void handle_ok_click(void)
{
    char full[MAX_PATH_LEN];

    if (selected_index < 0) return;
    if (file_is_dir[selected_index]) return;

    path_join(cwd, file_list[selected_index], full, sizeof(full));
    printf("%s\n", full);
    fflush(stdout);
}

/* ---------- Main ---------- */

int main(void)
{
    GR_EVENT ev;
    GR_SCREEN_INFO si;
    int running = 1;
    GR_COORD start_x = 10, start_y = 10;

    if (!getcwd(cwd, sizeof(cwd)))
        safe_strcpy(cwd, "/", sizeof(cwd));

    GrOpen();
    GrGetScreenInfo(&si);

    /* centered + offsets */
    start_x = (si.cols - win_width)/2 + WIN_CENTER_X_OFFSET;
    start_y = (si.rows - win_height)/2 + WIN_CENTER_Y_OFFSET;
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;

    win = GrNewWindow(GR_ROOT_WINDOW_ID,
                      start_x, start_y,
                      win_width, win_height,
                      1,
                      EGA_DGRAY,  /* border */
                      EGA_BLACK); /* background under border */

    gc = GrNewGC();

    GrSelectEvents(win,
        GR_EVENT_MASK_EXPOSURE |
        GR_EVENT_MASK_BUTTON_DOWN |
        GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(win);
    load_directory(cwd);

    while (running) {

        GrGetNextEvent(&ev);

        switch (ev.type) {

        case GR_EVENT_TYPE_EXPOSURE:
            draw_window();
            break;

        case GR_EVENT_TYPE_BUTTON_DOWN:
            compute_layout();

            /* OK button hit */
            if (ev.button.x >= ok_x && ev.button.x <= ok_x+ok_w &&
                ev.button.y >= ok_y && ev.button.y <= ok_y+ok_h)
            {
                handle_ok_click();
                running = 0;
            }
            /* Scroll arrows: PAGE scrolling */
            else if (hit_scroll_up(ev.button.x, ev.button.y)) {
                scroll_page_up();
                draw_window();
            }
            else if (hit_scroll_down(ev.button.x, ev.button.y)) {
                scroll_page_down();
                draw_window();
            }
            else {
                handle_list_click(ev.button.x, ev.button.y);
                draw_window();
            }
            break;

        case GR_EVENT_TYPE_CLOSE_REQ:
            running = 0;
            break;
        }
    }

    GrClose();
    return 0;
}
