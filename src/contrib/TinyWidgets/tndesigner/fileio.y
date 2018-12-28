%{
#include<tnWidgets.h>
#include "main.h"
char *wname;
char *pname;
char *caption;
char *filename = NULL;
int type;
int xpos, ypos;
int height, width;
int enabled;
int visible;
int count;
int haspixmap=0;
int orientation;
int minval,maxval,pagestep,linestep;
char *defaulttext=NULL;
TN_BOOL resize;
TN_BOOL stretch;
int fillcolor;
int fgcolor;
int discrete=0;
int stepsize;
char *label;
int exclusive;
int checkable;
int menubarypos;

char **callbacks;
char **funcnames;
char **dptrnames;
char zname[256];
int written[MAXCALLBACKS];
char *menucallback;
char *menufuncname;
char *menudptrname;
int menuwritten;
int i;

TN_WIDGET *main_widget,*parent,*newwidget;
RECORD *r;
static struct mhead *currmhead;
struct menumember *currmember;
						  
extern int lineno;
extern FILE *yyin;
%}
%{
int yylex();
void yyerror(char *s);
%}
%union {
	int number;
	char name[200];
}

%token <number> NUMBER
%token <name> NAME
%token <name> QSTRING
%token WIDGETNAME TYPE PARENT XPOS YPOS HEIGHT WIDTH ENABLED VISIBLE CAPTION EOW CALLBACKCOUNT CALLBACK RESIZE QUOTE STRETCH FILENAME HASPIXMAP DEFAULTTEXT ORIENTATION MINVAL MAXVAL PAGESTEP LINESTEP FILLCOLOR FGCOLOR DISCRETE STEPSIZE LABEL CHECKABLE EXCLUSIVE MENUBARYPOS EOL 

%%
widgetlist: widget
	    | widgetlist widget
	   ;

widget: PROPS EOW EOL{		if(type==TN_MENUBAR)	{
				if(!menuhead)	{
					menuhead=(struct mhead *)malloc(sizeof(struct mhead));
					strcpy(menuhead->mbarname,wname);
					menuhead->down=NULL;
					menuhead->member=NULL;
					currmhead=menuhead;
					}
				else	{
					for(currmhead=menuhead;currmhead->down;currmhead=currmhead->down);
					currmhead->down=(struct mhead *)malloc(sizeof(struct mhead));
					currmhead=currmhead->down;
					strcpy(currmhead->mbarname,wname);
					currmhead->down=NULL;
					currmhead->member=NULL;
				}
			}
			if(type==TN_POPUPMENU || type==TN_CASCADEMENU || type==TN_MENUITEM)			{
				if(!currmhead->member)	{
					currmhead->member=(struct menumember *)malloc(sizeof(struct menumember));
					currmember=currmhead->member;
				}
				else	{
				for(currmember=currmhead->member;currmember->next;currmember=currmember->next);
				currmember->next=(struct menumember *)malloc(sizeof(struct menumember));
				currmember=currmember->next;
				}
				currmember->next=NULL;
				if(type==TN_POPUPMENU)
					strcpy(currmember->type,"TN_POPUPMENU");

				if(type==TN_CASCADEMENU)
					strcpy(currmember->type,"TN_CASCADEMENU");

				if(type==TN_MENUITEM)
					strcpy(currmember->type,"TN_MENUITEM");

				strcpy(currmember->label,label);
				strcpy(currmember->name,wname);
				strcpy(currmember->parentname,pname);
				currmember->enabled=enabled;
				if(menucallback)	{
				currmember->written=menuwritten;
				strcpy(currmember->callbackname,menufuncname);
				strcpy(currmember->dptrname,menudptrname);
				}
				if(type!=TN_MENUITEM)
					currmember->exclusive=exclusive;
				else
					currmember->checkable=checkable;
				
				free(label);
				free(wname);
				free(pname);
				if(menucallback)
					{
					free(menucallback);
					free(menufuncname);
					free(menudptrname);
				}
			}
			else	{
	
			if(!strcmp(pname,"NULL"))
			   parent = main_widget;
			else
			   parent = lookupwidget(pname);
			if(!parent)
			{
				fprintf(stderr,"Cannot Find Parent\n");
				return 0;
			}
	
			newwidget = tnCreateWidget(type,parent,xpos,ypos,TN_HEIGHT,height,TN_WIDTH,width,TN_CAPTION,caption,TN_RESIZE,resize,TN_DEFAULTTEXT,defaulttext,TN_PIXMAP,haspixmap,TN_FILENAME,filename,TN_ORIENTATION,orientation,TN_PAGESTEP,pagestep,TN_LINESTEP,linestep,TN_MAXVAL,maxval,TN_MINVAL,minval,TN_FGCOLOR,fgcolor,TN_FILLCOLOR,fillcolor,TN_DISCRETE,discrete,TN_STEPSIZE,stepsize,TN_END);
			r = AddToProject(newwidget,parent);
			r->enabled = enabled;
			r->visible = visible;
			if(r->widget->type!=TN_MENUBAR)
		  {
			if(r->widget->type == TN_WINDOW || r->widget->type == TN_RADIOBUTTONGROUP) {
			tnRegisterCallBack(r->widget,CLICKED,container_clicked,NULL);
			if(r->widget->type==TN_RADIOBUTTONGROUP)
				tnRegisterCallBack(r->widget,SELECTED,radiobutton_clicked,NULL);
		   }
			else
			tnRegisterCallBack(r->widget,CLICKED,widget_clicked,NULL);
		}
		else
			tnRegisterCallBack(r->widget,CLICKED,invoke_menu_editor,NULL);
			r->count=count;
		 	r->menubarypos=menubarypos;
			r->callbacks=(char **)malloc(count*sizeof(char*));
		 	r->funcnames=(char **)malloc(count*sizeof(char*));
		 	r->dptrnames=(char **)malloc(count*sizeof(char*));
			
			for(i=0;i<count;i++)
			{
			 r->callbacks[i]=strdup(callbacks[i]);
			 r->funcnames[i]=strdup(funcnames[i]);
			 r->dptrnames[i]=strdup(dptrnames[i]);
			 r->written[i]=written[i];

			 free(callbacks[i]);
			 free(funcnames[i]);
			 free(dptrnames[i]);
			 }
			 free(callbacks);
			 free(funcnames);
			 free(dptrnames);
			 
			 
			tnAttachData(newwidget,wname);
			free(pname);
			if(caption)
				free(caption);
			caption = NULL;
			if(filename)
				free(filename);
			filename = NULL;
			if(defaulttext)
				free(defaulttext);
			defaulttext=NULL;
		}
			
}
	
PROPS: PROP
      | PROPS PROP
      ;
      
PROP: WIDGETNAME NAME EOL{ wname = strdup($2); }
     | TYPE NUMBER EOL{ type = $2; }
     | PARENT NAME EOL { pname = strdup($2); }
     | XPOS NUMBER EOL { xpos = $2; }
     | YPOS NUMBER EOL { ypos = $2; }
     | HEIGHT NUMBER EOL { height = $2; }
     | WIDTH NUMBER EOL { width = $2; }
     | ENABLED NUMBER EOL { enabled = $2; }
     | VISIBLE NUMBER EOL { visible = $2; }
     | CAPTION QSTRING EOL { caption = strdup($2); }
     | RESIZE NUMBER EOL { resize = $2; }
     | STRETCH NUMBER EOL { stretch = $2; }
     | FILENAME QSTRING EOL { filename = strdup($2); }
     | HASPIXMAP NUMBER EOL { haspixmap= $2;}
     | ORIENTATION NUMBER EOL { orientation = $2;}
     | MINVAL NUMBER EOL { minval = $2;}
     | MAXVAL NUMBER EOL { maxval = $2;}
     | PAGESTEP NUMBER EOL { pagestep = $2;}
     | LINESTEP NUMBER EOL { linestep = $2;}
     | FILLCOLOR NUMBER EOL {fillcolor = $2;}
     | FGCOLOR NUMBER EOL {fgcolor = $2;}
     | DISCRETE NUMBER EOL {discrete = $2;}
     | STEPSIZE NUMBER EOL {stepsize = $2;}
     | DEFAULTTEXT QSTRING EOL { defaulttext = strdup ($2);}
     | CALLBACKCOUNT NUMBER EOL 
     		{
		 count=$2;
		 i=0;
		 callbacks=(char **)malloc(count*sizeof(char*));
		 funcnames=(char **)malloc(count*sizeof(char*));
		 dptrnames=(char **)malloc(count*sizeof(char*));
		}
       | CALLBACK NAME NAME NAME NUMBER EOL
       		{
		  if(type==TN_POPUPMENU || type==TN_CASCADEMENU || type==TN_MENUITEM)		{
	
		  menucallback=strdup($2);
		  menufuncname=strdup($3);
		  menudptrname=strdup($4);
		  menuwritten=$5;
		}
		else	{
		  callbacks[i] = strdup($2);
		  funcnames[i] = strdup($3);
		  dptrnames[i] = strdup($4);
		  written[i++] = $5;
		}
		}
       | LABEL QSTRING EOL		{ label = strdup($2); }
       | EXCLUSIVE NUMBER EOL	{ exclusive = $2; }
       | CHECKABLE NUMBER EOL   { checkable = $2; }
       | MENUBARYPOS NUMBER EOL { menubarypos = $2; }
       
%%
void yyerror(char *s)
{
	printf("%d:%s\n",lineno,s);
	exit(0);
}

int loadrc(void)
{
	char name[7];
	int i;
	char *filename;
	char *message;
	filename = (char *)malloc((strlen(project_dir)+30)*sizeof(char));
	strcpy(filename,project_dir);
	strcat(filename,loadfile);
	
	yyin = fopen(filename,"r");
	if(!yyin)
	{
		message = (char *)malloc((strlen(project_dir)+30)*sizeof(char));
		strcpy(message,"Error Opening ");
		strcat(message,filename);
		DisplayMessage(message);
		free(message);
		free(filename);
		free(project_dir);
		project_dir = NULL;
		return(-1);
	}
	fscanf(yyin,"%s %d\n",name,&code_built);
	fscanf(yyin,"%s",name);
	for(i=0;i<MAXTOOLS;i++)
		fscanf(yyin," %d",&counter[i]);
	fscanf(yyin,"\n");
	return(yyparse());
}

