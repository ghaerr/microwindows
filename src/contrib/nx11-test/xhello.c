#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>   // I include this to test return values the lazy way
#include <unistd.h>   // So we got the profile for 10 seconds

#define NIL (0)       // A name for the void pointer
#define FONT "-misc-fixed-medium-r-normal--10-100-75-75-c-60-iso10646-1" /*6x10.pcf.gz*/

void terminate(Display *dpy)
{
         XCloseDisplay(dpy);
         exit(0);
}

int main(int argc, char ** argv){
int width,height;

XEvent e;
XKeyEvent y;

Display *dpy = XOpenDisplay(NIL);
assert(dpy);

int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

const char *text   = "Hello World!";
const char *keymsg = "Key:        ";
char letters[10];

Atom wdw = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

// Create the window

Window w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 
             200, 100, 2, blackColor, blackColor);

// We want to get MapNotify events

XSelectInput(dpy, w, ButtonPressMask|StructureNotifyMask|KeyPressMask);

// "Map" the window (that is, make it appear on the screen)

XMapWindow(dpy, w);

// Get the fixed font.
XFontStruct *font = XLoadQueryFont( dpy, FONT);
if( !font ){printf("Error, couldn't load font\n" ); return 1 ;}

// Create a "Graphics Context"

GC gc = XCreateGC(dpy, w, 0, NIL);

// Tell the GC we draw using the white color

XSetForeground(dpy, gc, whiteColor);

// Wait for the MapNotify event

for(;;) {
    XNextEvent(dpy, &e);
    if (e.type == MapNotify)
	  break;
    }

while(1){
    XNextEvent(dpy, &e);
	
    KeySym          keysym;
    XComposeStatus  compose;
    char        buf[40];      
    int         bufsize = 40;
    int		length;
    int 	i;
    
    switch(e.type){

    case ClientMessage:
	// Check if click on "X" - close window and exit 
	if (e.xclient.data.l[0] == wdw) terminate(dpy);
	return 0;
	break;

    case ConfigureNotify:
         // Draw string and line
         XDrawString( dpy, w, gc, 10, 16, text, strlen( text ) );
         XDrawString( dpy, w, gc, 10, 36, keymsg, strlen( text ) );
         XDrawLine(dpy, w, gc, 10, 60, 180, 20);
         break;

    case ButtonPress:
	// close window and exit if button pressed within window
	// terminate(dpy);
	break;

    case KeyPress:
	length = XLookupString((XKeyEvent*)&e, buf, bufsize, &keysym, &compose);
        /* clear the old letter */
        XSetForeground(dpy, gc, blackColor);
	XFillRectangle(dpy, w, gc, 30, 26, 20, 20);

        XSetForeground(dpy, gc, whiteColor);
        letters[0]=(char)keysym;
	XDrawString( dpy, w, gc, 35, 36, (const char*)letters, 1);
        break;
  
    default: /* ignore any other event types. */
        break;
    }
} //while


}
