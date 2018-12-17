#include "main.h"
char *RadioButtonTagStrings[RADIOBUTTON_PROPS]={"Caption"};
TN_WIDGET *pe_radiobutton_captiontb;
void displayradiobuttonprops(TN_WIDGET *widget)
{
	int i;
	char caption[100];
	
	for(i=0;i<RADIOBUTTON_PROPS;++i)
		tnAddItemToListBox(proptag,RadioButtonTagStrings[i]);
	
 	tnGetRadioButtonCaption(widget,caption);
	tnAddItemToListBox(propval,caption);
	
	tnRegisterCallBack(proptag,CLICKED,RadioButtonListBoxHandler,NULL);
}

void RadioButtonListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for radiobuttongroup properties*/
}
void radiobuttonPropEditFunc(int operation)
{
	char *string;
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Caption",TN_END);
			tnGetRadioButtonCaption(active_widget,pe_string);
			pe_radiobutton_captiontb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);
			pe_ypos+=20;
			break;
	
		case TND_UPDATE:
			
			string=tnGetText(pe_radiobutton_captiontb);
			tnSetRadioButtonCaption(active_widget,string);
			free(string);

	}
}


void radiobutton_write(RECORD *record,FILE *fp,int operation)
{
	char string[100];
	tnGetRadioButtonCaption(record->widget,string);
	
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

