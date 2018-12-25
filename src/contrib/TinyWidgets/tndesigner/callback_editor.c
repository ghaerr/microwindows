#include "main.h"

TN_WIDGET *window,*callbackcombo,*callbackevent,*callbackfunc,*callbackdptr,*callbacktextbox,*dptrtextbox;

void add_button_clicked(TN_WIDGET *widget,DATA_POINTER p)
{
	char *event=tnGetSelectedComboItem(callbackcombo);
	char *cbname=tnGetText(callbacktextbox);
	char *dptrname=tnGetText(dptrtextbox);
	if(strcmp(cbname,"")&&event)
	{
		tnAddItemToListBox(callbackevent,event);
		tnAddItemToListBox(callbackfunc,cbname);
		tnAddItemToListBox(callbackdptr,(strcmp(dptrname,""))?dptrname:"NULL");
		tnDeleteItemFromComboBox(callbackcombo,event);
		tnSetText(callbacktextbox,"");
		tnSetText(dptrtextbox,"");
		free(event);
	}
	free(cbname);
	return;
	
}

void del_button_clicked(TN_WIDGET *widget,DATA_POINTER p)
{
	char **listitems;
	int count,i;
	tnGetSelectedListItems(callbackevent,&listitems,&count);
	for(i=0;i<count;i++)
	{
		tnAddItemToComboBox(callbackcombo,listitems[i]);
	}
	tnDeleteSelectedItems(callbackevent);
	tnDeleteSelectedItems(callbackfunc);
	tnDeleteSelectedItems(callbackdptr);

	for(i=0;i<count;i++)
		free(listitems[i]);
	free(listitems);
	return;
}

void dismiss_button_clicked(TN_WIDGET *widget,DATA_POINTER p)
{
	RECORD *r=(RECORD*)p;
	int i;
	if(r->count)
	{
		for(i=0;i<r->count;i++)
		{
			free(r->callbacks[i]);
			free(r->funcnames[i]);
			free(r->dptrnames[i]);
		}
		free(r->callbacks);
		free(r->funcnames);
		free(r->dptrnames);
	}
	tnGetAllListItems(callbackevent,&r->callbacks,&r->count);
	tnGetAllListItems(callbackfunc,&r->funcnames,&r->count);
	tnGetAllListItems(callbackdptr,&r->dptrnames,&r->count);
	tnDestroyWidget(window);
	tnSetEnabled(deletewidget_button,TN_TRUE);
	tnSetEnabled(propedit_button,TN_TRUE);
	tnSetEnabled(callbacks_button,TN_TRUE);
	
	return;
}

void event_clicked(TN_WIDGET *widget,DATA_POINTER p)
{
	int *pos,count,i;
	
	tnGetSelectedListPos(callbackfunc,&pos,&count);
	for(i=0;i<count;i++)
	{
		tnSetSelectedListItemAt(callbackfunc,pos[i],TN_FALSE);
       		tnSetSelectedListItemAt(callbackdptr,pos[i],TN_FALSE);
	}		
					
	tnGetSelectedListPos(widget,&pos,&count);
	for(i=0;i<count;i++)
	{
		tnSetSelectedListItemAt(callbackfunc,pos[i],TN_TRUE);
		tnSetSelectedListItemAt(callbackdptr,pos[i],TN_TRUE);
	}
	return;
}
	
void invoke_callbacks_editor(RECORD *record)
{
	TN_WIDGET *addbutton,*delbutton,*dismissbutton;
	char wintitle[100];
	int i;
	
	strcpy(wintitle,tnGetAttachedData(record->widget));
	strcat(wintitle," - Callbacks");
	
	window=tnCreateWidget(TN_WINDOW,main_widget,20,20,TN_HEIGHT,350,TN_WIDTH,450,TN_CAPTION,wintitle,TN_END);
	tnCreateWidget(TN_LABEL,window,100,2,TN_CAPTION,"Event",TN_END);
	tnCreateWidget(TN_LABEL,window,200,2,TN_CAPTION,"Function",TN_END);
	tnCreateWidget(TN_LABEL,window,300,2,TN_CAPTION,"Data Pointer",TN_END);
	callbackevent=tnCreateWidget(TN_LISTBOX,window,100,20,TN_WIDTH,100,TN_HEIGHT,200,TN_END);
	callbackfunc=tnCreateWidget(TN_LISTBOX,window,200,20,TN_WIDTH,100,TN_HEIGHT,200,TN_ENABLED,TN_FALSE,TN_END);
	callbackdptr=tnCreateWidget(TN_LISTBOX,window,300,20,TN_WIDTH,100,TN_HEIGHT,200,TN_ENABLED,TN_FALSE,TN_END);
	
	callbacktextbox=tnCreateWidget(TN_TEXTBOX,window,200,235,TN_WIDTH,100,TN_END);
	dptrtextbox=tnCreateWidget(TN_TEXTBOX,window,300,235,TN_WIDTH,100,TN_END);
	callbackcombo=tnCreateWidget(TN_COMBOBOX,window,100,235,TN_END);
	
	switch(record->widget->type)
	{
		case TN_WINDOW:
			tnAddItemToComboBox(callbackcombo,"CLICKED");
			tnAddItemToComboBox(callbackcombo,"CLOSED");
			break;
			
		case TN_RADIOBUTTONGROUP:
			tnAddItemToComboBox(callbackcombo,"CLICKED");
			tnAddItemToComboBox(callbackcombo,"SELECTED");
			break;
			
		case TN_TEXTBOX:
			tnAddItemToComboBox(callbackcombo,"GOTFOCUS");
			tnAddItemToComboBox(callbackcombo,"LOSTFOCUS");
			tnAddItemToComboBox(callbackcombo,"MODIFIED");
			break;
			
		case TN_COMBOBOX:
			tnAddItemToComboBox(callbackcombo,"CLICKED");
			tnAddItemToComboBox(callbackcombo,"SELECTED");
			break;
			
		case TN_RAWWIDGET:
 			tnAddItemToComboBox(callbackcombo,"TN_SYSTEM_EVENT_EXPOSURE"); 		
			 tnAddItemToComboBox(callbackcombo,"TN_SYSTEM_EVENT_BUTTON_UP");
			 tnAddItemToComboBox(callbackcombo,"TN_SYSTEM_EVENT_BUTTON_DOWN");
			 tnAddItemToComboBox(callbackcombo,"TN_SYSTEM_EVENT_MOUSE_ENTER");
			 tnAddItemToComboBox(callbackcombo,"TN_SYSTEM_EVENT_MOUSE_EXIT");
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_MOUSE_MOTION");	
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_MOUSE_POSITION"); 
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_KEY_DOWN");
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_KEY_UP");	
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_FOCUS_IN");
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_FOCUS_OUT");
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_UPDATE");	
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_CHILD_UPDATE");		
			tnAddItemToComboBox(callbackcombo," TN_SYSTEM_EVENT_CLOSE_REQ");		
			break;
		default:
			if(record->widget->type!=TN_RADIOBUTTON)
				tnAddItemToComboBox(callbackcombo,"CLICKED");
	}
	addbutton=tnCreateWidget(TN_BUTTON,window,100,300,TN_CAPTION,"Add",TN_END);
	delbutton=tnCreateWidget(TN_BUTTON,window,200,300,TN_CAPTION,"Delete",TN_END);
	dismissbutton=tnCreateWidget(TN_BUTTON,window,300,300,TN_CAPTION,"Dismiss",TN_END);
	
	for(i=0;i<record->count;i++)
	{
		tnAddItemToListBox(callbackevent,record->callbacks[i]);
		tnDeleteItemFromComboBox(callbackcombo,record->callbacks[i]);
		tnAddItemToListBox(callbackfunc,record->funcnames[i]);
		tnAddItemToListBox(callbackdptr,record->dptrnames[i]);
	}
	
	tnRegisterCallBack(addbutton,CLICKED,add_button_clicked,NULL);
	tnRegisterCallBack(delbutton,CLICKED,del_button_clicked,NULL);
	tnRegisterCallBack(dismissbutton,CLICKED,dismiss_button_clicked,record);
 	tnRegisterCallBack(callbackevent,CLICKED,event_clicked,NULL);		
}

