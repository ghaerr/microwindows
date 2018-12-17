#include<stdio.h>
#include "../include/tnWidgets.h"
TN_WIDGET *window1,*menubar1;
void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void u_func1(TN_WIDGET *w, DATA_POINTER dptr)
{
	printf("\nOpening File ...");
		fflush(stdout);
}

void u_func2(TN_WIDGET *w, DATA_POINTER dptr)
{
	        printf("\nNo Help Available");
                fflush(stdout);
}

void u_func3(TN_WIDGET *w, DATA_POINTER dptr)
{
	printf("\nExiting...");
	fflush(stdout);
	tnDestroyWidget(window1);
}
	

int main(int argc,char **argv)
{
	
	TN_WIDGET *main_widget,*popup1,*popup2,*mitem1,*mitem2,*mitem4,*mitem5,*mitem6,*cmenu1,*cmenu2,*cmenu3,*mitem7,*mitem8,*mitem9,*mitem10,*mitem11;
	
	main_widget=tnAppInitialize(argc,argv);
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,400,TN_CAPTION,"Pull Down Menus",TN_END);
	menubar1=tnCreateWidget(TN_MENUBAR,window1,0,0,TN_END);
	popup1=tnCreateWidget(TN_POPUPMENU,menubar1,TN_AUTO,TN_AUTO,TN_CAPTION,"File",TN_ENABLED,1,TN_VISIBLE,GR_TRUE,TN_END);
        popup2=tnCreateWidget(TN_POPUPMENU,menubar1,TN_AUTO,TN_AUTO,TN_CAPTION,"Tools",TN_END);
	mitem2=tnCreateWidget(TN_MENUITEM,menubar1,TN_AUTO,TN_AUTO,TN_CAPTION,"About",TN_ENABLED,0,TN_END);
	mitem4=tnCreateWidget(TN_MENUITEM,menubar1,TN_AUTO,TN_AUTO,TN_CAPTION,"Help",TN_END);

	mitem1=tnCreateWidget(TN_MENUITEM,popup1,TN_AUTO,TN_AUTO,TN_CAPTION,"Open",TN_END);
	mitem5=tnCreateWidget(TN_MENUITEM,popup1,TN_AUTO,TN_AUTO,TN_CAPTION,"Save",TN_CHECKABLE,1,TN_END);
	mitem6=tnCreateWidget(TN_MENUITEM,popup1,TN_AUTO,TN_AUTO,TN_CAPTION,"Exit",TN_END);
	
	cmenu1=tnCreateWidget(TN_CASCADEMENU,popup2,TN_AUTO,TN_AUTO,TN_CAPTION,"Widget",TN_END);

	mitem7=tnCreateWidget(TN_MENUITEM,cmenu1,TN_AUTO,TN_AUTO,TN_CAPTION,"Window",TN_END);
	cmenu2=tnCreateWidget(TN_CASCADEMENU,cmenu1,TN_AUTO,TN_AUTO,TN_CAPTION,"Button",TN_END);

	mitem8=tnCreateWidget(TN_MENUITEM,cmenu2,TN_AUTO,TN_AUTO,TN_CAPTION,"PushB",TN_CHECKABLE,1,TN_END);
	mitem9=tnCreateWidget(TN_MENUITEM,cmenu2,TN_AUTO,TN_AUTO,TN_CAPTION,"RadioB",TN_CHECKABLE,1,TN_END);
	
	cmenu3=tnCreateWidget(TN_CASCADEMENU,cmenu2,TN_AUTO,TN_AUTO,TN_CAPTION,"Menu",TN_EXCLUSIVE,1,TN_END);
	mitem10=tnCreateWidget(TN_MENUITEM,cmenu3,TN_AUTO,TN_AUTO,TN_CAPTION,"PopUp",TN_CHECKABLE,1,TN_END);
	mitem11=tnCreateWidget(TN_MENUITEM,cmenu3,TN_AUTO,TN_AUTO,TN_CAPTION,"Cascade",TN_CHECKABLE,1,TN_ENABLED,0,TN_END);

	tnRegisterCallBack(mitem1,CLICKED,u_func1,NULL);		
	tnRegisterCallBack(mitem4,CLICKED,u_func2,NULL);
	tnRegisterCallBack(mitem2,CLICKED,u_func1,NULL);
	tnRegisterCallBack(mitem6,CLICKED,u_func3,NULL);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

