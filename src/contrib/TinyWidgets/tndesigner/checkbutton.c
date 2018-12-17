#include "main.h"
char *CheckButtonTagStrings[CHECKBUTTON_PROPS] = {"Caption"};
TN_WIDGET *pe_checkbutton_captiontb;

void displaycheckbuttonprops(TN_WIDGET *widget)
{
	/*TODO: add code for showing checkbutton properties here*/
	int i;
	char string[80];
	tnSetCheckButtonStatus(widget,0);
	for(i=0; i < CHECKBUTTON_PROPS;++i)
		tnAddItemToListBox(proptag,CheckButtonTagStrings[i]);

	tnGetCheckButtonCaption(widget,string);
	tnAddItemToListBox(propval,string);

	tnRegisterCallBack(proptag,CLICKED,CheckButtonListBoxHandler,NULL);
		
}
void CheckButtonListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for checkbutton properties*/
}
void checkbuttonPropEditFunc(int operation)
{
	char caption[80];
	char *string;
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Caption",TN_END);
			tnGetCheckButtonCaption(active_widget,caption);
			pe_checkbutton_captiontb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,caption,TN_END);
			break;
			
		case TND_UPDATE:
			string=tnGetText(pe_checkbutton_captiontb);
			tnSetCheckButtonCaption(active_widget,string);
			free(string);
	}
}


void checkbutton_write(RECORD *record,FILE *fp,int operation)
{
	char caption[80];
	tnGetCheckButtonCaption(record->widget,caption);
	
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"caption \"%s\"\n",caption);
			break;
		case TND_CODEGEN:
			fputs(",TN_CAPTION,\"",fp);
			fputs(caption,fp);
			fputs("\"",fp);
			break;
			
	}
}

