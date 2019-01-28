#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>

/* The window which contains the text. */

struct {
    int width;
    int height;
    char * text;
    int text_len;

    /* X Windows related variables. */

    Display * display;
    int screen;
    Window root;
    Window window;
    GC gc;
    XFontStruct * font;
    unsigned long black_pixel;    
    unsigned long white_pixel;
}
text_box;

/* Connect to the display, set up the basic variables. */

static void x_connect ()
{
    text_box.display = XOpenDisplay (NULL);
    if (! text_box.display) {
        fprintf (stderr, "Could not open display.\n");
         exit(1);
    }
    text_box.screen = DefaultScreen (text_box.display);
    text_box.root = RootWindow (text_box.display, text_box.screen);
    text_box.black_pixel = BlackPixel (text_box.display, text_box.screen);
    text_box.white_pixel = WhitePixel (text_box.display, text_box.screen);
}

/* Create the window. */

static void create_window ()
{
    text_box.width = 300;
    text_box.height = 300;
    text_box.window = 
        XCreateSimpleWindow (text_box.display,
                             text_box.root,
                             1, /* x */
                             1, /* y */
                             text_box.width,
                             text_box.height,
                             0, /* border width */
                             text_box.black_pixel, /* border pixel */
                             text_box.white_pixel  /* background */);
    XSelectInput (text_box.display, text_box.window,
                  ExposureMask);
    XMapWindow (text_box.display, text_box.window);
}

/* Set up the GC (Graphics Context). */

static void set_up_gc ()
{
    text_box.screen = DefaultScreen (text_box.display);
    text_box.gc = XCreateGC (text_box.display, text_box.window, 0, 0);
    XSetBackground (text_box.display, text_box.gc, text_box.white_pixel); 
    XSetForeground (text_box.display, text_box.gc, text_box.black_pixel); 
}

/* Set up the text font. */

static void set_up_font ()
{
    const char * fontname = "-*-helvetica-*-r-*-*-14-*-*-*-*-*-*-*";
    text_box.font = XLoadQueryFont (text_box.display, fontname);
    /* If the font could not be loaded, revert to the "fixed" font. */
    if (! text_box.font) {
        fprintf (stderr, "unable to load font %s: using fixed\n", fontname);
        text_box.font = XLoadQueryFont (text_box.display, "fixed");
    }
    XSetFont (text_box.display, text_box.gc, text_box.font->fid);
}

/* Draw the window. */

static void draw_screen ()
{
    int x;
    int y;
    int direction;
    int ascent;
    int descent;
    XCharStruct overall;

    /* Centre the text in the middle of the box. */

    XTextExtents (text_box.font, text_box.text, text_box.text_len,
                  & direction, & ascent, & descent, & overall);
    x = (text_box.width - overall.width) / 2;
    y = text_box.height / 2 + (ascent - descent) / 2;
    XClearWindow (text_box.display, text_box.window);
    XDrawString (text_box.display, text_box.window, text_box.gc,
                 x, y, text_box.text, text_box.text_len);
}


int main (int argc, char ** argv)
{
    text_box.text = "Hello World";
    text_box.text_len = strlen (text_box.text);
    x_connect ();
    create_window ();
    set_up_gc ();
    set_up_font ();
	/* Loop over events. */
    while (1) {
        XEvent e;
        XNextEvent (text_box.display, & e);
        if (e.type == Expose) {
            draw_screen ();
        }
    }
    return 0;
}
