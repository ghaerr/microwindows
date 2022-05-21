/*
 * Copyright (c) 2000, 2002, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 1991 David I. Bell
 *
 * Graphics server utility routines for windows.
 */
#include <stdio.h>
#include <stdlib.h>
#include "uni_std.h"
#include "serv.h"
#include "../drivers/fb.h"	/* for set_data_formatex()*/
#if HAVE_MMAP
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

/*
 * Redraw the screen completely.
 */
void
GsRedrawScreen(void)
{
	/* Redraw all windows*/
	GsExposeArea(rootwp, 0, 0, rootwp->width, rootwp->height, NULL);
}

/*
 * Activate Screen Saver.
 */
void
GsActivateScreenSaver(void *arg)
{
	screensaver_active = GR_TRUE;
	GsDeliverScreenSaverEvent(GR_TRUE);
}

/*
 * Deactivate screen saver and reset timer if active.
 */
void
GsResetScreenSaver(void)
{
	if(screensaver_active == GR_TRUE) {
		screensaver_active = GR_FALSE;
		GsDeliverScreenSaverEvent(GR_FALSE);
	}
#if MW_FEATURE_TIMERS
	if(screensaver_delay) {
		MWTIMER *timer;

		if((timer = GdFindTimer(GsActivateScreenSaver)))
			GdDestroyTimer(timer);

		GdAddTimer(screensaver_delay, GsActivateScreenSaver, GsActivateScreenSaver);
	}
#endif
}

/*
 * Unmap the window to make it and its children invisible on the screen.
 * This is a recursive routine which unrealizes this window and all of its
 * children, and causes exposure events for windows which are newly uncovered.
 * If temp_unmap set, don't reset focus or generate mouse/focus events,
 * as window will be mapped again momentarily (window move, resize, etc)
 */
void
GsUnrealizeWindow(GR_WINDOW *wp, GR_BOOL temp_unmap)
{
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*sibwp;		/* sibling window */
	GR_WINDOW	*childwp;	/* child window */
	GR_SIZE		bs;		/* border size of this window */

	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wp->id);
		return;
	}

	if (wp == clipwp)
		clipwp = NULL;

	/* if window isn't realized, nothing to do*/
	if (!wp->realized)
		return;

	/* set window invisible flag*/
	wp->realized = GR_FALSE;

	for (childwp = wp->children; childwp; childwp = childwp->siblings)
		GsUnrealizeWindow(childwp, temp_unmap);

	if (!temp_unmap && wp == mousewp) {
		GsCheckMouseWindow();
		GsCheckCursor();
	}

	if (!temp_unmap && wp == focuswp) {
		if (focusfixed)
			/* don't revert to mouse enter/leave focus if fixed*/
			focuswp = rootwp;
		else {
			focusfixed = GR_FALSE;
			GsCheckFocusWindow();
		}
	}

	/* Send unmap update event*/
	GsDeliverUpdateEvent(wp, (temp_unmap? GR_UPDATE_UNMAPTEMP: GR_UPDATE_UNMAP), 0, 0, 0, 0);

	/*
	 * If this is an input-only window or the parent window is
	 * still unrealized, then we are all done.
	 */
	if (!wp->parent->realized || !wp->output)
		return;

	/*
	 * Clear the area in the parent for this window, causing an
	 * exposure event for it.  Take into account the border size.
	 */
	bs = wp->bordersize;
	pwp = wp->parent;
	GsClearWindow(pwp, wp->x - pwp->x - bs, wp->y - pwp->y - bs,
		wp->width + bs * 2, wp->height + bs * 2, 1);

	/*
	 * Finally clear and redraw all parts of our lower sibling
	 * windows that were covered by this window.
	 */
	sibwp = wp;
	while (sibwp->siblings) {
		sibwp = sibwp->siblings;
		GsExposeArea(sibwp, wp->x - bs, wp->y - bs,
			wp->width + bs * 2, wp->height + bs * 2, NULL);
	}
}

/*
 * Map the window to possibly make it and its children visible on the screen.
 * This is a recursive routine which realizes this window and all of its
 * children, and causes exposure events for those windows which become visible.
 * If temp is set, then window is being mapped again after a temporary
 * unmap, so don't reset focus or generate mouse/focus events.
 */
void
GsRealizeWindow(GR_WINDOW *wp, GR_BOOL temp)
{
	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wp->id);
		return;
	}
/*printf("RealizeWindow %d, map %d realized %d, parent_realized %d\n",
wp->id, wp->mapped, wp->realized, wp->parent->realized);*/

#define OLDWAY 0
#if OLDWAY /* old way, doesn't quite work with unmap/map yourself*/
	/* 
	 * If window is already realized, or if window
	 * isn't set to be mapped, or parent isn't
	 * realized, then we're done
	 */
	if (wp->realized || !wp->mapped || !wp->parent->realized)
		return;

#else /* new way, still small bug with xfreecell and popup windows*/
	/* if window is already realized, we're done*/
	if (wp->realized)
		return;

	/* 
	 * Send map update event for window manager or others
	 */
	/* send map update event if not temp unmap/map*/
	if (!temp) {
		GsDeliverUpdateEvent(wp, GR_UPDATE_MAP, wp->x, wp->y,
			wp->width, wp->height);
	}

	/* 
	 * If window isn't set to be mapped, or parent isn't
	 * realized, then we're done
	 */
	if (!wp->mapped || !wp->parent->realized)
		return;
#endif

	/* set window visible flag*/
	wp->realized = GR_TRUE;

	if (!temp) {
		GsCheckMouseWindow();
		GsCheckFocusWindow();
		GsCheckCursor();
	}

#if OLDWAY
	/* send map update event if not temp unmap/map*/
	if (!temp) {
		GsDeliverUpdateEvent(wp, GR_UPDATE_MAP, wp->x, wp->y,
			wp->width, wp->height);
	}
#endif
	/*
	 * If the window is an output window, then draw its border, 
	 * clear it to the background color, and generate an exposure event.
	 */
	if (wp->output) {
		GsDrawBorder(wp);
		GsClearWindow(wp, 0, 0, wp->width, wp->height, 1);
	}

	/*
	 * Do the same thing for the children.
	 */
	for (wp = wp->children; wp; wp = wp->siblings)
		GsRealizeWindow(wp, temp);
}

/*
 * Destroy the specified window, and all of its children.
 * This is a recursive routine.
 */
void
GsDestroyWindow(GR_WINDOW *wp)
{
	GR_WINDOW	*prevwp;	/* previous window pointer */
	GR_EVENT_CLIENT	*ecp;		/* selections for window */
	GR_WINDOW_ID	oldwid;		/* old selection owner */
	GR_GRABBED_KEY *keygrab;
	GR_GRABBED_KEY **keygrab_prev_next;

	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wp->id);
		return;
	}

	/* Disable selection if this window is the owner */
	if(selection_owner.wid == wp->id) {
		oldwid = selection_owner.wid;
		selection_owner.wid = 0;
		if(selection_owner.typelist)
			free(selection_owner.typelist);
		selection_owner.typelist = NULL;
		GsDeliverSelectionChangedEvent(oldwid, 0);
	}

	/*
	 * Unmap the window first.
	 */
	if (wp->realized)
		GsUnrealizeWindow(wp, GR_FALSE);

	/* send destroy update event*/
	GsDeliverUpdateEvent(wp, GR_UPDATE_DESTROY, wp->x, wp->y,
		wp->width, wp->height);

	/*
	 * Destroy all children.
	 */
	while (wp->children)
		GsDestroyWindow(wp->children);

	/*
	 * Free all client selection structures.
	 */
	while (wp->eventclients) {
		ecp = wp->eventclients;
		wp->eventclients = ecp->next;
		free(ecp);
	}

	/*
	 * Remove this window from the child list of its parent.
	 */
	prevwp = wp->parent->children;
	if (prevwp == wp)
		wp->parent->children = wp->siblings;
	else {
		while (prevwp->siblings != wp)
			prevwp = prevwp->siblings;
		prevwp->siblings = wp->siblings;
	}
	wp->siblings = NULL;

	/*
	 * Remove this window from the complete list of windows.
	 */
	prevwp = listwp;
	if (prevwp == wp)
		listwp = wp->next;
	else {
		while (prevwp->next != wp)
			prevwp = prevwp->next;
		prevwp->next = wp->next;
	}
	wp->next = NULL;

	/*
	 * Forget various information if they related to this window.
	 * Then finally free the structure.
	 */
	if (wp == clipwp)
		clipwp = NULL;
	if (wp == grabbuttonwp)
		grabbuttonwp = NULL;
	if (wp == cachewp) {
		cachewindowid = 0;
		cachewp = NULL;
	}
	if (wp == focuswp) {
		/* don't revert to mouse enter/leave focus if fixed*/
		/*focusfixed = GR_FALSE;*/
		focuswp = rootwp;
	}

	GsCheckMouseWindow();

	/*
	 * Free title, pixmaps and clipregions associated with window.
	 */
	if (wp->title)
		free(wp->title);
	if (wp->bgpixmap)
		GsDestroyPixmap(wp->bgpixmap);
	if (wp->buffer)
		GsFreeWindowBuffer(wp);
#if DYNAMICREGIONS
	if (wp->clipregion)
		GdDestroyRegion(wp->clipregion);
#endif

	/* Remove any grabbed keys for this window. */
	keygrab_prev_next = &list_grabbed_keys;
	keygrab           =  list_grabbed_keys;
	while (keygrab != NULL) {
		if (keygrab->wid == wp->id){
			/* Delete keygrab. */
			*keygrab_prev_next = keygrab->next;
			free(keygrab);
			keygrab = *keygrab_prev_next;
		} else {
			keygrab_prev_next = &keygrab->next;
			keygrab           =  keygrab->next;
		}
	}

	free(wp);
}

/* Destroy a pixmap*/
void
GsDestroyPixmap(GR_PIXMAP *pp)
{
	GR_PIXMAP	*prevpp;
	PSD			psd = pp->psd;

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

/*
 * Draw a window's background pixmap.
 *
 * The flags mean:
 *   GR_BACKGROUND_TILE- tile the pixmap across the window (default).
 *   GR_BACKGROUND_TOPLEFT- draw the pixmap at (0,0) relative to the window.
 *   GR_BACKGROUND_CENTER- draw the pixmap in the middle of the window.
 *   GR_BACKGROUND_STRETCH- stretch the pixmap within the window.
 *   GR_BACKGROUND_TRANS- if the pixmap is smaller than the window and not
 *     using tile mode, there will be gaps around the pixmap. This flag causes
 *     to not fill in the spaces with the background colour.
 */
void
GsDrawBackgroundPixmap(GR_WINDOW *wp, GR_PIXMAP *pm, GR_COORD x,
	GR_COORD y, GR_SIZE width, GR_SIZE height)
{
	GR_SIZE destwidth, destheight, fillwidth, fillheight, pmwidth, pmheight;
	GR_COORD fromx, fromy, destx, desty, pixmapx = 0, pixmapy = 0;

	if(wp->bgpixmapflags & GR_BACKGROUND_STRETCH) {
		/* must use whole window coords or stretch will have incorrect ratios*/
		GdStretchBlit(wp->psd, wp->x, wp->y, wp->x + wp->width, wp->y + wp->height,
			pm->psd, 0, 0, pm->width - 1, pm->height - 1, MWROP_SRC_OVER);
		return;
	}

	if(wp->bgpixmapflags == GR_BACKGROUND_TILE) {
		GsTileBackgroundPixmap(wp, pm, x, y, width, height);
		return;
	}

	if(wp->bgpixmapflags & GR_BACKGROUND_CENTER) {
		if (pm->width < wp->width) pixmapx = (wp->width - pm->width) / 2;
		if (pm->height < wp->height) pixmapy = (wp->height - pm->height) / 2;
	}

	/* topleft & center calcs*/
	pmwidth = MWMIN(pm->width, wp->width);
	pmheight = MWMIN(pm->height, wp->height);

	if(x > pixmapx) {
		destx = x;
		fromx = x - pixmapx;
		destwidth = pixmapx + pmwidth - x;
	} else {
		destx = pixmapx;
		fromx = 0;
		destwidth = x + width - pixmapx;
	}

	if(y > pixmapy) {
		desty = y;
		fromy = y - pixmapy;
		destheight = pixmapy + pmheight - desty;
	} else {
		desty = pixmapy;
		fromy = 0;
		destheight = y + height - pixmapy;
	}

	if(destwidth > 0 && destheight > 0) {
		destwidth = MWMIN(width, destwidth);
		destheight = MWMIN(height, destheight);

		GdBlit(wp->psd, wp->x + destx, wp->y + desty, destwidth, destheight,
			pm->psd, fromx, fromy, MWROP_SRC_OVER);
	}

	if(wp->bgpixmapflags & GR_BACKGROUND_TRANS)
		return;

	/* Fill in the gaps around the pixmap */
	if(x < pixmapx) {
		fillwidth = pixmapx - x;
		if(fillwidth > width) fillwidth = width;
		fillheight = height;

		GdFillRect(wp->psd, wp->x + x, wp->y + y, fillwidth, fillheight);
	}
	if(x + width > pixmapx + pmwidth) {
		fillwidth = (x + width) - (pixmapx + pmwidth);
		if(fillwidth > width) fillwidth = width;
		fillheight = height;

		if(x < pixmapx + pmwidth)
			destx = pixmapx + pmwidth + wp->x;
		else destx = x + wp->x;

		GdFillRect(wp->psd, destx, wp->y + y, fillwidth, fillheight);
	}
	if(y < pixmapy) {
		fillheight = pixmapy - y;
		if(fillheight > height) fillheight = height;

		if(x < pixmapx)
			destx = pixmapx + wp->x;
		else destx = x + wp->x;
		if(x + width > pixmapx + pmwidth)
			fillwidth = pixmapx + pmwidth - destx;
		else fillwidth = x + width - destx;

		if(fillwidth > 0 && fillheight > 0)
			GdFillRect(wp->psd, destx, wp->y + y, fillwidth, fillheight);
	}
	if(y + height > pixmapy + pmheight) {
		fillheight = (y + height) - (pixmapy + pmheight);
		if(fillheight > height) fillheight = height;

		if(x < pixmapx)
			destx = pixmapx + wp->x;
		else destx = x + wp->x;
		if(y < pixmapy + pmheight)
			desty = pixmapy + pmheight + wp->y;
		else desty = y + wp->y;
	
		if(x + width > pixmapx + pmwidth)
			fillwidth = pixmapx + pmwidth - destx;
		else fillwidth = x + width - destx;
		if(fillwidth > 0 && fillheight > 0)
			GdFillRect(wp->psd, destx, desty, fillwidth, fillheight);
	}
}

/*
 * Draw a tiled pixmap window background.
 */
void 
GsTileBackgroundPixmap(GR_WINDOW *wp, GR_PIXMAP *pm, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height)
{
	GR_COORD tilex = 0, tiley = 0, fromx, fromy, cx, cy;
	GR_SIZE destwidth, destheight, pmwidth, pmheight, cwidth, cheight;

	if(pm->width > wp->width)
		pmwidth = wp->width;
	else pmwidth = pm->width;
	if(pm->height > wp->height)
		pmheight = wp->height;
	else pmheight = pm->height;

	for(; tiley < wp->height; tiley += pmheight, tilex = 0) {
		if(tiley > y + height)
			continue;
		if(y > tiley + pmheight)
			continue;

		if(tiley + pmheight > wp->height)
			destheight = wp->height - tiley;
		else destheight = pmheight;

		for(; tilex < wp->width; tilex += pmwidth) {
			if(tilex > x + width)
				continue;
			if(x > tilex + pmwidth)
				continue;

			if(tilex + pmwidth > wp->width)
				destwidth = wp->width - tilex;
			else destwidth = pmwidth;

			if(tilex >= x && (tilex + destwidth <= x + width)) {
				fromx = 0;
				cx = tilex + wp->x;
				cwidth = destwidth;
			} else {
				if(x > tilex) {
					fromx = x - tilex;
					cwidth = destwidth - fromx;
				} else {
					fromx = 0;
					cwidth = x + width - tilex;
				}
				if(cwidth > width) cwidth = width;
				if(cwidth > destwidth) cwidth = destwidth;
				cx = wp->x + tilex + fromx;
			}

			if(tiley >= y && (tiley + destheight <= y + height)) {
				fromy = 0;
				cy = tiley + wp->y;
				cheight = destheight;
			} else {
				if(y > tiley) {
					fromy = y - tiley;
					cheight = destheight - fromy;
				} else {
					fromy = 0;
					cheight = y + height - tiley;
				}
				if(cwidth > width) cwidth = width;
				if(cheight > destheight) cheight = destheight;
				cy = wp->y + tiley + fromy;
			}

			if(cwidth > 0 && cheight > 0)
				GdBlit(wp->psd, cx, cy, cwidth, cheight, pm->psd, fromx, fromy, MWROP_SRC_OVER);
		}
	}
}

/* init buffered windows by allocating pixmap buffer and clearing background*/
void
GsInitWindowBuffer(GR_WINDOW *wp, GR_SIZE width, GR_SIZE height)
{
	/* create same size RGBA pixmap for buffer*/
	GR_WINDOW_ID id;
	int data_format;
	
	/* check if buffer size changed*/
	if (wp->buffer) {
		if (wp->width == width && wp->height == height)
			return;
		GsFreeWindowBuffer(wp);
	}

	/* set window buffer pixel type*/
	if (wp->props & GR_WM_PROPS_BUFFER_BGRA)
		data_format = MWIF_BGRA8888;
	else if (wp->props & GR_WM_PROPS_BUFFER_MWPF)
		data_format = set_data_formatex(scrdev.pixtype, scrdev.bpp);
	else data_format = MWIF_RGBA8888;

	id = GsNewPixmap(width, height, data_format, NULL);
	wp->buffer = GsFindPixmap(id);
	if (!wp->buffer) {
		wp->props &= ~(GR_WM_PROPS_BUFFERED | GR_WM_PROPS_DRAWING_DONE);
		return;
	}

#if HAVE_MMAP
	/* convert window buffer to mmap'd file if requested*/
	if (wp->props & GR_WM_PROPS_BUFFER_MMAP) {
		int fd;
		PSD psd = wp->buffer->psd;
		void *addr;
		char path[256];

		/* buffer mmap'd to /tmp/.nano-fb{window id}*/
		sprintf(path, MW_PATH_BUFFER_MMAP, wp->id);
		/* create file and mmap*/
		if ((fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0666)) >= 0) {
			lseek(fd, psd->size - 1, SEEK_SET);
			write(fd, "", 1);
			addr = mmap(NULL, psd->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			close(fd);
			if (addr != (char *)-1) {
				free(psd->addr);
				psd->addr = addr;
				psd->flags &= ~PSF_ADDRMALLOC;
				psd->flags |= PSF_ADDRMMAP;
			} else DPRINTF("Window buffer mmap failed\n");
		} else DPRINTF("Window buffer create file failed: %s\n", path);
	}
#endif /* HAVE_MMAP*/

	/* mark buffer as not ready for display*/
	wp->props &= ~GR_WM_PROPS_DRAWING_DONE;

	/* clear buffer to background color*/
	if (!(wp->props & GR_WM_PROPS_NOBACKGROUND)) {
		GR_PIXMAP *pp = wp->buffer;

		/* clip to pixmap boundaries*/
#if DYNAMICREGIONS
		GdSetClipRegion(pp->psd, GdAllocRectRegion(0, 0, pp->psd->xvirtres, pp->psd->yvirtres));
#else
		MWCLIPRECT	cliprect;
		cliprect.x = 0;
		cliprect.y = 0;
		cliprect.width = pp->psd->xvirtres;
		cliprect.height = pp->psd->yvirtres;
		GdSetClipRects(pp->psd, 1, &cliprect);
#endif
		clipwp = NULL;			/* reset clip cache for next window draw*/
		curgcp = NULL;			/* invalidate gc cache since we're changing color and mode*/
		GdSetFillMode(GR_FILL_SOLID);
		GdSetMode(GR_MODE_COPY);
		GdSetForegroundColor(pp->psd, wp->background);

		GdFillRect(pp->psd, 0, 0, pp->width, pp->height);
	}
}

/* deallocate window buffer and unmap if mmaped*/
void
GsFreeWindowBuffer(GR_WINDOW *wp)
{
	if (!wp->buffer)
		return;
#if HAVE_MMAP
	/* destroy mmap file*/
	if (wp->props & GR_WM_PROPS_BUFFER_MMAP) {
		PSD psd = wp->buffer->psd;
		char path[256];

		if (psd->flags & PSF_ADDRMMAP) {
			munmap(psd->addr, psd->size);
			psd->flags &= ~PSF_ADDRMMAP;
			sprintf(path, MW_PATH_BUFFER_MMAP, wp->id);
			unlink(path);
		}
	}
#endif
	GsDestroyPixmap(wp->buffer);
	wp->buffer = NULL;
}

/*
 * Clear the specified area of a window and possibly make an exposure event.
 * This sets the area window to its background color or pixmap.  If the
 * exposeflag is 1, then this also creates an exposure event for the window.
 * For buffered windows, mark drawing finalized and draw if
 * exposeflag = 2.
 */
void
GsClearWindow(GR_WINDOW *wp, GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE  height, int exposeflag)
{
	if (!wp->realized || !wp->output)
		return;

	/*
	 * Reduce the arguments so that they actually lie within the window.
	 */
	if (x < 0) {
		width += x;
		x = 0;
	}
	if (y < 0) {
		height += y;
		y = 0;
	}
	if (x + width > wp->width)
		width = wp->width - x;
	if (y + height > wp->height)
		height = wp->height - y;

	/*
	 * Now see if the region is really in the window.  If not, then
	 * do nothing.
	 */
	if (x >= wp->width || y >= wp->height || width <= 0 || height <= 0)
		return;

	/*
	 * Buffered window drawing. First check if drawing finalized and
	 * set flag.  Physical window background erase is never performed
	 * with buffered windows, all drawing is postponed until the application
	 * is finished by calling GrClearWindow(..., 2), ie. GrFlushWindow()
	 */
	if (exposeflag == 2)
		wp->props |= GR_WM_PROPS_DRAWING_DONE;

	if (wp->props & GR_WM_PROPS_BUFFERED) {

		/* nothing to do until drawing finalized*/
		if (!(wp->props & GR_WM_PROPS_DRAWING_DONE))
			return;

		/* prepare clipping to window boundaries*/
		GsSetClipWindow(wp, NULL, 0);
		clipwp = NULL;		/* reset clip cache since no user regions used*/

#if DEBUG_EXPOSE
curgcp = NULL;
GdSetFillMode(GR_FILL_SOLID);
GdSetMode(GR_MODE_COPY);
GdSetForegroundColor(wp->psd, MWRGB(255,255,0)); /* yellow*/
GdFillRect(wp->psd, wp->x+x, wp->y+y, width, height);
usleep(500000);
#endif

		/* copy window pixmap buffer to window*/
		GdBlit(wp->psd, wp->x + x, wp->y + y, width, height, wp->buffer->psd, x, y, MWROP_COPY);
		return;				/* don't deliver exposure events*/
	}

	/*
	 * Unbuffered window: erase background unless nobackground flag set
	 */
	if (!(wp->props & GR_WM_PROPS_NOBACKGROUND)) {
		/* perhaps find a better way of determining whether pixmap needs src_over*/
		int hasalpha = wp->bgpixmap && (wp->bgpixmap->psd->data_format & MWIF_HASALPHA);

		/*
	 	 * Draw the background of the window.
	 	 * Invalidate the current graphics context since
	 	 * we are changing the foreground color and mode.
	 	 */
		GsSetClipWindow(wp, NULL, 0);
		clipwp = NULL;		/* reset clip cache since no user regions used*/

#if DEBUG_EXPOSE
GdSetFillMode(GR_FILL_SOLID);
GdSetMode(GR_MODE_COPY);
GdSetForegroundColor(wp->psd, MWRGB(255,255,0)); /* yellow*/
GdFillRect(wp->psd, wp->x+x, wp->y+y, width, height);
usleep(500000);
#endif

		curgcp = NULL;
		GdSetFillMode(GR_FILL_SOLID);
		GdSetMode(GR_MODE_COPY);
		GdSetForegroundColor(wp->psd, wp->background);

		/* if background pixmap w/alpha channel and stretchblit, fill entire (clipped) window*/
		if (hasalpha && wp->bgpixmapflags == GR_BACKGROUND_STRETCH)
				GdFillRect(wp->psd, wp->x, wp->y, wp->width, wp->height);
		else /* if no pixmap background clear exposed area*/
			if (!wp->bgpixmap || hasalpha)	/* FIXME will flash with pixmap, should check src_over*/
				if (!(wp->bgpixmapflags & GR_BACKGROUND_TRANS))
					GdFillRect(wp->psd, wp->x + x, wp->y + y, width, height);

		if (wp->bgpixmap)
			GsDrawBackgroundPixmap(wp, wp->bgpixmap, x, y, width, height);
	}

	/*
	 * Do the exposure if required for unbuffered windows.
	 */
	if (exposeflag)
		GsDeliverExposureEvent(wp, x, y, width, height);
}

/*
 * Handle the exposing of the specified absolute region of the screen,
 * starting with the specified window.  That window and all of its
 * children will be redrawn and/or exposure events generated if they
 * overlap the specified area.  This is a recursive routine.
 */
void
GsExposeArea(GR_WINDOW *wp, GR_COORD rootx, GR_COORD rooty, GR_SIZE width,
	GR_SIZE height, GR_WINDOW *stopwp)
{
	if (!wp->realized || wp == stopwp || !wp->output)
		return;

	/*
	 * First see if the area overlaps the window including the border.
	 * If not, then there is nothing more to do.
	 */
	if ((rootx >= wp->x + wp->width + wp->bordersize) ||
		(rooty >= wp->y + wp->height + wp->bordersize) ||
		(rootx + width <= wp->x - wp->bordersize) ||
		(rooty + height <= wp->y - wp->bordersize))
			return;

	/*
	 * The area does overlap the window.  See if the area overlaps
	 * the border, and if so, then redraw it.
	 */
	if (rootx < wp->x || rooty < wp->y ||
		(rootx + width > wp->x + wp->width) ||
		(rooty + height > wp->y + wp->height))
			GsDrawBorder(wp);

	/*
	 * Now clear the window itself in the specified area,
	 * which might cause an exposure event.
	 */
	GsClearWindow(wp, rootx - wp->x, rooty - wp->y, width, height, 1);

	/*
	 * Now do the same for all the children.
	 */
	for (wp = wp->children; wp; wp = wp->siblings)
		GsExposeArea(wp, rootx, rooty, width, height, stopwp);
}

/*
 * Draw the border of a window if there is one.
 * Note: To allow the border to be drawn with the correct clipping,
 * we temporarily grow the size of the window to include the border.
 */
void
GsDrawBorder(GR_WINDOW *wp)
{
	GR_COORD	lminx;		/* left edge minimum x */
	GR_COORD	rminx;		/* right edge minimum x */
	GR_COORD	tminy;		/* top edge minimum y */
	GR_COORD	bminy;		/* bottom edge minimum y */
	GR_COORD	topy;		/* top y value of window */
	GR_COORD	boty;		/* bottom y value of window */
	GR_SIZE		width;		/* original width of window */
	GR_SIZE		height;		/* original height of window */
	GR_SIZE		bs;		/* border size */

	bs = wp->bordersize;
	if (bs <= 0)
		return;

	width = wp->width;
	height = wp->height;
	lminx = wp->x - bs;
	rminx = wp->x + width;
	tminy = wp->y - bs;
	bminy = wp->y + height;
	topy = wp->y;
	boty = bminy - 1;
 
	wp->x -= bs;
	wp->y -= bs;
	wp->width += (bs * 2);
	wp->height += (bs * 2);
	wp->bordersize = 0;

	clipwp = NULL;
	/* FIXME: window clipregion will fail here */
	GsSetClipWindow(wp, NULL, 0);
	curgcp = NULL;
	GdSetMode(GR_MODE_COPY);
	GdSetForegroundColor(wp->psd, wp->bordercolor);
	GdSetDash(0, 0);
	GdSetFillMode(GR_FILL_SOLID);

	if (bs == 1) {
		GdLine(wp->psd, lminx, tminy, rminx, tminy, TRUE);
		GdLine(wp->psd, lminx, bminy, rminx, bminy, TRUE);
		GdLine(wp->psd, lminx, topy, lminx, boty, TRUE);
		GdLine(wp->psd, rminx, topy, rminx, boty, TRUE);
	} else {
		GdFillRect(wp->psd, lminx, tminy, width + bs * 2, bs);
		GdFillRect(wp->psd, lminx, bminy, width + bs * 2, bs);
		GdFillRect(wp->psd, lminx, topy, bs, height);
		GdFillRect(wp->psd, rminx, topy, bs, height);
	}

	/*
	 * Restore the true window size.
	 * Forget the currently clipped window since we messed it up.
	 */
	wp->x += bs;
	wp->y += bs;
	wp->width -= (bs * 2);
	wp->height -= (bs * 2);
	wp->bordersize = bs;
	clipwp = NULL;
}

/*
 * Check to see if the first window overlaps the second window.
 */
GR_BOOL
GsCheckOverlap(GR_WINDOW *topwp, GR_WINDOW *botwp)
{
	GR_COORD	minx1;
	GR_COORD	miny1;
	GR_COORD	maxx1;
	GR_COORD	maxy1;
	GR_COORD	minx2;
	GR_COORD	miny2;
	GR_COORD	maxx2;
	GR_COORD	maxy2;
	GR_SIZE		bs;

	if (!topwp->output || !topwp->realized || !botwp->realized)
		return GR_FALSE;

	bs = topwp->bordersize;
	minx1 = topwp->x - bs;
	miny1 = topwp->y - bs;
	maxx1 = topwp->x + topwp->width + bs - 1;
	maxy1 = topwp->y + topwp->height + bs - 1;

	bs = botwp->bordersize;
	minx2 = botwp->x - bs;
	miny2 = botwp->y - bs;
	maxx2 = botwp->x + botwp->width + bs - 1;
	maxy2 = botwp->y + botwp->height + bs - 1;

	if (minx1 > maxx2 || minx2 > maxx1 || miny1 > maxy2 || miny2 > maxy1)
			return GR_FALSE;

	return GR_TRUE;
}

/*
 * Return a pointer to the window structure with the specified window id.
 * Returns NULL if the window does not exist.
 */
GR_WINDOW *
GsFindWindow(GR_WINDOW_ID id)
{
	GR_WINDOW	*wp;		/* current window pointer */

	/*
	 * See if this is the root window or the same window as last time.
	 */
	if (id == GR_ROOT_WINDOW_ID)
		return rootwp;

	if ((id == cachewindowid) && id)
		return cachewp;

	/*
	 * No, search for it and cache it for future calls.
	 */
	for (wp = listwp; wp; wp = wp->next) {
		if (wp->id == id) {
			cachewindowid = id;
			cachewp = wp;
			return wp;
		}
	}

	return NULL;
}



/*
 * Return a pointer to the pixmap structure with the specified window id.
 * Returns NULL if the pixmap does not exist.
 */
GR_PIXMAP *
GsFindPixmap(GR_WINDOW_ID id)
{
	GR_PIXMAP	*pp;		/* current pixmap pointer */

	if ((id == cachepixmapid) && id)
		return cachepp;

	/*
	 * No, search for it and cache it for future calls.
	 */
	for (pp = listpp; pp; pp = pp->next) {
		if (pp->id == id) {
			cachepixmapid = id;
			cachepp = pp;
			return pp;
		}
	}

	return NULL;
}


/*
 * Return a pointer to the graphics context with the specified id.
 * Returns NULL if the graphics context does not exist, with an
 * error saved.
 */
GR_GC *
GsFindGC(GR_GC_ID gcid)
{
	GR_GC		*gcp;		/* current graphics context pointer */

	/*
	 * See if this is the same graphics context as last time.
	 */
	if ((gcid == cachegcid) && gcid)
		return cachegcp;

	/*
	 * No, search for it and cache it for future calls.
	 */
	for (gcp = listgcp; gcp; gcp = gcp->next) {
		if (gcp->id == gcid) {
			cachegcid = gcid;
			cachegcp = gcp;
			return gcp;
		}
	}

	GsError(GR_ERROR_BAD_GC_ID, gcid);

	return NULL;
}

/* Return a pointer to the region with the specified id.*/
GR_REGION *
GsFindRegion(GR_REGION_ID regionid)
{
	GR_REGION	*regionp;	/* current region pointer */

	for (regionp = listregionp; regionp; regionp = regionp->next) {
		if (regionp->id == regionid) {
			return regionp;
		}
	}

	return NULL;
}

/* find a font with specified id*/
GR_FONT *
GsFindFont(GR_FONT_ID fontid)
{
	GR_FONT		*fontp;

	for (fontp = listfontp; fontp; fontp = fontp->next) {
		if (fontp->id == fontid)
			return fontp;
	}
	return NULL;
}

/* find a cursor with specified id*/
GR_CURSOR *
GsFindCursor(GR_CURSOR_ID cursorid)
{
	GR_CURSOR	*cursorp;

	for (cursorp = listcursorp; cursorp; cursorp = cursorp->next) {
		if (cursorp->id == cursorid)
			return cursorp;
	}
	return NULL;
}

/*
 * Prepare to do drawing in a window or pixmap using the specified
 * graphics context.  Returns the drawable pointer if successful,
 * and the type of drawing id that was supplied.  Returns the special value
 * GR_DRAW_TYPE_NONE if an error is generated, or if drawing is useless.
 */
GR_DRAW_TYPE
GsPrepareDrawing(GR_DRAW_ID id, GR_GC_ID gcid, GR_DRAWABLE **retdp)
{
	GR_WINDOW	*wp;		/* found window */
	GR_PIXMAP	*pp;		/* found pixmap */
	GR_GC		*gcp;		/* found graphics context */
	GR_REGION	*regionp;	/* user clipping region */
	MWCLIPREGION*reg;

	*retdp = NULL;

	gcp = GsFindGC(gcid);
	if (gcp == NULL)
		return GR_DRAW_TYPE_NONE;

	/*
	 * If the graphics context is not the current one, then
	 * make it the current one and remember to update it.
	 */
	if (gcp != curgcp) {
		curgcp = gcp;
		gcp->changed = GR_TRUE;
	}

	/*
	 * Look for window or pixmap id
	 */
	pp = NULL;
	wp = GsFindWindow(id);
	if (wp == NULL) {
		pp = GsFindPixmap(id);
		if (pp == NULL)
				return GR_DRAW_TYPE_NONE;
havepixmap:
#if DYNAMICREGIONS
		reg = GdAllocRectRegion(0, 0, pp->psd->xvirtres, pp->psd->yvirtres);
		/* intersect with user region if any*/
		if (gcp->regionid) {
			regionp = GsFindRegion(gcp->regionid);
			if (regionp) {
				/* handle pixmap offsets*/
				if (gcp->xoff || gcp->yoff) {
					MWCLIPREGION *local = GdAllocRegion();

					GdCopyRegion(local, regionp->rgn);
					GdOffsetRegion(local, gcp->xoff, gcp->yoff);
					GdIntersectRegion(reg, reg, local);
					GdDestroyRegion(local);
				} else
					GdIntersectRegion(reg, reg, regionp->rgn);
			}
		}
		GdSetClipRegion(pp->psd, reg);
#else
		{
			MWCLIPRECT	cliprect;
			/* FIXME: setup pixmap clipping, different from windows*/
	        cliprect.x = 0;
	        cliprect.y = 0;
	        cliprect.width = pp->psd->xvirtres;
	        cliprect.height = pp->psd->yvirtres;
	        GdSetClipRects(pp->psd, 1, &cliprect);
		}
#endif
		/* reset clip cache for next window draw*/
		clipwp = NULL;
	} else {
		if (!wp->output) {
				GsError(GR_ERROR_INPUT_ONLY_WINDOW, id);
				return GR_DRAW_TYPE_NONE;
		}

		/* check if buffered window*/
		if (wp->props & GR_WM_PROPS_BUFFERED) {
			pp = wp->buffer;
			wp = NULL;
			goto havepixmap;		/* draw into pixmap buffer*/
		}

		if (!wp->realized)
				return GR_DRAW_TYPE_NONE;

		/*
		 * If the window is not the currently clipped one,
		 * then make it the current one and define its clip rectangles.
		 */
		if (wp != clipwp || gcp->changed) {
#if DYNAMICREGIONS
			/* find user region for intersect*/
			regionp = gcp->regionid? GsFindRegion(gcp->regionid): NULL;

		 	/* Special handling if user region is not at offset 0,0*/
			if (regionp && (gcp->xoff || gcp->yoff)) {
				MWCLIPREGION *local = GdAllocRegion();

				GdCopyRegion(local, regionp->rgn);
				GdOffsetRegion(local, gcp->xoff, gcp->yoff);

				GsSetClipWindow(wp, local, gcp->mode & ~GR_MODE_DRAWMASK);
				GdDestroyRegion(local);
			} else
				GsSetClipWindow(wp, regionp? regionp->rgn: NULL, gcp->mode & ~GR_MODE_DRAWMASK);
#else
				GsSetClipWindow(wp, NULL, gcp->mode & ~GR_MODE_DRAWMASK);
#endif /* DYNAMICREGIONS*/
		}
	}

	/*
	 * If the graphics context has been changed, then tell the
	 * device driver about it.
	 */
	if (gcp->changed) {
		PSD			psd = (wp ? wp->psd : pp->psd);
		uint32_t	mask;
		int			count;

		if (gcp->linestyle == GR_LINE_SOLID) {
			mask = 0;
			count = 0;
		} else {
			mask = gcp->dashmask;
			count = gcp->dashcount;
		}

		if (gcp->fgispixelval)
			GdSetForegroundPixelVal(psd, gcp->foreground);
		else
			GdSetForegroundColor(psd, gcp->foreground);

		if (gcp->bgispixelval)
			GdSetBackgroundPixelVal(psd, gcp->background);
		else
			GdSetBackgroundColor(psd, gcp->background);

		GdSetMode(gcp->mode & GR_MODE_DRAWMASK);
		GdSetUseBackground(gcp->usebackground);
		
#if MW_FEATURE_SHAPES
		GdSetDash(&mask, &count);
		GdSetFillMode(gcp->fillmode);
		GdSetTSOffset(gcp->ts_offset.x, gcp->ts_offset.y);

		switch(gcp->fillmode) {
		case GR_FILL_STIPPLE:
		case GR_FILL_OPAQUE_STIPPLE:
			GdSetStippleBitmap(gcp->stipple.bitmap, gcp->stipple.width, gcp->stipple.height);
			break;
		case GR_FILL_TILE:
			GdSetTilePixmap(gcp->tile.psd, gcp->tile.width, gcp->tile.height);
			break;
		}
#endif
		gcp->changed = GR_FALSE;
	}

	*retdp = wp? (GR_DRAWABLE *)wp: (GR_DRAWABLE *)pp;
	return wp? GR_DRAW_TYPE_WINDOW: GR_DRAW_TYPE_PIXMAP;
}

/*
 * Prepare the specified window for drawing into it.
 * This sets up the clipping regions to just allow drawing into it.
 * Returns NULL if the drawing is illegal (with an error generated),
 * or if the window is not mapped.
 */
GR_WINDOW *
GsPrepareWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* found window */

	wp = GsFindWindow(wid);
	if (wp == NULL)
		return NULL;
	
	if (!wp->output) {
		GsError(GR_ERROR_INPUT_ONLY_WINDOW, wid);
		return NULL;
	}

	if (!wp->realized)
		return NULL;

	if (wp != clipwp) {
		/* FIXME: no user region clipping here*/
		GsSetClipWindow(wp, NULL, 0);
	}

	return wp;
}

/*
 * Find the window which is currently visible for the specified coordinates.
 * This just walks down the window tree looking for the deepest mapped
 * window which contains the specified point.  If the coordinates are
 * off the screen, the root window is returned.
 */
GR_WINDOW *
GsFindVisibleWindow(GR_COORD x, GR_COORD y)
{
	GR_WINDOW	*wp;		/* current window */
	GR_WINDOW	*retwp;		/* returned window */

	wp = rootwp;
	retwp = wp;
	while (wp) {
		if (wp->realized &&
			((!wp->clipregion && (wp->x <= x) && (wp->y <= y) &&
		     (wp->x + wp->width > x) && (wp->y + wp->height > y))
#if DYNAMICREGIONS
			 || (wp->clipregion && GdPtInRegion(wp->clipregion, x - wp->x, y - wp->y))
#endif
			)) {
				retwp = wp;
				wp = wp->children;
				continue;
		}
		wp = wp->siblings;
	}
	return retwp;
}

/*
 * Check to see if the cursor shape is the correct shape for its current
 * location.  If not, its shape is changed.
 */
void
GsCheckCursor(void)
{
	GR_WINDOW	*wp;		/* window cursor is in */
	GR_CURSOR	*cp = NULL;	/* cursor definition */

	/*
	 * Get the cursor at its current position, and if it is not the
	 * currently defined one, then set the new cursor.  However,
	 * if the pointer is currently grabbed, then leave it alone.
	 */
	wp = grabbuttonwp;
	if (wp == NULL)
		wp = mousewp;

	/* Inherit cursur from parent window(s) if possible*/
	while (wp) {
		if (wp->cursorid) {
			cp = GsFindCursor(wp->cursorid);
			if (cp)
				break;
		}
		wp = wp->parent;
	}

	if (!cp)
		cp = stdcursor;
	if (cp == curcursor)
		return;

	/*
	 * It needs redefining, so do it.
	 */
	curcursor = cp;
	GdMoveCursor(cursorx - cp->cursor.hotx, cursory - cp->cursor.hoty);
	GdSetCursor(&cp->cursor);
}

/*
 * Check to see if the window the mouse is currently in has changed.
 * If so, generate enter and leave events as required.  The newest
 * mouse window is remembered in mousewp.  However, do not change the
 * window while it is grabbed.
 */
void
GsCheckMouseWindow(void)
{
#if 1
	GR_WINDOW *oldwp, *newwp;

	oldwp = grabbuttonwp;
	if (!oldwp)
		oldwp = mousewp;

	newwp = GsFindVisibleWindow(cursorx, cursory);
	if (oldwp != newwp) {
		GsDeliverGeneralEvent(oldwp, GR_EVENT_TYPE_MOUSE_EXIT, NULL);
		GsDeliverGeneralEvent(newwp, GR_EVENT_TYPE_MOUSE_ENTER, NULL);
	}

	mousewp = newwp;
#endif
#if 0
	GR_WINDOW	*wp;		/* newest window for mouse */

	wp = grabbuttonwp;
	if (wp == NULL)
		wp = GsFindVisibleWindow(cursorx, cursory);
	if (wp == mousewp)
		return;

	GsDeliverGeneralEvent(mousewp, GR_EVENT_TYPE_MOUSE_EXIT, NULL);

	mousewp = wp;

	GsDeliverGeneralEvent(wp, GR_EVENT_TYPE_MOUSE_ENTER, NULL);
#endif
}

/*
 * Determine the current focus window for the current mouse coordinates.
 * The mouse coordinates only matter if the focus is not fixed.  Otherwise,
 * the selected window is dependant on the window which wants keyboard
 * events.  This also sets the current focus for that found window.
 * The window with focus is remembered in focuswp.
 */
void
GsCheckFocusWindow(void)
{
	GR_WINDOW		*wp;		/* current window */
	GR_EVENT_CLIENT		*ecp;		/* current event client */

	if (focusfixed)
		return;

	/*
	 * Walk upwards from the current window containing the mouse
	 * looking for the first window which would accept a keyboard event.
	 */
	for (wp = mousewp; ;wp = wp->parent) {
		if (wp->props & GR_WM_PROPS_NOFOCUS)
			continue;
		for (ecp = wp->eventclients; ecp; ecp = ecp->next) {
			if (ecp->eventmask & GR_EVENT_MASK_KEY_DOWN) {
				GsSetFocus(wp);
				return;
			}
		}
		if ((wp == rootwp) || (wp->nopropmask & GR_EVENT_MASK_KEY_DOWN)) {
			GsSetFocus(rootwp);
			return;
		}
	}
}

/* Send an update activate event to top level window of passed window*/
void
GsNotifyActivate(GR_WINDOW *wp)
{
	GR_WINDOW	*pwp;

	for (pwp=wp; pwp->parent; pwp=pwp->parent)
		if (pwp->parent->id == GR_ROOT_WINDOW_ID)
			break;
	if (pwp->id != GR_ROOT_WINDOW_ID)
		GsDeliverUpdateEvent(pwp, GR_UPDATE_ACTIVATE, wp->x, wp->y,
			wp->width, wp->height);
}

/*
 * Set the input focus to the specified window.
 * This generates focus out and focus in events as necessary.
 */
void
GsSetFocus(GR_WINDOW *wp)
{
	GR_WINDOW	*oldfocus;

	if (wp == focuswp)
		return;

	GsDeliverGeneralEvent(focuswp, GR_EVENT_TYPE_FOCUS_OUT, wp);
	GsNotifyActivate(focuswp);

	oldfocus = focuswp;
	focuswp = wp;

	GsDeliverGeneralEvent(wp, GR_EVENT_TYPE_FOCUS_IN, oldfocus);
	GsNotifyActivate(focuswp);
}

/*
 * Set dynamic portrait mode and redraw screen.
 */
void
GsSetPortraitMode(int mode)
{
	GdSetPortraitMode(&scrdev, mode);
	GdRestrictMouse(0, 0, scrdev.xvirtres - 1, scrdev.yvirtres - 1);

	/* reset clip and root window size*/
	clipwp = NULL;
	rootwp->width = scrdev.xvirtres;
	rootwp->height = scrdev.yvirtres;

	/* deliver portrait changed event to all windows selecting it*/
	GsDeliverPortraitChangedEvent();
	
	/* redraw screen - apps may redraw/resize again causing flicker*/
	GsRedrawScreen();
}

/*
 * Check mouse coordinates and possibly set indicated portrait
 * mode from mouse position.
 */
void
GsSetPortraitModeFromXY(GR_COORD rootx, GR_COORD rooty)
{
	int newmode;

	if (rootx == 0) {
		/* rotate left*/
		switch (scrdev.portrait) {
		case MWPORTRAIT_NONE:
		default:
			newmode = MWPORTRAIT_LEFT;
			break;
		case MWPORTRAIT_LEFT:
			newmode = MWPORTRAIT_DOWN;
			break;
		case MWPORTRAIT_DOWN:
			newmode = MWPORTRAIT_RIGHT;
			break;
		case MWPORTRAIT_RIGHT:
			newmode = MWPORTRAIT_NONE;
			break;
		}
		GsSetPortraitMode(newmode);
		GrMoveCursor(5, rooty);
		GdMoveMouse(5, rooty);
	} else if (rootx == scrdev.xvirtres-1) {
		/* rotate right*/
		switch (scrdev.portrait) {
		case MWPORTRAIT_NONE:
		default:
			newmode = MWPORTRAIT_RIGHT;
			break;
		case MWPORTRAIT_LEFT:
			newmode = MWPORTRAIT_NONE;
			break;
		case MWPORTRAIT_DOWN:
			newmode = MWPORTRAIT_LEFT;
			break;
		case MWPORTRAIT_RIGHT:
			newmode = MWPORTRAIT_DOWN;
			break;
		}
		GsSetPortraitMode(newmode);
		GrMoveCursor(scrdev.xvirtres-5, rooty);
		GdMoveMouse(scrdev.xvirtres-5, rooty);
	}
}
