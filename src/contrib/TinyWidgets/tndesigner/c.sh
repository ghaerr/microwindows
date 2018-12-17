#!/bin/bash
lex fileio.l
yacc -d fileio.y
gcc -c *.c -I ../include -ggdb  
gcc -o tnd *.o -ltnW -lnano-X -L ../lib -lfl -ggdb

