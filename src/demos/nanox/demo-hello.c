#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

GR_WINDOW_ID  wid;
GR_GC_ID      gc;

void event_handler (GR_EVENT *event);

int main (void)
{
    if (GrOpen() < 0)
    {
        GrError("GrOpen failed");
        return 1;
    }

    gc = GrNewGC();
    GrSetGCUseBackground (gc, GR_FALSE);
    GrSetGCForeground (gc, RED);

    wid = GrNewWindowEx (GR_WM_PROPS_APPFRAME |
                         GR_WM_PROPS_CAPTION  |
                         GR_WM_PROPS_CLOSEBOX,
                         "Hello Window",
                         GR_ROOT_WINDOW_ID, 
                         50, 50, 200, 100, WHITE);

    GrSelectEvents (wid, GR_EVENT_MASK_EXPOSURE | 
                         GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow (wid);
    GrMainLoop (event_handler);
}

void event_handler (GR_EVENT *event)
{
    switch (event->type)
    {
    case GR_EVENT_TYPE_EXPOSURE:
        GrText (wid, gc, 50, 50, "Hello World",  -1, GR_TFASCII);
        break;

    case GR_EVENT_TYPE_CLOSE_REQ:
        GrClose();
        exit (0);
    }
} 
