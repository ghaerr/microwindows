/*
 * Copyright (c) 1999, 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002, 2003 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 1999, 2000 Alex Holden <alex@linuxhacker.org>
 * Copyright (c) 1991 David I. Bell
 * Copyright (c) 2000 Vidar Hokstad
 * Copyright (c) 2000 Morten Rolland <mortenro@screenmedia.no>
 *
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Client routines to do graphics with windows and graphics contexts.
 *
 * Rewritten heavily for speed by Greg Haerr
 *
 * Whenever you add a new API entry point, please comment it in the same way
 * as the rest of the functions in this file. Also add your functions to the
 * appropriate section(s) in doc/nano-X/nano-X-sections.txt and regenerate the
 * documentation by running make in the doc/nano-X/ directory. If you do not
 * have the necessary tools (gtk-doc and the docbook-tools) to rebuild the
 * documentation, just skip that step and we will do it for you.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#if HAVE_SHAREDMEM_SUPPORT
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include <sys/time.h>
#include <sys/socket.h>
#if ELKS
#include <linuxmt/na.h>
#include <linuxmt/time.h>
#define ADDR_FAM AF_NANO
#elif __ECOS
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <cyg/kernel/kapi.h>
#define ADDR_FAM AF_INET
#define _NO_SVR_MAPPING
#define getpid()	((int)cyg_thread_self())
#else
#include <sys/un.h>
#if hpux
#include <sys/time.h>
#else
#ifdef __GLIBC__
#include <sys/select.h>
#endif
#endif
#endif
#include "nano-X.h"
#include "serv.h"
#include "nxproto.h"
#include "lock.h"

#ifndef ADDR_FAM
/**
 * Default to unix socket for client/server.
 * @internal
 */
#define ADDR_FAM AF_UNIX
#endif

#define GR_CLOSE_FIX	1	/* dirty hack attempts to fix GrClose hang bug*/

/**
 * The granularity of the shared memory allocation.  The requested size
 * will be rounded up to a multiple of this many bytes.
 *
 * @internal
 */
#define SHM_BLOCK_SIZE	4096

#ifndef __ECOS
/* exported global data */
int 	   nxSocket = -1;	/* The network socket descriptor */
LOCK_DECLARE(nxGlobalLock);	/* global lock for threads safety*/
#if HAVE_SHAREDMEM_SUPPORT
char *	   nxSharedMem = 0;	/* Address of shared memory segment*/
static int nxSharedMemSize;	/* Size in bytes of shared mem segment*/
#endif

static int regfdmax = -1;	/* GrRegisterInput globals*/
static fd_set regfdset;

/**
 * Human-readable error strings.
 */
char *nxErrorStrings[] = {
	GR_ERROR_STRINGS
};

static EVENT_LIST *	evlist;

/*
 * The following is the user defined function for handling errors.
 * If this is not set, then the default action is to close the connection
 * to the server, describe the error, and then exit.  This error function
 * will only be called when the client asks for events.
 */
static GR_FNCALLBACKEVENT ErrorFunc = GrDefaultErrorHandler;

#else /* __ECOS*/
/*
 * eCos uses a thread data pointer to store all statics in...
 */
int ecos_nanox_client_data_index = CYGNUM_KERNEL_THREADS_DATA_MAX;

#endif

static void QueueEvent(GR_EVENT *ep);
static void GetNextQueuedEvent(GR_EVENT *ep);
static void _GrGetNextEventTimeout(GR_EVENT *ep, GR_TIMEOUT timeout);
static int  _GrPeekEvent(GR_EVENT * ep);

/**
 * Read n bytes of data from the server into block *b.  Make sure the data
 * you are about to read are actually of the correct type - e.g. make a
 * check for the first block of data you read as a response to a command
 * with the Typed version of this function.
 *
 * @param b Destination for read.
 * @param n Number of bytes to read.
 * @return  0 on success or -1 on failure.
 * @internal
 */
static int
ReadBlock(void *b, int n)
{
	int i = 0;
	char *v;
        ACCESS_PER_THREAD_DATA()

	v = (char *) b;

	nxFlushReq(0L,0);
	while(v < ((char *) b + n)) {
		i = read(nxSocket, v, ((char *) b + n - v));
		if ( i <= 0 ) {
			if ( i == 0 ) {
				/* We should maybe produce an event here,
				 * if possible.
				 */
				EPRINTF("nxclient: lost connection to Nano-X "
					"server\n");
				exit(1);
			}
			if ( errno == EINTR || errno == EAGAIN )
				continue;

			EPRINTF("nxclient: bad readblock %d, errno %d\n", i, errno);
			return -1;
		}
		v += i;
	}

	return 0;
}

/**
 * Read a byte of data from the server.
 *
 * @return The unsigned character read from the server, or -1 on error.
 *
 * @internal
 */
static int
ReadByte(void)
{
	unsigned char c;

	if(ReadBlock(&c, 1) == -1)
		return -1;
	else return (int) c;
}

/**
 * Check if this is a CLIENT_DATA event, in which case we need to read the
 * data for the event into a buffer and set the event data pointer to the
 * address of it (or NULL if the malloc() fails). We also don't try to read
 * any data if datalen is 0.
 *
 * @param evp Event to check
 *
 * @internal
 */
static void
CheckForClientData(GR_EVENT *evp)
{
	GR_EVENT_CLIENT_DATA *event;

	if(evp->type == GR_EVENT_TYPE_CLIENT_DATA) {
		event = (GR_EVENT_CLIENT_DATA *)evp;
		if(!event->datalen) {
			event->data = NULL;
			return;
		}
		if(!(event->data = malloc(event->datalen))) return;
		ReadBlock(event->data, event->datalen);
	}
}

/**
 * Check if the data we are about to read is of the correct type. This
 * must be done in order to avoid reading an event as part of the response
 * from the server to a command that requires a reply.
 *
 * @param packettype Expected type.
 * @return           packettype on success, -1 on error.
 *
 * @internal
 */
static int
CheckBlockType(short packettype)
{
	short		b;
	GR_EVENT	event;

	while (ReadBlock(&b,sizeof(b)) != -1) {
		if (b == packettype)
			return b;

		if (b == GrNumGetNextEvent) {
			/*DPRINTF("nxclient %d: Storing event (expected %d)\n",
				getpid(), packettype);*/

			/* read event and queue it for later processing*/
			ReadBlock(&event, sizeof(event));
			CheckForClientData(&event);
			QueueEvent(&event);
		} else {
			EPRINTF("nxclient %d: Wrong packet type %d "
				"(expected %d)\n", getpid(),b, packettype);
		}
	}
	EPRINTF("nxclient %d: Corrupted packet\n", getpid());
	return -1;
}

/**
 * Actually read a response from the server, much like the ReadBlock but
 * make sure the response is of the right kind, e.g. store the event that
 * may have sneaked into the stream.
 *
 * @param b    Destination for read data.
 * @param n    Number of bytes to read.
 * @param type The required packet type.
 * @return     0 on success or -1 on failure.
 *
 * @internal
 */
static int
TypedReadBlock(void *b, int n, int type)
{
	int r;
   
	r = CheckBlockType(type);
	if (r != type)
		return -1;
	return ReadBlock(b, n);
}

/**
 * Check if the passed event is an error event, and call the error handler if
 * there is one. After calling the handler (if it returns), the event type is
 * set to a non-event so that we don't return an error event through the
 * GetEvent() mechanism. This solution is simpler than creating a client-side
 * event queue.
 *
 * @param ep The event to check.
 *
 * @internal
 */
static void
CheckErrorEvent(GR_EVENT *ep)
{
        ACCESS_PER_THREAD_DATA()

	if (ep->type == GR_EVENT_TYPE_ERROR) {
	    if (ErrorFunc) {
			/* call error handler*/
			ErrorFunc(ep);

			/* then convert to null event*/
			ep->type = GR_EVENT_TYPE_NONE;
		}
	}
}

/**
 * Open a connection to the graphics server.
 *
 * @return the fd of the connection to the server or -1 on failure
 *
 * @ingroup nanox_general
 */
int 
GrOpen(void)
{
	size_t 		size;
	nxOpenReq	req;
	int		tries;
	int		ret = 0;
#if ADDR_FAM == AF_NANO
	struct sockaddr_na name;
#elif ADDR_FAM == AF_INET
	struct sockaddr_in name;
	struct hostent *he;
	/* allow specification of remote nano-X server address*/
	char *sockaddr = getenv("NXDISPLAY");
	if (!sockaddr)
		sockaddr = "127.0.0.1";		/* loopback address*/
#elif ADDR_FAM == AF_UNIX
	struct sockaddr_un name;
	/* allow override of named UNIX socket (default /tmp/.nano-X)*/
	char *sockname = getenv("NXDISPLAY");
	if (!sockname)
		sockname = GR_NAMED_SOCKET;
#else
#error "ADDR_FAM not defined to AF_NANO, AF_INET or AF_UNIX"
#endif
	ACCESS_PER_THREAD_DATA()
	
	/* check already open*/
	if (nxSocket >= 0)
        	return nxSocket;

	/* try to get socket*/
	if ((nxSocket = socket(ADDR_FAM, SOCK_STREAM, 0)) == -1)
		return -1;

	/* initialize global critical section lock*/
	LOCK_INIT(&nxGlobalLock);

#if ADDR_FAM == AF_NANO
	name.sun_family = AF_NANO;
	name.sun_no = GR_ELKS_SOCKET;		/* AF_NANO socket 79*/
	size = sizeof(struct sockaddr_na);
#elif ADDR_FAM == AF_INET
	name.sin_family = AF_INET;
	name.sin_port = htons(GR_NUM_SOCKET);	/* AF_INET socket 6600*/
	if (!(he = gethostbyname(sockaddr))) {
		EPRINTF("nxclient: Can't resolve address for server %s\n", sockaddr);
		close(nxSocket);
		nxSocket = -1;
		return -1;
	}
	name.sin_addr = *(struct in_addr *)he->h_addr_list[0];
	size = sizeof(struct sockaddr_in);
#elif ADDR_FAM == AF_UNIX
	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, sockname);
	size = (offsetof(struct sockaddr_un, sun_path) +
		strlen(name.sun_path) + 1);
#endif

	/*
	 * Try to open the connection ten times,
	 * waiting 0.1 or 2.0 seconds between attempts.
	 */
	for (tries=1; tries<=10; ++tries) {
		struct timespec req;

		ret = connect(nxSocket, (struct sockaddr *) &name, size);
		if (ret >= 0)
			break;
#if ADDR_FAM == AF_INET
		req.tv_sec = 0;
		req.tv_nsec = 100000000L;
#else
		req.tv_sec = 2;
		req.tv_nsec = 0;
#endif
		nanosleep(&req, NULL);
		EPRINTF("nxclient: retry connect attempt %d\n", tries);
	}
	if (ret == -1) {
		close(nxSocket);
		nxSocket = -1;
		return -1;
	}

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	/*
	 * By Performing the 'GrOpen' without allocating a buffer, just
	 * shuffeling the struct over the wire, we can postpone the
	 * allocation of the client size command buffer, which will never be
	 * allocated if the first command after GrOpen() is
	 * GrReqShmCmds() which allocates a replacement shared memory
	 * segment.
	 * So: Calling GrReqShmCmds() right after GrOpen will prevent the
	 * traditional command queue buffer from being allocated from
	 * the process heap - and only the shared memory segment is
	 * allocated.
	 */
	req.reqType = GrNumOpen;
	req.hilength = 0;
	req.length = sizeof(req);
	/* associate the process ID with the client*/
	req.pid = getpid();

	nxWriteSocket((char *)&req,sizeof(req));
	return nxSocket;
}

#if GR_CLOSE_FIX
static void
mySignalhandler(int sig)
{
	if (sig == SIGALRM) {
		EPRINTF("Oops! nxFlushReq() timed out, exiting\n");
		exit(127);
	}
}
#endif

/* Vladimir Cotfas: hang in GrFlush() --> nxFlushReq(0L,1); */
/**
 * Close the graphics device.  Flushes any waiting messages.
 *
 * @ingroup nanox_general
 */
void 
GrClose(void)
{
	ACCESS_PER_THREAD_DATA()

#if GR_CLOSE_FIX
	/* allow 1 second to flush*/
	void * oldSignalHandler = signal(SIGALRM, mySignalhandler);
	alarm(1);
#endif
	LOCK(&nxGlobalLock);
	AllocReq(Close);
	GrFlush();
	UNLOCK(&nxGlobalLock);

#if GR_CLOSE_FIX
	alarm(0);
	signal(SIGALRM, oldSignalHandler);
#endif
	close(nxSocket);
	nxSocket = -1;
	LOCK_FREE(&nxGlobalLock);
}

/**
 * Flush the message buffer of any messages it may contain.
 *
 * @ingroup nanox_general
 */
void 
GrFlush(void)
{
	nxFlushReq(0L,1);
  
#ifdef NOTUSED
	GR_EVENT event;

	/* And stick any incoming events on the local queue */
	while (_GrPeekEvent(&event)) {
		_GrGetNextEventTimeout(&event, 0L);
		QueueEvent(&event);
	}
#endif
}

/**
 * The default error handler.  This is called when the server reports an
 * error event and the client hasn't set up a handler of it's own.
 *
 * Generates a human readable error message describing what error
 * occurred and what function it occured in, then exits.
 *
 * @param ep The error event structure.
 *
 * @ingroup nanox_general
 */
void 
GrDefaultErrorHandler(GR_EVENT *ep)
{
	if (ep->type == GR_EVENT_TYPE_ERROR) {
		EPRINTF("nxclient %d: Error (%s) ", getpid(), ep->error.name);
		EPRINTF(nxErrorStrings[ep->error.code], ep->error.id);
		GrClose();
		exit(1);
	}
}

/**
 * Sets an error handling routine that will be called on any errors from
 * the server (assuming the client has asked to receive them). If zero is
 * used as the argument, errors will be returned as regular events instead.
 *
 * @param fncb the function to call to handle error events
 * @return     the address of the previous error handler
 *
 * @ingroup nanox_general
 */
GR_FNCALLBACKEVENT
GrSetErrorHandler(GR_FNCALLBACKEVENT fncb)
{
	GR_FNCALLBACKEVENT orig;
	ACCESS_PER_THREAD_DATA()

	LOCK(&nxGlobalLock);
	orig = ErrorFunc;
	ErrorFunc = fncb;
	UNLOCK(&nxGlobalLock);
	return orig;
}

/**
 * This function suspends execution of the program for the specified
 * number of milliseconds.
 *
 * @param msecs Number of milliseconds to delay.
 *
 * @ingroup nanox_timer
 */
void
GrDelay(GR_TIMEOUT msecs)
{
	struct timeval timeval;

	timeval.tv_sec = msecs / 1000;
	timeval.tv_usec = (msecs % 1000) * 1000;
	select(0, NULL, NULL, NULL, &timeval);
}

/**
 * Fills in the specified GR_SCREEN_INFO structure.
 *
 * @param sip Pointer to a GR_SCREEN_INFO structure
 *
 * @ingroup nanox_general
 */
void 
GrGetScreenInfo(GR_SCREEN_INFO *sip)
{
	LOCK(&nxGlobalLock);
	AllocReq(GetScreenInfo);
	TypedReadBlock(sip, sizeof(GR_SCREEN_INFO),GrNumGetScreenInfo);
	UNLOCK(&nxGlobalLock);
}

/**
 * Returns the colour at the specified index into the server's color look
 * up table. The colours in the table are those with names like
 * "GR_COLOR_DESKTOP", "GR_COLOR_ACTIVECAPTION", "GR_COLOR_APPWINDOW", etc.
 * as listed in nano-X.h
 *
 * @param index An index into the server's colour look up table.
 * @return      The color found at the specified index.
 *
 * @ingroup nanox_color
 */
GR_COLOR
GrGetSysColor(int index)
{
	nxGetSysColorReq *req;
	GR_COLOR color;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetSysColor);
	req->index = index;
	if(TypedReadBlock(&color, sizeof(color),GrNumGetSysColor) == -1)
		color = 0;
	UNLOCK(&nxGlobalLock);
	return color;
}

/**
 * Gets information about a font.
 *
 * @param font The font ID to query.
 * @param fip    Pointer to the GR_FONT_INFO structure to store the result.
 *
 * @ingroup nanox_font
 */
void 
GrGetFontInfo(GR_FONT_ID font, GR_FONT_INFO *fip)
{
	nxGetFontInfoReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetFontInfo);
	req->fontid = font;
	TypedReadBlock(fip, sizeof(GR_FONT_INFO),GrNumGetFontInfo);
	UNLOCK(&nxGlobalLock);
}

/**
 * Fills in the specified GR_GC_INFO structure with information regarding the
 * specified graphics context.
 *
 * @param gc   A graphics context.
 * @param gcip Pointer to a GR_GC_INFO structure to store the result.
 *
 * @ingroup nanox_draw
 */
void
GrGetGCInfo(GR_GC_ID gc, GR_GC_INFO *gcip)
{
	nxGetGCInfoReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetGCInfo);
	req->gcid = gc;
	TypedReadBlock(gcip, sizeof(GR_GC_INFO),GrNumGetGCInfo);
	UNLOCK(&nxGlobalLock);
}

/**
 * Calculates the dimensions of a specified text string.  Uses the current font
 * and flags in the specified graphics context. The count argument can be -1
 * if the string is null terminated.
 *
 * @param gc        The graphics context.
 * @param str       Pointer to a text string.
 * @param count     The length of the string.
 * @param flags     Text rendering flags. (GR_TF*).
 * @param retwidth  Pointer to the variable the width will be returned in.
 * @param retheight Pointer to the variable the height will be returned in.
 * @param retbase   Pointer to the variable the baseline height will be returned in.
 *
 * @ingroup nanox_font
 */
void
GrGetGCTextSize(GR_GC_ID gc, void *str, int count, GR_TEXTFLAGS flags,
	GR_SIZE *retwidth, GR_SIZE *retheight, GR_SIZE *retbase)
{
	nxGetGCTextSizeReq *req;
	int size;

	/* use strlen as char count when ascii or dbcs*/
	if(count == -1 && (flags&MWTF_PACKMASK) == MWTF_ASCII)
		count = strlen((char *)str);

	size = nxCalcStringBytes(str, count, flags);

	LOCK(&nxGlobalLock);
	req = AllocReqExtra(GetGCTextSize, size);
	req->gcid = gc;
	req->flags = flags;
	req->charcount = count;
	memcpy(GetReqData(req), str, size);
	TypedReadBlock(retwidth, sizeof(*retwidth),GrNumGetGCTextSize);
	ReadBlock(retheight, sizeof(*retheight));
	ReadBlock(retbase, sizeof(*retbase));
	UNLOCK(&nxGlobalLock);
}

/**
 * Register an extra file descriptor to monitor in the main select() call.
 * An event will be returned when the fd has data waiting to be read if that
 * event has been selected for.
 *
 * @param fd The file descriptor to monitor.
 *
 * @ingroup nanox_event
 */
void 
GrRegisterInput(int fd)
{
	ACCESS_PER_THREAD_DATA()

	if (fd < 0)
		return;

	LOCK(&nxGlobalLock);
	FD_SET(fd, &regfdset);
	if (fd >= regfdmax) regfdmax = fd + 1;
	UNLOCK(&nxGlobalLock);
}

/**
 * Stop monitoring a file descriptor previously registered with
 * GrRegisterInput().
 *
 * @param fd The file descriptor to stop monitoring.
 *
 * @ingroup nanox_event
 */
void
GrUnregisterInput(int fd)
{
	int i, max;
	ACCESS_PER_THREAD_DATA()

	LOCK(&nxGlobalLock);

	/* unregister all inputs if fd is -1 */
	if (fd == -1) {
		FD_ZERO(&regfdset);
		regfdmax = -1;
		UNLOCK(&nxGlobalLock);
		return;
	}

	FD_CLR(fd, &regfdset);
	/* recalculate the max file descriptor */
	for (i = 0, max = regfdmax, regfdmax = -1; i < max; i++)
		if (FD_ISSET(i, &regfdset))
			regfdmax = i + 1;
	UNLOCK(&nxGlobalLock);
}

/**
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
 *
 * @ingroup nanox_event
 */
void
GrPrepareSelect(int *maxfd,void *rfdset)
{
	fd_set *rfds = rfdset;
	int fd;

	ACCESS_PER_THREAD_DATA()

	LOCK(&nxGlobalLock);
	AllocReq(GetNextEvent);
	GrFlush();

	FD_SET(nxSocket, rfds);
	if(nxSocket > *maxfd)
		*maxfd = nxSocket;

	/* handle registered input file descriptors*/
	for (fd = 0; fd < regfdmax; fd++) {
		if (FD_ISSET(fd, &regfdset)) {
			FD_SET(fd, rfds);
			if (fd > *maxfd) *maxfd = fd;
		}
	}
	UNLOCK(&nxGlobalLock);
}

/**
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
 *
 * @ingroup nanox_event
 */
void
GrServiceSelect(void *rfdset, GR_FNCALLBACKEVENT fncb)
{
	fd_set *	rfds = rfdset;
	int		fd;
	GR_EVENT 	ev;

	ACCESS_PER_THREAD_DATA()
	LOCK(&nxGlobalLock);

        /* Clean out any event that might have arrived while waiting
	 * for other data, for instance by doing Nano-X requests
	 * between GrPrepareSelect() and GrServiceSelect(), or when
	 * an event is generated in Nano-X at the same time as the
	 * client wakes up for some reason and calls Nano-X functions.
	 */
	if (evlist) {
		/*DPRINTF("nxclient: Handling queued event\n");*/
		GetNextQueuedEvent(&ev);
		CheckErrorEvent(&ev);
		fncb(&ev);
	}
	else {
		if(FD_ISSET(nxSocket, rfds)) {
			TypedReadBlock(&ev, sizeof(ev),GrNumGetNextEvent);
			CheckForClientData(&ev);
			CheckErrorEvent(&ev);
			fncb(&ev);
		}
	}

	/* check for input on registered file descriptors */
	for (fd = 0; fd < regfdmax; fd++) {
		if (FD_ISSET(fd, &regfdset) && FD_ISSET(fd, rfds)) {
			ev.type = GR_EVENT_TYPE_FDINPUT;
			ev.fdinput.fd = fd;
			fncb(&ev);
		}
	}
	UNLOCK(&nxGlobalLock);
}

/**
 * An infinite loop that dispatches events.  Calls the specified callback
 * function whenever an event arrives or there is data to be read on a file
 * descriptor registered with GrRegisterInput(). Never returns.
 *
 * @param fncb Pointer to the function to call when an event needs handling.
 *
 * @ingroup nanox_event
 */
void
GrMainLoop(GR_FNCALLBACKEVENT fncb)
{
	fd_set	rfds;
	int	setsize = 0;

	for(;;) {
		FD_ZERO(&rfds);
		GrPrepareSelect(&setsize, &rfds);
		if(select(setsize+1, &rfds, NULL, NULL, NULL) > 0)
			GrServiceSelect(&rfds, fncb);
	}
}

/**
 * Queue an event in FIFO for later retrieval.
 *
 * @param ep The event to queue
 *
 * @internal
 */
static void
QueueEvent(GR_EVENT *ep)
{
	EVENT_LIST *	elp;
	EVENT_LIST *	prevelp;
        ACCESS_PER_THREAD_DATA()

	elp = malloc(sizeof(EVENT_LIST));
	if (elp) {
		elp->event = *ep;
		elp->next = NULL;

		/* add as last entry on list*/
		if (!evlist) {
			evlist = elp;
			return;
		}
		prevelp = evlist;
		while (prevelp->next)
			prevelp = prevelp->next;
		prevelp->next = elp;
	}
}

/**
 * Retrieve first event in FIFO event queue.  FIFO must not be empty.
 *
 * @param ep Destination for event.
 *
 * @internal
 */
static void
GetNextQueuedEvent(GR_EVENT *ep)
{
	EVENT_LIST	*elp;

        ACCESS_PER_THREAD_DATA()

	*ep = evlist->event;
	elp = evlist;
	evlist = evlist->next;
	free(elp);
}

/**
 * Gets the next event from the event queue.  If the queue is currently
 * empty, sleeps until the next event arrives from the server or input
 * is read on a file descriptor previously specified by GrRegisterInput().
 *
 * @param ep Pointer to the GR_EVENT structure to return the event in.
 *
 * @ingroup nanox_event
 */
void 
GrGetNextEvent(GR_EVENT *ep)
{
	GrGetNextEventTimeout(ep, 0L);
}

/**
 * Gets the next event from the event queue, with a time limit.  If the
 * queue is currently empty, we sleep until the next event arrives from
 * the server, input is read on a file descriptor previously specified
 * by GrRegisterInput(), or a timeout occurs.
 *
 * Note that a value of 0 for the timeout parameter doesn't mean "timeout
 * after 0 milliseconds" but is in fact a magic number meaning "never time
 * out".
 *
 * @param ep      Pointer to the GR_EVENT structure to return the event in.
 * @param timeout The number of milliseconds to wait before timing out, or
 *                0 for forever.
 *
 * @ingroup nanox_event
 */
void
GrGetNextEventTimeout(GR_EVENT * ep, GR_TIMEOUT timeout)
{
	LOCK(&nxGlobalLock);
	if (evlist) {
		/*DPRINTF("nxclient %d: Returning queued event\n",getpid());*/
		GetNextQueuedEvent(ep);
		CheckErrorEvent(ep);
		UNLOCK(&nxGlobalLock);
		return;
	}

	_GrGetNextEventTimeout(ep,timeout);
	UNLOCK(&nxGlobalLock);
}

/**
 * Sleep until the next event arrives, possibly with a time limit.
 * The SERVER_LOCK() must be held and the event queue must be empty
 * before calling this function.
 *
 * Sleeps until the next event arrives from the server, input is read
 * on a file descriptor previously specified by GrRegisterInput(), or
 * a timeout occurs.
 *
 * Note that a value of 0 for the timeout parameter doesn't mean "timeout
 * after 0 milliseconds" but is in fact a magic number meaning "never time
 * out".
 *
 * @param ep      Pointer to the GR_EVENT structure to return the event in.
 * @param timeout The number of milliseconds to wait before timing out, or
 *                0 for forever.
 *
 * @internal
 */
static void
_GrGetNextEventTimeout(GR_EVENT *ep, GR_TIMEOUT timeout)
{
	fd_set		rfds;
	int		setsize = 0;
	int		e;
	struct timeval	to;
	ACCESS_PER_THREAD_DATA()


	FD_ZERO(&rfds);
	/*
	 * This will cause a GrGetNextEvent to be sent down the wire.
	 * If we timeout before the server responds, and then
	 * call this procedure again, and the server has more than
	 * one event waiting for this process, then more than one
	 * event will be written on the socket by the server.  At
	 * that point, a single stored event won't work, and the
	 * client needs an event queue.
	 */
	GrPrepareSelect(&setsize, &rfds);
	if (timeout) {
		to.tv_sec = timeout / 1000;
		to.tv_usec = (timeout % 1000) * 1000;
	}

	if((e = select(setsize+1, &rfds, NULL, NULL, timeout ? &to : NULL))>0) {
		int fd;

		if(FD_ISSET(nxSocket, &rfds)) {
			/*
			 * This will never be GR_EVENT_NONE with the current
			 * implementation.
			 */
		        TypedReadBlock(ep, sizeof(*ep),GrNumGetNextEvent);
			CheckForClientData(ep);
			CheckErrorEvent(ep);
			return;
		}

		/* check for input on registered file descriptors */
		for (fd = 0; fd < regfdmax; fd++) {
			if (FD_ISSET(fd, &regfdset) && FD_ISSET(fd, &rfds)) {
				ep->type = GR_EVENT_TYPE_FDINPUT;
				ep->fdinput.fd = fd;
				break;
			}
		}
	}
	else if (e == 0) {
		/*
		 * Timeout has occured. We currently return a timeout event
		 * regardless of whether the client has selected for it.
		 */
		ep->type = GR_EVENT_TYPE_TIMEOUT;
	} else {
		if(errno == EINTR) {
			ep->type = GR_EVENT_TYPE_NONE;
		} else {
			EPRINTF("nxclient: select failed\n");
			GrClose();
			exit(1);
		}
	}
}

/**
 * Gets a copy of the next event on the queue, without actually
 * removing it from the queue.  Does not block - an event type of
 * GR_EVENT_TYPE_NONE is given if the queue is empty.
 *
 * @param ep Pointer to the GR_EVENT structure to return the event in.
 * @return   1 if an event was returned, or 0 if the queue was empty.
 *
 * @ingroup nanox_event
 */
int 
GrPeekEvent(GR_EVENT *ep)
{
	int ret;
	ACCESS_PER_THREAD_DATA()

	LOCK(&nxGlobalLock);
	if (evlist) {
		*ep = evlist->event;
		CheckErrorEvent(ep);
		UNLOCK(&nxGlobalLock);
		return 1;
	}

	ret = _GrPeekEvent(ep);
	UNLOCK(&nxGlobalLock);
	return ret;
}

static int
_GrPeekEvent(GR_EVENT * ep)
{
        int ret;

	AllocReq(PeekEvent);
	TypedReadBlock(ep, sizeof(*ep),GrNumPeekEvent);
	CheckForClientData(ep);
	ret = ReadByte();
	CheckErrorEvent(ep);
	return (ret > 0);	/* -1 ReadByte counts as 0 (no event)*/
}

/**
 * Wait until an event is available for a client, and then peek at it.
 *
 * @param ep Pointer to the GR_EVENT structure to return the event in.
 *
 * @ingroup nanox_event
 */
void
GrPeekWaitEvent(GR_EVENT *ep)
{
	EVENT_LIST *	elp;

	ACCESS_PER_THREAD_DATA()

	LOCK(&nxGlobalLock);
	if (evlist) {
		*ep = evlist->event;
		CheckErrorEvent(ep);
		UNLOCK(&nxGlobalLock);
		return;
	}

	/* wait for next event*/
	GrGetNextEvent(ep);

	/* add event back on head of list*/
	elp = malloc(sizeof(EVENT_LIST));
	if (elp) {
		elp->event = *ep;
		elp->next = evlist;
	}

	/* peek at it*/
	GrPeekEvent(ep);

	UNLOCK(&nxGlobalLock);
}

/**
 * Gets the next event from the event queue if there is one.  Returns
 * immediately with an event type of GR_EVENT_TYPE_NONE if the queue
 * is empty.
 *
 * @param ep Pointer to the GR_EVENT structure to return the event in.
 *
 * @ingroup nanox_event
 */
void 
GrCheckNextEvent(GR_EVENT *ep)
{
	ACCESS_PER_THREAD_DATA()

	LOCK(&nxGlobalLock);
	if (evlist) {
		/*DPRINTF("nxclient %d: Returning queued event\n",getpid());*/
		GetNextQueuedEvent(ep);
		CheckErrorEvent(ep);
		UNLOCK(&nxGlobalLock);
		return;
	}

	AllocReq(CheckNextEvent);
	TypedReadBlock(ep, sizeof(*ep),GrNumGetNextEvent);
	CheckForClientData(ep);
	CheckErrorEvent(ep);
	UNLOCK(&nxGlobalLock);
}

/* builtin callback function for GrGetTypedEvent*/
static GR_BOOL
GetTypedEventCallback(GR_WINDOW_ID wid, GR_EVENT_MASK mask, GR_UPDATE_TYPE update,
	GR_EVENT *ep, void *arg)
{
	GR_EVENT_MASK	emask = GR_EVENTMASK(ep->type);

DPRINTF("GetTypedEventCallback: wid %d mask %x update %d from %d type %d\n", wid, (unsigned)mask, update, ep->general.wid, ep->type);

	/* FIXME: not all events have wid field here... */
	if (wid && (wid != ep->general.wid))
		return 0;

	if (mask) {
		if ((mask & emask) == 0)
			return 0;

		if (update && ((mask & emask) == GR_EVENT_MASK_UPDATE))
			if (update != ep->update.utype)
				return 0;
	}

	return 1;
}

/**
 * Fills in the specified event structure with a copy of the next event on the
 * queue that matches the type parameters passed and removes it from the queue.
 * If block is GR_TRUE, the call will block until a matching event is found.
 * Otherwise, only the local queue is searched, and an event type of
 * GR_EVENT_TYPE_NONE is returned if the a match is not found.
 *
 * @param wid     Window id for which to check events. 0 means no window.
 * @param mask    Event mask of events for which to check. 0 means no check for mask.
 * @param update  Update event subtype when event mask is GR_EVENT_MASK_UPDATE.
 * @param ep      Pointer to the GR_EVENT structure to return the event in.
 * @param block   Specifies whether or not to block, GR_TRUE blocks, GR_FALSE does not.
 * @return        GR_EVENT_TYPE if an event was returned, or GR_EVENT_TYPE_NONE 
 *                if no events match.
 *
 * @ingroup nanox_event
 */
int
GrGetTypedEvent(GR_WINDOW_ID wid, GR_EVENT_MASK mask, GR_UPDATE_TYPE update,
	GR_EVENT *ep, GR_BOOL block)
{
	return GrGetTypedEventPred(wid, mask, update, ep, block,
		GetTypedEventCallback, NULL);
}

/**
 * The specified callback function is called with the passed event type parameters
 * for each event on the queue, until the callback function CheckFunction
 * returns GR_TRUE.  The event is then removed from the queue and returned.
 * If block is GR_TRUE, the call will block until a matching event is found.
 * Otherwise, only the local queue is searched, and an event type of
 * GR_EVENT_TYPE_NONE is returned if the a match is not found.
 *
 * @param wid     Window id for which to check events. 0 means no window.
 * @param mask    Event mask of events for which to check. 0 means no check for mask.
 * @param update  Update event subtype when event mask is GR_EVENT_MASK_UPDATE.
 * @param ep      Pointer to the GR_EVENT structure to return the event in.
 * @param block   Specifies whether or not to block, GR_TRUE blocks, GR_FALSE does not.
 * @param matchfn Specifies the callback function called for matching.
 * @param arg     A programmer-specified argument passed to the callback function.
 * @return        GR_EVENT_TYPE if an event was returned, or GR_EVENT_TYPE_NONE 
 *                if no events match.
 *
 * @ingroup nanox_event
 */
int
GrGetTypedEventPred(GR_WINDOW_ID wid, GR_EVENT_MASK mask, GR_UPDATE_TYPE update,
	GR_EVENT *ep, GR_BOOL block, GR_TYPED_EVENT_CALLBACK matchfn, void *arg)
{
	EVENT_LIST *elp, *prevelp;
	GR_EVENT event;

	ACCESS_PER_THREAD_DATA();
  
	LOCK(&nxGlobalLock);
	/* First, suck up all events and place them into the event queue */
	while(_GrPeekEvent(&event)) {
getevent:
		_GrGetNextEventTimeout(&event, 0L);
		QueueEvent(&event);
	}

	/* Now, run through the event queue, looking for matches of the type
	 * info that was passed.
	 */
	prevelp = NULL;
	elp = evlist;
	while (elp) {
		if (matchfn(wid, mask, update, &elp->event, arg)) {
			/* remove event from queue, return it*/
			if (prevelp == NULL)
				evlist = elp->next;
			else prevelp->next = elp->next;
			*ep = elp->event;

			UNLOCK(&nxGlobalLock);
			return ep->type;
		}
		prevelp = elp;
		elp = elp->next;
	}

	/* if event still not found and waiting ok, then wait*/
	if (block)
		goto getevent;

	/* return no event*/
	ep->type = GR_EVENT_TYPE_NONE;
	UNLOCK(&nxGlobalLock);
	return GR_EVENT_TYPE_NONE;
} 

/**
 * Select the event types which should be returned for the specified window.
 *
 * @param wid       The ID of the window to set the event mask of.
 * @param eventmask A bit field specifying the desired event mask.
 *
 * @ingroup nanox_event
 */
void 
GrSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask)
{
	nxSelectEventsReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SelectEvents);
	req->windowid = wid;
	req->eventmask = eventmask;
	UNLOCK(&nxGlobalLock);
}

/**
 * Create a new window.
 *
 * @param parent      The ID of the parent window.
 * @param x           The X coordinate of the new window relative to the parent window.
 * @param y           The Y coordinate of the new window relative to the parent window.
 * @param width       The width of the new window.
 * @param height      The height of the new window.
 * @param bordersize  The width of the window border.
 * @param background  The color of the window background.
 * @param bordercolor The color of the window border.
 * @return            The ID of the newly created window.
 *
 * @ingroup nanox_window
 */
GR_WINDOW_ID
GrNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_SIZE bordersize, GR_COLOR background,
	GR_COLOR bordercolor)
{
	nxNewWindowReq *req;
	GR_WINDOW_ID 	wid;

	LOCK(&nxGlobalLock);
	req = AllocReq(NewWindow);
	req->parentid = parent;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	req->backgroundcolor = background;
	req->bordercolor = bordercolor;
	req->bordersize = bordersize;
	if(TypedReadBlock(&wid, sizeof(wid),GrNumNewWindow) == -1)
		wid = 0;
	UNLOCK(&nxGlobalLock);
	return wid;
}
   
   
/**
 * Create a new server side pixmap.  This is an offscreen drawing area which
 * can be copied into a window using a GrCopyArea call.
 *
 * @param width  The width of the pixmap.
 * @param height The height of the pixmap.
 * @param pixels Currently unused in client/server mode.
 * @return       The ID of the newly created pixmap.
 *
 * @todo FIXME Add support for shared memory...
 *
 * @ingroup nanox_window
 */
GR_WINDOW_ID
GrNewPixmap(GR_SIZE width, GR_SIZE height, void *pixels)
{
	nxNewPixmapReq *req;
	GR_WINDOW_ID 	wid;

	LOCK(&nxGlobalLock);
	req = AllocReq(NewPixmap);
	req->width = width;
	req->height = height;
	if(TypedReadBlock(&wid, sizeof(wid), GrNumNewPixmap) == -1)
		wid = 0;
	UNLOCK(&nxGlobalLock);
	return wid;
}

/**
 * Create a new input-only window with the specified dimensions which is a
 * child of the specified parent window.
 *
 * @param parent The ID of the window to use as the parent of the new window.
 * @param x      The X coordinate of the new window relative to the parent window.
 * @param y      The Y coordinate of the new window relative to the parent window.
 * @param width  The width of the new window.
 * @param height The height of the new window.
 * @return       The ID of the newly created window.
 *
 * @ingroup nanox_window
 */
GR_WINDOW_ID
GrNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height)
{
	nxNewInputWindowReq *req;
	GR_WINDOW_ID 	     wid;

	LOCK(&nxGlobalLock);
	req = AllocReq(NewInputWindow);
	req->parentid = parent;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	if(TypedReadBlock(&wid, sizeof(wid), GrNumNewInputWindow) == -1)
		wid = 0;
	UNLOCK(&nxGlobalLock);
	return wid;
}

/**
 * Destroys a window and all of it's children.
 *
 * Recursively unmaps and frees the data structures associated with the
 * specified window and all of its children.
 *
 * @param wid The ID of the window to destroy.
 *
 * @ingroup nanox_window
 */
void 
GrDestroyWindow(GR_WINDOW_ID wid)
{
	nxDestroyWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(DestroyWindow);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Fills in a GR_WINDOW_INFO structure with information regarding a window.
 *
 * @param wid     The ID of the window to retrieve information about.
 * @param infoptr Pointer to a GR_WINDOW_INFO structure to return the information in.
 *
 * @ingroup nanox_window
 */
void 
GrGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr)
{
	nxGetWindowInfoReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetWindowInfo);
	req->windowid = wid;
	TypedReadBlock(infoptr, sizeof(GR_WINDOW_INFO), GrNumGetWindowInfo);
	UNLOCK(&nxGlobalLock);
}

/**
 * Creates a new graphics context structure. The structure is initialised
 * with a set of default parameters.
 *
 * @return The ID of the newly created graphics context or 0 on error.
 *
 * @ingroup nanox_draw
 */
GR_GC_ID 
GrNewGC(void)
{
	GR_GC_ID    gc;

	LOCK(&nxGlobalLock);
	AllocReq(NewGC);
	if(TypedReadBlock(&gc, sizeof(gc),GrNumNewGC) == -1)
		gc = 0;
	UNLOCK(&nxGlobalLock);
	return gc;
}

/**
 * Creates a new graphics context structure and copies the settings
 * from an already existing graphics context.
 *
 * @param gc The already existing graphics context to copy the parameters from.
 * @return   The ID of the newly created graphics context or 0 on error.
 *
 * @ingroup nanox_draw
 */
GR_GC_ID 
GrCopyGC(GR_GC_ID gc)
{
	nxCopyGCReq *req;
	GR_GC_ID     newgc;

	LOCK(&nxGlobalLock);
	req = AllocReq(CopyGC);
	req->gcid = gc;
	if(TypedReadBlock(&newgc, sizeof(newgc),GrNumCopyGC) == -1)
		newgc = 0;
	UNLOCK(&nxGlobalLock);
	return newgc;
}

/**
 * Destroys a graphics context structure.
 *
 * @param gc the ID of the graphics context structure to destroy
 *
 * @ingroup nanox_draw
 */
void
GrDestroyGC(GR_GC_ID gc)
{
	nxDestroyGCReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(DestroyGC);
	req->gcid = gc;
	UNLOCK(&nxGlobalLock);
}

/**
 * Creates a new region structure.
 * The structure is initialised with a set of default parameters.
 *
 * @return the ID of the newly created region
 *
 * @ingroup nanox_region
 */
GR_REGION_ID 
GrNewRegion(void)
{
	GR_REGION_ID    region;

	LOCK(&nxGlobalLock);
	AllocReq(NewRegion);
	if(TypedReadBlock(&region, sizeof(region),GrNumNewRegion) == -1)
		region = 0;
	UNLOCK(&nxGlobalLock);
	return region;
}

/**
 * Destroys a region structure.
 *
 * @param region The ID of the region structure to destroy.
 *
 * @ingroup nanox_region
 */
void
GrDestroyRegion(GR_REGION_ID region)
{
	nxDestroyRegionReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(DestroyRegion);
	req->regionid = region;
	UNLOCK(&nxGlobalLock);
}

/**
 * Makes a union of the specified region and the specified rectangle.
 * Places the result back in the source region.
 *
 * @param region The ID of the region to modify.
 * @param rect   A pointer to the rectangle to add to the region.
 *
 * @ingroup nanox_region
 */
void
GrUnionRectWithRegion(GR_REGION_ID region, GR_RECT *rect)
{
	nxUnionRectWithRegionReq *req;

	LOCK(&nxGlobalLock);
 	req = AllocReq(UnionRectWithRegion);
 	if(rect)
 		memcpy(&req->rect, rect, sizeof(*rect));
 	req->regionid = region;
	UNLOCK(&nxGlobalLock);
}

/**
 * Makes a union of two regions. Places the result in the
 * specified destination region.
 *
 * @param dst_rgn  The ID of the destination region.
 * @param src_rgn1 The ID of the first source region.
 * @param src_rgn2 The ID of the second source region.
 *
 * @ingroup nanox_region
 */
void
GrUnionRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	nxUnionRegionReq *req;

	LOCK(&nxGlobalLock);
 	req = AllocReq(UnionRegion);
 	req->regionid = dst_rgn;
 	req->srcregionid1 = src_rgn1;
 	req->srcregionid2 = src_rgn2;
	UNLOCK(&nxGlobalLock);
}

/**
 * Subtracts the second source region from the first source region and places
 * the result in the specified destination region.
 *
 * @param dst_rgn  The ID of the destination region.
 * @param src_rgn1 The ID of the first source region.
 * @param src_rgn2 The ID of the second source region.
 *
 * @ingroup nanox_region
 */
void
GrSubtractRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	nxSubtractRegionReq *req;

	LOCK(&nxGlobalLock);
 	req = AllocReq(SubtractRegion);
 	req->regionid = dst_rgn;
 	req->srcregionid1 = src_rgn1;
 	req->srcregionid2 = src_rgn2;
	UNLOCK(&nxGlobalLock);
}

/**
 * Performs a logical exclusive OR operation on the specified source regions
 * and places the result in the destination region. The destination region
 * will contain only the parts of the source regions which do not overlap.
 *
 * @param dst_rgn  The ID of the destination region.
 * @param src_rgn1 The ID of the first source region.
 * @param src_rgn2 The ID of the second source region.
 *
 * @ingroup nanox_region
 */
void
GrXorRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	nxXorRegionReq *req;

	LOCK(&nxGlobalLock);
 	req = AllocReq(XorRegion);
 	req->regionid = dst_rgn;
 	req->srcregionid1 = src_rgn1;
 	req->srcregionid2 = src_rgn2;
	UNLOCK(&nxGlobalLock);
}

/**
 * Calculates the intersection of the two specified source regions and places
 * the result in the specified destination region. The destination region
 * will contain only the parts of the source regions which overlap each other.
 *
 * @param dst_rgn  The ID of the destination region.
 * @param src_rgn1 The ID of the first source region.
 * @param src_rgn2 The ID of the second source region.
 *
 * @ingroup nanox_region
 */
void
GrIntersectRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	nxIntersectRegionReq *req;

	LOCK(&nxGlobalLock);
 	req = AllocReq(IntersectRegion);
 	req->regionid = dst_rgn;
 	req->srcregionid1 = src_rgn1;
 	req->srcregionid2 = src_rgn2;
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the clip mask of the specified graphics context to the specified
 * region. Subsequent drawing operations using this graphics context will not
 * draw outside the specified region. The region ID can be set to 0 to remove
 * the clipping region from the specified graphics context.
 *
 * @param gc     The ID of the graphics context to set the clip mask of.
 * @param region The ID of the region to use as the clip mask, or 0 for none.
 *
 * @ingroup nanox_region
 */
void
GrSetGCRegion(GR_GC_ID gc, GR_REGION_ID region)
{
	nxSetGCRegionReq *req;
	
	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCRegion);
	req->gcid = gc;
	req->regionid = region;
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the X,Y origin of the user clip region in the specified
 * graphics context.
 *
 * @param gc The ID of the graphics context with user clip region.
 * @param xoff  New X offset of user clip region.
 * @param yoff  New Y offset of user clip region.
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCClipOrigin(GR_GC_ID gc, int xoff, int yoff)
{
	nxSetGCClipOriginReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCClipOrigin);
	req->gcid = gc;
	req->xoff = xoff;
	req->yoff = yoff;
	UNLOCK(&nxGlobalLock);
}

/**
 * Controls if GR_EVENT_TYPE_EXPOSURE events are sent as a 
 * result of GrCopyArea using the specified graphics context.
 *
 * @param gc       The ID of the graphics context
 * @param exposure TRUE to send events, FALSE otherwise.
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCGraphicsExposure(GR_GC_ID gc, GR_BOOL exposure)
{
	nxSetGCGraphicsExposureReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCGraphicsExposure);
	req->gcid = gc;
	req->exposure = exposure;
	UNLOCK(&nxGlobalLock);
}

/**
 * Tests whether the specified point is within the specified region, and
 * then returns either True or False depending on the result.
 *
 * @param region the ID of the region to examine.
 * @param x      the X coordinate of the point to test for.
 * @param y      the Y coordinate of the point to test for.
 * @return       TRUE if the point is within the region, otherwise FALSE.
 *
 * @ingroup nanox_region
 */
GR_BOOL
GrPointInRegion(GR_REGION_ID region, GR_COORD x, GR_COORD y)
{
	nxPointInRegionReq *req;
	GR_BOOL             ret_value;

	LOCK(&nxGlobalLock);
	req = AllocReq(PointInRegion);
	req->regionid = region;
	req->x = x;
	req->y = y;
	if(TypedReadBlock(&ret_value, sizeof(ret_value),
	    GrNumPointInRegion) == -1)
		ret_value = GR_FALSE;
	UNLOCK(&nxGlobalLock);
	return ret_value;
}

/**
 * Tests whether the specified rectangle is contained within the specified
 * region. Returns GR_RECT_OUT if it is not inside it at all, GR_RECT_ALLIN
 * if it is completely contained within the region, or GR_RECT_PARTIN if
 * it is partially contained within the region.
 *
 * @param region The ID of the region to examine.
 * @param x      The X coordinates of the rectangle to test.
 * @param y      The Y coordinates of the rectangle to test.
 * @param w      The width of the rectangle to test.
 * @param h      The height of the rectangle to test.
 * @return       GR_RECT_PARTIN, GR_RECT_ALLIN, or GR_RECT_OUT.
 *
 * @ingroup nanox_region
 */
int
GrRectInRegion(GR_REGION_ID region, GR_COORD x, GR_COORD y, GR_COORD w,
	GR_COORD h)
{
	nxRectInRegionReq *req;
	unsigned short	   ret_value;
	
	LOCK(&nxGlobalLock);
	req = AllocReq(RectInRegion);
	req->regionid = region;
	req->x = x;
	req->y = y;
	req->w = w;
	req->h = h;
 	if(TypedReadBlock(&ret_value, sizeof(ret_value),
	    GrNumRectInRegion) == -1)
		ret_value = 0;
	UNLOCK(&nxGlobalLock);
	return (int)ret_value;
}

/**
 * Determines whether the specified region is empty.
 *
 * @param region The ID of the region to examine.
 * @return       GR_TRUE if the region is empty, or GR_FALSE if it is not.
 *
 * @ingroup nanox_region
 */
GR_BOOL
GrEmptyRegion(GR_REGION_ID region)
{
	nxEmptyRegionReq *req;
	GR_BOOL 	  ret_value;
	
	LOCK(&nxGlobalLock);
	req = AllocReq(EmptyRegion);
	req->regionid = region;
 	if(TypedReadBlock(&ret_value, sizeof(ret_value),
	    GrNumEmptyRegion) == -1)
		ret_value = GR_FALSE;
	UNLOCK(&nxGlobalLock);
	return ret_value;
}

/**
 * Determines whether the specified regions are identical, and returns GR_TRUE
 * if it is, or GR_FALSE otherwise.
 *
 * @param rgn1 The ID of the first region to examine.
 * @param rgn2 The ID of the second region to examine.
 * @return     GR_TRUE if the regions are equal, or GR_FALSE otherwise
 *
 * @ingroup nanox_region
 */
GR_BOOL
GrEqualRegion(GR_REGION_ID rgn1, GR_REGION_ID rgn2)
{
	nxEqualRegionReq *req;
	GR_BOOL 	  ret_value;
	
	LOCK(&nxGlobalLock);
	req = AllocReq(EqualRegion);
	req->region1 = rgn1;
	req->region2 = rgn2;
 	if(TypedReadBlock(&ret_value, sizeof(ret_value),
	    GrNumEqualRegion) == -1)
		ret_value = GR_FALSE;
	UNLOCK(&nxGlobalLock);
	return ret_value;
}

/**
 * Offsets the specified region by the specified distance.
 *
 * @param region The ID of the region to offset
 * @param dx     The distance to offset the region by in the X axis
 * @param dy     The distance to offset the region by in the Y axis
 *
 * @ingroup nanox_region
 */
void
GrOffsetRegion(GR_REGION_ID region, GR_SIZE dx, GR_SIZE dy)
{
	nxOffsetRegionReq *req;
	
	LOCK(&nxGlobalLock);
	req = AllocReq(OffsetRegion);
	req->region = region;
	req->dx = dx;
	req->dy = dy;
	UNLOCK(&nxGlobalLock);
}

/**
 * Fills in the specified rectangle structure with a bounding box that would
 * completely enclose the specified region, and also returns the type of the
 * specified region. 
 *
 * @param region The ID of the region to get the bounding box of
 * @param rect   Pointer to a rectangle structure
 * @return       The region type
 *
 * @todo FIXME check Doxygen comments from this point down.
 *
 * @ingroup nanox_region
 */
int
GrGetRegionBox(GR_REGION_ID region, GR_RECT *rect)
{
	nxGetRegionBoxReq *req;
	unsigned short	   ret_value;
	
	if (!rect)
		return GR_FALSE;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetRegionBox);
	req->regionid = region;
 	if(TypedReadBlock(rect, sizeof(*rect), GrNumGetRegionBox) == -1)
		return GR_FALSE;
 	if(TypedReadBlock(&ret_value, sizeof(ret_value),
	    GrNumGetRegionBox) == -1)
		ret_value = GR_FALSE;
	UNLOCK(&nxGlobalLock);
	return ret_value;
}

/**
 * Creates a new region structure, fills it with the region described by the
 * specified polygon, and returns the ID used to refer to it.
 *
 * @param mode  the polygon mode to use (GR_POLY_EVENODD or GR_POLY_WINDING)
 * @param count  the number of points in the polygon
 * @param points  pointer to an array of point structures describing the polygon
 * @return the ID of the newly allocated region structure, or 0 on error
 *
 * @ingroup nanox_region
 */
GR_REGION_ID 
GrNewPolygonRegion(int mode, GR_COUNT count, GR_POINT *points)
{
	nxNewPolygonRegionReq	*req;
	long			size;
	GR_REGION_ID		region;

	if(count == 0)
		return GrNewRegion();
	
	if(points == NULL)
		return 0;

	LOCK(&nxGlobalLock);
	size = (long)count * sizeof(GR_POINT);
	req = AllocReqExtra(NewPolygonRegion, size);
	req->mode = mode;
	/* FIXME: unportable method, depends on sizeof(int) in GR_POINT*/
	memcpy(GetReqData(req), points, size);

	if(TypedReadBlock(&region, sizeof(region),
	    GrNumNewPolygonRegion) == -1)
		region = 0;
	UNLOCK(&nxGlobalLock);
	return region;
}

/**
 * Recursively maps (makes visible) the specified window and all of the
 * child windows which have a sufficient map count. The border and background
 * of the window are painted, and an exposure event is generated for the
 * window and every child which becomes visible.
 *
 * @param wid  the ID of the window to map
 *
 * @ingroup nanox_window
 */
void 
GrMapWindow(GR_WINDOW_ID wid)
{
	nxMapWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(MapWindow);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Recursively unmaps (makes invisible) the specified window and all of the
 * child windows.
 *
 * @param wid  the ID of the window to unmap
 *
 * @ingroup nanox_window
 */
void 
GrUnmapWindow(GR_WINDOW_ID wid)
{
	nxUnmapWindowReq *req;
	
	LOCK(&nxGlobalLock);
	req = AllocReq(UnmapWindow);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Places the specified window at the top of its parents drawing stack, above
 * all of its sibling windows.
 *
 * @param wid  the ID of the window to raise
 *
 * @ingroup nanox_window
 */
void 
GrRaiseWindow(GR_WINDOW_ID wid)
{
	nxRaiseWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(RaiseWindow);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Places the specified window at the bottom of its parents drawing stack,
 * below all of its sibling windows.
 *
 * @param wid  the ID of the window to lower
 *
 * @ingroup nanox_window
 */
void 
GrLowerWindow(GR_WINDOW_ID wid)
{
	nxLowerWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(LowerWindow);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Moves the specified window to the specified position relative to its
 * parent window.
 *
 * @param wid  the ID of the window to move
 * @param x  the X coordinate to move the window to relative to its parent.
 * @param y  the Y coordinate to move the window to relative to its parent.
 *
 * @ingroup nanox_window
 */
void 
GrMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y)
{
	nxMoveWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(MoveWindow);
	req->windowid = wid;
	req->x = x;
	req->y = y;
	UNLOCK(&nxGlobalLock);
}

/**
 * Resizes the specified window to be the specified width and height.
 *
 * @param wid  the ID of the window to resize
 * @param width  the width to resize the window to
 * @param height  the height to resize the window to
 *
 * @ingroup nanox_window
 */
void 
GrResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height)
{
	nxResizeWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(ResizeWindow);
	req->windowid = wid;
	req->width = width;
	req->height = height;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the parent window of the specified window to the specified parent
 * window and places it at the specified coordinates relative to the new
 * parent.
 *
 * @param wid  the ID of the window to reparent
 * @param pwid  the ID of the new parent window
 * @param x  the X coordinate to place the window at relative to the new parent
 * @param y  the Y coordinate to place the window at relative to the new parent
 *
 * @ingroup nanox_window
 */
void 
GrReparentWindow(GR_WINDOW_ID wid, GR_WINDOW_ID pwid, GR_COORD x, GR_COORD y)
{
	nxReparentWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(ReparentWindow);
	req->windowid = wid;
	req->parentid = pwid;
	req->x = x;
	req->y = y;
	UNLOCK(&nxGlobalLock);
}

/**
 * Clears the specified window by to its background color or pixmap.
 * If exposeflag is non zero, an exposure event is generated for
 * the window after it has been cleared.
 *
 * @param wid        Window ID.
 * @param x          X co-ordinate of rectangle to clear.
 * @param y          Y co-ordinate of rectangle to clear.
 * @param width      Width of rectangle to clear.
 * @param height     Height of rectangle to clear.
 * @param exposeflag A flag indicating whether to also generate an exposure event.
 *
 * @ingroup nanox_draw
 */
void
GrClearArea(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_BOOL exposeflag)
{
	nxClearAreaReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(ClearArea);
	req->windowid = wid;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	req->exposeflag = exposeflag;
	UNLOCK(&nxGlobalLock);
}

/**
 * Returns the ID of the window which currently has the keyboard focus.
 *
 * @return the ID of the window which currently has the keyboard focus
 *
 * @ingroup nanox_window
 */
GR_WINDOW_ID
GrGetFocus(void)
{
	GR_WINDOW_ID    wid;

	LOCK(&nxGlobalLock);
	AllocReq(GetFocus);
	if(TypedReadBlock(&wid, sizeof(wid), GrNumGetFocus) == -1)
		wid = 0;
	UNLOCK(&nxGlobalLock);
	return wid;
}

/**
 * Sets the keyboard focus to the specified window.
 *
 * @param wid  the ID of the window to set the focus to
 *
 * @ingroup nanox_window
 */
void 
GrSetFocus(GR_WINDOW_ID wid)
{
	nxSetFocusReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetFocus);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Specify a cursor for a window.
 * This cursor will only be used within that window, and by default
 * for its new children.  If the cursor is currently within this
 * window, it will be changed to the new one immediately.
 * If the new cursor ID is 0, revert to the root window cursor.
 *
 * @param wid  the ID of the window to set the cursor for
 * @param cid  the cursor ID
 *
 * @ingroup nanox_cursor
 */
void
GrSetWindowCursor(GR_WINDOW_ID wid, GR_CURSOR_ID cid)
{
	nxSetWindowCursorReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetWindowCursor);
	req->windowid = wid;
	req->cursorid = cid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Creates a server-based cursor (mouse graphic) resource.
 *
 * @param width  the width of the pointer bitmap
 * @param height  the height of the pointer bitmap
 * @param hotx  the X coordinate within the bitmap used as the target of the pointer
 * @param hoty  the Y coordinate within the bitmap used as the target of the pointer
 * @param foreground  the colour to use for the foreground of the pointer
 * @param background  the colour to use for the background of the pointer
 * @param fgbitmap  pointer to bitmap data specifying the foreground of the pointer
 * @param bgbitmap  pointer to bitmap data specifying the background of the pointer
 *
 * @ingroup nanox_cursor
 */
GR_CURSOR_ID
GrNewCursor(GR_SIZE width, GR_SIZE height, GR_COORD hotx, GR_COORD hoty,
	GR_COLOR foreground, GR_COLOR background,
	GR_BITMAP *fgbitmap, GR_BITMAP *bgbitmap)
{
	nxNewCursorReq *req;
	int 	     	bitmapsize;
	char *	     	data;
	GR_CURSOR_ID	cursorid;

	bitmapsize = GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);
	LOCK(&nxGlobalLock);
	req = AllocReqExtra(NewCursor, bitmapsize*2);
	req->width = width;
	req->height = height;
	req->hotx = hotx;
	req->hoty = hoty;
	req->fgcolor = foreground;
	req->bgcolor = background;
	data = GetReqData(req);
	memcpy(data, fgbitmap, bitmapsize);
	memcpy(data+bitmapsize, bgbitmap, bitmapsize);

	if(TypedReadBlock(&cursorid, sizeof(cursorid), GrNumNewCursor) == -1)
		cursorid = 0;
	UNLOCK(&nxGlobalLock);
	return cursorid;
}

/**
 * Moves the cursor (mouse pointer) to the specified coordinates.
 * The coordinates are relative to the root window, where (0,0) is the upper
 * left hand corner of the screen. The reference point used for the pointer
 * is that of the "hot spot". After moving the pointer, the graphic used for
 * the pointer will change to the graphic defined for use in the window which
 * it is over.
 *
 * @param x  the X coordinate to move the pointer to
 * @param y  the Y coordinate to move the pointer to
 *
 * @ingroup nanox_cursor
 */
void 
GrMoveCursor(GR_COORD x, GR_COORD y)
{
	nxMoveCursorReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(MoveCursor);
	req->x = x;
	req->y = y;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the foreground colour of the specified graphics context to the
 * specified RGB colour.
 *
 * @param gc  the ID of the graphics context to set the foreground colour of
 * @param foreground  the RGB colour to use as the new foreground colour
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCForeground(GR_GC_ID gc, GR_COLOR foreground)
{
	nxSetGCForegroundReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCForeground);
	req->gcid = gc;
	req->color = foreground;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the background colour of the specified graphics context to the
 * specified RGB colour.
 *
 * @param gc  the ID of the graphics context to set the background colour of
 * @param background  the RGB colour to use as the new background colour
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCBackground(GR_GC_ID gc, GR_COLOR background)
{
	nxSetGCBackgroundReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCBackground);
	req->gcid = gc;
	req->color = background;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the foreground colour of the specified graphics context to the
 * specified hardware pixel value.
 *
 * @param gc         The ID of the graphics context to set the foreground colour of.
 * @param foreground The GR_PIXELVAL (i.e. hardware pixel value) to use as the new
 *                   foreground colour.
 *
 * @ingroup nanox_draw
 */
void
GrSetGCForegroundPixelVal(GR_GC_ID gc, GR_PIXELVAL foreground)
{
	nxSetGCForegroundPixelValReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCForegroundPixelVal);
	req->gcid = gc;
	req->pixelval = foreground;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the background colour of the specified graphics context to the
 * specified hardware pixel value.
 *
 * @param gc  the ID of the graphics context to set the background colour of
 * @param background  the GR_PIXELVAL (i.e. hardware pixel value) to use as the new
 *              background colour
 *
 * @ingroup nanox_draw
 */
void
GrSetGCBackgroundPixelVal(GR_GC_ID gc, GR_PIXELVAL background)
{
	nxSetGCBackgroundPixelValReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCBackgroundPixelVal);
	req->gcid = gc;
	req->pixelval = background;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the drawing mode (SET, XOR, OR, AND, etc.) of the specified
 * graphics context to the specified mode.
 *
 * @param gc  the ID of the graphics context to set the drawing mode of
 * @param mode  the new drawing mode
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCMode(GR_GC_ID gc, int mode)
{
	nxSetGCModeReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCMode);
	req->gcid = gc;
	req->mode = mode;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the line style to either SOLID or ON OFF DASHED 
 *
 * @param gc  the ID of the graphics context to set the drawing mode of
 * @param linestyle   The new style of the line
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCLineAttributes(GR_GC_ID gc, int linestyle)
{
	nxSetGCLineAttributesReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCLineAttributes);
	req->gcid = gc;
	req->linestyle = linestyle;
	UNLOCK(&nxGlobalLock);
}

/**
 * FIXME
 *
 * @param gc     Graphics context ID.
 * @param dashes FIXME
 * @param count  FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCDash(GR_GC_ID gc, char *dashes, int count)
{
	nxSetGCDashReq *req;
	int size;

	size = count * sizeof(char);
	LOCK(&nxGlobalLock);
	req = AllocReqExtra(SetGCDash, size);
	req->gcid = gc;
	req->count = count;
	memcpy(GetReqData(req), dashes, size);
	UNLOCK(&nxGlobalLock);
}

/**
 * FIXME
 *
 * @param gc       FIXME
 * @param fillmode FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_draw
 */
void
GrSetGCFillMode(GR_GC_ID gc, int fillmode)
{
  	nxSetGCFillModeReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCFillMode);
	req->gcid = gc;
	req->fillmode = fillmode;
	UNLOCK(&nxGlobalLock);
}

/**
 * FIXME
 *
 * @param gc     FIXME
 * @param bitmap FIXME
 * @param width  FIXME
 * @param height FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_draw
 */
void
GrSetGCStipple(GR_GC_ID gc, GR_BITMAP *bitmap, int width, int height)
{
	nxSetGCStippleReq *req;
	int size;

	size = GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);
	LOCK(&nxGlobalLock);
	req = AllocReqExtra(SetGCStipple, size);
	req->gcid = gc;
	req->width = width;
	req->height = height;
	memcpy(GetReqData(req), bitmap, size);
	UNLOCK(&nxGlobalLock);
}

/**
 * FIXME
 *
 * @param gc     FIXME
 * @param pixmap FIXME
 * @param width  FIXME
 * @param height FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_draw
 */
void
GrSetGCTile(GR_GC_ID gc, GR_WINDOW_ID pixmap, int width, int height)
{
	nxSetGCTileReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCTile);
	req->gcid = gc;
	req->pixmap = pixmap;
	req->width = width;
	req->height = height;
	UNLOCK(&nxGlobalLock);
}

/**
 * FIXME
 *
 * @param gc   FIXME
 * @param xoff FIXME
 * @param yoff FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_draw
 */
void
GrSetGCTSOffset(GR_GC_ID gc, int xoff, int yoff)
{
	nxSetGCTSOffsetReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCTSOffset);
	req->gcid = gc;
	req->xoffset = xoff;
	req->yoffset = yoff;
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the flag which chooses whether or not the background colour is used
 * when drawing bitmaps and text using the specified graphics context to the
 * specified value.
 *
 * @param gc  the ID of the graphics context to change the "use background" flag of
 * @param flag  flag specifying whether to use the background colour or not
 *
 * @ingroup nanox_draw
 */
void 
GrSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag)
{
	nxSetGCUseBackgroundReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCUseBackground);
	req->gcid = gc;
	req->flag = flag;
	UNLOCK(&nxGlobalLock);
}

/**
 * Attempts to locate a font with the desired attributes and returns a font
 * ID number which can be used to refer to it. If the plogfont argument is
 * not NULL, the values in that structure will be used to choose a font.
 * Otherwise, if the height is non zero, the built in font with the closest
 * height to that specified will be used. If the height is zero, the built
 * in font with the specified name will be used. If the desired font is not
 * found, the first built in font will be returned as a last resort.
 *
 * @param name  string containing the name of a built in font to look for
 * @param height  the desired height of the font
 * @param plogfont  pointer to a LOGFONT structure
 * @return a font ID number which can be used to refer to the font
 *
 * @ingroup nanox_font
 */
GR_FONT_ID
GrCreateFont(GR_CHAR *name, GR_COORD height, GR_LOGFONT *plogfont)
{
	GR_FONT_ID	fontid;

	LOCK(&nxGlobalLock);

	if (plogfont)
	{
		nxCreateLogFontReq *req;
		req = AllocReq(CreateLogFont);
		memcpy(&req->lf, plogfont, sizeof(*plogfont));

		if (TypedReadBlock(&fontid, sizeof(fontid), GrNumCreateLogFont) == -1)
			fontid = 0;
	}
	else
	{
		nxCreateFontReq *req;

		if (!name)
			name = "";

		req = AllocReqExtra(CreateFont, strlen(name) + 1);
		req->height = height;
		strcpy((char *)GetReqData(req), name);

		if (TypedReadBlock(&fontid, sizeof(fontid), GrNumCreateFont) == -1)
			fontid = 0;
	}

	UNLOCK(&nxGlobalLock);
	return fontid;
}

/**
 * Returns an array of strings containing the names of available fonts and an
 * integer that specifies the number of strings returned. 
 *
 * @param fonts  pointer used to return an array of font names.
 * @param numfonts  pointer used to return the number of names found.
 *
 * @ingroup nanox_font
 */
void 
GrGetFontList(GR_FONTLIST ***fonts, int *numfonts)
{
	nxGetFontListReq *req;
	char *tmpstr;
	GR_FONTLIST **flist;
	int num, len, i;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetFontList);

	if (TypedReadBlock(&num, sizeof(int), GrNumGetFontList) == -1)
		num = 0;
	
	*numfonts = num;

	if(num == -1)
		return;

	flist = (GR_FONTLIST**)malloc(num * sizeof(GR_FONTLIST*));

	for(i = 0; i < num; i++) 
		flist[i] = (GR_FONTLIST*)malloc(sizeof(GR_FONTLIST*));

	for(i = 0; i < num; i++) {
		ReadBlock(&len, sizeof(int));
		tmpstr = (char*)malloc(len * sizeof(char));
		ReadBlock(tmpstr, len * sizeof(char));
		flist[i]->ttname = tmpstr;

		ReadBlock(&len, sizeof(int));
		tmpstr = (char*)malloc(len * sizeof(char));
		ReadBlock(tmpstr, len * sizeof(char));
		flist[i]->mwname = tmpstr;
		
	}

	*fonts = flist;
	UNLOCK(&nxGlobalLock);
}

/**
 * Frees the specified font list array.
 *
 * @param fonts Pointer to array returned by GrGetFontList().
 * @param numfonts The number of font names in the array.
 *
 * @ingroup nanox_font
 */
void
GrFreeFontList(GR_FONTLIST ***fonts, int numfonts)
{
	int i;
	MWFONTLIST *g, **list = *fonts;

	LOCK(&nxGlobalLock);
	for (i = 0; i < numfonts; i++) {
		g = list[i];
		if(g) {
			if(g->mwname) 
				free(g->mwname);
			if(g->ttname) 
				free(g->ttname);
			free(g);
		}
	}
	free(list);
	*fonts = 0;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the size of the specified font to the specified size.
 *
 * @param fontid  the ID number of the font to change the size of
 * @param size  the size to change the font to
 *
 * @ingroup nanox_font
 */
void
GrSetFontSize(GR_FONT_ID fontid, GR_COORD size)
{
	nxSetFontSizeReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetFontSize);
	req->fontid = fontid;
	req->fontsize = size;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the rotation of the specified font to the specified angle.
 *
 * @param fontid  the ID number of the font to rotate
 * @param tenthdegrees  the angle to set the rotation to in tenths of a degree
 *
 * @ingroup nanox_font
 */
void
GrSetFontRotation(GR_FONT_ID fontid, int tenthdegrees)
{
	nxSetFontRotationReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetFontRotation);
	req->fontid = fontid;
	req->tenthdegrees = tenthdegrees;
	UNLOCK(&nxGlobalLock);
}

/**
 * Changes the attributes (GR_TFKERNING, GR_TFANTIALIAS, GR_TFUNDERLINE, etc.)
 * of the specified font according to the set and clear mask arguments.
 *
 * @param fontid  the ID of the font to set the attributes of
 * @param setflags  mask specifying attribute flags to set
 * @param clrflags  mask specifying attribute flags to clear
 *
 * @ingroup nanox_font
 */
void
GrSetFontAttr(GR_FONT_ID fontid, int setflags, int clrflags)
{
	nxSetFontAttrReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetFontAttr);
	req->fontid = fontid;
	req->setflags = setflags;
	req->clrflags = clrflags;
	UNLOCK(&nxGlobalLock);
}

/**
 * Frees all resources associated with the specified font ID, and if the font
 * is a non built in type and this is the last ID referring to it, unloads the
 * font from memory.
 *
 * @param fontid  the ID of the font to destroy
 *
 * @ingroup nanox_font
 */
void
GrDestroyFont(GR_FONT_ID fontid)
{
	nxDestroyFontReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(DestroyFont);
	req->fontid = fontid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the font to be used for text drawing in the specified graphics
 * context to the specified font ID.
 *
 * @param gc  the ID of the graphics context to set the font of
 * @param font  the ID of the font
 *
 * @ingroup nanox_font
 */
void
GrSetGCFont(GR_GC_ID gc, GR_FONT_ID font)
{
	nxSetGCFontReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetGCFont);
	req->gcid = gc;
	req->fontid = font;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws a line using the specified graphics context on the specified drawable
 * from (x1, y1) to (x2, y2), with coordinates given relative to the drawable.
 *
 * @param id  the ID of the drawable to draw the line on
 * @param gc  the ID of the graphics context to use when drawing the line
 * @param x1  the X coordinate of the start of the line relative to the drawable
 * @param y1  the Y coordinate of the start of the line relative to the drawable
 * @param x2  the X coordinate of the end of the line relative to the drawable
 * @param y2  the Y coordinate of the end of the line relative to the drawable
 *
 * @ingroup nanox_draw
 */
void 
GrLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1, GR_COORD x2,
	GR_COORD y2)
{
	nxLineReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(Line);
	req->drawid = id;
	req->gcid = gc;
	req->x1 = x1;
	req->y1 = y1;
	req->x2 = x2;
	req->y2 = y2;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draw the boundary of a rectangle of the specified dimensions and position
 * on the specified drawable using the specified graphics context.
 *
 * @param id  the ID of the drawable to draw the rectangle on
 * @param gc  the ID of the graphics context to use when drawing the rectangle
 * @param x  the X coordinate of the rectangle relative to the drawable
 * @param y  the Y coordinate of the rectangle relative to the drawable
 * @param width  the width of the rectangle
 * @param height  the height of the rectangle
 *
 * @ingroup nanox_draw
 */
void 
GrRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height)
{
	nxRectReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(Rect);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draw a filled rectangle of the specified dimensions and position on the
 * specified drawable using the specified graphics context.
 *
 * @param id  the ID of the drawable to draw the rectangle on
 * @param gc  the ID of the graphics context to use when drawing the rectangle
 * @param x  the X coordinate of the rectangle relative to the drawable
 * @param y  the Y coordinate of the rectangle relative to the drawable
 * @param width  the width of the rectangle
 * @param height  the height of the rectangle
 *
 * @ingroup nanox_draw
 */
void 
GrFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height)
{
	nxFillRectReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(FillRect);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws the boundary of ellipse at the specified position using the specified
 * dimensions and graphics context on the specified drawable.
 *
 * @param id  the ID of the drawable to draw the ellipse on
 * @param gc  the ID of the graphics context to use when drawing the ellipse
 * @param x  the X coordinate to draw the ellipse at relative to the drawable
 * @param y  the Y coordinate to draw the ellipse at relative to the drawable
 * @param rx  the radius of the ellipse on the X axis
 * @param ry  the radius of the ellipse on the Y axis
 *
 * @ingroup nanox_draw
 */
void 
GrEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx,
	GR_SIZE ry)
{
	nxEllipseReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(Ellipse);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->rx = rx;
	req->ry = ry;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws a filled ellipse at the specified position using the specified
 * dimensions and graphics context on the specified drawable.
 *
 * @param id  the ID of the drawable to draw the filled ellipse on
 * @param gc  the ID of the graphics context to use when drawing the ellipse
 * @param x  the X coordinate to draw the ellipse at relative to the drawable
 * @param y  the Y coordinate to draw the ellipse at relative to the drawable
 * @param rx  the radius of the ellipse on the X axis
 * @param ry  the radius of the ellipse on the Y axis
 *
 * @ingroup nanox_draw
 */
void 
GrFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE rx, GR_SIZE ry)
{
	nxFillEllipseReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(FillEllipse);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->rx = rx;
	req->ry = ry;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws an arc with the specified dimensions at the specified position
 * on the specified drawable using the specified graphics context.
 * The type specifies the fill type. Possible values include GR_ARC and
 * GR_PIE.
 *
 * @param id  the ID of the drawable to draw the arc on
 * @param gc  the graphics context to use when drawing the arc
 * @param x  the X coordinate to draw the arc at relative to the drawable
 * @param y  the Y coordinate to draw the arc at relative to the drawable
 * @param rx  the radius of the arc on the X axis
 * @param ry  the radius of the arc on the Y axis
 * @param ax  the X coordinate of the start of the arc relative to the drawable
 * @param ay  the Y coordinate of the start of the arc relative to the drawable
 * @param bx  the X coordinate of the end of the arc relative to the drawable
 * @param by  the Y coordinate of the end of the arc relative to the drawable
 * @param type  the fill style to use when drawing the arc
 *
 * @ingroup nanox_draw
 */
void	
GrArc(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE rx, GR_SIZE ry, GR_COORD ax, GR_COORD ay,
	GR_COORD bx, GR_COORD by, int type)
{
	nxArcReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(Arc);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->rx = rx;
	req->ry = ry;
	req->ax = ax;
	req->ay = ay;
	req->bx = bx;
	req->by = by;
	req->type = type;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws an arc with the specified dimensions at the specified position
 * on the specified drawable using the specified graphics context.
 * The type specifies the fill type. Possible values include GR_ARC and
 * GR_PIE. This function requires floating point support, and is slightly
 * slower than the GrArc() function which does not require floating point.
 *
 * @param id  the ID of the drawable to draw the arc on
 * @param gc  the graphics context to use when drawing the arc
 * @param x  the X coordinate to draw the arc at relative to the drawable
 * @param y  the Y coordinate to draw the arc at relative to the drawable
 * @param rx  the radius of the arc on the X axis
 * @param ry  the radius of the arc on the Y axis
 * @param angle1  the angle of the start of the arc
 * @param angle2  the angle of the end of the arc
 * @param type  the fill style to use when drawing the arc
 *
 * @ingroup nanox_draw
 */
void
GrArcAngle(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE rx, GR_SIZE ry, GR_COORD angle1, GR_COORD angle2, int type)
{
	nxArcAngleReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(ArcAngle);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->rx = rx;
	req->ry = ry;
	req->angle1 = angle1;
	req->angle2 = angle2;
	req->type = type;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws the monochrome bitmap data provided in the imagebits argument
 * at the specified position on the specified drawable using the specified
 * graphics context. Note that the bitmap data should be an array of aligned
 * 16 bit words. The usebackground flag in the graphics context specifies
 * whether to draw the background colour wherever a bit value is zero.
 *
 * @param id  the ID of the drawable to draw the bitmap onto
 * @param gc  the ID of the graphics context to use when drawing the bitmap
 * @param x  the X coordinate to draw the bitmap at relative to the drawable
 * @param y  the Y coordinate to draw the bitmap at relative to the drawable
 * @param width  the width of the bitmap
 * @param height  the height of the bitmap
 * @param imagebits  pointer to the bitmap data
 *
 * @ingroup nanox_draw
 */
void 
GrBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_BITMAP *imagebits)
{
	nxBitmapReq *req;
	long 	     bitmapsize;

	bitmapsize = (long)GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);
	LOCK(&nxGlobalLock);
	req = AllocReqExtra(Bitmap, bitmapsize);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	memcpy(GetReqData(req), imagebits, bitmapsize);
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws the image contained in the specified image structure onto the
 * specified drawable at the specified coordinates using the specified
 * graphics context.
 *
 * @param id  the ID of the drawable to draw the image onto
 * @param gc  the ID of the graphics context to use when drawing the image
 * @param x  the X coordinate to draw the image at relative to the drawable
 * @param y  the Y coordinate to draw the image at relative to the drawable
 * @param pimage  pointer to the image structure
 *
 * @ingroup nanox_draw
 */
void
GrDrawImageBits(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_IMAGE_HDR *pimage)
{
	nxDrawImageBitsReq	*req;
	int			imagesize;
	int			palsize;
	char			*addr;

	imagesize = pimage->pitch * pimage->height;
	palsize = pimage->palsize * sizeof(MWPALENTRY);
	LOCK(&nxGlobalLock);
	req = AllocReqExtra(DrawImageBits, imagesize + palsize);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	/* fill MWIMAGEHDR items passed externally*/
	req->width = pimage->width;
	req->height = pimage->height;
	req->planes = pimage->planes;
	req->bpp = pimage->bpp;
	req->pitch = pimage->pitch;
	req->bytesperpixel = pimage->bytesperpixel;
	req->compression = pimage->compression;
	req->palsize = pimage->palsize;
	req->transcolor = pimage->transcolor;
	addr = GetReqData(req);
	memcpy(addr, pimage->imagebits, imagesize);
	memcpy(addr+imagesize, pimage->palette, palsize);
	UNLOCK(&nxGlobalLock);
}

#if MW_FEATURE_IMAGES && defined(HAVE_FILEIO)
/**
 * Loads the specified image file and draws it at the specified position
 * on the specified drawable using the specified graphics context. The
 * width and height values specify the size of the image to draw- if the
 * actual image is a different size, it will be scaled to fit. The image type
 * is automatically detected using the magic numbers in the image header (ie.
 * the filename extension is irrelevant). The currently supported image types
 * include GIF, JPEG, Windows BMP, PNG, XPM, and both ascii and binary
 * variants of PBM, PGM, and PPM. However the image types supported by a
 * particular server depend on which image types were enabled in the server
 * configuration at build time. 
 *
 * @param id  the ID of the drawable to draw the image onto
 * @param gc  the ID of the graphics context to use when drawing the image
 * @param x  the X coordinate to draw the image at relative to the drawable
 * @param y  the Y coordinate to draw the image at relative to the drawable
 * @param width  the maximum image width
 * @param height  the maximum image height
 * @param path  string containing the filename of the image to load
 * @param flags  flags specific to the particular image loader
 *
 * @ingroup nanox_image
 */
void
GrDrawImageFromFile(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, char* path, int flags)
{
	nxDrawImageFromFileReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReqExtra(DrawImageFromFile, strlen(path)+1);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	req->flags = flags;
	memcpy(GetReqData(req), path, strlen(path)+1);
	UNLOCK(&nxGlobalLock);
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_FILEIO) */

#if MW_FEATURE_IMAGES && defined(HAVE_FILEIO)
/**
 * Loads the specified image file into a newly created server image buffer
 * and returns the ID of the buffer. The image type is automatically detected
 * using the magic numbers in the image header (ie. the filename extension is
 * irrelevant). The currently supported image types include GIF, JPEG, Windows
 * BMP, PNG, XPM, and both ascii and binary variants of PBM, PGM, and PPM.
 * However the image types supported by a particular server depend on which
 * image types were enabled in the server configuration at build time. 
 *
 * @param path  string containing the filename of the image to load
 * @param flags  flags specific to the particular image loader
 * @return ID of the image buffer the image was loaded into
 *
 * @ingroup nanox_image
 */
GR_IMAGE_ID
GrLoadImageFromFile(char *path, int flags)
{
	nxLoadImageFromFileReq *req;
	GR_IMAGE_ID		imageid;

	LOCK(&nxGlobalLock);
	req = AllocReqExtra(LoadImageFromFile, strlen(path)+1);
	req->flags = flags;
	memcpy(GetReqData(req), path, strlen(path)+1);

	if(TypedReadBlock(&imageid, sizeof(imageid),
	    GrNumLoadImageFromFile) == -1)
			imageid = 0;
	UNLOCK(&nxGlobalLock);
	return imageid;
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_FILEIO) */

#if MW_FEATURE_IMAGES
/**
 * Draws the image from the specified image buffer at the specified position
 * on the specified drawable using the specified graphics context. The
 * width and height values specify the size of the image to draw- if the
 * actual image is a different size, it will be scaled to fit.
 *
 * @param id  the ID of the drawable to draw the image onto
 * @param gc  the ID of the graphics context to use when drawing the image
 * @param x  the X coordinate to draw the image at relative to the drawable
 * @param y  the Y coordinate to draw the image at relative to the drawable
 * @param width  the maximum image width
 * @param height  the maximum image height
 * @param imageid  the ID of the image buffer containing the image to display
 *
 * @ingroup nanox_image
 */ 
void
GrDrawImageToFit(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, GR_IMAGE_ID imageid)
{
	nxDrawImageToFitReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(DrawImageToFit);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	req->imageid = imageid;
	UNLOCK(&nxGlobalLock);
}
#endif /* MW_FEATURE_IMAGES */

#if MW_FEATURE_IMAGES
/**
 * Destroys the specified image buffer and reclaims the memory used by it.
 *
 * @param id  ID of the image buffer to free
 *
 * @ingroup nanox_image
 */
void
GrFreeImage(GR_IMAGE_ID id)
{
	nxFreeImageReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(FreeImage);
	req->id = id;
	UNLOCK(&nxGlobalLock);
}
#endif /* MW_FEATURE_IMAGES */

#if MW_FEATURE_IMAGES
/**
 * Fills in the specified image information structure with the details of the
 * specified image buffer.
 *
 * @param id  ID of an image buffer
 * @param iip  pointer to a GR_IMAGE_INFO structure
 *
 * @ingroup nanox_image
 */
void
GrGetImageInfo(GR_IMAGE_ID id, GR_IMAGE_INFO *iip)
{
	nxGetImageInfoReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetImageInfo);
	req->id = id;
	TypedReadBlock(iip, sizeof(GR_IMAGE_INFO), GrNumGetImageInfo);
	UNLOCK(&nxGlobalLock);
}
#endif /* MW_FEATURE_IMAGES */

/**
 * Send a large buffer (typically an image) from the client to the server.
 *
 * @param buffer The buffer to send.
 * @param size   The size of the buffer, in bytes.
 * @return       A buffer ID allocated by the server, or 0 on error.
 */
static int
sendImageBuffer(const void *buffer, int size)
{
	int bufid;
	int bufsize = size;
	const char *bufptr = (const char *)buffer;
	nxImageBufferAllocReq *alloc;
	nxImageBufferSendReq *send;

	/* Step 1 - Allocate a buffer on the other side */
	alloc = AllocReq(ImageBufferAlloc);
	alloc->size = size;
	TypedReadBlock(&bufid, sizeof(bufid), GrNumImageBufferAlloc);

	if (bufid < 0)
		return 0;

	/* step 2 - Send the buffer across */
	while(bufsize > 0) {
		int chunk = (MAXREQUESTSZ - sizeof(nxImageBufferSendReq));
		if (chunk > bufsize)
			chunk = bufsize;

		send = AllocReqExtra(ImageBufferSend, chunk);
		send->buffer_id = bufid;
		send->size = chunk;

		memcpy(GetReqData(send), bufptr, chunk);
		bufptr += chunk;
		bufsize -= chunk;
	}

	return bufid;
}

#if MW_FEATURE_IMAGES
/**
 * FIXME
 *
 * @param buffer FIXME
 * @param size   FIXME
 * @param flags  FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_image
 */
GR_IMAGE_ID
GrLoadImageFromBuffer(void *buffer, int size, int flags)
{
	nxLoadImageFromBufferReq *req;
	int bufid;
	GR_IMAGE_ID imageid;

	LOCK(&nxGlobalLock);
	/* Step 1 - Send the buffer to the other side */
	bufid = sendImageBuffer(buffer, size);

	if (!bufid) {
		UNLOCK(&nxGlobalLock);
		return 0;
	}

	/* Step 2 - Send the command to load the image */
	/* Note - This will free the buffer automagically */
	req = AllocReq(LoadImageFromBuffer);
	req->flags = flags;
	req->buffer = bufid;

	if (TypedReadBlock(&imageid, sizeof(imageid),
	    GrNumLoadImageFromBuffer) == -1)
		imageid = 0;
	UNLOCK(&nxGlobalLock);
	return imageid;
}
#endif /* MW_FEATURE_IMAGES */

#if MW_FEATURE_IMAGES
/**
 * FIXME
 *
 * @param id     FIXME
 * @param gc     FIXME
 * @param x      FIXME
 * @param y      FIXME
 * @param width  FIXME
 * @param height FIXME
 * @param buffer FIXME
 * @param size   FIXME
 * @param flags  FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_image
 */
void
GrDrawImageFromBuffer(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
      GR_SIZE width, GR_SIZE height, void *buffer, int size, int flags)
{
	nxDrawImageFromBufferReq *req;
	int bufid;

	LOCK(&nxGlobalLock);
	/* Step 1 - Send the buffer to the other side */
	bufid = sendImageBuffer(buffer, size);

	if (!bufid) {
		UNLOCK(&nxGlobalLock);
		return;
	}

	/* Step 2 - Send the command to load/draw the image */
	/* Note - This will free the buffer automagically */
	req = AllocReq(DrawImageFromBuffer);
	req->flags = flags;
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	req->flags = flags;
	req->buffer = bufid;
	UNLOCK(&nxGlobalLock);
}
#endif /* MW_FEATURE_IMAGES */


/*
 * FIXME ... what does this comment relate to?
 *
 * Draw a rectangular area in the specified drawable using the specified
 * graphics context.  This differs from rectangle drawing in that the
 * color values for each pixel in the rectangle are specified.
 * The color table is indexed
 * row by row.  Values whose color matches the background color are only
 * written if the usebackground flag is set in the GC.
 *
 * The pixels are packed according to pixtype:
 *
 * pixtype		array of
 * MWPF_RGB		MWCOLORVAL (unsigned long)
 * MWPF_PIXELVAL	MWPIXELVAL (compile-time dependent)
 * MWPF_PALETTE		unsigned char
 * MWPF_TRUECOLOR0888	unsigned long
 * MWPF_TRUECOLOR888	packed struct {char r,char g,char b} (24 bits)
 * MWPF_TRUECOLOR565	unsigned short
 * MWPF_TRUECOLOR555	unsigned short
 * MWPF_TRUECOLOR332	unsigned char
 */
/**
 * Draws the specified pixel array of the specified size and format onto the
 * specified drawable using the specified graphics context at the specified
 * position. Note that colour conversion is currently only performed when using
 * the GR_PF_RGB format, which is an unsigned long containing RGBX data.
 *
 * @param id  the ID of the drawable to draw the area onto
 * @param gc  the ID of the graphics context to use when drawing the area
 * @param x  the X coordinate to draw the area at relative to the drawable
 * @param y  the Y coordinate to draw the area at relative to the drawable
 * @param width  the width of the area
 * @param height  the height of the area
 * @param pixels  pointer to an array containing the pixel data
 * @param pixtype  the format of the pixel data
 *
 * @ingroup nanox_draw
 */
void 
GrArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, void *pixels, int pixtype)
{
	nxAreaReq *req;
	long       size;
	long       chunk_y;
	int        pixsize;

	/* Calculate size of packed pixels*/
	switch(pixtype) {
	case MWPF_RGB:
		pixsize = sizeof(MWCOLORVAL);
		break;
	case MWPF_PIXELVAL:
		pixsize = sizeof(MWPIXELVAL);
		break;
	case MWPF_PALETTE:
	case MWPF_TRUECOLOR332:
		pixsize = sizeof(unsigned char);
		break;
	case MWPF_TRUECOLOR0888:
		pixsize = sizeof(unsigned long);
		break;
	case MWPF_TRUECOLOR888:
		pixsize = 3;
		break;
	case MWPF_TRUECOLOR565:
	case MWPF_TRUECOLOR555:
		pixsize = sizeof(unsigned short);
		break;
	default:
		return;
	}

	LOCK(&nxGlobalLock);
	/* Break request into MAXREQUESTSZ size packets*/
	while(height > 0) {
		chunk_y = (MAXREQUESTSZ - sizeof(nxAreaReq)) /
			((long)width * pixsize);
		if(chunk_y > height)
			chunk_y = height;
		size = chunk_y * ((long)width * pixsize);
		req = AllocReqExtra(Area, size);
		req->drawid = id;
		req->gcid = gc;
		req->x = x;
		req->y = y;
		req->width = width;
		req->height = chunk_y;
		req->pixtype = pixtype;
		memcpy(GetReqData(req), pixels, size);
		pixels = (void *)(((char *)pixels) + size);
		y += chunk_y;
		height -= chunk_y;
	}
	UNLOCK(&nxGlobalLock);
}

/**
 * Copies the specified area of the specified size between the specified
 * drawables at the specified positions using the specified graphics context
 * and ROP codes. 0 is a sensible default ROP code in most cases.
 *
 * @param id  the ID of the drawable to copy the area to
 * @param gc  the ID of the graphics context to use when copying the area
 * @param x  the X coordinate to copy the area to within the destination drawable
 * @param y  the Y coordinate to copy the area to within the destination drawable
 * @param width  the width of the area to copy
 * @param height  the height of the area to copy
 * @param srcid  the ID of the drawable to copy the area from
 * @param srcx  the X coordinate to copy the area from within the source drawable
 * @param srcy  the Y coordinate to copy the area from within the source drawable
 * @param op  the ROP codes to pass to the blitter when performing the copy
 *
 * @ingroup nanox_draw
 */
void
GrCopyArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, GR_DRAW_ID srcid,
	GR_COORD srcx, GR_COORD srcy, unsigned long op)
{
	nxCopyAreaReq *req;

	LOCK(&nxGlobalLock);
        req = AllocReq(CopyArea);
        req->drawid = id;
        req->gcid = gc;
        req->x = x;
        req->y = y;
        req->width = width;
        req->height = height;
        req->srcid = srcid;
        req->srcx = srcx;
        req->srcy = srcy;
        req->op = op;
	UNLOCK(&nxGlobalLock);
}
   
/**
 * Reads the pixel data of the specified size from the specified position on
 * the specified drawable into the specified pixel array. If the drawable is
 * a window, the data returned will be the pixel values from the relevant
 * position on the screen regardless of whether the window is obscured by other
 * windows. If the window is unmapped, or partially or fully outside a window
 * boundary, black pixel values will be returned.
 *
 * @param id  the ID of the drawable to read an area from
 * @param x  the X coordinate to read the area from relative to the drawable
 * @param y  the Y coordinate to read the area from relative to the drawable
 * @param width  the width of the area to read
 * @param height  the height of the area to read
 * @param pixels  pointer to an area of memory to place the pixel data in
 *
 * @ingroup nanox_draw
 */
void 
GrReadArea(GR_DRAW_ID id,GR_COORD x,GR_COORD y,GR_SIZE width,
	GR_SIZE height, GR_PIXELVAL *pixels)
{
	nxReadAreaReq *req;
	long           size;

	LOCK(&nxGlobalLock);
	req = AllocReq(ReadArea);
	req->drawid = id;
	req->x = x;
	req->y = y;
	req->width = width;
	req->height = height;
	size = (long)width * height * sizeof(MWPIXELVAL);
	TypedReadBlock(pixels, size, GrNumReadArea);
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws a point using the specified graphics context at the specified position
 * on the specified drawable.
 *
 * @param id  the ID of the drawable to draw a point on
 * @param gc  the ID of the graphics context to use when drawing the point
 * @param x  the X coordinate to draw the point at relative to the drawable
 * @param y  the Y coordinate to draw the point at relative to the drawable
 *
 * @ingroup nanox_draw
 */
void 
GrPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y)
{
	nxPointReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(Point);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws a set of points using the specified graphics context at the positions
 * specified by the point table on the specified drawable.
 *
 * @param id  the ID of the drawable to draw a point on
 * @param gc  the ID of the graphics context to use when drawing the point
 * @param count  the number of points in the point table
 * @param pointtable  pointer to a GR_POINT array which lists the points to draw
 *
 * @ingroup nanox_draw
 */
void
GrPoints(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	nxPointsReq *req;
	long	size;

	size = (long)count * sizeof(GR_POINT);
	LOCK(&nxGlobalLock);
	req = AllocReqExtra(Points, size);
	req->drawid = id;
	req->gcid = gc;
	memcpy(GetReqData(req), pointtable, size);
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws an unfilled polygon on the specified drawable using the specified
 * graphics context. The polygon is specified by an array of point structures.
 * The polygon is not automatically closed- if a closed polygon is desired,
 * the last point must be the same as the first.
 *
 * @param id  the ID of the drawable to draw the polygon onto
 * @param gc  the ID of the graphics context to use when drawing the polygon
 * @param count  the number of points in the point array
 * @param pointtable  pointer to an array of points describing the polygon
 *
 * @ingroup nanox_draw
 */
void 
GrPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	nxPolyReq *req;
	long       size;

	LOCK(&nxGlobalLock);
	size = (long)count * sizeof(GR_POINT);
	req = AllocReqExtra(Poly, size);
	req->drawid = id;
	req->gcid = gc;
	memcpy(GetReqData(req), pointtable, size);
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws a filled polygon on the specified drawable using the specified
 * graphics context. The polygon is specified by an array of point structures.
 * The polygon is automatically closed- the last point need not be the same as
 * the first in order for the polygon to be closed.
 *
 * @param id  the ID of the drawable to draw the polygon onto
 * @param gc  the ID of the graphics context to use when drawing the polygon
 * @param count  the number of points in the point array
 * @param pointtable  pointer to an array of points describing the polygon
 *
 * @ingroup nanox_draw
 */
void 
GrFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,GR_POINT *pointtable)
{
	nxFillPolyReq *req;
	long           size;

	LOCK(&nxGlobalLock);
	size = (long)count * sizeof(GR_POINT);
	req = AllocReqExtra(FillPoly, size);
	req->drawid = id;
	req->gcid = gc;
	memcpy(GetReqData(req), pointtable, size);
	UNLOCK(&nxGlobalLock);
}

/**
 * Draws the specified text string at the specified position on the specified
 * drawable using the specified graphics context and flags. The default flags
 * specify ASCII encoding and baseline alignment.
 *
 * @param id  the ID of the drawable to draw the text string onto
 * @param gc  the ID of the graphics context to use when drawing the text string
 * @param x  the X coordinate to draw the string at relative to the drawable
 * @param y  the Y coordinate to draw the string at relative to the drawable
 * @param str  the text string to draw
 * @param count  the number of characters (not bytes) in the string
 * @param flags  flags specifying text encoding, alignment, etc.
 *
 * @ingroup nanox_font
 */
void 
GrText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, void *str,
	GR_COUNT count, GR_TEXTFLAGS flags)
{
	nxTextReq *req;
	int	   size;

	/* use strlen as char count when ascii or dbcs*/
	if(count == -1 && (flags&MWTF_PACKMASK) == MWTF_ASCII)
		count = strlen((char *)str);

	size = nxCalcStringBytes(str, count, flags);

	LOCK(&nxGlobalLock);
	req = AllocReqExtra(Text, size);
	req->drawid = id;
	req->gcid = gc;
	req->x = x;
	req->y = y;
	req->count = count;
	req->flags = flags;
	memcpy(GetReqData(req), str, size);
	UNLOCK(&nxGlobalLock);
}


/**
 * Retrieves the system palette and places it in the specified palette
 * structure.
 *
 * @param pal  pointer to a palette structure to fill in with the system palette
 *
 * @ingroup nanox_color
 */
void
GrGetSystemPalette(GR_PALETTE *pal)
{
	LOCK(&nxGlobalLock);
	AllocReq(GetSystemPalette);
	TypedReadBlock(pal, sizeof(*pal), GrNumGetSystemPalette);
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the system palette to the values stored in the specified palette
 * structure. The values before the specified first value are not set.
 *
 * @param first  the first palette value to set
 * @param pal  pointer to a palette structure containing the new values
 *
 * @ingroup nanox_color
 */
void
GrSetSystemPalette(GR_COUNT first, GR_PALETTE *pal)
{
	nxSetSystemPaletteReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetSystemPalette);
	req->first = first;
	req->count = pal->count;
	memcpy(req->palette, pal->palette, sizeof(GR_PALENTRY) * pal->count);
	UNLOCK(&nxGlobalLock);
}

/**
 * Calculates the pixel value to use to display the specified colour value.
 * The colour value is specified as a GR_COLOR, which is a 32 bit truecolour
 * value stored as RGBX. The pixel value size depends on the architecture.
 *
 * @param c  the colour value to find
 * @param retpixel  pointer to the returned pixel value
 *
 * @ingroup nanox_color
 */
void
GrFindColor(GR_COLOR c, GR_PIXELVAL *retpixel)
{
	nxFindColorReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(FindColor);
	req->color = c;
	TypedReadBlock(retpixel, sizeof(*retpixel), GrNumFindColor);
	UNLOCK(&nxGlobalLock);
}

/**
 * Requests a shared memory area of the specified size to use for transferring
 * command arguments. This is faster but less portable than the standard BSD
 * sockets method of communication (and of course will only work if the client
 * and server are on the same machine). Apart from the initial allocation of
 * the area using this call, the use of shared memory is completely
 * transparent. Additionally, if the allocation fails we silently and
 * automatically fall back on socket communication. It is safe to call this
 * function even if shared memory support is not compiled in; it will simply
 * do nothing.
 *
 * @param shmsize  the size of the shared memory area to allocate
 *
 * @todo FIXME: how does the user decide what size of shared memory area to allocate?
 *
 * @ingroup nanox_misc
 */
void
GrReqShmCmds(long shmsize)
{
#if HAVE_SHAREDMEM_SUPPORT
	nxReqShmCmdsReq	req;
	int key, shmid;

	if (nxSharedMem != 0)
		return;

	LOCK(&nxGlobalLock);
	GrFlush();

	shmsize = (shmsize+SHM_BLOCK_SIZE-1) & ~(SHM_BLOCK_SIZE-1);
	req.reqType = GrNumReqShmCmds;
	req.hilength = 0;
	req.length = sizeof(req);
	req.size = shmsize;

	nxWriteSocket((char *)&req,sizeof(req));
	ReadBlock(&key,sizeof(key));

	if (!key) {
		EPRINTF("nxclient: no shared memory support on server\n");
		UNLOCK(&nxGlobalLock);
		return;
	}

	shmid = shmget(key, shmsize, 0);
	if (shmid == -1) {
		EPRINTF("nxclient: Can't shmget key %d: %m\n", key);
		UNLOCK(&nxGlobalLock);
		return;
	}

	nxSharedMem = shmat(shmid, 0, 0);
	shmctl(shmid,IPC_RMID,0);	/* Prevent other from attaching */
	if (nxSharedMem == (char *)-1) {
		UNLOCK(&nxGlobalLock);
		return;
	}

	nxSharedMemSize = shmsize;
	nxAssignReqbuffer(nxSharedMem, shmsize);
	UNLOCK(&nxGlobalLock);
#endif /* HAVE_SHAREDMEM_SUPPORT*/
}

/**
 * Sets the pointer invisible if the visible parameter is GR_FALSE, or visible
 * if it is GR_TRUE, then moves the pointer to the specified position and
 * generates a mouse event with the specified button status. Also performs
 * a GrFlush() so that the event takes effect immediately.
 *
 * @param x  the X coordinate of the pointer event relevant to the root window
 * @param y  the Y coordinate of the pointer event relevant to the root window
 * @param button  the pointer button status
 * @param visible  whether to display the pointer after the event
 *
 * @ingroup nanox_misc
 */
void
GrInjectPointerEvent(GR_COORD x, GR_COORD y, int button, int visible)
{
	nxInjectEventReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(InjectEvent);
	req->event_type = GR_INJECT_EVENT_POINTER;
	req->event.pointer.visible = visible;
	req->event.pointer.x = x;
	req->event.pointer.y = y;
	req->event.pointer.button = button;

	GrFlush();	/* FIXME?*/
	UNLOCK(&nxGlobalLock);
}

/**
 * Sends a keyboard event to the specified window, or to the window with the
 * current keyboard focus if 0 is used as the ID. The other arguments
 * correspond directly to the fields of the same names in the keyboard event
 * structure.
 *
 * @param wid      ID of the window to send the event to, or 0.
 * @param keyvalue Unicode keystroke value to inject.
 * @param modifiers Modifiers (shift, ctrl, alt, etc.) to inject.
 * @param scancode The key scan code to inject.
 * @param pressed  TRUE for a key press, FALSE for a key release.
 *
 * @ingroup nanox_misc
 */
void
GrInjectKeyboardEvent(GR_WINDOW_ID wid, GR_KEY keyvalue,
	GR_KEYMOD modifiers, GR_SCANCODE scancode, GR_BOOL pressed)
{
	nxInjectEventReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(InjectEvent);
	req->event_type = GR_INJECT_EVENT_KEYBOARD;
	req->event.keyboard.wid = wid;
	req->event.keyboard.keyvalue = keyvalue;
	req->event.keyboard.modifier = modifiers;
	req->event.keyboard.scancode = scancode;
	req->event.keyboard.pressed = pressed;

	GrFlush();	/* FIXME?*/
	UNLOCK(&nxGlobalLock);
}

/**
 * Copies the provided GR_WM_PROPERTIES structure into the the GR_WM_PROPERTIES
 * structure of the specified window id.
 *
 * @param wid  the ID of the window to set the WM properties of
 * @param props  pointer to a GR_WM_PROPERTIES structure
 *
 * @ingroup nanox_window
 */
void
GrSetWMProperties(GR_WINDOW_ID wid, GR_WM_PROPERTIES *props)
{
	nxSetWMPropertiesReq *req;
	char		*addr;
	int		s;

	if ((props->flags & GR_WM_FLAGS_TITLE) && props->title)
		s = strlen(props->title) + 1;
	else s = 0;

	LOCK(&nxGlobalLock);
	req = AllocReqExtra(SetWMProperties, s + sizeof(GR_WM_PROPERTIES));
	req->windowid = wid;
	addr = GetReqData(req);
	memcpy(addr, props, sizeof(GR_WM_PROPERTIES));
	if (s)
		memcpy(addr + sizeof(GR_WM_PROPERTIES), props->title, s);
	UNLOCK(&nxGlobalLock);
}

/**
 * Reads the GR_WM_PROPERTIES structure for the window with the specified
 * id and fills in the provided structure with the information.
 * It is the callers responsibility to free the title member as it is allocated
 * dynamically. The title field will be set to NULL if the window has no title.
 *
 * @param wid  the ID of the window to retreive the WM properties of
 * @param props  pointer to a GR_WM_PROPERTIES structure to fill in
 *
 * @ingroup nanox_window
 */
void
GrGetWMProperties(GR_WINDOW_ID wid, GR_WM_PROPERTIES *props)
{
	nxGetWMPropertiesReq *req;
	UINT16 textlen;
	GR_CHAR c;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetWMProperties);
	req->windowid = wid;

	TypedReadBlock(props, sizeof(GR_WM_PROPERTIES), GrNumGetWMProperties);
	ReadBlock(&textlen, sizeof(textlen));
	if(!textlen) {
		props->title = NULL;
		UNLOCK(&nxGlobalLock);
		return;
	}
	if(!(props->title = malloc(textlen))) {
		/* Oh dear, we're out of memory but still have to purge the
		   requested data (and throw it away) */
		while(textlen--)
			ReadBlock(&c, 1);
	} else {
		ReadBlock(props->title, textlen);
	}
	UNLOCK(&nxGlobalLock);
}

/**
 * Sends a CLOSE_REQ event to the specified window if the client has selected
 * to receive CLOSE_REQ events on this window. Used to request an application
 * to shut down but not force it to do so immediately, so the application can
 * ask whether to save changed files before shutting down cleanly.
 *
 * @param wid  the ID of the window to send the CLOSE_REQ event to
 *
 * @ingroup nanox_window
 */
void 
GrCloseWindow(GR_WINDOW_ID wid)
{
	nxCloseWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(CloseWindow);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Forcibly disconnects the client which owns this window with the specified
 * ID number. Used to kill an application which has locked up and is not
 * responding to CLOSE_REQ events.
 *
 * @param wid  the ID of the window to kill
 *
 * @ingroup nanox_window
 */
void 
GrKillWindow(GR_WINDOW_ID wid)
{
	nxKillWindowReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(KillWindow);
	req->windowid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the number of seconds of inactivity before a screen saver activate
 * event is sent to the root window ID. A value of 0 activates the
 * screen saver immediately, and a value of -1 disables the screen saver
 * function.
 *
 * @param timeout  the number of seconds of inactivity before screen saver activates
 *
 * @ingroup nanox_misc
 */
void 
GrSetScreenSaverTimeout(GR_TIMEOUT timeout)
{
	nxSetScreenSaverTimeoutReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetScreenSaverTimeout);
	req->timeout = timeout;
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the current selection (otherwise known as the clipboard) ownership
 * to the specified window. Specifying an owner of 0 disowns the selection.
 * The typelist argument is a list of mime types (seperated by space
 * characters) which the window is able to supply the data as. At least one
 * type must be specified unless you are disowning the selection (typically
 * text/plain for plain ASCII text or text/uri-list for a filename).
 *
 * The window which owns the current selection must be prepared to handle
 * SELECTION_LOST events (received when another window takes ownership of the
 * selection) and CLIENT_DATA_REQ events (received when a client wishes to
 * retreive the selection data).
 *
 * @param wid  the ID of the window to set the selection owner to
 * @param typelist  list of mime types selection data can be supplied as
 *
 * @ingroup nanox_selection
 */
void
GrSetSelectionOwner(GR_WINDOW_ID wid, GR_CHAR *typelist)
{
	nxSetSelectionOwnerReq *req;
	char *p;
	int len;

	LOCK(&nxGlobalLock);
	if(wid) {
		len = strlen(typelist) + 1;
		req = AllocReqExtra(SetSelectionOwner, len);
		p = GetReqData(req);
		memcpy(p, typelist, len);
	} else {
		req = AllocReq(SetSelectionOwner);
	}

	req->wid = wid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Finds the window which currently owns the selection and returns its ID,
 * or 0 if no window currently owns the selection. A pointer to the list of
 * mime types the selection owner is capable of supplying is placed in the
 * pointer specified by the typelist argument. The typelist is null terminated,
 * and the fields are seperated by space characters. It is the callers
 * responsibility to free the typelist string, as it is allocated dynamically.
 * If the allocation fails, it will be set to a NULL pointer, so remember to
 * check the value of it before using it.
 *
 * @param typelist  pointer used to return the list of available mime types 
 * @return the ID of the window which currently owns the selection, or 0
 *
 * @ingroup nanox_selection
 */
GR_WINDOW_ID
GrGetSelectionOwner(GR_CHAR **typelist)
{
	nxGetSelectionOwnerReq *req;
	UINT16 textlen;
	GR_CHAR c;
	GR_WINDOW_ID wid;

	LOCK(&nxGlobalLock);
	req = AllocReq(GetSelectionOwner);
	TypedReadBlock(&wid, sizeof(wid), GrNumGetSelectionOwner);
	if(wid) {
		ReadBlock(&textlen, sizeof(textlen));
		if(!(*typelist = malloc(textlen))) {
			/* Oh dear, we're out of memory but still have to
			purge the requested data (and throw it away) */
			while(textlen--)
				ReadBlock(&c, 1);
		} else {
			ReadBlock(*typelist, textlen);
		}
	}

	UNLOCK(&nxGlobalLock);
	return wid;
}

/**
 * Sends a CLIENT_DATA_REQ event to the specified window. Used for requesting
 * both selection and "drag and drop" data. The mimetype argument specifies
 * the format of the data you would like to receive, as an index into the list
 * returned by GrGetSelectionOwner (the first type in the list is index 0).
 * The server makes no guarantees as to when, or even if, the client will
 * reply to the request. If the client does reply, the reply will take the
 * form of one or more CLIENT_DATA events. The request serial number is
 * typically a unique ID which the client can assign to a request in order for
 * it to be able to keep track of transfers (CLIENT_DATA events contain the
 * same number in the sid field). Remember to free the data field of the
 * CLIENT_DATA events as they are dynamically allocated. Also note that if
 * the allocation fails the data field will be set to NULL, so you should
 * check the value before using it.
 *
 * @param wid  the ID of the window requesting the data
 * @param rid  the ID of the window to request the data from
 * @param serial  the serial number of the request
 * @param mimetype  the number of the desired mime type to request
 *
 * @ingroup nanox_selection
 */
void
GrRequestClientData(GR_WINDOW_ID wid, GR_WINDOW_ID rid, GR_SERIALNO serial,
							GR_MIMETYPE mimetype)
{
	nxRequestClientDataReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(RequestClientData);
	req->wid = wid;
	req->rid = rid;
	req->serial = serial;
	req->mimetype = mimetype;
	UNLOCK(&nxGlobalLock);
}

/**
 * Used as the response to a CLIENT_DATA_REQ event. Sends the specified data
 * of the specified length to the specified window using the specified source
 * window ID and transfer serial number. Any fragmentation of the data into
 * multiple CLIENT_DATA events which is required is handled automatically.
 * The serial number should always be set to the value supplied by the
 * CLIENT_DATA_REQ event. The thislen parameter is used internally to split
 * the data up into packets. It should be set to the same value as the len
 * parameter.
 *
 * @param wid     The ID of the window sending the data.
 * @param did     The ID of the destination window.
 * @param serial  The serial number of the request.
 * @param len     Number of bytes of data to transfer.
 * @param thislen Number of bytes in this packet.
 * @param data    Pointer to the data to transfer.
 *
 * @ingroup nanox_selection
 */
void
GrSendClientData(GR_WINDOW_ID wid, GR_WINDOW_ID did, GR_SERIALNO serial,
			GR_LENGTH len, GR_LENGTH thislen, void *data)
{
	nxSendClientDataReq *req;
	char *p;
	GR_LENGTH l, pos = 0;

	LOCK(&nxGlobalLock);
	while(pos < len) {
		l = MAXREQUESTSZ - sizeof(nxSendClientDataReq);
		if(l > (len - pos)) l = len - pos;
		req = AllocReqExtra(SendClientData, l);
		req->wid = wid;
		req->did = did;
		req->serial = serial;
		req->len = len;
		p = GetReqData(req);
		memcpy(p, ((char *)data + pos), l);
		pos += l;
	}
	UNLOCK(&nxGlobalLock);
}

/**
 * Asks the server to ring the console bell on behalf of the client (intended
 * for terminal apps to be able to ring the bell on the server even if they
 * are running remotely).
 *
 * @ingroup nanox_misc
 */
void
GrBell(void)
{
	LOCK(&nxGlobalLock);
	AllocReq(Bell);
	UNLOCK(&nxGlobalLock);
}

/**
 * Sets the background of the specified window to the specified pixmap.
 * The flags which specify how to draw the pixmap (in the top left of the
 * window, in the centre of the window, tiled, etc.) are those which start with
 * GR_BACKGROUND_ in nano-X.h. If the pixmap value is 0, the server will
 * disable the background pixmap and return to using a solid colour fill.
 *
 * @param wid  ID of the window to set the background of
 * @param pixmap  ID of the pixmap to use as the background
 * @param flags  flags specifying how to draw the pixmap onto the window
 *
 * @ingroup nanox_window
 */
void
GrSetBackgroundPixmap(GR_WINDOW_ID wid, GR_WINDOW_ID pixmap, int flags)
{
	nxSetBackgroundPixmapReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetBackgroundPixmap);
	req->wid = wid;
	req->pixmap = pixmap;
	req->flags = flags;
	UNLOCK(&nxGlobalLock);
}

/**
 * Destroys the specified server-based cursor and
 * reclaims the memory used by it.
 *
 * @param cid  ID of the cursor to destory
 *
 * @ingroup nanox_cursor
 */
void
GrDestroyCursor(GR_CURSOR_ID cid)
{
	nxDestroyCursorReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(DestroyCursor);
	req->cursorid = cid;
	UNLOCK(&nxGlobalLock);
}

/**
 * Return window parent and list of children.
 * Caller must free() children list after use.
 *
 * @param wid  window ID for query
 * @param parentid  returned parent ID
 * @param children  returned children ID list
 * @param nchildren  returned children count
 *
 * @ingroup nanox_window
 */
void
GrQueryTree(GR_WINDOW_ID wid, GR_WINDOW_ID *parentid, GR_WINDOW_ID **children,
	GR_COUNT *nchildren)
{
	nxQueryTreeReq *req;
	GR_COUNT	count;
	GR_WINDOW_ID	dummy;

	LOCK(&nxGlobalLock);
	req = AllocReq(QueryTree);
	req->windowid = wid;

	TypedReadBlock(parentid, sizeof(*parentid), GrNumQueryTree);
	ReadBlock(nchildren, sizeof(*nchildren));
	if (!*nchildren) {
		*children = NULL;
		UNLOCK(&nxGlobalLock);
		return;
	}
	count = *nchildren;
	if(!(*children = malloc(count * sizeof(GR_WINDOW_ID)))) {
		/* We're out of memory but still have to purge the
		   requested data (and throw it away) */
		while(count--)
			ReadBlock(&dummy, sizeof(GR_WINDOW_ID));
	} else {
		ReadBlock(*children, count * sizeof(GR_WINDOW_ID));
	}
	UNLOCK(&nxGlobalLock);
}

#if YOU_WANT_TO_IMPLEMENT_DRAG_AND_DROP
/*
 * Enables the specified window to be used as a drag and drop source. The
 * specified pixmap will be used as the icon shown whilst dragging, and the
 * null terminated, newline seperated list of mime types which the data can
 * be supplied as is specified by the typelist argument. At least one type
 * (typically text/plain for plain ASCII or text/uri-list for a filename or
 * list of filenames) must be specified. When the icon is dropped,
 * the window which it is dropped on will receive a DROP event (providing it
 * has selected for DROP events), and if the client wishes to accept the data
 * and is able to handle one of the mime types in the type list, it should use
 * GrRequestClientData() to retrieve the data from the drag and drop source
 * window. Remember to free the typelist field of the DROP event as it is
 * dynamically allocated. It is possible for a client to select for DROP events
 * on the Root window if it is desired to allow dropping icons on the desktop.
 *
 * @param wid  the ID of the window to use as the drag and drop source window
 * @param iid  the ID of the pixmap to use as the drag and drop icon
 * @param typelist  list of mime types the drag and drop data can be supplied as
 *
 * @ingroup nanox_window
 */
void
GrRegisterDragAndDropWindow(GR_WINDOW_ID wid, GR_WINDOW_ID iid, GR_CHAR *typelist)
{
}
#endif


#if MW_FEATURE_TIMERS
/**
 * FIXME
 *
 * @param wid    FIXME
 * @param period FIXME
 * @return       FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_timer
 */
GR_TIMER_ID
GrCreateTimer (GR_WINDOW_ID wid, GR_TIMEOUT period)
{
	nxCreateTimerReq  *req;
	GR_TIMER_ID  timerid;

	LOCK(&nxGlobalLock);
	req = AllocReq(CreateTimer);
	req->wid = wid;
	req->period = period;

	if(TypedReadBlock(&timerid, sizeof (timerid), GrNumCreateTimer) == -1)
		timerid = 0;
	UNLOCK(&nxGlobalLock);
	return timerid;
}
#endif /* MW_FEATURE_TIMERS */

#if MW_FEATURE_TIMERS
/**
 * FIXME
 *
 * @param tid    FIXME
 *
 * @todo FIXME document this
 *
 * @ingroup nanox_timer
 */
void
GrDestroyTimer (GR_TIMER_ID tid)
{
	nxDestroyTimerReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(DestroyTimer);
	req->timerid = tid;
	UNLOCK(&nxGlobalLock);
}
#endif /* MW_FEATURE_TIMERS */

/**
 * Set server portrait mode.
 *
 * @param portraitmode New portrait mode.
 *
 * @ingroup nanox_misc
 */
void
GrSetPortraitMode(int portraitmode)
{
	nxSetPortraitModeReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetPortraitMode);
	req->portraitmode = portraitmode;
	UNLOCK(&nxGlobalLock);
}

/**
 * Returns the current information for the pointer
 *
 * @param mwin    Window the mouse is current in
 * @param x       Current X pos of mouse (from root)
 * @param y       Current Y pos of mouse (from root)
 * @param bmask   Current button mask 
 *
 * @ingroup nanox_misc
 */
void 
GrQueryPointer(GR_WINDOW_ID *mwin, GR_COORD *x, GR_COORD *y, GR_BUTTON *bmask)
{
	nxQueryPointerReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(QueryPointer);
	TypedReadBlock(mwin, sizeof(*mwin), GrNumQueryPointer);
	ReadBlock(x, sizeof(*x));
	ReadBlock(y, sizeof(*y));
	ReadBlock(bmask, sizeof(*bmask));
	UNLOCK(&nxGlobalLock);
}

/**
 * Returns the current length of the client side queue.
 *
 * @return The current length of the client side queue.
 *
 * @ingroup nanox_event
 */
int 
GrQueueLength(void)
{
	int count = 0;
	EVENT_LIST *p;

	ACCESS_PER_THREAD_DATA();
	LOCK(&nxGlobalLock);
	for(p = evlist; p; p = p->next)
		++count;

	UNLOCK(&nxGlobalLock);
	return count;
}

/**
 * Creates a new region structure, fills it with the region described by the
 * specified polygon, and returns the ID used to refer to it. 1 bits in the
 * bitmap specify areas inside the region and 0 bits specify areas outside it.
 *
 * @param bitmap  pointer to a GR_BITMAP array specifying the region mask
 * @param width  the width of the bitmap
 * @param height  the height of the bitmap
 * @return the ID of the newly allocated region structure, or 0 on error
 *
 * @ingroup nanox_region
 */
GR_REGION_ID
GrNewBitmapRegion(GR_BITMAP *bitmap, GR_SIZE width, GR_SIZE height)
{
	int			size;
	GR_REGION_ID		region;
	nxNewBitmapRegionReq *	req;

	size = sizeof(GR_BITMAP) * height * (((width - 1) / 16) + 1); /* FIXME */

	LOCK(&nxGlobalLock);
	req = AllocReqExtra(NewBitmapRegion, size);
	req->width = width;
	req->height = height;
	memcpy(GetReqData(req), bitmap, size);

	if(TypedReadBlock(&region, sizeof(region), GrNumNewBitmapRegion) < 0)
		region = 0;
	UNLOCK(&nxGlobalLock);
	return region;
}

/**
 * Sets the bounding region of the specified window, not
 * to be confused with a GC clip region.  The bounding region
 * is used to implement non-rectangular windows.
 * A window is defined by two regions: the bounding region
 * and the clip region.  The bounding region defines the area
 * within the parent window that the window will occupy, including
 * border.  The clip region is the subset of the bounding region
 * that is available for subwindows and graphics.  The area between
 * the bounding region and the clip region is defined to be the
 * border of the window.
 * Currently, only the window bounding region is implemented.
 *
 * Copies the specified region and makes the copy be the bounding region used
 * for the specified window.  After setting the clipping region, all
 * drawing within the window will be clipped to the specified region (including
 * the drawing of the window background by the server), and mouse events will
 * pass through parts of the window which are outside the clipping region to
 * whatever is underneath them. Also, windows underneath the areas which are
 * outside the clipping region will be able to draw to the screen as if those
 * areas of the window were not there (in other words, you can see through the
 * gaps in the window). This is most commonly used to implement shaped windows
 * (ie. windows which are some shape other than a simple rectangle). Note that
 * if you are using this feature you will probably want to disable window
 * manager decorations so that the window manager does not draw its own
 * container window behind yours and spoil the desired effect. Also note that
 * shaped windows must always have a border size of 0. If you need a border
 * around a shaped window, add it to the clipping region and draw it yourself.
 *
 * @param wid  the ID of the window to set the clipping region of
 * @param rid  the ID of the region to assign to the specified window
 * @param type  region type, bounding or clip mask
 *
 * @ingroup nanox_window
 */
void
GrSetWindowRegion(GR_WINDOW_ID wid, GR_REGION_ID rid, int type)
{
	nxSetWindowRegionReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(SetWindowRegion);
	req->wid = wid;
	req->rid = rid;
	req->type = type;
	UNLOCK(&nxGlobalLock);
}

/**
 * Copies a region from one drawable to another.  Can stretch and/or flip
 * the image.  The stretch/flip maps (sx1,sy1) in the source to (dx1,dy1)
 * in the destination, and similarly (sx2,sy2) in the source maps to
 * (dx2,dy2) in the destination.  Both horizontal and vertical flips are
 * supported.
 *
 * Note that the bottom and right rows of pixels are excluded - i.e. a
 * target of (0,0)-(2,2) will not draw pixels with y=2 or x=2.
 *
 * 0 is a sensible default ROP code in most cases.
 *
 * @param dstid the ID of the drawable to copy the area to
 * @param gc    the ID of the graphics context to use when copying the area
 * @param dx1   the X coordinate of the first point describing the destination area
 * @param dy1   the Y coordinate of the first point describing the destination area
 * @param dx2   the X coordinate of the second point describing the destination area
 * @param dy2   the Y coordinate of the second point describing the destination area
 * @param srcid the ID of the drawable to copy the area from
 * @param sx1   the X coordinate of the first point describing the source area
 * @param sy1   the Y coordinate of the first point describing the source area
 * @param sx2   the X coordinate of the second point describing the source area
 * @param sy2   the Y coordinate of the second point describing the source area
 * @param op    the ROP codes to pass to the blitter when performing the copy
 *
 * @ingroup nanox_draw
 */
void
GrStretchArea(GR_DRAW_ID dstid, GR_GC_ID gc,
	      GR_COORD dx1, GR_COORD dy1,
	      GR_COORD dx2, GR_COORD dy2,
	      GR_DRAW_ID srcid,
	      GR_COORD sx1, GR_COORD sy1,
	      GR_COORD sx2, GR_COORD sy2,
	      unsigned long op)
{
	nxStretchAreaReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(StretchArea);
	req->drawid = dstid;
	req->gcid = gc;
	req->dx1 = dx1;
	req->dy1 = dy1;
	req->dx2 = dx2;
	req->dy2 = dy2;
	req->srcid = srcid;
	req->sx1 = sx1;
	req->sy1 = sy1;
	req->sx2 = sx2;
	req->sy2 = sy2;
	req->op = op;
	UNLOCK(&nxGlobalLock);
}

/**
 * Grab a key for a specific window.
 *
 * This function has two effects.  With any type other than
 * #GR_GRAB_HOTKEY it attempts to reserve the specified key for
 * exclusive use by the application.  In addition, with #GR_GRAB_HOTKEY
 * or #GR_GRAB_HOTKEY_EXCLUSIVE it requests hotkey events be sent to the
 * specified window whenever the specified key is pressed or released.
 *
 * A key can have any number of reservations of type #GR_GRAB_HOTKEY,
 * but at most one reservation of another type.  This means that 
 * grabs of type #GR_GRAB_HOTKEY always succeed, but grabs of
 * any other type will fail if the key is already grabbed in any
 * fashion except #GR_GRAB_HOTKEY.
 *
 * Note that all grabs are automatically released when the window
 * specified in the id paramater is deleted, or when the client
 * application closes it's connection to the Nano-X server.
 *
 * @param id   Window to send event to.
 * @param key  MWKEY value.
 * @param type The type of reservation to make.  Valid values are
 *             #GR_GRAB_HOTKEY_EXCLUSIVE,
 *             #GR_GRAB_HOTKEY, 
 *             #GR_GRAB_EXCLUSIVE and
 *             #GR_GRAB_EXCLUSIVE_MOUSE.
 * @return     #GR_TRUE on success, #GR_FALSE on error.
 *
 * @ingroup nanox_misc
 */
GR_BOOL
GrGrabKey(GR_WINDOW_ID id, GR_KEY key, int type)
{
	int ret = 0;
	nxGrabKeyReq *req;

	if ((unsigned)type > GR_GRAB_MAX)
		return GR_FALSE;
	
	LOCK(&nxGlobalLock);
	req = AllocReq(GrabKey);
	req->wid = id;
	req->type = type;
	req->key = key;

	/* GrGrabKey returns a value */
	if (TypedReadBlock(&ret, sizeof(ret), GrNumGrabKey) < 0)
		ret = -1;

	UNLOCK(&nxGlobalLock);
	return (GR_BOOL)ret;
}

/**
 * Ungrab a key for a specific window.
 *
 * @param id  Window to stop key grab.
 * @param key MWKEY value.
 *
 * @ingroup nanox_misc
 */
void
GrUngrabKey(GR_WINDOW_ID id, GR_KEY key)
{
	nxGrabKeyReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(GrabKey);
	req->wid = id;
	req->type = GR_GRAB_MAX + 1;
	req->key = key;
	UNLOCK(&nxGlobalLock);
}

/**
 * This passes transform data to the mouse input engine. 
 *
 * @param trans A GR_TRANSFORM structure that contains the transform data
 *              for the filter, or NULL to disable.
 *
 * @ingroup nanox_misc
 */
void
GrSetTransform(GR_TRANSFORM *trans)
{
	nxSetTransformReq *req;

	LOCK(&nxGlobalLock);

	req = AllocReq(SetTransform);
	req->mode = trans? 1: 0;

	if (trans) {
		req->trans_a = trans->a;
		req->trans_b = trans->b;
		req->trans_c = trans->c;
		req->trans_d = trans->d;
		req->trans_e = trans->e;
		req->trans_f = trans->f;
		req->trans_s = trans->s;
	}

	UNLOCK(&nxGlobalLock);
}

#if HAVE_FREETYPE_2_SUPPORT
/**
 * Creates a font from a buffer in memory.  This call copies
 * the buffer, so the caller may free or re-use it.
 *
 * Returns 0 if the font data is corrupt, or if the data format
 * is not recognised.
 *
 * This is useful for:
 * - Web browsers supporting downloading fonts from the web.
 * - Interactive digital TV recievers which support downloading
 *   fonts from the broadcast channel.
 *
 * This call obviously has to allocate "length" bytes of
 * memory, so excessive use should be avoided on low-memory
 * devices.
 *
 * Note that if you want several fonts created from the same
 * buffer (e.g. different sizes or styles), the best way to
 * do it is to create one using this call, and then use
 * GrCopyFont() to create copies which are (likely to be)
 * backed by the same buffer.  In this case, the internal
 * buffer will be reference-counted, and freed when all
 * the font objects that use it are destroyed.
 *
 * @param buffer  The buffer containing the font data.  May not
 *                be NULL.
 * @param length  The buffer length, in bytes.
 * @param format  A string describing the format of the data in
 *                the buffer.  NULL or "" request the font driver
 *                to autodetect.  Otherwise, a file extension such
 *                as "TTF" or "PFR".
 * @param height  The desired height of the font, in pixels.
 *
 * @ingroup nanox_font
 */
GR_FONT_ID
GrCreateFontFromBuffer(const void *buffer, unsigned length,
		       const char *format, GR_COORD height)
{
	GR_FONT_ID result;
	nxCreateFontFromBufferReq *req;
	int bufid;

	LOCK(&nxGlobalLock);

	/* Step 1 - Send the buffer to the other side */
	bufid = sendImageBuffer(buffer, length);

	if (!bufid) {
		UNLOCK(&nxGlobalLock);
		return 0;
	}

	req = AllocReq(CreateFontFromBuffer);
	req->height = height;
	req->buffer_id = bufid;

	if (format == NULL)
		format = "";

	strncpy(req->format, format, sizeof(req->format) - 1);
	req->format[sizeof(req->format) - 1] = '\0';

	if (TypedReadBlock
	    (&result, sizeof(result), GrNumCreateFontFromBuffer) == -1)
		result = 0;

	UNLOCK(&nxGlobalLock);
	return result;
}

/**
 * Creates a copy of a font, optionally using a different size.
 *
 * For the rationale behind this function, see GrCreateFontFromBuffer().
 *
 * @param fointid  The font to copy.
 * @param height   The size of the new copy, or 0 to make the copy the
 *                 same size as the original font.
 *
 * @ingroup nanox_font
 */
GR_FONT_ID
GrCopyFont(GR_FONT_ID fontid, GR_COORD height)
{
	GR_FONT_ID result;
	nxCopyFontReq *req;

	LOCK(&nxGlobalLock);
	req = AllocReq(CopyFont);
	req->fontid = fontid;
	req->height = height;

	if (TypedReadBlock(&result, sizeof(result), GrNumCopyFont) == -1)
		result = 0;

	UNLOCK(&nxGlobalLock);
	return result;
}
#endif /*HAVE_FREETYPE_2_SUPPORT*/



