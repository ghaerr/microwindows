/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Operating System Dependent Routines Header File
 */

MWTIMEOUT	GdGetTickCount(void);
void		GdDelay(MWTIMEOUT msecs);
void		GdPlatformInit(void);
void		GdBell(void);
void		GdResizeFrameWindow(int w, int h, const char *title);
