/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Main module of Microwindows
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifndef __PACIFIC__
#include <errno.h>
#include <sys/types.h>
#endif

#if UNIX | DOS_DJGPP
#include <unistd.h>
#if _MINIX
#include <sys/times.h>
#else
#include <sys/time.h>
#endif
#endif

#if ELKS
#include <linuxmt/posix_types.h>
#include <linuxmt/time.h>
#endif

#include "windows.h"
#include "wintern.h"
#include "device.h"
/*
 * External definitions defined here.
 */
HWND		listwp;			/* list of all windows */
HWND		rootwp;			/* root window pointer */
HWND		focuswp;		/* focus window for keyboard */
HWND		mousewp;		/* window mouse is currently in */
HWND		capturewp;		/* capture window*/
HWND		dragwp;			/* window user is dragging*/
HCURSOR		curcursor;		/* currently enabled cursor */
MWCOORD		cursorx;		/* current x position of cursor */
MWCOORD		cursory;		/* current y position of cursor */
DWORD		startTicks;		/* tickcount on startup */
int		keyb_fd;		/* the keyboard file descriptor */
int		mouse_fd;		/* the mouse file descriptor */
int		escape_quits = 1;	/* terminate when pressing ESC */

int
main(int ac,char **av)
{
	/* call user hook routine before anything*/
	if(MwUserInit(ac, av) < 0)
		exit(1);

	if(MwOpen() < 0)
		exit(1);

	/* call windows main program entry point*/
	WinMain(NULL, NULL, NULL, SW_SHOW);

	MwClose();
	return 0;
}

/*
 * Open a connection from a new client to the server.
 * Returns -1 on failure.
 */
int
MwOpen(void)
{
	/* Client calls this routine once.  We 
	 * init everything here
	 */
	if(MwInitialize() < 0)
		return -1;
        return 1;
}

/*
 * Close the connection to the server.
 */
void
MwClose(void)
{
	MwTerminate();
}

#if (UNIX | DOS_DJGPP) && !_MINIX
/*
 * Support for more than one user fd.
 * Chris Johns (ccj@acm.org)
 *
 * Register the specified file descriptor to post
 * WM_FDINPUT/WM_FDOUTPUT/WM_FDEXCEPT to the passed hwnd
 * when input/output/except is ready.
 *
 * Allow for any fd to be selected on.
 *
 * The user fd's are listed and scanned helping keep the
 * overhead down for a large list of fd's being selected on.
 */

typedef struct {
	HWND read;
	HWND write;
	HWND except;
	int  next;
} WNDUSERFD;

static WNDUSERFD userregfd[FD_SETSIZE];
static int       userregfd_head;

void
MwRegisterFdInput(HWND hwnd, int fd)
{
	if (fd < FD_SETSIZE && fd != mouse_fd && fd != keyb_fd) {
		if (!userregfd[fd].read) {
			userregfd[fd].read = hwnd;
			if (userregfd[fd].next == -1 && !userregfd[fd].write && !userregfd[fd].except) {
				userregfd[fd].next = userregfd_head;
				userregfd_head = fd;
			}
		}
	}
}

void
MwUnregisterFdInput(HWND hwnd, int fd)
{
	if (fd < FD_SETSIZE && fd != mouse_fd && fd != keyb_fd) {
		if (userregfd[fd].read == hwnd) {
			userregfd[fd].read = NULL;
			if (!userregfd[fd].write && !userregfd[fd].except) {
				int *listfd = &userregfd_head;
				while (*listfd != -1) {
					if (*listfd == fd) {
						*listfd = userregfd[fd].next;
						userregfd[fd].next = -1;
						return;
					}
					listfd = &userregfd[*listfd].next;
				}
				userregfd_head = fd;
			}
		}
	}
}

void
MwRegisterFdOutput(HWND hwnd, int fd)
{
	if (fd < FD_SETSIZE && fd != mouse_fd && fd != keyb_fd) {
		if (!userregfd[fd].write) {
			userregfd[fd].write = hwnd;
			if (userregfd[fd].next == -1 && !userregfd[fd].read && !userregfd[fd].except) {
				userregfd[fd].next = userregfd_head;
				userregfd_head = fd;
			}
		}
	}
}

void
MwUnregisterFdOutput(HWND hwnd, int fd)
{
	if (fd < FD_SETSIZE && fd != mouse_fd && fd != keyb_fd) {
		if (userregfd[fd].write == hwnd) {
			userregfd[fd].write = NULL;
			if (!userregfd[fd].read && !userregfd[fd].except) {
				int *listfd = &userregfd_head;
				while (*listfd != -1) {
					if (*listfd == fd) {
						*listfd = userregfd[fd].next;
						userregfd[fd].next = -1;
						return;
					}
					listfd = &userregfd[*listfd].next;
				}
				userregfd_head = fd;
			}
		}
	}
}

void
MwRegisterFdExcept(HWND hwnd, int fd)
{
	if (fd < FD_SETSIZE && fd != mouse_fd && fd != keyb_fd) {
		if (!userregfd[fd].except) {
			userregfd[fd].except = hwnd;
			if (userregfd[fd].next == -1 && !userregfd[fd].read && !userregfd[fd].write) {
				userregfd[fd].next = userregfd_head;
				userregfd_head = fd;
			}
		}
	}
}

void
MwUnregisterFdExcept(HWND hwnd, int fd)
{
	if (fd < FD_SETSIZE && fd != mouse_fd && fd != keyb_fd) {
		if (userregfd[fd].except == hwnd) {
			userregfd[fd].except = NULL;
			if (!userregfd[fd].read && !userregfd[fd].write) {
				int *listfd = &userregfd_head;
				while (*listfd != -1) {
					if (*listfd == fd) {
						*listfd = userregfd[fd].next;
						userregfd[fd].next = -1;
						return;
					}
					listfd = &userregfd[*listfd].next;
				}
				userregfd_head = fd;
			}
		}
	}
}

#endif /* UNIX | DOS_DJGPP*/

#if MSDOS | _MINIX
void
MwSelect(void)
{
	/* If mouse data present, service it*/
	if(mousedev.Poll())
		while(MwCheckMouseEvent())
			continue;

	/* If keyboard data present, service it*/
	if(kbddev.Poll())
		while(MwCheckKeyboardEvent())
			continue;

	MwHandleTimers();
}
#endif

#if UNIX && defined(HAVESELECT)
#if ANIMATEPALETTE
static int fade = 0;
#endif

void
MwSelect(void)
{
	fd_set	rfds;
	fd_set	wfds;
	fd_set	efds;
	int 	fd;
	int 	e;
	int	setsize = 0;
	UINT	timeout;
	struct timeval to;

	/* perform pre-select duties, if any*/
	if(scrdev.PreSelect)
		scrdev.PreSelect(&scrdev);

	/* Set up the FDs for use in the main select(): */
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);
  
	if(mouse_fd >= 0) {
		FD_SET(mouse_fd, &rfds);
		if(mouse_fd > setsize)
			setsize = mouse_fd;
	}
	if(keyb_fd >= 0) {
		FD_SET(keyb_fd, &rfds);
		if(keyb_fd > setsize)
			setsize = keyb_fd;
	}

	/* handle registered file descriptors */
	fd = userregfd_head;
	while (fd != -1) {
		if (userregfd[fd].read) FD_SET(fd, &rfds);
		if (userregfd[fd].write) FD_SET(fd, &wfds);
		if (userregfd[fd].except) FD_SET(fd, &efds);
		if(fd > setsize) setsize = fd;
		fd = userregfd[fd].next;
	}

	++setsize;

	/* Set up the timeout for the main select().  If
	 * the mouse is captured we're probably moving a window,
	 * so poll quickly to allow other windows to repaint while
	 * checking for more event input.
	 */
	if(dragwp)
		timeout = to.tv_sec = to.tv_usec = 0L;
	else {
		timeout = MwGetNextTimeoutValue();	/* returns ms*/
#if ANIMATEPALETTE
		if(fade < 100)
			timeout = 40;
#endif
if (!timeout) timeout = 10;	/* temp kluge required for mdemo to run ok*/
		GdGetNextTimeout(&to, timeout);
	}

	/* Wait for some input on any of the fds in the set or a timeout: */
	if((e = select(setsize, &rfds, &wfds, &efds, &to)) > 0) {
		
		/* If data is present on the mouse fd, service it: */
		if(mouse_fd >= 0 && FD_ISSET(mouse_fd, &rfds))
			while(MwCheckMouseEvent())
				continue;

		/* If data is present on the keyboard fd, service it: */
		if(keyb_fd >= 0 && FD_ISSET(keyb_fd, &rfds))
			while(MwCheckKeyboardEvent())
				continue;

		/* If registered descriptor, handle it */
		fd = userregfd_head;
		while (fd != -1) {
			if (userregfd[fd].read && FD_ISSET(fd, &rfds))
				PostMessage(userregfd[fd].read, WM_FDINPUT, fd, 0);
			if (userregfd[fd].write && FD_ISSET(fd, &wfds))
				PostMessage(userregfd[fd].write, WM_FDOUTPUT, fd, 0);
			if (userregfd[fd].except && FD_ISSET(fd, &efds))
				PostMessage(userregfd[fd].except, WM_FDEXCEPT, fd, 0);
			fd = userregfd[fd].next;
		}
	} 
	else if(e == 0) {
		/* timeout has occured*/
		if(GdTimeout() == FALSE)
			return;
#if ANIMATEPALETTE
		if(fade <= 100) {
			setfadelevel(&scrdev, fade);
			fade += 5;
		}
#endif
		MwHandleTimers();
	} else
		if(errno != EINTR)
			EPRINTF("Select() call in main failed\n");
}
#endif

#if VTSWITCH
static void
CheckVtChange(void *arg)
{
	if(MwCheckVtChange()) {
#if ANIMATEPALETTE
		fade = 0;
#endif
		MwRedrawScreen();
	}
	GdAddTimer(50, CheckVtChange, NULL);
}
#endif

/*
 * Initialize the graphics and mouse devices at startup.
 * Returns nonzero with a message printed if the initialization failed.
 */
int
MwInitialize(void)
{
	HWND		wp;		/* root window */
	PSD		psd;
	WNDCLASS	wc;
	int		fd;
	static MWCURSOR arrow = {	/* default arrow cursor*/
		16, 16,
		0,  0,
		RGB(255, 255, 255), RGB(0, 0, 0),
		{ 0xe000, 0x9800, 0x8600, 0x4180,
		  0x4060, 0x2018, 0x2004, 0x107c,
		  0x1020, 0x0910, 0x0988, 0x0544,
		  0x0522, 0x0211, 0x000a, 0x0004 },
		{ 0xe000, 0xf800, 0xfe00, 0x7f80,
		  0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
		  0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
		  0x073e, 0x021f, 0x000e, 0x0004 }
	};

	extern MWLISTHEAD mwClassHead;

#if (UNIX | DOS_DJGPP) && !_MINIX
	for (fd = 0; fd < FD_SETSIZE; fd++) {
		userregfd[fd].read = NULL;
		userregfd[fd].write = NULL;
		userregfd[fd].except = NULL;
		userregfd[fd].next = -1;
  	}
	userregfd_head = -1;
#endif
	/* catch terminate signal to restore tty state*/
	signal(SIGTERM, (void *)MwTerminate);

	startTicks = GetTickCount();
	
	if ((keyb_fd = GdOpenKeyboard()) == -1) {
		EPRINTF("Cannot initialise keyboard\n");
		return -1;
	}

	if ((psd = GdOpenScreen()) == NULL) {
		EPRINTF("Cannot initialise screen\n");
		GdCloseKeyboard();
		return -1;
	}

	if ((mouse_fd = GdOpenMouse()) == -1) {
		EPRINTF("Cannot initialise mouse\n");
		GdCloseScreen(psd);
		GdCloseKeyboard();
		return -1;
	}

#if ANIMATEPALETTE
	setfadelevel(psd, 0);
#endif
	/*
	 * Initialize the root window.
	 */
	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0; /*LoadIcon(GetHInstance(), MAKEINTRESOURCE( 1));*/
	wc.hCursor = 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
	wc.lpszMenuName = NULL;
	wc.lpszClassName =  "DeskTop";
	RegisterClass( &wc);
	
	wp = GdItemNew(struct hwnd);
	if (!wp) {
		EPRINTF("No memory for root window\n");
		GdCloseMouse();
		GdCloseScreen(psd);
		GdCloseKeyboard();
		return -1;
	}
	/* remove the WS_CAPTION to have bare desktop window*/
	/*wp->style = WS_CLIPCHILDREN | WS_CAPTION | WS_VISIBLE;*/
	wp->style = WS_CLIPCHILDREN | WS_VISIBLE;
	wp->exstyle = 0;
	wp->pClass = (PWNDCLASS)mwClassHead.head;
	wp->parent = NULL;
	wp->children = NULL;
	wp->siblings = NULL;
	wp->next = NULL;
	SetRect(&wp->winrect, 0, 0, psd->xvirtres, psd->yvirtres);
	MwCalcClientRect(wp);
	wp->cursor = NULL;
	wp->unmapcount = 0;
	wp->id = 0;
	strcpy(wp->szTitle, "Microwindows");
	wp->gotPaintMsg = PAINT_PAINTED;
#if UPDATEREGIONS
	wp->update = GdAllocRegion();
#endif

	listwp = wp;
	rootwp = wp;
	focuswp = wp;
	mousewp = wp;

	/* schedule desktop window paint*/
	InvalidateRect(rootwp, NULL, TRUE);

#if VTSWITCH
	MwInitVt();
	/* Check for VT change every 50 ms: */
	GdAddTimer(50, CheckVtChange, NULL);
#endif

	/*
	 * Initialize and position the default cursor.
	 */
	curcursor = NULL;
	cursorx = -1;
	cursory = -1;
	GdShowCursor(psd);
	MwMoveCursor(psd->xvirtres / 2, psd->yvirtres / 2);
	MwSetCursor(rootwp, &arrow);

	/*
	 * Finally tell the mouse driver some things.
	 */
	GdRestrictMouse(0, 0, psd->xvirtres - 1, psd->yvirtres - 1);
	GdMoveMouse(psd->xvirtres / 2, psd->yvirtres / 2);

	return 0;
}

/*
 * Here to close down the server.
 */
void
MwTerminate(void)
{
	GdCloseScreen(&scrdev);
	GdCloseMouse();
	GdCloseKeyboard();
#if VTSWITCH
	MwRedrawVt(mwvterm);
	MwExitVt();
#endif
	exit(0);
}

/*
 * Return # milliseconds elapsed since start of Microwindows
 * Granularity is 25 msec
 */
DWORD WINAPI
GetTickCount(VOID)
{
#if MSDOS
#include <time.h>
	return (DWORD)(clock() * 1000 / CLOCKS_PER_SEC);
#else
#if _MINIX
	struct tms	t;
	
	return (DWORD)times(&t) * 16;
#else
#if UNIX
	struct timeval t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 25000) * 25) - startTicks;
#else
	return 0L;
#endif
#endif
#endif
}

VOID WINAPI
Sleep(DWORD dwMilliseconds)
{
	int i, j, k;
	const int loops_per_ms = 20000;

	/* FIXME this is not calibrated */
	for(i=0; i < dwMilliseconds; i++)
		for(j=0; j < loops_per_ms; j++)
			k = i * j;
}
