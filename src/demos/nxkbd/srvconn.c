/*
 * Copyright (C) 2000 by Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 by VTech Informations LTD.
 * Vladimir Cotfas <vladimircotfas@vtech.ca> Aug 31, 2000
 */
#include <unistd.h>
#include <fcntl.h>
#include "nano-X.h"

#define KBDPIPE		0	/* =1 to use named pipe for soft kbd*/

#if KBDPIPE
static char KBD_NAMED_PIPE[] = "/tmp/.nano-X-softkbd";
static int kbd_fd = -1;

int 
KbdOpen(void)
{
	if (kbd_fd != -1)
		close(kbd_fd);
	
        if ((kbd_fd = open(KBD_NAMED_PIPE, O_WRONLY)) < 0)
		return -1;
			
        return kbd_fd;
}

void
KbdClose(void)
{
	if(kbd_fd >= 0) {
		close(kbd_fd);
		kbd_fd = -1;
	}
}

int
KbdWrite(int c)
{
	char cc = c & 0xff;
	
	return write(kbd_fd, &cc, 1);
}

#else /* !KBDPIPE*/

int 
KbdOpen(void)
{
        return 0;
}

void
KbdClose(void)
{
}

int
KbdWrite(int c)
{
	GR_WINDOW_ID	win = GrGetFocus();

	/* FIXME: modifiers are incorrect*/
	GrInjectKeyboardEvent(win, c, 0, 0, 1);
	GrInjectKeyboardEvent(win, c, 0, 0, 0);
	return 1;
}
#endif /* KBDPIPE*/
