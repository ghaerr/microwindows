#include "main.h"
char *RadioButtonGroupTagStrings[RADIOBUTTONGROUP_PROPS]={"Caption"};
TN_WIDGET *pe_radiobuttongroup_captiontb;
void displayradiobuttongroupprops(TN_WIDGET *widget)
{
	int i;
	char caption[100];
	
	for(i=0;i<RADIOBUTTONGROUP_PROPS;++i)
		tnAddItemToListBox(proptag,RadioButtonGroupTagStrings[i]);
	
 	tnGetRadioButtonGroupCaption(widget,caption);
	tnAddItemToListBox(propval,caption);
	
	tnRegisterCallBack(proptag,CLICKED,RadioButtonGroupListBoxHandler,NULL);
}

void RadioButtonGroupListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for radiobuttongroup properties*/
}
void radiobuttongroupPropEditFunc(int operation)
{
	char *string;
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Caption",TN_END);
			tnGetRadioButtonGroupCaption(active_widget,pe_string);
			pe_radiobuttongroup_captiontb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);
			pe_ypos+=20;
			break;
		case TND_UPDATE:
			string=tnGetText(pe_radiobuttongroup_captiontb);
			tnSetRadioButtonGroupCaption(active_widget,string);
			free(string);
	}
}


void radiobuttongroup_write(RECORD *record,FILE *fp,int operation)
{
	char string[100];
	tnGetRadioButtonGroupCaption(record->widget,string);
	
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"caption \"%s\"\n",string);
			break;
		case TND_CODEGEN:
			fputs(",TN_CAPTION,\"",fp);
			fputs(string,fp);
			fputs("\"",fp);
	}
}

