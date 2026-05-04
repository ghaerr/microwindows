/*
 * NXEDIT - ELKS/Nano-X swap-backed text editor
 *
 * This version is designed for ELKS-class 16-bit systems using Nano-X.
 *
 * Swap is NOT written on every edit operation.
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
 * They simply become unreachable.
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
 * BUILD SHAPE
 * -----------
 *   ia16-elf-gcc -melks -Os -fno-builtin nxedit.c -lnano-X -o nxedit
 *
 * Nano-X key names vary between trees. If arrows/page keys do not compile,
 * adjust translate_key() only.
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
#define FGCOLOR         BLACK
#define BGCOLOR         LTGRAY

#define LEFT_MARGIN     6
#define TOP_MARGIN      14

#define BLOCK_MAGIC     0xBA57U
#define BLOCK_VALID     0x0001U

/* ---------- compatibility fallbacks ---------- */

/*
 * Some Nano-X trees use MWKEY_* and MWKMOD_* names. If your ELKS headers
 * differ, fix these aliases here rather than throughout the editor.
 */
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

    k = translate_key(ks);

    switch (k.code) {
    case K_TEXT:
        insert_char(k.ch);
        break;

    case K_SAVE:
        save_file();
        break;

    //FIXME arrow keys not working, currently returning ESC [ A etc
    //case K_ESC:
        //return 1;

    case K_LEFT:
        move_left();
        break;

    case K_RIGHT:
        move_right();
        break;

    case K_UP:
        move_up();
        break;

    case K_DOWN:
        move_down();
        break;

    case K_HOME:
        cx = 0;
        break;

    case K_END:
        ensure_edit_current();
        cx = edit_len;
        break;

    case K_PGUP:
        scroll_y -= LINE_HEIGHT * 8;
        if (scroll_y < 0) {
            scroll_y = 0;
        }
        break;

    case K_PGDN:
        scroll_y += LINE_HEIGHT * 8;
        break;

    case K_BS:
        backspace_key();
        break;

    case K_DEL:
        delete_key();
        break;

    case K_ENTER:
        enter_key();
        break;

    case K_TAB:
        insert_tab();
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

    if (GrOpen() < 0) {
        return -1;
    }

    GR_FONT_ID regFont = GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL);
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
            quit = handle_key(&ev.keystroke);
            redraw();
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
