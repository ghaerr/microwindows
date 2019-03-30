/*
 * Copyright (c) 2000, 2002, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2006 by Andreas Foedrowitz
 *
 * Nano-X Client utility routines
 *
 * These routines are Gr* functionality without requiring new API entry points.
 */

/**
 * Create a new buffered window.  Sets properties for
 * no background erase and no redraw during resizing.
 * Same parameters as GrNewWindowEx.
 *
 * Quick conversion for double buffered windows in any application:
 * 	add GR_WM_PROPS_BUFFERED to GrNewWindowEx flags
 * 	add GR_WM_PROPS_NOBACKGROUND if app paints background
 * 	add GrFlushWindow(wid) call in paint routine
 * 	add update mask and handling for GR_UPDATE_MAP & GR_UPDATE_SIZE and call paint routine
 * 	remove Exposure mask and event handling
 * Buffered windows allocate a 32bpp RGBA pixmap for automatic offscreen buffered drawing.
 * Pixmap is erased to bg color on create and resize unless GR_WM_PROPS_NOBACKGROUND set
 * On window expose, buffered contents are copied, app never gets expose events.
 * App must process resize and map Update events, flush buffer with GrFlushWindow(wid)
 */
GR_WINDOW_ID
GrNewBufferedWindow(GR_WM_PROPS props, const char *title, GR_WINDOW_ID parent,
	GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height, GR_COLOR background)
{
	return GrNewWindowEx(props|GR_WM_PROPS_BUFFERED|GR_WM_PROPS_NOBACKGROUND,
		title, parent, x, y, width, height, background);
}

/**
 * Create new window with passed style, title and location.
 */
GR_WINDOW_ID
GrNewWindowEx(GR_WM_PROPS props, const char *title, GR_WINDOW_ID parent,
	GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height, GR_COLOR background)
{
	GR_WINDOW_ID wid;
	GR_WM_PROPERTIES wmprops;

	/* create window with no borders */
	wid = GrNewWindow(parent, x, y, width, height, 0, background, BLACK);
	if (wid) {
		/* set properties and title */
		wmprops.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
		wmprops.props = props;
		wmprops.title = (char *)title;
		GrSetWMProperties(wid, &wmprops);
	}
	return wid;
}

/* draw an array of lines */
void
GrDrawLines(GR_DRAW_ID w, GR_GC_ID gc, GR_POINT * points, GR_COUNT count)
{
	GR_POINT beg, end;

	if (count < 1)
		return;
	if (count == 1) {
		GrPoint(w, gc, points->x, points->y);
		return;
	}

	beg = *points++;
	while (--count > 0) {
		end = *points++;
		GrLine(w, gc, beg.x, beg.y, end.x, end.y);
		beg = end;
	}
}

/*
 * DEPRECATED.  Use GrNewCursor and GrSetWindowCursor for new code.
 */
GR_CURSOR_ID
GrSetCursor(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height, GR_COORD hotx,
	    GR_COORD hoty, GR_COLOR foreground, GR_COLOR background,
	    GR_BITMAP * fgbitmap, GR_BITMAP * bgbitmap)
{
	GR_CURSOR_ID cid;

	cid = GrNewCursor(width, height, hotx, hoty, foreground,
			  background, fgbitmap, bgbitmap);
	if (cid)
		GrSetWindowCursor(wid, cid);
	return cid;
}

/**
 * Copy an event, must be used for CLIENT_DATA events
 * when GrPeekEvent or GrPeekWaitEvent called, as event data later freed.
 */
void
GrCopyEvent(GR_EVENT *dst, GR_EVENT *src)
{
	*dst = *src;

	/* do a "deep" copy for client data events*/
	if (dst->type == GR_EVENT_TYPE_CLIENT_DATA) {
		GR_EVENT_CLIENT_DATA *csrc = (GR_EVENT_CLIENT_DATA *)src;
		GR_EVENT_CLIENT_DATA *cdst = (GR_EVENT_CLIENT_DATA *)dst;

		if (csrc->data && (csrc->datalen > 0)) {
			cdst->data = malloc(csrc->datalen);
			if (cdst->data) {
				memcpy(cdst->data, csrc->data, csrc->datalen);
			}
		} else {
			cdst->data = NULL;
			csrc->datalen = 0;
		}
	}
}

/**
 * Free an event after use with GrCopyEvent,
 * used with CLIENT_DATA events.
 */
void
GrFreeEvent(GR_EVENT *ev)
{
	/* free data in client data event*/
	if (ev->type == GR_EVENT_TYPE_CLIENT_DATA) {
		GR_EVENT_CLIENT_DATA *pev = (GR_EVENT_CLIENT_DATA *)ev;

		if (pev->data) {
			free(pev->data);
			pev->data = NULL;
		}
		pev->datalen = 0;
	}
}

void GrClearWindow(GR_WINDOW_ID wid,int exposeflag)
{
	GrClearArea(wid,0,0,0,0,exposeflag);
}
void GrFlushWindow(GR_WINDOW_ID wid)
{
	GrClearWindow(wid,2);
}
void GrSetWindowBackgroundColor(GR_WINDOW_ID wid,GR_COLOR color)
{
	GR_WM_PROPERTIES props;
	props.flags = GR_WM_FLAGS_BACKGROUND;
	props.background = color;
	GrSetWMProperties(wid, &props);
}
void GrSetWindowBorderSize(GR_WINDOW_ID wid,GR_COORD width)
{
	GR_WM_PROPERTIES props;
	props.flags = GR_WM_FLAGS_BORDERSIZE;
	props.bordersize = width;
	GrSetWMProperties(wid, &props);
}
void GrSetWindowBorderColor(GR_WINDOW_ID wid,GR_COLOR color)
{
	GR_WM_PROPERTIES props;
	props.flags = GR_WM_FLAGS_BORDERCOLOR;
	props.bordercolor = color;
	GrSetWMProperties(wid, &props);
}
void GrSetWindowTitle(GR_WINDOW_ID wid,char *name)
{
	GR_WM_PROPERTIES props;
	props.flags = GR_WM_FLAGS_TITLE;
	props.title = (char *)name;
	GrSetWMProperties(wid, &props);
}
