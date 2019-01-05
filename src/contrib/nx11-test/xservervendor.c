#include <X11/X.h>
#include <X11/Xlib.h>
#include <string.h>

/*
 * Test XServerVendor routine
 */

Display *d;

static void
draw(XEvent *ep)
{
	Window wid = ((XExposeEvent *)ep)->window;
	GC gc = XCreateGC(d, wid, 0, NULL);

	XDrawString(d,wid,gc,50,50,XServerVendor(d),strlen(XServerVendor(d))); 
	XDrawString(d,wid,gc,50,80,"\"The X.Org Foundation\" means using the standard Xlib library",60); 
	XDrawString(d,wid,gc,50,110,"\"CSET\" means using the Microwindows NX11 library",48);

	XFreeGC(d, gc);
}

int
main(int ac, char **av)
{
	XEvent ev;
	Window wid;

	if ((d = XOpenDisplay(NULL)) == NULL)
		return 1;

	wid = XCreateSimpleWindow(d, RootWindow(d, 0),
		0, 0, 400, 400,
		0, BlackPixel(d, 0), WhitePixel(d, 0));

	XSelectInput(d, wid, ExposureMask);
	XMapWindow(d, wid);

	while (1) {
		XNextEvent(d, &ev);

		if (ev.type == Expose)
			draw(&ev);
	}

	XCloseDisplay(d);
	return 0;
}
