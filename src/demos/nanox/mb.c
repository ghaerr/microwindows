/*-*- mode:c;indent-tabs-mode:t;c-basic-offset:8;tab-width:8;coding:utf-8   -*-│
│ vi: set noet ft=c ts=8 sw=8 fenc=utf-8                                   :vi │
╚──────────────────────────────────────────────────────────────────────────────╝
│                                                                              │
│  Musl Libc                                                                   │
│  Copyright © 2005-2014 Rich Felker, et al.                                   │
│                                                                              │
│  Permission is hereby granted, free of charge, to any person obtaining       │
│  a copy of this software and associated documentation files (the             │
│  "Software"), to deal in the Software without restriction, including         │
│  without limitation the rights to use, copy, modify, merge, publish,         │
│  distribute, sublicense, and/or sell copies of the Software, and to          │
│  permit persons to whom the Software is furnished to do so, subject to       │
│  the following conditions:                                                   │
│                                                                              │
│  The above copyright notice and this permission notice shall be              │
│  included in all copies or substantial portions of the Software.             │
│                                                                              │
│  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,             │
│  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF          │
│  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.      │
│  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY        │
│  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,        │
│  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE           │
│  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                      │
│                                                                              │
╚─────────────────────────────────────────────────────────────────────────────*/
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "wchar.h"

/* Upper 6 state bits are a negative integer offset to bound-check next byte */
/*    equivalent to: ( (b-0x80) | (b+offset) ) & ~0x3f      */
#define OOB(c,b) (((((b)>>3)-0x10)|(((b)>>3)+((int32_t)(c)>>26))) & ~7)

/* Interval [a,b). Either a must be 80 or b must be c0, lower 3 bits clear. */
#define R(a,b) ((uint32_t)((a==0x80 ? (uint32_t)0x40u-b : (uint32_t)0u-a) << 23))
#define FAILSTATE R(0x80,0x80)

#define SA 0xc2u
#define SB 0xf4u

/* Arbitrary encoding for representing code units instead of characters. */
#define CODEUNIT(c) (0xdfff & (signed char)(c))
#define IS_CODEUNIT(c) ((unsigned)(c)-0xdf80 < 0x80)

#define C(x) ( x<2 ? -1 : ( R(0x80,0xc0) | x ) )
#define D(x) C((x+16))
#define E(x) ( ( x==0 ? R(0xa0,0xc0) : \
                 x==0xd ? R(0x80,0xa0) : \
                 R(0x80,0xc0) ) \
             | ( R(0x80,0xc0) >> 6 ) \
             | x )
#define F(x) ( ( x>=5 ? 0 : \
                 x==0 ? R(0x90,0xc0) : \
                 x==4 ? R(0x80,0x90) : \
                 R(0x80,0xc0) ) \
             | ( R(0x80,0xc0) >> 6 ) \
             | ( R(0x80,0xc0) >> 12 ) \
             | x )

static const uint32_t bittab[] = {
                  C(0x2),C(0x3),C(0x4),C(0x5),C(0x6),C(0x7),
    C(0x8),C(0x9),C(0xa),C(0xb),C(0xc),C(0xd),C(0xe),C(0xf),
    D(0x0),D(0x1),D(0x2),D(0x3),D(0x4),D(0x5),D(0x6),D(0x7),
    D(0x8),D(0x9),D(0xa),D(0xb),D(0xc),D(0xd),D(0xe),D(0xf),
    E(0x0),E(0x1),E(0x2),E(0x3),E(0x4),E(0x5),E(0x6),E(0x7),
    E(0x8),E(0x9),E(0xa),E(0xb),E(0xc),E(0xd),E(0xe),E(0xf),
    F(0x0),F(0x1),F(0x2),F(0x3),F(0x4)
};

#pragma GCC diagnostic ignored "-Wparentheses"

size_t xmbrtowc(wchar_t *restrict wc, const char *restrict src, size_t n,
    Mbstate_t *restrict st)
{
    static uint32_t internal_state;
    uint32_t c;
    const unsigned char *s = (const void *)src;
    const size_t N = n;
    wchar_t dummy;

    if (!st) st = (void *)&internal_state;
    c = *(uint32_t *)st;

    if (!s) {
        if (c) goto ilseq;
        return 0;
    } else if (!wc) wc = &dummy;

    if (!n) return -2;
    if (!c) {
        if (*s < 0x80) return !!(*wc = *s);
        if (MB_CUR_MAX==1) return (*wc = CODEUNIT(*s)), 1;
        if (*s-SA > SB-SA) goto ilseq;
        c = bittab[*s++-SA]; n--;
    }

    if (n) {
        if (OOB(c,*s)) goto ilseq;
loop:
        c = c<<6 | *s++-0x80; n--;
        if (!(c&((uint32_t)1U<<31))) {
            *(uint32_t *)st = 0;
            *wc = c;
            return N-n;
        }
        if (n) {
            if (*s-0x80u >= 0x40) goto ilseq;
            goto loop;
        }
    }

    *(uint32_t *)st = c;
    return -2;
ilseq:
    *(uint32_t *)st = 0;
    errno = EILSEQ;
    return -1;
}

size_t xwcrtomb(char *restrict s, wchar_t wc, Mbstate_t *restrict st)
{
        if (!s) return 1;
        if ((unsigned)wc < 0x80) {
                *s = wc;
                return 1;
        } else if (MB_CUR_MAX == 1) {
                if (!IS_CODEUNIT(wc)) {
                        errno = EILSEQ;
                        return -1;
                }
                *s = wc;
                return 1;
        } else if ((unsigned)wc < 0x800) {
                *s++ = 0xc0 | (wc>>6);
                *s = 0x80 | (wc&0x3f);
                return 2;
        } else if ((unsigned)wc < 0xd800 || (unsigned)wc-0xe000 < 0x2000) {
                *s++ = 0xe0 | (wc>>12);
                *s++ = 0x80 | ((wc>>6)&0x3f);
                *s = 0x80 | (wc&0x3f);
                return 3;
        }
#if !ELKS
	else if ((unsigned)wc-0x10000 < 0x100000) {
                *s++ = 0xf0 | (wc>>18);
                *s++ = 0x80 | ((wc>>12)&0x3f);
                *s++ = 0x80 | ((wc>>6)&0x3f);
                *s = 0x80 | (wc&0x3f);
                return 4;
        }
#endif
        errno = EILSEQ;
        return -1;
}

int xwctomb(char *s, wchar_t wc)
{
        if (!s) return 0;
        return xwcrtomb(s, wc, 0);
}
