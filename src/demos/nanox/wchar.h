#ifndef WCHAR_H
#define WCHAR_H
/*
 * Standalone wide character/multibyte definitions to avoid using system
 * library routines. The intent is to allow use on macOS, Linux and ELKS
 * without intefering with system library header files.
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

/* Use system wchar_t if available */
#ifndef WCHAR_T_DEFINED
#define WCHAR_T_DEFINED
#ifndef _WCHAR_T
#define _WCHAR_T
typedef int wchar_t;
#endif
#endif

typedef uint32_t Mbstate_t;     /* incompatible with mbstate_t on some systems */

#undef MB_LEN_MAX
#define MB_LEN_MAX  4

#undef MB_CUR_MAX
#define MB_CUR_MAX  4

/* mbrtowc -> xmbrtowc as uses Mbstate_t rather than mbstate_t */
size_t xmbrtowc(wchar_t *restrict wc, const char *restrict src, size_t n,
    Mbstate_t *restrict st);
size_t xwcrtomb(char *restrict s, wchar_t wc, Mbstate_t *restrict st);
int xwctomb(char *s, wchar_t wc);

#endif
