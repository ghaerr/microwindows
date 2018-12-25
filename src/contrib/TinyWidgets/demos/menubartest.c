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
  switch ((intptr_t)dptr) {
    case 1:
	printf("Opening File ...\n");
	break;
    case 2:
	printf("Saving File ...\n");
        break;
    case 3:
	printf("About ...\n");
	break;
    case 4:
	printf("Window selected ...\n");
	break;
    case 5:
	printf("Pushbutton ...\n");
	break;
    case 6:
	printf("Radiobutton ...\n");
	break;
    case 7:
	printf("PopUp Menu ...\n");
	break;
    case 8:
	printf("Cascade Menu ...\n");
	break;
    default:
	break;
  }
  fflush(stdout);
  return;
}

void u_func2(TN_WIDGET *w, DATA_POINTER dptr)
{
        printf("No Help Available\n");
        fflush(stdout);
}

void u_func3(TN_WIDGET *w, DATA_POINTER dptr)
{
	printf("Exiting...\n");
	fflush(stdout);
	tnDestroyWidget(window1);
	tnEndApp();
}
	

int main(int argc,char **argv)
{
	
	TN_WIDGET *main_widget,*popup1,*popup2,*mitem1,*mitem2,*mitem4,*mitem5,*mitem6,*cmenu1,*cmenu2,*cmenu3,*mitem7,*mitem8,*mitem9,*mitem10,*mitem11;
	mitem6=mitem7=mitem8=mitem9=mitem10=mitem11=0;
	
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

	tnRegisterCallBack(mitem1,CLICKED,u_func1,(void*)1); //Open
	tnRegisterCallBack(mitem5,CLICKED,u_func1,(void*)2); //Save
	tnRegisterCallBack(mitem6,CLICKED,u_func3,NULL); //Exit
	
	tnRegisterCallBack(mitem2,CLICKED,u_func1,(void*)3); //About - disabled
	tnRegisterCallBack(mitem4,CLICKED,u_func2,NULL); //Help
	tnRegisterCallBack(mitem7,CLICKED,u_func1,(void*)4); //Window
	tnRegisterCallBack(mitem8,CLICKED,u_func1,(void*)5); //PushB
	tnRegisterCallBack(mitem9,CLICKED,u_func1,(void*)6); //RadioB
	tnRegisterCallBack(mitem10,CLICKED,u_func1,(void*)7); //PopUp
	tnRegisterCallBack(mitem11,CLICKED,u_func1,(void*)8); //Cascade - disabled
	
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

