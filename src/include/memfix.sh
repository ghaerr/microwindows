#! /bin/sh
# Run from microwin/src
find . \( -path './demos' -prune \) -o \( -name '*.[ch]' -a \! -name mwsystem.h -a \! -name XtoNX.h -a \! -name convbmp.c -a \! -name convbdf.c -print \) | xargs perl -i.bak -pwe 's/malloc(\s*\()/GdMalloc$1/g; s/calloc(\s*\()/GdCalloc$1/g; s/strdup(\s*\()/GdStrDup$1/g; s/free(\s*\()/GdFree$1/g;'
