/*
 * Copyright (c) 1999, 2000, 2001, 2003, 2004, 2010, 2019 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 1991 David I. Bell
 *
 * Main module of graphics server.
 */
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "uni_std.h"
#include "sys_time.h"

#if RTEMS
#include <rtems/mw_uid.h>
#endif

#define MWINCLUDECOLORS
#include "serv.h"
#include "osdep.h"

#if HAVE_VNCSERVER
#include "rfb/rfb.h"
extern rfbScreenInfoPtr rfbScreen;
extern int vnc_thread_fd;	/*  fd to be included in select */
#endif

/*
 * External definitions defined here.
 */
GR_WINDOW_ID	cachewindowid;		/* cached window id */
GR_WINDOW_ID    cachepixmapid;         /* cached pixmap id */
GR_GC_ID	cachegcid;		/* cached graphics context id */
GR_WINDOW	*cachewp;		/* cached window pointer */
GR_GC		*cachegcp;		/* cached graphics context */
GR_PIXMAP	*cachepp;               /* cached pixmap */
GR_PIXMAP	*listpp;                /* List of all pixmaps */
GR_WINDOW	*listwp;		/* list of all windows */
GR_WINDOW	*rootwp;		/* root window pointer */
GR_GC		*listgcp;		/* list of all gc */
GR_REGION	*listregionp;		/* list of all regions */
GR_FONT		*listfontp;		/* list of all fonts */
GR_CURSOR	*listcursorp;		/* list of all cursors */
GR_CURSOR	*stdcursor;		/* root window cursor */
GR_GC		*curgcp;		/* currently enabled gc */
GR_WINDOW	*clipwp;		/* window clipping is set for */
GR_WINDOW	*focuswp;		/* focus window for keyboard */
GR_WINDOW	*mousewp;		/* window mouse is currently in */
GR_WINDOW	*grabbuttonwp;		/* window grabbed by button */
GR_CURSOR	*curcursor;		/* currently enabled cursor */
GR_COORD	cursorx;		/* current x position of cursor */
GR_COORD	cursory;		/* current y position of cursor */
GR_BUTTON	curbuttons;		/* current state of buttons */
GR_CLIENT	*curclient;		/* client currently executing for */
GR_EVENT_LIST	*eventfree;		/* list of free events */
GR_BOOL		focusfixed;		/* TRUE if focus is fixed on a window */
PMWFONT		stdfont;		/* default font*/
char		*progname;		/* Name of this program.. */

int		current_fd;		/* the fd of the client talking to */
int		connectcount = 0;	/* number of connections to server */
GR_CLIENT	*root_client;		/* root entry of the client table */
char		*current_shm_cmds;
int			current_shm_cmds_size;
static int	keyb_fd;		/* the keyboard file descriptor */
static int	mouse_fd;		/* the mouse file descriptor */
char		*curfunc;		/* the name of the current server func*/
GR_BOOL		screensaver_active;	/* time before screensaver activates */
GR_SELECTIONOWNER selection_owner;	/* the selection owner and typelist */
int		autoportrait = FALSE;	/* auto portrait mode switching*/
MWCOORD		nxres;			/* requested server x resolution*/
MWCOORD		nyres;			/* requested server y resolution*/
GR_GRABBED_KEY  *list_grabbed_keys = NULL;     /* list of all grabbed keys */

#if MW_FEATURE_TIMERS
GR_TIMEOUT	screensaver_delay;	/* time before screensaver activates */
GR_TIMER_ID     cache_timer_id;         /* cached timer ID */
GR_TIMER        *cache_timer;           /* cached timer */
GR_TIMER        *list_timer;            /* list of all timers */
#endif
#if MW_FEATURE_TWO_KEYBOARDS
static int	keyb2_fd;		/* the keyboard file descriptor */
#endif

static int	persistent_mode = FALSE;
static int	portraitmode = MWPORTRAIT_NONE;

SERVER_LOCK_DECLARE /* Mutex for all public functions (only if NONETWORK and THREADSAFE) */

#if (_MSC_VER == 1500) && NONETWORK
int WinMain(int hInstance, int hPrevInstance, char *lpCmdLine, int nShowCmd)
{
	char *av[1] = { "nanox.exe" };
	int main(int ac, char **av);
	return main(1, av);
}
#endif

#if !NONETWORK
static int	Argc;
static char **	Argv;
int		un_sock;		/* the server socket descriptor */

static void
usage(void)
{
	EPRINTF("Usage: %s [-p] [-A] [-NLRD] [-x #] [-y #]"
#if FONTMAPPER
		" [-c <fontconfig-file>"
#endif
		" ...]\n", progname);
	exit(1);
}

#if ALLEGRO
int real_main(int ac, char **av);	/* ALLEGRO renamed main entry point*/
int main(int ac, char **av)
{
#define MAIN(ac,av)	real_main(ac,av)
	/* required to force allegro to run OSX API calls on the main thread*/
	extern int al_run_main(int ac, char **av, int (*main_ptr)());
	return al_run_main(ac, av, real_main);
}
#else
#define MAIN(ac,av)	main(ac,av)
#endif /* ALLEGRO*/

int
MAIN(int argc, char **argv)			/* ALLEGRO=real_main(), main() otherwise*/
{
	int t;

	progname = argv[0];

	t = 1;
	while ( t < argc ) {
		if ( !strcmp("-p",argv[t]) ) {
			persistent_mode = TRUE;
			++t;
			continue;
		}
		if ( !strcmp("-A",argv[t]) ) {
			autoportrait = TRUE;
			++t;
			continue;
		}
		if ( !strcmp("-N",argv[t]) ) {
			portraitmode = MWPORTRAIT_NONE;
			++t;
			continue;
		}
		if ( !strcmp("-L",argv[t]) ) {
			portraitmode = MWPORTRAIT_LEFT;
			++t;
			continue;
		}
		if ( !strcmp("-R",argv[t]) ) {
			portraitmode = MWPORTRAIT_RIGHT;
			++t;
			continue;
		}
		if ( !strcmp("-D",argv[t]) ) {
			portraitmode = MWPORTRAIT_DOWN;
			++t;
			continue;
		}
		if ( !strcmp("-x",argv[t]) ) {
			if (++t >= argc)
				usage();
			nxres = atoi(argv[t]);
			++t;
			continue;
		}
		if ( !strcmp("-y",argv[t]) ) {
			if (++t >= argc)
				usage();
			nyres = atoi(argv[t]);
			++t;
			continue;
		}
#if FONTMAPPER
		if ( !strcmp("-c",argv[t]) ) {
			int read_configfile(char *file);

			if (++t >= argc)
				usage();
			read_configfile(argv[t]);
			++t;
			continue;
		}
#endif
		usage();
	}

    Argc = argc;
    Argv = argv;
	/*
	 * This is the main server loop which initialises the server, services
	 * the clients, and shuts the server down when there are no more clients.
	 */
	if(GsInitialize() < 0)
		exit(1);

	while(1)
		GsSelect(GR_TIMEOUT_BLOCK);
	return 0;
}
#endif /* !NONETWORK*/

void
GsAcceptClientFd(int i)
{
	GR_CLIENT *client, *cl;

	if(!(client = malloc(sizeof(GR_CLIENT)))) {
#if !NONETWORK
		close(i);
#endif
		return;
	}

	client->id = i;
	client->eventhead = NULL;
	client->eventtail = NULL;
	/*client->errorevent.type = GR_EVENT_TYPE_NONE;*/
	client->next = NULL;
	client->prev = NULL;
	client->waiting_for_event = FALSE;
	client->shm_cmds = 0;

	if(connectcount++ == 0)
		root_client = client;
	else {
		cl = root_client;
			while(cl->next)
				cl = cl->next;
		client->prev = cl;
		cl->next = client;
	}
}

/*
 * Open a connection from a new client to the server.
 * Returns -1 on failure.
 */
int
GrOpen(void)
{
	GdPlatformInit();			/* platform-specific initialization*/

#if NONETWORK
	SERVER_LOCK();

	/* Client calls this routine once.  We init everything here*/
	if (connectcount <= 0) {
		if(GsInitialize() < 0) {
			SERVER_UNLOCK();
			return -1;
		}
		GsAcceptClientFd(999);
		curclient = root_client;
	}
	SERVER_UNLOCK();

#if MSDOS
	atexit(GsTerminate);
#endif
#endif /* NONETWORK*/

#if NANOWM
	wm_init();	/* init built-in window manager*/
#endif
    return 1;
}

/*
 * Close the current connection to the server.
 */
void
GrClose(void)
{
	SERVER_LOCK();
	GsClose(current_fd);
	SERVER_UNLOCK();
}

/*
 * Drop a specific server connection.
 */
void
GsClose(int fd)
{
	GsDropClient(fd);
	if(!persistent_mode && connectcount == 0)
		GsTerminate();
}

#if NONETWORK
/* client/server GsDropClient is in srvnet.c*/
void
GsDropClient(int fd)
{
	--connectcount;
}
#endif

#if UNIX && HAVE_SELECT && NONETWORK
/*
 * Register the specified file descriptor to return an event
 * when input is ready.
 */

static int regfdmax = -1;
static fd_set regfdset;

void
GrRegisterInput(int fd)
{
	SERVER_LOCK();
	FD_SET(fd, &regfdset);
	if (fd >= regfdmax) regfdmax = fd + 1;
	SERVER_UNLOCK();
}

void
GrUnregisterInput(int fd)
{
	int i, max;

	SERVER_LOCK();

	/* unregister all inputs if the FD is -1 */
	if (fd == -1) {
		FD_ZERO(&regfdset);
		regfdmax = -1;
		SERVER_UNLOCK();
		return;
	}

	FD_CLR(fd, &regfdset);
	/* recalculate the max file descriptor */
	for (i = 0, max = regfdmax, regfdmax = -1; i < max; i++)
		if (FD_ISSET(i, &regfdset))
			regfdmax = i + 1;

	SERVER_UNLOCK();
}

#endif /* UNIX && HAVE_SELECT && NONETWORK*/

/********************************************************************************/
#if UNIX && HAVE_SELECT

#if NONETWORK
int
GsPumpEvents(void)
{
	/* update mouse and keyboard events */
	if (scrdev.PreSelect)
	{
		/* returns # pending events*/
		if (scrdev.PreSelect(&scrdev))
		{
			/* poll for mouse data and service if found*/
			while (GsCheckMouseEvent())
				continue;

			/* poll for keyboard data and service if found*/
			while (GsCheckKeyboardEvent())
				continue;

			return 1;
		}
	}
    return 0;
}
#endif

void
GsSelect(GR_TIMEOUT timeout)
{
	fd_set	rfds;
	int 	e;
	int	setsize = 0;
	struct timeval tout;
	struct timeval *to;
#if NONETWORK
	int	fd;
#endif
#if HAVE_VNCSERVER 
#if VNCSERVER_PTHREADED
        int dummy;
#else
        rfbClientIteratorPtr i;
        rfbClientPtr cl;
#endif 
#endif

	/* X11/SDL perform single update of aggregate screen update region*/
	if (scrdev.PreSelect)
	{
		/* returns # pending events*/
		if (scrdev.PreSelect(&scrdev))
		{
			/* poll for mouse data and service if found*/
			while (GsCheckMouseEvent())
				continue;

			/* poll for keyboard data and service if found*/
			while (GsCheckKeyboardEvent())
				continue;

			/* if events found, don't return unless polling, events handled below*/
			if (timeout != GR_TIMEOUT_BLOCK)
				return;
		}
	}

	/* Set up the FDs for use in the main select(): */
	FD_ZERO(&rfds);
	if(mouse_fd >= 0)
	{
		FD_SET(mouse_fd, &rfds);
		if (mouse_fd > setsize)
			setsize = mouse_fd;
	}
	if(keyb_fd >= 0)
	{
		FD_SET(keyb_fd, &rfds);
		if (keyb_fd > setsize)
			setsize = keyb_fd;
	}
#if MW_FEATURE_TWO_KEYBOARDS
	if(keyb2_fd >= 0)
	{
		FD_SET(keyb2_fd, &rfds);
		if (keyb2_fd > setsize)
			setsize = keyb2_fd;
	}
#endif
#if NONETWORK
	/* handle registered input file descriptors*/
	for (fd = 0; fd < regfdmax; fd++)
	{
		if (!FD_ISSET(fd, &regfdset))
			continue;

		FD_SET(fd, &rfds);
		if (fd > setsize) setsize = fd;
	}
#else /* !NONETWORK */
	/* handle client socket connections*/
	FD_SET(un_sock, &rfds);
	if (un_sock > setsize) setsize = un_sock;
	curclient = root_client;
	while(curclient)
	{
		if(curclient->waiting_for_event && curclient->eventhead)
		{
			curclient->waiting_for_event = FALSE;
			GrGetNextEventWrapperFinish(curclient->id);
			return;
		}
		FD_SET(curclient->id, &rfds);
		if(curclient->id > setsize) setsize = curclient->id;
		curclient = curclient->next;
	}
#endif /* NONETWORK */

#if HAVE_VNCSERVER 
#if VNCSERVER_PTHREADED
	/* Add file vnc thread fd. This is useful to force handling of events generated by the VNC thread*/
	FD_SET( vnc_thread_fd, &(rfds) );
	if ( vnc_thread_fd > setsize )
		setsize = vnc_thread_fd;
#else
        /* Add all VNC open sockets to nano-X select set */
        FD_SET( rfbScreen->listenSock, &(rfds) );
        if ( rfbScreen->listenSock > setsize )
                setsize = rfbScreen->listenSock;

        FD_SET( rfbScreen->httpListenSock, &(rfds) );
        if ( rfbScreen->httpListenSock > setsize )
                setsize = rfbScreen->httpListenSock;

        i = rfbGetClientIterator(rfbScreen);
        cl = rfbClientIteratorNext(i);

        while ( cl ) {
                if ( cl->sock >= 0 ) {
                        FD_SET( cl->sock, &(rfds) );
                        if ( cl->sock > setsize )
                                setsize = cl->sock;

                }
                cl = rfbClientIteratorNext(i);
        }
        rfbReleaseClientIterator(i);
#endif
#endif /* HAVE_VNCSERVER*/


	/* setup timeval struct for block or poll in select()*/
	tout.tv_sec = tout.tv_usec = 0;					/* setup for assumed poll*/
	to = &tout;
	int poll = (timeout == GR_TIMEOUT_POLL);
	if (!poll)
	{
#if MW_FEATURE_TIMERS
		/* get next timer or use passed timeout and convert to timeval struct*/
		if (!GdGetNextTimeout(&tout, timeout))		/* no app timers or VTSWITCH?*/
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
#define WAITTIME	5000
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
#if NONETWORK
again:
	SERVER_UNLOCK();	/* allow other threads to run*/
#endif
	e = select(setsize+1, &rfds, NULL, NULL, to);
#if NONETWORK
	SERVER_LOCK();
#endif
	if(e > 0)			/* input ready*/
	{
		/* service mouse file descriptor*/
		if(mouse_fd >= 0 && FD_ISSET(mouse_fd, &rfds))
			while(GsCheckMouseEvent())
				continue;

		/* service keyboard file descriptor*/
		if( (keyb_fd >= 0 && FD_ISSET(keyb_fd, &rfds))
#if MW_FEATURE_TWO_KEYBOARDS
		    || (keyb2_fd >= 0 && FD_ISSET(keyb2_fd, &rfds))
#endif
		  )
			while(GsCheckKeyboardEvent())
				continue;

#if HAVE_VNCSERVER && VNCSERVER_PTHREADED
        if(vnc_thread_fd >= 0 && FD_ISSET(vnc_thread_fd, &rfds))
            /* Read from vnc pipe */
            read( vnc_thread_fd, &dummy, sizeof(int));

#endif
#if NONETWORK
		/* check for input on registered file descriptors */
		for (fd = 0; fd < regfdmax; fd++)
		{
			GR_EVENT_FDINPUT *	gp;

			if (!FD_ISSET(fd, &regfdset)  ||  !FD_ISSET(fd, &rfds))
				continue;

			gp = (GR_EVENT_FDINPUT *)GsAllocEvent(curclient);
			if(gp) {
				gp->type = GR_EVENT_TYPE_FDINPUT;
				gp->fd = fd;
			}
		}
#else /* !NONETWORK */

		/* If a client is trying to connect, accept it: */
		if(FD_ISSET(un_sock, &rfds))
			GsAcceptClient();

		/* If a client is sending us a command, handle it: */
		curclient = root_client;
		while (curclient)
		{
			GR_CLIENT *curclient_next;

			/* curclient may be freed in GsDropClient*/
			curclient_next = curclient->next;
			if(FD_ISSET(curclient->id, &rfds))
				GsHandleClient(curclient->id);
			curclient = curclient_next;
		}

#if HAVE_VNCSERVER && !VNCSERVER_PTHREADED
		rfbProcessEvents(rfbScreen, 0);
#endif
		
#endif /* NONETWORK */
	} 
	else if (e == 0)		/* timeout*/
	{
#if NONETWORK
		/* 
		 * Timeout has occured. Currently return a timeout event
		 * regardless of whether client has selected for it.
		 * Note: this will be changed back to GR_EVENT_TYPE_NONE
		 * for the GrCheckNextEvent/LINK_APP_TO_SERVER case
		 */
#if MW_FEATURE_TIMERS
		if(GdTimeout())
#endif
		{
			GR_EVENT_GENERAL *	gp;
			if ((gp = (GR_EVENT_GENERAL *)GsAllocEvent(curclient)) != NULL)
				gp->type = GR_EVENT_TYPE_TIMEOUT;
		}
		else if(!poll && timeout && (scrdev.flags & PSF_CANTBLOCK)) {
			if (!GsPumpEvents())    /* process mouse/kbd events */
				goto again;		/* retry until passed timeout */
		}
#else /* !NONETWORK */
#if MW_FEATURE_TIMERS
		/* check for timer timeouts and service if found*/
		GdTimeout();
#endif
#endif /* NONETWORK */
	} else if(errno != EINTR)
		EPRINTF("Select() call in main failed\n");
}

/********************************************************************************/
#elif RTEMS
extern struct MW_UID_MESSAGE m_kbd;
extern struct MW_UID_MESSAGE m_mou;

void
GsSelect (GR_TIMEOUT timeout)
{
	struct MW_UID_MESSAGE m;
	long uid_timeout;
	GR_EVENT_GENERAL *gp;
	int rc;
#if MW_FEATURE_TIMERS
	struct timeval tout;
#endif

	/* perform pre-select duties, if any*/
	if (scrdev.PreSelect)
		scrdev.PreSelect(&scrdev);

	/* let's make sure that the type is invalid */
	m.type = MV_UID_INVALID;

	/* wait up for events */
	if (timeout == GR_TIMEOUT_POLL)
		uid_timeout = 0;
	else {
#if MW_FEATURE_TIMERS
		if (GdGetNextTimeout(&tout, timeout))
			uid_timeout = tout.tv_sec * 1000 + (tout.tv_usec + 500) / 1000;
		else
#endif
		{
			if (timeout == 0)
				uid_timeout = (unsigned long) -1;
			else
				uid_timeout = timeout;
		}
	}
	rc = uid_read_message (&m, uid_timeout);

	/* return if timed-out or something went wrong */
	if (rc < 0)
	{
		if (errno != ETIMEDOUT)
				EPRINTF(" rc= %d, errno=%d\n", rc, errno);
		else
		{
#if MW_FEATURE_TIMERS
			/* check for timer timeouts and service if found*/
			if (GdTimeout())
#else
			if (timeout != 0)
#endif
			{
				/*
		 		 * Timeout has occured. Currently return a timeout event
		 		 * regardless of whether client has selected for it.
				 */
				if ((gp = (GR_EVENT_GENERAL *)GsAllocEvent(curclient)) != NULL)
					gp->type = GR_EVENT_TYPE_TIMEOUT;
			}
		}
		return;
	}

	/* let's pass the event up to Microwindows */
	switch (m.type) {
	case MV_UID_REL_POS:	/* Mouse or Touch Screen event */
	case MV_UID_ABS_POS:
		m_mou = m;
		while (GsCheckMouseEvent())
			continue;
		break;
	case MV_UID_KBD:	/* KBD event */
		m_kbd = m;
		GsCheckKeyboardEvent ();
		break;
	case MV_UID_TIMER:	/* Microwindows does nothing with these.. */
	case MV_UID_INVALID:
	default:
	        break;
	}
}

/********************************************************************************/
#else /* MSDOS | _MINIX | NDS | VXWORKS | PSP | __MINGW32__ | ALLEGRO | EMSCRIPTEN*/

/* this GsSelect() is used for all polling-based platforms not specially handled above*/
#define WAITTIME	50		/* blocking sleep interval in msecs unless polling*/

void 
GsSelect(GR_TIMEOUT timeout)
{
	int numevents = 0;
	GR_TIMEOUT waittime = 0;
	GR_EVENT_GENERAL *gp;

#if MW_FEATURE_TIMERS
	struct timeval tout;
	if (timeout != GR_TIMEOUT_POLL)
		GdGetNextTimeout(&tout, timeout);	/* set initial mainloop timeout*/
#endif
#if EMSCRIPTEN
	if (timeout == GR_TIMEOUT_POLL)
		timeout = 1;				/* need to give up some CPU in GdDelay even on poll*/
#endif
	/* input gathering loop */
	while (1)
	{
		/* perform single update of aggregate screen update region*/
		if(scrdev.PreSelect)
			scrdev.PreSelect(&scrdev);

		/* poll for mouse data and service if found*/
		while (GsCheckMouseEvent())
			if (++numevents > 10)
				break;				/* don't handle too many events at one shot*/

		/* poll for keyboard data and service if found*/
		while (GsCheckKeyboardEvent())
			if (++numevents > 10)
				break;				/* don't handle too many events at one shot*/
		
		/* did we handle any input or were we just polling?*/
		if (numevents || timeout == GR_TIMEOUT_POLL)
			return;					/* yes - return without sleeping*/

		/* give up time-slice & sleep for a bit */
		GdDelay(WAITTIME);
		waittime += WAITTIME; 

		/* have we timed out? */
		if (waittime >= timeout)
		{
#if MW_FEATURE_TIMERS
			/* check for timer timeouts and service if found*/
			if (GdTimeout() || timeout != 0)
#else
			/* special case: polling when timeout == 0 -- don't send timeout event */
			if (timeout != 0)
#endif
			{
				/* Timeout has occured.  
				 * Currently return a timeout event regardless of whether client 
				 * has selected for it.
				 */
				if ((gp = (GR_EVENT_GENERAL *)GsAllocEvent(curclient)) != NULL)
					gp->type = GR_EVENT_TYPE_TIMEOUT;
			}
			return;
		}
	}
}

/********************************************************************************/
#endif /* GsSelect() cases*/

#if UNIX && HAVE_SELECT && NONETWORK
/*
 * Prepare for the client to call select().  Asks the server to send the next
 * event but does not wait around for it to arrive.  Initializes the
 * specified fd_set structure with the client/server socket descriptor and any
 * previously registered external file descriptors.  Also compares the current
 * contents of maxfd, the client/server socket descriptor, and the previously
 * registered external file descriptors, and returns the highest of them in
 * maxfd.
 *
 * Usually used in conjunction with GrServiceSelect().
 *
 * Note that in a multithreaded client, the application must ensure that
 * no Nano-X calls are made between the calls to GrPrepareSelect() and
 * GrServiceSelect(), else there will be race conditions.
 *
 * @param maxfd  Pointer to a variable which the highest in use fd will be
 *               written to.  Must contain a valid value on input - will only
 *               be overwritten if the new value is higher than the old
 *               value.
 * @param rfdset Pointer to the file descriptor set structure to use.  Must
 *               be valid on input - file descriptors will be added to this
 *               set without clearing the previous contents.
 */
void
GrPrepareSelect(int *maxfd, void *rfdset)
{
	fd_set *rfds = (fd_set *) rfdset;
	int fd;

	SERVER_LOCK();

	/* update screen & flush buffers*/
	if(rootwp->psd->PreSelect)
		rootwp->psd->PreSelect(rootwp->psd);

	if(mouse_fd >= 0) {
		FD_SET(mouse_fd, rfds);
		if (mouse_fd > *maxfd)
			*maxfd = mouse_fd;
	}
	if(keyb_fd >= 0) {
		FD_SET(keyb_fd, rfds);
		if (keyb_fd > *maxfd)
			*maxfd = keyb_fd;
	}
#if MW_FEATURE_TWO_KEYBOARDS
	if(keyb2_fd >= 0) {
		FD_SET(keyb2_fd, rfds);
		if (keyb2_fd > *maxfd)
			*maxfd = keyb2_fd;
	}
#endif

	/* handle registered input file descriptors*/
	for (fd = 0; fd < regfdmax; fd++) {
		if (!FD_ISSET(fd, &regfdset))
			continue;

		FD_SET(fd, rfds);
		if (fd > *maxfd)
			*maxfd = fd;
	}

	SERVER_UNLOCK();
}

/*
 * Handles events after the client has done a select() call.
 *
 * Calls the specified callback function is an event has arrived, or if
 * there is data waiting on an external fd specified by GrRegisterInput().
 *
 * Used by GrMainLoop().
 *
 * @param rfdset Pointer to the file descriptor set containing those file
 *               descriptors that are ready for reading.
 * @param fncb   Pointer to the function to call when an event needs handling.
 */
void
GrServiceSelect(void *rfdset, GR_FNCALLBACKEVENT fncb)
{
	fd_set *	rfds = rfdset;
	GR_EVENT_LIST *	elp;
	GR_EVENT 	ev;
	int fd;

	SERVER_LOCK();

	/* If data is present on the mouse fd, service it: */
	if(mouse_fd >= 0 && FD_ISSET(mouse_fd, rfds))
		while(GsCheckMouseEvent())
			continue;

	/* If data is present on the keyboard fd, service it: */
	if( (keyb_fd >= 0 && FD_ISSET(keyb_fd, rfds))
#if MW_FEATURE_TWO_KEYBOARDS
	 || (keyb2_fd >= 0 && FD_ISSET(keyb2_fd, rfds))
#endif
	  )
		while(GsCheckKeyboardEvent())
			continue;

	/* Dispatch all queued events */
	while((elp = curclient->eventhead) != NULL) {

		ev = elp->event;

		/* Remove first event from queue*/
		curclient->eventhead = elp->next;
		if (curclient->eventtail == elp)
			curclient->eventtail = NULL;

		elp->next = eventfree;
		eventfree = elp;

		fncb(&ev);
	}

	/* check for input on registered file descriptors */
	for (fd = 0; fd < regfdmax; fd++) {
		if (!FD_ISSET(fd, &regfdset) || !FD_ISSET(fd, rfds))
			continue;

		ev.type = GR_EVENT_TYPE_FDINPUT;
		ev.fdinput.fd = fd;
		fncb(&ev);
	}

	SERVER_UNLOCK();
}
#endif /* UNIX && HAVESELECT && NONETWORK*/

#if VTSWITCH
static void
CheckVtChange(void *arg)
{
	if(MwCheckVtChange())
		GsRedrawScreen();
	GdAddTimer(50, CheckVtChange, NULL);
}
#endif
  
/*
 * Initialize the graphics and mouse devices at startup.
 * Returns nonzero with a message printed if the initialization failed.
 */
int
GsInitialize(void)
{
	GR_WINDOW	*wp;		/* root window */
	PSD		psd;
	GR_CURSOR_ID	cid;
	static const MWIMAGEBITS cursorbits[16] = {
	      0xe000, 0x9800, 0x8600, 0x4180,
	      0x4060, 0x2018, 0x2004, 0x107c,
	      0x1020, 0x0910, 0x0988, 0x0544,
	      0x0522, 0x0211, 0x000a, 0x0004
	};
	static const MWIMAGEBITS cursormask[16] = {
	      0xe000, 0xf800, 0xfe00, 0x7f80,
	      0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
	      0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
	      0x073e, 0x021f, 0x000e, 0x0004
	};

	/* If needed, initialize the server mutex. */
	SERVER_LOCK_INIT();

	wp = (GR_WINDOW *) malloc(sizeof(GR_WINDOW));
	if (wp == NULL) {
		EPRINTF("Cannot allocate root window\n");
		return -1;
	}

#if HAVE_SIGNAL
	/* catch terminate signal to restore tty state*/
	signal(SIGTERM, (void *)GsTerminate);
#endif

#if MW_FEATURE_TIMERS
	screensaver_delay = 0;
#endif
	screensaver_active = GR_FALSE;

	selection_owner.wid = 0;
	selection_owner.typelist = NULL;

#if !NONETWORK
#if HAVE_SIGNAL
	/* ignore pipe signal, sent when clients exit*/
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
#endif

	if (GsOpenSocket() < 0) {
		EPRINTF("Cannot bind to named socket\n");
		free(wp);
		return -1;
	}
#endif

	if ((keyb_fd = GdOpenKeyboard()) == -1) {
		EPRINTF("Cannot initialise keyboard\n");
		/*GsCloseSocket();*/
		free(wp);
		return -1;
	}

#if MW_FEATURE_TWO_KEYBOARDS
	if ((keyb2_fd = GdOpenKeyboard2()) == -1) {
		EPRINTF("Cannot initialise second keyboard\n");
		/*GsCloseSocket();*/
		free(wp);
		return -1;
	}
#endif

	if ((psd = GdOpenScreen()) == NULL) {
		EPRINTF("Cannot initialise screen\n");
		/*GsCloseSocket();*/
		GdCloseKeyboard();
		free(wp);
		return -1;
	}
	GdSetPortraitMode(psd, portraitmode);

	if ((mouse_fd = GdOpenMouse()) == -1) {
		EPRINTF("Cannot initialise mouse\n");
		/*GsCloseSocket();*/
		GdCloseScreen(psd);
		GdCloseKeyboard();
		free(wp);
		return -1;
	}

#if HAVE_VNCSERVER
        if (!GdOpenVNC(psd, Argc, Argv)) {
                EPRINTF("Cannot open VNC Socket\n");
                GdCloseMouse();
                GdCloseScreen(psd);
                GdCloseKeyboard();
                free(wp);
                return -1;
        }
#endif        
	/*
	 * Create std font.
	 */
#if (HAVE_BIG5_SUPPORT | HAVE_GB2312_SUPPORT | HAVE_JISX0213_SUPPORT | HAVE_KSC5601_SUPPORT)
	/* system fixed font looks better when mixed with builtin fixed fonts*/
	stdfont = GdCreateFont(psd, MWFONT_SYSTEM_FIXED, 0, 0, NULL);
#else
	stdfont = GdCreateFont(psd, MWFONT_SYSTEM_VAR, 0, 0, NULL);
#endif

	/*
	 * Initialize the root window.
	 */
	wp->x = 0;
	wp->y = 0;
	wp->width = psd->xvirtres;
	wp->height = psd->yvirtres;
	wp->psd = psd;
	wp->id = GR_ROOT_WINDOW_ID;
	wp->next = NULL;
	wp->owner = NULL;
	wp->parent = NULL;
	wp->children = NULL;
	wp->siblings = NULL;
	wp->bordersize = 0;
	wp->background = BLACK;
	wp->bordercolor = wp->background;
	wp->bgpixmap = NULL;
	wp->bgpixmapflags = GR_BACKGROUND_TILE;
	wp->nopropmask = 0;
	wp->eventclients = NULL;
	wp->cursorid = 0;
	wp->mapped = GR_TRUE;
	wp->realized = GR_TRUE;
	wp->output = GR_TRUE;
	wp->props = 0;
	wp->title = NULL;
	wp->clipregion = NULL;
	wp->buffer = NULL;

	listpp = NULL;
	listwp = wp;
	rootwp = wp;
	focuswp = wp;
	mousewp = wp;
	focusfixed = GR_FALSE;

	/*
	 * Initialize and position the default cursor.
	 */
	curcursor = NULL;
	cursorx = -1;
	cursory = -1;
	GdShowCursor(psd);
	GrMoveCursor(psd->xvirtres / 2, psd->yvirtres / 2);
	cid = GrNewCursor(16, 16, 0, 0, WHITE, BLACK, (MWIMAGEBITS *)cursorbits,
				(MWIMAGEBITS *)cursormask);
	GrSetWindowCursor(GR_ROOT_WINDOW_ID, cid);
	stdcursor = GsFindCursor(cid);

#if VTSWITCH
	MwInitVt();
	/* Check for VT change every 50 ms: */
	GdAddTimer(50, CheckVtChange, NULL);
#endif
	psd->FillRect(psd, 0, 0, psd->xvirtres-1, psd->yvirtres-1,
		GdFindColor(psd, wp->background));

	/*
	 * Tell the mouse driver some things.
	 */
	curbuttons = 0;
	GdRestrictMouse(0, 0, psd->xvirtres - 1, psd->yvirtres - 1);
	GdMoveMouse(psd->xvirtres / 2, psd->yvirtres / 2);

	/* Force root window screen paint*/
	GsRedrawScreen();

	/* 
	 * Force the cursor to appear on the screen at startup.
	 * (not required with above GsRedrawScreen)
	GdHideCursor(psd);
	GdShowCursor(psd);
	 */

	/*
	 * All done.
	 */
	connectcount = 0;
	return 0;
}

/*
 * Here to close down the server.
 */
void
GsTerminate(void)
{
#if !NONETWORK
	GsCloseSocket();
#endif

#if HAVE_VNCSERVER
	GdCloseVNC();
#endif

	GdCloseScreen(rootwp->psd);
	GdCloseMouse();
	GdCloseKeyboard();
#if VTSWITCH
	MwRedrawVt(mwvterm);
#endif
	_exit(0);
}
