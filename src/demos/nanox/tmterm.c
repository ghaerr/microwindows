/*
 * nxterm - terminal emulator for Nano-X using tmt
 *
 * Terminal emulation handled by tmt (Tiny Main Terminal) library.
 * PTY and Nano-X integration by Microwindows team.
 *
 * Copyright (c) 2024 Microwindows Team
 */

#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "tmt.h"

/* Default terminal size */
#define STDCOL 80
#define STDROW 50
#define KBDBUF 10240
#define TITLE "nxterm"

/* Color definitions */
#define GR_COLOR_WHITESMOKE MWRGB(245, 245, 245)
#define GR_COLOR_GAINSBORO  MWRGB(220, 220, 220)

/* Standard colors */
#define stdforeground WHITE
#define stdbackground BLACK

/* Globals */
static GR_WINDOW_ID w1;
static GR_GC_ID gc1;
static GR_FONT_ID regFont;
static GR_SCREEN_INFO si;
static GR_FONT_INFO fi;

#define fonh fi.height
#define fonw fi.maxwidth

static GR_GC_INFO gi;
static GR_BOOL havefocus = GR_FALSE;
static GR_BOOL cursor_visible = GR_FALSE;
static pid_t pid;
static int termfd;
static int visualbell = 0;
static char *startprogram = NULL;
static TMT *vt = NULL;

/* Color tables for tmt color mapping */
static GR_COLOR tmt_colors[18] = {
    MWRGB(0, 0, 0),         /* BLACK */
    MWRGB(0, 0, 170),       /* BLUE */
    MWRGB(0, 170, 0),       /* GREEN */
    MWRGB(0, 170, 170),     /* CYAN */
    MWRGB(170, 0, 0),       /* RED */
    MWRGB(170, 0, 170),     /* MAGENTA */
    MWRGB(170, 85, 0),      /* BROWN */
    MWRGB(170, 170, 170),   /* LTGRAY */
    MWRGB(85, 85, 85),      /* GRAY */
    MWRGB(85, 85, 255),     /* LTBLUE */
    MWRGB(85, 255, 85),     /* LTGREEN */
    MWRGB(85, 255, 255),    /* LTCYAN */
    MWRGB(255, 85, 85),     /* LTRED */
    MWRGB(255, 85, 255),    /* LTMAGENTA */
    MWRGB(255, 255, 85),    /* YELLOW */
    MWRGB(255, 255, 255),   /* WHITE */
    0                       /* DEFAULT - filled in at init */
};

/* Forward declarations */
static void tmt_callback(tmt_msg_t m, TMT *v, const void *a, void *p);
static void draw_screen(void);
static void draw_cursor(void);
static void hide_cursor(void);
static int do_special_key(unsigned char *buffer, int key, int modifiers);

/* PTY creation */
static int term_init(void);

/* Signal handlers */
static void *mysignal(int signum, void *handler);
static void sigpipe(int sig);
static void sigchild(int sig);
static void sigquit(int sig);

/* Convert tmt color to Nano-X GR_COLOR */
static GR_COLOR
color_from_tmt(tmt_color_t c, int bold)
{
    if (c == TMT_COLOR_DEFAULT)
        return tmt_colors[16];
    if (c > 7 && c < TMT_COLOR_DEFAULT && bold)
        return tmt_colors[c];  /* bright variant */
    return tmt_colors[c & 7];
}

/* Draw a single character cell */
static void
draw_cell(int x, int y, wchar_t ch, TMTATTRS attrs)
{
    GR_COLOR fg, bg;
    int fg_idx, bg_idx;
    int bold = attrs.bold;
    int reverse = attrs.reverse;

    /* Handle default colors */
    fg_idx = (attrs.fg == TMT_COLOR_DEFAULT) ? 16 : (attrs.fg & 15);
    bg_idx = (attrs.bg == TMT_COLOR_DEFAULT) ? 17 : (attrs.bg & 15);

    fg = tmt_colors[fg_idx];
    bg = tmt_colors[bg_idx];

    /* Handle dim */
    if (attrs.dim && fg_idx < 8)
        fg_idx += 8;

    /* Handle reverse */
    if (reverse) {
        GR_COLOR tmp = fg;
        fg = bg;
        bg = tmp;
    }

    /* Handle invisible */
    if (attrs.invisible)
        fg = bg;

    /* Draw background */
    GrSetGCForeground(gc1, bg);
    GrFillRect(w1, gc1, x * fonw, y * fonh, fonw, fonh);

    /* Draw character if visible */
    if (ch != L' ' && ch != TMT_INVALID_CHAR) {
        GrSetGCForeground(gc1, fg);

        /* Convert wchar_t to UTF-8 for GrText */
        char buf[8];
        int len = 0;

        if (ch < 0x80) {
            buf[len++] = ch;
        } else if (ch < 0x800) {
            buf[len++] = 0xC0 | (ch >> 6);
            buf[len++] = 0x80 | (ch & 0x3F);
        } else if (ch < 0x10000) {
            buf[len++] = 0xE0 | (ch >> 12);
            buf[len++] = 0x80 | ((ch >> 6) & 0x3F);
            buf[len++] = 0x80 | (ch & 0x3F);
        } else {
            buf[len++] = 0xF0 | (ch >> 18);
            buf[len++] = 0x80 | ((ch >> 12) & 0x3F);
            buf[len++] = 0x80 | ((ch >> 6) & 0x3F);
            buf[len++] = 0x80 | (ch & 0x3F);
        }
        buf[len] = '\0';

        GrText(w1, gc1, x * fonw, y * fonh, buf, len, GR_TFTOP);
    }

    /* Handle underline */
    if (attrs.underline) {
        GrSetGCForeground(gc1, fg);
        GrLine(w1, gc1, x * fonw, (y + 1) * fonh - 1,
               (x + 1) * fonw - 1, (y + 1) * fonh - 1);
    }
}

/* Draw dirty region of screen */
static void
draw_screen(void)
{
    const TMTSCREEN *screen = tmt_screen(vt);
    const TMTUPDATE *update = &screen->update;
    const TMTCURSOR *cursor = tmt_cursor(vt);

    if (!update->dirty)
        return;

    /* Draw dirty region */
    for (size_t y = update->y; y < update->h && y < screen->nline; y++) {
        for (size_t x = update->x; x < update->w && x < screen->ncol; x++) {
            TMTCHAR *ch = &screen->lines[y]->chars[x];
            draw_cell(x, y, ch->c, ch->a);
            /* If we redrew the cursor cell, reset cursor state */
            if (cursor_visible && x == cursor->c && y == cursor->r) {
                cursor_visible = GR_FALSE;
            }
        }
    }

    tmt_clean(vt);
}

/* Draw cursor */
static void
draw_cursor(void)
{
    const TMTCURSOR *cursor = tmt_cursor(vt);

    if (!cursor->hidden && havefocus && !cursor_visible) {
        GrSetGCMode(gc1, GR_MODE_XOR);
        GrSetGCForeground(gc1, WHITE);
        GrFillRect(w1, gc1, cursor->c * fonw, cursor->r * fonh + 1,
                   fonw, fonh - 1);
        GrSetGCForeground(gc1, gi.foreground);
        GrSetGCMode(gc1, GR_MODE_COPY);
        cursor_visible = GR_TRUE;
    }
}

/* Hide cursor */
static void
hide_cursor(void)
{
    const TMTCURSOR *cursor = tmt_cursor(vt);

    if (cursor_visible) {
        GrSetGCMode(gc1, GR_MODE_XOR);
        GrSetGCForeground(gc1, WHITE);
        GrFillRect(w1, gc1, cursor->c * fonw, cursor->r * fonh + 1,
                   fonw, fonh - 1);
        GrSetGCForeground(gc1, gi.foreground);
        GrSetGCMode(gc1, GR_MODE_COPY);
        cursor_visible = GR_FALSE;
    }
}

/* TMT callback */
static void
tmt_callback(tmt_msg_t m, TMT *v, const void *a, void *p)
{
    /* Skip drawing if vt not yet initialized (during tmt_open) */
    if (!vt) return;
    
    switch (m) {
    case TMT_MSG_UPDATE:
        draw_screen();
        break;

    case TMT_MSG_MOVED:
        /* Cursor moved - redraw old and new positions */
        draw_screen();
        break;

    case TMT_MSG_ANSWER:
        /* Send response to terminal */
        if (a)
            write(termfd, a, strlen((const char *)a));
        break;

    case TMT_MSG_BELL:
        if (!visualbell)
            GrBell();
        break;

    case TMT_MSG_TITLE:
        /* Could update window title here */
        break;

    case TMT_MSG_CURSOR:
        /* Cursor visibility change */
        break;

    default:
        break;
    }
}

/* Map Nano-X key to ANSI escape sequence */
static int
do_special_key(unsigned char *buffer, int key, int modifiers)
{
    const char *str = NULL;
    int len = 0;

    switch (key) {
    case MWKEY_UP:      str = TMT_KEY_UP; len = strlen(str); break;
    case MWKEY_DOWN:    str = TMT_KEY_DOWN; len = strlen(str); break;
    case MWKEY_LEFT:    str = TMT_KEY_LEFT; len = strlen(str); break;
    case MWKEY_RIGHT:   str = TMT_KEY_RIGHT; len = strlen(str); break;
    case MWKEY_HOME:    str = TMT_KEY_HOME; len = strlen(str); break;
    case MWKEY_END:     str = TMT_KEY_END; len = strlen(str); break;
    case MWKEY_INSERT:  str = TMT_KEY_INSERT; len = strlen(str); break;
    case MWKEY_DELETE:  str = TMT_KEY_DELETE; len = strlen(str); break;
    case MWKEY_PAGEUP:  str = TMT_KEY_PAGE_UP; len = strlen(str); break;
    case MWKEY_PAGEDOWN: str = TMT_KEY_PAGE_DOWN; len = strlen(str); break;
    case MWKEY_BACKSPACE: str = TMT_KEY_BACKSPACE; len = strlen(str); break;
    case MWKEY_ESCAPE:  str = TMT_KEY_ESCAPE; len = strlen(str); break;

    case MWKEY_F1:  str = TMT_KEY_F1; len = strlen(str); break;
    case MWKEY_F2:  str = TMT_KEY_F2; len = strlen(str); break;
    case MWKEY_F3:  str = TMT_KEY_F3; len = strlen(str); break;
    case MWKEY_F4:  str = TMT_KEY_F4; len = strlen(str); break;
    case MWKEY_F5:  str = TMT_KEY_F5; len = strlen(str); break;
    case MWKEY_F6:  str = TMT_KEY_F6; len = strlen(str); break;
    case MWKEY_F7:  str = TMT_KEY_F7; len = strlen(str); break;
    case MWKEY_F8:  str = TMT_KEY_F8; len = strlen(str); break;
    case MWKEY_F9:  str = TMT_KEY_F9; len = strlen(str); break;
    case MWKEY_F10: str = TMT_KEY_F10; len = strlen(str); break;
    case MWKEY_F11: str = TMT_KEY_F11; len = strlen(str); break;
    case MWKEY_F12: str = TMT_KEY_F12; len = strlen(str); break;

    default:
        return 0;
    }

    if (str && len > 0) {
        memcpy(buffer, str, len);
        return len;
    }
    return 0;
}

/* Main event loop */
static void
term(void)
{
    long in, l;
    GR_EVENT_KEYSTROKE *kp;
    int bufflen;
    int gotexpose = 0;
    GR_EVENT wevent;
    unsigned char buf[KBDBUF];

    if (startprogram) {
        usleep(1000);
        write(termfd, startprogram, strlen(startprogram));
        write(termfd, "\r", 1);
    }

    while (42) {
        GrGetNextEvent(&wevent);

        switch (wevent.type) {
        case GR_EVENT_TYPE_CLOSE_REQ:
            GrClose();
            exit(0);
            break;

        case GR_EVENT_TYPE_KEY_DOWN:
            kp = (GR_EVENT_KEYSTROKE *)&wevent;
            if (kp->ch & MWKEY_NONASCII_MASK) {
                bufflen = do_special_key(buf, kp->ch, kp->modifiers);
            } else {
                buf[0] = kp->ch & 0xff;
                bufflen = 1;
            }
            if (bufflen > 0)
                write(termfd, buf, bufflen);
            break;

        case GR_EVENT_TYPE_FOCUS_IN:
            havefocus = GR_TRUE;
            draw_cursor();
            break;

        case GR_EVENT_TYPE_FOCUS_OUT:
            havefocus = GR_FALSE;
            hide_cursor();
            break;

        case GR_EVENT_TYPE_UPDATE:
            if (wevent.update.utype == GR_UPDATE_UNMAPTEMP)
                hide_cursor();
            else if (wevent.update.utype == GR_UPDATE_MOVE ||
                     wevent.update.utype == GR_UPDATE_SIZE) {
                /* Redraw on move/resize to keep content in sync */
                if (gotexpose) {
                    tmt_dirty(vt, 0, 0, tmt_screen(vt)->ncol, tmt_screen(vt)->nline);
                    draw_screen();
                    if (havefocus)
                        draw_cursor();
                }
            }
            break;

        case GR_EVENT_TYPE_EXPOSURE:
            if (!gotexpose) {
                GrRegisterInput(termfd);
                gotexpose = GR_TRUE;
            } else {
                /* Redraw entire screen from tmt buffer */
                tmt_dirty(vt, 0, 0, STDCOL, STDROW);
                draw_screen();
                if (havefocus)
                    draw_cursor();
            }
            break;

        case GR_EVENT_TYPE_FDINPUT:
            if (!gotexpose)
                break;
            if ((in = read(termfd, buf, sizeof(buf))) > 0) {
                hide_cursor();
                tmt_write(vt, (char *)buf, in);
                if (havefocus)
                    draw_cursor();
            }
            break;

        case GR_EVENT_TYPE_NONE:
        case GR_EVENT_TYPE_TIMEOUT:
            break;

        default:
            break;
        }
    }
}

/* Signal handlers */
static void *
mysignal(int signum, void *handler)
{
    struct sigaction sa, so;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(signum, &sa, &so);
    return so.sa_handler;
}

static void
sigpipe(int sig)
{
    GrClose();
    kill(-pid, SIGHUP);
    _exit(sig);
}

static void
sigchild(int sig)
{
    GrClose();
    _exit(sig);
}

static void
sigquit(int sig)
{
    signal(sig, SIG_IGN);
    GrClose();
    kill(-pid, SIGHUP);
}

int
main(int argc, char **argv)
{
    GR_CURSOR_ID c1;
    GR_BITMAP bitmap1fg[7];
    GR_BITMAP bitmap1bg[7];
    int col = STDCOL;
    int row = STDROW;

    argv++;
    while (*argv && **argv == '-') {
        switch (*(*argv + 1)) {
        case 'v':
            visualbell = 1;
            argv++;
            break;
        default:
            fprintf(stderr, "usage: nxterm [-v] [command]\n");
            exit(1);
        }
    }

    if (*argv)
        startprogram = *argv++;

    if (GrOpen() < 0) {
        GrError("cannot open graphics\n");
        exit(1);
    }
    GrGetScreenInfo(&si);

    /* Create font and get metrics */
    regFont = GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL);
    GrGetFontInfo(regFont, &fi);

    int winw = col * fi.maxwidth;
    int winh = row * fi.height;

    /* Create window */
    w1 = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, TITLE, GR_ROOT_WINDOW_ID,
                       -1, -1, winw, winh, stdbackground);

    GrSelectEvents(w1, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_EXPOSURE |
                       GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_FOCUS_IN |
                       GR_EVENT_MASK_FOCUS_OUT | GR_EVENT_MASK_UPDATE |
                       GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(w1);

    /* Setup GC */
    gc1 = GrNewGC();
    GrSetGCFont(gc1, regFont);
    GrSetGCForeground(gc1, stdforeground);
    GrSetGCBackground(gc1, stdbackground);
    GrGetGCInfo(gc1, &gi);

    /* Setup cursor */
#define _ ((unsigned)0)
#define X ((unsigned)1)
#define MASK7(a, b, c, d, e, f, g) \
    (((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) + e) * 2) + f) * 2) + g) << 9)

    bitmap1fg[0] = MASK7(_, _, X, _, X, _, _);
    bitmap1fg[1] = MASK7(_, _, _, X, _, _, _);
    bitmap1fg[2] = MASK7(_, _, _, X, _, _, _);
    bitmap1fg[3] = MASK7(_, _, _, X, _, _, _);
    bitmap1fg[4] = MASK7(_, _, _, X, _, _, _);
    bitmap1fg[5] = MASK7(_, _, _, X, _, _, _);
    bitmap1fg[6] = MASK7(_, _, X, _, X, _, _);

    bitmap1bg[0] = MASK7(_, X, X, X, X, X, _);
    bitmap1bg[1] = MASK7(_, _, X, X, X, _, _);
    bitmap1bg[2] = MASK7(_, _, X, X, X, _, _);
    bitmap1bg[3] = MASK7(_, _, X, X, X, _, _);
    bitmap1bg[4] = MASK7(_, _, X, X, X, _, _);
    bitmap1bg[5] = MASK7(_, _, X, X, X, _, _);
    bitmap1bg[6] = MASK7(_, X, X, X, X, X, _);

    c1 = GrNewCursor(7, 7, 3, 3, stdforeground, stdbackground,
                     bitmap1fg, bitmap1bg);
    GrSetWindowCursor(w1, c1);

    /* Setup terminal colors - 16 standard terminal colors */
    tmt_colors[0] = MWRGB(0, 0, 0);         /* black */
    tmt_colors[1] = MWRGB(170, 0, 0);       /* red */
    tmt_colors[2] = MWRGB(0, 170, 0);       /* green */
    tmt_colors[3] = MWRGB(170, 85, 0);      /* brown/orange */
    tmt_colors[4] = MWRGB(0, 0, 170);       /* blue */
    tmt_colors[5] = MWRGB(170, 0, 170);     /* magenta */
    tmt_colors[6] = MWRGB(0, 170, 170);     /* cyan */
    tmt_colors[7] = MWRGB(170, 170, 170);   /* light gray */
    tmt_colors[8] = MWRGB(85, 85, 85);      /* dark gray */
    tmt_colors[9] = MWRGB(255, 0, 0);       /* bright red */
    tmt_colors[10] = MWRGB(0, 255, 0);      /* bright green */
    tmt_colors[11] = MWRGB(255, 255, 0);    /* yellow */
    tmt_colors[12] = MWRGB(0, 0, 255);      /* bright blue */
    tmt_colors[13] = MWRGB(255, 0, 255);    /* bright magenta */
    tmt_colors[14] = MWRGB(0, 255, 255);    /* bright cyan */
    tmt_colors[15] = MWRGB(255, 255, 255);  /* white */
    tmt_colors[16] = stdforeground;         /* default foreground */
    tmt_colors[17] = stdbackground;         /* default background */

    /* Set environment for child process */
    char sbuf[32];
    sprintf(sbuf, "%d", col);
    setenv("COLUMNS", sbuf, 1);
    sprintf(sbuf, "%d", row);
    setenv("LINES", sbuf, 1);
    setenv("TERM", "xterm", 1);

    /* Create PTY */
    termfd = term_init();
    if (termfd < 0) {
        GrClose();
        exit(1);
    }

    /* Setup signal handlers */
    mysignal(SIGTERM, sigquit);
    mysignal(SIGHUP, sigquit);
    mysignal(SIGQUIT, sigquit);
    mysignal(SIGPIPE, sigpipe);
    mysignal(SIGCHLD, sigchild);
    mysignal(SIGINT, SIG_IGN);
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif

    /* Initialize tmt */
    vt = tmt_open(row, col, tmt_callback, NULL, NULL);
    if (!vt) {
        fprintf(stderr, "tmt_open failed\n");
        GrClose();
        exit(1);
    }

    /* Register PTY for input */
    GrRegisterInput(termfd);

    /* Run terminal */
    term();

    return 0;
}

/* PTY creation for different platforms */
#if ELKS
static char *nargv[2] = {"/bin/sh", NULL};

static int
term_init(void)
{
    int tfd;
    pid_t pid;
    int n = 0;
    char pty_name[12];

again:
    strcpy(pty_name, "/dev/ptyp");
    strcat(pty_name, itoa(n));
    if ((tfd = open(pty_name, O_RDWR | O_NONBLOCK)) < 0) {
        if (errno == EBUSY && n++ < 3)
            goto again;
        GrError("Can't create %s\n", pty_name);
        return -1;
    }
    if ((pid = fork()) == -1) {
        GrError("No processes\n");
        return -1;
    }
    if (!pid) {
        setsid();
        close(tfd);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        pty_name[5] = 't';
        if ((tfd = open(pty_name, O_RDWR)) < 0) {
            GrError("Can't open %s\n", pty_name);
            return -1;
        }
        close(STDERR_FILENO);
        dup2(tfd, STDIN_FILENO);
        dup2(tfd, STDOUT_FILENO);
        dup2(tfd, STDERR_FILENO);
        execv(nargv[0], nargv);
        exit(1);
    }
    return tfd;
}

#elif defined(__APPLE__) || defined(__FreeBSD__)
#ifdef __APPLE__
#include <util.h>
#else
#include <libutil.h>
#endif

static char *nargv[2] = {"/bin/sh", NULL};

static int
term_init(void)
{
    int master, slave;
    struct winsize winsz;

    winsz.ws_col = STDCOL;
    winsz.ws_row = STDROW;
    winsz.ws_xpixel = 0;
    winsz.ws_ypixel = 0;
    if (openpty(&master, &slave, NULL, NULL, &winsz) < 0) {
        GrError("Can't create pty\n");
        return -1;
    }
    fcntl(master, F_SETFL, fcntl(master, F_GETFL) | O_NONBLOCK);
    if (!fork()) {
        setsid();
        close(master);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(slave, STDIN_FILENO);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);
        if (slave > STDERR_FILENO)
            close(slave);
        execv(nargv[0], nargv);
        exit(1);
    }
    close(slave);
    return master;
}

#else
/* Linux / UNIX98 - new style PTY */
static char *nargv[2] = {"/bin/sh", NULL};

static int
term_init(void)
{
    int tfd;
    pid_t pid;
    char ptyname[50];
    struct winsize winsz;

    winsz.ws_col = STDCOL;
    winsz.ws_row = STDROW;
    winsz.ws_xpixel = 0;
    winsz.ws_ypixel = 0;

    tfd = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (tfd < 0) {
        GrError("Can't create pty /dev/ptmx\n");
        return -1;
    }
    strcpy(ptyname, ptsname(tfd));

    signal(SIGCHLD, SIG_DFL);
    grantpt(tfd);
    unlockpt(tfd);

    if ((pid = fork()) == -1) {
        GrError("No processes\n");
        return -1;
    }
    if (!pid) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(tfd);

        setsid();
        if ((tfd = open(ptyname, O_RDWR)) < 0) {
            GrError("Can't open %s\n", ptyname);
            exit(1);
        }

        ioctl(tfd, TIOCSWINSZ, &winsz);

        close(STDERR_FILENO);
        dup2(tfd, STDIN_FILENO);
        dup2(tfd, STDOUT_FILENO);
        dup2(tfd, STDERR_FILENO);
        execv(nargv[0], nargv);
        exit(1);
    }
    return tfd;
}
#endif
