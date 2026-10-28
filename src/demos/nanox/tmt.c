/* Copyright (c) 2017 Rob King
 * All rights reserved.
 * Sep 2025 Enhancements for GFX library by Greg Haerr
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the copyright holder nor the
 *     names of contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS,
 * COPYRIGHT HOLDERS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tmt.h"

#define PAR_MAX 8
#define TITLE_MAX 31
#define TAB 8
#define MAX(x, y) (((size_t)(x) > (size_t)(y)) ? (size_t)(x) : (size_t)(y))
#define MIN(x, y) (((size_t)(x) < (size_t)(y)) ? (size_t)(x) : (size_t)(y))
#define CLINE(vt) (vt)->screen.lines[MIN((vt)->curs.r, (vt)->screen.nline - 1)]
#define CLINENO(vt) (MIN((vt)->curs.r, (vt)->screen.nline - 1))

#define SCR_DEF ((size_t)-1)

#define P0(x) (vt->pars[x])
#define P1(x) (vt->pars[x]? vt->pars[x] : 1)
#define CB(vt, m, a) ((vt)->cb? (vt)->cb(m, vt, a, (vt)->p) : (void)0)

#define COMMON_VARS             \
    TMTSCREEN *s = &vt->screen; \
    TMTCURSOR *c = &vt->curs;    \
    TMTLINE *l = CLINE(vt);     \
    TMTCHAR *t = vt->tabs->chars

#define HANDLER(name) static void name (TMT *vt) { COMMON_VARS;

struct TMT{
    enum {S_NUL, S_ESC, S_ARG, S_TITLE, S_TITLE_ARG, S_GT_ARG, S_LPAREN, S_RPAREN} state;

    bool acs, ignored, XN, q;
    bool unicode_to_acs;

    TMTSCREEN screen;
    TMTCURSOR curs, oldcurs;
    TMTATTRS attrs, oldattrs;
    TMTLINE *tabs;
    size_t minline;
    size_t maxline;

    TMTCALLBACK cb;
    void *p;
    const wchar_t *acschars;

    int charset;  // Are we in G0 or G1?
    int xlate[2]; // What's in the charset?  0=ASCII, 1=DEC Special Graphics

    Mbstate_t ms;
    size_t nmb;
    char mb[MB_LEN_MAX];

    char title[TITLE_MAX + 1];
    size_t ntitle;

    size_t pars[PAR_MAX];
    size_t npar;
    size_t arg;
};


static TMTATTRS defattrs = {.fg = TMT_COLOR_DEFAULT, .bg = TMT_COLOR_DEFAULT};
static void writecharatcurs(TMT *vt, wchar_t w);

bool
tmt_unicode_to_acs(TMT *vt, bool v)
{
    bool r = vt->unicode_to_acs;
    vt->unicode_to_acs = v;
    return r;
}

void
tmt_dirty(TMT *vt, size_t x, size_t y, size_t w, size_t h)
{
    TMTSCREEN *s = &vt->screen;

    s->update.dirty = true;
    s->update.x = MIN(x, s->update.x);
    s->update.y = MIN(y, s->update.y);
    s->update.w = MAX(s->update.w, x+w);
    s->update.h = MAX(s->update.h, y+h);
}

void
tmt_clean(TMT *vt)
{
    TMTSCREEN *s = &vt->screen;

    s->update.dirty = false;
    s->update.x = s->update.y = 32767;
    s->update.w = s->update.h = 0;
}

static void
dirtylines(TMT *vt, size_t s, size_t e)
{
   tmt_dirty(vt, 0, s, vt->screen.ncol, e-s);
}

static void
clearline(TMT *vt, TMTLINE *l, size_t s, size_t e)
{
    e = MIN(e, vt->screen.ncol);
    for (size_t i = s; i < e; i++){
        l->chars[i].a = vt->attrs;
        l->chars[i].c = L' ';
    }
    tmt_dirty(vt, s, CLINENO(vt), e-s, 1);
}

static void
clearlines(TMT *vt, size_t r, size_t n)
{
    for (size_t i = r; i < r + n && i < vt->screen.nline; i++)
        clearline(vt, vt->screen.lines[i], 0, vt->screen.ncol);
}

static void
scrup(TMT *vt, size_t r, ssize_t n)
{
    if (r == SCR_DEF) r = vt->minline;
    n = MIN(n, vt->maxline - r);

    if (n>0){
        TMTLINE *buf[n];
        if (r == 0) {
            for (int i = 0; i < n; ++i)
                CB(vt, TMT_MSG_SCROLL, &vt->screen.lines[i]->chars);
        }

        memcpy(buf, vt->screen.lines + r, n * sizeof(TMTLINE *));
        memmove(vt->screen.lines + r, vt->screen.lines + r + n,
                (vt->maxline - n - r + 1) * sizeof(TMTLINE *));
        memcpy(vt->screen.lines + (vt->maxline - n + 1),
               buf, n * sizeof(TMTLINE *));

        clearlines(vt, vt->maxline - n + 1, n);
        dirtylines(vt, r, vt->maxline+1);
    }
}

static void
scrdn(TMT *vt, size_t r, ssize_t n)
{
    if (r == SCR_DEF) r = vt->minline;
    n = MIN(n, vt->maxline - r);

    if (n>0){
        TMTLINE *buf[n];

        memcpy(buf, vt->screen.lines + (vt->maxline - n + 1),
               n * sizeof(TMTLINE *));
        memmove(vt->screen.lines + r + n, vt->screen.lines + r,
                (vt->maxline - n - r + 1) * sizeof(TMTLINE *));
        memcpy(vt->screen.lines + r, buf, n * sizeof(TMTLINE *));

        clearlines(vt, r, n);
        dirtylines(vt, r, vt->maxline+1);
    }
}

HANDLER(ed)
    size_t b = 0;
    size_t e = s->nline;

    switch (P0(0)){
        case 0: b = c->r + 1; clearline(vt, l, c->c, vt->screen.ncol); break;
        case 1: e = c->r    ; clearline(vt, l, 0, c->c);               break;
        case 2:  /* use defaults */                                    break;
        default: /* do nothing   */                                    return;
    }

    clearlines(vt, b, e - b);
}

HANDLER(ich)
    size_t n = P1(0); /* XXX use MAX */
    if (n > s->ncol - c->c - 1) n = s->ncol - c->c - 1;

    memmove(l->chars + c->c + n, l->chars + c->c,
            MIN(s->ncol - 1 - c->c,
            (s->ncol - c->c - n - 1)) * sizeof(TMTCHAR));
    clearline(vt, l, c->c, n);
}

HANDLER(dch)
    size_t n = P1(0); /* XXX use MAX */
    if (n > s->ncol - c->c) n = s->ncol - c->c;
    else if (n == 0) return;

    TMTATTRS oldattr = vt->attrs;
    vt->attrs = (l->chars + s->ncol - n)->a;

    memmove(l->chars + c->c, l->chars + c->c + n,
            (s->ncol - c->c - n) * sizeof(TMTCHAR));

    clearline(vt, l, s->ncol - n, s->ncol);
    vt->attrs = oldattr;
    /* VT102 manual says the attribute for the newly empty characters
     * should be the same as the last character moved left, which isn't
     * what clearline() currently does.
     */
}

HANDLER(el)
    switch (P0(0)){
        case 0: clearline(vt, l, c->c, vt->screen.ncol);         break;
        case 1: clearline(vt, l, 0, MIN(c->c + 1, s->ncol - 1)); break;
        case 2: clearline(vt, l, 0, vt->screen.ncol);            break;
    }
}

HANDLER(sgr)
    for (size_t i = 0; i < vt->npar; i++)
        switch (P0(i)) {
        case  0: vt->attrs                    = defattrs;   break;
        case  1: case 22: vt->attrs.bold      = P0(i) < 20; break;
        case  2: case 23: vt->attrs.dim       = P0(i) < 20; break;
        case  4: case 24: vt->attrs.underline = P0(i) < 20; break;
        case  5: case 25: vt->attrs.blink     = P0(i) < 20; break;
        case  7: case 27: vt->attrs.reverse   = P0(i) < 20; break;
        case  8: case 28: vt->attrs.invisible = P0(i) < 20; break;
        case 10: case 11: vt->acs             = P0(i) > 10; break;

        case 30: vt->attrs.fg = TMT_COLOR_BLACK;            break;
        case 31: vt->attrs.fg = TMT_COLOR_RED;              break;
        case 32: vt->attrs.fg = TMT_COLOR_GREEN;            break;
        case 33: vt->attrs.fg = TMT_COLOR_BROWN;            break;
        case 34: vt->attrs.fg = TMT_COLOR_BLUE;             break;
        case 35: vt->attrs.fg = TMT_COLOR_MAGENTA;          break;
        case 36: vt->attrs.fg = TMT_COLOR_CYAN;             break;
        case 37: vt->attrs.fg = TMT_COLOR_LTGRAY;           break;
        case 39: vt->attrs.fg = TMT_COLOR_DEFAULT;          break;

        case 40: vt->attrs.bg = TMT_COLOR_BLACK;            break;
        case 41: vt->attrs.bg = TMT_COLOR_RED;              break;
        case 42: vt->attrs.bg = TMT_COLOR_GREEN;            break;
        case 43: vt->attrs.bg = TMT_COLOR_BROWN;            break;
        case 44: vt->attrs.bg = TMT_COLOR_BLUE;             break;
        case 45: vt->attrs.bg = TMT_COLOR_MAGENTA;          break;
        case 46: vt->attrs.bg = TMT_COLOR_CYAN;             break;
        case 47: vt->attrs.bg = TMT_COLOR_LTGRAY;           break;
        case 49: vt->attrs.bg = TMT_COLOR_DEFAULT;          break;

        case 90: vt->attrs.fg = TMT_COLOR_GRAY;             break;
        case 91: vt->attrs.fg = TMT_COLOR_LTRED;            break;
        case 92: vt->attrs.fg = TMT_COLOR_LTGREEN;          break;
        case 93: vt->attrs.fg = TMT_COLOR_YELLOW;           break;
        case 94: vt->attrs.fg = TMT_COLOR_LTBLUE;           break;
        case 95: vt->attrs.fg = TMT_COLOR_LTMAGENTA;        break;
        case 96: vt->attrs.fg = TMT_COLOR_LTCYAN;           break;
        case 97: vt->attrs.fg = TMT_COLOR_WHITE;            break;

        case 100: vt->attrs.bg = TMT_COLOR_GRAY;            break;
        case 101: vt->attrs.bg = TMT_COLOR_LTRED;           break;
        case 102: vt->attrs.bg = TMT_COLOR_LTGREEN;         break;
        case 103: vt->attrs.bg = TMT_COLOR_YELLOW;          break;
        case 104: vt->attrs.bg = TMT_COLOR_LTBLUE;          break;
        case 105: vt->attrs.bg = TMT_COLOR_LTMAGENTA;       break;
        case 106: vt->attrs.bg = TMT_COLOR_LTCYAN;          break;
        case 107: vt->attrs.bg = TMT_COLOR_WHITE;           break;
    }
}

HANDLER(rep)
    if (!c->c) return;
    wchar_t r = l->chars[c->c - 1].c;
    for (size_t i = 0; i < P1(0); i++)
        writecharatcurs(vt, r);
}

HANDLER(dsr)
    char r[32];
    snprintf(r, sizeof(r), "\033[%d;%dR", (int)c->r + 1, (int)c->c + 1);
    CB(vt, TMT_MSG_ANSWER, (const char *)r);
}

HANDLER(resetparser)
    memset(vt->pars, 0, sizeof(vt->pars));
    vt->q = vt->ignored = false;
    vt->state = vt->npar = vt->arg = vt->ntitle = 0;
}

HANDLER(consumearg)
    if (vt->npar < PAR_MAX)
        vt->pars[vt->npar++] = vt->arg;
    vt->arg = 0;
}

HANDLER(fixcursor)
    c->r = MIN(c->r, s->nline - 1);
    c->c = MIN(c->c, s->ncol - 1);
}

HANDLER(sm)
    switch (P0(0)){
        case 25:
            if (vt->q) {
                vt->curs.hidden = false;
                CB(vt, TMT_MSG_CURSOR, "t");
            }
            break;
        default:
            for (int i = vt->npar; i < PAR_MAX; ++i)
                vt->pars[i] = (size_t)-1;
            CB(vt, TMT_MSG_SETMODE, &vt->pars[0]);
            break;
    }
}

HANDLER(rm)
    switch (P0(0)){
        case 25:
            if (vt->q) {
                vt->curs.hidden = true;
                CB(vt, TMT_MSG_CURSOR, "f");
            }
            break;
        default:
            for (int i = vt->npar; i < PAR_MAX; ++i)
                vt->pars[i] = (size_t)-1;
            CB(vt, TMT_MSG_UNSETMODE, &vt->pars[0]);
            break;
    }
}

HANDLER(title)
    if (vt->npar >= 1 && (P0(0) == 0 || P0(0) == 2))
        CB(vt, TMT_MSG_TITLE, vt->title);
}

static void
reverse_nl(TMT *vt)
{
    COMMON_VARS;

    vt->XN = false;
    if (c->r == vt->minline)
        scrdn(vt, SCR_DEF, 1);
    else if (c->r > 0)
        c->r--;
}

static void
nl(TMT *vt)
{
    COMMON_VARS;

    vt->XN = false;
    if (c->r == vt->maxline)
        scrup(vt, SCR_DEF, 1);
    else if (c->r < (s->nline-1))
        c->r++;
}

static void
margin(TMT *vt, size_t top, size_t bot)
{
    if (top >= bot) return;
    if (bot >= vt->screen.nline) return;
    vt->minline = top;
    vt->maxline = bot;
}

static bool
handlechar(TMT *vt, unsigned int i)
{
    COMMON_VARS;
    char cs[2];

    #define ON(S, C, A) if (vt->state == (S) && strchr(C, i)){ A; return true;}
    #define DO(S, C, A) ON(S, C, consumearg(vt); if (!vt->ignored) {A;} \
                                 fixcursor(vt); resetparser(vt););

    DO(S_NUL, "\x07",       CB(vt, TMT_MSG_BELL, NULL))
    DO(S_NUL, "\x08",       vt->XN = false; if (c->c) c->c--)
    DO(S_NUL, "\x09",       while (++c->c < s->ncol - 1 && t[c->c].c != L'*'))
    DO(S_NUL, "\x0a",       nl(vt))
    DO(S_NUL, "\x0d",       vt->XN = false; c->c = 0)
    DO(S_NUL, "\x0e",       vt->charset = 1) // ^N Shift Out (Switch to G1)
    DO(S_NUL, "\x0f",       vt->charset = 0) // ^O Shift In  (Switch to G0)
    ON(S_NUL, "\x1b",       vt->state = S_ESC)
    ON(S_ESC, "\x1b",       vt->state = S_ESC)
    DO(S_ESC, "=",          (void)0) // DECKPAM (application keypad)
    DO(S_ESC, ">",          (void)0) // DECKPNM (normal keypad)
    ON(S_ESC, "+*",         vt->ignored = true; vt->state = S_ARG)
    ON(S_ESC, "[",          vt->state = S_ARG)
    ON(S_ESC, "]",          vt->state = S_TITLE_ARG)
    ON(S_ESC, "(",          vt->state = S_LPAREN)
    ON(S_ESC, ")",          vt->state = S_RPAREN)
    DO(S_ESC, "7",          vt->oldcurs = vt->curs; vt->oldattrs = vt->attrs)
    DO(S_ESC, "8",          vt->curs = vt->oldcurs; vt->attrs = vt->oldattrs)
    DO(S_ESC, "c",          tmt_reset(vt))
    DO(S_ESC, "H",          t[c->c].c = L'*')
    DO(S_ESC, "M",          reverse_nl(vt))
    ON(S_ARG, "\x1b",       vt->state = S_ESC)
    ON(S_ARG, ";",          consumearg(vt))
    ON(S_ARG, "?",          vt->q = 1)
    ON(S_ARG, "0123456789", cs[0] = i; cs[1] = '\0'; vt->arg = vt->arg * 10 + atoi(cs))
    ON(S_TITLE_ARG, "012",  cs[0] = i; cs[1] = '\0'; vt->arg = vt->arg * 10 + atoi(cs))
    ON(S_TITLE_ARG, ";",    consumearg(vt); vt->state = S_TITLE)
    DO(S_ARG, "@",          ich(vt))
    DO(S_ARG, "A",          c->r = MAX(c->r - P1(0), 0))
    DO(S_ARG, "B",          c->r = MIN(c->r + P1(0), s->nline - 1))
    DO(S_ARG, "C",          c->c = MIN(c->c + P1(0), s->ncol - 1))
    DO(S_ARG, "D",          c->c = MIN(c->c - P1(0), c->c))
    DO(S_ARG, "E",          c->c = 0; c->r = MIN(c->r + P1(0), s->nline - 1))
    DO(S_ARG, "F",          c->c = 0; c->r = MAX(c->r - P1(0), 0))
    DO(S_ARG, "G",          c->c = MIN(P1(0) - 1, s->ncol - 1))
    DO(S_ARG, "d",          c->r = MIN(P1(0) - 1, s->nline - 1))
    DO(S_ARG, "r",          margin(vt, P1(0)-1, P1(1)-1))
    DO(S_ARG, "Hf",         vt->XN = false; c->r = P1(0) - 1; c->c = P1(1) - 1)
    DO(S_ARG, "I",          while (++c->c < s->ncol - 1 && t[c->c].c != L'*'))
    DO(S_ARG, "J",          ed(vt))
    DO(S_ARG, "K",          el(vt))
    DO(S_ARG, "L",          scrdn(vt, c->r, P1(0)))
    DO(S_ARG, "M",          scrup(vt, c->r, P1(0)))
    DO(S_ARG, "P",          dch(vt))
    DO(S_ARG, "S",          scrup(vt, SCR_DEF, P1(0)))
    DO(S_ARG, "T",          scrdn(vt, SCR_DEF, P1(0)))
    DO(S_ARG, "X",          clearline(vt, l, c->c, c->c+P1(0)))
    DO(S_ARG, "Z",          while (c->c && t[--c->c].c != L'*'))
    DO(S_ARG, "b",          rep(vt));
    DO(S_ARG, "c",          if (!vt->q) CB(vt, TMT_MSG_ANSWER, "\033[?6c"))
    DO(S_ARG, "g",          if (P0(0) == 3) clearline(vt, vt->tabs, 0, s->ncol))
    DO(S_ARG, "h",          sm(vt))
    DO(S_ARG, "i",          (void)0)
    DO(S_ARG, "l",          rm(vt))
    DO(S_ARG, "m",          sgr(vt))
    DO(S_ARG, "n",          if (P0(0) == 6) dsr(vt))
    DO(S_ARG, "s",          vt->oldcurs = vt->curs; vt->oldattrs = vt->attrs)
    DO(S_ARG, "u",          vt->curs = vt->oldcurs; vt->attrs = vt->oldattrs)
    ON(S_ARG, ">",          vt->state = S_GT_ARG)
    DO(S_GT_ARG, "c",       CB(vt, TMT_MSG_ANSWER, "\033[>0;95c")) // VT100;xterm
    DO(S_GT_ARG, "q",       CB(vt, TMT_MSG_ANSWER, "\033P>|" "XTerm(354)" "\033\\"))
    DO(S_TITLE, "\x07",     vt->title[vt->ntitle] = '\0'; title(vt))
    DO(S_LPAREN, "AB12",    vt->xlate[0] = 0)
    DO(S_LPAREN, "0",       vt->xlate[0] = 1)
    DO(S_RPAREN, "AB12",    vt->xlate[1] = 0)
    DO(S_RPAREN, "0",       vt->xlate[1] = 1)
    if (vt->state == S_TITLE && (i >= ' ' && vt->ntitle < TITLE_MAX)) {
        vt->title[vt->ntitle++] = i;
        return true;
    }

    return resetparser(vt), false;
}

static void
notify(TMT *vt, bool update, bool moved)
{
    if (update) CB(vt, TMT_MSG_UPDATE, &vt->screen);
    if (moved) CB(vt, TMT_MSG_MOVED, &vt->curs);
}

static TMTLINE *
allocline(TMT *vt, TMTLINE *o, size_t n, size_t pc)
{
    TMTLINE *l = realloc(o, sizeof(TMTLINE) + n * sizeof(TMTCHAR));
    if (!l) return NULL;

    clearline(vt, l, pc, n);
    return l;
}

static void
freelines(TMT *vt, size_t s, size_t n, bool screen)
{
    for (size_t i = s; vt->screen.lines && i < s + n; i++){
        free(vt->screen.lines[i]);
        vt->screen.lines[i] = NULL;
    }
    if (screen) free(vt->screen.lines);
}

TMT *
tmt_open(size_t nline, size_t ncol, TMTCALLBACK cb, void *p,
         const wchar_t *acs)
{
    TMT *vt = calloc(1, sizeof(TMT));
    if (!nline || !ncol || !vt) return free(vt), NULL;

    /* ASCII-safe defaults for box-drawing characters. */
    vt->acschars = acs? acs : L"><^v#+:o##+++++~---_++++|<>*!fo";
    vt->cb = cb;
    vt->p = p;
    vt->attrs = vt->oldattrs = defattrs;

    if (!tmt_resize(vt, nline, ncol)) return tmt_close(vt), NULL;
    return vt;
}

void
tmt_close(TMT *vt)
{
    free(vt->tabs);
    freelines(vt, 0, vt->screen.nline, true);
    free(vt);
}

bool
tmt_resize(TMT *vt, size_t nline, size_t ncol)
{
    if (nline < 2 || ncol < 2) return false;
    if (nline < vt->screen.nline)
        freelines(vt, nline, vt->screen.nline - nline, false);

    TMTLINE **l = realloc(vt->screen.lines, nline * sizeof(TMTLINE *));
    if (!l) return false;

    size_t pc = vt->screen.ncol;
    vt->screen.lines = l;
    vt->screen.ncol = ncol;
    for (size_t i = 0; i < nline; i++){
        TMTLINE *nl = NULL;
        if (i >= vt->screen.nline)
            nl = vt->screen.lines[i] = allocline(vt, NULL, ncol, 0);
        else
            nl = allocline(vt, vt->screen.lines[i], ncol, pc);

        if (!nl) return false;
        vt->screen.lines[i] = nl;
    }
    vt->screen.nline = nline;

    // We reset this.  Maybe we're supposed to maintain it?  Hopefully
    // anything that needs it will reset it in response to SIGWNCH?
    vt->minline = 0;
    vt->maxline = nline-1;

    vt->tabs = allocline(vt, vt->tabs, ncol, 0);
    if (!vt->tabs) return free(l), false;
    vt->tabs->chars[0].c = vt->tabs->chars[ncol - 1].c = L'*';
    for (size_t i = 0; i < ncol; i++) if (i % TAB == 0)
        vt->tabs->chars[i].c = L'*';

    fixcursor(vt);
    tmt_clean(vt);
    dirtylines(vt, 0, nline);
    notify(vt, true, true);
    return true;
}

static wchar_t
tacs(const TMT *vt, unsigned int c)
{
    /* The terminfo alternate character set for ANSI. */
    static unsigned char map[] = {0020U, 0021U, 0030U, 0031U, 0333U, 0004U,
                                  0261U, 0370U, 0361U, 0260U, 0331U, 0277U,
                                  0332U, 0300U, 0305U, 0176U, 0304U, 0304U,
                                  0304U, 0137U, 0303U, 0264U, 0301U, 0302U,
                                  0263U, 0363U, 0362U, 0343U, 0330U, 0234U,
                                  0376U};
    for (size_t i = 0; i < sizeof(map); i++) {
        if (map[i] == c)
            return vt->acschars[i];
    }
    return (wchar_t)c;
}

static wchar_t
dec_to_acs(TMT *vt, wchar_t w)
{
    // Translates from DEC Special Graphics to our ACS characters.
    // The capital letters are supposed to be symbols for control chars.
    // Specifically: Tab FormFeed CR LF NL VTab
    // 0xfa is hopefully an interpunct.

         if (w == '_'            ) w = ' ';                         // 0x5F NBSP
    else if (w >= '`' && w <= 'a') w = vt->acschars[w - '`' +  5];  // 0x60-61
    else if (w >= 'b' && w <= 'e') w = "TFCL"[w - 'b'];             // 0x62-65
    else if (w >= 'f' && w <= 'g') w = vt->acschars[w - 'f' +  7];  // 0x66-67
    else if (w >= 'h' && w <= 'i') w = "NV"[w - 'h'];               // 0x68-69
    else if (w >= 'j' && w <= '~') w = vt->acschars[w - 'j' + 10];  // 0x6a-7e

    return w;
}

static void
writecharatcurs(TMT *vt, wchar_t w)
{
    COMMON_VARS;

    if (vt->XN) {
        vt->XN = false;
        c->c = 0;
        c->r++;
        if (c->r > vt->maxline) {
            scrup(vt, SCR_DEF, 1);
            c->r = vt->maxline;
        }
    }

    if (vt->unicode_to_acs) {   // replace some unicode with ACS glyphs
        // We can add more mappings here, but the initial set here comes from:
        // justsolve.archiveteam.org/wiki/DEC_Special_Graphics_Character_Set
        // See also codepage.c from qodem.
        switch (w) {
        case 0x2192: w = vt->acschars[0]; break; // RIGHT ARROW
        case 0x2190: w = vt->acschars[1]; break; // LEFT ARROW
        case 0x2191: w = vt->acschars[2]; break; // UP ARROW
        case 0x2193: w = vt->acschars[3]; break; // DOWN ARROW
        case 0x2588: w = vt->acschars[4]; break; // BLOCK
        case 0x25A6: w = vt->acschars[9]; break; // BOARD
        case 0x00A0: w = dec_to_acs(vt, 0x5f); break; // NBSP
        case 0x2666:                                  // BLACK DIAMOND SUIT
        case 0x25C6: w = dec_to_acs(vt, 0x60); break; // BLACK DIAMOND
        case 0x2592: w = dec_to_acs(vt, 0x61); break; // MEDIUM SHADE
        case 0x2409: w = dec_to_acs(vt, 0x62); break; // SYMBOL FOR HORIZONTAL TABULATION
        case 0x240C: w = dec_to_acs(vt, 0x63); break; // SYMBOL FOR FORM FEED
        case 0x240D: w = dec_to_acs(vt, 0x64); break; // SYMBOL FOR CARRIAGE RETURN
        case 0x240A: w = dec_to_acs(vt, 0x65); break; // SYMBOL FOR LINE FEED
        case 0x00B0: w = dec_to_acs(vt, 0x66); break; // DEGREE SIGN
        case 0x00B1: w = dec_to_acs(vt, 0x67); break; // PLUS-MINUS SIGN
        case 0x2424: w = dec_to_acs(vt, 0x68); break; // SYMBOL FOR NEWLINE
        case 0x240B: w = dec_to_acs(vt, 0x69); break; // SYMBOL FOR VERTICAL TABULATION
        case 0x2518: w = dec_to_acs(vt, 0x6a); break; // BOX DRAWINGS LIGHT UP AND LEFT
        case 0x2510: w = dec_to_acs(vt, 0x6b); break; // BOX DRAWINGS LIGHT DOWN AND LEFT
        case 0x250C: w = dec_to_acs(vt, 0x6c); break; // BOX DRAWINGS LIGHT DOWN AND RIGHT
        case 0x2514: w = dec_to_acs(vt, 0x6d); break; // BOX DRAWINGS LIGHT UP AND RIGHT
        case 0x253C: w = dec_to_acs(vt, 0x6e); break; // BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
        case 0x23BA: w = dec_to_acs(vt, 0x6f); break; // HORIZONTAL SCAN LINE-1
        case 0x23BB: w = dec_to_acs(vt, 0x70); break; // HORIZONTAL SCAN LINE-3
        case 0x2500: w = dec_to_acs(vt, 0x71); break; // BOX DRAWINGS LIGHT HORIZONTAL
        case 0x23BC: w = dec_to_acs(vt, 0x72); break; // HORIZONTAL SCAN LINE-7
        case 0x23BD: w = dec_to_acs(vt, 0x73); break; // HORIZONTAL SCAN LINE-9
        case 0x251C: w = dec_to_acs(vt, 0x74); break; // BOX DRAWINGS LIGHT VERTICAL AND RIGHT
        case 0x2524: w = dec_to_acs(vt, 0x75); break; // BOX DRAWINGS LIGHT VERTICAL AND LEFT
        case 0x2534: w = dec_to_acs(vt, 0x76); break; // BOX DRAWINGS LIGHT UP AND HORIZONTAL
        case 0x252C: w = dec_to_acs(vt, 0x77); break; // BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
        case 0x2502: w = dec_to_acs(vt, 0x78); break; // BOX DRAWINGS LIGHT VERTICAL
        case 0x2264: w = dec_to_acs(vt, 0x79); break; // LESS-THAN OR EQUAL TO
        case 0x2265: w = dec_to_acs(vt, 0x7a); break; // GREATER-THAN OR EQUAL TO
        case 0x03C0: w = dec_to_acs(vt, 0x7b); break; // GREEK SMALL LETTER PI
        case 0x2260: w = dec_to_acs(vt, 0x7c); break; // NOT EQUAL TO
        case 0x00A3: w = dec_to_acs(vt, 0x7d); break; // POUND SIGN
        case 0x00B7: w = dec_to_acs(vt, 0x7e); break; // MIDDLE DOT
        }
    }

    if (vt->xlate[vt->charset])
        w = dec_to_acs(vt, w);

    #ifdef TMT_HAS_WCWIDTH
    extern int wcwidth(wchar_t c);
    if (wcwidth(w) > 1)  w = TMT_INVALID_CHAR;
    if (wcwidth(w) < 0) return;
    #endif

    CLINE(vt)->chars[vt->curs.c].c = w;
    CLINE(vt)->chars[vt->curs.c].a = vt->attrs;
    tmt_dirty(vt, vt->curs.c, CLINENO(vt), 1, 1);

    if (c->c < s->ncol - 1)
        c->c++;
    else vt->XN = true;
}

static wchar_t
getmbchar(TMT *vt)
{
    wchar_t wc = TMT_INVALID_CHAR;

    if (!vt->nmb) return -1;
    if (vt->nmb < MB_LEN_MAX) {
        if (xmbrtowc(&wc, &vt->mb[vt->nmb-1], 1, &vt->ms) == (size_t)-2)
            return -2;
    }
    vt->nmb = 0;
    return wc;
}

void
tmt_write(TMT *vt, const char *s, size_t n)
{
    TMTCURSOR oc = vt->curs;
    bool moved;
    wchar_t wc;
    n = n? n : strlen(s);

    for (size_t p = 0; p < n; p++){
        if (handlechar(vt, s[p] & 255))
            ;
        else if (vt->acs)
            writecharatcurs(vt, tacs(vt, s[p] & 255));
        else
        {
            vt->mb[vt->nmb++] = s[p];
            wc = getmbchar(vt);
            if (wc != -2 && wc != -1)
                writecharatcurs(vt, wc);
        }
    }

    moved = memcmp(&oc, &vt->curs, sizeof(oc)) != 0;
    if (moved) {
        tmt_dirty(vt, oc.c, oc.r, 1, 1);
        tmt_dirty(vt, vt->curs.c, vt->curs.r, 1, 1);
    }

    notify(vt, vt->screen.update.dirty, moved);
}

const TMTSCREEN *
tmt_screen(const TMT *vt)
{
    return &vt->screen;
}

const TMTCURSOR *
tmt_cursor(const TMT *vt)
{
    return &vt->curs;
}

void
tmt_reset(TMT *vt)
{
    vt->curs.r = vt->curs.c = vt->curs.hidden = 0;
    vt->oldcurs.r = vt->oldcurs.c = vt->oldcurs.hidden = 0;
    vt->acs = vt->XN = 0;
    vt->minline = 0;
    vt->maxline = vt->screen.nline-1;
    vt->attrs = vt->oldattrs = defattrs;
    resetparser(vt);
    memset(&vt->ms, 0, sizeof(vt->ms));
    clearlines(vt, 0, vt->screen.nline);
    CB(vt, TMT_MSG_CURSOR, "t");
    notify(vt, true, true);
}
