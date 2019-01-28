#include <X11/X.h>
#include <X11/Xlib.h>

/*
 * Arc drawing demo for X11
 */

#define GREEN	getcolor("green")
#define BLACK	BlackPixel(d, 0)

Display *d;

static unsigned long
getcolor(char *name)
{
	XColor xc, xc2;

	XAllocNamedColor(d, DefaultColormap(d, 0), name, &xc, &xc2);
	return xc.pixel;
}

static void
draw(XEvent *ep)
{
	Window wid = ((XExposeEvent *)ep)->window;
	GC gc = XCreateGC(d, wid, 0, NULL);
	int x = 20;
	int y = 20;
	int w, h;

	w = h = 60;
	XSetForeground(d, gc, BLACK);
	XDrawRectangle(d, wid, gc, x, y, w-1, h-1);
	XSetForeground(d, gc, GREEN);
	//XDrawArc(d, wid, gc, x, y, w-1, h-1, 0, 360*64);
	XFillArc(d, wid, gc, x, y, w, h, 0, 360*64);

	XSetForeground(d, gc, BLACK);
	XDrawPoint(d, wid, gc, x+w/2, y+h/2);
	//show if using libNX11 - prints CSET then
	XDrawString(d,wid,gc,1,10,XServerVendor(d),4);
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
		0, 0, 100, 100,
		0, BLACK, WhitePixel(d, 0));

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
