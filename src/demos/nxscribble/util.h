/***********************************************************************

util.h - memory allocation, error reporting, and other mundane stuff

Copyright (C) 1991 Dean Rubine

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License. See ../COPYING for
the full agreement.

**********************************************************************/

/*
 * General utility functionss
 *
 * Mostly for dealing with mundane issues such as:
 *	Memory allocation
 *	Error handling
 */

/*
 * General allocation macro
 *
 * Example:
 *	struct list *s; s = allocate(4, struct list)
 * returns space for an array of 4 list structures.
 * Allocate will die if there is no more memory
 */

#define	allocate(n, type)	\
	((type *) myalloc(n, sizeof(type), "type"))

/*
 * Functions
 */

#define	STREQ(a,b)	( ! strcmp(a,b) )

char	*myalloc();	/* Do not call this function directly */
char	*scopy();	/* allocates memory for a string */
void	debug();	/* printf on stderr -
			   setting DebugFlag = 0 turns off debugging */
void	error();	/* printf on stderr, then dies */
int	ucstrcmp();	/* strcmp, upper case = lower case */
char	*tempstring();	/* returns a pointer to space that will reused soon */

/*
   this is the wrong place for all of this, but got chosen since
   every file includes this one
 */

#ifdef unix
#	define GRAPHICS		/* only GDEV on unix machines */
#endif

#ifndef unix

/* various BSD to lattice C name changes */

#ifdef __ECOS
extern char *strdup(char *);
#else
#define	bcopy	movmem
#endif
#define index	strchr
#define	rindex	strrchr

#endif
