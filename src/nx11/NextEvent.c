#include "nxlib.h"
#include <stdlib.h>
#include <string.h>

#define EVDEBUG  0		/* seperate debug flag for this file*/

#if EVDEBUG
#define FUNC_ENTER DPRINTF("ENTER [%s]\n", __FUNCTION__)
#define FUNC_EXIT DPRINTF("LEAVE [%s]\n", __FUNCTION__)
#else
#define FUNC_ENTER
#define FUNC_EXIT
#undef DPRINTF
#define DPRINTF(str, ...)									  /* no debug output*/
#endif

static GR_UPDATE_TYPE
translateSubtype(int xtype)
{
	switch (xtype) {
	case MapNotify:
		return GR_UPDATE_MAP;
	case UnmapNotify:
		return GR_UPDATE_UNMAP;
	case ReparentNotify:
		return GR_UPDATE_REPARENT;
	case DestroyNotify:
		return GR_UPDATE_DESTROY;
	case ConfigureNotify:
		return GR_UPDATE_MOVE;	/* FIXME: | GR_UPDATE_SIZE*/
	}
	return 0;
}

static GR_EVENT_MASK
translateXEventType(int xtype)
{
	switch (xtype) {
	case EnterNotify:
		return GR_EVENT_MASK_MOUSE_ENTER;
	case LeaveNotify:
		return GR_EVENT_MASK_MOUSE_EXIT;
	case ButtonPress:
		return GR_EVENT_MASK_BUTTON_DOWN;
	case ButtonRelease:
		return GR_EVENT_MASK_BUTTON_UP;
	case KeyPress:
		return GR_EVENT_MASK_KEY_DOWN;
	case KeyRelease:
		return GR_EVENT_MASK_KEY_UP;
	case Expose:
		return GR_EVENT_MASK_EXPOSURE;
	case MapNotify:
	case UnmapNotify:
	case ReparentNotify:
	case DestroyNotify:
	case ConfigureNotify:
		return GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_CHLD_UPDATE;
	case MotionNotify:
		return GR_EVENT_MASK_MOUSE_MOTION;
	case FocusIn:
		return GR_EVENT_MASK_FOCUS_IN;
	case FocusOut:
		return GR_EVENT_MASK_FOCUS_OUT;
	}
	return GR_EVENT_MASK_NONE;

}

/* translate nano-X event to X11 event*/
static void
translateNXEvent(Display *dpy, GR_EVENT * ev, XEvent * event)
{
	static GR_TIMEOUT lasttime = 0;

	memset(event, 0, sizeof(*event)); /* for unhandled events...*/
	event->xany.display = dpy;

	switch (ev->type) {
	case GR_EVENT_TYPE_MOUSE_ENTER:
	case GR_EVENT_TYPE_MOUSE_EXIT:
		if (ev->type == GR_EVENT_TYPE_MOUSE_ENTER)
			event->type = EnterNotify;
		else
			event->type = LeaveNotify;
		{
			GR_EVENT_MOUSE *pev = (GR_EVENT_MOUSE *) ev;
			event->xcrossing.window = pev->wid;
			event->xcrossing.root = GR_ROOT_WINDOW_ID;
			//event->xcrossing.subwindow =
			event->xcrossing.time = lasttime; /* FIXME*/
			event->xcrossing.x = pev->x;
			event->xcrossing.y = pev->y;
			event->xcrossing.x_root = pev->rootx;
			event->xcrossing.y_root = pev->rooty;
			event->xcrossing.mode = NotifyNormal;
			//event->xcrossing.detail =
			//event->xcrossing.focus =
			event->xcrossing.same_screen = True;

			/* FIXME:  Add in the keyboard modifiers*/
			if (pev->buttons & GR_BUTTON_L)
				event->xcrossing.state |= Button1Mask;
			if (pev->buttons & GR_BUTTON_M)
				event->xcrossing.state |= Button2Mask;
			if (pev->buttons & GR_BUTTON_R)
				event->xcrossing.state |= Button3Mask;
			if (pev->buttons & GR_BUTTON_SCROLLUP)
				event->xcrossing.state |= Button4Mask;
			if (pev->buttons & GR_BUTTON_SCROLLDN)
				event->xcrossing.state |= Button5Mask;
		}
		break;

	case GR_EVENT_TYPE_MOUSE_MOTION:
		event->type = MotionNotify;
		{
			GR_EVENT_MOUSE *pev = (GR_EVENT_MOUSE *) ev;
			event->xmotion.window = pev->wid;
			event->xmotion.root = GR_ROOT_WINDOW_ID;
			event->xmotion.subwindow = pev->subwid;
			event->xmotion.time = lasttime; /* FIXME*/
			event->xmotion.x = pev->x;
			event->xmotion.y = pev->y;
			event->xmotion.x_root = pev->rootx;
			event->xmotion.y_root = pev->rooty;
			event->xmotion.is_hint = NotifyNormal;
			event->xmotion.same_screen = True;

			/* FIXME:  Add keyboard modifiers */
			if (pev->buttons & GR_BUTTON_L)
				event->xmotion.state |= Button1Mask;
			if (pev->buttons & GR_BUTTON_M)
				event->xmotion.state |= Button2Mask;
			if (pev->buttons & GR_BUTTON_R)
				event->xmotion.state |= Button3Mask;
			if (pev->buttons & GR_BUTTON_SCROLLUP)
				event->xmotion.state |= Button4Mask;
			if (pev->buttons & GR_BUTTON_SCROLLDN)
				event->xmotion.state |= Button5Mask;
		}
		break;

	case GR_EVENT_TYPE_BUTTON_DOWN:
		event->type = ButtonPress;
		{
			GR_EVENT_BUTTON *pev = (GR_EVENT_BUTTON *) ev;
			event->xbutton.window = pev->wid;
			event->xbutton.root = GR_ROOT_WINDOW_ID;
			event->xbutton.subwindow = pev->subwid;
			event->xbutton.time = lasttime = pev->time;
			event->xbutton.x = pev->x;
			event->xbutton.y = pev->y;
			event->xbutton.x_root = pev->rootx;
			event->xbutton.y_root = pev->rooty;
			event->xbutton.same_screen = True;

			/* FIXME: What about the whole OwnerGrabButton thing? */

			/* FIXME:  Add in the keyboard modifiers */
			if (pev->changebuttons & GR_BUTTON_L)
				event->xbutton.button = Button1;
			else if (pev->changebuttons & GR_BUTTON_M)
				event->xbutton.button = Button2;
			else if (pev->changebuttons & GR_BUTTON_R)
				event->xbutton.button = Button3;
			else if (pev->changebuttons & GR_BUTTON_SCROLLUP)
				event->xbutton.button = Button4;
			else if (pev->changebuttons & GR_BUTTON_SCROLLDN)
				event->xbutton.button = Button5;
			if (pev->buttons & GR_BUTTON_L)
				event->xbutton.state |= Button1Mask;
			if (pev->buttons & GR_BUTTON_M)
				event->xbutton.state |= Button2Mask;
			if (pev->buttons & GR_BUTTON_R)
				event->xbutton.state |= Button3Mask;
			if (pev->buttons & GR_BUTTON_SCROLLUP)
				event->xbutton.state |= Button4Mask;
			if (pev->buttons & GR_BUTTON_SCROLLDN)
				event->xbutton.state |= Button5Mask;
//DPRINTF("NextEvent.c: pev->changebuttons-%d,event->xbutton.button-%d,pev->buttons-%d,event->xbutton.state-%d\n",pev->changebuttons,event->xbutton.button,pev->buttons,event->xbutton.state);
		}
		break;
	case GR_EVENT_TYPE_BUTTON_UP:
		event->type = ButtonRelease;
		{
			GR_EVENT_BUTTON *pev = (GR_EVENT_BUTTON *) ev;
			event->xbutton.window = pev->wid;
			event->xbutton.root = GR_ROOT_WINDOW_ID;
			event->xbutton.subwindow = pev->subwid;
			event->xbutton.time = lasttime = pev->time;
			event->xbutton.x = pev->x;
			event->xbutton.y = pev->y;
			event->xbutton.x_root = pev->rootx;
			event->xbutton.y_root = pev->rooty;
			event->xbutton.same_screen = True;

			/* FIXME:  Add in the keyboard modifiers */
			event->xbutton.button = 0;
			if (pev->changebuttons & GR_BUTTON_L)
				event->xbutton.button = Button1;
			else if (pev->changebuttons & GR_BUTTON_M)
				event->xbutton.button = Button2;
			else if (pev->changebuttons & GR_BUTTON_R)
				event->xbutton.button = Button3;
			else if (pev->changebuttons & GR_BUTTON_SCROLLUP)
				event->xbutton.button = Button4;
			else if (pev->changebuttons & GR_BUTTON_SCROLLDN)
				event->xbutton.button = Button5;
			if (pev->buttons & GR_BUTTON_L)
				event->xbutton.state |= Button1Mask;
			if (pev->buttons & GR_BUTTON_M)
				event->xbutton.state |= Button2Mask;
			if (pev->buttons & GR_BUTTON_R)
				event->xbutton.state |= Button3Mask;
			if (pev->buttons & GR_BUTTON_SCROLLUP)
				event->xbutton.state |= Button4Mask;
			if (pev->buttons & GR_BUTTON_SCROLLDN)
				event->xbutton.state |= Button5Mask;
		}
		break;

	case GR_EVENT_TYPE_KEY_DOWN:
	case GR_EVENT_TYPE_KEY_UP:
		event->type = (ev->type == GR_EVENT_TYPE_KEY_DOWN) ?
			KeyPress : KeyRelease;
		{
			GR_EVENT_KEYSTROKE *pev = (GR_EVENT_KEYSTROKE *) ev;
			event->xkey.window = pev->wid;
			event->xkey.root = GR_ROOT_WINDOW_ID;
			event->xkey.subwindow = pev->subwid;
			event->xkey.time = lasttime; /* FIXME*/
			event->xkey.x = pev->x;
			event->xkey.y = pev->y;;
			event->xkey.x_root = pev->rootx;
			/* use unused y_root to read pev->ch in StrKeysym.c*/
			event->xkey.y_root = pev->ch; //write MWKey value into y_root 
			//event->xkey.y_root = pev->rooty;
			event->xkey.keycode = pev->scancode; /* note: not mwkey value*/
			event->xkey.same_screen = True;

			if (pev->modifiers & MWKMOD_CTRL)
				event->xkey.state |= ControlMask;
			if (pev->modifiers & MWKMOD_SHIFT)
				event->xkey.state |= ShiftMask;
			if (pev->modifiers & MWKMOD_ALT)
				event->xkey.state |= Mod1Mask;
			if (pev->modifiers & MWKMOD_CAPS)
				event->xkey.state |= ShiftMask;
			if (pev->modifiers & MWKMOD_SCR)
				event->xkey.state |= Mod5Mask;
			if (pev->modifiers & MWKMOD_NUM)
				event->xkey.state |= Mod2Mask;
		}
		break;
	case GR_EVENT_TYPE_EXPOSURE:
		event->type = Expose;
		{
			GR_EVENT_EXPOSURE *pev = (GR_EVENT_EXPOSURE *) ev;
			event->xexpose.window = pev->wid;
			event->xexpose.x = pev->x;
			event->xexpose.y = pev->y;
			event->xexpose.width = pev->width;
			event->xexpose.height = pev->height;
			event->xexpose.count = 0;	// FIXME?
		}
		break;
	case GR_EVENT_TYPE_UPDATE:
	case GR_EVENT_TYPE_CHLD_UPDATE:
		{
			GR_EVENT_UPDATE *pev = (GR_EVENT_UPDATE *) ev;

			if (pev->utype == GR_UPDATE_MAP) {
				event->type = MapNotify;
				event->xmap.event = pev->wid;
				event->xmap.window = pev->subwid;
				//event->xmap.override_redirect = 
			} else if (pev->utype == GR_UPDATE_UNMAP) {
				event->type = UnmapNotify;
				event->xunmap.event = pev->wid;
				event->xunmap.window = pev->subwid;
				event->xany.window = pev->wid;
				//event->xunmap.from_configure = 
#if 0 /* temp handled below*/
			} else if (pev->utype == GR_UPDATE_REPARENT) {
				event->type = ReparentNotify;
				event->xreparent.event = pev->wid;
				event->xreparent.window = pev->subwid;
				event->xreparent.x = pev->x;
				event->xreparent.y = pev->y;
				//event->xreparent.parent = pev->subwid;
				//event->xreparent.override_redirect = 
#endif
			} else if (pev->utype == GR_UPDATE_DESTROY) {
				event->type = DestroyNotify;
				event->xdestroywindow.event = pev->wid;
				event->xdestroywindow.window = pev->subwid;
			} else if (pev->utype == GR_UPDATE_SIZE
					|| pev->utype == GR_UPDATE_REPARENT
					/*|| pev->utype == GR_UPDATE_ACTIVATE*/
														) {
				/*
				 * FIXME: FLTK hack - for the time being, we
				 * handle GR_UPDATE_REPARENT as a ConfigureNotify
				 * event because the Microwindows window manager
				 * doesn't send an UPDATE_MOVE event when
				 * a window is decorated, and FLTK requires it.
				 */
				event->type = ConfigureNotify;
				event->xconfigure.event = pev->wid;
				event->xconfigure.window = pev->subwid;
				event->xconfigure.x = pev->x;
				event->xconfigure.y = pev->y;
				event->xconfigure.width = pev->width;
				event->xconfigure.height = pev->height;
				//event->xconfigure.border_width = 
				//event->xconfigure.above = 
				//event->xconfigure.override_redirect = 
			} else
				DPRINTF("translateNXEvent: unhandled update event subtype %d\n", pev->utype);
			break;
		}
	case GR_EVENT_TYPE_FOCUS_IN:
		event->type = FocusIn;
		{
			GR_EVENT_GENERAL *pev = (GR_EVENT_GENERAL *) ev;
			event->xfocus.window = pev->wid;
			event->xfocus.mode = NotifyNormal;
			event->xfocus.detail = NotifyDetailNone;  //FIXME?
		}
		break;
	case GR_EVENT_TYPE_FOCUS_OUT:
		event->type = FocusOut;
		{
			GR_EVENT_GENERAL *pev = (GR_EVENT_GENERAL *) ev;
			event->xfocus.window = pev->wid;
			event->xfocus.mode = NotifyNormal;
			event->xfocus.detail = NotifyDetailNone;  //FIXME?
		}
		break;
	case GR_EVENT_TYPE_CLOSE_REQ:
		/* Could check for WM_DELETE_WINDOW window property, if not set,
		 * then just call XDestroyWindow, otherwise send ClientMessage.
		 */
		/* dummy up WM_DELETE_WINDOW message for FLTK*/
		event->type = ClientMessage;
		{
			GR_EVENT_GENERAL *pev = (GR_EVENT_GENERAL *) ev;
			event->xclient.window = pev->wid;
			event->xclient.format = 32;
			event->xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
			//event->xclient.message_type = 		//FIXME?
		}
		break;
	default:
		DPRINTF("translateNXEvent: UNHANDLED EVENT %d\n", ev->type);
	}
}

static XEvent saved_event;
static int saved = 0;
int
XPutBackEvent(Display *display, XEvent *event)
{
	if (saved) {
		DPRINTF("XPutBackEvent: lost event\n");
		return 0;
	}
	saved = 1;
	saved_event = *event;
	return 0;
}

int
XEventsQueued(Display * display, int mode)
{
	int ret;
	GR_EVENT temp;

	FUNC_ENTER;
	ret = GrQueueLength();

	/* check hack for local saved event*/
	if (saved)
		return ret+1;

	if (!ret && mode != QueuedAlready) {
		if (mode == QueuedAfterFlush)
			GrFlush();
		if (GrPeekEvent(&temp))
			ret = 1;
	}

	if (ret)
		DPRINTF("Returning %d events ready\n", ret);

	return ret;
}

int
XPending(Display * dpy)
{
	DPRINTF("XPending ");
	return XEventsQueued(dpy, QueuedAfterFlush);
}

int
XQLength(Display * dpy)
{
	FUNC_ENTER;

	/* FIXME?
	 * FLTK hack: FLTK spins calling XQLength and
	 * then select().  Since Microwindows won't return
	 * an event without having called GrPeekEvent,
	 * using QueuedAfterReading rather than QueuedAlready
	 * makes FLTK work.
	 */
	/*return XEventsQueued(dpy, QueuedAlready);*/
	return XEventsQueued(dpy, QueuedAfterReading);
}

/* 
 * Return next event in queue, or if none, flush output and wait for
 * events.
 */
int
XNextEvent(Display * dpy, XEvent * event)
{
	GR_EVENT ev;

	FUNC_ENTER;
	if (saved) {
		*event = saved_event;
		saved = 0;
	} else {
		GrGetNextEvent(&ev);
		translateNXEvent(dpy, &ev, event);
	}
	FUNC_EXIT;
	return 1;
}

int
XPeekEvent(Display * dpy, XEvent * event)
{
	GR_EVENT ev;

	FUNC_ENTER;
	if (saved) {
		*event = saved_event;
	} else {
		GrPeekWaitEvent(&ev);
		translateNXEvent(dpy, &ev, event);
	}
	FUNC_EXIT;
	return 1;

}

int
XFilterEvent(XEvent * event, Window w)
{
	return False;
}

typedef Bool(*PredFunc) (Display *, XEvent *, XPointer);
typedef struct {
	Display *display;
	XEvent *event;
	XPointer arg;
	PredFunc func;
} XIfEventParm;

static GR_BOOL
_XIfEventCallback(GR_WINDOW_ID wid, GR_EVENT_MASK mask,
	GR_UPDATE_TYPE update, GR_EVENT *ep, void *arg)
{
	XIfEventParm *p = (XIfEventParm *) arg;

	translateNXEvent(p->display, ep, p->event);
	return (*p->func) (p->display, p->event, p->arg);
}

Bool
_XIfEvent(Display * display, XEvent * ev, PredFunc predicate, XPointer arg, int block)
{
	XIfEventParm p;
	GR_EVENT event;

	p.display = display;
	p.event = ev;
	p.func = predicate;
	p.arg = arg;

	FUNC_ENTER;
	/* note: event not returned directly but translated in callback routine*/
	return GrGetTypedEventPred(0, 0, 0, &event, block, _XIfEventCallback, &p);
}

Bool
XIfEvent(Display * display, XEvent * ev, PredFunc predicate, XPointer arg)
{
	DPRINTF("XIfEvent ");
	return _XIfEvent(display, ev, predicate, arg, 1);
}

Bool
XCheckIfEvent(Display * display, XEvent * ev, PredFunc predicate, XPointer arg)
{
	DPRINTF("XCheckIfEvent ");
	return _XIfEvent(display, ev, predicate, arg, 0);
}

/* Check masked events*/
Bool
XCheckWindowEvent(Display * display, Window w, long event_mask, XEvent * ev)
{
	GR_EVENT event;

	FUNC_ENTER;
	if (GrGetTypedEvent(w, _nxTranslateEventMask(event_mask), 0, &event, GR_FALSE)) {
		translateNXEvent(display, &event, ev);
		FUNC_EXIT;
		return True;
	}
	FUNC_EXIT;
	return False;
}

Bool
XCheckMaskEvent(Display * display, long event_mask, XEvent * ev)
{
	GR_EVENT event;

	FUNC_ENTER;
	if (GrGetTypedEvent(0, _nxTranslateEventMask(event_mask), 0, &event, GR_FALSE)) {
		translateNXEvent(display, &event, ev);
		FUNC_EXIT;
		return True;
	}
	FUNC_EXIT;
	return False;
}

/* Check typed events*/
Bool
XCheckTypedEvent(Display * display, int event_type, XEvent * ev)
{
	GR_EVENT event;

	FUNC_ENTER;
	if (GrGetTypedEvent(0, translateXEventType(event_type),
	    translateSubtype(event_type), &event, GR_FALSE)) {
		translateNXEvent(display, &event, ev);
		FUNC_EXIT;
		return True;
	}
	FUNC_EXIT;
	return False;
}

Bool
XCheckTypedWindowEvent(Display * display, Window w, int event_type, XEvent * ev)
{
	GR_EVENT event;

	FUNC_ENTER;
	if (GrGetTypedEvent(w, translateXEventType(event_type),
			    translateSubtype(event_type), &event, GR_FALSE)) {
		translateNXEvent(display, &event, ev);
		FUNC_EXIT;
		return True;
	}
	FUNC_EXIT;
	return False;
}

/* Blocking calls*/
int
XWindowEvent(Display * display, Window w, long event_mask, XEvent * ev)
{
	GR_BOOL block = GR_TRUE;
	GR_EVENT event;

	FUNC_ENTER;
	/*
	 * FIXME: FLTK is coded assuming at least a NoExpose event will
	 * be returned after an XCopyArea.  Since Microwindows doesn't
	 * do this, we check for ExposureMask, don't block, and return
	 * NoExpose in order to get FLTK to work. (fltk/test/scroll demo)
	 */
	if (event_mask == ExposureMask)
		block = GR_FALSE;

	if (GrGetTypedEvent(w, _nxTranslateEventMask(event_mask), 0, &event, block)) {
		translateNXEvent(display, &event, ev);
		FUNC_EXIT;
		return ev->type;
	}
	FUNC_EXIT;

	/* FLTK hack*/
	if (event_mask == ExposureMask)
		ev->type = NoExpose;

	return 0;
}

int
XMaskEvent(Display * display, long event_mask, XEvent * ev)
{
	GR_EVENT event;

	FUNC_ENTER;
	if (GrGetTypedEvent(0, _nxTranslateEventMask(event_mask), 0, &event, GR_TRUE)) {
		translateNXEvent(display, &event, ev);
		FUNC_EXIT;
		return ev->type;
	}

	FUNC_EXIT;
	return 0;
}

long
XExtendedMaxRequestSize(Display * display)
{ 
/*function returns zero if the specified display does not support 
 * an extended-length protocol encoding */
	return 0; 
} 

long
XMaxRequestSize(Display * display)
{ 
/* The protocol guarantees the size to be no smaller than 4096 units (16384 bytes). */  
	return 4096; 
} 
