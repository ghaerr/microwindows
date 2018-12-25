#include<stdio.h>
#include "../include/tnWidgets.h"

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void do_mousedown(TN_WIDGET *widget,DATA_POINTER ptr)
{
	int i;
	for(i=0;i<40;i++)
	{
		GrSetGCForeground((((TN_WIDGET **)ptr)[i])->gc,GR_RGB(255,0,0));
		GrFillRect((((TN_WIDGET**)ptr)[i])->wid,(((TN_WIDGET**)ptr)[i])->gc,0,0,5,20);
	}
}

void do_mouseup(TN_WIDGET *widget,DATA_POINTER ptr)
{
        int i;
	for(i=0;i<40;i++)
	{
		GrSetGCForeground((((TN_WIDGET **)ptr)[i])->gc,GR_RGB(0,255,0));
        	GrFillRect((((TN_WIDGET **)ptr)[i])->wid,(((TN_WIDGET **)ptr)[i])->gc,0,0,5,20);
	}
}

void do_mousemove(TN_WIDGET *widget,DATA_POINTER ptr)
{
	GR_EVENT *event;
	event=tnGetLastEvent(widget);
	GrSetGCForeground(widget->gc,GR_RGB(100,100,255));
	GrFillEllipse(widget->wid,widget->gc,event->button.x,event->button.y,2,2);

}
	



int main(int argc,char **argv)
{

	TN_WIDGET *main_widget,*rawwidget1,*window1,*rawwidget2[40];
	int xpos,i;
	
	main_widget=tnAppInitialize(argc,argv);
	
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,400,TN_CAPTION,"Raw Widget demo",TN_END);
	
	rawwidget1=tnCreateWidget(TN_RAWWIDGET,window1,50,50,TN_HEIGHT,200,TN_WIDTH,300,TN_BGCOLOR,GR_RGB(255,255,255),TN_END);
	
	for(i=0,xpos=10;i<40;i++,xpos+=10)
		rawwidget2[i]=tnCreateWidget(TN_RAWWIDGET,window1,xpos,260,TN_HEIGHT,20,TN_WIDTH,5,TN_BGCOLOR,GR_RGB(0,255,0),TN_END);
	
	tnCreateWidget(TN_LABEL,rawwidget1,50,50,TN_CAPTION,"Click or move the pointer around in this area",TN_END);
	tnRegisterCallBack(rawwidget1,TN_SYSTEM_EVENT_BUTTON_DOWN,do_mousedown,rawwidget2);
	tnRegisterCallBack(rawwidget1,TN_SYSTEM_EVENT_MOUSE_MOTION,do_mousemove,NULL);
	tnRegisterCallBack(rawwidget1,TN_SYSTEM_EVENT_BUTTON_UP,do_mouseup,rawwidget2);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	
	tnMainLoop();
	return 0;
}

