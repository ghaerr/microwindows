#include<stdio.h>
#include "../include/tnWidgets.h"

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}


void
u_func1 (TN_WIDGET * scrollbar, DATA_POINTER ptr)
{
  int event = tnGetLastScrollEvent (scrollbar);
  int position = tnGetThumbPosition (scrollbar);
  if (event == -1)
    return;
  if (event == TN_SCROLL_NOSCROLL)
    {
      printf ("No Scroll Event\n");
      return;
    }
  else if (event == TN_SCROLL_LINEUP)
    printf ("Line Up Event on vertical scrollbar\n");
  else if (event == TN_SCROLL_LINEDOWN)
    printf ("Line Down Event on vertical scrollbar\n");
  else if (event == TN_SCROLL_PAGEUP)
    printf ("Page Up Event on vertical scrollbar\n");
  else if (event == TN_SCROLL_PAGEDOWN)
    printf ("Page Down Event on vertical scrollbar\n");
  else if (event==TN_SCROLL_THUMBMOVE)
   printf ("Thumb Moved Event on vertical scrollbar\n");

  if (position == -1)
    return;
  printf ("Current Position on vertical scrollbar is: %d\n", position);
  fflush (stdout);
  return;
}

void
u_func2 (TN_WIDGET * scrollbar, DATA_POINTER ptr)
{
  int event = tnGetLastScrollEvent (scrollbar);
  int position = tnGetThumbPosition (scrollbar);
  if (event == -1)
    return;
  if (event == TN_SCROLL_NOSCROLL)
    {
      printf ("No Scroll Event\n");
      return;
    }
  else if (event == TN_SCROLL_LINEUP)
    printf ("Line Up Event on Horizontal scrollbar\n");
  else if (event == TN_SCROLL_LINEDOWN)
    printf ("Line Down Event on Horizontal scrollbar\n");
  else if (event == TN_SCROLL_PAGEUP)
    printf ("Page Up Event on Horizontal scrollbar\n");
  else if (event == TN_SCROLL_PAGEDOWN)
    printf ("Page Down Event on Horizontal scrollbar\n");
  else if (event==TN_SCROLL_THUMBMOVE)
	     printf ("Thumb Moved Event on Horizontal scrollbar\n");

  if (position == -1)
    return;
  printf ("Current Position on Horizontal scrollbar is: %d\n", position);
  fflush (stdout);
  return;
}




int
main (int argc, char **argv)
{

  TN_WIDGET *main_widget, *window1, *scrollbar1, *scrollbar2;
  main_widget = tnAppInitialize (argc, argv);
  window1 =
    tnCreateWidget (TN_WINDOW, main_widget, 50, 50, TN_HEIGHT, 300, TN_WIDTH,
		    400, TN_CAPTION, "Scrollbar Demo", TN_END);
  scrollbar1 =
    tnCreateWidget (TN_SCROLLBAR, window1, 50, 50,TN_MINVAL, 0, TN_MAXVAL, 100,TN_WIDTH,50, TN_END);
  scrollbar2 =
    tnCreateWidget (TN_SCROLLBAR, window1, 100, 190,TN_MINVAL,-200,TN_MAXVAL,200, TN_ORIENTATION, TN_HORIZONTAL,
		    TN_END);
  tnRegisterCallBack (scrollbar1, CLICKED, u_func1, NULL);
  tnRegisterCallBack (scrollbar2, CLICKED, u_func2, NULL);
  tnRegisterCallBack(window1,CLOSED,endapp,NULL);
  tnMainLoop ();
  return 0;
}
