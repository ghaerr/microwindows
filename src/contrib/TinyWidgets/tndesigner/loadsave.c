#include "main.h"
#include<dirent.h>
#include <sys/stat.h>
	
void WriteWidget(RECORD *win,RECORD *parent,FILE *savefd)
{

		TN_WIDGET_PROPS wprops;
		TN_WIDGET_TYPE type=0;
		char *name;
		int i;
		struct mhead *currmhead;
		struct menumember *currmember;
		fputs("widgetname ",savefd);
		name=(char *)tnGetAttachedData(win->widget);
		fputs(name,savefd);
		fputs("\n",savefd);
		fputs("parent ",savefd);
		if(parent)
			fputs((char *)tnGetAttachedData(parent->widget),savefd);
		else 
			fputs("NULL",savefd);
		fputs("\n",savefd);
		tnGetWidgetProps(win->widget,&wprops);
		fprintf(savefd,"xpos %d\n",wprops.posx);
		fprintf(savefd,"ypos %d\n",wprops.posy);
		fprintf(savefd,"type %d\n",wprops.type);
		fprintf(savefd,"height %d\n",wprops.height);
		fprintf(savefd,"width %d\n",wprops.width);
		fprintf(savefd,"enabled %d\n",win->enabled);
		fprintf(savefd,"visible %d\n",win->visible);
		/* Save Widget Specific Properties Here */
		(*WriteWidgetToFile[win->widget->type]) (win,savefd,TND_SAVE);			
		fprintf(savefd,"callbackcount %d\n",win->count);
		for(i=0;i<win->count;i++)
		{
			fprintf(savefd,"callback %s %s %s %d\n",win->callbacks[i],win->funcnames[i],win->dptrnames[i],win->written[i]);
		}
		fprintf(savefd,"$\n");
		if(win->widget->type==TN_MENUBAR)
		{
			for(currmhead=menuhead;currmhead;currmhead=currmhead->down)
				if(!strcmp(currmhead->mbarname,(char *)tnGetAttachedData(win->widget)))
					break;
			for(currmember=currmhead->member;currmember;currmember=currmember->next)
			{
				if(!strcmp(currmember->type,"TN_POPUPMENU"))
					type=TN_POPUPMENU;
				if(!strcmp(currmember->type,"TN_CASCADEMENU"))
					type=TN_CASCADEMENU;
				if(!strcmp(currmember->type,"TN_MENUITEM"))
					type=TN_MENUITEM;
				fprintf(savefd,"widgetname %s\nparent %s\ntype %d\nlabel \"%s\"\nenabled %d\n",currmember->name,currmember->parentname,type,currmember->label,currmember->enabled);
				if(!(strcmp(currmember->type,"TN_MENUITEM")))
					fprintf(savefd,"checkable %d\n",currmember->checkable);
				else
					fprintf(savefd,"exclusive %d\n",currmember->exclusive);
				if(strcmp(currmember->callbackname,""))
					fprintf(savefd,"callback CLICKED %s %s %d\n",currmember->callbackname,currmember->dptrname,currmember->written);
				fprintf(savefd,"$\n");
			}
		}

}

void SaveProject(void)
{
	FILE *savefd;
	DIR *fdir;
	char *filename;
	char *message;
	RECORD *win,*member,*child;
	int i;
	if(!project_dir)
	{
		InputProjectDir(TND_SAVE);
		return;
		
	}
	
	filename=(char *)malloc((strlen(project_dir) + 30)*sizeof(char));
	strcpy(filename,project_dir);
	if((fdir=opendir(filename))==NULL) {
	  DisplayMessage("New project directory made");
	  mkdir(filename, 0700);
	}  
	strcat(filename,savefile);
	savefd=fopen(filename,"w");
	if(!savefd)
	{
		message = (char *)malloc((strlen(project_dir) + 30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(message);
		free(filename);
		free(project_dir);
		project_dir = NULL;
		return;
	}
		 
	
	fprintf(savefd,"built %d\n",code_built);
	fputs("counts",savefd);
	
	for(i=0;i<MAXTOOLS;i++)
		fprintf(savefd," %d",counter[i]);
	fputs("\n",savefd);
	for(win=head->down;win;win=win->down)
	{
		WriteWidget(win,NULL,savefd);
		for(member=win->next;member;member=member->next)
		{
			WriteWidget(member,win,savefd);
			if(member->widget->type==TN_RADIOBUTTONGROUP)
				for(child=member->down;child;child=child->next)
				{
					WriteWidget(child,member,savefd);
				}
		}
		
	}
	free(filename);
	fclose(savefd);
	project_saved = 1;
	DisplayMessage("Project Saved");
}



void LoadProject(void)
{
	if(!project_saved && !questionasked)
	{
		DisplayMessage("Project Not Saved");
		questionasked = 1;
		return;
	}

	ClearProject();
	InputProjectDir(TND_LOAD);
}
