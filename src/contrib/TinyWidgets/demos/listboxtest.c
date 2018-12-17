#include<stdio.h>
#include "../include/tnWidgets.h"

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void u_func1(TN_WIDGET *listbox,DATA_POINTER ptr)
{
	char **selected_strings;
	int count;
	int i;
	int *pos;
	
	printf("Selected items as strings are :\n");
	tnGetSelectedListItems(listbox,&selected_strings,&count);
	if(count==-1) return;	
	for(i=0;i<count;i++)
		printf("%s\n",selected_strings[i]);
	tnDeleteItemFromListBox(listbox,selected_strings[0]);
	if(count)
	tnSetText((TN_WIDGET *)ptr,selected_strings[0]);
		
	printf("----------------------------\n");
	printf("Selected item indices are : \n");
	tnGetSelectedListPos(listbox,&pos,&count);
	for(i=0;i<count;i++)
		printf("%d\n",pos[i]);
	free(pos);	
	
	return;
}





int main(int argc,char **argv)
{

	char *str[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
	
	TN_WIDGET *main_widget,*lbox1,*window1,*textbox1;
	
	main_widget=tnAppInitialize(argc,argv);
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,400,TN_CAPTION,"Listbox demo",TN_END);
	lbox1=tnCreateWidget(TN_LISTBOX,window1,50,50,TN_LISTITEMS,str,TN_COUNT,12,TN_END);
	textbox1=tnCreateWidget(TN_TEXTBOX,window1,200,50,TN_END);
	tnRegisterCallBack(lbox1,CLICKED,u_func1,textbox1);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

