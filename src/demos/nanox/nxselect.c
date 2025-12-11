/*
 * nxselect - simple Nano-X "Open File" selector for ELKS / 16-bit
 *
 * - Displays files and directories in the current working directory
 * - Special entry "." means "go to parent directory"
 * - Click on a directory entry displays its contents
 * - Click on a file entry to select it (highlighted)
 * - OK button bottom-right prints full path to selected file and exits
 * 
 * Status: problem with colors in list.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <nano-X.h>

/* ---------- Constants ---------- */

#define MAX_FILES        64
#define MAX_NAME_LEN     40
#define MAX_PATH_LEN     128

#define ITEM_HEIGHT      18
#define MARGIN           4

#define OK_BUTTON_WIDTH  60
#define OK_BUTTON_HEIGHT 22

/* ---------- Valid RGB colors from real EGA16 palette ---------- */
/* These values are guaranteed to work in ELKS Nano-X 16-color mode */

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
static GR_COORD win_height = 200;

static char cwd[MAX_PATH_LEN];
static char file_list[MAX_FILES][MAX_NAME_LEN];
static unsigned char file_is_dir[MAX_FILES];
static int file_count = 0;
static int selected_index = -1;

static GR_COORD ok_x, ok_y;
static GR_SIZE  ok_w, ok_h;

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

    /* Parent entry */
    safe_strcpy(file_list[file_count], ".", MAX_NAME_LEN);
    file_is_dir[file_count] = 1;
    file_count++;

    d = opendir(path);
    if (!d)
        return;

    while ((de = readdir(d)) && file_count < MAX_FILES) {
        if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
            continue;

        safe_strcpy(file_list[file_count], de->d_name, MAX_NAME_LEN);
        path_join(path, de->d_name, full, sizeof(full));

        if (stat(full, &st) == 0 && (st.st_mode & S_IFDIR))
            file_is_dir[file_count] = 1;
        else
            file_is_dir[file_count] = 0;

        file_count++;
    }

    closedir(d);
}

/* ---------- Drawing ---------- */

static void draw_window(void)
{
    int i, y;
    GR_COLOR bg, fg;
    char title[200];

    /* Clear window */
    GrSetGCForeground(gc, WIN_BG_COLOR);
    GrFillRect(win, gc, 0, 0, win_width, win_height);

    /* Title text */
    safe_strcpy(title, "Dir: ", sizeof(title));
    strncat(title, cwd, sizeof(title)-strlen(title)-1);

    GrSetGCForeground(gc, TITLE_FG_COLOR);
    GrText(win, gc,
           MARGIN, MARGIN + 10,
           (void *)title, strlen(title), GR_TFASCII);

    /* File list */
    y = MARGIN + 20;

    for (i = 0; i < file_count; i++) {

        if (i == selected_index) {
            bg = SEL_BG_COLOR;
            fg = SEL_FG_COLOR;
        }
        else if (file_is_dir[i]) {
            bg = DIR_BG_COLOR;
            fg = DIR_FG_COLOR;
        }
        else {
            bg = FILE_BG_COLOR;
            fg = FILE_FG_COLOR;
        }

        GrSetGCForeground(gc, bg);
        GrFillRect(win, gc,
                   MARGIN, y,
                   win_width - 2*MARGIN, ITEM_HEIGHT);

        GrSetGCForeground(gc, fg);
        GrText(win, gc,
               MARGIN + 2, y + ITEM_HEIGHT - 4,
               (void *)file_list[i], strlen(file_list[i]), GR_TFASCII);

        y += ITEM_HEIGHT + 3;
    }

    /* OK button */
    ok_w = OK_BUTTON_WIDTH;
    ok_h = OK_BUTTON_HEIGHT;
    ok_x = win_width - ok_w - MARGIN;
    ok_y = win_height - ok_h - MARGIN;

    GrSetGCForeground(gc, OK_BG_COLOR);
    GrFillRect(win, gc, ok_x, ok_y, ok_w, ok_h);

    GrSetGCForeground(gc, OK_FG_COLOR);
    GrText(win, gc,
           ok_x + ok_w/2 - 6,
           ok_y + ok_h/2 + 4,
           (void *)"OK", 2, GR_TFASCII);
}

/* ---------- Input handling ---------- */

static void handle_list_click(int x, int y)
{
    int top = MARGIN + 20;
    int i;

    for (i = 0; i < file_count; i++) {
        int item_y = top + i*(ITEM_HEIGHT + 3);

        if (x >= MARGIN && x <= win_width-MARGIN &&
            y >= item_y && y <= item_y+ITEM_HEIGHT)
        {
            if (file_is_dir[i]) {
                if (!strcmp(file_list[i], "."))
                    go_parent();
                else {
                    char tmp[MAX_PATH_LEN];
                    path_join(cwd, file_list[i], tmp, sizeof(tmp));
                    safe_strcpy(cwd, tmp, MAX_PATH_LEN);
                }
                load_directory(cwd);
            }
            else {
                selected_index = i;
            }
            return;
        }
    }
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
    int running = 1;

    if (!getcwd(cwd, sizeof(cwd)))
        safe_strcpy(cwd, "/", sizeof(cwd));

    GrOpen();

    /* Tiny border window */
    win = GrNewWindow(GR_ROOT_WINDOW_ID,
                      10, 10,
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
            if (ev.button.x >= ok_x && ev.button.x <= ok_x+ok_w &&
                ev.button.y >= ok_y && ev.button.y <= ok_y+ok_h)
            {
                handle_ok_click();
                running = 0;
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
