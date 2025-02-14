/*
 * Nano-X server routines for UNIX && HAVE_SELECT
 */

static int regfdmax = -1;
static fd_set regfdset;

/*
 * Suspend execution of the program for the specified number of milliseconds.
 */
void
GdDelay(MWTIMEOUT msecs)
{
	struct timeval timeval;

	timeval.tv_sec = msecs / 1000;
	timeval.tv_usec = (msecs % 1000) * 1000;
	select(0, NULL, NULL, NULL, &timeval);
}

void
GsSelect(GR_TIMEOUT timeout)
{
	fd_set	rfds;
	int 	e;
	int	setsize = 0;
	struct timeval tout;
	struct timeval *to;
	int	fd;

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

			/* events found, return with no sleep*/
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

	/* handle registered input file descriptors*/
	for (fd = 0; fd < regfdmax; fd++)
	{
		if (!FD_ISSET(fd, &regfdset))
			continue;

		FD_SET(fd, &rfds);
		if (fd > setsize) setsize = fd;
	}

	/* setup timeval struct for block or poll in select()*/
	tout.tv_sec = tout.tv_usec = 0;					/* setup for assumed poll*/
	to = &tout;
	int poll = (timeout == (GR_TIMEOUT) -1L);		/* timeout = -1 means just poll*/
	if (!poll)
	{
		if (timeout)								/* setup mwin poll timer*/
		{
			/* convert wait timeout to timeval struct*/
			tout.tv_sec = timeout / 1000;
			tout.tv_usec = (timeout % 1000) * 1000;
		}
		else
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
	SERVER_UNLOCK();	/* allow other threads to run*/
	e = select(setsize+1, &rfds, NULL, NULL, to);
	SERVER_LOCK();
	if(e > 0)			/* input ready*/
	{
		/* service mouse file descriptor*/
		if(mouse_fd >= 0 && FD_ISSET(mouse_fd, &rfds))
			while(GsCheckMouseEvent())
				continue;

		/* service keyboard file descriptor*/
		if( (keyb_fd >= 0 && FD_ISSET(keyb_fd, &rfds)))
			while(GsCheckKeyboardEvent())
				continue;

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
	} 
	else if (e == 0)		/* timeout*/
	{
		/* 
		 * Timeout has occured. Currently return a timeout event
		 * regardless of whether client has selected for it.
		 * Note: this will be changed back to GR_EVENT_TYPE_NONE
		 * for the GrCheckNextEvent/LINK_APP_TO_SERVER case
		 */
		{
			GR_EVENT_GENERAL *	gp;
			if ((gp = (GR_EVENT_GENERAL *)GsAllocEvent(curclient)) != NULL)
				gp->type = GR_EVENT_TYPE_TIMEOUT;
		}
	}
}

/*** NONETWORK routines follow*/
/*
 * Register the specified file descriptor to return an event
 * when input is ready.
 */

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
	if( (keyb_fd >= 0 && FD_ISSET(keyb_fd, rfds)))
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
