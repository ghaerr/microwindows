/*
 * Copyright (c) 1999, 2000, 2004, 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * Main module of Microwindows
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

#if UNIX | DOS_DJGPP
#include <unistd.h>
#if _MINIX
#include <sys/times.h>
#else
#include <sys/time.h>
#endif
#endif

#if MSDOS
#include <time.h>
#endif

#if RTEMS
#include <rtems/mw_uid.h>
#endif

#if __ECOS
#include <cyg/kernel/kapi.h>
#endif

#if PSP
#include <pspkernel.h>
#include <psputils.h>
#define exit(...) sceKernelExitGame()
#endif

//#if EMSCRIPTEN
//#include <emscripten.h>
//#endif  

#include "windows.h"
#include "wintern.h"
#include "winres.h"
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
DWORD		lastWIN32Error = 0;	/* Last error */

#if PSP
int exit_callback(void)
{
	sceKernelExitGame();
	return 0;
}

void CallbackThread(void *arg)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
}
#endif

int
#if __ECOS | NOMAIN
invoke_WinMain(int ac, char **av)
#else
main(int ac, char **av)
#endif
{
    HINSTANCE hInstance;

#if PSP
	int thid;
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, 0);

        pspDebugScreenInit();
		pspDebugScreenPrintf("\n Microwindows init...");
#endif

	/* call user hook routine before anything*/
	if (MwUserInit(ac, av) < 0)
		exit(1);

	if (MwOpen() < 0)
		exit(1);

	if ((hInstance = mwCreateInstance(ac, av)) == NULL)
	    exit(1);
		
	rootwp->hInstance = hInstance;

	/* call windows main program entry point*/
	WinMain(hInstance, NULL, (LPSTR)((PMWAPPINSTANCE)hInstance)->szCmdLine, SW_SHOW);

	mwFreeInstance(hInstance);
	MwClose();

	exit(0);
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

#endif /* UNIX && HAVE_SELECT*/

/********************************************************************************/
#if UNIX && HAVE_SELECT

void
MwSelect(BOOL mayWait)
{
	fd_set	rfds;
	fd_set	wfds;
	fd_set	efds;
	int 	fd;
	int 	e;
	int	setsize = 0;
	UINT	timeout;
	struct timeval to, *pto;
	BOOL    maybeInfinite = TRUE;

	/* x11/sdl update screen & flush buffers*/
	if(scrdev.PreSelect)
	{
		/* returns # pending events*/
		if (scrdev.PreSelect(&scrdev))
		{
			while(MwCheckMouseEvent())
				continue;
			while(MwCheckKeyboardEvent())
				continue;
			return;
		}
	}

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
	timeout = to.tv_sec = to.tv_usec = 0L;
	pto = &to;
	if( !dragwp && mayWait ) {
		timeout = MwGetNextTimeoutValue();	/* returns ms*/
		if( (int)timeout == -1 ) // this means that no timers exists
			timeout = 0;
		else
			maybeInfinite = FALSE;
#if MW_FEATURE_TIMERS
		if( !GdGetNextTimeout(&to, timeout) ) {
			to.tv_sec = timeout / 1000;
			to.tv_usec = (timeout % 1000) * 1000;
		} else
			maybeInfinite = FALSE;
#else
		to.tv_sec = timeout / 1000;
		to.tv_usec = (timeout % 1000) * 1000;
#endif
#if SDL
//printf("May %d %d,%d\n", maybeInfinite, to.tv_sec, to.tv_usec);
		to.tv_sec = 0;
		to.tv_usec = 10;
#endif
		/*  If no timers are scheduled so the select function will wait forever...  */
		if( maybeInfinite && (to.tv_sec == 0) && (to.tv_usec == 0) )
#if SDL
			/* can't block in select as SDL backend is poll based*/
			;
#else
			pto = NULL;
#endif
	}

	/* Wait for some input on any of the fds in the set or a timeout: */
	if((e = select(setsize, &rfds, &wfds, &efds, pto)) > 0) {
		if(mouse_fd >= 0 && FD_ISSET(mouse_fd, &rfds))
			while(MwCheckMouseEvent())
				continue;

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
	else if(e == 0) { /* timeout*/
#if MW_FEATURE_TIMERS
		if(GdTimeout() == FALSE)
			return;
#endif /* MW_FEATURE_TIMERS */
		MwHandleTimers();
	} else
		if(errno != EINTR)
			EPRINTF("Select() call in main failed. Errno=%d\n", errno);
}

/********************************************************************************/
#elif VXWORKS | PSP

void 
MwSelect(BOOL mayWait)
{
	int mouseevents = 0;
	int keybdevents = 0;

	/* update screen & flush buffers*/
	if(scrdev.PreSelect)
		scrdev.PreSelect(&scrdev);

	/* If mouse data present, service it */
	while (mousedev.Poll() > 0)
	{
		MwCheckMouseEvent();
		if (mouseevents++ > 10)
			break;
	}
	
	
	/* If keyboard data present, service it */
	while (kbddev.Poll() > 0)
	{
		MwCheckKeyboardEvent();
		if (keybdevents++ > 10)
			break;
	}
	
	/* did we not process any input? if so, yield so we don't freeze system */
	if (mouseevents==0 && keybdevents==0)
		sceKernelDelayThread(100);

	MwHandleTimers();
}

/********************************************************************************/
#elif RTEMS | __ECOS

extern MWBOOL MwCheckMouseEvent();
extern MWBOOL MwCheckKeyboardEvent();
extern struct MW_UID_MESSAGE m_kbd;
extern struct MW_UID_MESSAGE m_mou;
extern HWND  dragwp;     /* window user is dragging*/

void MwSelect (BOOL mayWait)
{
        struct MW_UID_MESSAGE m;
	int rc;
	unsigned int timeout = 0;

	/* perform pre-select duties, if any*/
	if (scrdev.PreSelect)
		scrdev.PreSelect (&scrdev);

	/* Set up the timeout for the main select().
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
	
	/* wait up to 100 milisecons for events */
	rc = uid_read_message (&m, timeout);

	/* return if timed-out or something went wrong */
	if (rc < 0) {
	        if ( errno != ETIMEDOUT )
		        EPRINTF (" rc= %d, errno=%d\n", rc, errno);
		else {
			MwHandleTimers ();
		}
		return;
	}

	/* let's pass the event up to microwindows */
	switch (m.type) {
	case MV_UID_REL_POS:	/* Mouse or Touch Screen event */
	case MV_UID_ABS_POS:
	        m_mou = m;
		while (MwCheckMouseEvent ()) continue;
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
#else /*MSDOS | _MINIX | NDS | __MINGW32__ | ALLEGRO | EMSCRIPTEN*/

void
MwSelect(BOOL mayWait)
{
	/* update screen & flush buffers*/
	if(scrdev.PreSelect)
		scrdev.PreSelect(&scrdev);

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

	startTicks = GetTickCount();
	
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
	wp->hInstance = NULL;
	wp->nEraseBkGnd = 1;
	wp->paintBrush = NULL;
	wp->paintPen = NULL;

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
#if UNIX
	struct timeval t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 25000) * 25) - startTicks;
#elif MSDOS
	return (DWORD)(clock() * 1000 / CLOCKS_PER_SEC);
#elif _MINIX
	struct tms	t;
	
	return (DWORD)times(&t) * 16;
#elif __ECOS
  /* CYGNUM_HAL_RTC_NUMERATOR/CYGNUM_HAL_RTC_DENOMINATOR gives the length of one tick in nanoseconds */
   return (cyg_current_time()*(CYGNUM_HAL_RTC_NUMERATOR/CYGNUM_HAL_RTC_DENOMINATOR))/(1000*1000);
#else
	return 0L;
#endif
}

VOID WINAPI
Sleep(DWORD dwMilliseconds)
{
	int i, j;
	volatile int k;
	const int loops_per_ms = 20000;

	/* FIXME this is not calibrated */
	for(i=0; i < dwMilliseconds; i++)
		for(j=0; j < loops_per_ms; j++)
			k = i * j;
}
