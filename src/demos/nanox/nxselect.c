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
 * 		- Page indicator shows "Page n/k"
 * 		- OK button prints full path to selected file and exits
 *      - If no file is selected then "[]" is printed/returned
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <nano-X.h>

/* ---------- Constants ---------- */

#define MAX_FILES        70
#define MAX_NAME_LEN     40
#define MAX_PATH_LEN     128

#define ITEM_HEIGHT      18
#define ITEM_SPACING     3
#define MARGIN           4

#define OK_BUTTON_WIDTH  60
#define OK_BUTTON_HEIGHT 22

#define SCROLL_WIDTH     16

/* Number of visible rows INCLUDING "." */
#define VISIBLE_ROWS     10

/* Window placement */
#define WIN_CENTER_X_OFFSET   (-40)
#define WIN_CENTER_Y_OFFSET   (0)

/* ---------- Colors ---------- */

#define EGA_BLACK   GR_RGB(0,0,0)
#define EGA_BLUE    GR_RGB(0,0,128)
#define EGA_BROWN   GR_RGB(128,64,0)
#define EGA_LGRAY   GR_RGB(192,192,192)
#define EGA_DGRAY   GR_RGB(128,128,128)
#define EGA_WHITE   GR_RGB(255,255,255)

/* ---------- Theme ---------- */

#define WIN_BG_COLOR   EGA_LGRAY
#define FILE_BG_COLOR  EGA_BROWN
#define FILE_FG_COLOR  EGA_WHITE
#define DIR_BG_COLOR   EGA_BROWN
#define DIR_FG_COLOR   EGA_WHITE
#define SEL_BG_COLOR   EGA_BLUE
#define SEL_FG_COLOR   EGA_WHITE
#define TITLE_FG_COLOR EGA_WHITE
#define OK_BG_COLOR    EGA_BROWN
#define OK_FG_COLOR    EGA_WHITE

/* ---------- Globals ---------- */

static GR_WINDOW_ID win;
static GR_GC_ID gc;

static GR_COORD win_width  = 200;
static GR_COORD win_height =
    (MARGIN + 20) +
    (VISIBLE_ROWS * (ITEM_HEIGHT + ITEM_SPACING)) +
    MARGIN +
    OK_BUTTON_HEIGHT +
    MARGIN;

static char cwd[MAX_PATH_LEN];
static char file_list[MAX_FILES][MAX_NAME_LEN];
static unsigned char file_is_dir[MAX_FILES];
static int file_count;

static int selected_index = -1;
static int scroll_offset = 0;
static int accepted = 0;

/* Layout cache */
static int g_list_top;
static int g_list_height;
static int g_list_width;
static int g_visible_rows;
static int g_page_size;

/* OK button geometry */
static GR_COORD ok_x, ok_y;
static GR_SIZE  ok_w, ok_h;

/* ---------- Helpers ---------- */

static void safe_strcpy(char *d, const char *s, int n)
{
    strncpy(d, s, n - 1);
    d[n - 1] = 0;
}

static void path_join(const char *a, const char *b,
                      char *out, int n)
{
    snprintf(out, n, "%s/%s", a, b);
}

static void go_parent(void)
{
    char *p;

    if (!strcmp(cwd, "/"))
        return;

    p = strrchr(cwd, '/');
    if (p == cwd)
        cwd[1] = 0;
    else if (p)
        *p = 0;
}

/* ---------- Directory ---------- */

static void load_directory(const char *path)
{
    DIR *d;
    struct dirent *e;
    struct stat st;
    char full[MAX_PATH_LEN];

    file_count = 0;
    selected_index = -1;
    scroll_offset = 0;

    safe_strcpy(file_list[0], ".", MAX_NAME_LEN);
    file_is_dir[0] = 1;
    file_count = 1;

    d = opendir(path);
    if (!d) return;

    while ((e = readdir(d)) && file_count < MAX_FILES) {
        if (!strcmp(e->d_name,".") || !strcmp(e->d_name,".."))
            continue;

        safe_strcpy(file_list[file_count], e->d_name, MAX_NAME_LEN);
        path_join(path, e->d_name, full, sizeof(full));

        file_is_dir[file_count++] =
            (stat(full, &st) == 0 && S_ISDIR(st.st_mode));
    }
    closedir(d);
}

/* ---------- Layout ---------- */

static void compute_layout(void)
{
    ok_w = OK_BUTTON_WIDTH;
    ok_h = OK_BUTTON_HEIGHT;
    ok_x = win_width - ok_w - MARGIN;
    ok_y = win_height - ok_h - MARGIN;

    g_list_top = MARGIN + 20;
    g_visible_rows = VISIBLE_ROWS;
    g_page_size = g_visible_rows - 1;

    g_list_height = g_visible_rows * (ITEM_HEIGHT + ITEM_SPACING);
    g_list_width  = win_width - SCROLL_WIDTH - 2*MARGIN;
}

/* ---------- Drawing helpers ---------- */

static void draw_triangle_up(GR_COORD x, GR_COORD y)
{
    GrSetGCForeground(gc, EGA_DGRAY);
    GrLine(win, gc, x+8, y+3, x+3, y+13);
    GrLine(win, gc, x+8, y+3, x+13, y+13);
    GrLine(win, gc, x+3, y+13, x+13, y+13);
}

static void draw_triangle_down(GR_COORD x, GR_COORD y)
{
    GrSetGCForeground(gc, EGA_DGRAY);
    GrLine(win, gc, x+3, y+3, x+13, y+3);
    GrLine(win, gc, x+3, y+3, x+8, y+13);
    GrLine(win, gc, x+13, y+3, x+8, y+13);
}

/* ---------- Drawing ---------- */

static void draw_window(void)
{
    int i, y;
    int total_items, pages, page;
    char pagebuf[32];

    compute_layout();

    GrSetGCForeground(gc, WIN_BG_COLOR);
    GrFillRect(win, gc, 0, 0, win_width, win_height);

    GrSetGCForeground(gc, TITLE_FG_COLOR);
    GrText(win, gc, MARGIN, MARGIN + 10,
           cwd, strlen(cwd), GR_TFASCII);

    /* "." row */
    y = g_list_top;
    GrSetGCForeground(gc,
        selected_index == 0 ? SEL_BG_COLOR : DIR_BG_COLOR);
    GrFillRect(win, gc, MARGIN, y, g_list_width, ITEM_HEIGHT);
    GrSetGCForeground(gc, FILE_FG_COLOR);
    GrText(win, gc, MARGIN+2, y+ITEM_HEIGHT-4,
           ".", 1, GR_TFASCII);
    y += ITEM_HEIGHT + ITEM_SPACING;

    /* Page rows */
    for (i = 0; i < g_page_size; i++) {
        int idx = 1 + scroll_offset + i;
        if (idx >= file_count) break;

        GrSetGCForeground(gc,
            idx == selected_index ? SEL_BG_COLOR :
            file_is_dir[idx] ? DIR_BG_COLOR : FILE_BG_COLOR);

        GrFillRect(win, gc, MARGIN, y, g_list_width, ITEM_HEIGHT);
        GrSetGCForeground(gc, FILE_FG_COLOR);
        GrText(win, gc, MARGIN+2, y+ITEM_HEIGHT-4,
               file_list[idx], strlen(file_list[idx]), GR_TFASCII);
        y += ITEM_HEIGHT + ITEM_SPACING;
    }

    draw_triangle_up(win_width-SCROLL_WIDTH-MARGIN, g_list_top);
    draw_triangle_down(win_width-SCROLL_WIDTH-MARGIN,
                       g_list_top+g_list_height-SCROLL_WIDTH);

    total_items = file_count > 1 ? file_count-1 : 0;
    pages = (total_items + g_page_size - 1) / g_page_size;
    if (pages < 1) pages = 1;
    page = scroll_offset / g_page_size + 1;

    snprintf(pagebuf, sizeof(pagebuf), "Page %d/%d", page, pages);
    GrSetGCForeground(gc, OK_FG_COLOR);
    GrText(win, gc, ok_x-80, ok_y+ok_h/2+4,
           pagebuf, strlen(pagebuf), GR_TFASCII);

    GrSetGCForeground(gc, OK_BG_COLOR);
    GrFillRect(win, gc, ok_x, ok_y, ok_w, ok_h);
    GrSetGCForeground(gc, OK_FG_COLOR);
    GrRect(win, gc, ok_x, ok_y, ok_w, ok_h);
    GrText(win, gc, ok_x+ok_w/2-6, ok_y+ok_h/2+4,
           "OK", 2, GR_TFASCII);
}

/* ---------- Input ---------- */

static void handle_ok_click(void)
{
    char full[MAX_PATH_LEN];

    if (selected_index < 0)
        return;
    if (file_is_dir[selected_index])
        return;

    path_join(cwd, file_list[selected_index], full, sizeof(full));
    printf("%s\n", full);
    fflush(stdout);
    accepted = 1;
}

static void handle_list_click(int x, int y)
{
    int row, idx;

    if (x < MARGIN || x > MARGIN + g_list_width)
        return;
    if (y < g_list_top)
        return;

    row = (y - g_list_top) / (ITEM_HEIGHT + ITEM_SPACING);
    if (row == 0) {
        go_parent();
        load_directory(cwd);
        return;
    }

    idx = 1 + scroll_offset + (row - 1);
    if (idx >= file_count)
        return;

    if (file_is_dir[idx]) {
        char tmp[MAX_PATH_LEN];
        path_join(cwd, file_list[idx], tmp, sizeof(tmp));
        safe_strcpy(cwd, tmp, MAX_PATH_LEN);
        load_directory(cwd);
    } else {
        selected_index = idx;
    }
}

static void scroll_page_up(void)
{
    scroll_offset -= g_page_size;
    if (scroll_offset < 0) scroll_offset = 0;
}

static void scroll_page_down(void)
{
    int max_offset = (file_count-1) - g_page_size;
    if (max_offset < 0) max_offset = 0;

    scroll_offset += g_page_size;
    if (scroll_offset > max_offset)
        scroll_offset = max_offset;
}

static void set_title(GR_WINDOW_ID win, const char *title)
{
    GR_WM_PROPERTIES props;

    memset(&props, 0, sizeof(props));
    props.flags = GR_WM_FLAGS_TITLE | GR_WM_FLAGS_PROPS;
    props.title = (char *)title;
    props.props = GR_WM_PROPS_APPWINDOW;

    GrSetWMProperties(win, &props);
}

/* ---------- Main ---------- */

int main(void)
{
    GR_EVENT ev;
    GR_SCREEN_INFO si;
    int running = 1;

    getcwd(cwd, sizeof(cwd));

    GrOpen();
    GrGetScreenInfo(&si);

    win = GrNewWindow(GR_ROOT_WINDOW_ID,
        (si.cols-win_width)/2 + WIN_CENTER_X_OFFSET,
        (si.rows-win_height)/2 + WIN_CENTER_Y_OFFSET,
        win_width, win_height,
        1, EGA_DGRAY, EGA_BLACK);

	set_title(win, "Select File");
    gc = GrNewGC();

    GrSelectEvents(win,
        GR_EVENT_MASK_EXPOSURE |
        GR_EVENT_MASK_BUTTON_DOWN |
        GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(win);
    load_directory(cwd);

    while (running) {
        GrGetNextEvent(&ev);

        if (ev.type == GR_EVENT_TYPE_EXPOSURE)
            draw_window();
        else if (ev.type == GR_EVENT_TYPE_BUTTON_DOWN) {

            if (ev.button.x >= ok_x && ev.button.x <= ok_x+ok_w &&
                ev.button.y >= ok_y && ev.button.y <= ok_y+ok_h)
            {
                handle_ok_click();
                running = 0;
            }
            else if (ev.button.x >= win_width-SCROLL_WIDTH-MARGIN &&
                     ev.button.y < g_list_top + SCROLL_WIDTH) {
                scroll_page_up();
            }
            else if (ev.button.x >= win_width-SCROLL_WIDTH-MARGIN) {
                scroll_page_down();
            }
            else {
                handle_list_click(ev.button.x, ev.button.y);
            }
            draw_window();
        }
        else if (ev.type == GR_EVENT_TYPE_CLOSE_REQ)
            running = 0;
    }

    if (accepted)
		write(1,"OK\n",3);
	else
        write(1,"[]\n",3);
	
    GrClose();
    return 0;
}
