#include<stdio.h>
#include "../include/tnWidgets.h"
TN_WIDGET *window1, *button2, *window5;
int created;
void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}


void
u_func2 (TN_WIDGET * textbox, DATA_POINTER p)
{
  char *s;
  s = tnGetText (textbox);
//  printf ("String Length : %d\n", strlen (s));
  fflush (stdout);
  free (s);
  return;

}

void
u_func3 (TN_WIDGET * textbox, DATA_POINTER ptr)
{
  //printf("Got Focus\n");
	return;
}

void
u_func1 (TN_WIDGET * textbox, DATA_POINTER ptr)
{
  char *s;
  s = tnGetText (textbox);
  //if(s==NULL) 
//	  printf("\nNULL");
//  else 
//	  printf ("Text is: %s  Len : %d\n", s,strlen(s));
  fflush (stdout);
  free (s);
  s=NULL;
  return;
}





int
main (int argc, char **argv)
{

  TN_WIDGET *main_widget, *textbox1, *window1;
  char s[]="Amit Kulkarni is the best. i am the king. 123456789 @amms@Videsh Sanchar Nigam Limited, Freshmeat.net http://www.amitkulkarni.com Redhat Linux Amit Kulkarni is the best. i am the king. 123456789 @amms@ Videsh Sanchar Nigam Limited, Freshmeat.net http://www.amitkulkarni.com Redhat LinuxAmit Kulkarni is the best. i am the king. 123456789 @amms@ Videsh Sanchar Nigam Limited, Freshmeat.net http://www.amitkulkarni.com Redhat Linux Amit Kulkarni is the best. i am the king. 123456789 @amms@ Videsh Sanchar Nigam Limited, Freshmeat.net http://www.amitkulkarni.com Redhat Linux Videsh Sanchar Nigam Limited, Freshmeat.net http://www.amitkulkarni.com Redhat LinuxVidesh Sanchar Nigam Limited, Freshmeat.net http://www.amitkulkarni.com Redhat Linux@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
  main_widget = tnAppInitialize (argc, argv);
  window1 =
    tnCreateWidget (TN_WINDOW, main_widget, 50, 50, TN_HEIGHT, 300, TN_WIDTH,
		    400, TN_CAPTION, "Textbox Demo",TN_END);
  textbox1 =
    tnCreateWidget (TN_TEXTBOX, window1, 50, 50,TN_TEXTBOXTYPE,TN_MULTI_LINE,TN_DEFAULTTEXT, s,TN_WIDTH, 300,TN_HEIGHT,200,TN_END);
  tnRegisterCallBack (textbox1, MODIFIED, u_func1, NULL);
  tnRegisterCallBack (textbox1, LOSTFOCUS, u_func2, NULL);
  tnRegisterCallBack (textbox1, GOTFOCUS, u_func3, NULL);
  tnRegisterCallBack(window1,CLOSED,endapp,NULL);
  tnMainLoop ();
  return 0;
}
