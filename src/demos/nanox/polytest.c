#include <stdio.h>
#include "nano-X.h"

void draw(GR_EVENT * e)
{
  GR_GC_ID      gc;
  GR_POINT      points[4];

  int x = 10;
  int y = 10;
  int sz = 20;
  int sz2 = 5;

  gc = GrNewGC();

  GrSetGCBackground(gc, GR_RGB(0,0,0));
  //GrSetGCMode(gc, GR_MODE_XOR);

  points[0].x = x;
  points[0].y = y;

  points[1].x = x + sz;
  points[1].y = y;

  points[2].x = x + (sz/2) ;
  points[2].y = y + sz;

  GrSetGCForeground(gc, GR_RGB(255,255,255));
  GrFillPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,3,points);
  points[3].x = x;
  points[3].y = y;
  GrSetGCForeground(gc,GR_RGB(0,255,0));
  GrPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,4,points);

  y += sz + 10;

  points[0].x = x;
  points[0].y = y;

  points[1].x = x + sz + 1;
  points[1].y = y;

  points[2].x = x + (sz/2) ;
  points[2].y = y + sz;

  GrSetGCForeground(gc, GR_RGB(255,255,255));
  GrFillPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,3,points);
  points[3].x = x;
  points[3].y = y;
  GrSetGCForeground(gc,GR_RGB(0,255,0));
  GrPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,4,points);

  y += sz + 10;

  points[0].x = x;
  points[0].y = y;

  points[1].x = x + sz - 1;
  points[1].y = y;

  points[2].x = x + (sz/2) ;
  points[2].y = y + sz;

  GrSetGCForeground(gc, GR_RGB(255,255,255));
  GrFillPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,3,points);
  points[3].x = x;
  points[3].y = y;
  GrSetGCForeground(gc,GR_RGB(0,255,0));
  GrPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,4,points);

  /* draw right arrow*/
  sz = 10;
  sz2 = 8;

  x = 60;
  y = 60;

  points[0].x = x;
  points[0].y = y;

  y -= sz;

  points[1].x = x + sz2;
  points[1].y = y;

  y -= sz;

  points[2].x = x;
  points[2].y = y;

  GrSetGCForeground(gc, GR_RGB(255,255,255));
  GrFillPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,3,points);

  points[3].x = x;
  points[3].y = 60;

  GrSetGCForeground(gc,GR_RGB(0,255,0));
  GrPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,4,points);

  GrSetGCForeground(gc,GR_RGB(255,255,255));

  x = 60;
  y = 90;

  points[0].x = x;
  points[0].y = y;

  y -= sz;

  points[1].x = x + sz2;
  points[1].y = y;

  y -= sz;

  points[2].x = x;
  points[2].y = y;

  GrSetGCForeground(gc, GR_RGB(255,255,255));
  GrFillPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,3,points);
  points[3].x = x;
  points[3].y = 90;
  //GrPoly(((GR_EVENT_EXPOSURE*)e)->wid,gc,4,points);

  GrDestroyGC(gc);
}

main()
{
  GR_EVENT	event;
  GR_WINDOW_ID  w;

  if (GrOpen() < 0) {
    fprintf(stderr, "cannot open graphics\n");
    exit(1);
  }
	
  /* create window*/
  w = GrNewWindowEx(
		     GR_WM_PROPS_NOAUTOMOVE|GR_WM_PROPS_BORDER|GR_WM_PROPS_CAPTION|
		     GR_WM_PROPS_CLOSEBOX, "POLY FILL", GR_ROOT_WINDOW_ID, 
		     10, 10, 100, 300, GR_RGB(0,0,0));
  //  w = GrNewWindow(0,100,100,100,100,3,GR_RGB(0,0,255),GR_RGB(0,0,0));

  GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
  GrMapWindow(w);		
 
  while (1) {
    GrGetNextEvent(&event);

    switch (event.type) {
    case GR_EVENT_TYPE_EXPOSURE:
      draw(&event);
      break;
    case GR_EVENT_TYPE_CLOSE_REQ:
      GrClose();
      exit(0);
    }
  }

}
