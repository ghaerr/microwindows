#include "main.h"
struct mhead *currmhead;

void deletembar(TN_WIDGET *widget,DATA_POINTER d)
{
	struct mhead *prev;
	struct menumember *currmember0,*currmember1;
	RECORD *parent;
	
	TN_WIDGET *oldactive;
	
	currmember0=currmhead->member;
	while(currmember0)
	{
		currmember1=currmember0->next;
		free(currmember0);
		currmember0=currmember1;
	}
	currmhead->member=NULL;
	oldactive=active_widget;
	active_widget=(TN_WIDGET *)d;
	delete_widget_clicked(widget,NULL);
	active_widget=oldactive;
	parent=GetFromProject(active_widget);
	parent->menubarypos-=20;
	if(currmhead==menuhead)
		menuhead=currmhead->down;
	else
	{
		for(prev=menuhead;prev->down!=currmhead;prev=prev->down);
		prev->down=currmhead->down;
	}
	free(currmhead);
	tnDestroyWidget(menueditwin);
	menueditwin=NULL;
}

void addtomenulists(void)
{
	struct menumember *currmember;
	for(currmember=currmhead->member;currmember;currmember=currmember->next)
	{
		tnAddItemToListBox(me_labellist,currmember->label);
		tnAddItemToListBox(me_namelist,currmember->name);
		tnAddItemToListBox(me_typelist,currmember->type);
		tnAddItemToListBox(me_parentlist,currmember->parentname);
	}
}

void closeclicked(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(menueditwin);
	menueditwin=NULL;
}

void addclicked(TN_WIDGET *widget,DATA_POINTER d)
{
	struct menumember *currmember,*prev;
	char *label,*name,*parent,*type,*cbname,*dptrname;

	label=tnGetText(me_labeltb);
	if(!strcmp(label,""))
		return;
	name=tnGetText(me_nametb);
	if(!strcmp(name,""))
		return;
	parent=tnGetSelectedComboItem(me_parentcb);
	if(!parent || !strcmp(parent,""))
		return;
	type=tnGetSelectedComboItem(me_typecb);
	if(!type || !strcmp(type,""))
		return;
	
	cbname=tnGetText(me_callbacktb);
	dptrname=tnGetText(me_dptrtb);
	
	for(prev=currmember=currmhead->member;currmember;prev=currmember,currmember=currmember->next)
		if(!strcmp(currmember->name,name))
				return;
	
	if(!prev)
	{
		currmhead->member=(struct menumember *)malloc(sizeof(struct menumember));
		currmember=currmhead->member;
	}
	else
	{	
	prev->next=(struct menumember *)malloc(sizeof(struct menumember));
	currmember=prev->next;
	}
	strcpy(currmember->name,name);
	strcpy(currmember->label,label);
	strcpy(currmember->type,type);
	strcpy(currmember->parentname,parent);
	strcpy(currmember->callbackname,cbname);
	if(!strcmp(dptrname,""))
		strcpy(currmember->dptrname,"NULL");
	else
		strcpy(currmember->dptrname,dptrname);
	currmember->written=TN_FALSE;
	currmember->enabled=!strcmp(tnGetSelectedComboItem(me_enabledcb),"TN_TRUE")?TN_TRUE:TN_FALSE;
	if(!strcmp(type,"TN_MENUITEM"))
		currmember->exclusive=!strcmp(tnGetSelectedComboItem(me_exclusivecb),"TN_TRUE")?TN_TRUE:TN_FALSE;
	else
		currmember->checkable=!strcmp(tnGetSelectedComboItem(me_checkablecb),"TN_TRUE")?TN_TRUE:TN_FALSE;
	currmember->next=NULL;
	
	tnAddItemToListBox(me_labellist,label);
	tnAddItemToListBox(me_namelist,name);
	tnAddItemToListBox(me_typelist,type);
	tnAddItemToListBox(me_parentlist,parent);
			
	tnDeleteAllItemsFromComboBox(me_parentcb);
	tnSetText(me_nametb,"");
	tnSetText(me_labeltb,"");
	tnSetText(me_callbacktb,"");
	tnSetText(me_dptrtb,"");
	tnSetSelectedComboItem(me_enabledcb,"TN_TRUE",TN_TRUE);
	tnSetSelectedComboItem(me_exclusivecb,"TN_FALSE",TN_TRUE);
	tnSetSelectedComboItem(me_checkablecb,"TN_FALSE",TN_TRUE);
	tnSetSelectedComboItem(me_typecb,"TN_POPUPMENU",TN_FALSE);
}

void deletechildren(char *s)
{
	char str[80];
	struct menumember *currmember,*prev,*temp;
	int pos;
	currmember=currmhead->member;
	prev=currmember;
	while(currmember)
	{
		if(!strcmp(currmember->parentname,s))
		{
			temp=currmember;
			if(prev==currmember) /*first node*/
				currmhead->member=currmember->next;
			else
				prev->next=currmember->next;
			
			strcpy(str,temp->name);
			pos=tnGetListItemPos(me_namelist,temp->name);
			tnDeleteItemFromListBoxAt(me_labellist,pos);
			tnDeleteItemFromListBoxAt(me_namelist,pos);
			tnDeleteItemFromListBoxAt(me_typelist,pos);
			tnDeleteItemFromListBoxAt(me_parentlist,pos);
			free(temp);
			deletechildren(str);
			prev=currmember=currmhead->member;
			continue;
		}
		prev=currmember;
		currmember=currmember->next;
	}
}
		
void delclicked(TN_WIDGET *widget,DATA_POINTER d)
{
	int count,i;
	char **liststrings;
	struct menumember *currmember,*prev,*temp;
	tnGetSelectedListItems(me_namelist,&liststrings,&count);
	for(i=0;i<count;i++)
	{
		currmember=currmhead->member;
		prev=currmember;
		while(currmember)
		{
			if(!strcmp(currmember->name,liststrings[i]))
			{
				temp=currmember;
				if(prev==currmember)/*first node*/
				{
					currmhead->member=currmember->next;
					prev=currmember=currmember->next;
				}
				else
				{
					prev->next=currmember->next;	
					currmember=currmember->next;
				}
				free(temp);
				continue;
			}
			prev=currmember;
			currmember=currmember->next;
		}
		deletechildren(liststrings[i]);
	}
	tnDeleteSelectedItems(me_labellist);
	tnDeleteSelectedItems(me_namelist);
	tnDeleteSelectedItems(me_typelist);
	tnDeleteSelectedItems(me_parentlist);
}

void labelclicked(TN_WIDGET *widget,DATA_POINTER d)
{
	int *pos,count,i;
	
	tnGetSelectedListPos(me_namelist,&pos,&count);
	for(i=0;i<count;i++)
	{
		tnSetSelectedListItemAt(me_namelist,pos[i],TN_FALSE);
		tnSetSelectedListItemAt(me_typelist,pos[i],TN_FALSE);
       		tnSetSelectedListItemAt(me_parentlist,pos[i],TN_FALSE);
	}		
					
	tnGetSelectedListPos(widget,&pos,&count);
	for(i=0;i<count;i++)
	{
		tnSetSelectedListItemAt(me_namelist,pos[i],TN_TRUE);
		tnSetSelectedListItemAt(me_typelist,pos[i],TN_TRUE);
       		tnSetSelectedListItemAt(me_parentlist,pos[i],TN_TRUE);
	}
	return;
}

void setparentcb(TN_WIDGET *widget,DATA_POINTER d)
{
	struct menumember *currmember;
	tnDeleteAllItemsFromComboBox(me_parentcb);
	if(!strcmp(tnGetSelectedComboItem(widget),"TN_POPUPMENU"))
	{
		tnAddItemToComboBox(me_parentcb,currmhead->mbarname);
		tnSetSelectedComboItem(me_parentcb,currmhead->mbarname,TN_TRUE);
		tnSetVisible(me_checkablelbl,TN_FALSE);
		tnSetVisible(me_checkablecb,TN_FALSE);
		tnSetVisible(me_exclusivelbl,TN_TRUE);
		tnSetVisible(me_exclusivecb,TN_TRUE);

	}
	if(!strcmp(tnGetSelectedComboItem(widget),"TN_CASCADEMENU"))
	{
		for(currmember=currmhead->member;currmember;currmember=currmember->next)
			if(!strcmp(currmember->type,"TN_POPUPMENU"))
				tnAddItemToComboBox(me_parentcb,currmember->name);
		tnSetVisible(me_checkablelbl,TN_FALSE);
		tnSetVisible(me_checkablecb,TN_FALSE);
		tnSetVisible(me_exclusivelbl,TN_TRUE);
		tnSetVisible(me_exclusivecb,TN_TRUE);
	}
	if(!strcmp(tnGetSelectedComboItem(widget),"TN_MENUITEM"))
	{
		tnAddItemToComboBox(me_parentcb,currmhead->mbarname);
		for(currmember=currmhead->member;currmember;currmember=currmember->next)
			if(!strcmp(currmember->type,"TN_POPUPMENU") || !strcmp(currmember->type,"TN_CASCADEMENU"))
				tnAddItemToComboBox(me_parentcb,currmember->name);
		tnSetVisible(me_checkablelbl,TN_TRUE);
		tnSetVisible(me_checkablecb,TN_TRUE);
		tnSetVisible(me_exclusivelbl,TN_FALSE);
		tnSetVisible(me_exclusivecb,TN_FALSE);
	}
}

void invoke_menu_editor(TN_WIDGET *widget,DATA_POINTER d)
{
	TN_WIDGET *addbutton,*delbutton,*closebutton,*delmbarbtn;
	struct mhead *prev;
	char string[100];
	int first=0;
	char *boolcbstrings[]={"TN_TRUE","TN_FALSE"};
	char *typestrings[]={"TN_POPUPMENU","TN_CASCADEMENU","TN_MENUITEM"};
	
	project_saved = 0;
	questionasked = 0;
	strcpy(string,"Menu Editor -");
	strcat(string,(char *)tnGetAttachedData(widget));

	if(!menueditwin)
		menueditwin=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_WIDTH,600,TN_HEIGHT,350,TN_CAPTION,string,TN_END);

	if(!menuhead)
	{
		first=1;
		menuhead=(struct mhead *)malloc(sizeof(struct mhead));
		currmhead=menuhead;
		strcpy(currmhead->mbarname,(char *)tnGetAttachedData(widget));
		currmhead->down=NULL;
		currmhead->member=NULL;
	}
	else
	{
		for(prev=currmhead=menuhead;currmhead;prev=currmhead,currmhead=currmhead->down)
			if(!strcmp(currmhead->mbarname,(char *)tnGetAttachedData(widget)))
				break;
		if(!currmhead)
		{
			prev->down=(struct mhead *)malloc(sizeof(struct mhead));
			currmhead=prev->down;
			strcpy(currmhead->mbarname,(char *)tnGetAttachedData(widget));
			currmhead->down=NULL;
			currmhead->member=NULL;
		}
	}

	tnCreateWidget(TN_LABEL,menueditwin,2,2,TN_CAPTION,"Label",TN_END);
	me_labellist=tnCreateWidget(TN_LISTBOX,menueditwin,2,20,TN_WIDTH,80,TN_HEIGHT,200,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,82,2,TN_CAPTION,"Name",TN_END);
	me_namelist=tnCreateWidget(TN_LISTBOX,menueditwin,82,20,TN_WIDTH,80,TN_HEIGHT,200,TN_ENABLED,TN_FALSE,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,162,2,TN_CAPTION,"Type",TN_END);
	me_typelist=tnCreateWidget(TN_LISTBOX,menueditwin,162,20,TN_WIDTH,80,TN_HEIGHT,200,TN_ENABLED,TN_FALSE,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,242,2,TN_CAPTION,"Parent",TN_END);
	me_parentlist=tnCreateWidget(TN_LISTBOX,menueditwin,242,20,TN_WIDTH,80,TN_HEIGHT,200,TN_ENABLED,TN_FALSE,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,325,20,TN_CAPTION,"Label",TN_END);
	me_labeltb=tnCreateWidget(TN_TEXTBOX,menueditwin,375,20,TN_WIDTH,80,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,325,40,TN_CAPTION,"Name",TN_END);
	me_nametb=tnCreateWidget(TN_TEXTBOX,menueditwin,375,40,TN_WIDTH,80,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,325,60,TN_CAPTION,"Type",TN_END);
	me_typecb=tnCreateWidget(TN_COMBOBOX,menueditwin,375,60,TN_WIDTH,125,TN_LISTITEMS,typestrings,TN_COUNT,3,TN_END);
	
	tnCreateWidget(TN_LABEL,menueditwin,325,80,TN_CAPTION,"Parent",TN_END);
	me_parentcb=tnCreateWidget(TN_COMBOBOX,menueditwin,375,80,TN_WIDTH,80,TN_END);
	if(first)
	{
		tnAddItemToComboBox(me_parentcb,currmhead->mbarname);
		tnSetSelectedComboItem(me_parentcb,currmhead->mbarname,TN_TRUE);
		tnSetSelectedComboItem(me_typecb,"TN_POPUPMENU",TN_TRUE);
	}
		
	tnCreateWidget(TN_LABEL,menueditwin,325,100,TN_CAPTION,"Callback",TN_END);
	me_callbacktb=tnCreateWidget(TN_TEXTBOX,menueditwin,375,100,TN_WIDTH,80,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,456,100,TN_CAPTION,"Data Pointer",TN_END);
	me_dptrtb=tnCreateWidget(TN_TEXTBOX,menueditwin,516,100,TN_WIDTH,80,TN_END);
	tnCreateWidget(TN_LABEL,menueditwin,325,120,TN_CAPTION,"Enabled",TN_END);
	me_enabledcb=tnCreateWidget(TN_COMBOBOX,menueditwin,375,120,TN_LISTITEMS,boolcbstrings,TN_COUNT,2,TN_END);
	tnSetSelectedComboItem(me_enabledcb,"TN_TRUE",TN_TRUE);
	me_exclusivelbl=tnCreateWidget(TN_LABEL,menueditwin,325,140,TN_CAPTION,"Exclusive",TN_VISIBLE,TN_TRUE,TN_END);
	me_exclusivecb=tnCreateWidget(TN_COMBOBOX,menueditwin,375,140,TN_LISTITEMS,boolcbstrings,TN_COUNT,2,TN_END);
	tnSetSelectedComboItem(me_exclusivecb,"TN_FALSE",TN_TRUE);
	me_checkablelbl=tnCreateWidget(TN_LABEL,menueditwin,324,160,TN_CAPTION,"Checkable",TN_VISIBLE,TN_FALSE,TN_END);
	me_checkablecb=tnCreateWidget(TN_COMBOBOX,menueditwin,375,160,TN_LISTITEMS,boolcbstrings,TN_COUNT,2,TN_VISIBLE,TN_FALSE,TN_END);
	tnSetSelectedComboItem(me_checkablecb,"TN_FALSE",TN_TRUE);
	addbutton=tnCreateWidget(TN_BUTTON,menueditwin,350,200,TN_CAPTION,"Add",TN_END);
	delbutton=tnCreateWidget(TN_BUTTON,menueditwin,420,200,TN_CAPTION,"Delete",TN_END);
	closebutton=tnCreateWidget(TN_BUTTON,menueditwin,375,260,TN_WIDTH,80,TN_CAPTION,"Close",TN_END);
	delmbarbtn=tnCreateWidget(TN_BUTTON,menueditwin,355,310,TN_WIDTH,120,TN_CAPTION,"Remove Menubar",TN_END);

	tnRegisterCallBack(addbutton,CLICKED,addclicked,NULL);
	tnRegisterCallBack(delbutton,CLICKED,delclicked,NULL);
	tnRegisterCallBack(closebutton,CLICKED,closeclicked,NULL);
	tnRegisterCallBack(delmbarbtn,CLICKED,deletembar,widget);

	tnRegisterCallBack(me_labellist,CLICKED,labelclicked,NULL);
	tnRegisterCallBack(me_typecb,SELECTED,setparentcb,NULL);
	if(!first)
		addtomenulists();
}
	
void menubar_write(RECORD *record,FILE *fp,int operation)
{
	switch(operation)
	{
		case TND_SAVE:
			break;
		case TND_CODEGEN:
			break;
	}
}

