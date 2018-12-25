#include<stdio.h>
#include "../include/tnWidgets.h"
TN_WIDGET *window1,*button2,*window5,*pbar;
int created;
void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void u_func3(TN_WIDGET *button,DATA_POINTER ptr)
{
	//tnSetEnabled(pbar,0);
	if(TN_ISVISIBLE(pbar)) 
		tnSetVisible(pbar,0);
	else
		tnSetVisible(pbar,1);
	return;
}
	

void u_func1(TN_WIDGET *button,DATA_POINTER ptr)
{
	
	static int vl=0;
	vl+=20;
	if (vl > 100) vl=0;
	//printf("\nin testp: %d",vl);
	printf("\n%*d, Push Button Clicked \n",3,vl);
	tnProgressBarUpdate(pbar,vl);
//	tnDestroyWidget(window1);
	return;
}

int main(int argc,char **argv)
{
	
	TN_WIDGET *main_widget,*button1,*button3;
	//label1=0;
	
	main_widget=tnAppInitialize(argc,argv);
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,400,TN_CAPTION,"Window",TN_END);
	button1=tnCreateWidget(TN_BUTTON,window1,50,50,TN_WIDTH, 100,TN_HEIGHT, 40,TN_FONTNAME,"arial", TN_CAPTION,"Ok",TN_END);
	button3=tnCreateWidget(TN_BUTTON,window1,200,50,40,TN_CAPTION, "Hide Me :)",TN_END);
	tnCreateWidget(TN_LABEL,window1,10,160,TN_CAPTION,"Label",TN_FGCOLOR,200,TN_END);	
	tnRegisterCallBack(button1,CLICKED,u_func1,NULL);
	tnRegisterCallBack(button3,CLICKED,u_func3,NULL);
	pbar=tnCreateWidget(TN_PROGRESSBAR,window1,100,200,TN_FILLCOLOR,GR_RGB(100,100,225),TN_WIDTH,200,TN_END);
	tnSetProgressBarFillColor(pbar,255);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

