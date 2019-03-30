/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Operating System Dependent Routines Header File
 */

#define register

#define ALLOCA			malloc
#define FREEA			free

#define EPRINTF			GdError		/* error output*/
#define DPRINTF			GdError		/* debug output*/

void		nop(void);

int			GdError(const char *format, ...);
MWTIMEOUT	GdGetTickCount(void);
void		GdDelay(MWTIMEOUT msecs);
void		GdPlatformInit(void);
void		GdBell(void);
