/* 
* Xlib programming demo - display a window with a cross on the screen 
* based on:
* http://xopendisplay.hilltopia.ca/2009/Jan/Xlib-tutorial-part-1----Beginnings.html
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char ** argv){
	int screen_num, width, height;
    unsigned long fcolor, bcolor;
	Window win;
	XEvent ev;
	Display *dpy;

	/* First connect to the display server */
	dpy = XOpenDisplay(NULL);
	if (!dpy) {fprintf(stderr, "unable to connect to display");return 7;}

	/* these are macros that pull useful data out of the display object */
	/* we use these bits of info enough to want them in their own variables */
	screen_num = DefaultScreen(dpy);
	bcolor = BlackPixel(dpy, screen_num);
	fcolor = WhitePixel(dpy, screen_num);

	// get the default colormap
	Colormap colmap = DefaultColormap(dpy, DefaultScreen(dpy));
    
	/* allocate a "red" color map entry. */
	XColor system_blue, system_red;
	Status rc;

	/* allocate blue color with values (0, 0, 0x8000) in RGB. */
	system_blue.red = system_blue.green =0;
	system_blue.blue = 0x8000;
	rc = XAllocColor(dpy,colmap,&system_blue);
	/* make sure the allocation succeeded. */
	if (rc == 0) {printf("XAllocColor blue failed.\n"); return 2;}
	/* allocate red color with values (0xFFFF, 0, 0) in RGB. */
	system_red.red = 0xFFFF;
	system_red.green = system_red.blue = 0;
	rc = XAllocColor(dpy, colmap, &system_red);
	if (rc == 0) {printf("XAllocColor red.\n"); return 3;}

	width = height = 400; /* size of window in pixel */
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), /* display, parent */
		20,20, /* place of window on screen */
		width, height, 
		5, fcolor, /* border width & color */
		system_blue.pixel); /* background color */

	/* tell the display server what kind of events we would like to see */
	XSelectInput(dpy, win, ButtonPressMask);

    // Get the fixed font.   
	XFontStruct *font = XLoadQueryFont( dpy, "System" );
    if( !font ){printf("Error, couldn't load font\n" ); return 1 ;}
	
	// in order to draw, we need a graphics context.
	XGCValues gv;
    GC gc = XCreateGC( dpy, screen_num, GCFunction | GCForeground | GCBackground, &gv );

	XSetForeground(dpy, gc, system_red.pixel);

	/* put the window on the screen */
	XMapWindow(dpy, win);

    // get the geometry of our window
    XWindowAttributes attr;
    XGetWindowAttributes( dpy, win, &attr );
    int w = attr.width, h = attr.height;
    
    // Draw a big cross that covers the whole window.
    XDrawLine( dpy, win, gc, 0, 0, w, h );
    XDrawLine( dpy, win, gc, 0, h, w, 0 );

    const char *text = "Hello World!";
    XDrawString( dpy, win, gc, 170, 60, text, strlen( text ) );

	/* Exit if button is pressed inside window */
	while(1){
		XNextEvent(dpy, &ev);
		switch(ev.type){
		case ButtonPress:
			XCloseDisplay(dpy);
			GrClose(); //Nano-X function - return to text screen mode
			return 0;
		}
	}
}
