#include<stdio.h>
#include<string.h>
#include "../include/tnWidgets.h"

#define TEST 0

#define SWAP(a,b,t) ((t)=(a), (a)=(b), (b)=(t)) //t=temp variable

TN_WIDGET *rawwidget1, *label1, *scrollbar1, *scrollbar2, *scrollbar3;
TN_WIDGET *colors16[16];
GR_COLOR red=64,green=128,blue=128;
char rgbcolor[32];

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void showcolor() 
{
  GrSetGCForeground(rawwidget1->gc,GR_RGB(red,green,blue));
  GrFillRect(rawwidget1->wid,rawwidget1->gc,0,0,300,50);
  sprintf(rgbcolor,"Red Green Blue: %3d, %3d, %3d",red,green,blue);
  tnSetLabelCaption(label1, rgbcolor);
  return;
}

int testoutput(int event, int position)
{
  if (event == TN_SCROLL_NOSCROLL)
    { printf ("No Scroll Event\n"); return 1; }
  else if (event == TN_SCROLL_LINEUP)
    printf ("Line Up Event scrollbar \n");
  else if (event == TN_SCROLL_LINEDOWN)
    printf ("Line Down Event scrollbar\n");
  else if (event == TN_SCROLL_PAGEUP)
    printf ("Page Up Event scrollbar \n");
  else if (event == TN_SCROLL_PAGEDOWN)
    printf ("Page Down Event scrollbar \n");
  else if (event==TN_SCROLL_THUMBMOVE)
   printf ("Thumb Moved Event scrollbar\n");

  if (position == -1) return -1; 
  printf ("Current Position scrollbar is: %d\n", position);
  fflush (stdout);
  return 0;
}

void
u_func1 (TN_WIDGET * scrollbar, DATA_POINTER ptr)
{
  int event = tnGetLastScrollEvent (scrollbar);
  red = tnGetThumbPosition (scrollbar);
  if (event == -1) return;
  if (red == -1) return;
#if TEST  
  int result = testoutput(event, red);
#endif  
  showcolor();
  return;
}

void
u_func2 (TN_WIDGET * scrollbar, DATA_POINTER ptr)
{
  int event = tnGetLastScrollEvent (scrollbar);
  green = tnGetThumbPosition (scrollbar);
  if (event == -1) return;
  if (green == -1) return;
#if TEST  
  int result = testoutput(event, green);
#endif  
  showcolor();
  return;
}

void
u_func3 (TN_WIDGET * scrollbar, DATA_POINTER ptr)
{
  int event = tnGetLastScrollEvent (scrollbar);
  blue = tnGetThumbPosition (scrollbar);
  if (event == -1) return;
  if (blue == -1) return;
#if TEST  
  int result = testoutput(event, blue);
#endif  
  showcolor();
  return;
}

void u_func4(TN_WIDGET *w, DATA_POINTER dptr)
{ 
  switch ((intptr_t)dptr) {
    case 0:
        red=0;green=0;blue=0;
	break;
    case 1:
	red=128;green=0;blue=0;
	break;
    case 2:
	red=0;green=128;blue=0;
        break;
    case 3:
	red=0;green=0;blue=128;
	break;
    case 4:
	red=255;green=0;blue=0;
	break;
    case 5:
	red=0;green=255;blue=0;
	break;
    case 6:
	red=0;green=0;blue=255;
	break;
    case 7:
	red=255;green=0;blue=255;
	break;
    case 8:
	red=0;green=255;blue=255;
	break;
    case 9:
	red=255;green=255;blue=0;
	break;
    case 10:
	red=128;green=128;blue=0;
	break;
    case 11:
	red=128;green=0;blue=128;
	break;
    case 12:
	red=0;green=128;blue=128;
	break;
    case 13:
	red=128;green=128;blue=128;
	break;
    case 14:
	red=255;green=255;blue=255;
	break;
    case 15:
	red=192;green=192;blue=192;
	break;
    default:
	break;
  }
  	tnSetThumbPosition(scrollbar1,red);
	tnSetThumbPosition(scrollbar2,green);
	tnSetThumbPosition(scrollbar3,blue);
	showcolor();

  return;
}

void u_func5(TN_WIDGET *widget,DATA_POINTER ptr)
{
  showcolor();
}
	



int main(int argc,char **argv)
{

	TN_WIDGET *main_widget, *window1;
	int r=0,g=0,b=0,t=0,i=0;
	
	main_widget=tnAppInitialize(argc,argv);
	
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_WIDTH,400,TN_HEIGHT,340,TN_CAPTION,"Colorpicker",TN_END);
	
	rawwidget1=tnCreateWidget(TN_RAWWIDGET,window1,50,50,TN_WIDTH,300,TN_HEIGHT,50,TN_BGCOLOR,GR_RGB(128,128,128),TN_END);
	
	scrollbar1 = tnCreateWidget (TN_SCROLLBAR, window1, 50, 150,TN_MINVAL, 0, TN_MAXVAL, 255, TN_WIDTH, 300, TN_ORIENTATION, TN_HORIZONTAL, TN_LINESTEP,1, TN_PAGESTEP, 10, TN_END);
	scrollbar2 = tnCreateWidget (TN_SCROLLBAR, window1, 50, 190,TN_MINVAL, 0 ,TN_MAXVAL, 255, TN_WIDTH, 300, TN_ORIENTATION, TN_HORIZONTAL, TN_LINESTEP,1, TN_PAGESTEP, 10, TN_END);
	scrollbar3 = tnCreateWidget (TN_SCROLLBAR, window1, 50, 230,TN_MINVAL, 0 ,TN_MAXVAL, 255, TN_WIDTH, 300, TN_ORIENTATION, TN_HORIZONTAL, TN_LINESTEP,1, TN_PAGESTEP, 10, TN_END);
  
	tnSetThumbPosition(scrollbar1,64);
	tnSetThumbPosition(scrollbar2,128);
	tnSetThumbPosition(scrollbar3,128);
	
	tnCreateWidget(TN_BUTTON,window1,20,150,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION," ",TN_BGCOLOR,GR_RGB(255,0,0),TN_END);
	tnCreateWidget(TN_BUTTON,window1,20,190,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION," ",TN_BGCOLOR,GR_RGB(0,255,0),TN_END);
	tnCreateWidget(TN_BUTTON,window1,20,230,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION," ",TN_BGCOLOR,GR_RGB(0,0,255),TN_END);
	
	label1=tnCreateWidget(TN_LABEL,window1,50,120,TN_CAPTION,"Red Green Blue: 128, 128, 128",TN_FGCOLOR, GR_RGB(0,0,0),TN_END);	
	//tnRegisterCallBack(label1,CLICKED,u_func4,NULL);
	
	tnCreateWidget(TN_LABEL,window1,30,270,TN_CAPTION,"Select color:",TN_FGCOLOR, GR_RGB(0,0,0),TN_END);	
	
	for(i=0;i<15;i++)
		{
			colors16[i] = tnCreateWidget(TN_BUTTON,window1,30+i*20,290,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION," ",TN_BGCOLOR,GR_RGB(r*127,g*127,b*127),TN_END);
			if (i<3) {
			  if (b==0 && g==1) SWAP(b,g,t);
			  if (g==0 && r==1) SWAP(r,g,t);
			  if (g==0 && b==0) r=1;
			} else if (i<6) {
			  if (b==0 && g==2) SWAP(b,g,t);
			  if (g==0 && r==2) SWAP(r,g,t);
			  if (g==0 && b==1) {r=2;b=0;}
			} else if (i<9) {
			  if (b==2 && g==2) SWAP(r,b,t);
			  if (g==0 && r==2) SWAP(r,g,t);
			  if (g==0 && b==2) {r=2;}
			} else if (i<12) {
			  if (i==9) printf("\n");
			  if (r==1 && b==1) SWAP(r,g,t);
			  if (g==1 && r==1) SWAP(b,g,t);
			  if (g==2 && b==0) {r=1;g=1;}
			} else {
			  if (i==12) r=1;
			  if (i==13) {r=2;g=2;b=2;}
			}
			//printf("i=%2d:%d,%d,%d\n",i+1,r,g,b);
			tnRegisterCallBack(colors16[i],CLICKED,u_func4,(void*)(long)i);
		}
	colors16[15] = tnCreateWidget(TN_BUTTON,window1,30+15*20,290,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION," ",TN_BGCOLOR,GR_RGB(192,192,192),TN_END);
	tnRegisterCallBack(colors16[15],CLICKED,u_func4,(void*)15);
	
	tnRegisterCallBack (scrollbar1, CLICKED, u_func1, NULL);
	tnRegisterCallBack (scrollbar2, CLICKED, u_func2, NULL);
	tnRegisterCallBack (scrollbar3, CLICKED, u_func3, NULL);
	
	tnRegisterCallBack(rawwidget1,TN_SYSTEM_EVENT_EXPOSURE,u_func5,NULL);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	
	tnMainLoop();
	return 0;
}

