#include<stdio.h>
#include "../include/tnWidgets.h"

TN_WIDGET *main_widget,*lbox1,*window1,*window2=NULL,*button1,*button2,*textbox1,*label1,*label2;
void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void u_func2(TN_WIDGET *button,DATA_POINTER ptr)
{
	char *name=tnGetText(textbox1);
	tnDestroyWidget(window2);
	window2=NULL;
	tnAddItemToListBox(lbox1,name);
	free(name);
	return;
}
void u_func1(TN_WIDGET *button,DATA_POINTER ptr)
{
	if(window2!=NULL)
		return;
	window2=tnCreateWidget(TN_WINDOW,main_widget,75,70,TN_HEIGHT,150,TN_WIDTH,200,TN_CAPTION,"Input box",TN_END);
	label2=tnCreateWidget(TN_LABEL,window2,50,10,TN_CAPTION,"Please enter your name",TN_END);
	textbox1=tnCreateWidget(TN_TEXTBOX,window2,50,50,TN_END);
	button2=tnCreateWidget(TN_BUTTON,window2,50,100,TN_CAPTION,"OK",TN_END);
	
	tnRegisterCallBack(button2,CLICKED,u_func2,NULL);
	return;
}

int main(int argc,char **argv)
{

	
	main_widget=tnAppInitialize(argc,argv);
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,400,TN_CAPTION,"Hello folks demo",TN_END);
	label1=tnCreateWidget(TN_LABEL,window1,60,10,TN_CAPTION,"Visitors in Wonderland",TN_FONTNAME,"impact",TN_FONTSIZE,15,TN_END);
	lbox1=tnCreateWidget(TN_LISTBOX,window1,50,50,TN_END);
	button1=tnCreateWidget(TN_BUTTON,window1,50,150,TN_CAPTION,"Click Me!",TN_END);
	tnRegisterCallBack(button1,CLICKED,u_func1,NULL);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

