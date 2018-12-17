#include "main.h"
#include<time.h>
#include<dirent.h>
#include<string.h>
#include<unistd.h>

 struct dirent **namelist;
 DIR *fdir;
 int n,dirs=0;
 char path[500]="/";	
 char currpath[500]="/";
	   
void saveokclicked(TN_WIDGET *widget,DATA_POINTER d)
{
	if(project_dir)
		free(project_dir);
	project_dir=tnGetText((TN_WIDGET *)d);
	project_dir[strlen(project_dir)-1]='\0';
	tnDestroyWidget(GetParent(widget));
	switch(projectstatus)
	{
		case TND_CODEGEN:
			build_code(NULL,NULL);
			break;

		case TND_SAVE:
			SaveProject();
			break;

		case TND_LOAD:
			loadrc();
			break;
	}		
}

void savecancelclicked(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(GetParent(widget));
}

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

void savelboxclicked(TN_WIDGET *widget,DATA_POINTER dptr)
{
	TN_WIDGET *textbox = (TN_WIDGET *)dptr;
	TN_WIDGET *scrollbar = (TN_WIDGET *)tnGetAttachedData(widget);
	char **str;
	int m,i;
	tnGetSelectedListItems(widget,&str,&m);
	if(!m)
		return;
	if(strcmp(str[0],".")==0)
	{
		tnSetSelectedListItem(widget,str[0],GR_FALSE);
		return;
	}
	if(strcmp(str[0],"..") == 0)
	{
		i = strlen(path)-2;
		while(i >=0 && path[i] != '/')
			i--;
		path[i+1] = '\0';
	}
	else
	{
		strcat(path,str[0]);
		strcat(path,"/");
	}
	tnSetText(textbox,path);
	freelist(str,m);
	freedir();
	getdir(path);
	tnDeleteAllItemsFromListBox(widget);
	dirs = 0;
	for(i=0;i<n;i++)
	{
		strcpy(currpath,path);		
		strcat(currpath,namelist[i]->d_name);
		if((fdir=opendir(currpath))!=NULL)
		{
			closedir(fdir);
			tnAddItemToListBox(widget,namelist[i]->d_name);
			dirs++;
		}
	}
	tnSetScrollRange(scrollbar,0,dirs < 15?0:dirs-15);
	return;
}


void savescrollfunc(TN_WIDGET *widget, DATA_POINTER dptr)
{
	int position = tnGetThumbPosition (widget);
	TN_WIDGET *lbox = (TN_WIDGET *)dptr;
	int currtop = tnGetListTop(lbox);
	if(position < currtop)
		tnListItemsLineDown(lbox,currtop-position);
	if(position > currtop)
		tnListItemsLineUp(lbox,position-currtop);
	return;
}

	
void InputProjectDir(int status)
{
	TN_WIDGET *win,*okbtn,*cancelbtn,*textb,*lbox,*scrollbar;
	int i;
	projectstatus = status;
	if (strcmp(path,"/")==0) { /*do not start from root, use current directory*/
	  getcwd(path, sizeof(path));
	  strcat(path,"/");
	}
	win=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_WIDTH,160,TN_HEIGHT,320,TN_CAPTION,"Project Directory",TN_END);
	lbox = tnCreateWidget(TN_LISTBOX,win,2,10,TN_HEIGHT,220,TN_WIDTH,120,TN_RESIZE,TN_FALSE,TN_END);
	scrollbar = tnCreateWidget(TN_SCROLLBAR,win,122,10,TN_HEIGHT,220,TN_WIDTH,20,TN_ORIENTATION,TN_VERTICAL,TN_END);
	textb=tnCreateWidget(TN_TEXTBOX,win,2,250,TN_WIDTH,142,TN_DEFAULTTEXT,path,TN_END);
	okbtn=tnCreateWidget(TN_BUTTON,win,10,280,TN_WIDTH,50,TN_CAPTION,"OK",TN_END);
	cancelbtn=tnCreateWidget(TN_BUTTON,win,90,280,TN_WIDTH,50,TN_CAPTION,"Cancel",TN_END);
	tnAttachData(lbox,scrollbar);
	tnRegisterCallBack(okbtn,CLICKED,saveokclicked,textb);
	tnRegisterCallBack(cancelbtn,CLICKED,savecancelclicked,NULL);
	tnRegisterCallBack(lbox,CLICKED,savelboxclicked,textb);
	tnRegisterCallBack(scrollbar,CLICKED,savescrollfunc,lbox);
	getdir(path);
	dirs = 0;
	for(i=0;i<n;i++)
	{
		strcpy(currpath,path);		
		strcat(currpath,namelist[i]->d_name);
		if((fdir=opendir(currpath))!=NULL)
		{
			closedir(fdir);
			tnAddItemToListBox(lbox,namelist[i]->d_name);
			dirs++;
		}
	}
	tnSetScrollRange(scrollbar,0,dirs < 15?0:dirs-15);
	return;
}	


void build_code(TN_WIDGET *widget,DATA_POINTER p)
{
	char *filename;
	char *message;
	FILE *mainfd,*callbackfd,*clbkhfd,*supportfd,*interfacefd,*makefd;
	FILE *ifhfd;
	
	if(!project_dir)
	{
		InputProjectDir(TND_CODEGEN);
		return;
	}

		
	filename=(char *)malloc((strlen(project_dir) + 30)*sizeof(char));

	if(!code_built)
	{
	strcpy(filename,project_dir);
	strcat(filename,"/main.c");	
	mainfd=fopen(filename,"w");
	if(mainfd==NULL)
	{
		message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(filename);
		free(message);
		free(project_dir);
		project_dir = NULL;			
		return;
	}
	write_main_c_file(mainfd);
	}
	
	strcpy(filename,project_dir);
	strcat(filename,"/callback.h");

	clbkhfd=fopen(filename,"a");
	
	if(clbkhfd==NULL)
	{
		message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(filename);
		free(message);
		free(project_dir);
		project_dir = NULL;			
		return;
	}
	
	write_callback_h_file(clbkhfd);

	strcpy(filename,project_dir);
        strcat(filename,"/callback.c");
	callbackfd=fopen(filename,"a");
	
	if(callbackfd==NULL)
	{
		message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(filename);
		free(message);
		free(project_dir);
		project_dir = NULL;			
		return;
	}
	
	write_callback_c_file(callbackfd);
	

	if(!code_built)
	{
	strcpy(filename,project_dir);
        strcat(filename,"/support.c");
        supportfd=fopen(filename,"w");
	if(supportfd==NULL)
	{
		message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(filename);
		free(message);
		free(project_dir);
		project_dir = NULL;			
		return;
	}
		write_support_c_file(supportfd);
	}
	
	
	strcpy(filename,project_dir);
        strcat(filename,"/interface.c");
	interfacefd=fopen(filename,"w");
	if(interfacefd==NULL)
	{
		message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(filename);
		free(message);
		free(project_dir);
		project_dir = NULL;			
		return;
	}
	write_interface_c_file(interfacefd);
	
	strcpy(filename,project_dir);
        strcat(filename,"/interface.h");
	ifhfd=fopen(filename,"w");
	if(ifhfd==NULL)
	{
		message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(filename);
		free(message);
		free(project_dir);
		project_dir = NULL;		
		return;
	}
	
	write_interface_h_file(ifhfd);

	
	if(!code_built)
	{
		strcpy(filename,project_dir);
		strcat(filename,"/Makefile");
	
		makefd=fopen(filename,"w");
		
		if(makefd==NULL)
		{
			message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
			strcpy(message,"Error Opening ");
			strcat(message,filename);
			DisplayMessage(message);
			free(filename);
			free(message);
			free(project_dir);
			project_dir = NULL;			
			return;
		}

		write_make_file(makefd);
	}
	
	free(filename);
	code_built=1;
	DisplayMessage("Code Generated");
	SaveProject();
	return;
}


void write_main_c_file(FILE *mainfd)
{
	RECORD *r;
	char *name;
	fputs("/* This (main.c) file has been generated automatically by TinyDesigner. This file will be generated only once. Edit according to your need*/\n",mainfd);
	
	fputs("#include<microwin/tnWidgets.h>\n#include \"interface.h\"\n#include \"callback.h\"\n\n" \
	      "int main(int argc,char *argv[])\n" \
	      "{\n"\
	      "TN_WIDGET *main_widget;\n",mainfd);
	

	if((r=head->down))
	{
		fputs("TN_WIDGET *",mainfd);
		name=(char *)tnGetAttachedData(r->widget);
		fputs(name,mainfd);
		fputs(";\n",mainfd);
	}

	fputs("main_widget=tnAppInitialize(argc,argv);\n",mainfd);

	if(r)
	{
		fputs("/*This line has been put so that you see something on the screen on running the program. Remove it if you do not want this to be the case*/\n",mainfd);
		fputs(name,mainfd);
		fputs("=create_",mainfd);
		fputs(name,mainfd);
		fputs("(main_widget);\n",mainfd);
	}
	fputs("tnMainLoop();\nreturn 0;\n}\n",mainfd);
	fclose(mainfd);
	return;
}

void write_callback_c_file(FILE *callbackfd)
{
RECORD *win,*member;
struct mhead *currmhead;
struct menumember *currmember;
int i;
if(!code_built)
{
	fputs("/*This (callback.c) file has been generated automatically by TinyDesigner. This file will always be APPENDED TO and NEVER OVERWRITTEN. Add code for callbacks in this file*/\n",callbackfd);
	fputs("#include <microwin/tnWidgets.h>\n\n",callbackfd);
}

for(win=head->down;win;win=win->down)
{
	for(i=0;i<win->count;++i)
		if(!win->written[i])
		{
			fputs("void ",callbackfd);
			fputs(win->funcnames[i],callbackfd);
			fputs("(TN_WIDGET *widget, DATA_POINTER dptr)\n{\n\t/*TODO: Add code for callback here*/\n}\n",callbackfd);
			win->written[i]=TN_TRUE;
		}
	
	for(member=win->next;member;member=member->next)
	{
		for(i=0;i<member->count;++i)
			if(!member->written[i])
			{
				fputs("void ",callbackfd);
				fputs(member->funcnames[i],callbackfd);
				fputs("(TN_WIDGET *widget, DATA_POINTER dptr)\n{\n\t/*TODO: Add code for callback here*/\n}\n",callbackfd);
				member->written[i]=TN_TRUE;
			}
		if(member->widget->type==TN_MENUBAR)
		{
			for(currmhead=menuhead;currmhead;currmhead=currmhead->down)
				if(!strcmp(currmhead->mbarname,(char *)tnGetAttachedData(member->widget)))
					break;
			for(currmember=currmhead->member;currmember;currmember=currmember->next)
				if(!currmember->written)
				{
					if(!strcmp(currmember->callbackname,""))
						continue;
					fputs("void ",callbackfd);
					fputs(currmember->callbackname,callbackfd);
					fputs("(TN_WIDGET *widget, DATA_POINTER dptr)\n{\n\t/*TODO: Add code for callback here*/\n}\n",callbackfd);
					currmember->written=TN_TRUE;
				}
					
		}
				
		/*no callbacks for radiobutton yet :)*/
	}
				
}
fclose(callbackfd);
}

void write_callback_h_file(FILE *clbkhfd)
{
RECORD *win,*member;
struct mhead *currmhead;
struct menumember *currmember;
int i;
if(!code_built)
	fputs("/*This (callback.h) file has been generated automatically by TinyDesigner. This file will always be APPENDED and NEVER OVERWRITTEN. Add code for any extra prototypes and globals here*/\n",clbkhfd);

for(win=head->down;win;win=win->down)
{
	for(i=0;i<win->count;++i)
		if(!win->written[i])
		{
			fputs("void ",clbkhfd);
			fputs(win->funcnames[i],clbkhfd);
			fputs("(TN_WIDGET *, DATA_POINTER);\n",clbkhfd);
		}
	
	for(member=win->next;member;member=member->next)
	{
		for(i=0;i<member->count;++i)
			if(!member->written[i])
			{
				fputs("void ",clbkhfd);
				fputs(member->funcnames[i],clbkhfd);
				fputs("(TN_WIDGET *, DATA_POINTER);\n",clbkhfd);
			}
		if(member->widget->type==TN_MENUBAR)
		{
			for(currmhead=menuhead;currmhead;currmhead=currmhead->down)
				if(!strcmp(currmhead->mbarname,(char *)tnGetAttachedData(member->widget)))
					break;
		for(currmember=currmhead->member;currmember;currmember=currmember->next)
			if(!currmember->written)
			{
				if(!strcmp(currmember->callbackname,""))
					continue;
				fputs("void ",clbkhfd);
				fputs(currmember->callbackname,clbkhfd);
				fputs("(TN_WIDGET *, DATA_POINTER);\n",clbkhfd);
			}
		}
		/*no callbacks for radiobutton yet :)*/
	}
				
}
fclose(clbkhfd);
}
void write_support_c_file(FILE *supportfd)
{
	fputs("/*This (support.c) file has been generated automatically by TinyDesigner. This file may get overwritten by TinyDesigner.DO NOT edit this file*/\n",supportfd);


fputs(\
"#include <microwin/tnWidgets.h>\n\n\
TN_WIDGET *lookup_widget(TN_WIDGET *start,char *name)\n\
{\n\
	TN_WIDGET_PROPS wprops;\n\
	TN_WIDGET *found=NULL;\n\
	char *widget_name;\n\
	\n\
	if(start==NULL||name==NULL)\n\
		return NULL;\n\
\n\
	widget_name = (char *)tnGetAttachedData(start);\n\
	if(widget_name && strcmp(widget_name,name)==0)\n\
		return start;\n\
	else \n\
	{\n\
		tnGetWidgetProps(start,&wprops);\n\
		found = lookup_widget(wprops.child,name);\n\
		if(found!=NULL)\n\
			return found;		\n\
		\n\
		found=lookup_widget(wprops.sibling,name);\n\
		return found;\n\
	}\n\
}" , supportfd);
return;
}

void write_interface_h_file(FILE *ifhfd)
{
	RECORD *win;
	
	fputs("/*This (interface.h) file has been generated automatically by TinyDesigner. This file may get overwritten by TinyDesigner.DO NOT edit this file*/\n",ifhfd);

	for(win=head->down;win;win=win->down)
	{
		fputs("TN_WIDGET * create_",ifhfd);
		fputs((char *)tnGetAttachedData(win->widget),ifhfd);
		fputs("(TN_WIDGET *);\n",ifhfd);
	}
	fclose(ifhfd);
}


void write_interface_c_file(FILE *interfacefd)
{
	RECORD *win; 
	RECORD *member;
	RECORD *child;
	struct mhead *currmhead;
	struct menumember *currmember;
	char *name;
	TN_WIDGET_PROPS wprops;
	char temp[80];
	int i;
	char *widget_name_string[]={
					"TN_WINDOW",
					"TN_BUTTON",
					"TN_LABEL",
					"TN_CHECKBUTTON",
					"TN_RADIOBUTTONGROUP",
					"TN_RADIOBUTTON",
					"TN_TEXTBOX",    
					"TN_SCROLLBAR",
					"TN_PROGRESSBAR",
					"TN_LISTBOX",
					"TN_PICTURE",
					"TN_COMBOBOX",
					"TN_RAWWIDGET",
					"TN_MENUBAR",
					"TN_POPUPMENU",
					"TN_CASCADEMENU",
					"TN_MENUITEM"
	};
	
	fputs("/*This (interface.c) file has been generated automatically by TinyDesigner. This file may get overwritten by TinyDesigner.DO NOT edit this file*/\n",interfacefd);

	fputs("#include<microwin/tnWidgets.h>\n#include \"interface.h\"\n#include \"callback.h\"\n\n",interfacefd);
	
	for(win=head->down;win;win=win->down)
	{
		fputs("TN_WIDGET * create_",interfacefd);
		fputs((name=(char *)tnGetAttachedData(win->widget)),interfacefd);
		fputs("(TN_WIDGET *parent)\n{\n",interfacefd);

		fputs("TN_WIDGET *",interfacefd);
		fputs(name,interfacefd);
		fputs(";\n",interfacefd);
		
		for(member=win->next;member;member=member->next)
		{
			fputs("TN_WIDGET *",interfacefd);
			fputs((char *)tnGetAttachedData(member->widget),interfacefd);
			fputs(";\n",interfacefd);
			if(member->widget->type==TN_RADIOBUTTONGROUP)
				for(child=member->down;child;child=child->next)
				{
					fputs("TN_WIDGET *",interfacefd);
					fputs((char *)tnGetAttachedData(child->widget),interfacefd);
					fputs(";\n",interfacefd);
				}
			
			if(member->widget->type==TN_MENUBAR)
			{
				for(currmhead=menuhead;currmhead;currmhead=currmhead->down)
					if(!strcmp(currmhead->mbarname,(char *)tnGetAttachedData(member->widget)))
						break;
				for(currmember=currmhead->member;currmember;currmember=currmember->next)
				{
					fputs("TN_WIDGET *",interfacefd);
					fputs(currmember->name,interfacefd);
					fputs(";\n",interfacefd);
				}
			}
		}
		fputs("\n",interfacefd);

		fputs(name,interfacefd);
		fputs("=tnCreateWidget(TN_WINDOW,parent,",interfacefd);
		tnGetWidgetProps(win->widget,&wprops);
		sprintf(temp,"%d,%d,TN_WIDTH,%d,TN_HEIGHT,%d",wprops.posx,wprops.posy,wprops.width,wprops.height);
		fputs(temp,interfacefd);
		fputs(",TN_ENABLED,",interfacefd);
		fputs((win->enabled)?"TN_TRUE":"TN_FALSE" , interfacefd);
		fputs(",TN_VISIBLE,",interfacefd);
		fputs((win->visible)?"TN_TRUE":"TN_FALSE" , interfacefd);
		(*WriteWidgetToFile[win->widget->type])(win,interfacefd,TND_CODEGEN);
		fputs(",TN_END);\n",interfacefd);
		fprintf(interfacefd,"tnAttachData(%s,strdup(\"%s\"));\n",name,name);
		

		for(member=win->next;member;member=member->next)
		{
			fputs((char *)tnGetAttachedData(member->widget),interfacefd);
			fputs("=tnCreateWidget(",interfacefd);
			fputs(widget_name_string[member->widget->type],interfacefd);
			tnGetWidgetProps(member->widget,&wprops);
			sprintf(temp,",%s,%d,%d,TN_WIDTH,%d,TN_HEIGHT,%d",name,wprops.posx,wprops.posy,wprops.width,wprops.height);
			fputs(temp,interfacefd);
			fputs(",TN_ENABLED,",interfacefd);
			fputs((member->enabled)?"TN_TRUE":"TN_FALSE" , interfacefd);
			fputs(",TN_VISIBLE,",interfacefd);			                        fputs((member->visible)?"TN_TRUE":"TN_FALSE" , interfacefd);
			/*widget specific props*/
			(*WriteWidgetToFile[member->widget->type])(member,interfacefd,TND_CODEGEN);
			fputs(",TN_END);\n",interfacefd);
			fprintf(interfacefd,"tnAttachData(%s,strdup(\"%s\"));\n",(char *)tnGetAttachedData(member->widget),(char *)tnGetAttachedData(member->widget));
			if(member->widget->type == TN_RADIOBUTTONGROUP)
				for(child=member->down;child;child=child->next)
				{
					fputs((char *)tnGetAttachedData(child->widget),interfacefd);
					fputs("=tnCreateWidget(",interfacefd);
					fputs(widget_name_string[child->widget->type],interfacefd);
					tnGetWidgetProps(child->widget,&wprops);
					sprintf(temp,",%s,%d,%d,TN_WIDTH,%d,TN_HEIGHT,%d",(char *)tnGetAttachedData(member->widget),wprops.posx,wprops.posy,wprops.width,wprops.height);
					fputs(temp,interfacefd);
					fputs(",TN_ENABLED,",interfacefd);
					fputs((child->enabled)?"TN_TRUE":"TN_FALSE" , interfacefd);
					fputs(",TN_VISIBLE,",interfacefd);			                        fputs((child->visible)?"TN_TRUE":"TN_FALSE" , interfacefd);
					(*WriteWidgetToFile[child->widget->type]) (child,interfacefd,TND_CODEGEN);
					fputs(",TN_END);\n",interfacefd);
					fprintf(interfacefd,"tnAttachData(%s,strdup(\"%s\"));\n",(char *)tnGetAttachedData(child->widget),(char *)tnGetAttachedData(child->widget));
					
				}
			if(member->widget->type==TN_MENUBAR)
			{
				/*menu specific code generation*/
			for(currmhead=menuhead;currmhead;currmhead=currmhead->down)
				if(!strcmp(currmhead->mbarname,(char *)tnGetAttachedData(member->widget)))
					break;
			for(currmember=currmhead->member;currmember;currmember=currmember->next)
				{
					fputs(currmember->name,interfacefd);
					fputs("=tnCreateWidget(",interfacefd);
					fputs(currmember->type,interfacefd);
					sprintf(temp,",%s,TN_AUTO,TN_AUTO,TN_CAPTION,\"%s\",TN_ENABLED,",currmember->parentname,currmember->label);
					fputs(temp,interfacefd);
					fputs(currmember->enabled?"TN_TRUE":"TN_FALSE",interfacefd);
					if(!strcmp(currmember->type,"TN_MENUITEM"))
					{
						fputs(",TN_CHECKABLE,",interfacefd);
						fputs(currmember->checkable?"TN_TRUE":"TN_FALSE",interfacefd);
					}
					else
					{
						fputs(",TN_EXCLUSIVE,",interfacefd);
						fputs(currmember->exclusive?"TN_TRUE":"TN_FALSE",interfacefd);
					}						
					fputs(",TN_END);\n",interfacefd);
				}
			}
			
		}

		for(i=0;i<win->count;i++)
		{
			fputs("tnRegisterCallBack(",interfacefd);
			fputs(name,interfacefd);
			fputs(",",interfacefd);
			fputs(win->callbacks[i],interfacefd);
			fputs(",",interfacefd);
			fputs(win->funcnames[i],interfacefd);
			fputs(",",interfacefd);
			fputs(win->dptrnames[i],interfacefd);
			fputs(");\n",interfacefd);
		}
		for(member=win->next;member;member=member->next)
		{
			for(i=0;i<member->count;i++)
			{
			 fputs("tnRegisterCallBack(",interfacefd);
			 fputs((char *)tnGetAttachedData(member->widget),interfacefd);
			 fputs(",",interfacefd);
			 fputs(member->callbacks[i],interfacefd);
			 fputs(",",interfacefd);
			 fputs(member->funcnames[i],interfacefd);
			 fputs(",",interfacefd);
			 fputs(member->dptrnames[i],interfacefd);
			 fputs(");\n",interfacefd);
			}
			
			/*
			 * Note: Radiobutton has no callbacks. If they are added
			 * later then: Write more code here!
			 */ 
			if(member->widget->type==TN_MENUBAR)
			{
			for(currmhead=menuhead;currmhead;currmhead=currmhead->down)
				if(!strcmp(currmhead->mbarname,(char *)tnGetAttachedData(member->widget)))
					break;
			for(currmember=currmhead->member;currmember;currmember=currmember->next)
				
					if(!strcmp(currmember->callbackname,""))
						continue;
					else
					{
						fputs("tnRegisterCallBack(",interfacefd);
						fputs(currmember->name,interfacefd);
						fputs(",CLICKED,",interfacefd);
						fputs(currmember->callbackname,interfacefd);
						if(!strcmp(currmember->dptrname,""))
							fputs(",NULL);\n",interfacefd);/*null for testing*/
						else
						{
							fputs(",",interfacefd);
							fputs(currmember->dptrname,interfacefd);
							fputs(");\n",interfacefd);
						}
						
					}
			
				
			}		
			
		
		}
				
		fputs("return ",interfacefd);
		fputs(name,interfacefd);
		fputs(";\n}\n",interfacefd);
		fputs("\n",interfacefd);
	}

	fclose(interfacefd);
	return;	
}

void write_make_file(FILE *makefd)
{
	char *projname=rindex(project_dir,'/');
	time_t t=time(NULL);
	
	fprintf(makefd,"#Makefile for project %s\n\
	\n#Generated %s#by TinyWidgets Designer\n\
	\nCC=gcc\nCFLAGS=-Wall -O2\nLIBS=-ltnW -lnano-X\n\
			\nOBJS=main.o interface.o support.o callback.o\
			\n%s: $(OBJS)\n\t$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)\n\
			\nclean:\n\t@rm -f *.o core *~ %s\n\
			\n%%.o:%%.c\n\t	$(CC) $(CFLAGS) -c $<\n",projname+1,ctime(&t),projname+1,projname+1);
	fclose(makefd);
}


