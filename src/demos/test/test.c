/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Test for select()
 */
#include <stdio.h>
#include <stdlib.h>

#ifndef __PACIFIC__
#include <errno.h>
#include <sys/types.h>
#endif

#if UNIX
#include <unistd.h>
#include <sys/time.h>
#endif
#if ELKS
#include <linuxmt/posix_types.h>
#include <linuxmt/time.h>
#endif
#include "windows.h"
#include "wintern.h"

int		keyb_fd;		/* the keyboard file descriptor */
int		mouse_fd;		/* the mouse file descriptor */

int
main(int ac,char **av)
{
	if ((keyb_fd = GdOpenKeyboard()) < 0) {
		PRINT_ERROR("Cannot initialise keyboard");
		return -1;
	}

	if ((mouse_fd = GdOpenMouse()) < 0) {
		PRINT_ERROR("Cannot initialise mouse");
		GdCloseKeyboard();
		return -1;
	}

	for(;;)
		GsSelect();

	return 0;
}

void
GsTerminate(void)
{
	GdCloseMouse();
	GdCloseKeyboard();
	exit(0);
}

#if MSDOS
void
GsSelect(void)
{
	/* If mouse data present, service it*/
	if(mousedev.Poll())
		GsCheckMouseEvent();

	/* If keyboard data present, service it*/
	if(kbddev.Poll())
		GsCheckKeyboardEvent();

}
#endif

#if UNIX && defined(HAVESELECT)
void
GsSelect(void)
{
	fd_set	rfds;
	int 	e;
	struct timeval to;
	int	n;

	/* Set up the FDs for use in the main select(): */
	FD_ZERO(&rfds);
	FD_SET(mouse_fd, &rfds);
	n = mouse_fd;
	FD_SET(keyb_fd, &rfds);
	if(keyb_fd > n)
		n = keyb_fd;

	/* Set up the timeout for the main select().  If
	 * the mouse is captured we're probably moving a window,
	 * so poll quickly to allow other windows to repaint while
	 * checking for more event input.
	 */
	to.tv_sec = 0L;
to.tv_sec = 1L;
	to.tv_usec = 0L;
to.tv_usec = 10000L;

	/* Wait for some input on any of the fds in the set or a timeout: */
	if((e = select(n+1, &rfds, NULL, NULL, &to)) > 0) {
		
		/* If data is present on the mouse fd, service it: */
		if(FD_ISSET(mouse_fd, &rfds))
			GsCheckMouseEvent();

		/* If data is present on the keyboard fd, service it: */
		if(FD_ISSET(keyb_fd, &rfds))
			GsCheckKeyboardEvent();

	} 
	else if(!e) {
		printf("select() timeout\n");
	} else
		if(errno != EINTR)
			PRINT_ERROR("Select() call in main failed");
}
#endif

/*
 * Update mouse status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the mouse.
 */
BOOL GsCheckMouseEvent(void)
{
	COORD		rootx;		/* latest mouse x position */
	COORD		rooty;		/* latest mouse y position */
	BUTTON		newbuttons;	/* latest buttons */
	int		mousestatus;	/* latest mouse status */

	/* Read the latest mouse status: */
	mousestatus = GdReadMouse(&rootx, &rooty, &newbuttons);
	if(mousestatus < 0) {
		printf("Mouse error\n");
		return FALSE;
	} else if(mousestatus) { /* Deliver events as appropriate: */	
		printf("mouse %d,%d,%d\n", rootx, rooty, newbuttons);
      return TRUE;
   }
   return FALSE;
}

/*
 * Update keyboard status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the keyboard.
 */
BOOL GsCheckKeyboardEvent(void)
{
	unsigned char	ch;		/* latest character */
	MODIFIER	modifiers;	/* latest modifiers */
	int		keystatus;	/* latest keyboard status */

	/* Read the latest keyboard status: */
	keystatus = GdReadKeyboard(&ch, &modifiers);
	if(keystatus < 0) {
		if(keystatus == -2)	/* special case for ESC pressed*/
			GsTerminate();
		printf("Kbd error\n");
		return FALSE;
	} else if(keystatus) { /* Deliver events as appropriate: */	
		printf("kbd '%c',%d\n", ch, modifiers);
      return TRUE;
   }
   return FALSE;
}
