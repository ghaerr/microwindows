#include<stdio.h>
#include "../include/tnWidgets.h"
TN_WIDGET *window1,*button2,*window5,*button1;
int created;

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}


				
void u_func2(TN_WIDGET *c, DATA_POINTER p)
{
	if(TN_CHECKBUTTONACTIVE(c))	
	{
		tnSetEnabled(button1,0);
		printf("Button checked\n");
		tnSetWindowTitle((TN_WIDGET *)p,"Button checked");
		fflush(stdout);
	}
	else
	{
		tnSetEnabled(button1,1);
		printf("Button unchecked\n");
		tnSetWindowTitle((TN_WIDGET *)p,"Button unchecked");
		fflush(stdout);
	}
}

void u_func1(TN_WIDGET *button,DATA_POINTER ptr)
{
	//tnSetVisible(button2,GR_TRUE);
	system("../../bin/demo-grabkey &");
	return;
}

void u_func3(TN_WIDGET *button,DATA_POINTER ptr)
{
	//        tnSetVisible(button1,GR_FALSE);
	system("../../bin/nxterm &");
	tnRemoveButtonPixmap(button);
	tnSetButtonCaption(button,"Nxterm");
	        return;
}

void u_func4(TN_WIDGET *label,DATA_POINTER ptr)
{
	printf("Label clicked\n");
	fflush(stdout);
}

int main(int argc,char **argv)
{
	
	TN_WIDGET *main_widget,*label1,*chkbutton ;
	
	main_widget=tnAppInitialize(argc,argv);
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,400,TN_CAPTION,"Launchtest",TN_END);
	tnCreateWidget(TN_LABEL,window1,50,20,TN_CAPTION,"Please click an icon to launch a program",TN_FGCOLOR, GR_RGB(255,0,0),TN_END);	
	button1=tnCreateWidget(TN_BUTTON,window1,50,50,TN_WIDTH, 60,TN_HEIGHT, 60,TN_CAPTION, "Grabdemo",TN_PIXMAP,1,TN_FILENAME,"demos/bmp/notepad.xpm",TN_END);
	button2=tnCreateWidget(TN_BUTTON,window1,50,120,TN_WIDTH, 60,TN_HEIGHT, 60,TN_CAPTION, "NXterm",TN_PIXMAP,1,TN_FILENAME,"demos/bmp/linuxterm.xpm",TN_VISIBLE,1,TN_END);
	chkbutton=tnCreateWidget(TN_CHECKBUTTON,window1,50,200,TN_CAPTION, "Check",TN_FONTNAME ,"arial",TN_END);
	label1=tnCreateWidget(TN_LABEL,window1,50,270,TN_CAPTION,"****************************************",TN_FGCOLOR, GR_RGB(0,0,255),TN_END);	
	tnRegisterCallBack(button1,CLICKED,u_func1,NULL);
	tnRegisterCallBack(chkbutton,CLICKED,u_func2,window1);
	tnRegisterCallBack(button2,CLICKED,u_func3,NULL);
	tnRegisterCallBack(label1,CLICKED,u_func4,NULL);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);	 
	tnMainLoop();
	return 0;
}

