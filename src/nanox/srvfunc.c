/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "serv.h"

static int	nextid = GR_ROOT_WINDOW_ID + 1;

static void CheckNextEvent(GR_EVENT *ep, GR_BOOL doCheckEvent);
 
/*
 * Return information about the screen for clients to use.
 */
void 
GrGetScreenInfo(GR_SCREEN_INFO *sip)
{
	SERVER_LOCK();

	GdGetScreenInfo(rootwp->psd, sip);

	/* set virtual screen sizing (for PDA emulation on desktop)*/
	sip->vs_width = nxres? nxres: sip->cols;
	sip->vs_height = nyres? nyres: sip->rows;

	/* set workspace equal to virtual screen area minus 22 pixel taskbar*/
	sip->ws_width = sip->vs_width;
	sip->ws_height = sip->vs_height - 22;

	SERVER_UNLOCK();
}

/*
 * Return the size of a text string for the font in a graphics context.
 * This is the width of the string, the height of the string,
 * and the height above the bottom of the font of the baseline for the font.
 */
void 
GrGetGCTextSize(GR_GC_ID gc, void *str, int count, GR_TEXTFLAGS flags,
	GR_SIZE *retwidth, GR_SIZE *retheight, GR_SIZE *retbase)
{
	GR_GC		*gcp;
	GR_FONT		*fontp;
	PMWFONT		pf;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp == NULL)
		fontp = NULL;
	else
		fontp = GsFindFont(gcp->fontid);
	pf = fontp? fontp->pfont: stdfont;
	GdGetTextSize(pf, str, count, retwidth, retheight, retbase, flags);

	SERVER_UNLOCK();
}

#if NONETWORK
/*
 * Return the next waiting event for a client, or wait for one if there
 * is none yet.  The event is copied into the specified structure, and
 * then is moved from the event queue to the free event queue.  If there
 * is an error event waiting, it is delivered before any other events.
 */
void
GrGetNextEvent(GR_EVENT *ep)
{
	GrGetNextEventTimeout(ep, 0L);
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
	/* If no event ready, wait for one*/
	/* Note: won't work for multiple clients*/
	/* This is OK, since only static linked apps call this function*/
	while(curclient->eventhead == NULL)
		GsSelect(timeout);
	CheckNextEvent(ep, GR_FALSE);
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
		GsSelect(0L);
	GrPeekEvent(ep);
	SERVER_UNLOCK();
}
#endif

/*
 * Return the next event from the event queue if one is ready.
 * If one is not ready, then the type GR_EVENT_TYPE_NONE is returned.
 * If it is an error event, then a user-specified routine is called
 * if it was defined, otherwise we clean up and exit.
 */
void
GrCheckNextEvent(GR_EVENT *ep)
{
	SERVER_LOCK();
	CheckNextEvent(ep, GR_TRUE);
	SERVER_UNLOCK();
}

static void
CheckNextEvent(GR_EVENT *ep, GR_BOOL doCheckEvent)
{
	GR_EVENT_LIST *	elp;

#if 0 /* was NONETWORK*/
	/* NOTE: select() now called through GrPeekEvent when bound to server*/
	if (doCheckEvent)
		GsSelect(1L);
#endif
	/* Copy first event if any*/
	if(!GrPeekEvent(ep))
		return;

	/* Get first event again*/
	elp = curclient->eventhead;

#if NONETWORK
	/* if GrCheckEvent, turn timeouts into no event*/
	if (doCheckEvent && elp->event.type == GR_EVENT_TYPE_TIMEOUT)
		ep->type = GR_EVENT_TYPE_NONE;
#endif

	/* Remove first event from queue*/
	curclient->eventhead = elp->next;
	if (curclient->eventtail == elp)
		curclient->eventtail = NULL;

	elp->next = eventfree;
	eventfree = elp;
}

/*
 * Peek at the event queue for the current client to see if there are any
 * outstanding events.  Returns the event at the head of the queue, or
 * else a null event type.  The event is still left in the queue, however.
 */
int
GrPeekEvent(GR_EVENT *ep)
{
	GR_EVENT_LIST *	elp;

	SERVER_LOCK();
	elp = curclient->eventhead;
#if NONETWORK
	/* if no events on queue, force select() event check*/
	if (elp == NULL) {
		GsSelect(-1L);	/* poll*/
		elp = curclient->eventhead;
	}
#endif
	if(elp == NULL) {
		ep->type = GR_EVENT_TYPE_NONE;
		SERVER_UNLOCK();
		return 0;
	}

	/* copy event out*/
	*ep = elp->event;

	SERVER_UNLOCK();
	return 1;
}

/*
 * Return information about a window id.
 */
void
GrGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_PIXMAP	*pp;
	GR_EVENT_CLIENT	*evp;		/* event-client structure */

	SERVER_LOCK();

	/* first check window list*/
	wp = GsFindWindow(wid);
	if (wp) {
		infoptr->wid = wid;
		/* report parent-relative x,y coordinates*/
		infoptr->x = wp->x - (wp->parent ? wp->parent->x : 0);
		infoptr->y = wp->y - (wp->parent ? wp->parent->y : 0);
		infoptr->width = wp->width;
		infoptr->height = wp->height;
		infoptr->parent = wp->parent? wp->parent->id: 0;
		infoptr->child = wp->children? wp->children->id: 0;
		infoptr->sibling = wp->siblings? wp->siblings->id: 0;
		infoptr->mapped = wp->mapped;
		infoptr->realized = wp->realized;
		infoptr->inputonly = !wp->output;
		infoptr->bordersize = wp->bordersize;
		infoptr->bordercolor = wp->bordercolor;
		infoptr->background = wp->background;
		infoptr->props = wp->props;
		infoptr->cursor = wp->cursorid;
		infoptr->processid = wp->owner? wp->owner->processid: 0;
		infoptr->eventmask = 0;

		for (evp = wp->eventclients; evp; evp = evp->next) {
			if (evp->client == curclient)
				infoptr->eventmask = evp->eventmask;
		}
		SERVER_UNLOCK();
		return;
	}

	/* then pixmap list*/
	pp = GsFindPixmap(wid);
	if (pp) {
		infoptr->wid = wid;
		infoptr->x = pp->x;
		infoptr->y = pp->y;
		infoptr->width = pp->width;
		infoptr->height = pp->height;
		infoptr->parent = 0;
		infoptr->child = 0;
		infoptr->sibling = 0;
		infoptr->mapped = GR_FALSE;
		infoptr->realized = GR_TRUE;
		infoptr->inputonly = GR_FALSE;
		infoptr->bordersize = 0;
		infoptr->bordercolor = 0;
		infoptr->background = 0;
		infoptr->eventmask = 0;
		infoptr->cursor = 0;
		infoptr->processid = pp->owner? pp->owner->processid: 0;
		SERVER_UNLOCK();
		return;
	}

	/* No error if window id is invalid.*/
	memset(infoptr, 0, sizeof(GR_WINDOW_INFO));
	SERVER_UNLOCK();
}

/*
 * Destroy an existing window and all of its children.
 * Also used to destroy a pixmap.
 */
void
GrDestroyWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_PIXMAP	*pp;
	GR_PIXMAP	*prevpp;
	PSD		psd;

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (wp) {
		GsWpDestroyWindow(wp);
	} else {
		pp = GsFindPixmap(wid);
		if (pp) {
			psd = pp->psd;
			/* deallocate pixmap memory*/
			if (psd->flags & PSF_ADDRMALLOC)
				free(psd->addr);

			/* deallocate mem gc*/
			psd->FreeMemGC(psd);

			/*
			 * Remove this pixmap from the complete list of pixmaps.
			 */
			prevpp = listpp;
			if (prevpp == pp)
				listpp = pp->next;
			else {
				while (prevpp->next != pp)
					prevpp = prevpp->next;
				prevpp->next = pp->next;
			}

			/*
			 * Forget various information if they related to this
			 * pixmap.  Then finally free the structure.
			 */
			if (pp == cachepp) {
				cachepixmapid = 0;
				cachepp = NULL;
			}
			free(pp);
		}
	}

	SERVER_UNLOCK();
}


/*
 * Raise a window to the highest level among its siblings.
 */
void
GrRaiseWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_WINDOW	*prevwp;	/* previous window pointer */
	GR_BOOL		overlap;	/* TRUE if there was overlap */

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if ((wp == NULL) || (wp == rootwp)) {
		SERVER_UNLOCK();
		return;
	}

	/*
	 * If this is already the highest window then we are done.
	 */
	prevwp = wp->parent->children;
	if (prevwp == wp) {
		SERVER_UNLOCK();
		return;
	}

	/*
	 * Find the sibling just before this window so we can unlink it.
	 * Also, determine if any sibling ahead of us overlaps the window.
	 * Remember that for exposure events.
	 */
	overlap = GR_FALSE;
	while (prevwp->siblings != wp) {
		overlap |= GsCheckOverlap(prevwp, wp);
		prevwp = prevwp->siblings;
	}
	overlap |= GsCheckOverlap(prevwp, wp);

	/*
	 * Now unlink the window and relink it in at the front of the
	 * sibling chain.
	 */
	prevwp->siblings = wp->siblings;
	wp->siblings = wp->parent->children;
	wp->parent->children = wp;

	/*
	 * Finally redraw the window if necessary.
	 */
	if (overlap) {
		GsDrawBorder(wp);
		GsExposeArea(wp, wp->x, wp->y, wp->width, wp->height, NULL);
	}

	SERVER_UNLOCK();
}

/*
 * Lower a window to the lowest level among its siblings.
 */
void
GrLowerWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_WINDOW	*prevwp;	/* previous window pointer */
	GR_WINDOW	*sibwp;		/* sibling window */
	GR_WINDOW	*expwp;		/* siblings being exposed */

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if ((wp == NULL) || (wp == rootwp)) {
		SERVER_UNLOCK();
		return;
	}
	if (wp->siblings == NULL) {
		SERVER_UNLOCK();
		return;
	}

	/*
	 * Find the sibling just before this window so we can unlink us.
	 */
	prevwp = wp->parent->children;
	if (prevwp != wp) {
		while (prevwp->siblings != wp)
			prevwp = prevwp->siblings;
	}

	/*
	 * Remember the first sibling that is after us, so we can
	 * generate exposure events for the remaining siblings.  Then
	 * walk down the sibling chain looking for the last sibling.
	 */
	expwp = wp->siblings;
	sibwp = wp;
	while (sibwp->siblings)
		sibwp = sibwp->siblings;

	/*
	 * Now unlink the window and relink it in at the end of the
	 * sibling chain.
	 */
	if (prevwp == wp)
		wp->parent->children = wp->siblings;
	else
		prevwp->siblings = wp->siblings;
	sibwp->siblings = wp;

	wp->siblings = NULL;

	/*
	 * Finally redraw the sibling windows which this window covered
	 * if they overlapped our window.
	 */
	while (expwp && (expwp != wp)) {
		if (GsCheckOverlap(wp, expwp)) {
			GsExposeArea(expwp, wp->x - wp->bordersize,
				wp->y - wp->bordersize,
				wp->width + wp->bordersize * 2,
				wp->height + wp->bordersize * 2, NULL);
		}
		expwp = expwp->siblings;
	}

	SERVER_UNLOCK();
}

/* Offset a window position and all children by offx,offy*/
static void
OffsetWindow(GR_WINDOW *wp, GR_COORD offx, GR_COORD offy)
{
	GR_WINDOW	*cp;

	wp->x += offx;
	wp->y += offy;
	for(cp=wp->children; cp; cp=cp->siblings)
		OffsetWindow(cp, offx, offy);
}

/* deliver an update move event to window and all children*/
static void
DeliverUpdateMoveEventAndChildren(GR_WINDOW *wp)
{
	GR_WINDOW *	childwp;

	GsDeliverUpdateEvent(wp, GR_UPDATE_MOVE, wp->x, wp->y,
		wp->width, wp->height);

	for (childwp = wp->children; childwp; childwp = childwp->siblings)
		DeliverUpdateMoveEventAndChildren(childwp);
}

/*
 * Move the window to the specified position relative to its parent.
 */
void
GrMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_COORD	offx, offy;

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (wp == NULL) {
		SERVER_UNLOCK();
		return;
	}
	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wid);
		SERVER_UNLOCK();
		return;
	}

	x += wp->parent->x;
	y += wp->parent->y;
	offx = x - wp->x;
	offy = y - wp->y;

	if (wp->x == x && wp->y == y) {
		SERVER_UNLOCK();
		return;
	}

	/* * move algorithms not requiring unmap/map ***/
#if 1
	/* perform screen blit if topmost and mapped - no flicker!*/
	if (wp->mapped && wp == wp->parent->children
		&& wp->parent->id == GR_ROOT_WINDOW_ID

		/* temp don't blit in portrait mode, still buggy*/
		/* *&& !(wp->psd->portrait & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT))***/

		/* don't blit if window has custom frame, background not right*/
		&& !wp->clipregion
	   ) {
		int 		oldx = wp->x;
		int 		oldy = wp->y;
		GR_WINDOW_ID 	pixid = GrNewPixmap(wp->width, wp->height,NULL);
		GR_GC_ID	gc = GrNewGC();
		GR_WINDOW * 	stopwp = wp;
		int		X, Y, W, H;

		/* must hide cursor first or GdFixCursor() will show it*/
		GdHideCursor(rootwp->psd);

		/* turn off clipping of root's children*/
		GrSetGCMode(gc, GR_MODE_COPY|GR_MODE_EXCLUDECHILDREN);

		/* copy topmost window contents offscreen*/
		GrCopyArea(pixid, gc, 0, 0, wp->width, wp->height,
			GR_ROOT_WINDOW_ID, oldx, oldy, MWROP_COPY);

		/* calc new window offsets*/
		OffsetWindow(wp, offx, offy);

		/* force recalc of clip region*/
		clipwp = NULL;

		/* copy window bits to new location*/
		GrCopyArea(GR_ROOT_WINDOW_ID, gc, wp->x, wp->y, wp->width,
			wp->height, pixid, 0, 0, MWROP_COPY);

		/*
		 * If any portion of the window was offscreen
		 * and is coming onscreen, must send expose events
		 * to this window as well.
		 */
		if ((oldx < 0 && wp->x > oldx) ||
		    (oldy < 0 && wp->y > oldy) ||
		    (oldx+wp->width > rootwp->width && wp->x < oldx) ||
		    (oldy+wp->height > rootwp->height && wp->y < oldy))
			stopwp = NULL;

		/* 
		 * Calculate bounded exposed area and
		 * redraw anything lower than stopwp window.
		 */
		X = MWMIN(oldx, wp->x);
		Y = MWMIN(oldy, wp->y);
		W = MWMAX(oldx, wp->x) + wp->width - X;
		H = MWMAX(oldy, wp->y) + wp->height - Y;
		GsExposeArea(rootwp, X, Y, W, H, stopwp);

		GdShowCursor(rootwp->psd);
		GrDestroyGC(gc);
		GrDestroyWindow(pixid);
		DeliverUpdateMoveEventAndChildren(wp);
		SERVER_UNLOCK();
		return;
	}
#endif
#if 0
	/* perform quick move and expose if topmost and mapped - no blit*/
	if (wp->mapped && wp == wp->parent->children) {
		int	oldx = wp->x;
		int	oldy = wp->y;
		int	X, Y, W, H;

		OffsetWindow(wp, offx, offy);

		/* force recalc of clip region*/
		clipwp = NULL;

		X = MWMIN(oldx, wp->x);
		Y = MWMIN(oldy, wp->y);
		W = MWMAX(oldx, wp->x) + wp->width - X;
		H = MWMAX(oldy, wp->y) + wp->height - Y;
		GsExposeArea(rootwp, X, Y, W, H, NULL);
		DeliverUpdateMoveEventAndChildren(wp);
		SERVER_UNLOCK();
		return;
	}
#endif
	/*
	 * This method will redraw the window entirely,
	 * resulting in considerable flicker.
	 */
	GsWpUnrealizeWindow(wp, GR_TRUE);
	OffsetWindow(wp, offx, offy);
	GsWpRealizeWindow(wp, GR_FALSE);
	DeliverUpdateMoveEventAndChildren(wp);

	SERVER_UNLOCK();
	return;
}

/*
 * Resize the window to be the specified size.
 */
void
GrResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height)
{
	GR_WINDOW	*wp;		/* window structure */

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (wp == NULL) {
		SERVER_UNLOCK();
		return;
	}
	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wid);
		SERVER_UNLOCK();
		return;
	}
	if ((width <= 0) || (height <= 0)) {
		GsError(GR_ERROR_BAD_WINDOW_SIZE, wid);
		SERVER_UNLOCK();
		return;
	}

	if ((wp->width == width) && (wp->height == height)) {
		SERVER_UNLOCK();
		return;
	}

	if (!wp->realized || !wp->output) {
		wp->width = width;
		wp->height = height;
		SERVER_UNLOCK();
		return;
	}

	/*
	 * This should be optimized to not require redrawing of the window
	 * when possible.
	 */
	GsWpUnrealizeWindow(wp, GR_TRUE);
	wp->width = width;
	wp->height = height;
	/* send size update before expose event*/
	GsDeliverUpdateEvent(wp, GR_UPDATE_SIZE, wp->x, wp->y, width, height);
	GsWpRealizeWindow(wp, GR_FALSE);

	SERVER_UNLOCK();
}

/*
 * Reparent window to new parent, position at passed x, y
 */
void
GrReparentWindow(GR_WINDOW_ID wid, GR_WINDOW_ID pwid, GR_COORD x, GR_COORD y)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_WINDOW	*pwp;		/* parent window structure */
	GR_WINDOW	**mysibptr;	/* handle to my sibling ptr */
	GR_COORD	offx, offy;

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	pwp = GsFindWindow(pwid);
	if (!wp || !pwp || wp == pwp) {
		SERVER_UNLOCK();
		return;
	}
	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wid);
		SERVER_UNLOCK();
		return;
	}

/*printf("grreparent: pid %d wid %d (oldpid %d) realized %d,%d\n", pwid, wid, wp->parent->id, pwp->realized, wp->realized);*/
	x += pwp->x;
	y += pwp->y;
	offx = x - wp->x;
	offy = y - wp->y;

	/* 
	 * Unrealize window and all children.  No effect if
	 * this window isn't already realized.
	 */
	GsWpUnrealizeWindow(wp, GR_TRUE);

	/* link window into new parent chain*/
	for(mysibptr = &(wp->parent->children); *mysibptr != wp; 
		mysibptr = &((*mysibptr)->siblings))
			continue;

	*mysibptr = wp->siblings;
	wp->parent = pwp;
	wp->siblings = pwp->children;
	pwp->children = wp;

	if (offx || offy)
		OffsetWindow(wp, offx, offy);

	/*
	 * Realize window again. Window will become visible if
	 * the parent window is realized and this window is mapped.
	 *
	 * Temp flag is set TRUE, no additional
	 * UPDATE_MAP will be sent.
	 */
	GsWpRealizeWindow(wp, GR_TRUE);

	/* send reparent update event*/
	GsDeliverUpdateEvent(wp, GR_UPDATE_REPARENT, wp->x, wp->y, wp->width, wp->height);

	SERVER_UNLOCK();
}

static int nextgcid = 1000;
/*
 * Allocate a new GC with default parameters.
 * The GC is owned by the current client.
 */
GR_GC_ID 
GrNewGC(void)
{
	GR_GC	*gcp;

	SERVER_LOCK();

	gcp = (GR_GC *) malloc(sizeof(GR_GC));
	if (gcp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}

	gcp->id = nextgcid++;
	gcp->mode = GR_MODE_COPY;
	gcp->regionid = 0;	/* no region*/
	gcp->xoff = 0;		/* no offset*/
	gcp->yoff = 0;
	gcp->fontid = 0;	/* 0 is default font*/
	gcp->foreground = WHITE;
	gcp->background = BLACK;
	gcp->fgispixelval = GR_FALSE;
	gcp->bgispixelval = GR_FALSE;
	gcp->usebackground = GR_TRUE;

	gcp->exposure = GR_TRUE;

	gcp->linestyle = GR_LINE_SOLID;
	gcp->fillmode = GR_FILL_SOLID;

	gcp->dashcount = 0;
	gcp->dashmask = 0;

	gcp->stipple.bitmap = NULL;
	gcp->stipple.width = 0;
	gcp->stipple.height = 0;
	
	gcp->tile.psd = NULL;
	gcp->tile.width = 0;
	gcp->tile.height = 0;
	
	gcp->ts_offset.x = 0;
	gcp->ts_offset.y = 0;

	gcp->changed = GR_TRUE;
	gcp->owner = curclient;
	gcp->next = listgcp;

	listgcp = gcp;

	SERVER_UNLOCK();

	return gcp->id;
}

/*
 * Destroy an existing graphics context.
 */
void
GrDestroyGC(GR_GC_ID gc)
{
	GR_GC		*gcp;		/* graphics context */
	GR_GC		*prevgcp;	/* previous graphics context */

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp == NULL) {
		SERVER_UNLOCK();
		return;
	}

	if (gc == cachegcid) {
		cachegcid = 0;
		cachegcp = NULL;
	}
	if (gcp == curgcp)
		curgcp = NULL;

	if (listgcp == gcp)
		listgcp = gcp->next;
	else {
		prevgcp = listgcp;
		while (prevgcp->next != gcp)
			prevgcp = prevgcp->next;

		prevgcp->next = gcp->next;
	}

	if (gcp->stipple.bitmap)
		free(gcp->stipple.bitmap);
	free(gcp);

	SERVER_UNLOCK();
}

/*
 * Allocate a new GC which is a copy of another one.
 * The GC is owned by the current client.
 */
GR_GC_ID 
GrCopyGC(GR_GC_ID gc)
{
	GR_GC		*oldgcp;	/* old graphics context */
	GR_GC		*gcp;		/* new graphics context */
	GR_GC_ID id;

	SERVER_LOCK();

	oldgcp = GsFindGC(gc);
	if (oldgcp == NULL) {
		SERVER_UNLOCK();
		return 0;
	}

	gcp = (GR_GC *) malloc(sizeof(GR_GC));
	if (gcp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}

	/*
	 * Copy all the old gcp values into the new one, except allocate
	 * a new id for it and link it into the list of GCs.
	 */
	*gcp = *oldgcp;
	gcp->id = nextgcid++;
	id = gcp->id;
	gcp->changed = GR_TRUE;
	gcp->owner = curclient;
	gcp->next = listgcp;
	listgcp = gcp;

	SERVER_UNLOCK();

	return id;
}

/*
 * Return information about the specified graphics context.
 */
void 
GrGetGCInfo(GR_GC_ID gc, GR_GC_INFO *gcip)
{
	GR_GC		*gcp;

	SERVER_LOCK();

	/*
	 * Find the GC manually so that an error is not generated.
	 */
	for (gcp = listgcp; gcp && (gcp->id != gc); gcp = gcp->next)
		continue;

	if (gcp == NULL) {
		memset(gcip, 0, sizeof(GR_GC_INFO));
		SERVER_UNLOCK();
		return;
	}

	gcip->gcid = gc;
	gcip->mode = gcp->mode;
	gcip->region = gcp->regionid;
	gcip->xoff = gcp->xoff;
	gcip->yoff = gcp->yoff;
	gcip->font = gcp->fontid;
	gcip->foreground = gcp->foreground;
	gcip->background = gcp->background;
	gcip->fgispixelval = gcp->fgispixelval;
	gcip->bgispixelval = gcp->bgispixelval;
	gcip->usebackground = gcp->usebackground;
	gcip->exposure = gcp->exposure;

	SERVER_UNLOCK();
}

static int nextregionid = 1000;
/*
 * Allocate a new REGION with default parameters.
 * The REGION is owned by the current client.
 */
GR_REGION_ID
GrNewRegion(void)
{
	GR_REGION *regionp;
	GR_REGION_ID id;

	SERVER_LOCK();
           
	regionp = (GR_REGION *) malloc(sizeof(GR_REGION));
	if (regionp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}
	
	regionp->rgn = GdAllocRegion();
	regionp->id = nextregionid++;
	regionp->owner = curclient;
	regionp->next = listregionp;

	listregionp = regionp;

	id = regionp->id;

	SERVER_UNLOCK();

	return id;
}

/*
 * Allocate a new region from a set of points interpreted as a polygon.
 * The REGION is owned by the current client.
 */
GR_REGION_ID
GrNewPolygonRegion(int mode, GR_COUNT count, GR_POINT *points)
{
#if POLYREGIONS
	GR_REGION *regionp;
	GR_REGION_ID id;

	SERVER_LOCK();
           
	regionp = (GR_REGION *) malloc(sizeof(GR_REGION));
	if (regionp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}
	
	regionp->rgn = GdAllocPolygonRegion(points, count, mode);
	regionp->id = nextregionid++;
	regionp->owner = curclient;
	regionp->next = listregionp;

	listregionp = regionp;

	id = regionp->id;

	SERVER_UNLOCK();

	return id;
#else
	return 0;
#endif
}

/*
 * Destroy an existing region.
 */
void
GrDestroyRegion(GR_REGION_ID region)
{
	GR_REGION	*regionp;	/* region */
	GR_REGION	*prevregionp;	/* previous region */

	SERVER_LOCK();

	regionp = GsFindRegion(region);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return;
	}

	if (listregionp == regionp) {
		listregionp = regionp->next;
	} else {
		prevregionp = listregionp;
		while (prevregionp->next != regionp)
			prevregionp = prevregionp->next;

		prevregionp->next = regionp->next;
	}
	GdDestroyRegion(regionp->rgn);
	free(regionp);

	SERVER_UNLOCK();
}

/*
 * Updates the region from a union of the specified rectangle
 * and the original region.
 */
void
GrUnionRectWithRegion(GR_REGION_ID region, GR_RECT *rect)
{
	GR_REGION	*regionp;
	MWRECT		rc;
	
	SERVER_LOCK();
	
	regionp = GsFindRegion(region);
	if (regionp) {
		/* convert Nano-X rect to MW rect*/
		rc.left = rect->x;
		rc.top = rect->y;
		rc.right = rect->x + rect->width;
		rc.bottom = rect->y + rect->height;
		GdUnionRectWithRegion(&rc, regionp->rgn);	
	}

	SERVER_UNLOCK();
}

/*
 * Updates the region from a union of two regions.
 */ 
void
GrUnionRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	GR_REGION	*regionp;
	GR_REGION	*srcregionp1;
	GR_REGION	*srcregionp2;
	
	SERVER_LOCK();
	
	regionp = GsFindRegion(dst_rgn);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp1 = GsFindRegion(src_rgn1);
	if (srcregionp1 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp2 = GsFindRegion(src_rgn2);
	if (srcregionp2 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	GdUnionRegion(regionp->rgn, srcregionp1->rgn, srcregionp2->rgn);

	SERVER_UNLOCK();
}

/*
 * Updates the region by subtracting a region from another.
 */ 
void
GrSubtractRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	GR_REGION	*regionp;
	GR_REGION	*srcregionp1;
	GR_REGION	*srcregionp2;
	
	SERVER_LOCK();
	
	regionp = GsFindRegion(dst_rgn);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp1 = GsFindRegion(src_rgn1);
	if (srcregionp1 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp2 = GsFindRegion(src_rgn2);
	if (srcregionp2 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	GdSubtractRegion(regionp->rgn, srcregionp1->rgn, srcregionp2->rgn);
	
	SERVER_UNLOCK();
}

/*
 * Updates the region to the difference of two regions.
 */ 
void
GrXorRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	GR_REGION	*regionp;
	GR_REGION	*srcregionp1;
	GR_REGION	*srcregionp2;
	
	SERVER_LOCK();
	
	regionp = GsFindRegion(dst_rgn);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp1 = GsFindRegion(src_rgn1);
	if (srcregionp1 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp2 = GsFindRegion(src_rgn2);
	if (srcregionp2 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	GdXorRegion(regionp->rgn, srcregionp1->rgn, srcregionp2->rgn);
	
	SERVER_UNLOCK();
}

/*
 * Updates the region from a intersection of two regions.
 */ 
void
GrIntersectRegion(GR_REGION_ID dst_rgn, GR_REGION_ID src_rgn1,
	GR_REGION_ID src_rgn2)
{
	GR_REGION	*regionp;
	GR_REGION	*srcregionp1;
	GR_REGION	*srcregionp2;
	
	SERVER_LOCK();
	
	regionp = GsFindRegion(dst_rgn);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp1 = GsFindRegion(src_rgn1);
	if (srcregionp1 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	srcregionp2 = GsFindRegion(src_rgn2);
	if (srcregionp2 == NULL) {
		SERVER_UNLOCK();
		return;
	}
	
	GdIntersectRegion(regionp->rgn, srcregionp1->rgn, srcregionp2->rgn);
	
	SERVER_UNLOCK();
}

/*
 * Sets the clip-mask in the GC to the specified region.
 */
void
GrSetGCRegion(GR_GC_ID gc, GR_REGION_ID region)
{
	GR_GC		*gcp;
	
	SERVER_LOCK();
	
	gcp = GsFindGC(gc);
	if (gcp) {
		gcp->regionid = region;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Set the x,y origin of user clip region in GC.
 */
void
GrSetGCClipOrigin(GR_GC_ID gc, int xoff, int yoff)
{
	GR_GC		*gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp) {
		gcp->xoff = xoff;
		gcp->yoff = yoff;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Determines whether a specified point resides in a region.
 */ 
GR_BOOL
GrPointInRegion(GR_REGION_ID region, GR_COORD x, GR_COORD y)
{
	GR_REGION	*regionp;
	GR_BOOL		result;
	
	SERVER_LOCK();
	
	regionp =  GsFindRegion(region);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return GR_FALSE;
	}
	
	result = GdPtInRegion(regionp->rgn, x, y);
	
	SERVER_UNLOCK();
	
	return result;
}

/*
 * Determines whether a specified rectangle at least partly resides
 * in a region.
 */ 
int
GrRectInRegion(GR_REGION_ID region, GR_COORD x, GR_COORD y, GR_COORD w,
	GR_COORD h)
{
	GR_REGION	*regionp;
	MWRECT		rect;
	int		result;
	
	SERVER_LOCK();
	
	regionp =  GsFindRegion(region);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return MWRECT_OUT;
	}
	
	rect.left = x;
	rect.top = y;
	rect.right = x + w;
	rect.bottom = y + h;
	result = GdRectInRegion(regionp->rgn, &rect);
	
	SERVER_UNLOCK();
	
	return result;
}

/*
 * Return GR_TRUE if a region is empty.
 */ 
GR_BOOL
GrEmptyRegion(GR_REGION_ID region)
{
	GR_REGION	*regionp;
	GR_BOOL		result;
	
	SERVER_LOCK();
	
	regionp =  GsFindRegion(region);
	if (regionp == NULL) {
		SERVER_UNLOCK();
		return GR_TRUE;
	}
	
	result = GdEmptyRegion(regionp->rgn);
	
	SERVER_UNLOCK();
	
	return result;
}

/*
 * Return GR_TRUE if two regions are identical.
 */ 
GR_BOOL
GrEqualRegion(GR_REGION_ID rgn1, GR_REGION_ID rgn2)
{
	GR_REGION	*prgn1;
	GR_REGION	*prgn2;
	GR_BOOL result;
	
	SERVER_LOCK();
	
	prgn1 = GsFindRegion(rgn1);
	prgn2 = GsFindRegion(rgn2);

	if (!prgn1 && !prgn2) {
		result = GR_TRUE;
	} else if (!prgn1 || !prgn2) {
		result = GR_FALSE;
	} else {
		result = GdEqualRegion(prgn1->rgn, prgn2->rgn);
	}
	
	SERVER_UNLOCK();
	
	return result;
}

/*
 * Offset a region by dx, dy.
 */ 
void
GrOffsetRegion(GR_REGION_ID region, GR_SIZE dx, GR_SIZE dy)
{
	GR_REGION	*regionp;
	
	SERVER_LOCK();
	
	regionp =  GsFindRegion(region);
	if (regionp)
		GdOffsetRegion(regionp->rgn, dx, dy);
	
	SERVER_UNLOCK();
}

/*
 * Return the bounding box for the specified region.
 */
int
GrGetRegionBox(GR_REGION_ID region, GR_RECT *rect)
{
	GR_REGION	*regionp;
	MWRECT		rc;
	int		ret_val;
	
	SERVER_LOCK();
	
	regionp =  GsFindRegion(region);
	if (regionp == NULL) {
		memset(rect, 0, sizeof(GR_RECT));
		SERVER_UNLOCK();
		return MWREGION_ERROR;
	}
	
	ret_val = GdGetRegionBox(regionp->rgn, &rc);
	/* convert MW rect to Nano-X rect*/
	rect->x = rc.left;
	rect->y = rc.top;
	rect->width = rc.right - rc.left;
	rect->height = rc.bottom - rc.top;
	
	SERVER_UNLOCK();
	
	return ret_val;
}

static int nextfontid = 1000;
/*
 * Allocate a new GC with default parameters.
 * The GC is owned by the current client.
 */
GR_FONT_ID
GrCreateFont(GR_CHAR *name, GR_COORD height, GR_LOGFONT *plogfont)
{
	GR_FONT	*fontp;

	SERVER_LOCK();
	
	fontp = (GR_FONT *) malloc(sizeof(GR_FONT));
	if (fontp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}

	if (plogfont)
		fontp->pfont = GdCreateFont(&scrdev, NULL, 0, plogfont);
	else
		fontp->pfont = GdCreateFont(&scrdev, (const char *)name, height, NULL);

	/* if no font created, deallocate and return ID 0*/
	if (!fontp->pfont) {
		free(fontp);
		return 0;
	}

	fontp->id = nextfontid++;
	fontp->owner = curclient;
	fontp->next = listfontp;

	listfontp = fontp;

	SERVER_UNLOCK();
	
	return fontp->id;
}

#if HAVE_FREETYPE_2_SUPPORT
/*
 * Create a new font from memory.
 * The font is owned by the current client.
 */
GR_FONT_ID
GrCreateFontFromBuffer(const void *buffer, unsigned length,
		       const char *format, GR_COORD height)
{
	GR_FONT *fontp;

	SERVER_LOCK();
	
	fontp = (GR_FONT *)malloc(sizeof(GR_FONT));
	if (fontp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}

	/* DPRINTF("Font magic = '%c%c%c%c', len = %u @ GrCreateFontFromBuffer\n",
	 *         (char) buffer[0], (char) buffer[1],
	 *         (char) buffer[2], (char) buffer[3], length);
	 */

	fontp->pfont = GdCreateFontFromBuffer(&scrdev, buffer, length, format,
				       height);
	if (fontp->pfont == NULL) {
		/* Error loading font, probably corrupt data or unsupported format. */
		free(fontp);
		SERVER_UNLOCK();
		return 0;
	}

	fontp->id = nextfontid++;
	fontp->owner = curclient;
	fontp->next = listfontp;
	listfontp = fontp;

	SERVER_UNLOCK();
	return fontp->id;
}

/*
 * Create a new font from memory.
 * The font is owned by the current client.
 */
GR_FONT_ID
GrCopyFont(GR_FONT_ID fontid, GR_COORD height)
{
	GR_FONT *srcfontp;
	GR_FONT *fontp;

	SERVER_LOCK();

	fontp = (GR_FONT *)malloc(sizeof(GR_FONT));
	if (fontp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}

	srcfontp = GsFindFont(fontid);
	if (srcfontp)
		fontp->pfont = GdDuplicateFont(&scrdev, srcfontp->pfont,height);
	else
		fontp->pfont = GdCreateFont(&scrdev, NULL, height, NULL);

	fontp->id = nextfontid++;
	fontp->owner = curclient;
	fontp->next = listfontp;
	listfontp = fontp;
	
	SERVER_UNLOCK();
	return fontp->id;
}
#endif /*HAVE_FREETYPE_2_SUPPORT*/

/* Set the font size for the passed font*/
void
GrSetFontSize(GR_FONT_ID fontid, GR_COORD size)
{
	GR_FONT		*fontp;

	SERVER_LOCK();

	fontp = GsFindFont(fontid);
	if (fontp)
		GdSetFontSize(fontp->pfont, size);

	SERVER_UNLOCK();
}

/* Set the font rotation in tenths of degrees for the passed font*/
void
GrSetFontRotation(GR_FONT_ID fontid, int tenthdegrees)
{
	GR_FONT		*fontp;

	SERVER_LOCK();

	fontp = GsFindFont(fontid);
	if (fontp)
		GdSetFontRotation(fontp->pfont, tenthdegrees);

	SERVER_UNLOCK();
}

/* Set the font size for the passed font*/
void
GrSetFontAttr(GR_FONT_ID fontid, int setflags, int clrflags)
{
	GR_FONT		*fontp;

	SERVER_LOCK();

	fontp = GsFindFont(fontid);
	if (fontp)
		GdSetFontAttr(fontp->pfont, setflags, clrflags);

	SERVER_UNLOCK();
}

/*
 * Unload and deallocate an existing font.
 */
void
GrDestroyFont(GR_FONT_ID fontid)
{
	GR_FONT		*fontp;
	GR_FONT		*prevfontp;

	SERVER_LOCK();

	fontp = GsFindFont(fontid);
	if (fontp == NULL) {
		SERVER_UNLOCK();
		return;
	}

	if (listfontp == fontp)
		listfontp = fontp->next;
	else {
		prevfontp = listfontp;
		while (prevfontp->next != fontp)
			prevfontp = prevfontp->next;

		prevfontp->next = fontp->next;
	}
	GdDestroyFont(fontp->pfont);
	free(fontp);

	SERVER_UNLOCK();
}

/*
 * Return useful information about the specified font.
 * Font #0 returns info about the standard font.
 */
void
GrGetFontInfo(GR_FONT_ID font, GR_FONT_INFO *fip)
{
	GR_FONT	*fontp;
	PMWFONT	pf;

	SERVER_LOCK();

	if (font == 0)
		pf = stdfont;
	else {
		fontp = GsFindFont(font);
		if (!fontp) {
			memset(fip, 0, sizeof(GR_FONT_INFO));
			SERVER_UNLOCK();
			return;
		}
		pf = fontp->pfont;
	}
	GdGetFontInfo(pf, fip);

	SERVER_UNLOCK();
}

/*
 * Select events for a window for this client.
 * The events are a bitmask for the events desired.
 */
void 
GrSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_EVENT_CLIENT	*evp;		/* event-client structure */

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (wp == NULL) {
		SERVER_UNLOCK();
		return;
	}

	/*
	 * See if this client is already in the event client list.
	 * If so, then just replace the events he is selecting for.
	 */
	for (evp = wp->eventclients; evp; evp = evp->next) {
		if (evp->client == curclient) {
			evp->eventmask = eventmask;
			SERVER_UNLOCK();
			return;
		}
	}

	/*
	 * A new client for this window, so allocate a new event client
	 * structure and insert it into the front of the list in the window.
	 */
	evp = (GR_EVENT_CLIENT *) malloc(sizeof(GR_EVENT_CLIENT));
	if (evp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, wid);
		SERVER_UNLOCK();
		return;
	}

	evp->client = curclient;
	evp->eventmask = eventmask;
	evp->next = wp->eventclients;
	wp->eventclients = evp;

	/*
	 * If it's a request for child updates to the root window,
	 * then search entire list and send map events for
	 * mapped windows now.  This allows a window manager
	 * to get the mapped window list without another API call.
	 */
	if (wid==GR_ROOT_WINDOW_ID && (eventmask & GR_EVENT_MASK_CHLD_UPDATE)) {
		for (wp = listwp; wp; wp = wp->next) {
			if (wp->realized)
				GsDeliverUpdateEvent(wp, GR_UPDATE_MAP,
					wp->x, wp->y, wp->width, wp->height);
		}
	}

	SERVER_UNLOCK();
}

static GR_WINDOW *
NewWindow(GR_WINDOW *pwp, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_SIZE bordersize, GR_COLOR background,
	GR_COLOR bordercolor)
{
	GR_WINDOW	*wp;	/* new window*/

	if (width <= 0 || height <= 0 || bordersize < 0) {
		GsError(GR_ERROR_BAD_WINDOW_SIZE, 0);
		return NULL;
	}

	wp = (GR_WINDOW *) malloc(sizeof(GR_WINDOW));
	if (wp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		return NULL;
	}

	wp->id = nextid++;
	wp->psd = rootwp->psd;
	wp->parent = pwp;
	wp->children = NULL;
	wp->siblings = pwp->children;
	wp->next = listwp;
	wp->x = pwp->x + x;
	wp->y = pwp->y + y;
	wp->width = width;
	wp->height = height;
	wp->bordersize = bordersize;
	wp->background = background;
	wp->bgpixmap = NULL;
	wp->bgpixmapflags = GR_BACKGROUND_TILE;
	wp->bordercolor = bordercolor;
	wp->nopropmask = 0;
	wp->eventclients = NULL;
	wp->owner = curclient;
	wp->cursorid = pwp->cursorid;
	wp->mapped = GR_FALSE;
	wp->realized = GR_FALSE;
	wp->output = GR_TRUE;
	wp->props = 0;
	wp->title = NULL;
	wp->clipregion = NULL;

	pwp->children = wp;
	listwp = wp;

	return wp;
}

/*
 * Allocate a new window which is a child of the specified window.
 * The window inherits the cursor of the parent window.
 * The window is owned by the current client.
 */
GR_WINDOW_ID
GrNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_SIZE bordersize, GR_COLOR background,
	GR_COLOR bordercolor)
{
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*wp;		/* new window */
	GR_WINDOW_ID id;

	SERVER_LOCK();

	pwp = GsFindWindow(parent);
	if (pwp == NULL) {
		SERVER_UNLOCK();
		return 0;
	}

	if (!pwp->output) {
		GsError(GR_ERROR_INPUT_ONLY_WINDOW, pwp->id);
		SERVER_UNLOCK();
		return 0;
	}

	wp = NewWindow(pwp, x, y, width, height, bordersize, background,
		bordercolor);
	id = wp? wp->id: 0;
	
	SERVER_UNLOCK();
	
	return id;
}

/*
 * Allocate a new input-only window which is a child of the specified window.
 * Such a window is invisible, cannot be drawn into, and is only used to
 * return events.  The window inherits the cursor of the parent window.
 * The window is owned by the current client.
 */
GR_WINDOW_ID
GrNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height)
{
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*wp;		/* new window */
	GR_WINDOW_ID id;

	SERVER_LOCK();

	pwp = GsFindWindow(parent);
	if (pwp == NULL) {
		SERVER_UNLOCK();
		return 0;
	}

	wp = NewWindow(pwp, x, y, width, height, 0, BLACK, BLACK);
	if (wp) {
		/* convert to input-only window*/
		wp->output = GR_FALSE;
		id = wp->id;
		SERVER_UNLOCK();
		return id;
	}
	SERVER_UNLOCK();
	return 0;
}

/*
 * Allocate a pixmap, can be used with any drawing functions
 * for offscreen drawing
 */
GR_WINDOW_ID
GrNewPixmap(GR_SIZE width, GR_SIZE height, void * pixels)
{
	GR_PIXMAP	*pp;
	PSD		psd;
        int 		size, linelen, bpp, planes;
	GR_WINDOW_ID id;
   
	if (width <= 0 || height <= 0) {
		/* no error for now, server will desynchronize w/app*/
		/*GsError(GR_ERROR_BAD_WINDOW_SIZE, 0);*/
		return 0;
	}

	SERVER_LOCK();

	/*
	 * allocate offscreen psd.  If screen driver doesn't
	 * support blitting, this will fail.  Use root window screen
	 * device for compatibility for now.
	 */
	planes = rootwp->psd->planes;
	bpp = rootwp->psd->bpp;
        psd = rootwp->psd->AllocateMemGC(rootwp->psd);
	if (!psd) {
		SERVER_UNLOCK();
		return 0;
	}

	pp = (GR_PIXMAP *) malloc(sizeof(GR_PIXMAP));
	if (pp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		psd->FreeMemGC(psd);
		SERVER_UNLOCK();
		return 0;
	}

        GdCalcMemGCAlloc(psd, width, height, 0, 0, &size, &linelen);

	/* Allocate space for pixel values */
        if (!pixels) {
	        pixels = calloc(size, 1);
		psd->flags |= PSF_ADDRMALLOC;
	}
	if (!pixels) {
		free(pp);
		psd->FreeMemGC(psd);
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}
  
	pp->id = nextid++;
	pp->next = listpp;
	pp->psd = psd;
	pp->x = 0;
	pp->y = 0;
	pp->width = width;
	pp->height = height;
	pp->owner = curclient;

        psd->MapMemGC(psd, width, height, planes, bpp, linelen, size,
		pixels);
	
        listpp = pp;
	
	id = pp->id;
	
	SERVER_UNLOCK();
	
	return id;
}

/*
 * Map the window to make it (and possibly its children) visible on the screen.
 */
void
GrMapWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (!wp || wp->mapped) {
		SERVER_UNLOCK();
		return;
	}

	wp->mapped = GR_TRUE;

	GsWpRealizeWindow(wp, GR_FALSE);

	SERVER_UNLOCK();
}

/*
 * Unmap the window to make it and its children invisible on the screen.
 */
void
GrUnmapWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */

	SERVER_LOCK();
	
	wp = GsFindWindow(wid);
	if (!wp || !wp->mapped) {
		SERVER_UNLOCK();
		return;
	}

	GsWpUnrealizeWindow(wp, GR_FALSE);

	wp->mapped = GR_FALSE;

	SERVER_UNLOCK();
}

/*
 * Clear the associated area of a window to its background color
 * or pixmap.  Generate expose event for window if exposeflag set.
 */
void
GrClearArea(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_BOOL exposeflag)
{
	GR_WINDOW		*wp;	/* window structure */

	SERVER_LOCK();

	wp = GsPrepareWindow(wid);
	if (wp) {
		if (width == 0)
			width = wp->width;
		if (height == 0)
			height = wp->height;
		GsWpClearWindow(wp, x, y, width, height, exposeflag);
	}

	SERVER_UNLOCK();
}

/* Return window with keyboard focus.*/
GR_WINDOW_ID
GrGetFocus(void)
{
	GR_WINDOW_ID id;

	SERVER_LOCK();
	id = focuswp->id;
	SERVER_UNLOCK();

	return id;
}

/*
 * Set the focus to a particular window.
 * This makes keyboard events only visible to that window or children of it,
 * depending on the pointer location.
 */
void
GrSetFocus(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (wp == NULL) {
		SERVER_UNLOCK();
		return;
	}

	if (!wp->realized) {
		GsError(GR_ERROR_UNMAPPED_FOCUS_WINDOW, wid);
		SERVER_UNLOCK();
		return;
	}

	/* Check if window wants focus, if not, ignore call*/
	if (wp->props & GR_WM_PROPS_NOFOCUS) {
		SERVER_UNLOCK();
		return;
	}

	focusfixed = (wp != rootwp);
	GsWpSetFocus(wp);

	SERVER_UNLOCK();
}

/*
 * Create a new server-based cursor resource.
 */
static int nextcursorid = 1000;
GR_CURSOR_ID
GrNewCursor(GR_SIZE width, GR_SIZE height, GR_COORD hotx, GR_COORD hoty,
	GR_COLOR foreground, GR_COLOR background, GR_BITMAP *fgbitmap,
	GR_BITMAP *bgbitmap)
{
	GR_CURSOR	*cp;
	int		bytes;
	GR_CURSOR_ID id;

	SERVER_LOCK();

	/*
	 * Make sure the size of the bitmap is reasonable.
	 */
	if (width <= 0 || width > MWMAX_CURSOR_SIZE ||
	    height <= 0 || height > MWMAX_CURSOR_SIZE) {
		GsError(GR_ERROR_BAD_CURSOR_SIZE, 0);
		SERVER_UNLOCK();
		return 0;
	}

	cp = (GR_CURSOR *)malloc(sizeof(GR_CURSOR));
	if (cp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}

	/* fill in cursor structure*/
	cp->cursor.width = width;
	cp->cursor.height = height;
	cp->cursor.hotx = hotx;
	cp->cursor.hoty = hoty;
	cp->cursor.fgcolor = foreground;
	cp->cursor.bgcolor = background;

	bytes = GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);
	memcpy(&cp->cursor.image, fgbitmap, bytes);
	memcpy(&cp->cursor.mask, bgbitmap, bytes);

	cp->id = nextcursorid++;
	cp->owner = curclient;
	cp->next = listcursorp;
	listcursorp = cp;

	id = cp->id;
	
	SERVER_UNLOCK();
	
	return id;
}

/*
 * Destroy a server-based cursor.
 */
void
GrDestroyCursor(GR_CURSOR_ID cid)
{
	GR_CURSOR	*cursorp;
	GR_CURSOR	*prevcursorp;

	SERVER_LOCK();

	cursorp = GsFindCursor(cid);
	if (cursorp == NULL) {
		SERVER_UNLOCK();
		return;
	}

	if (listcursorp == cursorp)
		listcursorp = cursorp->next;
	else {
		prevcursorp = listcursorp;
		while (prevcursorp->next != cursorp)
			prevcursorp = prevcursorp->next;

		prevcursorp->next = cursorp->next;
	}

	if (curcursor == cursorp)
		curcursor = NULL;

	free(cursorp);
	GsCheckCursor();

	SERVER_UNLOCK();
}

/*
 * Specify a cursor for a window.
 * This cursor will only be used within that window, and by default
 * for its new children.  If the cursor is currently within this
 * window, it will be changed to the new one immediately.
 * If the new cursor id is 0, revert to the root window cursor.
 */
void
GrSetWindowCursor(GR_WINDOW_ID wid, GR_CURSOR_ID cid)
{
	GR_WINDOW	*wp;
	GR_CURSOR	*cp;		/* cursor structure */

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (wp == NULL) {
		SERVER_UNLOCK();
		return;
	}

	if (cid == 0)
		cp = stdcursor;
	else {
		cp = GsFindCursor(cid);
		if (!cp) {
			SERVER_UNLOCK();
			return;		/* FIXME add server errmsg*/
		}
	}
	wp->cursorid = cid;

	/*
	 * If this was the current cursor, then draw the new one.
	 */
	if (cp == curcursor || curcursor == NULL) {
		GdMoveCursor(cursorx - cp->cursor.hotx,
			cursory - cp->cursor.hoty);
		GdSetCursor(&cp->cursor);
	}

	GsCheckCursor();

	SERVER_UNLOCK();
}

/*
 * Move the cursor to the specified absolute screen coordinates.
 * The coordinates are that of the defined hot spot of the cursor.
 * The cursor's appearance is changed to that defined for the window
 * in which the cursor is moved to.  In addition, mouse enter, mouse
 * exit, focus in, and focus out events are generated if necessary.
 * The current mouse location is also changed.
 */
void
GrMoveCursor(GR_COORD x, GR_COORD y)
{

	SERVER_LOCK();

	/*
	 * Move the cursor only if necessary, offsetting it to
	 * place the hot spot at the specified coordinates.
	 */
	if (x != cursorx || y != cursory) {
		if(curcursor) {
			GdMoveCursor(x - curcursor->cursor.hotx,
				y - curcursor->cursor.hoty);
			GdMoveMouse(x, y);
		}
		cursorx = x;
		cursory = y;
	}

	/*
	 * Now check to see which window the mouse is in, whether or
	 * not the cursor shape should be changed, and whether or not
	 * the input focus window should be changed.
	 */
	GsCheckMouseWindow();
	GsCheckFocusWindow();
	GsCheckCursor();

	SERVER_UNLOCK();
}

/*
 * Set the foreground color in a graphics context from a passed RGB color value.
 */
void
GrSetGCForeground(GR_GC_ID gc, GR_COLOR foreground)
{
	GR_GC		*gcp;		/* graphics context */

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp && ((gcp->foreground != foreground) || gcp->fgispixelval)) {
		gcp->foreground = foreground;
		gcp->fgispixelval = GR_FALSE;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Set the background color in a graphics context from a passed RGB color value.
 */
void
GrSetGCBackground(GR_GC_ID gc, GR_COLOR background)
{
	GR_GC		*gcp;		/* graphics context */

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp && ((gcp->background != background) || gcp->bgispixelval)) {
		gcp->background = background;
		gcp->bgispixelval = GR_FALSE;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Set the foreground color in a graphics context from a passed pixel value.
 */
void
GrSetGCForegroundPixelVal(GR_GC_ID gc, GR_PIXELVAL foreground)
{
	GR_GC *gcp;		/* graphics context */

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp && ((gcp->foreground != foreground) || !gcp->fgispixelval)) {
		gcp->foreground = foreground;
		gcp->fgispixelval = GR_TRUE;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Set the background color in a graphics context from a passed pixel value.
 */
void
GrSetGCBackgroundPixelVal(GR_GC_ID gc, GR_PIXELVAL background)
{
	GR_GC *gcp;		/* graphics context */

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp && ((gcp->background != background) || !gcp->bgispixelval)) {
		gcp->background = background;
		gcp->bgispixelval = GR_TRUE;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Set whether or not the background color is drawn in bitmaps and text.
 */
void
GrSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag)
{
	GR_GC		*gcp;		/* graphics context */

	SERVER_LOCK();

	flag = (flag != 0);
	gcp = GsFindGC(gc);
	if (gcp && gcp->usebackground != flag) {
		gcp->usebackground = flag;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Set the drawing mode in a graphics context.
 */
void
GrSetGCMode(GR_GC_ID gc, int mode)
{
	GR_GC		*gcp;		/* graphics context */

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (!gcp || gcp->mode == mode) {
		SERVER_UNLOCK();
		return;
	}
	if ((mode & GR_MODE_DRAWMASK) > GR_MAX_MODE) {
		GsError(GR_ERROR_BAD_DRAWING_MODE, gc);
		SERVER_UNLOCK();
		return;
	}

	gcp->mode = mode;
	gcp->changed = GR_TRUE;

	SERVER_UNLOCK();
}

/* 
 * Set the attributes of the line.  
 */
void
GrSetGCLineAttributes(GR_GC_ID gc, int linestyle)
{
	GR_GC *gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (!gcp) {
		SERVER_UNLOCK();
		return;
	}

	switch (linestyle) {
	case GR_LINE_SOLID:
	case GR_LINE_ONOFF_DASH:
		gcp->linestyle = linestyle;
		break;

	default:
		GsError(GR_ERROR_BAD_LINE_ATTRIBUTE, gc);
		SERVER_UNLOCK();
		return;
	}
	gcp->changed = GR_TRUE;

	SERVER_UNLOCK();
}

/*
 * Set the dash mode 
 * A series of numbers are passed indicating the on / off state 
 * for example { 3, 1 } indicates 3 on and 1 off 
 */
void
GrSetGCDash(GR_GC_ID gc, char *dashes, int count)
{
	GR_GC *gcp;
	unsigned long dmask = 0;
	int dcount = 0;
	int onoff = 1;
	int i;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (!gcp) {
		SERVER_UNLOCK();
		return;
	}

	/* Build the bitmask (up to 32 bits) */
	for (i = 0; i < count; i++) {
		int b = 0;

		for (; b < dashes[i]; b++) {
			if (onoff)
				dmask |= (1 << dcount);
			if ((++dcount) == 32)
				break;
		}

		onoff = (onoff + 1) % 2;
		if (dcount == 32)
			break;
	}
	gcp->dashmask = dmask;
	gcp->dashcount = dcount;
	gcp->changed = GR_TRUE;

	SERVER_UNLOCK();
}

void
GrSetGCFillMode(GR_GC_ID gc, int fillmode)
{
	GR_GC *gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (!gcp) {
		SERVER_UNLOCK();
		return;
	}

	switch (fillmode) {
	case GR_FILL_SOLID:
	case GR_FILL_STIPPLE:
	case GR_FILL_OPAQUE_STIPPLE:
	case GR_FILL_TILE:
		gcp->fillmode = fillmode;
		break;

	default:
		GsError(GR_ERROR_BAD_FILL_MODE, gc);
		SERVER_UNLOCK();
		return;
	}
	gcp->changed = GR_TRUE;

	SERVER_UNLOCK();
}

void
GrSetGCStipple(GR_GC_ID gc, GR_BITMAP * bitmap, GR_SIZE width, GR_SIZE height)
{
	GR_GC *gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (!gcp) {
		SERVER_UNLOCK();
		return;
	}

	if (gcp->stipple.bitmap)
		free(gcp->stipple.bitmap);

	if (!bitmap) {
		gcp->stipple.bitmap = 0;
		gcp->stipple.width = gcp->stipple.height = 0;
		gcp->changed = GR_TRUE;
		SERVER_UNLOCK();
		return;
	}

	gcp->stipple.bitmap =
		malloc(GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP));
	memcpy(gcp->stipple.bitmap, bitmap,
	       GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP));

	gcp->stipple.width = width;
	gcp->stipple.height = height;
	gcp->changed = GR_TRUE;

	SERVER_UNLOCK();
}

void
GrSetGCTile(GR_GC_ID gc, GR_WINDOW_ID pixmap, GR_SIZE width, GR_SIZE height)
{
	GR_WINDOW *win;
	GR_GC *gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (!gcp) {
		SERVER_UNLOCK();
		return;
	}

	if (!pixmap) {
		gcp->tile.psd = NULL;
		gcp->tile.width = gcp->tile.height = 0;
		gcp->changed = GR_TRUE;
		SERVER_UNLOCK();
		return;
	}

	win = GsFindWindow(pixmap);
	if (win)
		gcp->tile.psd = win->psd;
	else {
		GR_PIXMAP *pix = GsFindPixmap(pixmap);
		if (!pix) {
			gcp->tile.psd = NULL;
			gcp->tile.width = gcp->tile.height = 0;
			gcp->changed = GR_TRUE;
			SERVER_UNLOCK();
			return;
		}
		gcp->tile.psd = pix->psd;
	}

	/* FIXME:  Set a size restriction here? */
	gcp->tile.width = width;
	gcp->tile.height = height;
	gcp->changed = GR_TRUE;

	SERVER_UNLOCK();
}

void
GrSetGCTSOffset(GR_GC_ID gc, GR_COORD xoff, GR_COORD yoff)
{
	GR_GC *gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp) {
		gcp->ts_offset.x = xoff;
		gcp->ts_offset.y = yoff;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/* 
 * Boolean that sets if we send EXPOSE events on a GrCopyArea
 */
void 
GrSetGCGraphicsExposure(GR_GC_ID gc, GR_BOOL exposure)
{
	GR_GC		*gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp) {
		gcp->exposure = exposure;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}


/*
 * Set the text font in a graphics context.
 */
void
GrSetGCFont(GR_GC_ID gc, GR_FONT_ID font)
{
	GR_GC		*gcp;

	SERVER_LOCK();

	gcp = GsFindGC(gc);
	if (gcp && gcp->fontid != font) {
		gcp->fontid = font;
		gcp->changed = GR_TRUE;
	}

	SERVER_UNLOCK();
}

/*
 * Draw a line in the specified drawable using the specified graphics context.
 */

void
GrLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1, GR_COORD x2,
	GR_COORD y2)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdLine(dp->psd, dp->x + x1, dp->y + y1,
				dp->x + x2, dp->y + y2, TRUE);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Draw the boundary of a rectangle in the specified drawable using the
 * specified graphics context.
 * NOTE: this function draws a rectangle 1 pixel wider and higher
 * than Xlib's XDrawRectangle().
 */
void 
GrRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
      	        case GR_DRAW_TYPE_PIXMAP:
			GdRect(dp->psd, dp->x + x, dp->y + y, width, height);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Fill a rectangle in the specified drawable using the specified
 * graphics context.
 */
void
GrFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdFillRect(dp->psd, dp->x + x, dp->y + y, width,height);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Draw the boundary of an ellipse in the specified drawable with
 * the specified graphics context.  Integer only.
 */
void
GrEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx,
	GR_SIZE ry)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdEllipse(dp->psd, dp->x + x, dp->y + y, rx, ry, FALSE);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Fill an ellipse in the specified drawable using the specified
 * graphics context.  Integer only.
 */
void
GrFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx,
	GR_SIZE ry)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdEllipse(dp->psd, dp->x + x, dp->y + y, rx, ry, TRUE);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Draw an arc, pie or ellipse in the specified drawable using
 * the specified graphics context.  Integer only.
 */
void	
GrArc(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE rx, GR_SIZE ry, GR_COORD ax, GR_COORD ay,
	GR_COORD bx, GR_COORD by, int type)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdArc(dp->psd, dp->x + x, dp->y + y, rx, ry, ax, ay,
					bx, by, type);

			break;
	}

	SERVER_UNLOCK();
}

/*
 * Draw an arc or pie in the specified drawable using
 * the specified graphics context.  Requires floating point.
 */
void
GrArcAngle(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE rx, GR_SIZE ry, GR_COORD angle1, GR_COORD angle2, int type)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdArcAngle(dp->psd, dp->x + x, dp->y + y, rx, ry,
				angle1, angle2, type);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Draw a rectangular area in the specified drawable using the specified
 * graphics, as determined by the specified bit map.  This differs from
 * rectangle drawing in that the rectangle is drawn using the foreground
 * color and possibly the background color as determined by the bit map.
 * Each row of bits is aligned to the next bitmap word boundary (so there
 * is padding at the end of the row).  The background bit values are only
 * written if the usebackground flag is set in the GC.
 */
void
GrBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_BITMAP *imagebits)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdBitmap(dp->psd, dp->x + x, dp->y + y, width, height,
				imagebits);
			break;
	}

	SERVER_UNLOCK();
}

/* draw a multicolor image at x, y*/
void
GrDrawImageBits(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_IMAGE_HDR *pimage)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdDrawImage(dp->psd, dp->x + x, dp->y + y, pimage);
			break;
	}

	SERVER_UNLOCK();
}

#if MW_FEATURE_IMAGES && defined(HAVE_FILEIO)
/* Load an image file from disk and display it at the specified coordinates*/
void
GrDrawImageFromFile(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, char* path, int flags)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
 	        case GR_DRAW_TYPE_PIXMAP:
			GdDrawImageFromFile(dp->psd, dp->x + x, dp->y + y,
				width, height, path, flags);
			break;
	}

	SERVER_UNLOCK();
}

/* load image from file and cache it*/
GR_IMAGE_ID
GrLoadImageFromFile(char *path, int flags)
{
	GR_IMAGE_ID	id;
	GR_IMAGE *	imagep;

	SERVER_LOCK();

	id = GdLoadImageFromFile(&scrdev, path, flags);
	if (!id) {
		SERVER_UNLOCK();
		return 0;
	}

	imagep = (GR_IMAGE *) malloc(sizeof(GR_IMAGE));
	if (!imagep) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		GdFreeImage(id);
		SERVER_UNLOCK();
		return 0;
	}
	
	imagep->id = id;
	imagep->owner = curclient;
	imagep->next = listimagep;
	listimagep = imagep;

	SERVER_UNLOCK();
	return id;
}
#endif /* MW_FEATURE_IMAGES && defined(HAVE_FILEIO) */

#if MW_FEATURE_IMAGES
/* Draw an image from a buffer */
void
GrDrawImageFromBuffer(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, void *buffer, int size, int flags)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
	case GR_DRAW_TYPE_WINDOW:
	case GR_DRAW_TYPE_PIXMAP:
		GdDrawImageFromBuffer(dp->psd, dp->x + x, dp->y + y,
			width, height, buffer, size, flags);
		break;
	}

	SERVER_UNLOCK();
}

/* load image from the given buffer and cache it*/
GR_IMAGE_ID
GrLoadImageFromBuffer(void *buffer, int size, int flags)
{
	GR_IMAGE_ID	id;
	GR_IMAGE *	imagep;

	SERVER_LOCK();

	id = GdLoadImageFromBuffer(&scrdev, buffer, size, flags);
	if (!id) {
		SERVER_UNLOCK();
		return 0;
	}

	imagep = (GR_IMAGE *) malloc(sizeof(GR_IMAGE));
	if (!imagep) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		GdFreeImage(id);
		SERVER_UNLOCK();
		return 0;
	}

	imagep->id = id;
	imagep->owner = curclient;
	imagep->next = listimagep;
	listimagep = imagep;

	SERVER_UNLOCK();

	return id;
}

/* draw cached image*/
void
GrDrawImageToFit(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, GR_IMAGE_ID imageid)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
 	        case GR_DRAW_TYPE_PIXMAP:
			GdDrawImageToFit(dp->psd, dp->x + x, dp->y + y,
				width, height, imageid);
			break;
	   
	}

	SERVER_UNLOCK();
}

/* free cached image*/
void
GrFreeImage(GR_IMAGE_ID id)
{
	GR_IMAGE	*imagep;
	GR_IMAGE	*previmagep;

	SERVER_LOCK();

	for (imagep = listimagep; imagep; imagep = imagep->next) {
		if (imagep->id == id) {

			if (listimagep == imagep)
				listimagep = imagep->next;
			else {
				previmagep = listimagep;
				while (previmagep->next != imagep)
					previmagep = previmagep->next;

				previmagep->next = imagep->next;
			}

			GdFreeImage(imagep->id);
			free(imagep);
			SERVER_UNLOCK();
			return;
		}
	}

	SERVER_UNLOCK();
}

/* return cached image information*/
void
GrGetImageInfo(GR_IMAGE_ID id, GR_IMAGE_INFO *iip)
{
	SERVER_LOCK();
	GdGetImageInfo(id, iip);
	SERVER_UNLOCK();
}
#endif /* MW_FEATURE_IMAGES */

/*
 * Draw a rectangular area in the specified drawable using the specified
 * graphics context.  This differs from rectangle drawing in that the
 * color values for each pixel in the rectangle are specified.  
 * The color table is indexed row by row.
 */
void
GrArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, void *pixels, int pixtype)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdArea(dp->psd, dp->x + x, dp->y + y, width, height,
				pixels, pixtype);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Copy a rectangle from one drawable to another or the same
 */
void
GrCopyArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, GR_DRAW_ID srcid,
	GR_COORD srcx, GR_COORD srcy, unsigned long op)
{
  	GR_GC		*gcp;
	GR_BOOL         exposure = GR_TRUE;

	GR_DRAWABLE	*dp;
        GR_WINDOW	*swp;
        GR_PIXMAP	*spp = NULL;
        GR_DRAW_TYPE	type;
        PSD 		srcpsd;

	SERVER_LOCK();
   
        srcpsd = NULL;

        swp = GsFindWindow(srcid);
        type = GsPrepareDrawing(id, gc, &dp);
	if (type == GR_DRAW_TYPE_NONE) {
		SERVER_UNLOCK();
		return;
	}

        if (swp) {
		srcpsd = swp->psd;
		srcx += swp->x;
		srcy += swp->y;
	} else {
	       spp = GsFindPixmap(srcid);
	       if (spp)
		     srcpsd = spp->psd;
	}
	if (!srcpsd) {
		SERVER_UNLOCK();
		return;
	}

#if DYNAMICREGIONS  
	gcp = GsFindGC(gc);
	if (gcp)
		exposure = gcp->exposure;
	
	/*
	 * Skip blit and send expose event if window is partly
	 * obscured and source and destination are onscreen.
	 * Also check that receiving window's first client has
	 * selected for expose events.  This keeps brain-dead
	 * programs that don't process exposure events somewhat working.
	 */
	if (exposure && swp && (srcpsd == dp->psd) && swp->eventclients &&
	    (swp->eventclients->eventmask & GR_EVENT_MASK_EXPOSURE)) {
		MWRECT 		rc;
		extern MWCLIPREGION *clipregion;

		/* clip blit rectangle to source screen/bitmap size*/
		if(srcx+width > srcpsd->xvirtres)
			width = srcpsd->xvirtres - srcx;
		if(srcy+height > srcpsd->yvirtres)
			height = srcpsd->yvirtres - srcy;

		rc.left = srcx;
		rc.top = srcy;
		rc.right = srcx + width;
		rc.bottom = srcy + height;

		/*
		 * if source isn't entirely within clip region, then
		 * the blit is partly obscured and will copy some garbage.
		 * In this case, skip the blit, punt, and deliver an
		 * exposure event instead for proper display.
		 */
		if (GdRectInRegion(clipregion, &rc) != MWRECT_ALLIN) {
EPRINTF("nano-X: skipping blit, sending expose event\n");
			GsDeliverExposureEvent(swp, dp->x+x, dp->y+y,
				width, height);
			SERVER_UNLOCK();
			return;
		}
	}
#endif

	if (op == MWROP_USE_GC_MODE) {
		GR_GC *gcp = GsFindGC(gc);

		if (gcp == NULL) {
			op = MWROP_COPY;
		} else {
			op = MWMODE_TO_ROP(gcp->mode);
		}
	}

	/* perform blit*/
	GdCheckCursor(srcpsd, srcx, srcy, srcx+width, srcy+height); /* FIXME*/
	GdBlit(dp->psd, dp->x+x, dp->y+y, width, height, srcpsd, srcx, srcy,op);
	GdFixCursor(srcpsd); /* FIXME*/

	SERVER_UNLOCK();
}


/*
 * Read the color values from the specified rectangular area of the
 * specified drawable into a supplied buffer.  If the drawable is a
 * window which is obscured by other windows, then the returned values
 * will include the values from the covering windows.  Regions outside
 * of the screen boundaries, or unmapped windows will return black.
 */
void
GrReadArea(GR_DRAW_ID id,GR_COORD x,GR_COORD y,GR_SIZE width,GR_SIZE height,
	GR_PIXELVAL *pixels)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pp = NULL;

	SERVER_LOCK();

	if ((wp = GsFindWindow(id)) == NULL && (pp = GsFindPixmap(id)) == NULL){
		GsError(GR_ERROR_BAD_WINDOW_ID, id);
		SERVER_UNLOCK();
		return;
	}

	if (wp != NULL) {
		if (!wp->realized || (x >= wp->width) || (y >= wp->height) ||
		   (x + width <= 0) || (y + height <= 0)) {
			/* long		count;
			* GR_PIXELVAL	black;
			*
			* black = GdFindColor(BLACK);
			* count = width * height;
			* while (count-- > 0)
			*	*pixels++ = black;
			*/
			SERVER_UNLOCK();
			return;
		}
		GdReadArea(wp->psd, wp->x+x, wp->y+y, width, height, pixels);
	}
	if (pp != NULL) {
		if ((x >= pp->width) || (y >= pp->height) ||
		    (x + width <= 0) || (y + height <= 0)) {
			SERVER_UNLOCK();
			return;
		}
		GdReadArea(pp->psd, x, y, width, height, pixels);
	}

	SERVER_UNLOCK();
}

/*
 * Draw a point in the specified drawable using the specified
 * graphics context.
 */
void
GrPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdPoint(dp->psd, dp->x + x, dp->y + y);
			break;
	}

	SERVER_UNLOCK();
}

/*
 * Draw points in the specified drawable using the specified
 * graphics context.
 */
void
GrPoints(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	GR_DRAWABLE	*dp;
	GR_POINT	*pp;
	GR_COUNT	i;
        PSD 		psd;

	SERVER_LOCK();
   
	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
	                psd = dp->psd;
	                break;
		default:
			SERVER_UNLOCK();
			return;
	}

	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		GdPoint(psd, pp->x + dp->x, pp->y + dp->y);
	}

	SERVER_UNLOCK();
}

/*
 * Draw a polygon in the specified drawable using the specified
 * graphics context.  The polygon is only complete if the first
 * point is repeated at the end.
 */
void
GrPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	GR_DRAWABLE	*dp;
	GR_POINT	*pp;
	GR_COUNT	i;
        PSD 		psd;

	SERVER_LOCK();
   
	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
	                psd = dp->psd;
	                break;
		default:
			SERVER_UNLOCK();
			return;
	}

	/*
	 * Here for drawing to a window.
	 * Relocate all the points relative to the window.
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x += dp->x;
		pp->y += dp->y;
	}

	GdPoly(psd, count, pointtable);

#ifdef NONETWORK   
	/*
	 * The following is only necessary when the server
	 * isn't a separate process.  We don't want to change the
	 * user's arguments!
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x -= dp->x;
		pp->y -= dp->y;
	}
#endif

	SERVER_UNLOCK();
}

/*
 * Draw a filled polygon in the specified drawable using the specified
 * graphics context.  The last point may be a duplicate of the first
 * point, but this is not required.
 */
void
GrFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	GR_DRAWABLE	*dp;
	GR_POINT	*pp;
	GR_COUNT	i;
        PSD 		psd;

	SERVER_LOCK();
   
	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
	                psd = dp->psd;
			break;
		default:
			SERVER_UNLOCK();
			return;
	}

	/*
	 * Here for drawing to a window.
	 * Relocate all the points relative to the window.
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x += dp->x;
		pp->y += dp->y;
	}

	GdFillPoly(psd, count, pointtable);

#ifdef NONETWORK
	/*
	 * The following is only necessary when the server
	 * isn't a separate process.  We don't want to change the
	 * user's arguments!
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x -= dp->x;
		pp->y -= dp->y;
	}
#endif   

	SERVER_UNLOCK();
}

/*
 * Draw a text string in the specified drawable using the
 * specified graphics context.
 */
void
GrText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, void *str,
	GR_COUNT count, GR_TEXTFLAGS flags)
{
	GR_DRAWABLE	*dp;

	SERVER_LOCK();

	/* default to baseline alignment if none specified*/
	if((flags&(MWTF_TOP|MWTF_BASELINE|MWTF_BOTTOM)) == 0)
		flags |= MWTF_BASELINE;

	switch (GsPrepareDrawing(id, gc, &dp)) {
		case GR_DRAW_TYPE_WINDOW:
	        case GR_DRAW_TYPE_PIXMAP:
			GdText(dp->psd, dp->x + x, dp->y + y, str, count,flags);
			break;
	}

	SERVER_UNLOCK();
}

/* Return the system palette entries*/
void
GrGetSystemPalette(GR_PALETTE *pal)
{

	SERVER_LOCK();

	/* return 0 count if not in palettized mode*/
	memset(pal, 0, sizeof(GR_PALETTE *));

	if(rootwp->psd->pixtype == MWPF_PALETTE) {
		pal->count = (int)rootwp->psd->ncolors;
		GdGetPalette(rootwp->psd, 0, pal->count, pal->palette);
	}

	SERVER_UNLOCK();
}

/* Set the system palette entries from first for count*/
void
GrSetSystemPalette(GR_COUNT first, GR_PALETTE *pal)
{
	SERVER_LOCK();

	GdSetPalette(rootwp->psd, first, pal->count, pal->palette);
	if (first == 0)
		GsRedrawScreen();

	SERVER_UNLOCK();
}

/* Convert passed color value to pixel value, depending on system mode*/
void
GrFindColor(GR_COLOR c, GR_PIXELVAL *retpixel)
{
	SERVER_LOCK();
	*retpixel = GdFindColor(&scrdev, c);
	SERVER_UNLOCK();
}

/* visible =0, no cursor change; =1, show; else hide*/
void
GrInjectPointerEvent(GR_COORD x, GR_COORD y, int button, int visible)
{
	SERVER_LOCK();

	if (visible != 0) {
		if (visible == 1)
			GdShowCursor(&scrdev);
		else
			GdHideCursor(&scrdev);
	}

	GdMoveMouse(x, y);
	GsHandleMouseStatus(x, y, button);

	SERVER_UNLOCK();
}

void
GrInjectKeyboardEvent(GR_WINDOW_ID wid, GR_KEY keyvalue, GR_KEYMOD modifiers,
	GR_SCANCODE scancode, GR_BOOL pressed)
{
	SERVER_LOCK();

	/* create a keyboard event */
	GsDeliverKeyboardEvent(wid,
		pressed? GR_EVENT_TYPE_KEY_DOWN: GR_EVENT_TYPE_KEY_UP,
		keyvalue, modifiers, scancode);

	SERVER_UNLOCK();
}

/*
 * Set certain window properties, according to flags value
 * passed in props.
 */
void
GrSetWMProperties(GR_WINDOW_ID wid, GR_WM_PROPERTIES *props)
{
	GR_WINDOW *wp;
	int tl = 0;    /* Initialized to avoid warning */

	SERVER_LOCK();

	/* Find the window structure (generate an error if it doesn't exist) */
	wp = GsFindWindow(wid);
	if(!wp) {
		GsError(GR_ERROR_BAD_WINDOW_ID, wid);
		SERVER_UNLOCK();
		return;
	}

	/* Set window properties*/
	if (props->flags & GR_WM_FLAGS_PROPS)
		wp->props = props->props;

	/* Set window title*/
	if (props->flags & GR_WM_FLAGS_TITLE) {
		/* Remove the old title if it exists */
		if(wp->title)
			free(wp->title);

		/* Calculate the space needed to store the new title */
		if(props->title)
			tl = strlen((const char *)props->title) + 1;

		/* Check for empty title*/
		if(!props->title || tl == 1) {
			wp->title = NULL;
		} else {
			/* Otherwise, allocate some space for the new title */
			if(!(wp->title = malloc(tl)))
				GsError(GR_ERROR_MALLOC_FAILED, wid);
			else
				memcpy(wp->title, props->title, tl);
		}
		
		/* send UPDATE_ACTIVATE event to force redraw*/
		GsWpNotifyActivate(focuswp);
	}

	/* Set window background*/
	if (props->flags & GR_WM_FLAGS_BACKGROUND) {
		if (wp->background != props->background) {
			wp->background = props->background;
			GsExposeArea(wp, wp->x, wp->y, wp->width, wp->height,
				NULL);
		}
	}

	/* Set window border size*/
	if (props->flags & GR_WM_FLAGS_BORDERSIZE) {
		if (wp->bordersize != props->bordersize) {
			/* FIXME: check if this works if not already realized*/
			GsWpUnrealizeWindow(wp, GR_TRUE);
			wp->bordersize = props->bordersize;
			GsWpRealizeWindow(wp, GR_TRUE);
		}
	}

	/* Set window border color*/
	if (props->flags & GR_WM_FLAGS_BORDERCOLOR) {
		if (wp->bordercolor != props->bordercolor) {
			wp->bordercolor = props->bordercolor;
			if (wp->bordersize)
				GsDrawBorder(wp);
		}
	}

	SERVER_UNLOCK();
}

/*
 * Return all window properties
 */
void
GrGetWMProperties(GR_WINDOW_ID wid, GR_WM_PROPERTIES *props)
{
	GR_WINDOW *wp;

	SERVER_LOCK();

	/* Find the window structure, no error on invalid window id*/ 
	wp = GsFindWindow(wid);
	if(!wp) {
		/* set flags to 0 on bad window id*/
		memset(props, 0, sizeof(GR_WM_PROPERTIES));
		SERVER_UNLOCK();
		return;
	}

	/* Return everything, regardless of props->flags*/
	props->flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE |
		GR_WM_FLAGS_BACKGROUND | GR_WM_FLAGS_BORDERSIZE |
		GR_WM_FLAGS_BORDERCOLOR;
	props->props = wp->props;
	props->title = wp->title;
	props->background = wp->background;
	props->bordersize = wp->bordersize;
	props->bordercolor = wp->bordercolor;

	SERVER_UNLOCK();
}

void
GrCloseWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW *wp;

	SERVER_LOCK();

	/* Find the window structure (generate an error if it doesn't exist) */
	wp = GsFindWindow(wid);
	if(!wp) {
		/* 
		 * no error for now, client/server problems
		 * with nxwm when sent
		 */
		/*GsError(GR_ERROR_BAD_WINDOW_ID, wid);*/
		SERVER_UNLOCK();
		return;
	}

	/* Send a CLOSE_REQ event to the client */
	GsDeliverGeneralEvent(wp, GR_EVENT_TYPE_CLOSE_REQ, NULL);

	SERVER_UNLOCK();
}

void
GrKillWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW *wp;

	SERVER_LOCK();

	/* Find the window structure (generate an error if it doesn't exist) */
	wp = GsFindWindow(wid);
	if(!wp) {
		GsError(GR_ERROR_BAD_WINDOW_ID, wid);
		SERVER_UNLOCK();
		return;
	}

	/* Forcibly kill the connection to the client */
	GsClose(wp->owner->id);

	SERVER_UNLOCK();
}

/*
 * GrGetSystemColor color scheme definitions
 */ 
/* define color scheme: A (tan), B (winstd) or C (old)*/
#define A

#define A_RGB(r,g,b)
#define B_RGB(r,g,b)
#define C_RGB(r,g,b)

#ifdef A
#undef  A_RGB
#define A_RGB(r,g,b)	GR_RGB(r,g,b),
#endif
#ifdef B
#undef  B_RGB
#define B_RGB(r,g,b)	GR_RGB(r,g,b),
#endif
#ifdef C
#undef  C_RGB
#define C_RGB(r,g,b)	GR_RGB(r,g,b),
#endif

#define MAXSYSCOLORS	20	/* # of GR_COLOR_* system colors*/

static GR_COLOR sysColors[MAXSYSCOLORS] = {
	/* desktop background*/
	GR_RGB(  0, 128, 128),  /* GR_COLOR_DESKTOP             */

	/* caption colors*/
	A_RGB(128,   0,   0)	/* GR_COLOR_ACTIVECAPTION       */
	B_RGB(128,   0, 128)	/* GR_COLOR_ACTIVECAPTION       */
	C_RGB(128,   0, 128)	/* GR_COLOR_ACTIVECAPTION       */
	GR_RGB(255, 255, 255),  /* GR_COLOR_ACTIVECAPTIONTEXT   */
	A_RGB(162, 141, 104)	/* GR_COLOR_INACTIVECAPTION     */
	B_RGB(128, 128, 128)  	/* GR_COLOR_INACTIVECAPTION     */
	C_RGB(  0,  64, 128)	/* GR_COLOR_INACTIVECAPTION     */
	GR_RGB(192, 192, 192),  /* GR_COLOR_INACTIVECAPTIONTEXT */

	/* 3d border shades*/
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWFRAME         */
	A_RGB(162, 141, 104)	/* GR_COLOR_BTNSHADOW           */
	B_RGB(128, 128, 128)	/* GR_COLOR_BTNSHADOW           */
	C_RGB(128, 128, 128)	/* GR_COLOR_BTNSHADOW           */
	A_RGB(213, 204, 187)	/* GR_COLOR_3DLIGHT             */
	B_RGB(223, 223, 223)	/* GR_COLOR_3DLIGHT             */
	C_RGB(192, 192, 192)	/* GR_COLOR_3DLIGHT             */
	A_RGB(234, 230, 221)  	/* GR_COLOR_BTNHIGHLIGHT        */
	B_RGB(255, 255, 255)  	/* GR_COLOR_BTNHIGHLIGHT        */
	C_RGB(223, 223, 223)  	/* GR_COLOR_BTNHIGHLIGHT        */

	/* top level application window backgrounds/text*/
	A_RGB(213, 204, 187)	/* GR_COLOR_APPWINDOW           */
	B_RGB(192, 192, 192)	/* GR_COLOR_APPWINDOW           */
	C_RGB(160, 160, 160)	/* GR_COLOR_APPWINDOW           */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_APPTEXT             */

	/* button control backgrounds/text (usually same as app window colors)*/
	A_RGB(213, 204, 187)	/* GR_COLOR_BTNFACE             */
	B_RGB(192, 192, 192)	/* GR_COLOR_BTNFACE             */
	C_RGB(160, 160, 160)	/* GR_COLOR_BTNFACE             */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_BTNTEXT             */

	/* edit/listbox control backgrounds/text, selected highlights*/
	GR_RGB(255, 255, 255),  /* GR_COLOR_WINDOW              */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_WINDOWTEXT          */
	GR_RGB(128,   0,   0),  /* GR_COLOR_HIGHLIGHT           */
	GR_RGB(255, 255, 255),  /* GR_COLOR_HIGHLIGHTTEXT       */
	GR_RGB( 64,  64,  64),  /* GR_COLOR_GRAYTEXT            */

	/* menu backgrounds/text*/
	A_RGB(213, 204, 187)	/* GR_COLOR_MENU                */
	B_RGB(192, 192, 192)	/* GR_COLOR_MENU                */
	C_RGB(160, 160, 160)	/* GR_COLOR_MENU                */
	GR_RGB(  0,   0,   0),  /* GR_COLOR_MENUTEXT            */
};

/* Return system-defined color*/
GR_COLOR
GrGetSysColor(int index)
{
	GR_COLOR color = 0;
	SERVER_LOCK();
	if(index >= 0 && index < MAXSYSCOLORS)
		color = sysColors[index];
	SERVER_UNLOCK();
	return color;
}

void
GrSetScreenSaverTimeout(GR_TIMEOUT timeout)
{
#if MW_FEATURE_TIMERS
	MWTIMER *timer;

	SERVER_LOCK();

	screensaver_delay = timeout * 1000;

	if((timer = GdFindTimer(GsActivateScreenSaver)))
		GdDestroyTimer(timer);

	/* 0 timeout cancels timer*/
	if (timeout == 0) {
		SERVER_UNLOCK();
		return;
	}

	GdAddTimer(screensaver_delay, GsActivateScreenSaver,
					GsActivateScreenSaver);

	SERVER_UNLOCK();
#endif /* MW_FEATURE_TIMERS */
}

void
GrSetSelectionOwner(GR_WINDOW_ID wid, GR_CHAR *typelist)
{
	GR_WINDOW_ID oldwid = selection_owner.wid;

	SERVER_LOCK();

	if(selection_owner.typelist) free(selection_owner.typelist);

	selection_owner.wid = wid;

	if(wid) {
		if(!(selection_owner.typelist = (GR_CHAR *)strdup((const char *)typelist))) {
			GsError(GR_ERROR_MALLOC_FAILED, wid);
			selection_owner.wid = 0;
		}
	} else selection_owner.typelist = NULL;

	GsDeliverSelectionChangedEvent(oldwid, wid);

	SERVER_UNLOCK();
}

GR_WINDOW_ID
GrGetSelectionOwner(GR_CHAR **typelist)
{
	GR_WINDOW_ID result;

	SERVER_LOCK();
	*typelist = selection_owner.typelist;
	result = selection_owner.wid;
	SERVER_UNLOCK();

	return result;
}

void
GrRequestClientData(GR_WINDOW_ID wid, GR_WINDOW_ID rid, GR_SERIALNO serial,
							GR_MIMETYPE mimetype)
{
	SERVER_LOCK();
	GsDeliverClientDataReqEvent(rid, wid, serial, mimetype);
	SERVER_UNLOCK();
}

void
GrSendClientData(GR_WINDOW_ID wid, GR_WINDOW_ID did, GR_SERIALNO serial,
				GR_LENGTH len, GR_LENGTH thislen, void *data)
{
	void *p;

	SERVER_LOCK();

	if(!(p = malloc(len))) {
		GsError(GR_ERROR_MALLOC_FAILED, wid);
		SERVER_UNLOCK();
		return;		/* FIXME note no error to application*/
	}
	memcpy(p, data, thislen);

	GsDeliverClientDataEvent(did, wid, serial, len, thislen, p);

	SERVER_UNLOCK();
}

/*
 * Set a window's background pixmap.  Note that this doesn't
 * cause a screen refresh, use GrClearWindow if required.
 */
void
GrSetBackgroundPixmap(GR_WINDOW_ID wid, GR_WINDOW_ID pixmap, int flags)
{
	GR_WINDOW *wp;
	GR_PIXMAP *pp = NULL;

	SERVER_LOCK();

	if (!(wp = GsFindWindow(wid))) {
		GsError(GR_ERROR_BAD_WINDOW_ID, wid);
		SERVER_UNLOCK();
		return;
	}

	if (pixmap && !(pp = GsFindPixmap(pixmap))) {
		GsError(GR_ERROR_BAD_WINDOW_ID, pixmap);
		SERVER_UNLOCK();
		return;
	}
	wp->bgpixmap = pp;
	wp->bgpixmapflags = flags;

	SERVER_UNLOCK();
}

void
GrGetFontList(GR_FONTLIST ***fonts, int *numfonts)
{
	SERVER_LOCK();
	GdGetFontList(fonts,numfonts);
	SERVER_UNLOCK();
}

void 
GrFreeFontList(GR_FONTLIST ***fonts, int numfonts)
{
	SERVER_LOCK();
	GdFreeFontList(fonts, numfonts);
	SERVER_UNLOCK();
}

/*
 * Copy and stretch a rectangle.
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
	GR_DRAWABLE *dp;
	GR_WINDOW *swp;
	GR_PIXMAP *spp = NULL;
	GR_DRAW_TYPE type;
	PSD srcpsd;

	SERVER_LOCK();

	srcpsd = NULL;

	type = GsPrepareDrawing(dstid, gc, &dp);
	if (type == GR_DRAW_TYPE_NONE) {
		SERVER_UNLOCK();
		return;
	}

	dx1 += dp->x;
	dy1 += dp->y;
	dx2 += dp->x;
	dy2 += dp->y;

	swp = GsFindWindow(srcid);
	if (swp) {
		srcpsd = swp->psd;
		sx1 += swp->x;
		sy1 += swp->y;
		sx2 += swp->x;
		sy2 += swp->y;
	} else {
		spp = GsFindPixmap(srcid);
		if (spp)
			srcpsd = spp->psd;
	}
	if (!srcpsd) {
		SERVER_UNLOCK();
		return;
	}

	if (op == MWROP_USE_GC_MODE) {
		GR_GC *gcp;
		gcp = GsFindGC(gc);
		if (gcp == NULL) {
			op = MWROP_COPY;
		} else {
			op = MWMODE_TO_ROP(gcp->mode);
		}
	}

	/* perform blit */

	/* DPRINTF("Nano-X: GrStretchArea() calling GdStretchBlit()\n"); */
	//GdCheckCursor(srcpsd, s1x, s1y, s2x, s2y); /* FIXME*/
	/*GdStretchBlit(dp->psd, dx, dy, dw, dh,
	   srcpsd,  sx, sy, sw, sh,
	   op); */
	GdStretchBlitEx(dp->psd, dx1, dy1, dx2, dy2,
			srcpsd, sx1, sy1, sx2, sy2, op);
	/* DPRINTF("Nano-X: GrStretchArea() done GdStretchBlitEx()\n"); */
	//GdFixCursor(srcpsd); /* FIXME*/

	SERVER_UNLOCK();
}

/*
 * Return window parent and list of children.
 * Caller must free() children list after use.
 */
void
GrQueryTree(GR_WINDOW_ID wid, GR_WINDOW_ID *parentid, GR_WINDOW_ID **children,
	GR_COUNT *nchildren)
{
	GR_WINDOW	*wp;
	GR_WINDOW	*cp;
	GR_WINDOW_ID	*retarray;
	GR_COUNT	n = 0;

	SERVER_LOCK();

	wp = GsFindWindow(wid);
	if (!wp) {
		*parentid = 0;
nochildren:
		*children = NULL;
		*nchildren = 0;
		SERVER_UNLOCK();
		return;
	}
	*parentid = wp->parent? wp->parent->id: 0;

	/* count children for alloc*/
	for(cp=wp->children; cp; cp=cp->siblings)
		++n;
	if (n == 0)
		goto nochildren;
	
	/* alloc return child array*/
	retarray = (GR_WINDOW_ID *)malloc(n * sizeof(GR_WINDOW_ID));
	if (!retarray) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		goto nochildren;
	}

	/* fill in return array*/
	n = 0;
	for(cp=wp->children; cp; cp=cp->siblings) {
		retarray[n++] = cp->id;
	}
	*children = retarray;
	*nchildren = n;

	SERVER_UNLOCK();
}


#if MW_FEATURE_TIMERS
static int next_timer_id = 1000;

/**
 * Creates a Nano-X timer with the specified period.
 * NOTE: There is a potential for more GR_TIMER_EVENTS to be queued
 * in the connection between the Nano-X server and client.  The client
 * should be able to handle late arriving GR_TIMER_EVENTs.
 *
 * @param wid the ID of the window to use as a destination for GR_TIMER_EVENT
 *	events that result from this timer.
 * @param period the timer period in milliseconds
 * @return the ID of the newly created timer, or 0 if failure.
 */
GR_TIMER_ID
GrCreateTimer (GR_WINDOW_ID wid, GR_TIMEOUT period)
{
    GR_TIMER  *timer;
	GR_TIMER_ID id;

	SERVER_LOCK();

    /* Create a nano-X layer timr */
    timer = (GR_TIMER*) malloc (sizeof (GR_TIMER));
    if (timer == NULL)
    {
        GsError (GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
        return 0;
    }

    /* Create a device independent layer timer */
    timer->timer = GdAddPeriodicTimer (period, GsTimerCB, timer);
    if (timer->timer == NULL)
    {
        free (timer);
        GsError (GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
        return 0;
    }

    /* 
     * Fill in the rest of the timer structure, and 
     * link the new timer into the servers list of timers.
     */
    timer->id    = next_timer_id++;
    timer->owner = curclient;
    timer->wid   = wid;
    timer->next  = list_timer;
    list_timer   = timer;

    id = timer->id;

	SERVER_UNLOCK();

	return id;
}

/**
 * Destroys a timer previously created with GrCreateTimer().
 *
 * @param tid the ID of the timer to destroy
 */
void
GrDestroyTimer (GR_TIMER_ID tid)
{
    GR_TIMER  *timer;
    GR_TIMER  *prev_timer;

	SERVER_LOCK();
    
    /* Find the timer structure that corresponds to "tid" */
    timer = GsFindTimer (tid);
    if (timer == NULL) {
		SERVER_UNLOCK();
        return;
	}

    if (tid == cache_timer_id) 
    {
        cache_timer_id = 0;
        cache_timer = NULL;
    }

    /* Delete the timer from the device independent engine layer */
    GdDestroyTimer (timer->timer);

    /* Pull the timer out of the servers list */
    if (list_timer == timer)
    {
        list_timer = timer->next;
    }
    else 
    {
        prev_timer = list_timer;
        while (prev_timer->next != timer)
        {
            prev_timer = prev_timer->next;
        }
        prev_timer->next = timer->next;
    }
    free (timer);

	SERVER_UNLOCK();
}
#endif /* MW_FEATURE_TIMERS */


/**
 * Sets new server portrait mode and redraws all windows.
 *
 * @param portraitmode New portrait mode.
 */
void
GrSetPortraitMode(int portraitmode)
{
	SERVER_LOCK();
	GsSetPortraitMode(portraitmode);
	SERVER_UNLOCK();
}

/**
 * Returns the current information for the pointer
 *
 * @param mwin	Window the mouse is current in
 * @param x	Current X pos of mouse (from root)
 * @param y	Current Y pos of mouse (from root)
 * @param bmask Current button mask
 */
void 
GrQueryPointer(GR_WINDOW_ID *mwin, GR_COORD *x, GR_COORD *y, GR_BUTTON *bmask)
{
	SERVER_LOCK();
	*mwin = mousewp->id;
	*x = cursorx;
	*y = cursory;
	*bmask = curbuttons;
	SERVER_UNLOCK();
}

/**
 * Creates a new region from a bitmap mask.
 *
 * @param bitmap	monochrome source bitmap
 * @param width		width of source bitmap
 * @param height	height of source bitmap
 */
GR_REGION_ID
GrNewBitmapRegion(GR_BITMAP *bitmap, GR_SIZE width, GR_SIZE height)
{
#if DYNAMICREGIONS
	GR_REGION *regionp;
	GR_REGION_ID id;

	SERVER_LOCK();

	if (!(regionp = malloc(sizeof(GR_REGION)))) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return 0;
	}

	regionp->rgn = GdAllocBitmapRegion(bitmap, width, height);
	regionp->id = nextregionid++;
	regionp->owner = curclient;
	regionp->next = listregionp;

	listregionp = regionp;
	id = regionp->id;
	
	SERVER_UNLOCK();
	
	return id;
#else
	return 0;
#endif
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
 * @param wid		window id
 * @param rid		region id
 * @param type	region type, bounding or clip mask
 */
void
GrSetWindowRegion(GR_WINDOW_ID wid, GR_REGION_ID rid, int type)
{
#if DYNAMICREGIONS
	GR_WINDOW *wp;
	GR_REGION *region;
	MWCLIPREGION *newregion;

	SERVER_LOCK();

	/*FIXME clip window region not yet implemented*/

	if (!(wp = GsFindWindow(wid))) {
		GsError(GR_ERROR_BAD_WINDOW_ID, wid);
		SERVER_UNLOCK();
		return;
	}

	if (rid) {
		if (!(region = GsFindRegion(rid))) {
			GsError(GR_ERROR_BAD_REGION_ID, rid);
			SERVER_UNLOCK();
			return;
		}

		if (!(newregion = GdAllocRegion())) {
			GsError(GR_ERROR_MALLOC_FAILED, 0);
			SERVER_UNLOCK();
			return;
		}

		GdCopyRegion(newregion, region->rgn);
	} else newregion = NULL;

	if (wp->clipregion)
		GdDestroyRegion(wp->clipregion);
	wp->clipregion = newregion;

	SERVER_UNLOCK();
#endif
}

/**
 * This passes transform data to the mouse input engine.
 *
 * @param trans the GR_TRANSFORM structure that contains the transform data
 * for the filter, or NULL for raw mode.
 */
void
GrSetTransform(GR_TRANSFORM *trans)
{
	SERVER_LOCK();
	GdSetTransform(trans);
	SERVER_UNLOCK();
}

/**
 * Grab a key for a specific window.
 *
 * @param id Window to send event to.
 * @param key MWKEY value.
 * @param type The type of reservation to make.  Valid values are
 *             #GR_GRAB_HOTKEY_EXCLUSIVE,
 *             #GR_GRAB_HOTKEY, 
 *             #GR_GRAB_EXCLUSIVE and
 *             #GR_GRAB_EXCLUSIVE_MOUSE.
 * @return #GR_TRUE on success, #GR_FALSE on error.
 */
GR_BOOL
GrGrabKey(GR_WINDOW_ID id, GR_KEY key, int type)
{
	GR_GRABBED_KEY *keygrab;
	GR_GRABBED_KEY *last_keygrab;

	if ((unsigned)type > GR_GRAB_MAX)
		return GR_FALSE;

	SERVER_LOCK();

	/* Check we can do the grab. Look for previous grabs on the same key. */
	last_keygrab = NULL;
	for (keygrab = list_grabbed_keys; keygrab != NULL; 
	     keygrab = keygrab->next) {
	     	last_keygrab = keygrab; /* Save the last non-NULL pointer */
		if (keygrab->key == key) {
			if ((keygrab->wid == id)
			 && (keygrab->type == type)
			 && (keygrab->owner == curclient)) {
				/*
				 * Already there.  Success.
				 */
				SERVER_UNLOCK();
				return GR_TRUE;
			}
			else if ((type != GR_GRAB_HOTKEY)
			      && (keygrab->type != GR_GRAB_HOTKEY)) {
				/*
				 * Attempting to have two exclusive
				 * reservations.  Fail.
				 */
				SERVER_UNLOCK();
				return GR_FALSE;
			}
		}
	}

	/* Create a GR_GRABBED_KEY */
	keygrab = (GR_GRABBED_KEY *)malloc(sizeof(GR_GRABBED_KEY));
	if (keygrab == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		SERVER_UNLOCK();
		return GR_FALSE;
	}

	/* Fill in the structure. */
	keygrab->key = key;
	keygrab->wid = id;
	keygrab->type = type;
	keygrab->owner = curclient;
	
	/* 
	 * Link into the servers list.  We must have all GR_GRAB_HOTKEY
	 * events for a particular key after any other grabs for that
	 * key.  We accomplish this simply by adding GR_GRAB_HOTKEY
	 * grabs to the end of the list, and all other types to the
	 * beginning.
	 */
	if ((type != GR_GRAB_HOTKEY) || (last_keygrab == NULL)) {
		/* Add to start of list (or list is empty). */
		keygrab->next = list_grabbed_keys;
		list_grabbed_keys = keygrab;
	} else {
		/* Add to end of list. */
		keygrab->next = NULL;
		last_keygrab->next = keygrab;
	}

	SERVER_UNLOCK();
	return GR_TRUE;
}

/**
 * Ungrab a key for a specific window.
 *
 * @param id window to stop key grab.
 * @param key MWKEY value.
 */
void
GrUngrabKey(GR_WINDOW_ID id, GR_KEY key)
{
	GR_GRABBED_KEY *keygrab;
	GR_GRABBED_KEY **keygrab_prev_next;

	SERVER_LOCK();

	keygrab_prev_next = &list_grabbed_keys;
	keygrab           =  list_grabbed_keys;
	while (keygrab != NULL) {
		if ((keygrab->key == key) && (keygrab->wid == id) &&
		    (keygrab->owner == curclient)) {
			*keygrab_prev_next = keygrab->next;
			free(keygrab);
			SERVER_UNLOCK();
			return;
		}
		keygrab_prev_next = &keygrab->next;
		keygrab           =  keygrab->next;
	}

	SERVER_UNLOCK();
}
