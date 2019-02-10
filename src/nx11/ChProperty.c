#include <string.h>
#include <stdlib.h>
#include "nxlib.h"
#include "X11/Xatom.h"

#define SZHASHTABLE	32
struct window_props {
	Atom property;
	Atom type;
	int format;
	unsigned char *data;
	int nelements;
	int bytes;
	struct window_props *next;
};

struct windows {
	Window w;
	struct window_props *properties;
	struct windows *next;
};
static struct windows *window_list[SZHASHTABLE];

int
XChangeProperty(Display * display, Window w, Atom property,
		Atom type, int format, int mode,
		_Xconst unsigned char *data, int nelements)
{
	struct windows *win = NULL;
	struct window_props *prop = NULL;
	Window prop_window;
	int hash = w % SZHASHTABLE;
	GR_WM_PROPERTIES props;
	
	if (!nelements || data == NULL) {
		DPRINTF("XChangeProperty called with NULL data [Atom:%lx,m:%x,d:%lx]\n", type, mode, (long)data);
		return 1;
	}

	DPRINTF("XChangeProperty called [Atom:%lx, mode:%x, data:%lx]\n", type, mode, (long)data);
	win = window_list[hash];
	if (!win) {
		win = window_list[hash] =
//			(struct windows *) Xcalloc(sizeof(struct windows), 1);
			(struct windows *) Xcalloc(1, sizeof(struct windows));
	} else {
		struct windows *t;

		for (t=win; ; t=t->next) {
			if (t->w == w) {
				win = t;
				break;
			}
			if (!t->next)
				break;
		}

		if (!win) {
			win = t->next =
				//(struct windows *) Xcalloc(sizeof(struct windows), 1);
				(struct windows *) Xcalloc(1, sizeof(struct windows));
		}
	}
	win->w = w;

	if (!win->properties) {
		prop = win->properties =
			//(struct window_props *) Xcalloc(sizeof(struct window_props), 1);
			(struct window_props *) Xcalloc(1, sizeof(struct window_props));
	} else {
		struct window_props *t;

		for (t=win->properties; ; t=t->next) {
			if (t->property == property) {
				prop = t;
				break;
			}
			if (!t->next)
				break;
		}

		if (!prop) {
			prop = t->next =
				//(struct window_props *) Xcalloc(sizeof(struct window_props), 1);
				(struct window_props *) Xcalloc(1, sizeof(struct window_props));
		}
	}
	DPRINTF("XChangeProperty: win:%x prop:%x\n", (int)w, (int)prop);

	switch (mode) {
	case PropModeAppend:	// 2
	case PropModePrepend:	// 1
		if (prop->data) {
			int f8 = prop->format / 8;
			char *p;
			int bytes;

			//DPRINTF("XChangeProperty: %d,%d\n", format, bytes);
			if (type != prop->type || format != prop->format)
				return 0;

			bytes = (prop->nelements + nelements) * f8;
			p = (char *)Xmalloc(bytes+1);  /* alloc + 1 for string '\0'*/
			p[bytes] = '\0';

			if (mode == PropModeAppend) {
				memcpy(p, prop->data, prop->nelements * f8);
				memcpy(p + (prop->nelements * f8), data,
				       (nelements * f8));
			} else {
				memcpy(p, data, nelements * f8);
				memcpy(p + (nelements * f8), prop->data,
				       (prop->nelements * f8));
			}

			Xfree(prop->data);
			prop->data = (unsigned char *)p;
			prop->nelements += nelements;
			prop->bytes = bytes;
			break;
		}
		/* Fall through */

	case PropModeReplace:	// 0
		if (prop->data) Xfree(prop->data);
		if (format<0 || format/8 > 4) {
			DPRINTF("XChangeProperty: format[%d]\n", format);
			format = 8;
		}
		prop->bytes = nelements * (format / 8);
		prop->data = (unsigned char *) Xmalloc(prop->bytes+1); /* +1 for string '\0'*/
		prop->data[prop->bytes] = '\0';

		//DPRINTF("XChangeProperty: [%x,%d,%x]\n", prop,nelements,prop->bytes);
		memcpy(prop->data, data, prop->bytes);

		prop->property = property;
		prop->type = type;
		prop->format = format;
		prop->nelements = nelements;
		break;
	}

	switch (property) {
		case XA_WM_TRANSIENT_FOR:
			prop_window = *((Window *)data);
			GrGetWMProperties(prop_window, &props);
			if (props.title)
				free(props.title);
			props.flags = GR_WM_FLAGS_PROPS;
			props.props |= (GR_WM_PROPS_NORAISE | GR_WM_PROPS_NOMOVE | GR_WM_PROPS_NOFOCUS);
			GrSetWMProperties(prop_window, &props);
			break;
		case XA_WM_NAME:
			props.flags = GR_WM_FLAGS_TITLE;
			props.title = (char *)prop->data;
			GrSetWMProperties(w, &props);
			break;
		default:
			break;
	}

	return 1;
}

static void
_nxDelPropData(struct window_props *prop)
{
	/* delete XA_WM_TRANSIENT_FOR data*/
	if ((prop->property == XA_WM_TRANSIENT_FOR) && prop->data) {
		Window prop_window = *((Window*)prop->data);
		GR_WM_PROPERTIES props;

		GrGetWMProperties(prop_window, &props);
		if (props.title)
			free(props.title);
		props.flags = GR_WM_FLAGS_PROPS;
		props.props &= ~(GR_WM_PROPS_NORAISE | GR_WM_PROPS_NOMOVE | GR_WM_PROPS_NOFOCUS);
		GrSetWMProperties(prop_window, &props);

		GrSetFocus(prop_window);
	}

	if (prop->data)
		Xfree(prop->data);
	Xfree(prop);
}

int
XDeleteProperty(Display *display, Window w, Atom property)
{
	struct windows *win;
	struct window_props *prop;
	int hash = w % SZHASHTABLE;

	for (win = window_list[hash]; win; win = win->next)
		if (win->w == w) {
			struct window_props *prev = NULL;

			for (prop = win->properties; prop; prop = prop->next)
				if (prop->property == property) {
					if (prev)
						prev->next = prop->next;
					else
						win->properties = prop->next;
					_nxDelPropData(prop);

					if (win == window_list[hash])
						window_list[hash] = NULL;
					return 1;
				}
		}

	return 1;
}

int
_nxDelAllProperty(Window w)
{
	struct windows *win;
	struct window_props *prop;
	int hash = w % SZHASHTABLE;

	for (win = window_list[hash]; win; win = win->next)
		if (win->w == w) {
			prop = win->properties;
			while (prop) {
				struct window_props *next = prop->next;
				_nxDelPropData(prop);
				prop = next;
			}

			Xfree(win);
			if (win == window_list[hash])
				window_list[hash] = NULL;
			return 1;
		}
	return 1;
}

int
XGetWindowProperty(Display * dpy, Window w, Atom property, long offset,
	long len, Bool bDel, Atom req, Atom * type, int *format,
	unsigned long *nitems, unsigned long *bytes, unsigned char **data)
{
	struct windows *win;
	struct window_props *prop;
	int hash = w % SZHASHTABLE;

	for (win = window_list[hash]; win; win = win->next) {
		if (win->w == w) {
			for (prop = win->properties; prop; prop = prop->next) {
				if (prop->property == property) {
					*type = prop->type;
					*format = prop->format;
					*data = prop->data;
					*nitems = prop->nelements;
					*bytes = prop->bytes;
					return 0;
				}
			}
		}
	}
	*type = None;
	*format = 0;
	*data = 0;
	*nitems = 0;
	*bytes = 0;
	return 1;		/* failure */
}

