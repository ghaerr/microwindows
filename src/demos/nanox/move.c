#include <stdio.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
/*
 * Demo to test child window movement and redrawing
 */
int 
main(int ac,char **av)
{
  GR_COORD      offset_x = 0, offset_y = 0;
  GR_WINDOW_ID 	window1, subwindow1, subsubwin1;
  GR_WINDOW_ID 	window2, subwindow2;
  /*GR_WINDOW_ID	subsubwin2;*/
  GR_EVENT 	event;

  fprintf(stderr,"This is a demo program.\n");
  fprintf(stderr,"Left-button drags window\n");
  fprintf(stderr,"Right-button raises window\n");
  
  GrOpen();
  window1 = GrNewWindow(GR_ROOT_WINDOW_ID, 20, 20, 100, 60, 4, BLACK, BLUE);
  subwindow1 = GrNewWindow(window1, 5, 5, 90, 50, 4, WHITE, RED);
  subsubwin1 = GrNewWindow(subwindow1, 10, 10, 10, 10, 2, GREEN, BLUE);

  window2 = GrNewWindow(GR_ROOT_WINDOW_ID, 20, 100, 100, 60, 4, BLACK, BLUE);
  subwindow2 = GrNewWindow(window2, 5, 5, 90, 50, 4, WHITE, RED);
/*    subsubwin2 = GrNewWindow(subwindow2, 10, 10, 10, 10, 2, GREEN, BLUE); */

  GrSelectEvents(window1, 
  		 GR_EVENT_MASK_EXPOSURE |
		 GR_EVENT_MASK_BUTTON_DOWN |
		 GR_EVENT_MASK_BUTTON_UP |
		 GR_EVENT_MASK_MOUSE_ENTER |
		 GR_EVENT_MASK_MOUSE_EXIT |
		 GR_EVENT_MASK_MOUSE_MOTION |
		 GR_EVENT_MASK_CLOSE_REQ);

  GrSelectEvents(window2, 
  		 GR_EVENT_MASK_EXPOSURE |
		 GR_EVENT_MASK_BUTTON_DOWN |
		 GR_EVENT_MASK_BUTTON_UP |
		 GR_EVENT_MASK_MOUSE_ENTER |
		 GR_EVENT_MASK_MOUSE_EXIT |
		 GR_EVENT_MASK_MOUSE_MOTION |
		 GR_EVENT_MASK_CLOSE_REQ);

  GrSelectEvents(subsubwin1, 
		 GR_EVENT_MASK_BUTTON_DOWN |
		 0);

  GrMapWindow(subsubwin1);
  GrMapWindow(subwindow1);
  GrMapWindow(window1);

  /*GrMapWindow(subsubwin2);*/
  GrMapWindow(subwindow2);
  GrMapWindow(window2);
  
  while(1) {
    GrGetNextEvent(&event);
    
    switch (event.type) {
    case GR_EVENT_TYPE_NONE:
      break;
    case GR_EVENT_TYPE_BUTTON_DOWN:
	offset_x = event.button.x;
	offset_y = event.button.y;

      if (event.button.changebuttons & GR_BUTTON_R) {
	GrRaiseWindow(event.button.wid);
      }
      if (event.button.wid == subsubwin1) {
	GR_WINDOW_INFO winfo;
	GrGetWindowInfo(subsubwin1, &winfo);
	if (winfo.parent == subwindow1) {
	  GrReparentWindow(subsubwin1, subwindow2, 10, 10);
	} else {
	  GrReparentWindow(subsubwin1, subwindow1, 10, 10);
	}
      }
    case GR_EVENT_TYPE_MOUSE_MOTION:
      if (event.mouse.buttons == GR_BUTTON_L && 
	  (event.mouse.wid == window1 || event.mouse.wid == window2)) {
	GrMoveWindow(event.mouse.wid, 
		     event.mouse.rootx - offset_x, 
		     event.mouse.rooty - offset_y);
      }
      if (event.mouse.buttons == GR_BUTTON_R) {
	GrResizeWindow(event.mouse.wid, 
		     event.mouse.x + 1, 
		     event.mouse.y + 1);
      }
      break;
    case GR_EVENT_TYPE_EXPOSURE:
      /*GrFillRect(event.exposure.wid, defgc,
	event.exposure.x, event.exposure.y,
	event.exposure.width, event.exposure.height);*/
      break;
    case GR_EVENT_TYPE_CLOSE_REQ:
      GrClose();
      exit(0);
    default:
      fprintf(stderr, "%d\n", event.type);
    }
  }
  GrClose();
}
