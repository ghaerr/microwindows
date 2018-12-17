#include<stdio.h>
#include "../include/tnWidgets.h"
#include <dirent.h>
           struct dirent **namelist;
           int n;
	   char currpath[500] = "";
	   char path[500]="";	
	   TN_WIDGET *picture,*window2=NULL,*main_widget;

int getdir(char *dir)
{ 
           n = scandir(dir, &namelist, 0, alphasort);
           if (n < 0)
		return -1;
           else
		return 1;
}

void freedir(void)
{
	int i;
	for(i=0; i < n; i++)
		free(namelist[i]);
	free(namelist);
	return;
}

void freelist(char **str,int m)
{
	int i;
	for(i=0;i<m;i++)
		free(str[i]);
	free(str);
	return;
}

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void listfunc(TN_WIDGET *listbox,DATA_POINTER dptr)
{
	char **str;
	DIR *fdir;
	char t[500] = "/";
	TN_WIDGET *scrollbar = (TN_WIDGET *)dptr;
	int m,i;
	tnGetSelectedListItems(listbox,&str,&m);
	if(!m)
		return;
	for(i=0; i < m-1; i++)
		tnSetSelectedListItem(listbox,str[i],GR_FALSE);
	
	strcat(path,strcat(t,str[m-1]));
	fdir = opendir(path);
	if(fdir == NULL)
	{
		if(window2)
		  	tnDestroyWidget(window2);
		 window2 = tnCreateWidget(TN_WINDOW,main_widget,100,100,TN_CAPTION,path,TN_HEIGHT,400,TN_WIDTH,300,TN_END);
  	         picture=tnCreateWidget(TN_PICTURE,window2,0,0,TN_STRETCH,GR_TRUE,TN_FILENAME,path,TN_END);
		strcpy(path,currpath);
	}
	else
	{
		closedir(fdir);
		strcpy(currpath,path);
		freedir();
		getdir(currpath);
		tnDeleteAllItemsFromListBox(listbox);
		for(i=0;i<n; i++)
		  tnAddItemToListBox(listbox,namelist[i]->d_name);
		tnSetScrollRange(scrollbar,0,n<15?0:n-15);	
		tnSetThumbPosition(scrollbar,0);  
	}
	freelist(str,m);
	
	return;
}

void scrollfunc(TN_WIDGET *scrollbar,DATA_POINTER dptr)
{
	int position = tnGetThumbPosition (scrollbar);
	TN_WIDGET *lbox = (TN_WIDGET *)dptr;
	int currtop = tnGetListTop(lbox);
	if(position < currtop)
		tnListItemsLineDown(lbox,currtop-position);
	if(position > currtop)
		tnListItemsLineUp(lbox,position-currtop);
	return;
}
			






int main(int argc,char **argv)
{

	
	TN_WIDGET *window1,*listbox,*scrollbar;
	int i;
	
	main_widget=tnAppInitialize(argc,argv);
	window1=tnCreateWidget(TN_WINDOW,main_widget,0,0,TN_HEIGHT,400,TN_WIDTH,480,TN_CAPTION,"Listbox demo",TN_END);
	listbox=tnCreateWidget(TN_LISTBOX,window1,50,50,TN_RESIZE,GR_FALSE,TN_HEIGHT,200,TN_WIDTH,80,TN_END);
	scrollbar=tnCreateWidget(TN_SCROLLBAR,window1,130,50,TN_HEIGHT,200,TN_WIDTH,20,TN_ORIENTATION,TN_VERTICAL,TN_END);
	getdir("/");
	for(i=0;i<n;i++)
		tnAddItemToListBox(listbox,namelist[i]->d_name);
	tnSetScrollRange(scrollbar,0,n<15?0:n-15);

	tnRegisterCallBack(scrollbar,CLICKED,scrollfunc,listbox);
	tnRegisterCallBack(listbox,CLICKED,listfunc,scrollbar);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	tnMainLoop();
	
	return 0;
}

