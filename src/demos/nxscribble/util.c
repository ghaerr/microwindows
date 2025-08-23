/* *********************************************************************

util.c - memory allocation, error reporting, and other mundane stuff

Copyright (C) 1991 Dean Rubine

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License. See ../COPYING for
the full agreement.

**********************************************************************/
/*
 * Mundane utility routines
 *	see util.h
 */

/*LINTLIBRARY*/

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#include <string.h>
/*#include <stdlib.h>*/ /* removed for conflicting 'error' inline in RH 8.0?*/

extern char* li_err_msg;
static char err_msg[BUFSIZ];

/*
 * Function used by allocation macro
 */

char *
myalloc(nitems, itemsize, typename)
int nitems;
int itemsize;
char *typename;
{
	register unsigned int bytes = nitems * itemsize;
	register char *p = malloc(bytes);
	if(p == NULL)
	     error("Can't get mem for %d %s's (each %d bytes, %d total bytes)",
		nitems, typename, itemsize, bytes);
	return p;
}

/*
 * Return a copy of a string
 */

char *
scopy(s)
char *s;
{
	register char *p = allocate(strlen(s) + 1, char);
	(void) strcpy(p, s);
	return p;
}

/*
 * Save error message, then return to recognition manager.
 */

/*VARARGS1*/
void
error(a, b, c, d, e, f, g, h, i, j)
char *a;
int  b, c, d, e, f, g, h, i, j;
{
	sprintf(err_msg, a, b, c, d, e, f, g, h, i, j);
	li_err_msg = err_msg;
}

/*
 * Print error message, exit.
*/
/*VARARGS1*/
void
exit_error(a, b, c, d, e, f, g, h, i, j)
char *a;
int  b, c, d, e, f, g, h, i, j;
{
	GrError( a, b, c, d, e, f, g, h, i, j);
	exit(1);
}


/*
 * print a message if DebugFlag is non-zero
 */

int	DebugFlag = 1;

void
debug(a, b, c, d, e, f, g)
char *a;
int  b, c, d, e, f, g;
{
	if(DebugFlag)
		GrError( a, b, c, d, e, f, g);
}

#define	upper(c)	(islower(c) ? toupper(c) : (c))

int
ucstrcmp(s1, s2)
register char *s1, *s2;
{
	register int i;

	for(; *s1 && *s2; s1++, s2++)
		if( (i = (upper(*s1) - upper(*s2))) != 0)
			return i;
	return (upper(*s1) - upper(*s2));
}

#define NSTRINGS 3

char *
tempstring()
{
	static char strings[NSTRINGS][100];
	static int index;
	if(index >= NSTRINGS) index = 0;
	return strings[index++];
}
