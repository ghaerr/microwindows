#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>

Display *dis;
Window win;
XEvent report;
GC green_gc;
XColor green_col;
Colormap colormap;

/*
The color can also be from $(X11_RGBTXT) (rgb.txt), such as RoyalBlue4.
A # (number sign) is only needed when using hexadecimal colors.
*/
char green[] = "aquamarine";
//char green[] = "#00FF00";

int main() {
	dis = XOpenDisplay(NULL);
	win = XCreateSimpleWindow(dis, RootWindow(dis, 0), 1, 1, 500, 500, 0, BlackPixel (dis, 0), BlackPixel(dis, 0));
	XMapWindow(dis, win);
	colormap = DefaultColormap(dis, 0);
	green_gc = XCreateGC(dis, win, 0, 0);
	XAllocNamedColor(dis, colormap, green, &green_col, &green_col);
	//XParseColor(dis, colormap, green, &green_col);
	//XAllocColor(dis, colormap, &green_col);
	XSetForeground(dis, green_gc, green_col.pixel);

	XSelectInput(dis, win, StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask);

	while (1)  {
	XNextEvent(dis, &report);
		switch  (report.type) {
		case Expose:   
			XDrawRectangle(dis, win, green_gc, 10, 10, 477, 477);
			XFillRectangle(dis, win, green_gc, 50, 50, 398, 398);
			XFlush(dis);
			break;
		case ClientMessage:
			// Check if click on "X" - close window and exit 
			exit(0);
	 	case KeyPress:
		/*Close the program if q is pressed.*/
			if (XLookupKeysym(&report.xkey, 0) == XK_q) {
			exit(0);
		}
		break;
		}
	}

return 0;
}
