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

#define MAX_FILES      64 /* check if /bin is more */
#define MAX_NAME_LEN   40
#define MAX_PATH_LEN   128

#define ITEM_HEIGHT    16
#define MARGIN         4
#define OK_BUTTON_WIDTH   60
#define OK_BUTTON_HEIGHT  20

/* Nano-X globals */
static GR_WINDOW_ID win;
static GR_GC_ID gc;
static GR_COORD win_width  = 320;
static GR_COORD win_height = 200;

/* File browser state */
static char cwd[MAX_PATH_LEN];
static char file_list[MAX_FILES][MAX_NAME_LEN];
static unsigned char file_is_dir[MAX_FILES];
static int file_count = 0;
static int selected_index = -1;

/* OK button rectangle */
static GR_COORD ok_x, ok_y;
static GR_SIZE  ok_w, ok_h;

/* --------- Helpers --------- */

static void safe_strcpy(char *dst, const char *src, int size)
{
    if (size <= 0) return;
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

static void path_join(const char *dir, const char *name, char *out, int outsize)
{
    int len;

    if (outsize <= 0) return;

    safe_strcpy(out, dir, outsize);
    len = strlen(out);

    if (len == 0) {
        /* Just copy name */
        safe_strcpy(out, name, outsize);
        return;
    }

    if (out[len - 1] != '/' && len < outsize - 1) {
        out[len] = '/';
        out[len + 1] = '\0';
        len++;
    }

    strncat(out, name, outsize - 1 - len);
    out[outsize - 1] = '\0';
}

static void go_parent(void)
{
    int len;

    if (strcmp(cwd, "/") == 0)
        return;

    len = strlen(cwd);

    while (len > 0 && cwd[len - 1] == '/') {
        cwd[--len] = '\0';
    }

    while (len > 0 && cwd[len - 1] != '/') {
        cwd[--len] = '\0';
    }

    if (len == 0) {
        safe_strcpy(cwd, "/", MAX_PATH_LEN);
    } else {
        cwd[len] = '\0';
    }
}

/* --------- Directory loading --------- */

static void load_directory(const char *path)
{
    DIR *d;
    struct dirent *de;
    struct stat st;
    char full[MAX_PATH_LEN];

    file_count = 0;
    selected_index = -1;

    /* First entry: "." used as "go parent" */
    safe_strcpy(file_list[file_count], ".", MAX_NAME_LEN);
    file_is_dir[file_count] = 1;
    file_count++;

    d = opendir(path);
    if (d == NULL)
        return;

    while ((de = readdir(d)) != NULL && file_count < MAX_FILES) {
        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0) {
            continue;
        }

        safe_strcpy(file_list[file_count], de->d_name, MAX_NAME_LEN);

        path_join(path, de->d_name, full, sizeof(full));

        if (stat(full, &st) == 0 && (st.st_mode & S_IFDIR)) {
            file_is_dir[file_count] = 1;
        } else {
            file_is_dir[file_count] = 0;
        }

        file_count++;
    }

    closedir(d);
}

/* --------- Drawing --------- */

static void draw_window(void)
{
    int i;
    int y;
    GR_COLOR bg, fg;
    char title[160];

    /* Clear background */
    GrSetGCForeground(gc, GR_RGB(200, 200, 200));
    GrFillRect(win, gc, 0, 0, win_width, win_height);

    /* Draw current directory at top */
    GrSetGCForeground(gc, GR_RGB(0, 0, 0));
    safe_strcpy(title, "Dir: ", sizeof(title));
    strncat(title, cwd, sizeof(title) - 1 - strlen(title));
    title[sizeof(title) - 1] = '\0';
    GrText(win, gc, MARGIN, MARGIN + 10, title, strlen(title), GR_TFASCII);

    /* Draw list starting below title */
    y = MARGIN + 16;

    for (i = 0; i < file_count; i++) {
        if (i == selected_index) {
            bg = GR_RGB(0, 0, 128);
            fg = GR_RGB(255, 255, 255);
        } else if (file_is_dir[i]) {
            bg = GR_RGB(180, 200, 220);
            fg = GR_RGB(0, 0, 0);
        } else {
            bg = GR_RGB(230, 230, 230);
            fg = GR_RGB(0, 0, 0);
        }

        GrSetGCForeground(gc, bg);
        GrFillRect(win, gc,
                   MARGIN, y,
                   win_width - 2 * MARGIN, ITEM_HEIGHT);

        GrSetGCForeground(gc, fg);
        GrText(win, gc,
               MARGIN + 2, y + ITEM_HEIGHT - 4,
               file_list[i], strlen(file_list[i]), GR_TFASCII);

        y += ITEM_HEIGHT + 1;
    }

    /* OK button geometry */
    ok_w = OK_BUTTON_WIDTH;
    ok_h = OK_BUTTON_HEIGHT;
    ok_x = win_width - ok_w - MARGIN;
    ok_y = win_height - ok_h - MARGIN;

    /* Draw OK button */
    GrSetGCForeground(gc, GR_RGB(150, 200, 150));
    GrFillRect(win, gc, ok_x, ok_y, ok_w, ok_h);

    GrSetGCForeground(gc, GR_RGB(0, 0, 0));
    GrText(win, gc,
           ok_x + (ok_w / 2) - 8,
           ok_y + (ok_h / 2) + 4,
           "OK", 2, GR_TFASCII);
}

/* --------- Input handling --------- */

static void handle_list_click(int x, int y)
{
    int list_top;
    int i;
    int item_y;

    list_top = MARGIN + 16;

    for (i = 0; i < file_count; i++) {
        item_y = list_top + i * (ITEM_HEIGHT + 1);

        if (y >= item_y && y <= item_y + ITEM_HEIGHT &&
            x >= MARGIN && x <= win_width - MARGIN) {

            /* Clicked entry i */
            if (file_is_dir[i]) {
                if (strcmp(file_list[i], ".") == 0) {
                    go_parent();
                } else {
                    char tmp[MAX_PATH_LEN];
                    path_join(cwd, file_list[i], tmp, sizeof(tmp));
                    safe_strcpy(cwd, tmp, MAX_PATH_LEN);
                }
                load_directory(cwd);
            } else {
                /* Select file */
                selected_index = i;
            }
            return;
        }
    }
}

static void handle_ok_click(void)
{
    char full[MAX_PATH_LEN];

    /*if (selected_index < 0)
        return;
    if (file_is_dir[selected_index])
        return;

    path_join(cwd, file_list[selected_index], full, sizeof(full));
    */
    strcpy(full, "/bin/pic1.jpg");

    /* Either write(1, "/full/path/to/file\n", length); or (printf + fflush) is required to avoid buffering */
    printf("%s\n", full);
    fflush(stdout); /* Required */
}

int main(void)
{
    GR_EVENT event;
    int running = 1;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        /* Fallback */
        safe_strcpy(cwd, "/", sizeof(cwd));
    }

    GrOpen();

    win = GrNewWindow(GR_ROOT_WINDOW_ID,
                      10, 10,
                      win_width, win_height,
                      1,
                      GR_RGB(100, 100, 100),
                      GR_RGB(0, 0, 0));

    gc = GrNewGC();

    GrSelectEvents(win,
                   GR_EVENT_MASK_EXPOSURE |
                   GR_EVENT_MASK_BUTTON_DOWN |
                   GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(win);

    load_directory(cwd);

    while (running) {
        
        GrGetNextEvent(&event);

        switch (event.type) {
        case GR_EVENT_TYPE_EXPOSURE:
            draw_window();
            break;

        case GR_EVENT_TYPE_BUTTON_DOWN:
            {
                int x = event.button.x;
                int y = event.button.y;

                /* Check OK button */
                if (x >= ok_x && x <= ok_x + ok_w &&
                    y >= ok_y && y <= ok_y + ok_h) {
                    handle_ok_click();
                    running = 0;
                } else {
                    handle_list_click(x, y);
                    draw_window();
                }
            }
            break;

        case GR_EVENT_TYPE_CLOSE_REQ:
            running = 0;
            break;

        default:
            break;
        }
    }

    GrClose();
    return 0;
}

