/*
 * Copyright (c) 2000, 2003, 2010, 2019 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 *
 * Nano-X server routines for LINK_APP_INTO_SERVER=Y case (NONETWORK=1)
 */
#include <stdlib.h>
#include "serv.h"

#if EMSCRIPTEN
#include <emscripten.h>
#endif

#if NONETWORK /* entire file is NONETWORK*/
void
GrFlush(void)
{
	/* flush systems with delayed screen output by calling driver Preselect()*/
	if (scrdev.flags & PSF_DELAYUPDATE)
	{
		/* perform single update of aggregate screen update region*/
		if(scrdev.PreSelect)
			scrdev.PreSelect(&scrdev);
	}
}

void
GrMainLoop(GR_FNCALLBACKEVENT fncb)
{
	GR_EVENT event;

	for(;;) {
		GrGetNextEvent(&event);
		fncb(&event);
	}
}

void
GrReqShmCmds(long shmsize)
{
	/* no action required, no client/server*/
}

/*
 * Return the next waiting event for a client, or wait for one if there
 * is none yet.  The event is copied into the specified structure, and
 * then is moved from the event queue to the free event queue.  If there
 * is an error event waiting, it is delivered before any other events.
 */
void
GrGetNextEvent(GR_EVENT *ep)
{
	GrGetNextEventTimeout(ep, GR_TIMEOUT_BLOCK);
}

/*
 * Return the next event from the event queue, or
 * wait for a new one if one is not ready.  If timeout
 * is nonzero, return timeout event if time elapsed.
 */
void
GrGetNextEventTimeout(GR_EVENT *ep, GR_TIMEOUT timeout)
{
	SERVER_LOCK();

	/*
	 * If no event ready, wait for one.
	 * Note: won't work for multiple clients, ok since only static
	 * linked apps call this function.
	 */
	while(curclient->eventhead == NULL)
	{
		GsSelect(timeout);
	}

	GsCheckNextEvent(ep, GR_FALSE);
	SERVER_UNLOCK();
}

/*
 * Wait until an event is available for a client, and then peek at it.
 */
void
GrPeekWaitEvent(GR_EVENT *ep)
{
	SERVER_LOCK();
	while(curclient->eventhead == NULL)
	{
		GsSelect(GR_TIMEOUT_BLOCK);
	}
	GrPeekEvent(ep);
	SERVER_UNLOCK();
}

/*
 * Return the current length of the input queue.
 */
int 
GrQueueLength(void)
{
	int count = 0;
	GR_EVENT_LIST *elp;

	SERVER_LOCK();
	for (elp=curclient->eventhead; elp; elp=elp->next)
		++count;
	SERVER_UNLOCK();
	return count;
}

/* builtin callback function for GrGetTypedEvent*/
static GR_BOOL
GetTypedEventCallback(GR_WINDOW_ID wid, GR_EVENT_MASK mask, GR_UPDATE_TYPE update,GR_EVENT *ep,void *arg)
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

/*
 * Fills in the specified event structure with a copy of the next event on the
 * queue that matches the type parameters passed and removes it from the queue.
 * If block is GR_TRUE, the call will block until a matching event is found.
 * Otherwise, only the local queue is searched, and an event type of
 * GR_EVENT_TYPE_NONE is returned if the a match is not found.
 */
int
GrGetTypedEvent(GR_WINDOW_ID wid, GR_EVENT_MASK mask, GR_UPDATE_TYPE update, GR_EVENT *ep, GR_BOOL block)
{
	return GrGetTypedEventPred(wid, mask, update, ep, block, GetTypedEventCallback, NULL);
}

/*
 * The specified callback function is called with the passed event type parameters
 * for each event on the queue, until the callback function CheckFunction
 * returns GR_TRUE.  The event is then removed from the queue and returned.
 * If block is GR_TRUE, the call will block until a matching event is found.
 * Otherwise, only the local queue is searched, and an event type of
 * GR_EVENT_TYPE_NONE is returned if a match is not found.
 */
int
GrGetTypedEventPred(GR_WINDOW_ID wid, GR_EVENT_MASK mask, GR_UPDATE_TYPE update, GR_EVENT *ep,
	GR_BOOL block, GR_TYPED_EVENT_CALLBACK matchfn, void *arg)
{
	GR_EVENT_LIST *elp, *prevelp;

	/* process server events, required for williams.c XMaskEvent style app in LINK_APP_INTO_SERVER case*/
	GsSelect(GR_TIMEOUT_POLL); 		/* poll for event*/
#if EMSCRIPTEN
	/* required for williams.c style XCheckMaskEvent polling apps*/
	emscripten_sleep(1); /* allow EMSCRIPTEN/SDL javascript to run after SDL screen flush*/
#endif

	SERVER_LOCK();
	/* determine if we need to wait for any events*/
	while(curclient->eventhead == NULL)
	{
getevent:
		GsSelect(block? GR_TIMEOUT_BLOCK: GR_TIMEOUT_POLL); /* wait/poll for event*/

#if NANOWM
		if (curclient->eventhead)
		{
			GR_EVENT_LIST *	elp = curclient->eventhead;
			/* let inline window manager look at event, required for williams.c XMaskEvent style app*/
			wm_handle_event(&elp->event);		/* don't change event type for Mask* functions*/
		}
#endif

		if (!block)
			break;
	}

	/* Now, run through the event queue, looking for matches of the type
	 * info that was passed.
	 */
	prevelp = NULL;
	elp = curclient->eventhead;
	while (elp) {
		if (matchfn(wid, mask, update, &elp->event, arg)) {
			/* remove event from queue, return it*/
			if (prevelp == NULL)
				curclient->eventhead = elp->next;
			else prevelp->next = elp->next;
			if (curclient->eventtail == elp)
				curclient->eventtail = prevelp;
			elp->next = eventfree;
			eventfree = elp;

			*ep = elp->event;
			SERVER_UNLOCK();
#if NANOWM
			/* let inline window manager look at event*/
			wm_handle_event(ep);	/* don't change even type*/
#endif
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
	SERVER_UNLOCK();
	return GR_EVENT_TYPE_NONE;
} 

/* [copied from client.c]
 * The following is the user defined function for handling errors.
 * If this is not set, then the default action is to close the connection
 * to the server, describe the error, and then exit.  This error function
 * will only be called when the client asks for events.
 */
static GR_FNCALLBACKEVENT ErrorFunc = GrDefaultErrorHandler;

/* 
 * The default error handler which is called when the server
 * reports an error event and the client hasn't set a handler for error events.
 */
void 
GrDefaultErrorHandler(GR_EVENT *ep)
{
	if (ep->type == GR_EVENT_TYPE_ERROR) {
		EPRINTF("nxclient: Error (%s) ", ep->error.name);
		EPRINTF(nxErrorStrings[ep->error.code], ep->error.id);
		GrClose();
		exit(1);
	}
}

/*
 * Set an error handling routine, which will be called on any errors from
 * the server (when events are asked for by the client).  If zero is given,
 * then errors will be returned as regular events.  
 * Returns the previous error handler.
 */
GR_FNCALLBACKEVENT
GrSetErrorHandler(GR_FNCALLBACKEVENT fncb)
{
	GR_FNCALLBACKEVENT orig;

	SERVER_LOCK();

	orig = ErrorFunc;
	ErrorFunc = fncb;

	SERVER_UNLOCK();

	return orig;
}
#endif /* NONETWORK*/
