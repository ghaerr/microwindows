#include "main.h"

void on_pe_cancel_clicked(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(propeditwin);
	propeditwin=NULL;
	tnSetEnabled(deletewidget_button,TN_TRUE);
	tnSetEnabled(propedit_button,TN_TRUE);
	tnSetEnabled(callbacks_button,TN_TRUE);

}

void on_pe_ok_clicked(TN_WIDGET *widget,DATA_POINTER d)
{
	TN_WIDGET_PROPS wprops;
	char *name;
	char *string;
	int posx,posy,height,width;
	RECORD *r;
	
	/*Code to update common properties*/
	tnGetWidgetProps(active_widget,&wprops);
	
	name=tnGetText(pe_nametb);
	if(strcmp(name,"")==0)
	{
		free(name);
	}
	else
	{
		free(tnGetAttachedData(active_widget));
		tnAttachData(active_widget,name);
	}

	string=tnGetText(pe_heighttb);
	if(strcmp(string,""))
	{
		height=atoi(string);
		if(height<=0)
			height=wprops.height;
	}
	else
		height=wprops.height;
	free(string);

	string=tnGetText(pe_widthtb);
	if(strcmp(string,""))
	{
		width=atoi(string);
		if(width<=0)
			width=wprops.width;
	}
	else
		width=wprops.width;
	free(string);
	string=tnGetText(pe_xpostb);
	if(strcmp(string,""))
	{
		posx=(int)strtol(string,&endp,10);
		if(*endp!='\0' || posx<0)
			posx=wprops.posx;
	}
	else
		posx=wprops.posx;
			
	free(string);
	
	string=tnGetText(pe_ypostb);
	if(strcmp(string,""))
	{
		posy=(int)strtol(string,&endp,10);
		if(*endp!='\0' || posy<0)
			posy=wprops.posy;
	}
	else
		posy=wprops.posy;

	free(string);
	
	r=GetFromProject(active_widget);
	if(r==NULL)
		return; /*unknown record!*/
	
	string=tnGetSelectedComboItem(pe_enabledcb);
	if(string)
	{
		r->enabled=(strcmp(string,"TN_TRUE")==0)?TN_TRUE:TN_FALSE;
		free(string);
	}
	
	string=tnGetSelectedComboItem(pe_visiblecb);
	if(string)
	{
		r->visible=(strcmp(string,"TN_TRUE")==0)?TN_TRUE:TN_FALSE;
		free(string);
	}
	

	wprops.height=height;
	wprops.width=width;
	wprops.posx=posx;
	wprops.posy=posy;
	tnSetWidgetProps(active_widget,&wprops);
	
	/*Code to update widget specific properties*/
	(*PropEditFunc[active_widget->type]) (TND_UPDATE);
	/*Update the properties display too*/
	ShowProps(active_widget);
	
	tnDestroyWidget(propeditwin);
	propeditwin=NULL;
	tnSetEnabled(propedit_button,TN_TRUE);
	tnSetEnabled(deletewidget_button,TN_TRUE);
	tnSetEnabled(callbacks_button,TN_TRUE);
}

void invoke_property_editor(RECORD *record)
{
	TN_WIDGET *okbutton,*cancelbutton;
	TN_WIDGET_PROPS wprops;
	char *text;
	char *combostrings[]={"TN_TRUE","TN_FALSE"};
	
	tnSetEnabled(propedit_button,TN_FALSE);
	pe_ypos=10;
		
	if(!propeditwin)
		tnDestroyWidget(propeditwin);
	
	propeditwin=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,200,TN_CAPTION,"Property Editor",TN_END);
	
	tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Name",TN_END);
	text=(char *)tnGetAttachedData(record->widget);
	pe_nametb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,text,TN_END);
	
	pe_ypos+=20;
	tnGetWidgetProps(record->widget,&wprops);
	tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Height",TN_END);
	sprintf(pe_string,"%d",wprops.height);
	pe_heighttb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);
	pe_ypos+=20;
	tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Width",TN_END);
	sprintf(pe_string,"%d",wprops.width);
	pe_widthtb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);

	pe_ypos+=20;
	tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"X Position",TN_END);
	sprintf(pe_string,"%d",wprops.posx);
	pe_xpostb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);

	pe_ypos+=20;
	tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Y Position",TN_END);
	sprintf(pe_string,"%d",wprops.posy);
	pe_ypostb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);


	pe_ypos+=20;
	tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Enabled",TN_END);
	pe_enabledcb=tnCreateWidget(TN_COMBOBOX,propeditwin,85,pe_ypos,TN_LISTITEMS,combostrings,TN_COUNT,2,TN_END);
	tnSetSelectedComboItem(pe_enabledcb,(record->enabled)?"TN_TRUE":"TN_FALSE",TN_TRUE);
	
	pe_ypos+=20;
	tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Visible",TN_END);
	pe_visiblecb=tnCreateWidget(TN_COMBOBOX,propeditwin,85,pe_ypos,TN_LISTITEMS,combostrings,TN_COUNT,2,TN_END);
	tnSetSelectedComboItem(pe_visiblecb,(record->visible)?"TN_TRUE":"TN_FALSE",TN_TRUE);
	pe_ypos+=20;
	okbutton=tnCreateWidget(TN_BUTTON,propeditwin,20,250,TN_CAPTION,"OK",TN_END);
	cancelbutton=tnCreateWidget(TN_BUTTON,propeditwin,100,250,TN_CAPTION,"Cancel",TN_END);
	tnRegisterCallBack(cancelbutton,CLICKED,on_pe_cancel_clicked,NULL);
	tnRegisterCallBack(okbutton,CLICKED,on_pe_ok_clicked,NULL);

	(*PropEditFunc[record->widget->type]) (TND_CREATE);
	
}
	

