/*
 * NXEDIT - ELKS/Nano-X swap-backed text editor
 *
 * This version is designed for ELKS-class 16-bit systems using Nano-X.
 *
 * Contributors: Anton Andreev (@toncho11), Greg Haerr (@ghaerr)
 *
 * The editor keeps a small editable RAM cache. Dirty lines remain in RAM.
 * A dirty line is appended to the swap file only when:
 *
 *   1. the cache is full and that dirty slot must be reused,
 *   2. Ctrl-S saves the document,
 *   3. the editor quits,
 *   4. a structural edit such as split/merge needs stable line data.
 *
 * Normal typing, Backspace/Delete inside a line, cursor movement, redraw,
 * Home/End, and moving between cached lines do not write to disk.
 *
 * CONTROL VARIABLES / TUNING
 * --------------------------
 *   SWAP_PATH       Default append-only swap file.
 *   MAX_LINES       Maximum logical lines.
 *   MAX_LINE_LEN    Maximum bytes in one logical line.
 *   CACHE_LINES     Editable RAM line cache. Increase this to reduce writes.
 *   TAB_WIDTH       Spaces inserted for Tab.
 *
 * HOW STORAGE WORKS
 * -----------------
 * line_table[] stores the latest flushed location of each logical line.
 * cache[] stores recently used lines and may contain dirty newer versions.
 *
 * If a cache line is dirty, it is newer than line_table[] and is authoritative.
 * When flushed, it is appended to /tmp/nxedit and line_table[] is updated.
 *
 * Old swap blocks are never deleted, overwritten, compressed, or compacted.
 * They simply become unreachable. This is done to gain speed as we often
 * have enough disk space, but not enough RAM and CPU.
 *
 * BASIC CONTROLS
 * --------------
 *   Text keys       insert character
 *   Arrows          move cursor
 *   Home / End      line start / line end
 *   Page Up/Down    scroll
 *   Backspace       delete left, or merge with previous line at column zero
 *   Delete          delete under cursor, or merge next line at EOL
 *   Enter           split line
 *   Tab             insert spaces
 *   Ctrl-S          save
 *   Escape          quit
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <nano-X.h>

/* ---------- user-tunable limits ---------- */

#define SWAP_PATH       "/tmp/nxedit"
#define MAX_LINES       512
#define MAX_LINE_LEN    240
#define CACHE_LINES     8
#define TAB_WIDTH       5

#define TITLE           "nxedit"
#define COLS            80
#define LINES           25

#define FGCOLOR         fg_color
#define BGCOLOR         bg_color

#define LEFT_MARGIN     6
#define TOP_MARGIN      14

#define BLOCK_MAGIC     0xBA57U
#define BLOCK_VALID     0x0001U

/* ---------- compatibility fallbacks ---------- */

#ifndef MWKEY_ESCAPE
#define MWKEY_ESCAPE    27
#endif
#ifndef MWKEY_ENTER
#define MWKEY_ENTER     13
#endif
#ifndef MWKEY_BACKSPACE
#define MWKEY_BACKSPACE 8
#endif
#ifndef MWKEY_TAB
#define MWKEY_TAB       9
#endif
#ifndef MWKEY_DELETE
#define MWKEY_DELETE    127
#endif

#ifndef MWKMOD_CTRL
#define MWKMOD_CTRL     0x0004
#endif

/* ---------- internal key codes ---------- */

#define K_NONE      0
#define K_TEXT      1
#define K_ESC       2
#define K_LEFT      3
#define K_RIGHT     4
#define K_UP        5
#define K_DOWN      6
#define K_HOME      7
#define K_END       8
#define K_PGUP      9
#define K_PGDN      10
#define K_BS        11
#define K_DEL       12
#define K_ENTER     13
#define K_TAB       14
#define K_SAVE      15

/* ---------- repaint actions ---------- */

#define PAINT_NONE       0
#define PAINT_CURSOR     1
#define PAINT_LINE_TAIL  2
#define PAINT_FROM_LINE  3
#define PAINT_FULL       4
#define PAINT_STATUS     5

/* ---------- cursor repaint tuning ---------- */

/*
 * Cursor repaint modes:
 *
 *   0 = fastest: erase only the old vertical cursor line with BGCOLOR.
 *       This is the smallest repaint, but it can leave small glyph scratches
 *       on some Nano-X/font combinations.
 *
 *   1 = retype one character at the old cursor column without clearing a
 *       rectangle. It first erases only the old vertical cursor line with
 *       BGCOLOR, then redraws s[col]. This is the preferred mode for arrow
 *       movement because it avoids rectangle-clear damage.
 *
 *   2 = repaint one character at the old cursor column using a rectangle:
 *       s[col]. This is more aggressive than RETYPE1 and may damage the
 *       next character if the rectangle/font metrics do not align perfectly.
 *
 *   3 = repaint two characters starting at the old cursor column:
 *       s[col] and s[col + 1]. Use this only if the cursor damages the next
 *       character too.
 *
 *   4 = repaint three characters around the old cursor position:
 *       s[col - 1], s[col], and s[col + 1]. This is useful if the cursor
 *       cleanup needs one character of context on both sides. It depends on
 *       CHAR_WIDTH matching the real fixed-font advance well.
 *
 *   5 = safest: repaint the whole old cursor line. This is still much cheaper
 *       than full-window redraw, and useful if small partial cursor repainting
 *       causes character corruption.
 */
#define CURSOR_REPAINT_CURSOR_ONLY 0
#define CURSOR_REPAINT_RETYPE1     1
#define CURSOR_REPAINT_AFTER1      2
#define CURSOR_REPAINT_AFTER2      3
#define CURSOR_REPAINT_3CHARS      4
#define CURSOR_REPAINT_LINE        5

#define CURSOR_REPAINT_MODE        CURSOR_REPAINT_RETYPE1

/* Extra horizontal safety pixels around the tiny cursor repaint rectangle. */
#define CURSOR_REPAINT_XPAD        1

/*
 * Destination-side cursor repair.
 *
 * This is separate from repairing the place the cursor left.
 * It is useful when drawing the new vertical cursor damages pixels in the
 * character(s) to the right of the destination cursor.
 *
 */
#define CURSOR_DEST_REPAIR_ENABLE       0
#define CURSOR_DEST_REPAIR_AFTER_DRAW   1
#define CURSOR_DEST_REPAIR_START_OFF    1
#define CURSOR_DEST_REPAIR_CHARS        2
#define CURSOR_DEST_REPAIR_XPAD         0

typedef struct {
    int mode;
    unsigned short line;
    unsigned short col;
} PaintAction;

/* ---------- swap structures ---------- */

typedef unsigned long disk_off_t;

typedef struct {
    unsigned short magic;
    unsigned short len;
    unsigned short flags;
    unsigned short gen;
} BlockHeader;

typedef struct {
    disk_off_t off;
    unsigned short len;
    unsigned short gen;
    unsigned char valid;
} LineRef;

typedef struct {
    unsigned short line_no;
    unsigned short gen;
    unsigned char valid;
    char text[MAX_LINE_LEN + 1];
} LineCache;

/* ---------- globals kept deliberately small ---------- */

static int WIN_W, WIN_H;
static int CHAR_WIDTH, LINE_HEIGHT;

static LineRef line_table[MAX_LINES];
static LineCache cache[CACHE_LINES];

static char current_file[128];
static char editbuf[MAX_LINE_LEN + 1];
static char scratch[MAX_LINE_LEN + 1];

static unsigned short line_count = 1;
static unsigned short edit_line = 0;
static unsigned short edit_len = 0;
static unsigned char edit_dirty = 0;

static unsigned short cx = 0;
static unsigned short cy = 0;
static int scroll_y = 0;

static int swap_fd = -1;

static GR_WINDOW_ID win;
static GR_GC_ID gc;

static MWCOLORVAL fg_color;
static MWCOLORVAL bg_color;

static PaintAction paint_action;

static MWCOLORVAL make_color(int r, int g, int b)
{
    /*
     * Use MWRGB with runtime arguments to avoid compile-time overflow warnings
     * from BLACK/LTGRAY on small MWCOLORVAL builds.
     */
    return MWRGB(r, g, b);
}

/* ---------- small utilities ---------- */

static unsigned short c_strlen_u(const char *s)
{
    unsigned short n;

    n = 0;
    while (s[n] != '\0' && n < MAX_LINE_LEN) {
        n++;
    }
    return n;
}

static void cache_invalidate_all(void)
{
    unsigned short i;

    for (i = 0; i < CACHE_LINES; i++) {
        cache[i].valid = 0;
    }
}

static void line_set_empty(unsigned short n)
{
    line_table[n].off = 0L;
    line_table[n].len = 0;
    line_table[n].gen = 0;
    line_table[n].valid = 1;
}

static void paint_set(int mode, unsigned short line, unsigned short col)
{
    paint_action.mode = mode;
    paint_action.line = line;
    paint_action.col = col;
}

/* ---------- append-only swap file ---------- */

static int swap_open_file(const char *path)
{
    swap_fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (swap_fd < 0) {
        return -1;
    }
    return 0;
}

static void swap_close_file(void)
{
    if (swap_fd >= 0) {
        close(swap_fd);
        swap_fd = -1;
    }
}

static int swap_append_line(const char *buf, unsigned short len,
                            unsigned short gen, disk_off_t *out_off)
{
    BlockHeader h;
    long pos;
    int wr;

    if (swap_fd < 0) {
        return -1;
    }

    if (len > MAX_LINE_LEN) {
        len = MAX_LINE_LEN;
    }

    pos = lseek(swap_fd, 0L, SEEK_END);
    if (pos < 0L) {
        return -1;
    }

    h.magic = BLOCK_MAGIC;
    h.len = len;
    h.flags = BLOCK_VALID;
    h.gen = gen;

    wr = write(swap_fd, (char *)&h, sizeof(h));
    if (wr != (int)sizeof(h)) {
        return -1;
    }

    if (len != 0) {
        wr = write(swap_fd, buf, len);
        if (wr != (int)len) {
            return -1;
        }
    }

    *out_off = (disk_off_t)pos;
    return 0;
}

static int swap_read_line(disk_off_t off, char *buf, unsigned short maxlen)
{
    BlockHeader h;
    unsigned short n;
    int rd;

    if (swap_fd < 0 || maxlen == 0) {
        return -1;
    }

    if (lseek(swap_fd, (long)off, SEEK_SET) < 0L) {
        return -1;
    }

    rd = read(swap_fd, (char *)&h, sizeof(h));
    if (rd != (int)sizeof(h)) {
        return -1;
    }

    if (h.magic != BLOCK_MAGIC) {
        return -1;
    }

    n = h.len;
    if (n >= maxlen) {
        n = maxlen - 1;
    }

    if (n != 0) {
        rd = read(swap_fd, buf, n);
        if (rd != (int)n) {
            return -1;
        }
    }

    buf[n] = '\0';
    return (int)n;
}

static int line_write_new(unsigned short n, const char *buf, unsigned short len)
{
    disk_off_t off;
    unsigned short gen;

    if (n >= MAX_LINES) {
        return -1;
    }

    gen = line_table[n].gen + 1;

    if (swap_append_line(buf, len, gen, &off) < 0) {
        return -1;
    }

    line_table[n].off = off;
    line_table[n].len = len;
    line_table[n].gen = gen;
    line_table[n].valid = 1;

    cache_invalidate_all();
    return 0;
}

static int line_read(unsigned short n, char *buf, unsigned short maxlen)
{
    if (n >= line_count || !line_table[n].valid) {
        if (maxlen != 0) {
            buf[0] = '\0';
        }
        return 0;
    }

    if (line_table[n].len == 0) {
        if (maxlen != 0) {
            buf[0] = '\0';
        }
        return 0;
    }

    return swap_read_line(line_table[n].off, buf, maxlen);
}

/* ---------- edit buffer ---------- */

static int flush_edit_line(void)
{
    if (!edit_dirty) {
        return 0;
    }

    editbuf[edit_len] = '\0';
    if (line_write_new(edit_line, editbuf, edit_len) < 0) {
        return -1;
    }

    edit_dirty = 0;
    return 0;
}

static int load_edit_line(unsigned short n)
{
    int r;

    if (n >= line_count) {
        return -1;
    }

    if (flush_edit_line() < 0) {
        return -1;
    }

    r = line_read(n, editbuf, sizeof(editbuf));
    if (r < 0) {
        editbuf[0] = '\0';
        edit_len = 0;
    } else {
        edit_len = (unsigned short)r;
    }

    edit_line = n;
    edit_dirty = 0;

    if (cx > edit_len) {
        cx = edit_len;
    }

    return 0;
}

static void ensure_edit_current(void)
{
    if (edit_line != cy) {
        load_edit_line(cy);
    }
}

static char *cache_get_line(unsigned short n)
{
    unsigned short i;
    unsigned short slot;

    if (n == edit_line) {
        return editbuf;
    }

    for (i = 0; i < CACHE_LINES; i++) {
        if (cache[i].valid &&
            cache[i].line_no == n &&
            cache[i].gen == line_table[n].gen) {
            return cache[i].text;
        }
    }

    slot = n % CACHE_LINES;
    cache[slot].line_no = n;
    cache[slot].gen = line_table[n].gen;
    cache[slot].valid = 1;

    if (line_read(n, cache[slot].text, sizeof(cache[slot].text)) < 0) {
        cache[slot].text[0] = '\0';
    }

    return cache[slot].text;
}

/* ---------- document loading/saving ---------- */

static void doc_new_empty(void)
{
    line_count = 1;
    line_set_empty(0);
    edit_line = 0;
    edit_len = 0;
    edit_dirty = 0;
    editbuf[0] = '\0';
    cx = 0;
    cy = 0;
    cache_invalidate_all();

    line_write_new(0, "", 0);
    load_edit_line(0);
}

static void load_file(const char *name)
{
    FILE *f;
    char inbuf[MAX_LINE_LEN + 8];
    unsigned short len;
    char *p;

    strncpy(current_file, name, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = '\0';

    f = fopen(name, "r");
    if (f == NULL) {
        doc_new_empty();
        return;
    }

    line_count = 0;

    while (line_count < MAX_LINES && fgets(inbuf, sizeof(inbuf), f) != NULL) {
        p = inbuf;
        len = 0;

        while (*p != '\0' && *p != '\n' && *p != '\r' && len < MAX_LINE_LEN) {
            if (*p == '\t') {
                unsigned short k;
                for (k = 0; k < TAB_WIDTH && len < MAX_LINE_LEN; k++) {
                    scratch[len++] = ' ';
                }
            } else {
                scratch[len++] = *p;
            }
            p++;
        }

        scratch[len] = '\0';
        line_set_empty(line_count);
        line_write_new(line_count, scratch, len);
        line_count++;
    }

    fclose(f);

    if (line_count == 0) {
        doc_new_empty();
    } else {
        cx = 0;
        cy = 0;
        edit_line = 0;
        edit_dirty = 0;
        load_edit_line(0);
    }
}

static int save_file(void)
{
    FILE *f;
    unsigned short i;
    char *s;

    if (current_file[0] == '\0') {
        return -1;
    }

    if (flush_edit_line() < 0) {
        return -1;
    }

    f = fopen(current_file, "w");
    if (f == NULL) {
        return -1;
    }

    for (i = 0; i < line_count; i++) {
        if (i == edit_line) {
            s = editbuf;
        } else {
            if (line_read(i, scratch, sizeof(scratch)) < 0) {
                scratch[0] = '\0';
            }
            s = scratch;
        }

        fputs(s, f);
        fputc('\n', f);
    }

    fclose(f);
    return 0;
}

/* ---------- editing operations ---------- */

static void insert_char(int ch)
{
    unsigned short i;

    ensure_edit_current();

    if (edit_len >= MAX_LINE_LEN) {
        return;
    }

    if (cx > edit_len) {
        cx = edit_len;
    }

    for (i = edit_len; i > cx; i--) {
        editbuf[i] = editbuf[i - 1];
    }

    editbuf[cx] = (char)ch;
    edit_len++;
    cx++;
    editbuf[edit_len] = '\0';
    edit_dirty = 1;
}

static void insert_tab(void)
{
    unsigned short i;

    for (i = 0; i < TAB_WIDTH; i++) {
        insert_char(' ');
    }
}

static void backspace_key(void)
{
    unsigned short i;
    unsigned short prev_len;
    unsigned short cur_len;

    ensure_edit_current();

    if (cx > 0) {
        for (i = cx - 1; i < edit_len; i++) {
            editbuf[i] = editbuf[i + 1];
        }
        edit_len--;
        cx--;
        edit_dirty = 1;
        return;
    }

    if (cy == 0) {
        return;
    }

    /* Merge current line into previous line. */
    cur_len = edit_len;

    flush_edit_line();

    line_read((unsigned short)(cy - 1), scratch, sizeof(scratch));
    prev_len = c_strlen_u(scratch);

    if ((unsigned short)(prev_len + cur_len) > MAX_LINE_LEN) {
        return;
    }

    for (i = 0; i < cur_len && prev_len + i < MAX_LINE_LEN; i++) {
        scratch[prev_len + i] = editbuf[i];
    }
    scratch[prev_len + cur_len] = '\0';

    line_write_new((unsigned short)(cy - 1), scratch,
                   (unsigned short)(prev_len + cur_len));

    for (i = cy; i + 1 < line_count; i++) {
        line_table[i] = line_table[i + 1];
    }

    line_count--;
    cy--;
    cx = prev_len;
    cache_invalidate_all();
    load_edit_line(cy);
}

static void delete_key(void)
{
    unsigned short i;
    unsigned short next_len;

    ensure_edit_current();

    if (cx < edit_len) {
        for (i = cx; i < edit_len; i++) {
            editbuf[i] = editbuf[i + 1];
        }
        edit_len--;
        edit_dirty = 1;
        return;
    }

    if (cy + 1 >= line_count) {
        return;
    }

    /* Merge next line into current line. */
    line_read((unsigned short)(cy + 1), scratch, sizeof(scratch));
    next_len = c_strlen_u(scratch);

    if ((unsigned short)(edit_len + next_len) > MAX_LINE_LEN) {
        return;
    }

    for (i = 0; i < next_len; i++) {
        editbuf[edit_len + i] = scratch[i];
    }
    edit_len += next_len;
    editbuf[edit_len] = '\0';
    edit_dirty = 1;

    for (i = cy + 1; i + 1 < line_count; i++) {
        line_table[i] = line_table[i + 1];
    }

    line_count--;
    cache_invalidate_all();
}

static void enter_key(void)
{
    unsigned short i;
    unsigned short tail_len;

    ensure_edit_current();

    if (line_count >= MAX_LINES) {
        return;
    }

    if (cx > edit_len) {
        cx = edit_len;
    }

    tail_len = edit_len - cx;

    for (i = 0; i < tail_len; i++) {
        scratch[i] = editbuf[cx + i];
    }
    scratch[tail_len] = '\0';

    edit_len = cx;
    editbuf[edit_len] = '\0';
    edit_dirty = 1;
    flush_edit_line();

    for (i = line_count; i > cy + 1; i--) {
        line_table[i] = line_table[i - 1];
    }

    line_set_empty((unsigned short)(cy + 1));
    line_write_new((unsigned short)(cy + 1), scratch, tail_len);
    line_count++;

    cy++;
    cx = 0;
    cache_invalidate_all();
    load_edit_line(cy);
}

static void move_left(void)
{
    if (cx > 0) {
        cx--;
    } else if (cy > 0) {
        load_edit_line((unsigned short)(cy - 1));
        cy--;
        cx = edit_len;
    }
}

static void move_right(void)
{
    ensure_edit_current();

    if (cx < edit_len) {
        cx++;
    } else if (cy + 1 < line_count) {
        cy++;
        cx = 0;
        load_edit_line(cy);
    }
}

static void move_up(void)
{
    unsigned short oldx;

    if (cy == 0) {
        return;
    }

    oldx = cx;
    cy--;
    load_edit_line(cy);
    if (oldx < edit_len) {
        cx = oldx;
    } else {
        cx = edit_len;
    }
}

static void move_down(void)
{
    unsigned short oldx;

    if (cy + 1 >= line_count) {
        return;
    }

    oldx = cx;
    cy++;
    load_edit_line(cy);
    if (oldx < edit_len) {
        cx = oldx;
    } else {
        cx = edit_len;
    }
}

/* ---------- Nano-X drawing ---------- */

static int visible_rows_count(void)
{
    int visible_rows;

    visible_rows = (WIN_H - TOP_MARGIN - LINE_HEIGHT - 4) / LINE_HEIGHT;
    if (visible_rows < 1) {
        visible_rows = 1;
    }

    return visible_rows;
}

static int first_visible_line(void)
{
    int first_line;

    first_line = scroll_y / LINE_HEIGHT;
    if (first_line < 0) {
        first_line = 0;
    }

    return first_line;
}

static int line_is_visible(unsigned short line_no)
{
    int first_line;
    int visible_rows;

    first_line = first_visible_line();
    visible_rows = visible_rows_count();

    return ((int)line_no >= first_line &&
            (int)line_no < first_line + visible_rows);
}

static int line_baseline_y(unsigned short line_no)
{
    int first_line;

    first_line = first_visible_line();
    return TOP_MARGIN + ((int)line_no - first_line) * LINE_HEIGHT;
}

static void set_fg(int color)
{
    GrSetGCForeground(gc, color);
}

static void clear_rect(int x, int y, int w, int h)
{
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x >= WIN_W || y >= WIN_H) {
        return;
    }
    if (x + w > WIN_W) {
        w = WIN_W - x;
    }
    if (y + h > WIN_H) {
        h = WIN_H - y;
    }
    if (w <= 0 || h <= 0) {
        return;
    }

    set_fg(BGCOLOR);
    GrFillRect(win, gc, x, y, w, h);
    set_fg(FGCOLOR);
}

static void clear_text_row_y(int y)
{
    /* Stay inside one text row. Using LINE_HEIGHT + 3 can touch the next row. */
    clear_rect(0, y - LINE_HEIGHT + 1, WIN_W, LINE_HEIGHT);
}

static void draw_text_line_no_clear(unsigned short line_no)
{
    char *s;
    unsigned short len;
    int y;

    if (!line_is_visible(line_no)) {
        return;
    }

    y = line_baseline_y(line_no);
    s = cache_get_line(line_no);
    len = c_strlen_u(s);

    if (len > 0) {
        GrText(win, gc, LEFT_MARGIN, y, s, len, GR_TFASCII);
    }
}

static void draw_line_tail(unsigned short line_no, unsigned short col)
{
    char *s;
    unsigned short len;
    int x;
    int y;

    if (!line_is_visible(line_no)) {
        return;
    }

    x = LEFT_MARGIN + (int)col * CHAR_WIDTH;
    y = line_baseline_y(line_no);

    clear_rect(x, y - LINE_HEIGHT + 1, WIN_W - x, LINE_HEIGHT);

    s = cache_get_line(line_no);
    len = c_strlen_u(s);

    if (col < len) {
        GrText(win, gc, x, y, s + col, len - col, GR_TFASCII);
    }
}

static void draw_from_line(unsigned short start_line)
{
    int first_line;
    int visible_rows;
    int end_line;
    int i;
    int y;

    first_line = first_visible_line();
    visible_rows = visible_rows_count();
    end_line = first_line + visible_rows;

    if ((int)start_line < first_line) {
        start_line = (unsigned short)first_line;
    }

    for (i = (int)start_line; i < end_line; i++) {
        y = TOP_MARGIN + (i - first_line) * LINE_HEIGHT;
        clear_text_row_y(y);
        if (i >= 0 && i < (int)line_count) {
            draw_text_line_no_clear((unsigned short)i);
        }
    }
}

static void draw_cursor_at(unsigned short line_no, unsigned short col, int color)
{
    int cursor_x;
    int cursor_y;

    if (!line_is_visible(line_no)) {
        return;
    }

    cursor_x = LEFT_MARGIN + (int)col * CHAR_WIDTH;
    cursor_y = line_baseline_y(line_no);

    set_fg(color);
    GrLine(win, gc,
           cursor_x, cursor_y - LINE_HEIGHT + 2,
           cursor_x, cursor_y);
    set_fg(FGCOLOR);
}

static void erase_cursor_only(unsigned short line_no, unsigned short col)
{
    draw_cursor_at(line_no, col, BGCOLOR);
}

static void retype_cursor_char_no_rect(unsigned short line_no, unsigned short col)
{
    char *s;
    unsigned short len;
    int x;
    int y;

    if (!line_is_visible(line_no)) {
        return;
    }

    /*
     * No rectangle clear here. This mode is for cursor movement only:
     * erase the old vertical cursor line, then redraw the character that
     * may have been crossed by that cursor line.
     */
    erase_cursor_only(line_no, col);

    s = cache_get_line(line_no);
    len = c_strlen_u(s);

    if (col < len) {
        x = LEFT_MARGIN + (int)col * CHAR_WIDTH;
        y = line_baseline_y(line_no);
        GrText(win, gc, x, y, s + col, 1, GR_TFASCII);
    }
}

#if CURSOR_REPAINT_MODE != CURSOR_REPAINT_CURSOR_ONLY && \
    CURSOR_REPAINT_MODE != CURSOR_REPAINT_RETYPE1
static void redraw_cursor_area(unsigned short line_no, unsigned short col)
{
    if (!line_is_visible(line_no)) {
        return;
    }

#if CURSOR_REPAINT_MODE == CURSOR_REPAINT_AFTER1
    {
        char *s;
        unsigned short len;
        int x;
        int y;

        x = LEFT_MARGIN + (int)col * CHAR_WIDTH;
        y = line_baseline_y(line_no);

        clear_rect(x, y - LINE_HEIGHT + 1, CHAR_WIDTH, LINE_HEIGHT);

        s = cache_get_line(line_no);
        len = c_strlen_u(s);

        if (col < len) {
            GrText(win, gc, x, y, s + col, 1, GR_TFASCII);
        }
    }
#elif CURSOR_REPAINT_MODE == CURSOR_REPAINT_AFTER2
    {
        char *s;
        unsigned short len;
        unsigned short max_count;
        int x;
        int y;

        max_count = 2;
        x = LEFT_MARGIN + (int)col * CHAR_WIDTH;
        y = line_baseline_y(line_no);

        clear_rect(x, y - LINE_HEIGHT + 1,
                   (int)max_count * CHAR_WIDTH, LINE_HEIGHT);

        s = cache_get_line(line_no);
        len = c_strlen_u(s);

        if (col < len) {
            if ((unsigned short)(col + max_count) > len) {
                max_count = (unsigned short)(len - col);
            }
            GrText(win, gc, x, y, s + col, max_count, GR_TFASCII);
        }
    }
#elif CURSOR_REPAINT_MODE == CURSOR_REPAINT_3CHARS
    {
        char *s;
        unsigned short len;
        unsigned short start_col;
        unsigned short max_count;
        int x;
        int y;

        if (col > 0) {
            start_col = (unsigned short)(col - 1);
        } else {
            start_col = 0;
        }

        max_count = 3;
        x = LEFT_MARGIN + (int)start_col * CHAR_WIDTH;
        y = line_baseline_y(line_no);

        clear_rect(x - CURSOR_REPAINT_XPAD, y - LINE_HEIGHT + 1,
                   (int)max_count * CHAR_WIDTH + 2 * CURSOR_REPAINT_XPAD,
                   LINE_HEIGHT);

        s = cache_get_line(line_no);
        len = c_strlen_u(s);

        if (start_col < len) {
            if ((unsigned short)(start_col + max_count) > len) {
                max_count = (unsigned short)(len - start_col);
            }
            GrText(win, gc, x, y, s + start_col, max_count, GR_TFASCII);
        }
    }
#elif CURSOR_REPAINT_MODE == CURSOR_REPAINT_LINE
    clear_text_row_y(line_baseline_y(line_no));
    draw_text_line_no_clear(line_no);
#endif
}
#endif

#if CURSOR_DEST_REPAIR_ENABLE
static void redraw_destination_side(unsigned short line_no, unsigned short col)
{
    char *s;
    unsigned short len;
    unsigned short start_col;
    unsigned short count;
    int x;
    int y;

    if (!line_is_visible(line_no)) {
        return;
    }

    /*
     * Repair relative to the destination cursor position.
     * With START_OFF = 1, this starts at s[col + 1], not s[col], so it
     * avoids erasing or weakening the visible cursor itself.
     */
    start_col = (unsigned short)(col + CURSOR_DEST_REPAIR_START_OFF);
    count = CURSOR_DEST_REPAIR_CHARS;

    if (start_col >= MAX_LINE_LEN || count == 0) {
        return;
    }

    x = LEFT_MARGIN + (int)start_col * CHAR_WIDTH;
    y = line_baseline_y(line_no);

    clear_rect(x - CURSOR_DEST_REPAIR_XPAD, y - LINE_HEIGHT + 1,
               (int)count * CHAR_WIDTH + 2 * CURSOR_DEST_REPAIR_XPAD,
               LINE_HEIGHT);

    s = cache_get_line(line_no);
    len = c_strlen_u(s);

    if (start_col < len) {
        if ((unsigned short)(start_col + count) > len) {
            count = (unsigned short)(len - start_col);
        }
        GrText(win, gc, x, y, s + start_col, count, GR_TFASCII);
    }
}
#endif

static void draw_cursor(void)
{
    ensure_edit_current();
    draw_cursor_at(cy, cx, FGCOLOR);
}

static void draw_status(void)
{
    char status[96];
    int y;

    y = WIN_H - 4;

    status[0] = '\0';
    strcat(status, "nxedit ELKS swap  ");
    if (current_file[0] != '\0') {
        strncat(status, current_file, sizeof(status) - strlen(status) - 1);
    } else {
        strncat(status, "(no file)", sizeof(status) - strlen(status) - 1);
    }

    GrText(win, gc, LEFT_MARGIN, y, status, strlen(status), GR_TFASCII);
}

static void redraw(void)
{
    int y;
    unsigned short line_no;
    int visible_rows;
    char *s;
    unsigned short len;
    int cursor_x;
    int cursor_y;
    int first_line;

    GrClearWindow(win, GR_FALSE);

    visible_rows = (WIN_H - TOP_MARGIN - LINE_HEIGHT - 4) / LINE_HEIGHT;
    if (visible_rows < 1) {
        visible_rows = 1;
    }

    first_line = scroll_y / LINE_HEIGHT;
    if (first_line < 0) {
        first_line = 0;
    }

    y = TOP_MARGIN;

    for (line_no = (unsigned short)first_line;
         line_no < line_count && y < WIN_H - LINE_HEIGHT;
         line_no++) {
        s = cache_get_line(line_no);
        len = c_strlen_u(s);

        if (len > 0) {
            GrText(win, gc, LEFT_MARGIN, y, s, len, GR_TFASCII);
        }

        y += LINE_HEIGHT;
    }

    ensure_edit_current();

    if (cy >= (unsigned short)first_line &&
        cy < (unsigned short)(first_line + visible_rows)) {
        cursor_x = LEFT_MARGIN + (int)cx * CHAR_WIDTH;
        cursor_y = TOP_MARGIN + ((int)cy - first_line) * LINE_HEIGHT;

        GrLine(win, gc,
               cursor_x, cursor_y - LINE_HEIGHT + 2,
               cursor_x, cursor_y + 2);
    }

    draw_status();
    GrFlush();
}

static void repaint_after_key(unsigned short old_cx, unsigned short old_cy,
                              int old_scroll_y)
{
    if (scroll_y != old_scroll_y || paint_action.mode == PAINT_FULL) {
        redraw();
        return;
    }

    switch (paint_action.mode) {
    case PAINT_CURSOR:
#if CURSOR_REPAINT_MODE == CURSOR_REPAINT_CURSOR_ONLY
        erase_cursor_only(old_cy, old_cx);
        draw_cursor();
#elif CURSOR_REPAINT_MODE == CURSOR_REPAINT_RETYPE1
        /*
         * Cursor movement path without rectangle clearing:
         * repair only the old cursor position, then draw the new cursor.
         */
        retype_cursor_char_no_rect(old_cy, old_cx);
        draw_cursor();
#else
        /* Other cursor modes may use rectangle clearing if explicitly selected. */
        redraw_cursor_area(old_cy, old_cx);
        draw_cursor();
#endif
        break;

    case PAINT_LINE_TAIL:
        draw_line_tail(paint_action.line, paint_action.col);
        draw_cursor();
#if CURSOR_DEST_REPAIR_ENABLE && CURSOR_DEST_REPAIR_AFTER_DRAW
        redraw_destination_side(cy, cx);
#endif
        break;

    case PAINT_FROM_LINE:
        draw_from_line(paint_action.line);
        draw_cursor();
#if CURSOR_DEST_REPAIR_ENABLE && CURSOR_DEST_REPAIR_AFTER_DRAW
        redraw_destination_side(cy, cx);
#endif
        break;

    case PAINT_STATUS:
        draw_status();
        draw_cursor();
#if CURSOR_DEST_REPAIR_ENABLE && CURSOR_DEST_REPAIR_AFTER_DRAW
        redraw_destination_side(cy, cx);
#endif
        break;

    case PAINT_NONE:
    default:
        break;
    }

    GrFlush();
}

static void keep_cursor_visible(void)
{
    int top_line;
    int visible_rows;

    visible_rows = (WIN_H - TOP_MARGIN - LINE_HEIGHT - 4) / LINE_HEIGHT;
    if (visible_rows < 1) {
        visible_rows = 1;
    }

    top_line = scroll_y / LINE_HEIGHT;

    if ((int)cy < top_line) {
        scroll_y = (int)cy * LINE_HEIGHT;
    } else if ((int)cy >= top_line + visible_rows) {
        scroll_y = ((int)cy - visible_rows + 1) * LINE_HEIGHT;
    }

    if (scroll_y < 0) {
        scroll_y = 0;
    }
}

/* ---------- key translation ---------- */

typedef struct {
    int code;
    int ch;
    int ctrl;
} KeyInfo;

static KeyInfo translate_key(GR_EVENT_KEYSTROKE *ks)
{
    KeyInfo k;
    int key;
    int ch;
    int mods;

    k.code = K_NONE;
    k.ch = 0;
    k.ctrl = 0;

    key = (int)ks->ch;
    ch = (int)ks->ch;
    mods = (int)ks->modifiers;

    if (mods & MWKMOD_CTRL) {
        k.ctrl = 1;
    }

    if (k.ctrl && (ch == 's' || ch == 'S' || key == 's' || key == 'S')) {
        k.code = K_SAVE;
        return k;
    }

    /*
     * Special key names vary slightly between Nano-X versions. These #ifdefs
     * allow the same file to compile against common trees.
     */
#ifdef MWKEY_LEFT
    if (key == MWKEY_LEFT) { k.code = K_LEFT; return k; }
#endif
#ifdef MWKEY_RIGHT
    if (key == MWKEY_RIGHT) { k.code = K_RIGHT; return k; }
#endif
#ifdef MWKEY_UP
    if (key == MWKEY_UP) { k.code = K_UP; return k; }
#endif
#ifdef MWKEY_DOWN
    if (key == MWKEY_DOWN) { k.code = K_DOWN; return k; }
#endif
#ifdef MWKEY_HOME
    if (key == MWKEY_HOME) { k.code = K_HOME; return k; }
#endif
#ifdef MWKEY_END
    if (key == MWKEY_END) { k.code = K_END; return k; }
#endif
#ifdef MWKEY_PAGEUP
    if (key == MWKEY_PAGEUP) { k.code = K_PGUP; return k; }
#endif
#ifdef MWKEY_PAGEDOWN
    if (key == MWKEY_PAGEDOWN) { k.code = K_PGDN; return k; }
#endif

    if (key == MWKEY_ESCAPE || ch == 27) {
        k.code = K_ESC;
    } else if (key == MWKEY_BACKSPACE || ch == 8) {
        k.code = K_BS;
    } else if (key == MWKEY_DELETE || ch == 127) {
        k.code = K_DEL;
    } else if (key == MWKEY_ENTER || ch == '\n' || ch == '\r') {
        k.code = K_ENTER;
    } else if (key == MWKEY_TAB || ch == '\t') {
        k.code = K_TAB;
    } else if (ch >= 32 && ch < 127) {
        k.code = K_TEXT;
        k.ch = ch;
    }

    return k;
}

static int handle_key(GR_EVENT_KEYSTROKE *ks)
{
    KeyInfo k;
    unsigned short old_cx;
    unsigned short old_cy;
    unsigned short old_len;
    unsigned short old_line_count;

    old_cx = cx;
    old_cy = cy;
    old_len = edit_len;
    old_line_count = line_count;

    paint_set(PAINT_NONE, cy, cx);

    k = translate_key(ks);

    switch (k.code) {
    case K_TEXT:
        insert_char(k.ch);
        if (cx != old_cx || edit_len != old_len) {
            paint_set(PAINT_LINE_TAIL, old_cy, old_cx);
        }
        break;

    case K_SAVE:
        save_file();
        paint_set(PAINT_STATUS, cy, cx);
        break;

    //FIXME arrow keys not working, currently returning ESC [ A etc
    //case K_ESC:
        //return 1;

    case K_LEFT:
        move_left();
        if (cx != old_cx || cy != old_cy) {
            paint_set(PAINT_CURSOR, cy, cx);
        }
        break;

    case K_RIGHT:
        move_right();
        if (cx != old_cx || cy != old_cy) {
            paint_set(PAINT_CURSOR, cy, cx);
        }
        break;

    case K_UP:
        move_up();
        if (cx != old_cx || cy != old_cy) {
            paint_set(PAINT_CURSOR, cy, cx);
        }
        break;

    case K_DOWN:
        move_down();
        if (cx != old_cx || cy != old_cy) {
            paint_set(PAINT_CURSOR, cy, cx);
        }
        break;

    case K_HOME:
        cx = 0;
        if (cx != old_cx) {
            paint_set(PAINT_CURSOR, cy, cx);
        }
        break;

    case K_END:
        ensure_edit_current();
        cx = edit_len;
        if (cx != old_cx) {
            paint_set(PAINT_CURSOR, cy, cx);
        }
        break;

    case K_PGUP:
        scroll_y -= LINE_HEIGHT * 8;
        if (scroll_y < 0) {
            scroll_y = 0;
        }
        paint_set(PAINT_FULL, cy, cx);
        break;

    case K_PGDN:
        scroll_y += LINE_HEIGHT * 8;
        paint_set(PAINT_FULL, cy, cx);
        break;

    case K_BS:
        if (cx > 0) {
            backspace_key();
            if (cx != old_cx || edit_len != old_len) {
                paint_set(PAINT_LINE_TAIL, old_cy,
                          (unsigned short)(old_cx - 1));
            }
        } else if (cy > 0) {
            backspace_key();
            if (line_count != old_line_count || cy != old_cy) {
                paint_set(PAINT_FROM_LINE,
                          (unsigned short)(old_cy - 1), 0);
            }
        }
        break;

    case K_DEL:
        ensure_edit_current();
        old_len = edit_len;
        if (cx < edit_len) {
            delete_key();
            if (edit_len != old_len) {
                paint_set(PAINT_LINE_TAIL, old_cy, old_cx);
            }
        } else if (cy + 1 < line_count) {
            delete_key();
            if (line_count != old_line_count) {
                paint_set(PAINT_FROM_LINE, old_cy, 0);
            }
        }
        break;

    case K_ENTER:
        if (line_count < MAX_LINES) {
            enter_key();
            if (line_count != old_line_count || cy != old_cy) {
                paint_set(PAINT_FROM_LINE, old_cy, 0);
            }
        }
        break;

    case K_TAB:
        insert_tab();
        if (cx != old_cx || edit_len != old_len) {
            paint_set(PAINT_LINE_TAIL, old_cy, old_cx);
        }
        break;

    default:
        break;
    }

    keep_cursor_visible();
    return 0;
}

/* ---------- Nano-X setup and main ---------- */

static int nx_init(void)
{
    GR_FONT_INFO    fi;
    GR_FONT_ID      regFont;

    if (GrOpen() < 0) {
        return -1;
    }

    fg_color = make_color(0, 0, 0);
    bg_color = make_color(192, 192, 192);

    regFont = GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL);
    GrGetFontInfo(regFont, &fi);
    CHAR_WIDTH = fi.maxwidth;
    LINE_HEIGHT = fi.height;
    WIN_W = COLS * CHAR_WIDTH;
    WIN_H = LINES * LINE_HEIGHT;

    win = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, TITLE, GR_ROOT_WINDOW_ID,
        -1, -1, WIN_W, WIN_H, BGCOLOR);

    if (win == 0) {
        GrClose();
        return -1;
    }

    gc = GrNewGC();
    if (gc == 0) {
        GrClose();
        return -1;
    }

    GrSetGCFont(gc, regFont);
    GrSetGCForeground(gc, FGCOLOR);
    GrSetGCBackground(gc, BGCOLOR);

    GrSelectEvents(win,
                   GR_EVENT_MASK_EXPOSURE |
                   GR_EVENT_MASK_KEY_DOWN |
                   GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(win);
    return 0;
}

int main(int argc, char **argv)
{
    GR_EVENT ev;
    int quit;
    unsigned short old_cx;
    unsigned short old_cy;
    int old_scroll_y;

    current_file[0] = '\0';

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            printf("nxedit - ELKS/Nano-X swap-backed editor\n");
            printf("Usage: nxedit [file]\n");
            printf("Controls: arrows, Home/End, PgUp/PgDn, Ctrl-S save, Esc quit\n");
            return 0;
        }

        strncpy(current_file, argv[1], sizeof(current_file) - 1);
        current_file[sizeof(current_file) - 1] = '\0';
    }

    if (swap_open_file(SWAP_PATH) < 0) {
        fprintf(stderr, "nxedit: cannot open swap file %s\n", SWAP_PATH);
        return 1;
    }

    if (current_file[0] != '\0') {
        load_file(current_file);
    } else {
        doc_new_empty();
    }

    if (nx_init() < 0) {
        fprintf(stderr, "nxedit: cannot open Nano-X\n");
        swap_close_file();
        return 1;
    }

    quit = 0;
    redraw();

    while (!quit) {
        GrGetNextEvent(&ev);

        switch (ev.type) {
        case GR_EVENT_TYPE_EXPOSURE:
            redraw();
            break;

        case GR_EVENT_TYPE_KEY_DOWN:
            old_cx = cx;
            old_cy = cy;
            old_scroll_y = scroll_y;
            quit = handle_key(&ev.keystroke);
            repaint_after_key(old_cx, old_cy, old_scroll_y);
            break;

        case GR_EVENT_TYPE_CLOSE_REQ:
            quit = 1;
            break;

        default:
            break;
        }
    }

    flush_edit_line();

    if (gc != 0) {
        GrDestroyGC(gc);
    }

    GrClose();
    swap_close_file();

    return 0;
}
