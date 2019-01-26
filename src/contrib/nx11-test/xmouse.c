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

XEvent event;

Display *dpy = XOpenDisplay(NIL);
assert(dpy);

int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

const char *text   = "Pressed button: ";
char letters[10];
int button;
int x=-1,y=-1;

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
    XNextEvent(dpy, &event);
    if (event.type == MapNotify)
	  break;
    }

while(1){
    XNextEvent(dpy, &event);
	
    KeySym          keysym;
    XComposeStatus  compose;
    char        buf[40];      
    int         bufsize = 40;
    int		length;
    int 	i;
    
    switch(event.type){

    case ClientMessage:
	// Check if click on "X" - close window and exit 
	if (event.xclient.data.l[0] == wdw) terminate(dpy);
	return 0;
	break;

    case ConfigureNotify:
         break;

    case ButtonPress:
            switch(event.xbutton.button)
            {
            case Button1:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button1;
                break;
            case Button2:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button2;
                break;
            case Button3:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button3;
                break;
            case Button4:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button4;
                break;
            case Button5:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button5;
                break;
            default:
                break;
            }

        /* clear the old button number */
        XSetForeground(dpy, gc, blackColor);
        XFillRectangle(dpy, w, gc, 100, 6, 95, 20);

        XSetForeground(dpy, gc, whiteColor);
	XDrawString( dpy, w, gc, 10, 16, text, strlen( text ) );
	sprintf(letters, "%d at %d,%d", button,x,y);
	XDrawString( dpy, w, gc, 110, 16, (const char*)letters, strlen(letters) );            
	
        break;
#if 0	    
        case ButtonRelease:
            switch(event.xbutton.button)
            {
            case Button1:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button1;
                break;
            case Button2:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button2;
                break;
            case Button3:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button3;
                break;
            case Button4:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button4;
                break;
            case Button5:
                x=event.xbutton.x;
                y=event.xbutton.y;
                button=Button5;
                break;
            default:
                break;
            }
            break;
#endif  

	default: /* ignore any other event types. */
        break;
    }
} //while


}
