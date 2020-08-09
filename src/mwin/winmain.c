/*
 * Copyright (c) 1999, 2000, 2004, 2005, 2010, 2019 Greg Haerr <greg@censoft.com>
 *
 * Main module of Microwindows
 */
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include "uni_std.h"
#include "sys_time.h"

#if RTEMS
#include <rtems/mw_uid.h>
#endif

#if __ECOS
#include <cyg/kernel/kapi.h>
#endif

#include "windows.h"
#include "wintern.h"
#include "winres.h"
#include "windlg.h"
#include "device.h"
#include "osdep.h"

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
int		keyb_fd;		/* the keyboard file descriptor */
int		mouse_fd;		/* the mouse file descriptor */
DWORD		lastWIN32Error = 0;	/* Last error */

int WINAPI
invoke_WinMain_Start(int ac, char **av)
{
    HINSTANCE hInstance;

	/* call user hook routine before anything*/
	if (MwUserInit(ac, av) < 0)
		exit(1);

	GdPlatformInit();			/* platform-specific initialization*/

	if(MwInitialize() < 0)
		exit(1);

	if ((hInstance = mwCreateInstance(ac, av)) == NULL)
	    exit(1);
	rootwp->hInstance = hInstance;

	return 0;
}

void WINAPI
invoke_WinMain_End(void)
{
	mwFreeInstance(rootwp->hInstance);
	MwTerminate();
	exit(0);
}

#if UNIX && HAVE_SELECT
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

void WINAPI
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

void WINAPI
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

void WINAPI
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

void WINAPI
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

void WINAPI
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

void WINAPI
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

#endif /* UNIX && HAVE_SELECT*/

/********************************************************************************/
#if UNIX && HAVE_SELECT

void
MwSelect(BOOL canBlock)
{
	fd_set	rfds;
	fd_set	wfds;
	fd_set	efds;
	int 	fd;
	int 	e;
	int	setsize = 0;
	MWTIMEOUT	timeout;
	struct timeval tout, *to;

	/* x11/sdl update screen & flush buffers*/
	if(scrdev.PreSelect)
	{
		/* returns # pending events*/
		if (scrdev.PreSelect(&scrdev))
		{
			/* poll for mouse data and service if found*/
			while (MwCheckMouseEvent())
				continue;

			/* poll for keyboard data and service if found*/
			while (MwCheckKeyboardEvent())
				continue;

			/* events found, return with no sleep*/
			return;
		}
	}

	/* Set up the FDs for use in the main select(): */
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);
  
	if (mouse_fd >= 0)
	{
		FD_SET(mouse_fd, &rfds);
		if(mouse_fd > setsize)
			setsize = mouse_fd;
	}
	if (keyb_fd >= 0)
	{
		FD_SET(keyb_fd, &rfds);
		if(keyb_fd > setsize)
			setsize = keyb_fd;
	}

	/* handle registered file descriptors */
	fd = userregfd_head;
	while (fd != -1)
	{
		if (userregfd[fd].read) FD_SET(fd, &rfds);
		if (userregfd[fd].write) FD_SET(fd, &wfds);
		if (userregfd[fd].except) FD_SET(fd, &efds);
		if (fd > setsize) setsize = fd;
		fd = userregfd[fd].next;
	}

	++setsize;

	/*
	 * Setup timeval struct for block or poll in select().
	 * If we're moving a window, poll quickly to allow other windows
	 * to repaint while checking for more event input.
	 */
	timeout = tout.tv_sec = tout.tv_usec = 0L;
	to = &tout;
	int poll = (!canBlock || dragwp);		/* just poll if can't block or window move in progress*/
	if (!poll)
	{
		if ((timeout = MwGetNextTimeoutValue()) == (MWTIMEOUT) -1L)	/* get next mwin timer*/
			timeout = 0;											/* no mwin timers*/
#if MW_FEATURE_TIMERS
		/* get next timer or use passed timeout and convert to timeval struct*/
		if (!GdGetNextTimeout(&tout, timeout))		/* no VTSWITCH timer?*/
#else
		if (timeout)								/* setup mwin poll timer*/
		{
			/* convert wait timeout to timeval struct*/
			tout.tv_sec = timeout / 1000;
			tout.tv_usec = (timeout % 1000) * 1000;
		}
		else
#endif
		{
			to = NULL;								/* no timers, block*/
		}
	}

	/* some drivers can't block in select as backend is poll based (SDL)*/
	if (scrdev.flags & PSF_CANTBLOCK)
	{
#define WAITTIME	100
		/* check if would block permanently or timeout > WAITTIME*/
		if (to == NULL || tout.tv_sec != 0 || tout.tv_usec > WAITTIME)
		{
			/* override timeouts and wait for max WAITTIME ms*/
			to = &tout;
			tout.tv_sec = 0;
			tout.tv_usec = WAITTIME;
		}
	}

	/* Wait for some input on any of the fds in the set or a timeout*/
	if ((e = select(setsize, &rfds, &wfds, &efds, to)) > 0)
	{
		/* service mouse file descriptor*/
		if (mouse_fd >= 0 && FD_ISSET(mouse_fd, &rfds))
			while (MwCheckMouseEvent())
				continue;

		/* service keyboard file descriptor*/
		if (keyb_fd >= 0 && FD_ISSET(keyb_fd, &rfds))
			while (MwCheckKeyboardEvent())
				continue;

		/* If registered descriptor, handle it */
		fd = userregfd_head;
		while (fd != -1)
		{
			if (userregfd[fd].read && FD_ISSET(fd, &rfds))
				PostMessage(userregfd[fd].read, WM_FDINPUT, fd, 0);
			if (userregfd[fd].write && FD_ISSET(fd, &wfds))
				PostMessage(userregfd[fd].write, WM_FDOUTPUT, fd, 0);
			if (userregfd[fd].except && FD_ISSET(fd, &efds))
				PostMessage(userregfd[fd].except, WM_FDEXCEPT, fd, 0);
			fd = userregfd[fd].next;
		}
	} 
	else if(e == 0)			/* timeout*/
	{
#if MW_FEATURE_TIMERS
		/* check for timer timeouts and service if found*/
		GdTimeout();
#endif
	} else if(errno != EINTR)
		EPRINTF("Select() call in main failed. Errno=%d\n", errno);
}

/********************************************************************************/
#elif RTEMS | __ECOS

extern struct MW_UID_MESSAGE m_kbd;
extern struct MW_UID_MESSAGE m_mou;

void MwSelect (BOOL canBlock)
{
        struct MW_UID_MESSAGE m;
	int rc;
	unsigned int timeout = 0;

	/* perform pre-select duties, if any*/
	if (scrdev.PreSelect)
		scrdev.PreSelect (&scrdev);

	/* Set up the timeout for waiting.
	 * If the mouse is captured we're probably moving a window,
	 * so poll quickly to allow other windows to repaint while
	 * checking for more event input.
	 */
	if (!dragwp) {
	        timeout = MwGetNextTimeoutValue ();     /* returns ms*/
		        if (timeout < 10)
		                timeout = 10;       /* 10ms required for vt fb switch*/
	}										
	/* let's make sure that the type is invalid */
	m.type = MV_UID_INVALID;
	
	/* wait up to 100 miliseconds for events */
	rc = uid_read_message (&m, timeout);

	/* return if timed-out or something went wrong */
	if (rc < 0) {
	    if ( errno != ETIMEDOUT )
		        EPRINTF (" rc= %d, errno=%d\n", rc, errno);
		return;
	}

	/* let's pass the event up to microwindows */
	switch (m.type) {
	case MV_UID_REL_POS:	/* Mouse or Touch Screen event */
	case MV_UID_ABS_POS:
		m_mou = m;
		while (MwCheckMouseEvent())
			continue;
		break;
	case MV_UID_KBD:	/* KBD event */
		m_kbd = m;
		MwCheckKeyboardEvent();
		break;
	case MV_UID_TIMER:	/* Microwindows does nothing with these.. */
	case MV_UID_INVALID:
	default:
	        break;
	}
}

/********************************************************************************/
#else /* MSDOS | _MINIX | NDS | VXWORKS | PSP | __MINGW32__ | ALLEGRO | EMSCRIPTEN | AJAGUAR */

/* this MwSelect() is used for all polling-based platforms not specially handled above*/
#define WAITTIME	50		/* blocking sleep interval in msecs unless polling*/

void 
MwSelect(BOOL canBlock)
{
	int numevents = 0;

	/* perform single update of aggregate screen update region*/
	if (scrdev.PreSelect)
		scrdev.PreSelect(&scrdev);

	/* poll for mouse data and service if found*/
	while (MwCheckMouseEvent())
		if (++numevents > 10)
			break;				/* don't handle too many events at one shot*/
	
	/* poll for keyboard data and service if found*/
	while (MwCheckKeyboardEvent())
		if (++numevents > 10)
			break;				/* don't handle too many events at one shot*/
	
#if MW_FEATURE_TIMERS
	/* check for timer timeouts and service if found*/
	if (GdTimeout())
		++numevents;
#endif

	/* did we handle any input or were we just polling?*/
	if (numevents || !canBlock)
		return;					/* yes - return without sleeping*/

	/* no input processed, yield so we don't freeze system*/
	GdDelay(WAITTIME);
}

/********************************************************************************/
#endif /* MwSelect() cases*/

#if VTSWITCH
static void
CheckVtChange(void *arg)
{
	if(MwCheckVtChange()) {
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

#if UNIX && HAVE_SELECT
	for (fd = 0; fd < FD_SETSIZE; fd++) {
		userregfd[fd].read = NULL;
		userregfd[fd].write = NULL;
		userregfd[fd].except = NULL;
		userregfd[fd].next = -1;
  	}
	userregfd_head = -1;
#endif

#if HAVE_SIGNAL
	/* catch terminate signal to restore tty state*/
	signal(SIGTERM, (void *)MwTerminate);
#endif	

	if ((keyb_fd = GdOpenKeyboard()) == -1) {
		EPRINTF("Cannot initialise keyboard\n");
		return -1;
	}

	if ((psd = GdOpenScreenExt(FALSE)) == NULL) {
		EPRINTF("Cannot initialise screen\n");
		GdCloseKeyboard();
		return -1;
	}
	/* delay x11/sdl driver updates until preselect time for speed*/
	psd->flags |= PSF_DELAYUPDATE;

	if ((mouse_fd = GdOpenMouse()) == -1) {
		EPRINTF("Cannot initialise mouse\n");
		GdCloseScreen(psd);
		GdCloseKeyboard();
		return -1;
	}

	/*
	 * Initialize the root window.
	 */
	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	wc.lpfnWndProcBridge = NULL;
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
	wp->szTitle = (LPTSTR) malloc ( 64 );
	wp->lpfnWndProc = wc.lpfnWndProc;
	wp->lpfnWndProcBridge = NULL;
	wp->hInstance = NULL;
	wp->nEraseBkGnd = 1;
	wp->paintBrush = NULL;
	wp->paintPen = NULL;
	wp->color_key = 0;
	wp->alpha = 100;
	wp->layered_flags = 0;

	strcpy(wp->szTitle, "Microwindows");
	wp->gotPaintMsg = PAINT_PAINTED;
#if UPDATEREGIONS
	wp->update = GdAllocRegion();
#endif

	listwp = wp;
	rootwp = wp;
	focuswp = wp;
	mousewp = wp;

	/* set default work area to whole root window*/
	SystemParametersInfo(SPI_SETWORKAREA, 0, NULL, 0);

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

#if WINEXTRA
	MwInitializeDialogs(rootwp->hInstance);
#endif
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

#if !EMSCRIPTEN
VOID /*WINAPI*/
Sleep(DWORD dwMilliseconds)
{
	GdDelay(dwMilliseconds);
}
#endif
