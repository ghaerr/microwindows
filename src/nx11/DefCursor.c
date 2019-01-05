#include "nxlib.h"

int
XDefineCursor(Display *dpy, Window w, Cursor cursor)
{
    // FIXME assumes Cursor is nano-X compatible
    GrSetWindowCursor(w, cursor);
    return 1;
}
