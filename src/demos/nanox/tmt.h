/* Copyright (c) 2017 Rob King
 * All rights reserved.
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

#ifndef TMT_H
#define TMT_H

#include <stdbool.h>
#include <stddef.h>
#include "wchar.h"

/**** INVALID WIDE CHARACTER */
#define TMT_INVALID_CHAR ((wchar_t)0x007f)  /* use DEL box glyph for smaller fonts */
#ifndef TMT_INVALID_CHAR
#define TMT_INVALID_CHAR ((wchar_t)0xfffd)
#endif

/**** INPUT SEQUENCES */
#define TMT_KEY_UP             "\033[A"
#define TMT_KEY_DOWN           "\033[B"
#define TMT_KEY_RIGHT          "\033[C"
#define TMT_KEY_LEFT           "\033[D"
#define TMT_KEY_HOME           "\033[1~"
#define TMT_KEY_END            "\033[4~"
#define TMT_KEY_INSERT         "\033[L"
#define TMT_KEY_BACKSPACE      "\x7f"
#define TMT_KEY_DELETE         "\033[3~"
#define TMT_KEY_ESCAPE         "\x1b"
#define TMT_KEY_BACK_TAB       "\033\x09"
#define TMT_KEY_PAGE_UP        "\033[5~"
#define TMT_KEY_PAGE_DOWN      "\033[6~"
#define TMT_KEY_F1             "\033[[A"
#define TMT_KEY_F2             "\033[[B"
#define TMT_KEY_F3             "\033[[C"
#define TMT_KEY_F4             "\033[[D"
#define TMT_KEY_F5             "\033[[E"
#define TMT_KEY_F6             "\033[17~"
#define TMT_KEY_F7             "\033[18~"
#define TMT_KEY_F8             "\033[19~"
#define TMT_KEY_F9             "\033[20~"
#define TMT_KEY_F10            "\033[21~"
#define TMT_KEY_F11            "\033[23~"
#define TMT_KEY_F12            "\033[24~"

/**** BASIC DATA STRUCTURES */
typedef struct TMT TMT;

/* colors arranged in CGA/EGA palette order */
typedef enum{
    TMT_COLOR_BLACK = 0,
    TMT_COLOR_BLUE,
    TMT_COLOR_GREEN,
    TMT_COLOR_CYAN,
    TMT_COLOR_RED,
    TMT_COLOR_MAGENTA,
    TMT_COLOR_BROWN,
    TMT_COLOR_LTGRAY,
    TMT_COLOR_GRAY,
    TMT_COLOR_LTBLUE,
    TMT_COLOR_LTGREEN,
    TMT_COLOR_LTCYAN,
    TMT_COLOR_LTRED,
    TMT_COLOR_LTMAGENTA,
    TMT_COLOR_YELLOW,
    TMT_COLOR_WHITE,
    TMT_COLOR_DEFAULT
} tmt_color_t;

typedef struct TMTATTRS TMTATTRS;
struct TMTATTRS{
    bool bold:1;
    bool dim:1;
    bool underline:1;
    bool blink:1;
    bool reverse:1;
    bool invisible:1;
    tmt_color_t fg:5;
    tmt_color_t bg:5;
} __attribute__((packed));

typedef struct TMTCHAR TMTCHAR;
struct TMTCHAR{
    wchar_t c;
    TMTATTRS a;
} __attribute__((packed));

typedef struct TMTCURSOR TMTCURSOR;
struct TMTCURSOR{
    size_t r;
    size_t c;
    bool hidden;
};

typedef struct TMTUPDATE TMTUPDATE;
struct TMTUPDATE{
    size_t x, y;
    size_t w, h;
    bool dirty;
};

typedef struct TMTLINE TMTLINE;
struct TMTLINE{
    int reserved;
    TMTCHAR chars[];
};

typedef struct TMTSCREEN TMTSCREEN;
struct TMTSCREEN{
    size_t nline;
    size_t ncol;
    TMTUPDATE update;
    TMTLINE **lines;
};

/**** CALLBACK SUPPORT */
typedef enum{
    TMT_MSG_MOVED,
    TMT_MSG_UPDATE,
    TMT_MSG_ANSWER,
    TMT_MSG_TITLE,
    TMT_MSG_BELL,
    TMT_MSG_CURSOR,
    TMT_MSG_SETMODE,
    TMT_MSG_UNSETMODE,
    TMT_MSG_SCROLL,
} tmt_msg_t;
#define TMT_HAS_MSG_SCROLL 1

typedef void (*TMTCALLBACK)(tmt_msg_t m, struct TMT *v, const void *r, void *p);

/**** PUBLIC FUNCTIONS */
TMT *tmt_open(size_t nline, size_t ncol, TMTCALLBACK cb, void *p, const wchar_t *acs);
bool tmt_unicode_to_acs(TMT *vt, bool v);
void tmt_close(TMT *vt);
bool tmt_resize(TMT *vt, size_t nline, size_t ncol);
void tmt_write(TMT *vt, const char *s, size_t n);
const TMTSCREEN *tmt_screen(const TMT *vt);
const TMTCURSOR *tmt_cursor(const TMT *vt);
void tmt_clean(TMT *vt);
void tmt_dirty(TMT *vt, size_t x, size_t y, size_t w, size_t h);
void tmt_reset(TMT *vt);

#endif
