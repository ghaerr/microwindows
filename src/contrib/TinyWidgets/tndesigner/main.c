#include "../include/tnWidgets.h"
#include "main.h"
TN_WIDGET *tools,*proplistwin;
void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	if(!project_saved && !questionasked)
	{
		DisplayMessage("Project Not Saved");
		questionasked = 1;
		return;
	}
	tnDestroyWidget(widget);
	tnDestroyWidget(tools);
	tnDestroyWidget(proplistwin);
	tnEndApp();
}

int main(int argc,char **argv)
{
	TN_WIDGET *mainappwin,**btn;
	
	tndinit();
	btn=(TN_WIDGET **) malloc(MAXTOOLS * sizeof(TN_WIDGET *));
	main_widget=tnAppInitialize(argc,argv);
	mainappwin=tnCreateWidget(TN_WINDOW,main_widget,150,1,TN_WIDTH,210,TN_HEIGHT,60,TN_CAPTION,"TnDesigner",TN_END);
	tools=tnCreateWidget(TN_WINDOW,main_widget,1,70,TN_WIDTH,110,TN_HEIGHT,320,TN_CAPTION,"Tools",TN_END);
	proplistwin=tnCreateWidget(TN_WINDOW,main_widget,450,70,TN_WIDTH,165,TN_HEIGHT,280,TN_CAPTION,"Property List",TN_END);
	tnRegisterCallBack(mainappwin,CLOSED,endapp,NULL);
	createmainappwin(mainappwin);
	createtools(tools,btn);
	createproplist(proplistwin);
	tnMainLoop();
	return 0;
}

void tndinit(void)
{
	int i;
	head = (RECORD *)malloc(sizeof(RECORD));
	head->next=NULL;
	head->down=NULL;
	
	ToolSelected[0]=TN_WINDOW;
	ToolSelected[1]=TN_BUTTON;
	ToolSelected[2]=TN_LABEL;
	ToolSelected[3]=TN_CHECKBUTTON;
	ToolSelected[4]=TN_RADIOBUTTONGROUP;
	ToolSelected[5]=TN_RADIOBUTTON;
	ToolSelected[6]=TN_TEXTBOX;
	ToolSelected[7]=TN_SCROLLBAR;
	ToolSelected[8]=TN_PROGRESSBAR;
	ToolSelected[9]=TN_LISTBOX;
	ToolSelected[10]=TN_PICTURE;
	ToolSelected[11]=TN_COMBOBOX;
	ToolSelected[12]=TN_RAWWIDGET;
	ToolSelected[13]=TN_MENUBAR;
	
	selected=-1;
	DisplayProps[TN_WINDOW]=displaywindowprops;
	DisplayProps[TN_BUTTON]=displaybuttonprops;
	DisplayProps[TN_LABEL]=displaylabelprops;
	DisplayProps[TN_CHECKBUTTON]=displaycheckbuttonprops;
	DisplayProps[TN_RADIOBUTTONGROUP]=displayradiobuttongroupprops;
	DisplayProps[TN_RADIOBUTTON]=displayradiobuttonprops;
	DisplayProps[TN_TEXTBOX]=displaytextboxprops;
	DisplayProps[TN_SCROLLBAR]=displayscrollbarprops;
	DisplayProps[TN_PROGRESSBAR]=displayprogressbarprops;
	DisplayProps[TN_LISTBOX]=displaylistboxprops;
	DisplayProps[TN_PICTURE]=displaypictureprops;
	DisplayProps[TN_COMBOBOX]=displaycomboboxprops;
	DisplayProps[TN_RAWWIDGET]=displayrawwidgetprops;
	
	for(i=0;i<MAXTOOLS;i++)
		counter[i]=0;

	strcpy(strings[0],"window");
	strcpy(strings[1],"button");
	strcpy(strings[2],"label");
	strcpy(strings[3],"checkbutton");
	strcpy(strings[4],"radiobuttongroup");
	strcpy(strings[5],"radiobutton");
	strcpy(strings[6],"textbox");
	strcpy(strings[7],"scrollbar");
	strcpy(strings[8],"progressbar");
	strcpy(strings[9],"listbox");
	strcpy(strings[10],"picture");
	strcpy(strings[11],"combobox");
	strcpy(strings[12],"rawwidget");
	strcpy(strings[13],"menubar");

		
	PropEditFunc[TN_WINDOW]=windowPropEditFunc;
	PropEditFunc[TN_BUTTON]=buttonPropEditFunc;
	PropEditFunc[TN_LABEL]=labelPropEditFunc;
	PropEditFunc[TN_CHECKBUTTON]=checkbuttonPropEditFunc;
	PropEditFunc[TN_RADIOBUTTONGROUP]=radiobuttongroupPropEditFunc;
	PropEditFunc[TN_RADIOBUTTON]=radiobuttonPropEditFunc;
	PropEditFunc[TN_TEXTBOX]=textboxPropEditFunc;
	PropEditFunc[TN_SCROLLBAR]=scrollbarPropEditFunc;
	PropEditFunc[TN_PROGRESSBAR]=progressbarPropEditFunc;
	PropEditFunc[TN_LISTBOX]=listboxPropEditFunc;
	PropEditFunc[TN_PICTURE]=picturePropEditFunc;
	PropEditFunc[TN_COMBOBOX]=comboboxPropEditFunc;
	PropEditFunc[TN_RAWWIDGET]=rawwidgetPropEditFunc;
	propeditwin=NULL;		
	menueditwin=NULL;
	menuhead=NULL;
	project_saved=1;
	project_dir=NULL;
	savefile=strdup("/Project.rc");
	loadfile=strdup("/Project.rc");
	code_built=0;
	questionasked=1;

	WriteWidgetToFile[TN_WINDOW]=window_write;
	WriteWidgetToFile[TN_BUTTON]=button_write;
	WriteWidgetToFile[TN_LABEL]=label_write;
	WriteWidgetToFile[TN_CHECKBUTTON]=checkbutton_write;
	WriteWidgetToFile[TN_RADIOBUTTONGROUP]=radiobuttongroup_write;
	WriteWidgetToFile[TN_RADIOBUTTON]=radiobutton_write;
	WriteWidgetToFile[TN_TEXTBOX]=textbox_write;
	WriteWidgetToFile[TN_SCROLLBAR]=scrollbar_write;
	WriteWidgetToFile[TN_PROGRESSBAR]=progressbar_write;
	WriteWidgetToFile[TN_LISTBOX]=listbox_write;
	WriteWidgetToFile[TN_PICTURE]=picture_write;
	WriteWidgetToFile[TN_COMBOBOX]=combobox_write;
	WriteWidgetToFile[TN_RAWWIDGET]=rawwidget_write;
	WriteWidgetToFile[TN_MENUBAR]=menubar_write;
	
	return;
}
void save_proj(TN_WIDGET *widget,DATA_POINTER p)
{
	SaveProject();
}
void load_proj(TN_WIDGET *widget,DATA_POINTER p)
{
	LoadProject();
}

void new_proj(TN_WIDGET *widget,DATA_POINTER d)
{
	if(!project_saved && !questionasked)
	{
		DisplayMessage("Project Not Saved");
		questionasked = 1;
		return;
	}
	ClearProject();
	return;
}

void createmainappwin(TN_WIDGET *window)
{
	TN_WIDGET *button;
	
#if 0 /*toolbar buttons in text mode*/	
	button=tnCreateWidget(TN_BUTTON,window,5,5,TN_HEIGHT,50,TN_WIDTH,50,TN_CAPTION,"New",TN_END);
	tnRegisterCallBack(button,CLICKED,new_proj,NULL);
	button=tnCreateWidget(TN_BUTTON,window,55,5,TN_HEIGHT,50,TN_WIDTH,50,TN_CAPTION,"Open",TN_END);
	tnRegisterCallBack(button,CLICKED,load_proj,NULL);
	button=tnCreateWidget(TN_BUTTON,window,105,5,TN_HEIGHT,50,TN_WIDTH,50,TN_CAPTION,"Save",TN_END);
	tnRegisterCallBack(button,CLICKED,save_proj,NULL);
	button=tnCreateWidget(TN_BUTTON,window,155,5,TN_HEIGHT,50,TN_WIDTH,50,TN_CAPTION,"Build",TN_END);	
	tnRegisterCallBack(button,CLICKED,build_code,NULL);
#else /*toolbar buttons with icons*/	
	button=tnCreateWidget(TN_BUTTON,window,5,5,TN_HEIGHT,50,TN_WIDTH,50/*,TN_CAPTION,"New"*/,TN_PIXMAP,TN_TRUE,TN_FILENAME,"Icons/filenew.xpm",TN_END);
	tnRegisterCallBack(button,CLICKED,new_proj,NULL);
	button=tnCreateWidget(TN_BUTTON,window,55,5,TN_HEIGHT,50,TN_WIDTH,50,/*TN_CAPTION,"Open",*/TN_PIXMAP,TN_TRUE,TN_FILENAME,"Icons/fileopen.xpm",TN_END);
	tnRegisterCallBack(button,CLICKED,load_proj,NULL);
	button=tnCreateWidget(TN_BUTTON,window,105,5,TN_HEIGHT,50,TN_WIDTH,50,/*TN_CAPTION,"Save",*/TN_PIXMAP,TN_TRUE,TN_FILENAME,"Icons/filesave.xpm",TN_END);
	tnRegisterCallBack(button,CLICKED,save_proj,NULL);
	button=tnCreateWidget(TN_BUTTON,window,155,5,TN_HEIGHT,50,TN_WIDTH,50,/*TN_CAPTION,"Build",*/TN_PIXMAP,TN_TRUE,TN_FILENAME,"Icons/build.xpm",TN_END);
	tnRegisterCallBack(button,CLICKED,build_code,NULL);
#endif	

	return;
}


RECORD * AddToProject(TN_WIDGET *widget,TN_WIDGET *parent)
{
	RECORD *r = (RECORD *)malloc(sizeof(RECORD));
	RECORD *curr,*par;
	int i;
	r->next=NULL;
	r->down=NULL;
	r->widget=widget;
	r->enabled=TN_TRUE;
	r->visible=TN_TRUE;
	r->count=0;
	for(i=0;i<MAXCALLBACKS;++i)
		r->written[i]=TN_FALSE;
	switch(widget->type)
	{
		case TN_WINDOW:
			for(curr=head;curr->down;curr=curr->down);
			curr->down=r;
			r->menubarypos=0;
			break;

		case TN_RADIOBUTTON:
			par=GetFromProject(parent);
			for(curr=par->down;curr&&curr->next;curr=curr->next);
			if(curr)
				curr->next=r;
			else
				par->down=r;
			break;

		default:
			par=GetFromProject(parent);
			for(curr=par;curr->next;curr=curr->next);
			curr->next=r;
			break;
	}
			
	return r;		
}

RECORD *GetFromProject(TN_WIDGET *widget)
{
	RECORD *win,*curr,*child;
	
	for(win=head->down;win;win=win->down)
		if(win->widget==widget) 
			return win;
		else
			for(curr=win->next;curr;curr=curr->next)
				if(curr->widget==widget) 
					return curr;
				else 
					/*FIXME - depth is limited - this is ugly ;)*/
					for(child=curr->down;child;child=child->next)
						if(child->widget==widget) 
							return child;
				
					
	return NULL;
}

TN_WIDGET *lookupwidget(char *name)
{
	RECORD *win = head->down;
	RECORD *member, *child;
	if(!name)
	   return NULL;	
	while(win)
	{
		if(!strcmp(name,(char *)tnGetAttachedData(win->widget)))
			return win->widget;
		for(member=win->next;member;member=member->next)
		    if(!strcmp(name,(char *)tnGetAttachedData(member->widget)))
		        return member->widget;
		     else
		       for(child=member->down;child;child=child->next)
		          if(!strcmp(name,(char *)tnGetAttachedData(child->widget)))
			       return child->widget;
		win = win->down;
	}
	return NULL;
}
	
void ShowProps(TN_WIDGET *widget)
{
	TN_WIDGET_PROPS props;
	RECORD *r;
	char *WidgetTagStrings[WIDGET_PROPS]={"Name","Height","Width","X Pos","Y Pos","Enabled","Visible"};
	int i;
	char string[80];

	active_widget=widget;
	tnSetEnabled(callbacks_button,TN_TRUE);
	tnSetEnabled(propedit_button,TN_TRUE);
	tnSetEnabled(deletewidget_button,TN_TRUE);
	
	tnGetWidgetProps(widget,&props);
	tnDeleteAllItemsFromListBox(proptag);
	tnDeleteAllItemsFromListBox(propval);
	for(i=0;i<WIDGET_PROPS;++i)
		tnAddItemToListBox(proptag,WidgetTagStrings[i]);
	tnAddItemToListBox(propval,(char *)tnGetAttachedData(widget));
	sprintf(string,"%d",props.height);
	tnAddItemToListBox(propval,string);
	sprintf(string,"%d",props.width);
	tnAddItemToListBox(propval,string);
	sprintf(string,"%d",props.posx);
	tnAddItemToListBox(propval,string);
	sprintf(string,"%d",props.posy);
	tnAddItemToListBox(propval,string);
	r=GetFromProject(widget);
	tnAddItemToListBox(propval,r->enabled?"TRUE":"FALSE");
	tnAddItemToListBox(propval,r->visible?"TRUE":"FALSE");
	(*DisplayProps[props.type])(widget);
	return;
}


void widget_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	if(active_widget!=widget)
		ShowProps(widget);
	return;
}

void change_properties_clicked(TN_WIDGET *widget,DATA_POINTER d)
{
	tnSetEnabled(deletewidget_button,TN_FALSE);
	tnSetEnabled(propedit_button,TN_FALSE);
	tnSetEnabled(callbacks_button,TN_FALSE);
	project_saved = 0;
	questionasked = 0;

	invoke_property_editor(GetFromProject(active_widget));
}

void delnode(RECORD *r)
{
	if(r==NULL)
		return;
	delnode(r->down);
	delnode(r->next);
	free(r);
	return;
}

void delete_widget_clicked(TN_WIDGET *widget,DATA_POINTER d)
{
	RECORD *win,*prevwin=head,*member,*prevmember,*child,*prevchild;
	
	project_saved = 0;
	questionasked = 0;
	tnSetEnabled(deletewidget_button,TN_FALSE);
	tnSetEnabled(propedit_button,TN_FALSE);
	tnSetEnabled(callbacks_button,TN_FALSE);
				
	win=head->down;
	while(win)
	{
		if(win->widget==active_widget)
		{
			/*Delete the window and all its children*/
			prevwin->down=win->down;
			win->down=NULL;
			delnode(win);
			tnDestroyWidget(active_widget);
			active_widget=NULL;
			tnDeleteAllItemsFromListBox(proptag);
			tnDeleteAllItemsFromListBox(propval);
			return;
		}
		prevmember=win;
		member=win->next;
		while(member)
		{
			if(member->widget==active_widget)
			{
				prevmember->next=member->next;
				member->next=NULL;
				delnode(member);
				tnDestroyWidget(active_widget);
				active_widget=NULL;
				tnDeleteAllItemsFromListBox(proptag);
				tnDeleteAllItemsFromListBox(propval);
				return;
			}
			child=member->down;
			prevchild=NULL;
			while(child)
			{
				if(child->widget==active_widget)
				{
					if(prevchild==NULL)
						member->down=child->next;
					else
						prevchild->next=child->next;
					
					tnDestroyWidget(active_widget);
					active_widget=NULL;
					tnDeleteAllItemsFromListBox(proptag);
					tnDeleteAllItemsFromListBox(propval);
				
					free(child);
					return;
					
				}
				prevchild=child;
				child=child->next;
			}
			prevmember=member;
			member=member->next;
		}
		prevwin=win;
		win=win->down;
	}
			

}

void createproplist(TN_WIDGET *proplistwin)
{
	proptag=tnCreateWidget(TN_LISTBOX,proplistwin,1,1,TN_RESIZE,TN_FALSE,TN_HEIGHT,180,TN_END);
	propval=tnCreateWidget(TN_LISTBOX,proplistwin,TN_LISTBOX_WIDTH + 1 ,1,TN_ENABLED,TN_FALSE,TN_RESIZE,TN_FALSE,TN_HEIGHT,180,TN_END);
	callbacks_button=tnCreateWidget(TN_BUTTON,proplistwin,1,190,TN_WIDTH,160,TN_CAPTION,"Edit Callbacks",TN_ENABLED,TN_FALSE,TN_END);
	tnRegisterCallBack(callbacks_button,CLICKED,edit_callbacks_clicked,NULL);	
	propedit_button=tnCreateWidget(TN_BUTTON,proplistwin,1,220,TN_WIDTH,160,TN_CAPTION,"Change Properties",TN_ENABLED,TN_FALSE,TN_END);
	tnRegisterCallBack(propedit_button,CLICKED,change_properties_clicked,NULL);
	deletewidget_button=tnCreateWidget(TN_BUTTON,proplistwin,1,250,TN_WIDTH,160,TN_CAPTION,"Delete Widget",TN_ENABLED,TN_FALSE,TN_END);
        tnRegisterCallBack(deletewidget_button,CLICKED,delete_widget_clicked,NULL);
	return;
}
	
void edit_callbacks_clicked(TN_WIDGET *widget,DATA_POINTER d)
{
	tnSetEnabled(deletewidget_button,TN_FALSE);
	tnSetEnabled(propedit_button,TN_FALSE);
	tnSetEnabled(callbacks_button,TN_FALSE);
	project_saved = 0;
	questionasked = 0;

	invoke_callbacks_editor(GetFromProject(active_widget));
}

void radiobutton_clicked(TN_WIDGET *widget, DATA_POINTER d)
{
	TN_WIDGET *rdbtn = tnGetSelectedRadioButton(widget);
	if(rdbtn == NULL)
		return;
	tnSetRadioButtonStatus(rdbtn,TN_FALSE);
	ShowProps(rdbtn);
	return;
}

void container_clicked(TN_WIDGET *widget,DATA_POINTER d)
{
	int x,y;
	TN_WIDGET *NewWidget;
	RECORD *parent;
	char *namestring;
	char numstring[10];
	TN_WIDGET_PROPS props;
	tnGetWidgetProps(widget,&props);
	
	if(selected == -1)
	{
		ShowProps(widget);
		return;
	}
	if(props.type == TN_WINDOW)
		tnGetClickedPos(widget,&x,&y);	
	else if(props.type == TN_RADIOBUTTONGROUP)
		tnGetRadioButtonGroupClickedPos(widget,&x,&y);
	
	if(selected==TN_MENUBAR)
	{
		x=0;
		parent=GetFromProject(widget);
		y=parent->menubarypos;
		parent->menubarypos+=20;
	}

	project_saved = 0;
	questionasked = 0;
	NewWidget=tnCreateWidget(selected,widget,x,y,TN_END);
	if(NewWidget==NULL)
		return;
	sprintf(numstring,"%d",(counter[selected]++));
	namestring=(char *)malloc((strlen(numstring) + strlen(strings[selected]) +1) * sizeof(char));
	strcpy(namestring,strings[selected]);
	strcat(namestring,numstring);
	tnAttachData(NewWidget,namestring);
	if(selected!=TN_MENUBAR)
	{
		if(selected==TN_WINDOW || selected==TN_RADIOBUTTONGROUP)
			{
			tnRegisterCallBack(NewWidget,CLICKED,container_clicked,NULL);
			if(selected==TN_RADIOBUTTONGROUP)
				tnRegisterCallBack(NewWidget,SELECTED,radiobutton_clicked,NULL);		
			}

		else
			tnRegisterCallBack(NewWidget,CLICKED,widget_clicked,NULL);
		AddToProject(NewWidget,widget);
		ShowProps(NewWidget);
	}
	else
	{
		tnRegisterCallBack(NewWidget,CLICKED,invoke_menu_editor,NULL);
		AddToProject(NewWidget,widget);
		invoke_menu_editor(NewWidget,NULL);
		
	}
	selected = -1;
}

void window_selected(TN_WIDGET *b,DATA_POINTER d)
{
	TN_WIDGET *window=tnCreateWidget(TN_WINDOW,main_widget,100,100,TN_END);
	char *namestring;
	char numstring[20];
	project_saved = 0;
	questionasked = 0;
	selected=TN_WINDOW;
	tnRegisterCallBack(window,CLICKED,container_clicked,NULL);

	sprintf(numstring,"%d",(counter[selected]++));
	namestring=(char *)malloc((strlen(numstring) + strlen(strings[selected]) +1) * sizeof(char));
	strcpy(namestring,strings[selected]);
	strcat(namestring,numstring);
	tnAttachData(window,namestring);
	AddToProject(window,main_widget);
	ShowProps(window);
 	selected=-1;		
}


void Selection(TN_WIDGET *widget,DATA_POINTER dptr)
{
	int *s = (int *)dptr;
	selected = *s;
	return;
}

void createtools(TN_WIDGET *tools,TN_WIDGET **btn)
{
	int i,init=35;
	TN_WIDGET *button;
	
	/*Selection tool*/
	button=tnCreateWidget(TN_BUTTON,tools,1,15,TN_CAPTION,"selector",TN_HEIGHT,15,TN_WIDTH,100,TN_END);
	tnRegisterCallBack(button,CLICKED,selector_clicked,NULL);
	
	
	for(i=0;i<MAXTOOLS;++i)
	{
		btn[i]=tnCreateWidget(TN_BUTTON,tools,1,init,TN_CAPTION,strings[i],/*TN_PIXMAP,1,TN_FILENAME,btnpixmaps[i],*/TN_HEIGHT,15,TN_WIDTH,100,TN_END);
		init+=20;
		tnRegisterCallBack(btn[i],CLICKED,Selection,&ToolSelected[i]);
	}
	tnRegisterCallBack(btn[0],CLICKED,window_selected,NULL);
	
}

void selector_clicked(TN_WIDGET *widget,DATA_POINTER data)
{
	selected = -1;
	return;
}


void ClearProject(void)
{
	RECORD *win0,*win1,*member0,*member1,*child0,*child1;
	struct mhead *mhead0,*mhead1;
	struct menumember *mmember0,*mmember1;
	win0=head->down;
	while(win0)
	{
		win1=win0->down;
		member0=win0->next;
		while(member0)
		{
			member1=member0->next;
			if(member0->widget->type == TN_RADIOBUTTONGROUP)
			{
				child0=member0->down;
				while(child0)
				{
					child1=child0->next;
					free(child0);
					child0=child1;
				}
			}
			free(member0);
			member0=member1;
		}
		tnDestroyWidget(win0->widget);
		free(win0);
		win0=win1;
	}	
	head->down=NULL;
	head->next=NULL;
	tnDeleteAllItemsFromListBox(propval);
	tnDeleteAllItemsFromListBox(proptag);
	/*Clean menubar list*/
	mhead0=menuhead;
	while(mhead0)
	{
		mhead1=mhead0->down;
		mmember0=mhead0->member;
		while(mmember0)
		{
			mmember1=mmember0->next;
			free(mmember0);
			mmember0=mmember1;
		}
		free(mhead0);
		mhead0=mhead1;
	}
	menuhead=NULL;
	code_built = 0;
	return;
}


void DisplayMessage(char *message)
{
	TN_WIDGET *win, *btn;
	win = tnCreateWidget(TN_WINDOW,main_widget,100,100,TN_HEIGHT,70,TN_WIDTH,200,TN_CAPTION,"Message Box",TN_END);
	tnCreateWidget(TN_LABEL,win,10,10,TN_CAPTION,message,TN_END);
	btn = tnCreateWidget(TN_BUTTON,win,50,35,TN_HEIGHT,20,TN_WIDTH,35,TN_CAPTION,"Ok",TN_END);
	tnRegisterCallBack(btn, CLICKED, DispMsgOkClicked, win);
	return;
}

void DispMsgOkClicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	TN_WIDGET *win = (TN_WIDGET *)dptr;
	tnDestroyWidget(win);
	return;
}
	

