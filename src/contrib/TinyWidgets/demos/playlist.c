#include<stdio.h>
#include "../include/tnWidgets.h"
#include<dirent.h>
#include<sys/stat.h>
#include<signal.h>
#include<unistd.h>

           struct dirent **namelist;
	   DIR *fdir;
           int n,dirs=0,files=0;
	   int pid;
	   char path[500]="/";	
	   char currpath[500]="/";
	   TN_WIDGET *main_widget,*listbox2,*scrollbar2;
	   struct stat stbuf;

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

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void freelist(char **str,int m)
{
	int i;
	for(i=0;i<m;i++)
		free(str[i]);
	free(str);
	return;
}

void buttonfunc(TN_WIDGET *button, DATA_POINTER dptr)
{
/*	TN_WIDGET *listbox1 = (TN_WIDGET *)dptr;
	char **str;
	char filename[500];
	int count,m,i;
	tnGetSelectedListItems(listbox1,&str,&count);
	if(!m)
		return;
	for(i=0;i<count;i++)
	{
		strcpy(filename,path);
		strcat(filename,str[i]);
		tnAddItemToListBox(listbox2,filename);
		tnSetSelectedItemInListBox(listbox2,filename,GR_TRUE);
	}
	freelist(str,count);
	return;*/
	char str[10] = "mpg123";
	char str2[500];
	char **str1;
	int count;
//	str[7] = '"';
//	str[8] = '\0';
//	strcat(str,path);
	strcpy(str2,path);
	tnGetSelectedListItems(listbox2,&str1,&count);
	if(!count)
		return;
//	printf("%s\n",str1[0]);
//	fflush(stdout);
//	for(i=strlen(str),j=0;str1[0][j]!='\0';i++,j++)
//		str[i] = str1[0][j];
	strcat(str2,str1[0]);
//	str[i] = '"';
//	str[i+1] = '\0';
	printf("%s  %s\n",str,str2);
	if((pid = fork()) == 0)
	  {
		  execlp(str,str,str2,NULL);
		  exit(0);
	  }
	return;
}
		
void listfunc1(TN_WIDGET *listbox,DATA_POINTER dptr)
{
	char **str;
	TN_WIDGET *scrollbar = (TN_WIDGET *)dptr;
	int m,i;
	tnGetSelectedListItems(listbox,&str,&m);
	if(!m)
		return;
	if(strcmp(str[0],".")==0)
	{
		tnSetSelectedListItem(listbox,str[0],GR_FALSE);
		return;
	}
	if(strcmp(str[0],"..") == 0)
	{
		i = strlen(path)-2;
		while(i && path[i] != '/')
			i--;
		path[i+1] = '\0';
	}
	else
	{
	  strcat(path,str[0]);
	  strcat(path,"/");
	}
	freelist(str,m);
	freedir();
	getdir(path);
	tnDeleteAllItemsFromListBox(listbox);
	tnDeleteAllItemsFromListBox(listbox2);
	dirs = 0;
	files = 0;
	for(i=0;i<n;i++)
	{
		strcpy(currpath,path);		
		strcat(currpath,namelist[i]->d_name);
		if((fdir=opendir(currpath))!=NULL)
		{
			closedir(fdir);
			tnAddItemToListBox(listbox,namelist[i]->d_name);
			dirs++;
		}
		else
			if(strstr(namelist[i]->d_name,"mp3")|| strstr(namelist[i]->d_name,"MP3"))
			{
				tnAddItemToListBox(listbox2,namelist[i]->d_name);
				files++;
			}
	}
	tnSetScrollRange(scrollbar,0,dirs < 15?0:dirs-15);
	tnSetScrollRange(scrollbar2,0,files < 15?0:files-15);
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

void buttonfunc1(TN_WIDGET *button,DATA_POINTER dptr)
{
	printf("%d\n",pid);
	if(pid!=0)
		kill(pid,SIGKILL);
	return;
}
			






int main(int argc,char **argv)
{

	
	TN_WIDGET *window,*listbox1,*scrollbar1,*listbox3,*scrollbar3,*button1,*button2;
	int i;
	
	main_widget=tnAppInitialize(argc,argv);
	window=tnCreateWidget(TN_WINDOW,main_widget,0,0,TN_HEIGHT,400,TN_WIDTH,480,TN_CAPTION,"Listbox demo",TN_END);
	listbox1=tnCreateWidget(TN_LISTBOX,window,50,50,TN_RESIZE,GR_FALSE,TN_HEIGHT,200,TN_WIDTH,80,TN_END);
	scrollbar1=tnCreateWidget(TN_SCROLLBAR,window,130,50,TN_HEIGHT,200,TN_WIDTH,20,TN_ORIENTATION,TN_VERTICAL,TN_END);
	listbox2=tnCreateWidget(TN_LISTBOX,window,200,50,TN_RESIZE,GR_FALSE,TN_HEIGHT,200,TN_WIDTH,80,TN_END);
	scrollbar2=tnCreateWidget(TN_SCROLLBAR,window,280,50,TN_HEIGHT,200,TN_WIDTH,20,TN_ORIENTATION,TN_VERTICAL,TN_END);
	listbox3=tnCreateWidget(TN_LISTBOX,window,350,50,TN_RESIZE,GR_FALSE,TN_HEIGHT,200,TN_WIDTH,80,TN_ENABLED,GR_FALSE,TN_END);
	scrollbar3=tnCreateWidget(TN_SCROLLBAR,window,430,50,TN_HEIGHT,200,TN_WIDTH,20,TN_ORIENTATION,TN_VERTICAL,TN_END);
	button1 = tnCreateWidget(TN_BUTTON,window,170,50,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION,">",TN_END);
	button2 = tnCreateWidget(TN_BUTTON,window,170,90,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION,"S",TN_END);
	getdir("/");
	for(i=0;i<n;i++)
	{	
		strcat(currpath,namelist[i]->d_name);
		if((fdir=opendir(currpath))!=NULL)
		{
			closedir(fdir);
			tnAddItemToListBox(listbox1,namelist[i]->d_name);
			dirs++;
		}
		else
			if(strstr(namelist[i]->d_name,"mp3") || strstr(namelist[i]->d_name,"MP3"))
			{
				tnAddItemToListBox(listbox2,namelist[i]->d_name);
				files++;
			}
		strcpy(currpath,path);
	}
			
	tnSetScrollRange(scrollbar1,0,dirs<15?0:dirs-15);
	tnSetScrollRange(scrollbar2,0,files<15?0:files-15);
	tnSetScrollRange(scrollbar3,0,0);

	tnRegisterCallBack(scrollbar1,CLICKED,scrollfunc,listbox1);
	tnRegisterCallBack(scrollbar2,CLICKED,scrollfunc,listbox2);
	tnRegisterCallBack(scrollbar3,CLICKED,scrollfunc,listbox3);
	tnRegisterCallBack(listbox1,CLICKED,listfunc1,scrollbar1);
	tnRegisterCallBack(button1,CLICKED,buttonfunc,listbox2);
	tnRegisterCallBack(button2,CLICKED,buttonfunc1,NULL);
	tnRegisterCallBack(window,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

