/*
 * Copyright (c) 2003 Koninklijke Philips Electronics
 *
 * Memory allocation functions.
 *
 * Contents of this file are not for general export
 */

#ifndef MWSYSTEM_H_INCLUDED 
#define MWSYSTEM_H_INCLUDED

/*
 * This file defines the memory allocation functions for Microwindows.
 * It is useful when porting to platforms where Microwindows needs to
 * use a non-standard memory allocator.
 *
 * This file defines macros or functions with the following prototypes:
 *
 * void * GdMalloc(size_t new_size);
 * void * GdCalloc(size_t num, size_t size);
 * void * GdRealloc(void * block, size_t old_size, size_t new_size);
 * void * GdFree(void * block);
 * char * GdStrDup(const char * src);
 *
 * The default implementation is to use the standard malloc()/free().
 *
 * Note the extra "old_size" parameter to GdRealloc().
 */

/* for definitions of calloc, malloc, realloc, and free */
#include <stdlib.h>

#define GdMalloc(size)                  malloc((size))
#define GdCalloc(num,size)              calloc((num),(size))
#define GdRealloc(addr,oldsize,newsize) realloc((addr),(newsize))
#define GdFree(addr)                    free((addr))
#define GdStrDup(string)                strdup((string))


/*
Hint:  To change old source code to use the memory allocator defined
in this file, do (or rather, read, understand, then do):

find . \( -path './demos' -prune \) -o \( -name '*.[ch]' -a \! -name mwsystem.h -a -print \) | \
xargs perl -i -pWe \
's/malloc(\s*\()/GdMalloc$1/g; s/calloc(\s*\()/GdCalloc$1/g; s/strdup(\s*\()/GdStrDup$1/g; s/free(\s*\()/GdFree$1/g;'

(this script is also in src/include/memfix.sh)
You have to fix calls to realloc() by hand, since the parameters are different.
*/
#endif
