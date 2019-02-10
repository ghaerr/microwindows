#include "nxlib.h"
#include <stdlib.h>

/*
 * Local colormap functions to emulate X11 colormap scheme
 */

static nxColormap *colormap_hash[32];
static int colormap_id = 1;
static Colormap _defaultColormap = 0;

static Colormap
createColormap(int color_count)
{
	int val = (colormap_id - 1) % 32;
	nxColormap *local = (nxColormap *)Xcalloc(sizeof(nxColormap), 1);

	if (!local)
		return 0;
	local->id = colormap_id++;

	if (color_count) {
		/* The initial count of colors is simply a good suggestion.
		 * It can grow / shrink
		 */
		local->colorval =
			(nxColorval *)Xmalloc(color_count *
						sizeof(nxColorval));

		if (local->colorval) {
			local->color_alloc = color_count;
			local->cur_color = 0;
		}
	}

	if (!colormap_hash[val])
		colormap_hash[val] = local;
	else {
		nxColormap *t = colormap_hash[val];
		while (t->next)
			t = t->next;
		t->next = local;
	}

	return local->id;
}

#if LATER
static void
destroyColormap(Colormap id)
{
	int val = id % 32;
	nxColormap *ptr, *prev;

	for (ptr = colormap_hash[val], prev = 0; ptr;
	     prev = ptr, ptr = ptr->next) {
		if (ptr->id == id) {
			if (prev)
				prev->next = ptr->next;
			else
				colormap_hash[val] = ptr->next;

			if (ptr->colorval)
				Xfree(ptr->colorval);
			Xfree(ptr);
			return;
		}
	}
}
#endif

nxColormap *
_nxFindColormap(Colormap id)
{
	int val = (id - 1) % 32;
	nxColormap *ptr = colormap_hash[val];

	if (id == 0)
		return 0;

	for (ptr = colormap_hash[val]; ptr; ptr = ptr->next)
		if (ptr->id == id)
			return ptr;

	return 0;
}

/* Populate the colormap with the 32 system colors */
Colormap
_nxDefaultColormap(Display *dpy)
{
	XColor t;
	Colormap cm;

	if (_defaultColormap)
		return _defaultColormap;

	cm = createColormap(32);

	/*
	 * Note the following won't allocate a colormap entry
	 * on truecolor hardware...
	 */

	/* Create the 0th index to have black */
	t.red = t.blue = t.green = 0 << 8;
	XAllocColor(dpy, cm, &t);

	/* Create the 1st index to have white */
	t.red = t.blue = t.green = 255 << 8;
	XAllocColor(dpy, cm, &t);

	_defaultColormap = cm;
	return cm;
}

/*
 * Create a colormap.  We always use the default colormap
 * for the time being.  Non-default visuals aren't supported either.
 */
Colormap
XCreateColormap(Display * display, Window w, Visual * visual, int alloc)
{
	return _nxDefaultColormap(display);
#if 0 // FIXME?
	switch (visual->class) {
	case DirectColor:
		return createColormap(0);

	case GrayScale:
	case PseudoColor:
		return createColormap(visual->map_entries);

	case StaticGray:
	case StaticColor:
	case TrueColor:
		if (alloc == AllocAll)
			return 0;
		return createColormap(visual->map_entries);
	}

	return 0;
#endif
}

int
XFreeColormap(Display * display, Colormap colormap)
{
#if 0 // FIXME?
	destroyColormap(colormap);
#endif
	return 1;
}

#include "X11/Xutil.h"
// required for Qt
Status XGetRGBColormaps(Display *display, Window w, XStandardColormap **std_colormap, int *count, Atom property)
{
	DPRINTF("XGetRGBColormaps called...\n");
//	*std_colormap = _nxDefaultColormap(display);
	return 0;
}

XStandardColormap *XAllocStandardColormap()
{
	return calloc(1, sizeof(XStandardColormap));
}
